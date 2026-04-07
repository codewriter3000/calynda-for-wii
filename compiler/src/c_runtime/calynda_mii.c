/*
 * calynda_mii.c — Mii Channel data package implementation.
 *
 * Wraps libmii so Mii data can be read from Calynda via
 *   import io.mii;  mii.load();  mii.name(0);  etc.
 *
 * On non-Wii builds (host) the functions are stubs that print
 * diagnostics so the compiler test-suite still links.
 */

#include "calynda_mii.h"

#include <stdio.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include "mii.h"
#include <malloc.h>
#include <ogc/lwp_queue.h>
#include <ogc/video.h>
#include <wiiuse/wpad.h>
#include <wiiuse/wiiuse.h>
#endif

/* ------------------------------------------------------------------ */
/* Package object                                                       */
/* ------------------------------------------------------------------ */

CalyndaRtPackage __calynda_pkg_mii = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_PACKAGE },
    "mii"
};

/* ------------------------------------------------------------------ */
/* Internal state                                                       */
/* ------------------------------------------------------------------ */

#ifdef CALYNDA_WII_BUILD
static Mii *loaded_miis = NULL;
static int   mii_count  = 0;

/* Wiimote EEPROM Mii block: offset 0x0FCA, 752 bytes, up to 10 Miis.
 * Block = 750 data bytes + 2 CRC bytes.  Data layout varies:
 *   RNCD(4) + parade_mask(2) + padding(2) + 10×Mii(740) + CRC(2)
 * Some EEPROMs omit the padding, so we auto-detect the header size
 * by trying offsets 4, 6, and 8 and picking the first that yields
 * a non-empty Mii name.  This handles all known format variants. */
#define WIIMOTE_MII_OFFSET  0x0FCA
#define WIIMOTE_MII_SIZE    0x02F0
#define WIIMOTE_MII_MAX     10

/* Internal wiimote_t** pointer from WPAD — not in public headers but
 * is a global BSS symbol in libwiiuse.a, linkable by extern decl.
 * Must be declared as pointer-to-pointer (not array) to match the
 * actual definition: static wiimote **__wpads = NULL; in wpad.c */
extern struct wiimote_t **__wpads;

/* Synchronization for the async EEPROM read. */
static volatile int  _mii_read_done = 0;

static void _mii_eeprom_read_cb(struct wiimote_t *wm, ubyte *data, uword len) {
    (void)wm; (void)data; (void)len;
    _mii_read_done = 1;
}

/* Read Mii data from a connected Wii Remote's EEPROM.
 * Replaces loaded_miis/mii_count with the remote's Miis. */
static int load_miis_from_remote(int chan) {
    static Mii remote_miis[WIIMOTE_MII_MAX];
    unsigned char *buf;
    int i, count;
    u32 probe_type;

    if (chan < 0 || chan > 3) return 0;

    /* Ensure WPAD is initialised — safe to call multiple times. */
    WPAD_Init();

    /* Wait for the controller to finish connecting (up to ~3 s). */
    { int wait = 180;
      while (WPAD_Probe(chan, &probe_type) != 0 && wait-- > 0) {
          WPAD_ScanPads();
          VIDEO_WaitVSync();
      }
    }
    if (WPAD_Probe(chan, &probe_type) != 0) {
        printf("libmii: no remote on chan %d\n", chan);
        return 0;
    }

    buf = (unsigned char *)memalign(32, WIIMOTE_MII_SIZE);
    if (!buf) return 0;
    memset(buf, 0, WIIMOTE_MII_SIZE);

    _mii_read_done = 0;
    if (wiiuse_read_data(__wpads[chan], buf, WIIMOTE_MII_OFFSET,
                         WIIMOTE_MII_SIZE, _mii_eeprom_read_cb) == 0) {
        printf("libmii: wiiuse_read_data failed on chan %d\n", chan);
        free(buf);
        return 0;
    }

    /* Busy-wait until the async read completes (pumps BT events). */
    { int timeout = 300; /* ~5 seconds at 60 fps */
      while (!_mii_read_done && timeout-- > 0) {
          WPAD_ScanPads();
          VIDEO_WaitVSync();
      }
    }
    if (!_mii_read_done) {
        printf("libmii: EEPROM read timed out on chan %d\n", chan);
        free(buf);
        return 0;
    }

    printf("libmii: EEPROM read OK, magic=%c%c%c%c, first 32 bytes:",
           buf[0], buf[1], buf[2], buf[3]);
    for (i = 0; i < 32 && i < WIIMOTE_MII_SIZE; i++)
        printf(" %02x", buf[i]);
    printf("\n");

    /* Auto-detect the header size before the first Mii entry.
     * Known variants: 4 (RNCD only), 6 (RNCD+parade), 8 (RNCD+parade+pad).
     * Try each and pick the first that yields at least one non-empty Mii. */
    { static const int hdr_candidates[] = { 4, 6, 8 };
      int best_hdr = 4, best_count = 0, h;
      for (h = 0; h < 3; h++) {
          int hdr = hdr_candidates[h];
          int n = 0;
          for (i = 0; i < WIIMOTE_MII_MAX; i++) {
              int off = hdr + i * MII_SIZE;
              if (off + MII_SIZE > WIIMOTE_MII_SIZE) break;
              /* Quick check: low byte of first UTF-16BE name char. */
              if (buf[off + 0x02 + 1] != 0) n++;
          }
          if (n > best_count) { best_count = n; best_hdr = hdr; }
      }
      printf("libmii: auto-detected header=%d (%d candidate miis)\n",
             best_hdr, best_count);

      count = 0;
      for (i = 0; i < WIIMOTE_MII_MAX; i++) {
          int offset = best_hdr + i * MII_SIZE;
          if (offset + MII_SIZE > WIIMOTE_MII_SIZE) break;
          Mii m = loadMii_from_raw(offset, (char *)buf);
          printf("libmii: slot %d @%d: inv=%d name[0]=0x%02x name='%s'\n",
                 i, offset, m.invalid, (unsigned char)m.name[0], m.name);
          if (m.name[0] != '\0') {
              remote_miis[count++] = m;
          }
      }
    }

    free(buf);
    loaded_miis = remote_miis;
    mii_count = count;
    return count;
}
#endif

/* ------------------------------------------------------------------ */
/* Extern callable enum values                                          */
/* ------------------------------------------------------------------ */

enum {
    MII_CALL_LOAD = 200,
    MII_CALL_COUNT,
    MII_CALL_NAME,
    MII_CALL_CREATOR,
    MII_CALL_IS_FEMALE,
    MII_CALL_IS_FAVORITE,
    MII_CALL_HEIGHT,
    MII_CALL_WEIGHT,
    MII_CALL_FAV_COLOR,
    MII_CALL_FACE_SHAPE,
    MII_CALL_SKIN_COLOR,
    MII_CALL_FACIAL_FEATURE,
    MII_CALL_HAIR_TYPE,
    MII_CALL_HAIR_COLOR,
    MII_CALL_HAIR_PART,
    MII_CALL_EYE_TYPE,
    MII_CALL_EYE_COLOR,
    MII_CALL_EYEBROW_TYPE,
    MII_CALL_EYEBROW_COLOR,
    MII_CALL_NOSE_TYPE,
    MII_CALL_LIP_TYPE,
    MII_CALL_LIP_COLOR,
    MII_CALL_GLASSES_TYPE,
    MII_CALL_GLASSES_COLOR,
    MII_CALL_MUSTACHE_TYPE,
    MII_CALL_BEARD_TYPE,
    MII_CALL_FACIAL_HAIR_COLOR,
    MII_CALL_HAS_MOLE,
    MII_CALL_LOAD_REMOTE,
    /* Color constants */
    MII_CALL_RED,
    MII_CALL_ORANGE,
    MII_CALL_YELLOW,
    MII_CALL_LIGHT_GREEN,
    MII_CALL_GREEN,
    MII_CALL_BLUE,
    MII_CALL_LIGHT_BLUE,
    MII_CALL_PINK,
    MII_CALL_PURPLE,
    MII_CALL_BROWN,
    MII_CALL_WHITE,
    MII_CALL_BLACK
};

/* ------------------------------------------------------------------ */
/* Callable objects (one per exposed function)                          */
/* ------------------------------------------------------------------ */

#define MII_CALLABLE(var, id, label) \
    static CalyndaRtExternCallable var = { \
        { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE }, \
        (CalyndaRtExternCallableKind)(id), \
        label \
    }

/* Core functions */
MII_CALLABLE(MII_LOAD_CALLABLE,    MII_CALL_LOAD,    "load");
MII_CALLABLE(MII_COUNT_CALLABLE,   MII_CALL_COUNT,   "count");
MII_CALLABLE(MII_NAME_CALLABLE,    MII_CALL_NAME,    "name");
MII_CALLABLE(MII_CREATOR_CALLABLE, MII_CALL_CREATOR, "creator");

/* Basic info getters */
MII_CALLABLE(MII_IS_FEMALE_CALLABLE,   MII_CALL_IS_FEMALE,   "is_female");
MII_CALLABLE(MII_IS_FAVORITE_CALLABLE, MII_CALL_IS_FAVORITE, "is_favorite");
MII_CALLABLE(MII_HEIGHT_CALLABLE,      MII_CALL_HEIGHT,      "height");
MII_CALLABLE(MII_WEIGHT_CALLABLE,      MII_CALL_WEIGHT,      "weight");
MII_CALLABLE(MII_FAV_COLOR_CALLABLE,   MII_CALL_FAV_COLOR,   "fav_color");

/* Face/body attribute getters */
MII_CALLABLE(MII_FACE_SHAPE_CALLABLE,      MII_CALL_FACE_SHAPE,      "face_shape");
MII_CALLABLE(MII_SKIN_COLOR_CALLABLE,      MII_CALL_SKIN_COLOR,      "skin_color");
MII_CALLABLE(MII_FACIAL_FEATURE_CALLABLE,  MII_CALL_FACIAL_FEATURE,  "facial_feature");
MII_CALLABLE(MII_HAIR_TYPE_CALLABLE,       MII_CALL_HAIR_TYPE,       "hair_type");
MII_CALLABLE(MII_HAIR_COLOR_CALLABLE,      MII_CALL_HAIR_COLOR,      "hair_color");
MII_CALLABLE(MII_HAIR_PART_CALLABLE,       MII_CALL_HAIR_PART,       "hair_part");
MII_CALLABLE(MII_EYE_TYPE_CALLABLE,        MII_CALL_EYE_TYPE,        "eye_type");
MII_CALLABLE(MII_EYE_COLOR_CALLABLE,       MII_CALL_EYE_COLOR,       "eye_color");
MII_CALLABLE(MII_EYEBROW_TYPE_CALLABLE,    MII_CALL_EYEBROW_TYPE,    "eyebrow_type");
MII_CALLABLE(MII_EYEBROW_COLOR_CALLABLE,   MII_CALL_EYEBROW_COLOR,   "eyebrow_color");
MII_CALLABLE(MII_NOSE_TYPE_CALLABLE,       MII_CALL_NOSE_TYPE,       "nose_type");
MII_CALLABLE(MII_LIP_TYPE_CALLABLE,        MII_CALL_LIP_TYPE,        "lip_type");
MII_CALLABLE(MII_LIP_COLOR_CALLABLE,       MII_CALL_LIP_COLOR,       "lip_color");
MII_CALLABLE(MII_GLASSES_TYPE_CALLABLE,    MII_CALL_GLASSES_TYPE,    "glasses_type");
MII_CALLABLE(MII_GLASSES_COLOR_CALLABLE,   MII_CALL_GLASSES_COLOR,   "glasses_color");
MII_CALLABLE(MII_MUSTACHE_TYPE_CALLABLE,   MII_CALL_MUSTACHE_TYPE,   "mustache_type");
MII_CALLABLE(MII_BEARD_TYPE_CALLABLE,      MII_CALL_BEARD_TYPE,      "beard_type");
MII_CALLABLE(MII_FACIAL_HAIR_COLOR_CALLABLE, MII_CALL_FACIAL_HAIR_COLOR, "facial_hair_color");
MII_CALLABLE(MII_HAS_MOLE_CALLABLE,       MII_CALL_HAS_MOLE,        "has_mole");
MII_CALLABLE(MII_LOAD_REMOTE_CALLABLE,    MII_CALL_LOAD_REMOTE,     "load_remote");

/* Color constants */
MII_CALLABLE(MII_RED_CALLABLE,         MII_CALL_RED,         "RED");
MII_CALLABLE(MII_ORANGE_CALLABLE,      MII_CALL_ORANGE,      "ORANGE");
MII_CALLABLE(MII_YELLOW_CALLABLE,      MII_CALL_YELLOW,      "YELLOW");
MII_CALLABLE(MII_LIGHT_GREEN_CALLABLE, MII_CALL_LIGHT_GREEN, "LIGHT_GREEN");
MII_CALLABLE(MII_GREEN_CALLABLE,       MII_CALL_GREEN,       "GREEN");
MII_CALLABLE(MII_BLUE_CALLABLE,        MII_CALL_BLUE,        "BLUE");
MII_CALLABLE(MII_LIGHT_BLUE_CALLABLE,  MII_CALL_LIGHT_BLUE,  "LIGHT_BLUE");
MII_CALLABLE(MII_PINK_CALLABLE,        MII_CALL_PINK,        "PINK");
MII_CALLABLE(MII_PURPLE_CALLABLE,      MII_CALL_PURPLE,      "PURPLE");
MII_CALLABLE(MII_BROWN_CALLABLE,       MII_CALL_BROWN,       "BROWN");
MII_CALLABLE(MII_WHITE_CALLABLE,       MII_CALL_WHITE,       "WHITE");
MII_CALLABLE(MII_BLACK_CALLABLE,       MII_CALL_BLACK,       "BLACK");

#undef MII_CALLABLE

/* Flat list for registration. */
static CalyndaRtExternCallable *ALL_MII_CALLABLES[] = {
    &MII_LOAD_CALLABLE,
    &MII_COUNT_CALLABLE,
    &MII_NAME_CALLABLE,
    &MII_CREATOR_CALLABLE,
    &MII_IS_FEMALE_CALLABLE,
    &MII_IS_FAVORITE_CALLABLE,
    &MII_HEIGHT_CALLABLE,
    &MII_WEIGHT_CALLABLE,
    &MII_FAV_COLOR_CALLABLE,
    &MII_FACE_SHAPE_CALLABLE,
    &MII_SKIN_COLOR_CALLABLE,
    &MII_FACIAL_FEATURE_CALLABLE,
    &MII_HAIR_TYPE_CALLABLE,
    &MII_HAIR_COLOR_CALLABLE,
    &MII_HAIR_PART_CALLABLE,
    &MII_EYE_TYPE_CALLABLE,
    &MII_EYE_COLOR_CALLABLE,
    &MII_EYEBROW_TYPE_CALLABLE,
    &MII_EYEBROW_COLOR_CALLABLE,
    &MII_NOSE_TYPE_CALLABLE,
    &MII_LIP_TYPE_CALLABLE,
    &MII_LIP_COLOR_CALLABLE,
    &MII_GLASSES_TYPE_CALLABLE,
    &MII_GLASSES_COLOR_CALLABLE,
    &MII_MUSTACHE_TYPE_CALLABLE,
    &MII_BEARD_TYPE_CALLABLE,
    &MII_FACIAL_HAIR_COLOR_CALLABLE,
    &MII_HAS_MOLE_CALLABLE,
    &MII_LOAD_REMOTE_CALLABLE,
    &MII_RED_CALLABLE,
    &MII_ORANGE_CALLABLE,
    &MII_YELLOW_CALLABLE,
    &MII_LIGHT_GREEN_CALLABLE,
    &MII_GREEN_CALLABLE,
    &MII_BLUE_CALLABLE,
    &MII_LIGHT_BLUE_CALLABLE,
    &MII_PINK_CALLABLE,
    &MII_PURPLE_CALLABLE,
    &MII_BROWN_CALLABLE,
    &MII_WHITE_CALLABLE,
    &MII_BLACK_CALLABLE,
};
#define MII_CALLABLE_COUNT \
    (sizeof(ALL_MII_CALLABLES) / sizeof(ALL_MII_CALLABLES[0]))

/* ------------------------------------------------------------------ */
/* Object registration                                                  */
/* ------------------------------------------------------------------ */

void calynda_mii_register_objects(void) {
    size_t i;

    calynda_rt_register_static_object(&__calynda_pkg_mii.header);
    for (i = 0; i < MII_CALLABLE_COUNT; i++) {
        calynda_rt_register_static_object(
            &ALL_MII_CALLABLES[i]->header);
    }
}

/* ------------------------------------------------------------------ */
/* Dispatch                                                             */
/* ------------------------------------------------------------------ */

CalyndaRtWord calynda_mii_dispatch(const CalyndaRtExternCallable *callable,
                                   size_t argument_count,
                                   const CalyndaRtWord *arguments) {
    (void)argument_count;
    (void)arguments;

    switch ((int)callable->kind) {

    /* ---- Core functions ---- */
    case MII_CALL_LOAD:
#ifdef CALYNDA_WII_BUILD
        loaded_miis = loadMiis_Wii();
        if (loaded_miis) {
            /* Count only valid, non-empty slots and compact them to
             * the front of the array so index 0..mii_count-1 are all
             * real Miis.  The 'invalid' flag (bit 7 of byte 0) marks
             * unused/blank slots in the RNOD database. */
            int i;
            mii_count = 0;
            for (i = 0; i < MII_MAX; i++) {
                if (!loaded_miis[i].invalid && loaded_miis[i].name[0] != '\0') {
                    if (mii_count != i) {
                        loaded_miis[mii_count] = loaded_miis[i];
                    }
                    mii_count++;
                }
            }
        }
        return (CalyndaRtWord)(loaded_miis && mii_count > 0 ? 1 : 0);
#else
        fprintf(stderr, "[mii.load] stub on host\n");
        return 0;
#endif

    case MII_CALL_COUNT:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)mii_count;
#else
        fprintf(stderr, "[mii.count] stub on host\n");
        return 0;
#endif

    case MII_CALL_NAME: {
#ifdef CALYNDA_WII_BUILD
        int idx = argument_count > 0 ? (int)arguments[0] : 0;
        if (!loaded_miis || idx < 0 || idx >= mii_count) return 0;
        return calynda_rt_make_string_copy(loaded_miis[idx].name);
#else
        fprintf(stderr, "[mii.name] stub on host\n");
        return calynda_rt_make_string_copy("StubMii");
#endif
    }

    case MII_CALL_CREATOR: {
#ifdef CALYNDA_WII_BUILD
        int idx = argument_count > 0 ? (int)arguments[0] : 0;
        if (!loaded_miis || idx < 0 || idx >= mii_count) return 0;
        return calynda_rt_make_string_copy(
            loaded_miis[idx].creator);
#else
        fprintf(stderr, "[mii.creator] stub on host\n");
        return calynda_rt_make_string_copy("StubCreator");
#endif
    }

    /* ---- Integer field getters (all take index) ---- */
#ifdef CALYNDA_WII_BUILD
#define MII_INT_GETTER(call_id, field_name) \
    case call_id: { \
        int idx = argument_count > 0 ? (int)arguments[0] : 0; \
        if (!loaded_miis || idx < 0 || idx >= mii_count) return 0; \
        return (CalyndaRtWord)loaded_miis[idx].field_name; \
    }
#else
#define MII_INT_GETTER(call_id, field_name) \
    case call_id: \
        fprintf(stderr, "[mii." #field_name "] stub on host\n"); \
        return 0;
#endif

    MII_INT_GETTER(MII_CALL_IS_FEMALE,         female)
    MII_INT_GETTER(MII_CALL_IS_FAVORITE,       favorite)
    MII_INT_GETTER(MII_CALL_HEIGHT,            height)
    MII_INT_GETTER(MII_CALL_WEIGHT,            weight)
    MII_INT_GETTER(MII_CALL_FAV_COLOR,         favColor)
    MII_INT_GETTER(MII_CALL_FACE_SHAPE,        faceShape)
    MII_INT_GETTER(MII_CALL_SKIN_COLOR,        skinColor)
    MII_INT_GETTER(MII_CALL_FACIAL_FEATURE,    facialFeature)
    MII_INT_GETTER(MII_CALL_HAIR_TYPE,         hairType)
    MII_INT_GETTER(MII_CALL_HAIR_COLOR,        hairColor)
    MII_INT_GETTER(MII_CALL_HAIR_PART,         hairPart)
    MII_INT_GETTER(MII_CALL_EYE_TYPE,          eyeType)
    MII_INT_GETTER(MII_CALL_EYE_COLOR,         eyeColor)
    MII_INT_GETTER(MII_CALL_EYEBROW_TYPE,      eyebrowType)
    MII_INT_GETTER(MII_CALL_EYEBROW_COLOR,     eyebrowColor)
    MII_INT_GETTER(MII_CALL_NOSE_TYPE,         noseType)
    MII_INT_GETTER(MII_CALL_LIP_TYPE,          lipType)
    MII_INT_GETTER(MII_CALL_LIP_COLOR,         lipColor)
    MII_INT_GETTER(MII_CALL_GLASSES_TYPE,      glassesType)
    MII_INT_GETTER(MII_CALL_GLASSES_COLOR,     glassesColor)
    MII_INT_GETTER(MII_CALL_MUSTACHE_TYPE,     mustacheType)
    MII_INT_GETTER(MII_CALL_BEARD_TYPE,        beardType)
    MII_INT_GETTER(MII_CALL_FACIAL_HAIR_COLOR, facialHairColor)
    MII_INT_GETTER(MII_CALL_HAS_MOLE,          mole)

#undef MII_INT_GETTER

    /* ---- Load from Wii Remote EEPROM ---- */
    case MII_CALL_LOAD_REMOTE:
#ifdef CALYNDA_WII_BUILD
    {
        int chan = argument_count > 0 ? (int)arguments[0] : 0;
        return (CalyndaRtWord)load_miis_from_remote(chan);
    }
#else
        fprintf(stderr, "[mii.load_remote] stub on host\n");
        return 0;
#endif

    /* ---- Color constants (zero-argument, return the value) ---- */
    case MII_CALL_RED:         return 0;
    case MII_CALL_ORANGE:      return 1;
    case MII_CALL_YELLOW:      return 2;
    case MII_CALL_LIGHT_GREEN: return 3;
    case MII_CALL_GREEN:       return 4;
    case MII_CALL_BLUE:        return 5;
    case MII_CALL_LIGHT_BLUE:  return 6;
    case MII_CALL_PINK:        return 7;
    case MII_CALL_PURPLE:      return 8;
    case MII_CALL_BROWN:       return 9;
    case MII_CALL_WHITE:       return 10;
    case MII_CALL_BLACK:       return 11;
    }

    fprintf(stderr, "runtime: unsupported mii callable kind %d\n",
            (int)callable->kind);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Member load — called by __calynda_rt_member_load                     */
/* ------------------------------------------------------------------ */

static CalyndaRtWord rt_make_obj(void *p) {
    return (CalyndaRtWord)(uintptr_t)p;
}

CalyndaRtWord calynda_mii_member_load(const char *member) {
    if (!member) return 0;

    /* Core functions */
    if (strcmp(member, "load")        == 0) return rt_make_obj(&MII_LOAD_CALLABLE);
    if (strcmp(member, "load_remote") == 0) return rt_make_obj(&MII_LOAD_REMOTE_CALLABLE);
    if (strcmp(member, "count")       == 0) return rt_make_obj(&MII_COUNT_CALLABLE);
    if (strcmp(member, "name")    == 0) return rt_make_obj(&MII_NAME_CALLABLE);
    if (strcmp(member, "creator") == 0) return rt_make_obj(&MII_CREATOR_CALLABLE);

    /* Basic info */
    if (strcmp(member, "is_female")   == 0) return rt_make_obj(&MII_IS_FEMALE_CALLABLE);
    if (strcmp(member, "is_favorite") == 0) return rt_make_obj(&MII_IS_FAVORITE_CALLABLE);
    if (strcmp(member, "height")      == 0) return rt_make_obj(&MII_HEIGHT_CALLABLE);
    if (strcmp(member, "weight")      == 0) return rt_make_obj(&MII_WEIGHT_CALLABLE);
    if (strcmp(member, "fav_color")   == 0) return rt_make_obj(&MII_FAV_COLOR_CALLABLE);

    /* Face/body attributes */
    if (strcmp(member, "face_shape")      == 0) return rt_make_obj(&MII_FACE_SHAPE_CALLABLE);
    if (strcmp(member, "skin_color")      == 0) return rt_make_obj(&MII_SKIN_COLOR_CALLABLE);
    if (strcmp(member, "facial_feature")  == 0) return rt_make_obj(&MII_FACIAL_FEATURE_CALLABLE);
    if (strcmp(member, "hair_type")       == 0) return rt_make_obj(&MII_HAIR_TYPE_CALLABLE);
    if (strcmp(member, "hair_color")      == 0) return rt_make_obj(&MII_HAIR_COLOR_CALLABLE);
    if (strcmp(member, "hair_part")       == 0) return rt_make_obj(&MII_HAIR_PART_CALLABLE);
    if (strcmp(member, "eye_type")        == 0) return rt_make_obj(&MII_EYE_TYPE_CALLABLE);
    if (strcmp(member, "eye_color")       == 0) return rt_make_obj(&MII_EYE_COLOR_CALLABLE);
    if (strcmp(member, "eyebrow_type")    == 0) return rt_make_obj(&MII_EYEBROW_TYPE_CALLABLE);
    if (strcmp(member, "eyebrow_color")   == 0) return rt_make_obj(&MII_EYEBROW_COLOR_CALLABLE);
    if (strcmp(member, "nose_type")       == 0) return rt_make_obj(&MII_NOSE_TYPE_CALLABLE);
    if (strcmp(member, "lip_type")        == 0) return rt_make_obj(&MII_LIP_TYPE_CALLABLE);
    if (strcmp(member, "lip_color")       == 0) return rt_make_obj(&MII_LIP_COLOR_CALLABLE);
    if (strcmp(member, "glasses_type")    == 0) return rt_make_obj(&MII_GLASSES_TYPE_CALLABLE);
    if (strcmp(member, "glasses_color")   == 0) return rt_make_obj(&MII_GLASSES_COLOR_CALLABLE);
    if (strcmp(member, "mustache_type")   == 0) return rt_make_obj(&MII_MUSTACHE_TYPE_CALLABLE);
    if (strcmp(member, "beard_type")      == 0) return rt_make_obj(&MII_BEARD_TYPE_CALLABLE);
    if (strcmp(member, "facial_hair_color") == 0) return rt_make_obj(&MII_FACIAL_HAIR_COLOR_CALLABLE);
    if (strcmp(member, "has_mole")        == 0) return rt_make_obj(&MII_HAS_MOLE_CALLABLE);

    /* Color constants */
    if (strcmp(member, "RED")         == 0) return rt_make_obj(&MII_RED_CALLABLE);
    if (strcmp(member, "ORANGE")      == 0) return rt_make_obj(&MII_ORANGE_CALLABLE);
    if (strcmp(member, "YELLOW")      == 0) return rt_make_obj(&MII_YELLOW_CALLABLE);
    if (strcmp(member, "LIGHT_GREEN") == 0) return rt_make_obj(&MII_LIGHT_GREEN_CALLABLE);
    if (strcmp(member, "GREEN")       == 0) return rt_make_obj(&MII_GREEN_CALLABLE);
    if (strcmp(member, "BLUE")        == 0) return rt_make_obj(&MII_BLUE_CALLABLE);
    if (strcmp(member, "LIGHT_BLUE")  == 0) return rt_make_obj(&MII_LIGHT_BLUE_CALLABLE);
    if (strcmp(member, "PINK")        == 0) return rt_make_obj(&MII_PINK_CALLABLE);
    if (strcmp(member, "PURPLE")      == 0) return rt_make_obj(&MII_PURPLE_CALLABLE);
    if (strcmp(member, "BROWN")       == 0) return rt_make_obj(&MII_BROWN_CALLABLE);
    if (strcmp(member, "WHITE")       == 0) return rt_make_obj(&MII_WHITE_CALLABLE);
    if (strcmp(member, "BLACK")       == 0) return rt_make_obj(&MII_BLACK_CALLABLE);

    fprintf(stderr, "runtime: mii has no member '%s'\n", member);
    return 0;
}

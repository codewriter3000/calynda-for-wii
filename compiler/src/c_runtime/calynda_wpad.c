/*
 * calynda_wpad.c — Wii Remote (WPAD) package implementation.
 *
 * Wraps libogc's WPAD functions so they can be called from Calynda
 * via  import io.wpad;  wpad.init();  etc.
 *
 * On non-Wii builds (host) the functions are stubs that print
 * diagnostics so the compiler test-suite still links.
 */

#include "calynda_wpad.h"

#include <stdio.h>
#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include <ogc/lwp_queue.h>
#include <wiiuse/wpad.h>
#endif

/* ------------------------------------------------------------------ */
/* Package object                                                       */
/* ------------------------------------------------------------------ */

CalyndaRtPackage __calynda_pkg_wpad = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_PACKAGE },
    "wpad"
};

/* ------------------------------------------------------------------ */
/* Extern callable enum values (local to this file)                     */
/* ------------------------------------------------------------------ */

enum {
    WPAD_CALL_INIT = 100,
    WPAD_CALL_SCAN,
    WPAD_CALL_BUTTONS_DOWN,
    WPAD_CALL_BUTTONS_HELD,
    WPAD_CALL_BUTTONS_UP,
    WPAD_CALL_RUMBLE,
    WPAD_CALL_PROBE,
    /* Button constant helpers */
    WPAD_CALL_BUTTON_A,
    WPAD_CALL_BUTTON_B,
    WPAD_CALL_BUTTON_1,
    WPAD_CALL_BUTTON_2,
    WPAD_CALL_BUTTON_UP,
    WPAD_CALL_BUTTON_DOWN,
    WPAD_CALL_BUTTON_LEFT,
    WPAD_CALL_BUTTON_RIGHT,
    WPAD_CALL_BUTTON_PLUS,
    WPAD_CALL_BUTTON_MINUS,
    WPAD_CALL_BUTTON_HOME,
    /* Utility */
    WPAD_CALL_HAS_BUTTON
};

/* ------------------------------------------------------------------ */
/* Callable objects (one per exposed function)                          */
/* ------------------------------------------------------------------ */

#define WPAD_CALLABLE(var, id, label) \
    static CalyndaRtExternCallable var = { \
        { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE }, \
        (CalyndaRtExternCallableKind)(id), \
        label \
    }

WPAD_CALLABLE(WPAD_INIT_CALLABLE,          WPAD_CALL_INIT,          "init");
WPAD_CALLABLE(WPAD_SCAN_CALLABLE,          WPAD_CALL_SCAN,          "scan");
WPAD_CALLABLE(WPAD_BUTTONS_DOWN_CALLABLE,  WPAD_CALL_BUTTONS_DOWN,  "buttons_down");
WPAD_CALLABLE(WPAD_BUTTONS_HELD_CALLABLE,  WPAD_CALL_BUTTONS_HELD,  "buttons_held");
WPAD_CALLABLE(WPAD_BUTTONS_UP_CALLABLE,    WPAD_CALL_BUTTONS_UP,    "buttons_up");
WPAD_CALLABLE(WPAD_RUMBLE_CALLABLE,        WPAD_CALL_RUMBLE,        "rumble");
WPAD_CALLABLE(WPAD_PROBE_CALLABLE,         WPAD_CALL_PROBE,         "probe");
WPAD_CALLABLE(WPAD_BUTTON_A_CALLABLE,      WPAD_CALL_BUTTON_A,      "BUTTON_A");
WPAD_CALLABLE(WPAD_BUTTON_B_CALLABLE,      WPAD_CALL_BUTTON_B,      "BUTTON_B");
WPAD_CALLABLE(WPAD_BUTTON_1_CALLABLE,      WPAD_CALL_BUTTON_1,      "BUTTON_1");
WPAD_CALLABLE(WPAD_BUTTON_2_CALLABLE,      WPAD_CALL_BUTTON_2,      "BUTTON_2");
WPAD_CALLABLE(WPAD_BUTTON_UP_CALLABLE,     WPAD_CALL_BUTTON_UP,     "BUTTON_UP");
WPAD_CALLABLE(WPAD_BUTTON_DOWN_CALLABLE,   WPAD_CALL_BUTTON_DOWN,   "BUTTON_DOWN");
WPAD_CALLABLE(WPAD_BUTTON_LEFT_CALLABLE,   WPAD_CALL_BUTTON_LEFT,   "BUTTON_LEFT");
WPAD_CALLABLE(WPAD_BUTTON_RIGHT_CALLABLE,  WPAD_CALL_BUTTON_RIGHT,  "BUTTON_RIGHT");
WPAD_CALLABLE(WPAD_BUTTON_PLUS_CALLABLE,   WPAD_CALL_BUTTON_PLUS,   "BUTTON_PLUS");
WPAD_CALLABLE(WPAD_BUTTON_MINUS_CALLABLE,  WPAD_CALL_BUTTON_MINUS,  "BUTTON_MINUS");
WPAD_CALLABLE(WPAD_BUTTON_HOME_CALLABLE,   WPAD_CALL_BUTTON_HOME,   "BUTTON_HOME");
WPAD_CALLABLE(WPAD_HAS_BUTTON_CALLABLE,    WPAD_CALL_HAS_BUTTON,    "has_button");

#undef WPAD_CALLABLE

/* Flat list so we can register + recognise them all easily. */
static CalyndaRtExternCallable *ALL_WPAD_CALLABLES[] = {
    &WPAD_INIT_CALLABLE,
    &WPAD_SCAN_CALLABLE,
    &WPAD_BUTTONS_DOWN_CALLABLE,
    &WPAD_BUTTONS_HELD_CALLABLE,
    &WPAD_BUTTONS_UP_CALLABLE,
    &WPAD_RUMBLE_CALLABLE,
    &WPAD_PROBE_CALLABLE,
    &WPAD_BUTTON_A_CALLABLE,
    &WPAD_BUTTON_B_CALLABLE,
    &WPAD_BUTTON_1_CALLABLE,
    &WPAD_BUTTON_2_CALLABLE,
    &WPAD_BUTTON_UP_CALLABLE,
    &WPAD_BUTTON_DOWN_CALLABLE,
    &WPAD_BUTTON_LEFT_CALLABLE,
    &WPAD_BUTTON_RIGHT_CALLABLE,
    &WPAD_BUTTON_PLUS_CALLABLE,
    &WPAD_BUTTON_MINUS_CALLABLE,
    &WPAD_BUTTON_HOME_CALLABLE,
    &WPAD_HAS_BUTTON_CALLABLE,
};
#define WPAD_CALLABLE_COUNT (sizeof(ALL_WPAD_CALLABLES) / sizeof(ALL_WPAD_CALLABLES[0]))

/* ------------------------------------------------------------------ */
/* Object registration                                                  */
/* ------------------------------------------------------------------ */

void calynda_wpad_register_objects(void) {
    size_t i;

    calynda_rt_register_static_object(&__calynda_pkg_wpad.header);
    for (i = 0; i < WPAD_CALLABLE_COUNT; i++) {
        calynda_rt_register_static_object(&ALL_WPAD_CALLABLES[i]->header);
    }
}

/* ------------------------------------------------------------------ */
/* Dispatch                                                             */
/* ------------------------------------------------------------------ */

CalyndaRtWord calynda_wpad_dispatch(const CalyndaRtExternCallable *callable,
                                    size_t argument_count,
                                    const CalyndaRtWord *arguments) {
    (void)argument_count;
    (void)arguments;

    switch ((int)callable->kind) {

    /* ---- Core WPAD functions ---- */
    case WPAD_CALL_INIT:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)WPAD_Init();
#else
        fprintf(stderr, "[wpad.init] stub on host\n");
        return 0;
#endif

    case WPAD_CALL_SCAN:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)WPAD_ScanPads();
#else
        fprintf(stderr, "[wpad.scan] stub on host\n");
        return 0;
#endif

    case WPAD_CALL_BUTTONS_DOWN:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)WPAD_ButtonsDown(
            argument_count > 0 ? (int)arguments[0] : 0);
#else
        fprintf(stderr, "[wpad.buttons_down] stub on host\n");
        return 0;
#endif

    case WPAD_CALL_BUTTONS_HELD:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)WPAD_ButtonsHeld(
            argument_count > 0 ? (int)arguments[0] : 0);
#else
        fprintf(stderr, "[wpad.buttons_held] stub on host\n");
        return 0;
#endif

    case WPAD_CALL_BUTTONS_UP:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)WPAD_ButtonsUp(
            argument_count > 0 ? (int)arguments[0] : 0);
#else
        fprintf(stderr, "[wpad.buttons_up] stub on host\n");
        return 0;
#endif

    case WPAD_CALL_RUMBLE:
#ifdef CALYNDA_WII_BUILD
        return (CalyndaRtWord)WPAD_Rumble(
            argument_count > 0 ? (int)arguments[0] : 0,
            argument_count > 1 ? (int)arguments[1] : 0);
#else
        fprintf(stderr, "[wpad.rumble] stub on host\n");
        return 0;
#endif

    case WPAD_CALL_PROBE: {
#ifdef CALYNDA_WII_BUILD
        u32 type = 0;
        s32 rc = WPAD_Probe(
            argument_count > 0 ? (int)arguments[0] : 0, &type);
        /* Return 0 = connected, non-zero = not connected */
        return (CalyndaRtWord)rc;
#else
        fprintf(stderr, "[wpad.probe] stub on host\n");
        return (CalyndaRtWord)-1;
#endif
    }

    /* ---- Button mask constants (zero-argument, return the mask) ---- */
#ifdef CALYNDA_WII_BUILD
    case WPAD_CALL_BUTTON_A:     return (CalyndaRtWord)WPAD_BUTTON_A;
    case WPAD_CALL_BUTTON_B:     return (CalyndaRtWord)WPAD_BUTTON_B;
    case WPAD_CALL_BUTTON_1:     return (CalyndaRtWord)WPAD_BUTTON_1;
    case WPAD_CALL_BUTTON_2:     return (CalyndaRtWord)WPAD_BUTTON_2;
    case WPAD_CALL_BUTTON_UP:    return (CalyndaRtWord)WPAD_BUTTON_UP;
    case WPAD_CALL_BUTTON_DOWN:  return (CalyndaRtWord)WPAD_BUTTON_DOWN;
    case WPAD_CALL_BUTTON_LEFT:  return (CalyndaRtWord)WPAD_BUTTON_LEFT;
    case WPAD_CALL_BUTTON_RIGHT: return (CalyndaRtWord)WPAD_BUTTON_RIGHT;
    case WPAD_CALL_BUTTON_PLUS:  return (CalyndaRtWord)WPAD_BUTTON_PLUS;
    case WPAD_CALL_BUTTON_MINUS: return (CalyndaRtWord)WPAD_BUTTON_MINUS;
    case WPAD_CALL_BUTTON_HOME:  return (CalyndaRtWord)WPAD_BUTTON_HOME;
#else
    case WPAD_CALL_BUTTON_A:     return 0x0008;
    case WPAD_CALL_BUTTON_B:     return 0x0004;
    case WPAD_CALL_BUTTON_1:     return 0x0002;
    case WPAD_CALL_BUTTON_2:     return 0x0001;
    case WPAD_CALL_BUTTON_UP:    return 0x0800;
    case WPAD_CALL_BUTTON_DOWN:  return 0x0400;
    case WPAD_CALL_BUTTON_LEFT:  return 0x0100;
    case WPAD_CALL_BUTTON_RIGHT: return 0x0200;
    case WPAD_CALL_BUTTON_PLUS:  return 0x1000;
    case WPAD_CALL_BUTTON_MINUS: return 0x0010;
    case WPAD_CALL_BUTTON_HOME:  return 0x0080;
#endif

    /* ---- Utility: has_button(buttons, mask) -> bool (1/0) ---- */
    case WPAD_CALL_HAS_BUTTON: {
        CalyndaRtWord buttons = argument_count > 0 ? arguments[0] : 0;
        CalyndaRtWord mask    = argument_count > 1 ? arguments[1] : 0;
        return (buttons & mask) ? 1 : 0;
    }

    }

    fprintf(stderr, "runtime: unsupported wpad callable kind %d\n",
            (int)callable->kind);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Member load — called by __calynda_rt_member_load                     */
/* ------------------------------------------------------------------ */

static CalyndaRtWord rt_make_obj(void *p) {
    return (CalyndaRtWord)(uintptr_t)p;
}

CalyndaRtWord calynda_wpad_member_load(const char *member) {
    if (!member) return 0;

    if (strcmp(member, "init")         == 0) return rt_make_obj(&WPAD_INIT_CALLABLE);
    if (strcmp(member, "scan")         == 0) return rt_make_obj(&WPAD_SCAN_CALLABLE);
    if (strcmp(member, "buttons_down") == 0) return rt_make_obj(&WPAD_BUTTONS_DOWN_CALLABLE);
    if (strcmp(member, "buttons_held") == 0) return rt_make_obj(&WPAD_BUTTONS_HELD_CALLABLE);
    if (strcmp(member, "buttons_up")   == 0) return rt_make_obj(&WPAD_BUTTONS_UP_CALLABLE);
    if (strcmp(member, "rumble")       == 0) return rt_make_obj(&WPAD_RUMBLE_CALLABLE);
    if (strcmp(member, "probe")        == 0) return rt_make_obj(&WPAD_PROBE_CALLABLE);
    if (strcmp(member, "BUTTON_A")     == 0) return rt_make_obj(&WPAD_BUTTON_A_CALLABLE);
    if (strcmp(member, "BUTTON_B")     == 0) return rt_make_obj(&WPAD_BUTTON_B_CALLABLE);
    if (strcmp(member, "BUTTON_1")     == 0) return rt_make_obj(&WPAD_BUTTON_1_CALLABLE);
    if (strcmp(member, "BUTTON_2")     == 0) return rt_make_obj(&WPAD_BUTTON_2_CALLABLE);
    if (strcmp(member, "BUTTON_UP")    == 0) return rt_make_obj(&WPAD_BUTTON_UP_CALLABLE);
    if (strcmp(member, "BUTTON_DOWN")  == 0) return rt_make_obj(&WPAD_BUTTON_DOWN_CALLABLE);
    if (strcmp(member, "BUTTON_LEFT")  == 0) return rt_make_obj(&WPAD_BUTTON_LEFT_CALLABLE);
    if (strcmp(member, "BUTTON_RIGHT") == 0) return rt_make_obj(&WPAD_BUTTON_RIGHT_CALLABLE);
    if (strcmp(member, "BUTTON_PLUS")  == 0) return rt_make_obj(&WPAD_BUTTON_PLUS_CALLABLE);
    if (strcmp(member, "BUTTON_MINUS") == 0) return rt_make_obj(&WPAD_BUTTON_MINUS_CALLABLE);
    if (strcmp(member, "BUTTON_HOME")  == 0) return rt_make_obj(&WPAD_BUTTON_HOME_CALLABLE);
    if (strcmp(member, "has_button")   == 0) return rt_make_obj(&WPAD_HAS_BUTTON_CALLABLE);

    fprintf(stderr, "runtime: wpad has no member '%s'\n", member);
    return 0;
}

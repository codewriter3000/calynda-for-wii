/*
 * Wii / GameCube disc image loader.
 *
 * Detects GameCube (GCM, unencrypted) and Wii (encrypted) disc images
 * and exposes what can be read without decryption.
 *
 * GameCube discs:
 *   - Magic 0xC2339F3D at offset 0x1C.
 *   - main.dol file offset stored as 32-bit big-endian at 0x420.
 *
 * Wii discs:
 *   - Magic 0x5D1C9EA3 at offset 0x18.
 *   - Partition info table at 0x40000; each entry is 8 bytes
 *     (offset_shifted_2, partition_type). Partition contents are
 *     AES-128-CBC encrypted in 0x8000-byte groups using a title key
 *     wrapped by the console common key. This loader lists the
 *     partition table only; extraction of main.dol from encrypted
 *     partitions requires a caller-supplied common key and is not
 *     implemented here.
 *
 * The loader streams from disk rather than mapping the whole image, so
 * multi-GB Wii ISOs do not need to be loaded into memory.
 */
#ifndef CALYNDA_DECOMPILER_ISO_H
#define CALYNDA_DECOMPILER_ISO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    ISO_FORMAT_UNKNOWN = 0,
    ISO_FORMAT_GAMECUBE,
    ISO_FORMAT_WII
} IsoFormat;

typedef enum {
    ISO_OK = 0,
    ISO_ERR_NULL_ARG,
    ISO_ERR_IO,
    ISO_ERR_TOO_SMALL,
    ISO_ERR_UNKNOWN_FORMAT,
    ISO_ERR_ENCRYPTED            /* valid but needs keys to proceed */
} IsoStatus;

#define ISO_GAME_ID_LEN 6

typedef struct {
    uint32_t offset;             /* partition start (absolute bytes) */
    uint32_t type;               /* 0=data, 1=update, 2=channel, ... */
} IsoWiiPartition;

typedef struct {
    IsoFormat format;
    char      game_id[ISO_GAME_ID_LEN + 1];  /* NUL terminated */
    char      game_title[65];                /* NUL terminated */

    /* GameCube only. Zero when format != ISO_FORMAT_GAMECUBE. */
    uint32_t  gc_dol_offset;

    /* Wii only. Up to 4 partition groups with up to 8 partitions each;
     * flattened here with a total count. */
    IsoWiiPartition wii_partitions[32];
    size_t    wii_partition_count;
} IsoInfo;

/* Open, probe, fill `out`, and leave the file at an undefined position.
 * `fp` is borrowed; the caller owns it and must keep it open while
 * later iso_* calls reference it. */
IsoStatus iso_probe(FILE *fp, IsoInfo *out);

/*
 * Read the GameCube main.dol from `fp` into a freshly-allocated buffer
 * (via malloc). The caller frees the returned pointer with free().
 * Fails with ISO_ERR_ENCRYPTED for Wii discs.
 */
IsoStatus iso_extract_gc_dol(FILE *fp,
                             const IsoInfo *info,
                             uint8_t **out_buf,
                             size_t *out_size);

/*
 * Read the main.dol from a Wii partition, given a user-supplied 16-byte
 * common key. `partition_index` must be a valid index into
 * `info->wii_partitions`. Returns a freshly-allocated buffer owned by
 * the caller (free with free()). Returns ISO_ERR_ENCRYPTED if the key
 * is wrong or the partition header is malformed.
 */
IsoStatus iso_extract_wii_dol(FILE *fp,
                              const IsoInfo *info,
                              size_t partition_index,
                              const uint8_t common_key[16],
                              uint8_t **out_buf,
                              size_t *out_size);

const char *iso_status_str(IsoStatus s);
const char *iso_format_str(IsoFormat f);

#endif /* CALYNDA_DECOMPILER_ISO_H */

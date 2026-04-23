#include "iso.h"

#include "wii_crypto.h"

#include <stdlib.h>
#include <string.h>

#define WII_MAGIC           0x5D1C9EA3u
#define WII_MAGIC_OFFSET    0x18u
#define GC_MAGIC            0xC2339F3Du
#define GC_MAGIC_OFFSET     0x1Cu
#define GC_DOL_OFFSET_AT    0x420u
#define WII_PART_TABLE_AT   0x40000u

/* DOL header size bookkeeping to size the extracted buffer. See dol.h
 * for the full layout. */
#define DOL_HEADER_SIZE 0x100u

static IsoStatus read_at(FILE *fp, uint64_t off, void *buf, size_t n) {
    if (fseek(fp, (long)off, SEEK_SET) != 0) return ISO_ERR_IO;
    if (fread(buf, 1, n, fp) != n)           return ISO_ERR_IO;
    return ISO_OK;
}

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

const char *iso_status_str(IsoStatus s) {
    switch (s) {
        case ISO_OK:                  return "ok";
        case ISO_ERR_NULL_ARG:        return "null argument";
        case ISO_ERR_IO:              return "I/O error";
        case ISO_ERR_TOO_SMALL:       return "image smaller than disc header";
        case ISO_ERR_UNKNOWN_FORMAT:  return "unknown disc format";
        case ISO_ERR_ENCRYPTED:       return "encrypted partition; key required";
    }
    return "unknown";
}

const char *iso_format_str(IsoFormat f) {
    switch (f) {
        case ISO_FORMAT_UNKNOWN:   return "unknown";
        case ISO_FORMAT_GAMECUBE:  return "gamecube";
        case ISO_FORMAT_WII:       return "wii";
    }
    return "?";
}

static IsoStatus probe_wii_partitions(FILE *fp, IsoInfo *out) {
    /* The partition info table has 4 entries of (count, table_offset>>2).
     * Each entry's table has `count` records of (partition_offset>>2,
     * partition_type). Offsets are 4-byte-shifted so a 32-bit field can
     * address anywhere on the disc. */
    uint8_t table[0x20];
    IsoStatus st = read_at(fp, WII_PART_TABLE_AT, table, sizeof(table));
    if (st != ISO_OK) return st;

    out->wii_partition_count = 0;
    for (size_t i = 0; i < 4; i++) {
        uint32_t count = be32(table + i * 8);
        uint64_t tbl   = (uint64_t)be32(table + i * 8 + 4) << 2;
        if (count == 0 || count > 8) continue;
        uint8_t records[8 * 8];
        if (read_at(fp, tbl, records, count * 8u) != ISO_OK) continue;
        for (uint32_t r = 0; r < count; r++) {
            if (out->wii_partition_count >= 32) break;
            IsoWiiPartition *p = &out->wii_partitions[out->wii_partition_count++];
            p->offset = be32(records + r * 8) << 2;
            p->type   = be32(records + r * 8 + 4);
        }
    }
    return ISO_OK;
}

IsoStatus iso_probe(FILE *fp, IsoInfo *out) {
    if (!fp || !out) return ISO_ERR_NULL_ARG;
    memset(out, 0, sizeof(*out));

    uint8_t hdr[0x440];
    IsoStatus st = read_at(fp, 0, hdr, sizeof(hdr));
    if (st != ISO_OK) return ISO_ERR_TOO_SMALL;

    memcpy(out->game_id, hdr, ISO_GAME_ID_LEN);
    out->game_id[ISO_GAME_ID_LEN] = '\0';
    memcpy(out->game_title, hdr + 0x20, sizeof(out->game_title) - 1);
    out->game_title[sizeof(out->game_title) - 1] = '\0';

    uint32_t wii_magic = be32(hdr + WII_MAGIC_OFFSET);
    uint32_t gc_magic  = be32(hdr + GC_MAGIC_OFFSET);

    if (wii_magic == WII_MAGIC) {
        out->format = ISO_FORMAT_WII;
        probe_wii_partitions(fp, out);
        return ISO_OK;
    }
    if (gc_magic == GC_MAGIC) {
        out->format = ISO_FORMAT_GAMECUBE;
        out->gc_dol_offset = be32(hdr + GC_DOL_OFFSET_AT);
        return ISO_OK;
    }
    return ISO_ERR_UNKNOWN_FORMAT;
}

IsoStatus iso_extract_gc_dol(FILE *fp, const IsoInfo *info,
                             uint8_t **out_buf, size_t *out_size) {
    if (!fp || !info || !out_buf || !out_size) return ISO_ERR_NULL_ARG;
    *out_buf = NULL;
    *out_size = 0;
    if (info->format == ISO_FORMAT_WII) return ISO_ERR_ENCRYPTED;
    if (info->format != ISO_FORMAT_GAMECUBE) return ISO_ERR_UNKNOWN_FORMAT;

    /* Read the DOL header first, sum text/data section sizes and offsets
     * to compute total DOL length, then read the whole thing. */
    uint8_t dolhdr[DOL_HEADER_SIZE];
    IsoStatus st = read_at(fp, info->gc_dol_offset, dolhdr, sizeof(dolhdr));
    if (st != ISO_OK) return st;

    uint32_t max_end = DOL_HEADER_SIZE;
    for (unsigned i = 0; i < 7; i++) {
        uint32_t off = be32(dolhdr + 0x00 + i * 4);
        uint32_t sz  = be32(dolhdr + 0x90 + i * 4);
        if (sz && off + sz > max_end) max_end = off + sz;
    }
    for (unsigned i = 0; i < 11; i++) {
        uint32_t off = be32(dolhdr + 0x1C + i * 4);
        uint32_t sz  = be32(dolhdr + 0xAC + i * 4);
        if (sz && off + sz > max_end) max_end = off + sz;
    }

    uint8_t *buf = (uint8_t *)malloc(max_end);
    if (!buf) return ISO_ERR_IO;
    st = read_at(fp, info->gc_dol_offset, buf, max_end);
    if (st != ISO_OK) { free(buf); return st; }

    *out_buf = buf;
    *out_size = max_end;
    return ISO_OK;
}

/*
 * Inside a Wii partition's decrypted user-data view, the "partition
 * boot header" mirrors the GameCube boot header layout, but every disc
 * offset is stored shifted right by 2. The offsets we care about:
 *   0x0420: main.dol offset (shifted 2), relative to partition data.
 */
#define WII_PART_DOL_OFFSET_AT 0x0420u

static uint32_t be32_local(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

IsoStatus iso_extract_wii_dol(FILE *fp,
                              const IsoInfo *info,
                              size_t partition_index,
                              const uint8_t common_key[16],
                              uint8_t **out_buf,
                              size_t *out_size) {
    if (!fp || !info || !common_key || !out_buf || !out_size) {
        return ISO_ERR_NULL_ARG;
    }
    *out_buf = NULL; *out_size = 0;
    if (info->format != ISO_FORMAT_WII) return ISO_ERR_UNKNOWN_FORMAT;
    if (partition_index >= info->wii_partition_count) return ISO_ERR_NULL_ARG;

    WiiPartitionReader r;
    WiiStatus wst = wii_partition_open(
        fp, info->wii_partitions[partition_index].offset, common_key, &r);
    if (wst != WII_OK) return ISO_ERR_ENCRYPTED;

    /* Read the partition boot header to find the DOL offset. */
    uint8_t boot[0x440];
    if (wii_partition_read(&r, 0, boot, sizeof(boot)) != WII_OK) {
        return ISO_ERR_ENCRYPTED;
    }
    uint64_t dol_off = (uint64_t)be32_local(boot + WII_PART_DOL_OFFSET_AT) << 2;

    /* Read the DOL header, compute total size, then read the DOL. */
    uint8_t dolhdr[0x100];
    if (wii_partition_read(&r, dol_off, dolhdr, sizeof(dolhdr)) != WII_OK) {
        return ISO_ERR_ENCRYPTED;
    }
    uint32_t max_end = 0x100u;
    for (unsigned i = 0; i < 7; i++) {
        uint32_t off = be32_local(dolhdr + 0x00 + i * 4);
        uint32_t sz  = be32_local(dolhdr + 0x90 + i * 4);
        if (sz && off + sz > max_end) max_end = off + sz;
    }
    for (unsigned i = 0; i < 11; i++) {
        uint32_t off = be32_local(dolhdr + 0x1C + i * 4);
        uint32_t sz  = be32_local(dolhdr + 0xAC + i * 4);
        if (sz && off + sz > max_end) max_end = off + sz;
    }

    uint8_t *buf = (uint8_t *)malloc(max_end);
    if (!buf) return ISO_ERR_IO;
    if (wii_partition_read(&r, dol_off, buf, max_end) != WII_OK) {
        free(buf);
        return ISO_ERR_ENCRYPTED;
    }
    *out_buf = buf;
    *out_size = max_end;
    return ISO_OK;
}

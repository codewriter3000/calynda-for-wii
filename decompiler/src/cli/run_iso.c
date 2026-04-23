#include "cli/run.h"
#include "iso.h"
#include "wii_crypto.h"
#include "run_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_hex_key(const char *hex, uint8_t out[16]) {
    if (!hex) return 0;
    size_t n = strlen(hex);
    if (n != 32) return 0;
    for (size_t i = 0; i < 16; i++) {
        unsigned hi, lo;
        char hc = hex[i * 2], lc = hex[i * 2 + 1];
        if      (hc >= '0' && hc <= '9') hi = (unsigned)(hc - '0');
        else if (hc >= 'a' && hc <= 'f') hi = (unsigned)(hc - 'a' + 10);
        else if (hc >= 'A' && hc <= 'F') hi = (unsigned)(hc - 'A' + 10);
        else return 0;
        if      (lc >= '0' && lc <= '9') lo = (unsigned)(lc - '0');
        else if (lc >= 'a' && lc <= 'f') lo = (unsigned)(lc - 'a' + 10);
        else if (lc >= 'A' && lc <= 'F') lo = (unsigned)(lc - 'A' + 10);
        else return 0;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return 1;
}

static size_t default_data_partition(const IsoInfo *info) {
    for (size_t i = 0; i < info->wii_partition_count; i++) {
        if (info->wii_partitions[i].type == 0) return i;
    }
    return 0;
}

static int run_gc(FILE *fp, const IsoInfo *info, const RunOptions *opts,
                  const SymTable *syms, FILE *out) {
    fprintf(stderr, "gc dol offset:  0x%08x\n", info->gc_dol_offset);
    uint8_t *dol = NULL; size_t dn = 0;
    IsoStatus ist = iso_extract_gc_dol(fp, info, &dol, &dn);
    fclose(fp);
    if (ist != ISO_OK) {
        fprintf(stderr, "error: iso: %s\n", iso_status_str(ist));
        return 1;
    }
    int rc = disassemble_dol(dol, dn, opts, syms, out);
    free(dol);
    return rc;
}

static int run_wii(FILE *fp, const IsoInfo *info, const RunOptions *opts,
                   const char *key_path, const char *key_hex,
                   int partition_arg, const SymTable *syms, FILE *out) {
    fprintf(stderr, "wii partitions: %zu\n", info->wii_partition_count);
    for (size_t i = 0; i < info->wii_partition_count; i++) {
        fprintf(stderr, "  [%zu] offset=0x%08x  type=%u\n",
               i, info->wii_partitions[i].offset,
               info->wii_partitions[i].type);
    }
    if (!key_path && !key_hex) {
        fprintf(stderr,
            "\nnote: Wii partitions are AES-encrypted. Supply your\n"
            "      own common key with --common-key <path> or --key-hex <hex>.\n");
        fclose(fp);
        return 0;
    }
    uint8_t common_key[16];
    if (key_hex) {
        if (!parse_hex_key(key_hex, common_key)) {
            fprintf(stderr, "--key-hex: expected exactly 32 hex characters\n");
            fclose(fp); return 1;
        }
    } else {
        WiiStatus kst = wii_load_key_file(key_path, common_key);
        if (kst != WII_OK) {
            fprintf(stderr, "common key: %s\n", wii_status_str(kst));
            fclose(fp); return 1;
        }
    }
    size_t pidx = partition_arg >= 0
        ? (size_t)partition_arg : default_data_partition(info);
    if (pidx >= info->wii_partition_count) {
        fprintf(stderr, "partition index %zu out of range (%zu)\n",
                pidx, info->wii_partition_count);
        fclose(fp); return 1;
    }
    fprintf(stderr, "using partition: %zu (offset=0x%08x, type=%u)\n",
           pidx, info->wii_partitions[pidx].offset,
           info->wii_partitions[pidx].type);
    uint8_t *dol = NULL; size_t dn = 0;
    IsoStatus ist = iso_extract_wii_dol(fp, info, pidx, common_key, &dol, &dn);
    fclose(fp);
    if (ist != ISO_OK) {
        fprintf(stderr,
                "error: iso: %s\n"
                "hint: wrong common key, or the partition data is unreadable.\n",
                iso_status_str(ist));
        return 1;
    }
    int rc = disassemble_dol(dol, dn, opts, syms, out);
    free(dol);
    return rc;
}

int run_on_iso_file(const char *path, const RunOptions *opts,
                    const char *key_path, const char *key_hex,
                    int partition_arg, const SymTable *syms, FILE *out) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { perror(path); return 1; }
    IsoInfo info;
    IsoStatus ist = iso_probe(fp, &info);
    if (ist != ISO_OK) {
        fprintf(stderr, "error: iso: %s\n", iso_status_str(ist));
        fclose(fp); return 1;
    }
    fprintf(stderr, "iso format:     %s\n", iso_format_str(info.format));
    fprintf(stderr, "game id:        %s\n", info.game_id);
    fprintf(stderr, "game title:     %s\n", info.game_title);

    if (info.format == ISO_FORMAT_GAMECUBE)
        return run_gc(fp, &info, opts, syms, out);
    if (info.format == ISO_FORMAT_WII)
        return run_wii(fp, &info, opts, key_path, key_hex, partition_arg,
                       syms, out);
    fclose(fp);
    return 1;
}

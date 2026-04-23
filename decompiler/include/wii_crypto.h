/*
 * Wii encrypted-partition reader.
 *
 * Wii game data lives inside partitions encrypted with AES-128-CBC
 * using a per-partition title key. The title key itself is stored in
 * the partition's ticket, encrypted with the console's "common key"
 * (also AES-128-CBC, IV = title_id || 8 zero bytes).
 *
 * This module:
 *   1. Unwraps a partition's title key given a user-supplied common
 *      key (no keys are bundled with this repository).
 *   2. Exposes random-access reads against the *decrypted* user-data
 *      view of the partition, transparently decrypting the
 *      0x8000-byte groups on demand.
 *
 * Hash verification (H0/H1/H2/H3) is intentionally skipped: we only
 * need the plaintext for static analysis, and skipping verification
 * lets us read valid-but-modified test fixtures.
 */
#ifndef CALYNDA_DECOMPILER_WII_CRYPTO_H
#define CALYNDA_DECOMPILER_WII_CRYPTO_H

#include "aes.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define WII_BLOCK_SIZE        0x8000u   /* encrypted group size */
#define WII_BLOCK_DATA_SIZE   0x7C00u   /* plaintext payload per group */
#define WII_BLOCK_HASH_SIZE   0x0400u   /* hash prologue per group */

typedef enum {
    WII_OK = 0,
    WII_ERR_NULL_ARG,
    WII_ERR_IO,
    WII_ERR_BAD_KEY_FILE,
    WII_ERR_OUT_OF_BOUNDS
} WiiStatus;

typedef struct {
    FILE    *fp;                 /* borrowed */
    uint64_t partition_offset;   /* absolute byte offset inside the ISO */
    uint64_t data_offset;        /* user-data offset relative to partition */
    uint8_t  title_key[16];

    /* One-block decrypted cache, keyed by user-data group index. */
    uint32_t cached_group;
    int      cache_valid;
    uint8_t  cache[WII_BLOCK_DATA_SIZE];
} WiiPartitionReader;

/* Load a 16-byte binary key from disk. Returns WII_OK on success. */
WiiStatus wii_load_key_file(const char *path, uint8_t out_key[16]);

/*
 * Open a partition for reading. Reads the ticket, unwraps the title
 * key, and records the data-section offset. `fp` and the data behind
 * `common_key` must remain valid while `out` is used.
 */
WiiStatus wii_partition_open(FILE *fp,
                             uint64_t partition_offset,
                             const uint8_t common_key[16],
                             WiiPartitionReader *out);

/*
 * Random-access read from the partition's plaintext user-data view.
 * `offset` is the decrypted-bytes offset; the reader transparently
 * maps it to encrypted groups on disk.
 */
WiiStatus wii_partition_read(WiiPartitionReader *r,
                             uint64_t offset,
                             uint8_t *buf,
                             size_t len);

const char *wii_status_str(WiiStatus s);

#endif /* CALYNDA_DECOMPILER_WII_CRYPTO_H */

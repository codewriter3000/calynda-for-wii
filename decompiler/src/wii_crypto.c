#include "wii_crypto.h"

#include <stdlib.h>
#include <string.h>

/* Declare POSIX fseeko/off_t without requiring feature-test macros. */
#if !defined(_WIN32)
#include <sys/types.h>
int fseeko(FILE *stream, off_t offset, int whence);
#endif

/* Ticket layout (offsets relative to partition start):
 *   0x01BF: encrypted title key (16 bytes)
 *   0x01DC: title id (8 bytes)
 *
 * After the ticket, at offset 0x02B8 (u32 BE, shifted by 2) sits the
 * data section offset relative to partition start.
 */
#define TICKET_ENC_TITLE_KEY 0x01BFu
#define TICKET_TITLE_ID      0x01DCu
#define PART_DATA_OFFSET_AT  0x02B8u

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

static WiiStatus read_at(FILE *fp, uint64_t off, void *buf, size_t n) {
    /* fseek on large files: use _fseeki64 where available, else fall
     * back to multiple relative seeks. */
#if defined(_WIN32)
    if (_fseeki64(fp, (long long)off, SEEK_SET) != 0) return WII_ERR_IO;
#else
    if (fseeko(fp, (off_t)off, SEEK_SET) != 0) return WII_ERR_IO;
#endif
    if (fread(buf, 1, n, fp) != n) return WII_ERR_IO;
    return WII_OK;
}

const char *wii_status_str(WiiStatus s) {
    switch (s) {
        case WII_OK:                 return "ok";
        case WII_ERR_NULL_ARG:       return "null argument";
        case WII_ERR_IO:             return "I/O error";
        case WII_ERR_BAD_KEY_FILE:   return "key file not exactly 16 bytes";
        case WII_ERR_OUT_OF_BOUNDS:  return "read past partition data";
    }
    return "unknown";
}

WiiStatus wii_load_key_file(const char *path, uint8_t out_key[16]) {
    if (!path || !out_key) return WII_ERR_NULL_ARG;
    FILE *fp = fopen(path, "rb");
    if (!fp) return WII_ERR_IO;
    uint8_t buf[17];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (n != 16) return WII_ERR_BAD_KEY_FILE;
    memcpy(out_key, buf, 16);
    return WII_OK;
}

WiiStatus wii_partition_open(FILE *fp,
                             uint64_t partition_offset,
                             const uint8_t common_key[16],
                             WiiPartitionReader *out) {
    if (!fp || !common_key || !out) return WII_ERR_NULL_ARG;
    memset(out, 0, sizeof(*out));
    out->fp = fp;
    out->partition_offset = partition_offset;
    out->cache_valid = 0;

    /* Read enough of the partition header to cover ticket + data offset. */
    uint8_t hdr[0x2C0];
    WiiStatus st = read_at(fp, partition_offset, hdr, sizeof(hdr));
    if (st != WII_OK) return st;

    uint8_t iv[16];
    memset(iv, 0, 16);
    memcpy(iv, hdr + TICKET_TITLE_ID, 8);

    aes128_cbc_decrypt(common_key, iv,
                       hdr + TICKET_ENC_TITLE_KEY, out->title_key, 16);

    out->data_offset = (uint64_t)be32(hdr + PART_DATA_OFFSET_AT) << 2;
    return WII_OK;
}

static WiiStatus fetch_group(WiiPartitionReader *r, uint32_t group) {
    if (r->cache_valid && r->cached_group == group) return WII_OK;
    uint64_t base = r->partition_offset + r->data_offset +
                    (uint64_t)group * WII_BLOCK_SIZE;
    uint8_t enc[WII_BLOCK_SIZE];
    WiiStatus st = read_at(r->fp, base, enc, WII_BLOCK_SIZE);
    if (st != WII_OK) return st;

    /* Decrypt the hash prologue with IV=0 to recover the IV used for
     * the user-data payload. The payload IV lives at bytes 0x3D0..0x3DF
     * of the decrypted hash block. */
    uint8_t hash_iv[16]; memset(hash_iv, 0, 16);
    uint8_t hash_plain[WII_BLOCK_HASH_SIZE];
    aes128_cbc_decrypt(r->title_key, hash_iv,
                       enc, hash_plain, WII_BLOCK_HASH_SIZE);

    uint8_t data_iv[16];
    memcpy(data_iv, hash_plain + 0x3D0, 16);
    aes128_cbc_decrypt(r->title_key, data_iv,
                       enc + WII_BLOCK_HASH_SIZE,
                       r->cache, WII_BLOCK_DATA_SIZE);

    r->cached_group = group;
    r->cache_valid = 1;
    return WII_OK;
}

WiiStatus wii_partition_read(WiiPartitionReader *r,
                             uint64_t offset,
                             uint8_t *buf,
                             size_t len) {
    if (!r || !buf) return WII_ERR_NULL_ARG;
    while (len > 0) {
        uint32_t group = (uint32_t)(offset / WII_BLOCK_DATA_SIZE);
        uint32_t within = (uint32_t)(offset % WII_BLOCK_DATA_SIZE);
        WiiStatus st = fetch_group(r, group);
        if (st != WII_OK) return st;
        size_t avail = WII_BLOCK_DATA_SIZE - within;
        size_t take  = len < avail ? len : avail;
        memcpy(buf, r->cache + within, take);
        buf += take; offset += take; len -= take;
    }
    return WII_OK;
}

/*
 * Minimal AES-128 block cipher + CBC mode.
 *
 * Small, dependency-free implementation of FIPS-197 sufficient to
 * decrypt Wii partition title keys and partition data. Intentionally
 * not constant-time: this is for offline static analysis of game
 * binaries, not for production cryptographic use.
 */
#ifndef CALYNDA_DECOMPILER_AES_H
#define CALYNDA_DECOMPILER_AES_H

#include <stddef.h>
#include <stdint.h>

#define AES_BLOCK_SIZE 16
#define AES128_KEY_SIZE 16

typedef struct {
    uint8_t round_keys[11][16]; /* 10 rounds + initial */
} Aes128Ctx;

void aes128_init(Aes128Ctx *ctx, const uint8_t key[16]);
void aes128_encrypt_block(const Aes128Ctx *ctx,
                          const uint8_t in[16], uint8_t out[16]);
void aes128_decrypt_block(const Aes128Ctx *ctx,
                          const uint8_t in[16], uint8_t out[16]);

/*
 * AES-CBC with a 128-bit key. `len` must be a multiple of 16.
 * `out` may alias `in`. `iv` is not updated in place.
 */
void aes128_cbc_encrypt(const uint8_t key[16], const uint8_t iv[16],
                        const uint8_t *in, uint8_t *out, size_t len);
void aes128_cbc_decrypt(const uint8_t key[16], const uint8_t iv[16],
                        const uint8_t *in, uint8_t *out, size_t len);

#endif

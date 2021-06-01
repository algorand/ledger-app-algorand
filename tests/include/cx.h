#pragma once

#include <stddef.h>
#include <stdint.h>

typedef unsigned int cx_curve_t;

#define CX_CURVE_256K1     0x1234
#define CX_CURVE_SECP256K1 CX_CURVE_256K1

/** Message Digest algorithm identifiers. */
enum cx_md_e {
    /** NONE Digest */
    CX_NONE,
    /** RIPEMD160 Digest */
    CX_RIPEMD160,  // 20 bytes
    /** SHA224 Digest */
    CX_SHA224,  // 28 bytes
    /** SHA256 Digest */
    CX_SHA256,  // 32 bytes
    /** SHA384 Digest */
    CX_SHA384,  // 48 bytes
    /** SHA512 Digest */
    CX_SHA512,  // 64 bytes
    /** Keccak (pre-SHA3) Digest */
    CX_KECCAK,  // 28,32,48,64 bytes
    /** SHA3 Digest */
    CX_SHA3,  // 28,32,48,64 bytes
    /** Groestl Digest */
    CX_GROESTL,
    /** Blake Digest */
    CX_BLAKE2B,
    /** SHAKE-128 Digest */
    CX_SHAKE128,  // any bytes
    /** SHAKE-128 Digest */
    CX_SHAKE256,  // any bytes
};
/** Convenience type. See #cx_md_e. */
typedef enum cx_md_e cx_md_t;

struct cx_hash_header_s {
    /** Message digest identifier, See cx_md_e. */
    cx_md_t algo;
    /** Number of block already processed */
    unsigned int counter;
};
typedef struct cx_hash_header_s cx_hash_t;

struct cx_sha256_s {
    /** @copydoc cx_ripemd160_s::header */
    struct cx_hash_header_s header;
    /** @internal @copydoc cx_ripemd160_s::blen */
    unsigned int blen;
    /** @internal @copydoc cx_ripemd160_s::block */
    unsigned char block[64];
    /** @copydoc cx_ripemd160_s::acc */
    unsigned char acc[8 * 4];
};
/** Convenience type. See #cx_sha256_s. */
typedef struct cx_sha256_s cx_sha256_t;

/**
 * SHA-384 and SHA-512 context
 */
struct cx_sha512_s {
  /** @copydoc cx_ripemd160_s::header */
  struct cx_hash_header_s header;
  /** @internal @copydoc cx_ripemd160_s::blen */
  unsigned int blen;
  /** @internal @copydoc cx_ripemd160_s::block */
  unsigned char block[128];
  /** @copydoc cx_ripemd160_s::acc */
  unsigned char acc[8 * 8];
};
/** Convenience type. See #cx_sha512_s. */
typedef struct cx_sha512_s cx_sha512_t;

#define CX_LAST (1 << 0)

struct cx_ecfp_256_private_key_s;
typedef struct cx_ecfp_256_private_key_s cx_ecfp_private_key_t;

int cx_sha256_init(cx_sha256_t *hash);
int cx_sha512_init(cx_sha512_t *hash);
int cx_hash(cx_hash_t *hash,
            int mode,
            const uint8_t *in,
            size_t len,
            uint8_t *out,
            size_t out_len);

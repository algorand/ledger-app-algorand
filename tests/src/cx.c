#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <openssl/sha.h>

#include "cx.h"
#include "algo_keys.h"

/* mock fetch_public_key */
int fetch_public_key(uint32_t accountId, struct pubkey_s *pubkey)
{
  memset(pubkey->data, accountId & 0xff, sizeof(pubkey->data));
  return 0;
}

int cx_sha256_init(cx_sha256_t *hash)
{
  memset(hash, 0, sizeof(cx_sha256_t));
  hash->header.algo = CX_SHA256;
  return CX_SHA256;
}

int cx_sha512_init(cx_sha512_t *hash)
{
  memset(hash, 0, sizeof(cx_sha512_t));
  hash->header.algo = CX_SHA512;
  return CX_SHA512;
}

int cx_hash(cx_hash_t *hash,
            int mode,
            const uint8_t *in,
            size_t len,
            uint8_t *out,
            size_t out_len)
{
    if (hash->algo == CX_SHA256) {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, in, len);
        SHA256_Final(out, &sha256);
    } else if (hash->algo == CX_SHA512) {
        SHA512_CTX sha512;
        SHA512_Init(&sha512);
        SHA512_Update(&sha512, in, len);
        SHA512_Final(out, &sha512);
    } else {
      assert(false);
    }

    return 0;
}

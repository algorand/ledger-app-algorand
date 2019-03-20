#include "os.h"

#include "algo_addr.h"
#include "base32.h"
#include "sha512_256.h"

void
checksummed_addr(const uint8_t *publicKey, char *out)
{
  uint8_t hash[32];
  sha512_256(publicKey, 32, hash);

  uint8_t checksummed[36];
  os_memmove(&checksummed[0], publicKey, 32);
  os_memmove(&checksummed[32], &hash[28], 4);

  os_memset(out, 0, 65);
  base32_encode(checksummed, sizeof(checksummed), (unsigned char*) out);
}

#include "algo_keys.h"

#define CSUM_ADDR_SIZE 65

struct addr_s {
  char data[CSUM_ADDR_SIZE];
};

// The input public key should be 32 bytes long.
// The output buffer must be at least 65 bytes long.
void checksummed_addr(const struct pubkey_s *public_key, struct addr_s *addr);

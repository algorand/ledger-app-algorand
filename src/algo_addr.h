#define CSUM_ADDR_SIZE 65

struct addr_s {
  char data[CSUM_ADDR_SIZE];
};

// The input public key should be 32 bytes long.
// The output buffer must be at least 65 bytes long.
void checksummed_addr(const uint8_t *publicKey, struct addr_s *addr);

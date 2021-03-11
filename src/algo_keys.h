#include "cx.h"
#include "stdbool.h"

#define PUBKEY_SIZE 32

struct pubkey_s {
  uint8_t data[PUBKEY_SIZE];
};

typedef struct{
    bool initialized;
    uint32_t accountID;
    struct pubkey_s pubkey;
} already_computed_key_t;

extern already_computed_key_t current_pubkey;

void algorand_key_derive(uint32_t accountId, cx_ecfp_private_key_t *privateKey);
void fetch_public_key(uint32_t accountId, struct pubkey_s *pubkey);

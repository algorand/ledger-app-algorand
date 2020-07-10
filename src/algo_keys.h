#include "cx.h"
#include "stdbool.h"

typedef struct{
    bool initialized;
    uint32_t accountID;
    uint8_t pubkey[32];
} already_computed_key_t;

extern already_computed_key_t current_pubkey;

void algorand_key_derive(uint32_t accountId, cx_ecfp_private_key_t *privateKey);
size_t fetch_public_key(uint32_t accountId, uint8_t* pubkey);
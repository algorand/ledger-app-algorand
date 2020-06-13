#include "cx.h"

extern uint8_t publicKey[32];

void algorand_key_derive(uint32_t accountId, cx_ecfp_private_key_t *privateKey);
void algorand_public_key(cx_ecfp_private_key_t *privateKey, uint8_t *buf);

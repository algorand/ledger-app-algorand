#include "cx.h"

extern uint8_t publicKey[32];

void algorand_key_derive(uint32_t accountId, uint8_t *privateKeyData);
void algorand_private_key(uint8_t *privateKeyData,
                          cx_ecfp_private_key_t *privateKey);
void algorand_public_key(uint8_t *privateKeyData, uint8_t *buf);

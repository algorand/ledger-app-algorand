#include "cx.h"

extern uint8_t publicKey[32];

void algorand_key_derive(void);
void algorand_private_key(cx_ecfp_private_key_t *privateKey);
void algorand_public_key(uint8_t *buf);

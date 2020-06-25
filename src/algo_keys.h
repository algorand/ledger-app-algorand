#include "cx.h"

void algorand_key_derive(uint32_t accountId, cx_ecfp_private_key_t *privateKey);
int algorand_public_key(const cx_ecfp_private_key_t *privateKey, uint8_t *buf);

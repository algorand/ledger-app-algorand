#ifndef _ALGO_KEYS_H
#define _ALGO_KEYS_H

#include "os.h"
#include "cx.h"

//------------------------------------------------------------------------------

void algorand_private_key(cx_ecfp_private_key_t *privateKey);
void algorand_public_key(uint8_t *buf);

//------------------------------------------------------------------------------

#endif //_ALGO_KEYS_H

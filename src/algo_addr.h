#ifndef _ALGO_ADDR_H
#define _ALGO_ADDR_H

#include "os.h"

//------------------------------------------------------------------------------

// The input public key should be 32 bytes long.
// The output buffer must be at least 65 bytes long.
void checksummed_addr(const uint8_t *publicKey, char *out);

//------------------------------------------------------------------------------

#endif //_ALGO_ADDR_H

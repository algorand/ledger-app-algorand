
#ifndef _MAIN_H
#define _MAIN_H

#include "os.h"
#include "cx.h"
#include "algo_tx.h"
#include "algo_keys.h"
#include "algo_addr.h"

//------------------------------------------------------------------------------

#define CTX_STATE_None 0

typedef struct context_t {
  uint8_t state;
  struct tx_t current_tx;
} context_t;

//------------------------------------------------------------------------------

extern context_t context;

//------------------------------------------------------------------------------

#endif //_MAIN_H
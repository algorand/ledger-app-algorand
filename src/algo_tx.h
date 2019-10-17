#ifndef _ALGO_TX_H
#define _ALGO_TX_H

#include "os.h"

//------------------------------------------------------------------------------

#define MAX_NOTE_FIELD_SIZE 512

//------------------------------------------------------------------------------

typedef enum txType_e {
  UNKNOWN,
  PAYMENT,
  KEYREG
} txType_e;

typedef struct tx_t {
  enum txType_e type;

  // Common header fields
  uint8_t sender[32];
  uint64_t fee;
  uint64_t firstValid;
  uint64_t lastValid;
  char genesisID[33];
  uint8_t genesisHash[32];

  union {
    struct tx_payment_t {
      uint8_t receiver[32];
      uint64_t amount;
      uint8_t close[32];
    } payment;

    struct tx_keyreg_t {
      uint8_t votepk[32];
      uint8_t vrfpk[32];
    } keyreg;
  } payload;

  uint8_t note[MAX_NOTE_FIELD_SIZE];
  uint32_t note_size;
} tx_t;

//------------------------------------------------------------------------------

// tx_encode produces a canonical msgpack encoding of a transaction.
// buflen is the size of the buffer.  The return value is the length
// of the resulting encoding.
unsigned int tx_encode(struct tx_t *t, uint8_t *buf, unsigned int buflen);

/*
// Two callbacks into the main code: approve and deny signing.
void txn_approve();
void txn_deny();
*/

//------------------------------------------------------------------------------

#endif //_ALGO_TX_H

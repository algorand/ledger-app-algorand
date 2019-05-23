enum TXTYPE {
  UNKNOWN,
  PAYMENT,
  KEYREG
};

struct txn {
  enum TXTYPE type;

  // Common header fields
  uint8_t sender[32];
  uint64_t fee;
  uint64_t firstValid;
  uint64_t lastValid;
  char genesisID[33];
  uint8_t genesisHash[32];

  // Payments
  uint8_t receiver[32];
  uint64_t amount;
  uint8_t close[32];

  // Keyreg
  uint8_t votepk[32];
  uint8_t vrfpk[32];
};

// tx_encode produces a canonical msgpack encoding of a transaction.
// buflen is the size of the buffer.  The return value is the length
// of the resulting encoding.
unsigned int tx_encode(struct txn *t, uint8_t *buf, int buflen);

// We have a global transaction that is the subject of the current
// operation, if any.
extern struct txn current_txn;

// Two callbacks into the main code: approve and deny signing.
void txn_approve();
void txn_deny();

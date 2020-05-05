#include <stdint.h>

enum TXTYPE {
  UNKNOWN,
  PAYMENT,
  KEYREG,
  ASSET_XFER,
  ASSET_FREEZE,
  ASSET_CONFIG,
  APPLICATION,
  ALL_TYPES,
};

typedef enum oncompletion {
  NOOPOC       = 0,
  OPTINOC      = 1,
  CLOSEOUTOC   = 2,
  CLEARSTATEOC = 3,
  UPDATEAPPOC  = 4,
  DELETEAPPOC  = 5,
} oncompletion_t;

struct asset_params {
  uint64_t total;
  uint64_t decimals;
  uint8_t default_frozen;
  char unitname[8];
  char assetname[32];
  char url[32];
  uint8_t metadata_hash[32];
  uint8_t manager[32];
  uint8_t reserve[32];
  uint8_t freeze[32];
  uint8_t clawback[32];
};

struct state_schema {
  uint64_t num_uint;
  uint64_t num_byteslice;
};

struct txn_payment {
  uint8_t receiver[32];
  uint64_t amount;
  uint8_t close[32];
};

struct txn_keyreg {
  uint8_t votepk[32];
  uint8_t vrfpk[32];
  uint64_t voteFirst;
  uint64_t voteLast;
  uint64_t keyDilution;
  uint8_t nonpartFlag;
};

struct txn_asset_xfer {
  uint64_t id;
  uint64_t amount;
  uint8_t sender[32];
  uint8_t receiver[32];
  uint8_t close[32];
};

struct txn_asset_freeze {
  uint64_t id;
  uint8_t account[32];
  uint8_t flag;
};

struct txn_asset_config {
  uint64_t id;
  struct asset_params params;
};

#define MAX_APP_ARG 2
#define MAX_ARG_LEN 32

#define MAX_ACCT 1
#define ACCT_LEN 32

typedef uint8_t app_args_t[MAX_APP_ARG][MAX_ARG_LEN];
typedef size_t app_args_len_t[MAX_APP_ARG];

typedef uint8_t accounts_t[MAX_ACCT][ACCT_LEN];

struct txn_application {
  uint64_t id;
  uint64_t oncompletion;

  accounts_t accounts;
  size_t num_accounts;

  app_args_t app_args;
  app_args_len_t app_arg_len;
  size_t num_app_args;

  uint64_t foreign_apps[1];
  size_t num_foreign_apps;

  uint8_t approv_prog[128];
  size_t approv_prog_len;

  uint8_t clear_prog[128];
  size_t clear_prog_len;

  struct state_schema local_schema;
  struct state_schema global_schema;
};

struct txn {
  enum TXTYPE type;

  // Common header fields
  uint8_t sender[32];
  uint8_t rekey[32];
  uint64_t fee;
  uint64_t firstValid;
  uint64_t lastValid;
  char genesisID[32];
  uint8_t genesisHash[32];

#if defined(TARGET_NANOX)
  uint8_t note[512];
#else
  uint8_t note[32];
#endif

  size_t note_len;

  // Fields for specific tx types
  union {
    struct txn_payment payment;
    struct txn_keyreg keyreg;
    struct txn_asset_xfer asset_xfer;
    struct txn_asset_freeze asset_freeze;
    struct txn_asset_config asset_config;
    struct txn_application application;
  };
};

// tx_encode produces a canonical msgpack encoding of a transaction.
// buflen is the size of the buffer.  The return value is the length
// of the resulting encoding.
unsigned int tx_encode(struct txn *t, uint8_t *buf, int buflen);

// tx_decode takes a canonical msgpack encoding of a transaction, and
// unpacks it into a struct txn.  The return value is NULL for success,
// or a string describing the error on failure.  Decoding may or may
// not succeed for a non-canonical encoding.
char* tx_decode(uint8_t *buf, int buflen, struct txn *t);

// We have a global transaction that is the subject of the current
// operation, if any.
extern struct txn current_txn;

// Two callbacks into the main code: approve and deny signing.
void txn_approve();
void txn_deny();

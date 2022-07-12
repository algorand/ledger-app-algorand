/*******************************************************************************
*  (c) 2019 Zondax GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef enum TxType {
  TX_UNKNOWN,
  TX_PAYMENT,
  TX_KEYREG,
  TX_ASSET_XFER,
  TX_ASSET_FREEZE,
  TX_ASSET_CONFIG,
  TX_APPLICATION,
  TX_ALL_TYPES,
} TxType;

#define KEY_COMMON_TYPE           "type"

#define KEY_TX_PAY                "pay"
#define KEY_TX_KEYREG             "keyreg"
#define KEY_TX_ASSET_XFER         "axfer"
#define KEY_TX_ASSET_FREEZE       "afrz"
#define KEY_TX_ASSET_CONFIG       "acfg"
#define KEY_TX_APPLICATION        "appl"

#define KEY_COMMON_SENDER         "snd"
#define KEY_COMMON_REKEY          "rekey"
#define KEY_COMMON_FEE            "fee"
#define KEY_COMMON_FIRST_VALID    "fv"
#define KEY_COMMON_LAST_VALID     "lv"
#define KEY_COMMON_GEN_ID         "gen"
#define KEY_COMMON_GEN_HASH       "gh"
#define KEY_COMMON_GROUP_ID       "grp"
#define KEY_COMMON_NOTE           "note"

#define KEY_PAY_AMOUNT            "amt"
#define KEY_PAY_RECEIVER          "rcv"
#define KEY_PAY_CLOSE             "close"

#define KEY_VRF_PK                "selkey"
#define KEY_SPRF_PK               "sprfkey"
#define KEY_VOTE_PK               "votekey"
#define KEY_VOTE_FIRST            "votefst"
#define KEY_VOTE_LAST             "votelst"
#define KEY_VOTE_KEY_DILUTION     "votekd"
#define KEY_VOTE_NON_PART_FLAG    "nonpart"

#define KEY_XFER_AMOUNT           "aamt"
#define KEY_XFER_CLOSE            "aclose"
#define KEY_XFER_RECEIVER         "arcv"
#define KEY_XFER_SENDER           "asnd"
#define KEY_XFER_ID               "xaid"

#define KEY_FREEZE_ID             "faid"
#define KEY_FREEZE_ACCOUNT        "fadd"
#define KEY_FREEZE_FLAG           "afrz"

#define KEY_CONFIG_ID             "caid"
#define KEY_CONFIG_PARAMS         "apar"

#define KEY_APP_ID                "apid"
#define KEY_APP_ARGS              "apaa"
#define KEY_APP_APROG_LEN         "apap"
#define KEY_APP_CPROG_LEN         "apsu"
#define KEY_APP_ONCOMPLETION      "apan"
#define KEY_APP_ACCOUNTS          "apat"
#define KEY_APP_LOCAL_SCHEMA      "apls"
#define KEY_APP_GLOBAL_SCHEMA     "apgs"
#define KEY_APP_FOREIGN_APPS      "apfa"
#define KEY_APP_FOREIGN_ASSETS    "apas"

#define KEY_APARAMS_TOTAL         "t"
#define KEY_APARAMS_DECIMALS      "dc"
#define KEY_APARAMS_DEF_FROZEN    "df"
#define KEY_APARAMS_UNIT_NAME     "un"
#define KEY_APARAMS_ASSET_NAME    "an"
#define KEY_APARAMS_URL           "au"
#define KEY_APARAMS_METADATA_HASH "am"
#define KEY_APARAMS_MANAGER       "m"
#define KEY_APARAMS_RESERVE       "r"
#define KEY_APARAMS_FREEZE        "f"
#define KEY_APARAMS_CLAWBACK      "c"


typedef enum oncompletion {
  NOOPOC       = 0,
  OPTINOC      = 1,
  CLOSEOUTOC   = 2,
  CLEARSTATEOC = 3,
  UPDATEAPPOC  = 4,
  DELETEAPPOC  = 5,
} oncompletion_t;

typedef struct {
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
} asset_params;

typedef struct {
  uint64_t num_uint;
  uint64_t num_byteslice;
} state_schema;

#define MAX_ACCT 2
typedef uint8_t accounts_t[MAX_ACCT][32];

#define MAX_ARG 2
#define MAX_ARGLEN 32
#define MAX_FOREIGN_APPS 1
#define MAX_FOREIGN_ASSETS 1
#define MAX_APPROV_LEN 128
#define MAX_CLEAR_LEN 32

// TXs structs
typedef struct {
  uint8_t receiver[32];
  uint64_t amount;
  uint8_t close[32];
} txn_payment;

typedef struct {
  uint8_t votepk[32];
  uint8_t vrfpk[32];
  uint8_t sprfkey[64];
  uint64_t voteFirst;
  uint64_t voteLast;
  uint64_t keyDilution;
  uint8_t nonpartFlag;
} txn_keyreg;

typedef struct {
  uint64_t id;
  uint64_t amount;
  uint8_t sender[32];
  uint8_t receiver[32];
  uint8_t close[32];
} txn_asset_xfer;

typedef struct {
  uint64_t id;
  uint8_t account[32];
  uint8_t flag;
} txn_asset_freeze;

typedef struct {
  uint64_t id;
  asset_params params;
} txn_asset_config;

typedef struct {
  uint64_t id;
  uint64_t oncompletion;

  uint8_t accounts[MAX_ACCT][32];
  size_t num_accounts;

  uint64_t foreign_apps[MAX_FOREIGN_APPS];
  size_t num_foreign_apps;

  uint64_t foreign_assets[MAX_FOREIGN_ASSETS];
  size_t num_foreign_assets;

  uint8_t app_args[MAX_ARG][MAX_ARGLEN];
  size_t app_args_len[MAX_ARG];
  size_t num_app_args;

  uint8_t aprog[MAX_APPROV_LEN];
  size_t aprog_len;

  uint8_t cprog[MAX_CLEAR_LEN];
  size_t cprog_len;

  state_schema local_schema;
  state_schema global_schema;
} txn_application;

typedef struct{
      // Fields for specific tx types
    union {
    txn_payment payment;
    txn_keyreg keyreg;
    txn_asset_xfer asset_xfer;
    txn_asset_freeze asset_freeze;
    txn_asset_config asset_config;
    txn_application application;
    };

  TxType type;
  // Account Id asscociated with this transaction.
  uint32_t accountId;

  // Common header fields
  uint8_t sender[32];
  uint8_t rekey[32];
  uint64_t fee;
  uint64_t firstValid;
  uint64_t lastValid;
  char genesisID[32];
  uint8_t genesisHash[32];
  uint8_t groupID[32];

  size_t note_len;

#if defined(TARGET_NANOS)
  uint8_t note[32];
#else
  uint8_t note[512];
#endif

} parser_tx_t;

typedef parser_tx_t txn_t;

#ifdef __cplusplus
}
#endif

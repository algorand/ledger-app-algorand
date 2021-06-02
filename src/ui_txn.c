#include <string.h>
#include "os.h"

#include "algo_ui.h"
#include "algo_tx.h"
#include "algo_addr.h"
#include "algo_keys.h"
#include "algo_asa.h"
#include "base64.h"
#include "glyphs.h"
#include "ui_txn.h"

#define CHECKEDSUM_BUFFER_SIZE 65
#define MAX_DIGITS_IN_UINT64 27


bool is_opt_in_tx(){

  if(current_txn.type == ASSET_XFER &&
     current_txn.asset_xfer.amount == 0 &&
     current_txn.asset_xfer.id != 0 &&
     memcmp(current_txn.asset_xfer.receiver,
            current_txn.asset_xfer.sender,
            sizeof(current_txn.asset_xfer.receiver)) == 0){
      return true;
  }
  return false;
}

char caption[20];

bool adjustDecimals(char *src, uint32_t srcLength, char *target,
                    uint32_t targetLength, uint8_t decimals) {
    uint32_t startOffset;
    uint32_t lastZeroOffset = 0;
    uint32_t offset = 0;
    if ((srcLength == 1) && (*src == '0')) {
        if (targetLength < 2) {
                return false;
        }
        target[0] = '0';
        target[1] = '\0';
        return true;
    }
    if (srcLength <= decimals) {
        uint32_t delta = decimals - srcLength;
        if (targetLength < srcLength + 1 + 2 + delta) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '.';
        for (uint32_t i = 0; i < delta; i++) {
            target[offset++] = '0';
        }
        startOffset = offset;
        for (uint32_t i = 0; i < srcLength; i++) {
            target[offset++] = src[i];
        }
        target[offset] = '\0';
    } else {
        uint32_t sourceOffset = 0;
        uint32_t delta = srcLength - decimals;
        if (targetLength < srcLength + 1 + 1) {
            return false;
        }
        while (offset < delta) {
            target[offset++] = src[sourceOffset++];
        }
        if (decimals != 0) {
            target[offset++] = '.';
        }
        startOffset = offset;
        while (sourceOffset < srcLength) {
            target[offset++] = src[sourceOffset++];
        }
  target[offset] = '\0';
    }
    for (uint32_t i = startOffset; i < offset; i++) {
        if (target[i] == '0') {
            if (lastZeroOffset == 0) {
                lastZeroOffset = i;
            }
        } else {
            lastZeroOffset = 0;
        }
    }
    if (lastZeroOffset != 0) {
        target[lastZeroOffset] = '\0';
        if (target[lastZeroOffset - 1] == '.') {
                target[lastZeroOffset - 1] = '\0';
        }
    }
    return true;
}

static char*
amount_to_str(uint64_t amount, uint8_t decimals){
  char* result = u64str(amount);
  char tmp[MAX_DIGITS_IN_UINT64];
  memcpy(tmp, result, sizeof(tmp));
  explicit_bzero(result, sizeof(tmp));
  adjustDecimals(tmp, strlen(tmp), result, MAX_DIGITS_IN_UINT64, decimals);
  result[MAX_DIGITS_IN_UINT64-1] = '\0';
  return result;
}


static void checksum_and_put_text(const uint8_t * buffer)
{
  char checksummed[CHECKEDSUM_BUFFER_SIZE];
  memset(checksummed,0,CHECKEDSUM_BUFFER_SIZE);
  convert_to_public_address(buffer, checksummed);
  ui_text_put(checksummed);
}


static int
all_zero_key(uint8_t *buf)
{
  for (int i = 0; i < 32; i++) {
    if (buf[i] != 0) {
      return 0;
    }
  }

  return 1;
}

static char *
b64hash_data(unsigned char *data, size_t data_len)
{
  static char b64hash[45];
  unsigned char hash[32];

  // Hash program and b64 encode for display
  cx_sha256_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  cx_sha256_init(&ctx);
  cx_hash(&ctx.header, CX_LAST, data, data_len, hash, sizeof(hash));
  base64_encode((const char *)hash, sizeof(hash), b64hash, sizeof(b64hash));

  return b64hash;
}

static int step_txn_type() {
  switch (current_txn.type) {
  case PAYMENT:
    ui_text_put("Payment");
    break;

  case KEYREG:
    ui_text_put("Key reg");
    break;

  case ASSET_XFER:
    if(is_opt_in_tx()){
      ui_text_put("Opt-in");
    }else{
      ui_text_put("Asset xfer");
    }
    break;

  case ASSET_FREEZE:
    ui_text_put("Asset freeze");
    break;

  case ASSET_CONFIG:
    ui_text_put("Asset config");
    break;

  case APPLICATION:
    ui_text_put("Application");
    break;

  default:
    ui_text_put("Unknown");
  }
  return 1;
}

static int step_sender() {
  checksum_and_put_text(current_txn.sender);
  return 1;
}

static int step_rekey() {
  if (all_zero_key(current_txn.rekey)) {
    return 0;
  }
  checksum_and_put_text(current_txn.rekey);
  return 1;
}

static int step_fee() {
  ui_text_put(amount_to_str(current_txn.fee, ALGORAND_DECIMALS));
  return 1;
}

// static int step_firstvalid() {
//   ui_text_put(u64str(current_txn.firstValid));
//   return 1;
// }

// static int step_lastvalid() {
//   ui_text_put(u64str(current_txn.lastValid));
//   return 1;
// }

static const char* default_genesisID = "mainnet-v1.0";
static const uint8_t default_genesisHash[] = {
  0xc0, 0x61, 0xc4, 0xd8, 0xfc, 0x1d, 0xbd, 0xde, 0xd2, 0xd7, 0x60, 0x4b, 0xe4, 0x56, 0x8e, 0x3f, 0x6d, 0x4, 0x19, 0x87, 0xac, 0x37, 0xbd, 0xe4, 0xb6, 0x20, 0xb5, 0xab, 0x39, 0x24, 0x8a, 0xdf,
};

static int step_genesisID() {
  if (strncmp(current_txn.genesisID, default_genesisID, sizeof(current_txn.genesisID)) == 0) {
    return 0;
  }

  if (current_txn.genesisID[0] == '\0') {
    return 0;
  }

  ui_text_put(current_txn.genesisID);
  return 1;
}

static int step_genesisHash() {
  if (all_zero_key(current_txn.genesisHash)) {
    return 0;
  }

  if (strncmp(current_txn.genesisID, default_genesisID, sizeof(current_txn.genesisID)) == 0 ||
      current_txn.genesisID[0] == '\0') {
    if (memcmp(current_txn.genesisHash, default_genesisHash, sizeof(current_txn.genesisHash)) == 0) {
      return 0;
    }
  }

  char buf[45];
  base64_encode((const char*) current_txn.genesisHash, sizeof(current_txn.genesisHash), buf, sizeof(buf));
  ui_text_put(buf);
  return 1;
}

static int step_note() {
  if (current_txn.note_len == 0) {
    return 0;
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%d bytes", current_txn.note_len);
  ui_text_put(buf);
  return 1;
}

static int step_receiver() {
  checksum_and_put_text(current_txn.payment.receiver);
  return 1;
}

static int step_amount() {
  ui_text_put(amount_to_str(current_txn.payment.amount, ALGORAND_DECIMALS));
  return 1;
}

static int step_close() {
  if (all_zero_key(current_txn.payment.close)) {
    return 0;
  }

  checksum_and_put_text(current_txn.payment.close);
  return 1;
}

static int step_votepk() {
  char buf[45];
  base64_encode((const char*) current_txn.keyreg.votepk, sizeof(current_txn.keyreg.votepk), buf, sizeof(buf));
  ui_text_put(buf);
  return 1;
}

static int step_vrfpk() {
  char buf[45];
  base64_encode((const char*) current_txn.keyreg.vrfpk, sizeof(current_txn.keyreg.vrfpk), buf, sizeof(buf));
  ui_text_put(buf);
  return 1;
}

static int step_votefirst() {
  ui_text_put_u64(current_txn.keyreg.voteFirst);
  return 1;
}

static int step_votelast() {
  ui_text_put_u64(current_txn.keyreg.voteLast);
  return 1;
}

static int step_keydilution() {
  ui_text_put_u64(current_txn.keyreg.keyDilution);
  return 1;
}

static int step_participating() {
  if (current_txn.keyreg.nonpartFlag) {
    ui_text_put("No");
  } else {
    ui_text_put("Yes");
  }
  return 1;
}

static int step_asset_xfer_id() {
  const algo_asset_info_t *asa = algo_asa_get(current_txn.asset_xfer.id);
  const char *id = u64str(current_txn.asset_xfer.id);

  if (asa == NULL) {
    snprintf(text, sizeof(text), "#%s", id);
  } else {
    snprintf(text, sizeof(text), "%s (#%s)", asa->name, id);
  }
  return 1;
}

static int step_asset_xfer_amount() {
  if(is_opt_in_tx()){
    return 0;
  }

  const algo_asset_info_t *asa = algo_asa_get(current_txn.asset_xfer.id);
  if (asa != NULL) {
    snprintf(caption, sizeof(caption), "Amount (%s)", asa->unit);
    ui_text_put(amount_to_str(current_txn.asset_xfer.amount, asa->decimals));
  } else {
    snprintf(caption, sizeof(caption), "Amount (base unit)");
    ui_text_put_u64(current_txn.asset_xfer.amount);
  }
  return 1;
}

static int step_asset_xfer_sender() {
  if (all_zero_key(current_txn.asset_xfer.sender)) {
    return 0;
  }


  checksum_and_put_text(current_txn.asset_xfer.sender);
  return 1;
}

static int step_asset_xfer_receiver() {
  if (all_zero_key(current_txn.asset_xfer.receiver) ||
      is_opt_in_tx()) {
    return 0;
  }


  checksum_and_put_text(current_txn.asset_xfer.receiver);
  return 1;
}

static int step_asset_xfer_close() {
  if (all_zero_key(current_txn.asset_xfer.close)) {
    return 0;
  }

  checksum_and_put_text(current_txn.asset_xfer.close);
  return 1;
}

static int step_asset_freeze_id() {
  ui_text_put_u64(current_txn.asset_freeze.id);
  return 1;
}

static int step_asset_freeze_account() {
  if (all_zero_key(current_txn.asset_freeze.account)) {
    return 0;
  }

  checksum_and_put_text(current_txn.asset_freeze.account);
  return 1;
}

static int step_asset_freeze_flag() {
  if (current_txn.asset_freeze.flag) {
    ui_text_put("Frozen");
  } else {
    ui_text_put("Unfrozen");
  }
  return 1;
}

static int step_asset_config_id() {
  if (current_txn.asset_config.id == 0) {
    ui_text_put("Create");
  } else {
    ui_text_put_u64(current_txn.asset_config.id);
  }
  return 1;
}

static int step_asset_config_total() {
  if (current_txn.asset_config.id != 0 && current_txn.asset_config.params.total == 0) {
    return 0;
  }

  ui_text_put_u64(current_txn.asset_config.params.total);
  return 1;
}

static int step_asset_config_default_frozen() {
  if (current_txn.asset_config.id != 0 && current_txn.asset_config.params.default_frozen == 0) {
    return 0;
  }

  if (current_txn.asset_config.params.default_frozen) {
    ui_text_put("Frozen");
  } else {
    ui_text_put("Unfrozen");
  }
  return 1;
}

static int step_asset_config_unitname() {
  if (current_txn.asset_config.params.unitname[0] == '\0') {
    return 0;
  }

  ui_text_put(current_txn.asset_config.params.unitname);
  return 1;
}

static int step_asset_config_decimals() {
  if (current_txn.asset_config.params.decimals == 0) {
    return 0;
  }

  ui_text_put_u64(current_txn.asset_config.params.decimals);
  return 1;
}

static int step_asset_config_assetname() {
  if (current_txn.asset_config.params.assetname[0] == '\0') {
    return 0;
  }

  ui_text_put(current_txn.asset_config.params.assetname);
  return 1;
}

static int step_asset_config_url() {
  if (current_txn.asset_config.params.url[0] == '\0') {
    return 0;
  }

  ui_text_put(current_txn.asset_config.params.url);
  return 1;
}

static int step_asset_config_metadata_hash() {
  if (all_zero_key(current_txn.asset_config.params.metadata_hash)) {
    return 0;
  }

  char buf[45];
  base64_encode((const char*) current_txn.asset_config.params.metadata_hash, sizeof(current_txn.asset_config.params.metadata_hash), buf, sizeof(buf));
  ui_text_put(buf);
  return 1;
}

static int step_asset_config_addr_helper(uint8_t *addr) {
  if (all_zero_key(addr)) {
    ui_text_put("Zero");
  } else {
    checksum_and_put_text(addr);
  }
  return 1;
}

static int step_asset_config_manager() {
  return step_asset_config_addr_helper(current_txn.asset_config.params.manager);
}

static int step_asset_config_reserve() {
  return step_asset_config_addr_helper(current_txn.asset_config.params.reserve);
}

static int step_asset_config_freeze() {
  return step_asset_config_addr_helper(current_txn.asset_config.params.freeze);
}

static int step_asset_config_clawback() {
  return step_asset_config_addr_helper(current_txn.asset_config.params.clawback);
}

static int step_application_id() {
  ui_text_put_u64(current_txn.application.id);
  return 1;
}

static int step_application_oncompletion() {
  switch (current_txn.application.oncompletion) {
  case NOOPOC:
    ui_text_put("NoOp");
    break;

  case OPTINOC:
    ui_text_put("OptIn");
    break;

  case CLOSEOUTOC:
    ui_text_put("CloseOut");
    break;

  case CLEARSTATEOC:
    ui_text_put("ClearState");
    break;

  case UPDATEAPPOC:
    ui_text_put("UpdateApp");
    break;

  case DELETEAPPOC:
    ui_text_put("DeleteApp");
    break;

  default:
    ui_text_put("Unknown");
  }
  return 1;
}

static int display_schema(struct state_schema *schema) {
  // Don't display if nonzero schema cannot be valid
  if (current_txn.application.id != 0) {
    return 0;
  }

  char schm_repr[65];
  char uint_part[32];
  char byte_part[32];
  snprintf(uint_part, sizeof(uint_part), "uint: %s", u64str(schema->num_uint));
  snprintf(byte_part, sizeof(byte_part), "byte: %s", u64str(schema->num_byteslice));
  snprintf(schm_repr, sizeof(schm_repr), "%s, %s",   uint_part, byte_part);
  ui_text_put(schm_repr);
  return 1;
}

static int step_application_global_schema() {
  return display_schema(&current_txn.application.global_schema);
}

static int step_application_local_schema() {
  return display_schema(&current_txn.application.local_schema);
}

static int display_prog(uint8_t *prog_bytes, size_t prog_len) {
  // Don't display if nonzero program cannot be valid
  if (current_txn.application.id != 0 && current_txn.application.oncompletion != UPDATEAPPOC) {
    return 0;
  }

  ui_text_put(b64hash_data((unsigned char *)prog_bytes, prog_len));
  return 1;
}

static int step_application_approve_prog() {
  return display_prog(current_txn.application.aprog, current_txn.application.aprog_len);
}

static int step_application_clear_prog() {
  return display_prog(current_txn.application.cprog, current_txn.application.cprog_len);
}

static int step_application_account(unsigned int idx) {
  if (idx >= current_txn.application.num_accounts) {
    return 0;
  }

  checksum_and_put_text(current_txn.application.accounts[idx]);
  return 1;
}

static int step_application_account_0() {
  return step_application_account(0);
}

static int step_application_account_1() {
  return step_application_account(1);
}

static int step_application_foreign_app(unsigned int idx) {
  if (idx >= current_txn.application.num_foreign_apps) {
    return 0;
  }

  ui_text_put_u64(current_txn.application.foreign_apps[idx]);
  return 1;
}

static int step_application_foreign_app_0() {
  return step_application_foreign_app(0);
}

static int step_application_foreign_asset(unsigned int idx) {
  if (idx >= current_txn.application.num_foreign_assets) {
    return 0;
  }

  ui_text_put_u64(current_txn.application.foreign_assets[idx]);
  return 1;
}

static int step_application_foreign_asset_0() {
  return step_application_foreign_asset(0);
}

static int step_application_arg(unsigned int idx) {
  if (idx >= current_txn.application.num_app_args) {
    return 0;
  }

  ui_text_put(b64hash_data(current_txn.application.app_args[idx], current_txn.application.app_args_len[idx]));
  return 1;
}

static int step_application_arg_0() {
  return step_application_arg(0);
}

static int step_application_arg_1() {
  return step_application_arg(1);
}

screen_t const screen_table[] = {
  {"Txn type", &step_txn_type, ALL_TYPES},
  {"Sender", &step_sender, ALL_TYPES},
  {"Rekey to", &step_rekey, ALL_TYPES},
  {"Fee (Alg)", &step_fee, ALL_TYPES},
  // {"First valid", step_firstvalid, ALL_TYPES},
  // {"Last valid", step_lastvalid, ALL_TYPES},
  {"Genesis ID", &step_genesisID, ALL_TYPES},
  {"Genesis hash", &step_genesisHash, ALL_TYPES},
  {"Note", &step_note, ALL_TYPES},

  {"Receiver", &step_receiver, PAYMENT},
  {"Amount (Alg)", step_amount, PAYMENT},
  {"Close to", &step_close, PAYMENT},

  {"Vote PK", &step_votepk, KEYREG},
  {"VRF PK", &step_vrfpk, KEYREG},
  {"Vote first", &step_votefirst, KEYREG},
  {"Vote last", &step_votelast, KEYREG},
  {"Key dilution", &step_keydilution, KEYREG},
  {"Participating", &step_participating, KEYREG},

  {"Asset ID", &step_asset_xfer_id, ASSET_XFER},
  {SCREEN_DYN_CAPTION, &step_asset_xfer_amount, ASSET_XFER},
  {"Asset src", &step_asset_xfer_sender, ASSET_XFER},
  {"Asset dst", &step_asset_xfer_receiver, ASSET_XFER},
  {"Asset close", &step_asset_xfer_close, ASSET_XFER},

  {"Asset ID", &step_asset_freeze_id, ASSET_FREEZE},
  {"Asset account", &step_asset_freeze_account, ASSET_FREEZE},
  {"Freeze flag", &step_asset_freeze_flag, ASSET_FREEZE},

  {"Asset ID", &step_asset_config_id, ASSET_CONFIG},
  {"Total units", &step_asset_config_total, ASSET_CONFIG},
  {"Default frozen", &step_asset_config_default_frozen, ASSET_CONFIG},
  {"Unit name", &step_asset_config_unitname, ASSET_CONFIG},
  {"Decimals", &step_asset_config_decimals, ASSET_CONFIG},
  {"Asset name", &step_asset_config_assetname, ASSET_CONFIG},
  {"URL", &step_asset_config_url, ASSET_CONFIG},
  {"Metadata hash", &step_asset_config_metadata_hash, ASSET_CONFIG},
  {"Manager", &step_asset_config_manager, ASSET_CONFIG},
  {"Reserve", &step_asset_config_reserve, ASSET_CONFIG},
  {"Freezer", &step_asset_config_freeze, ASSET_CONFIG},
  {"Clawback", &step_asset_config_clawback, ASSET_CONFIG},

  {"App ID", &step_application_id, APPLICATION},
  {"On completion", &step_application_oncompletion, APPLICATION},
  {"Foreign app 0", &step_application_foreign_app_0, APPLICATION},
  {"Foreign asset 0", &step_application_foreign_asset_0, APPLICATION},
  {"App account 0", &step_application_account_0, APPLICATION},
  {"App account 1", &step_application_account_1, APPLICATION},
  {"App arg 0 (sha256)", &step_application_arg_0, APPLICATION},
  {"App arg 1 (sha256)", &step_application_arg_1, APPLICATION},
  {"Global schema", &step_application_global_schema, APPLICATION},
  {"Local schema", &step_application_local_schema, APPLICATION},
  {"Apprv (sha256)", &step_application_approve_prog, APPLICATION},
  {"Clear (sha256)", &step_application_clear_prog, APPLICATION},
};

const uint8_t screen_num = sizeof(screen_table) / sizeof(screen_t);

#include <string.h>
#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"
#include "algo_tx.h"
#include "algo_addr.h"
#include "algo_keys.h"
#include "base64.h"
#include "glyphs.h"

static char *
u64str(uint64_t v)
{
  static char buf[24];

  char *p = &buf[sizeof(buf)];
  *(--p) = '\0';

  if (v == 0) {
    *(--p) = '0';
    return p;
  }

  while (v > 0) {
    *(--p) = '0' + (v % 10);
    v = v/10;
  }

  return p;
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

static int step_txn_type() {
  switch (current_txn.type) {
  case PAYMENT:
    ui_text_put("Payment");
    break;

  case KEYREG:
    ui_text_put("Key reg");
    break;

  case ASSET_XFER:
    ui_text_put("Asset xfer");
    break;

  case ASSET_FREEZE:
    ui_text_put("Asset freeze");
    break;

  case ASSET_CONFIG:
    ui_text_put("Asset config");
    break;

  default:
    ui_text_put("Unknown");
  }
  return 1;
}

static int step_sender() {
  if (os_memcmp(publicKey, current_txn.sender, sizeof(current_txn.sender)) == 0) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.sender, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_rekey() {
  if (all_zero_key(current_txn.rekey)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.rekey, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_fee() {
  ui_text_put(u64str(current_txn.fee));
  return 1;
}

static int step_firstvalid() {
  ui_text_put(u64str(current_txn.firstValid));
  return 1;
}

static int step_lastvalid() {
  ui_text_put(u64str(current_txn.lastValid));
  return 1;
}

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

  ui_text_putn(current_txn.genesisID, sizeof(current_txn.genesisID));
  return 1;
}

static int step_genesisHash() {
  if (all_zero_key(current_txn.genesisHash)) {
    return 0;
  }

  if (strncmp(current_txn.genesisID, default_genesisID, sizeof(current_txn.genesisID)) == 0 ||
      current_txn.genesisID[0] == '\0') {
    if (os_memcmp(current_txn.genesisHash, default_genesisHash, sizeof(current_txn.genesisHash)) == 0) {
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
  char checksummed[65];
  checksummed_addr(current_txn.payment.receiver, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_amount() {
  ui_text_put(u64str(current_txn.payment.amount));
  return 1;
}

static int step_close() {
  if (all_zero_key(current_txn.payment.close)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.payment.close, checksummed);
  ui_text_put(checksummed);
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
  ui_text_put(u64str(current_txn.keyreg.voteFirst));
  return 1;
}

static int step_votelast() {
  ui_text_put(u64str(current_txn.keyreg.voteLast));
  return 1;
}

static int step_nonpart() {
  if (current_txn.keyreg.nonpartFlag) {
    ui_text_put("True");
  } else {
    ui_text_put("False");
  }
  return 1;
}

static int step_asset_xfer_id() {
  ui_text_put(u64str(current_txn.asset_xfer.id));
  return 1;
}

static int step_asset_xfer_amount() {
  ui_text_put(u64str(current_txn.asset_xfer.amount));
  return 1;
}

static int step_asset_xfer_sender() {
  if (all_zero_key(current_txn.asset_xfer.sender)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.asset_xfer.sender, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_asset_xfer_receiver() {
  if (all_zero_key(current_txn.asset_xfer.receiver)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.asset_xfer.receiver, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_asset_xfer_close() {
  if (all_zero_key(current_txn.asset_xfer.close)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.asset_xfer.close, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_asset_freeze_id() {
  ui_text_put(u64str(current_txn.asset_freeze.id));
  return 1;
}

static int step_asset_freeze_account() {
  if (all_zero_key(current_txn.asset_freeze.account)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.asset_freeze.account, checksummed);
  ui_text_put(checksummed);
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
    ui_text_put(u64str(current_txn.asset_config.id));
  }
  return 1;
}

static int step_asset_config_total() {
  if (current_txn.asset_config.id != 0 && current_txn.asset_config.params.total == 0) {
    return 0;
  }

  ui_text_put(u64str(current_txn.asset_config.params.total));
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

  ui_text_putn(current_txn.asset_config.params.unitname, sizeof(current_txn.asset_config.params.unitname));
  return 1;
}

static int step_asset_config_decimals() {
  if (current_txn.asset_config.params.decimals == 0) {
    return 0;
  }

  ui_text_put(u64str(current_txn.asset_config.params.decimals));
  return 1;
}

static int step_asset_config_assetname() {
  if (current_txn.asset_config.params.assetname[0] == '\0') {
    return 0;
  }

  ui_text_putn(current_txn.asset_config.params.assetname, sizeof(current_txn.asset_config.params.assetname));
  return 1;
}

static int step_asset_config_url() {
  if (current_txn.asset_config.params.url[0] == '\0') {
    return 0;
  }

  ui_text_putn(current_txn.asset_config.params.url, sizeof(current_txn.asset_config.params.url));
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
    char checksummed[65];
    checksummed_addr(addr, checksummed);
    ui_text_put(checksummed);
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

static int step_always_displayed() {
  return 1;
}

UX_STEP_NOCB_INIT(all_type_0, bn,          step_txn_type(),    {"Txn type",     text});
UX_STEP_NOCB_INIT(all_type_1, bnnn_paging, step_sender(),      {"Sender",       text});
UX_STEP_NOCB_INIT(all_type_2, bnnn_paging, step_rekey(),       {"RekeyTo",      text});
UX_STEP_NOCB_INIT(all_type_3, bn,          step_fee(),         {"Fee (uAlg)",   text});
UX_STEP_NOCB_INIT(all_type_4, bn,          step_firstvalid(),  {"First valid",  text});
UX_STEP_NOCB_INIT(all_type_5, bn,          step_lastvalid(),   {"Last valid",   text});
UX_STEP_NOCB_INIT(all_type_6, bn,          step_genesisID(),   {"Genesis ID",   text});
UX_STEP_NOCB_INIT(all_type_7, bnnn_paging, step_genesisHash(), {"Genesis hash", text});
UX_STEP_NOCB_INIT(all_type_8, bn,          step_note(),        {"Note",         text});

UX_STEP_NOCB_INIT(payment_type_0, bnnn_paging, step_receiver(), {"Receiver",      text});
UX_STEP_NOCB_INIT(payment_type_1, bn,          step_amount(),   {"Amount (uAlg)", text});
UX_STEP_NOCB_INIT(payment_type_2, bnnn_paging, step_close(),    {"Close to",      text});

UX_STEP_NOCB_INIT(keyreg_type_0, bnnn_paging, step_votepk(),    {"Vote PK",          text});
UX_STEP_NOCB_INIT(keyreg_type_1, bnnn_paging, step_vrfpk(),     {"VRF PK",           text});
UX_STEP_NOCB_INIT(keyreg_type_2, bn,          step_votefirst(), {"Vote first",       text});
UX_STEP_NOCB_INIT(keyreg_type_3, bn,          step_votelast(),  {"Vote last",        text});
UX_STEP_NOCB_INIT(keyreg_type_4, bn,          step_nonpart(),   {"Nonparticipating", text});

UX_STEP_NOCB_INIT(axfer_type_0, bn,          step_asset_xfer_id(),       {"Asset ID",   text});
UX_STEP_NOCB_INIT(axfer_type_1, bn,          step_asset_xfer_amount(),   {"Asset amt",   text});
UX_STEP_NOCB_INIT(axfer_type_2, bnnn_paging, step_asset_xfer_sender(),   {"Asset src",   text});
UX_STEP_NOCB_INIT(axfer_type_3, bnnn_paging, step_asset_xfer_receiver(), {"Asset dst",   text});
UX_STEP_NOCB_INIT(axfer_type_4, bnnn_paging, step_asset_xfer_close(),    {"Asset close", text});

UX_STEP_NOCB_INIT(afrz_type_0, bn,          step_asset_freeze_id(),      {"Asset ID",      text});
UX_STEP_NOCB_INIT(afrz_type_1, bnnn_paging, step_asset_freeze_account(), {"Asset account", text});
UX_STEP_NOCB_INIT(afrz_type_2, bn,          step_asset_freeze_flag(),    {"Freeze flag",   text});

UX_STEP_NOCB_INIT(acfg_type_0,  bn,          step_asset_config_id(),             {"Asset ID",       text});
UX_STEP_NOCB_INIT(acfg_type_1,  bn,          step_asset_config_total(),          {"Total units",    text});
UX_STEP_NOCB_INIT(acfg_type_2,  bn,          step_asset_config_default_frozen(), {"Default frozen", text});
UX_STEP_NOCB_INIT(acfg_type_3,  bnnn_paging, step_asset_config_unitname(),       {"Unit name",      text});
UX_STEP_NOCB_INIT(acfg_type_4,  bn,          step_asset_config_decimals(),       {"Decimals",       text});
UX_STEP_NOCB_INIT(acfg_type_5,  bnnn_paging, step_asset_config_assetname(),      {"Asset name",     text});
UX_STEP_NOCB_INIT(acfg_type_6,  bnnn_paging, step_asset_config_url(),            {"URL",            text});
UX_STEP_NOCB_INIT(acfg_type_7,  bnnn_paging, step_asset_config_metadata_hash(),  {"Metadata hash",  text});
UX_STEP_NOCB_INIT(acfg_type_8,  bnnn_paging, step_asset_config_manager(),        {"Manager",        text});
UX_STEP_NOCB_INIT(acfg_type_9,  bnnn_paging, step_asset_config_reserve(),        {"Reserve",        text});
UX_STEP_NOCB_INIT(acfg_type_10, bnnn_paging, step_asset_config_freeze(),         {"Freezer",        text});
UX_STEP_NOCB_INIT(acfg_type_11, bnnn_paging, step_asset_config_clawback(),       {"Clawback",       text});

UX_STEP(approv_0, pbb, NULL, 0, txn_approve(), NULL, {&C_icon_validate_14, "Sign",   "transaction"});
UX_STEP(approv_1, pbb, NULL, 0, txn_deny(),    NULL, {&C_icon_crossmark,   "Cancel", "signature"});

typedef struct typed_step {
  int type;
  int (*check_display)(void);
  const ux_flow_step_t * const step;
} typed_step_t;

const typed_step_t typed_steps[] = {
  {ALL_TYPES, &step_txn_type,    &all_type_0},
  {ALL_TYPES, &step_sender,      &all_type_1},
  {ALL_TYPES, &step_rekey,       &all_type_2},
  {ALL_TYPES, &step_fee,         &all_type_3},
  {ALL_TYPES, &step_firstvalid,  &all_type_4},
  {ALL_TYPES, &step_lastvalid,   &all_type_5},
  {ALL_TYPES, &step_genesisID,   &all_type_6},
  {ALL_TYPES, &step_genesisHash, &all_type_7},
  {ALL_TYPES, &step_note,        &all_type_8},

  {PAYMENT, &step_receiver, &payment_type_0},
  {PAYMENT, &step_amount,   &payment_type_1},
  {PAYMENT, &step_close,    &payment_type_2},

  {KEYREG, &step_votepk,    &keyreg_type_0},
  {KEYREG, &step_vrfpk,     &keyreg_type_1},
  {KEYREG, &step_votefirst, &keyreg_type_2},
  {KEYREG, &step_votelast,  &keyreg_type_3},
  {KEYREG, &step_nonpart,   &keyreg_type_4},

  {ASSET_XFER, &step_asset_xfer_id,       &axfer_type_0},
  {ASSET_XFER, &step_asset_xfer_amount,   &axfer_type_1},
  {ASSET_XFER, &step_asset_xfer_sender,   &axfer_type_2},
  {ASSET_XFER, &step_asset_xfer_receiver, &axfer_type_3},
  {ASSET_XFER, &step_asset_xfer_close,    &axfer_type_4},

  {ASSET_FREEZE, &step_asset_freeze_id,      &afrz_type_0},
  {ASSET_FREEZE, &step_asset_freeze_account, &afrz_type_1},
  {ASSET_FREEZE, &step_asset_freeze_flag,    &afrz_type_2},

  {ASSET_CONFIG, &step_asset_config_id,             &acfg_type_0},
  {ASSET_CONFIG, &step_asset_config_total,          &acfg_type_1},
  {ASSET_CONFIG, &step_asset_config_default_frozen, &acfg_type_2},
  {ASSET_CONFIG, &step_asset_config_unitname,       &acfg_type_3},
  {ASSET_CONFIG, &step_asset_config_decimals,       &acfg_type_4},
  {ASSET_CONFIG, &step_asset_config_assetname,      &acfg_type_5},
  {ASSET_CONFIG, &step_asset_config_url,            &acfg_type_6},
  {ASSET_CONFIG, &step_asset_config_metadata_hash,  &acfg_type_7},
  {ASSET_CONFIG, &step_asset_config_manager,        &acfg_type_8},
  {ASSET_CONFIG, &step_asset_config_reserve,        &acfg_type_9},
  {ASSET_CONFIG, &step_asset_config_freeze,         &acfg_type_10},
  {ASSET_CONFIG, &step_asset_config_clawback,       &acfg_type_11},

  {ALL_TYPES, &step_always_displayed, &approv_0},
  {ALL_TYPES, &step_always_displayed, &approv_1},
};

#define COUNT(array) \
  (sizeof(array) / sizeof(array[0]))

// Enough room for all steps + FLOW_END_STEP
const ux_flow_step_t *computed_flow[COUNT(typed_steps) + 1];

void
ui_txn()
{
  PRINTF("Transaction:\n");
  PRINTF("  Type: %d\n", current_txn.type);
  PRINTF("  Sender: %.*h\n", 32, current_txn.sender);
  PRINTF("  Fee: %s\n", u64str(current_txn.fee));
  PRINTF("  First valid: %s\n", u64str(current_txn.firstValid));
  PRINTF("  Last valid: %s\n", u64str(current_txn.lastValid));
  PRINTF("  Genesis ID: %.*s\n", 32, current_txn.genesisID);
  PRINTF("  Genesis hash: %.*h\n", 32, current_txn.genesisHash);
  if (current_txn.type == PAYMENT) {
    PRINTF("  Receiver: %.*h\n", 32, current_txn.payment.receiver);
    PRINTF("  Amount: %s\n", u64str(current_txn.payment.amount));
    PRINTF("  Close to: %.*h\n", 32, current_txn.payment.close);
  }
  if (current_txn.type == KEYREG) {
    PRINTF("  Vote PK: %.*h\n", 32, current_txn.keyreg.votepk);
    PRINTF("  VRF PK: %.*h\n", 32, current_txn.keyreg.vrfpk);
  }

  // Compute the flow
  size_t next_computed_step = 0;
  for (size_t i = 0; i < COUNT(typed_steps); i++) {
    // For this step, if it's for all types or the current txn's type, and the
    // step has something to display, add it to the flow.
    int txtype = typed_steps[i].type;
    if (txtype == ALL_TYPES || txtype == current_txn.type) {
      int (*check_display)(void) = (int (*)(void)) PIC(typed_steps[i].check_display);
      if (check_display()) {
        computed_flow[next_computed_step++] = typed_steps[i].step;
      }
    }
  }
  computed_flow[next_computed_step] = FLOW_END_STEP;

  // Start flow
  if (G_ux.stack_count == 0) {
    ux_stack_push();
  }
  ux_flow_init(0, (const ux_flow_step_t * const *)computed_flow, NULL);
}

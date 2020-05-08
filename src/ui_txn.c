#include <string.h>
#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"
#include "algo_tx.h"
#include "algo_addr.h"
#include "algo_keys.h"
#include "base64.h"
#include "glyphs.h"

static unsigned int ux_replay_state;
static bool ux_step_replay;

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

static char *
b64hash_data(unsigned char *data, size_t data_len) {
  static char b64hash[45];
  static unsigned char hash[32];

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
    ui_text_put("Asset xfer");
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

#if defined(TARGET_NANOX)
static unsigned int ux_last_step;

ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 0, bn,          step_txn_type(),    {"Txn type",     text});
ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 1, bnnn_paging, step_sender(),      {"Sender",       text});
ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 2, bnnn_paging, step_rekey(),       {"RekeyTo",      text});
ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 3, bn,          step_fee(),         {"Fee (uAlg)",   text});
ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 4, bn,          step_firstvalid(),  {"First valid",  text});
ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 5, bn,          step_lastvalid(),   {"Last valid",   text});
ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 6, bn,          step_genesisID(),   {"Genesis ID",   text});
ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 7, bnnn_paging, step_genesisHash(), {"Genesis hash", text});
ALGO_UX_STEP_NOCB_INIT(ALL_TYPES, 8, bn,          step_note(),        {"Note",         text});

ALGO_UX_STEP_NOCB_INIT(PAYMENT, 9,  bnnn_paging, step_receiver(), {"Receiver",      text});
ALGO_UX_STEP_NOCB_INIT(PAYMENT, 10, bn,          step_amount(),   {"Amount (uAlg)", text});
ALGO_UX_STEP_NOCB_INIT(PAYMENT, 11, bnnn_paging, step_close(),    {"Close to",      text});

ALGO_UX_STEP_NOCB_INIT(KEYREG, 12, bnnn_paging, step_votepk(),    {"Vote PK",          text});
ALGO_UX_STEP_NOCB_INIT(KEYREG, 13, bnnn_paging, step_vrfpk(),     {"VRF PK",           text});
ALGO_UX_STEP_NOCB_INIT(KEYREG, 14, bn,          step_votefirst(), {"Vote first",       text});
ALGO_UX_STEP_NOCB_INIT(KEYREG, 15, bn,          step_votelast(),  {"Vote last",        text});
ALGO_UX_STEP_NOCB_INIT(KEYREG, 16, bn,          step_nonpart(),   {"Nonparticipating", text});

ALGO_UX_STEP_NOCB_INIT(ASSET_XFER, 17, bn,          step_asset_xfer_id(),       {"Asset ID",   text});
ALGO_UX_STEP_NOCB_INIT(ASSET_XFER, 18, bn,          step_asset_xfer_amount(),   {"Asset amt",   text});
ALGO_UX_STEP_NOCB_INIT(ASSET_XFER, 19, bnnn_paging, step_asset_xfer_sender(),   {"Asset src",   text});
ALGO_UX_STEP_NOCB_INIT(ASSET_XFER, 20, bnnn_paging, step_asset_xfer_receiver(), {"Asset dst",   text});
ALGO_UX_STEP_NOCB_INIT(ASSET_XFER, 21, bnnn_paging, step_asset_xfer_close(),    {"Asset close", text});

ALGO_UX_STEP_NOCB_INIT(ASSET_FREEZE, 22, bn,          step_asset_freeze_id(),      {"Asset ID",      text});
ALGO_UX_STEP_NOCB_INIT(ASSET_FREEZE, 23, bnnn_paging, step_asset_freeze_account(), {"Asset account", text});
ALGO_UX_STEP_NOCB_INIT(ASSET_FREEZE, 24, bn,          step_asset_freeze_flag(),    {"Freeze flag",   text});

ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 25, bn,          step_asset_config_id(),             {"Asset ID",       text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 26, bn,          step_asset_config_total(),          {"Total units",    text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 27, bn,          step_asset_config_default_frozen(), {"Default frozen", text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 28, bnnn_paging, step_asset_config_unitname(),       {"Unit name",      text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 29, bn,          step_asset_config_decimals(),       {"Decimals",       text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 30, bnnn_paging, step_asset_config_assetname(),      {"Asset name",     text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 31, bnnn_paging, step_asset_config_url(),            {"URL",            text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 32, bnnn_paging, step_asset_config_metadata_hash(),  {"Metadata hash",  text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 33, bnnn_paging, step_asset_config_manager(),        {"Manager",        text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 34, bnnn_paging, step_asset_config_reserve(),        {"Reserve",        text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 35, bnnn_paging, step_asset_config_freeze(),         {"Freezer",        text});
ALGO_UX_STEP_NOCB_INIT(ASSET_CONFIG, 36, bnnn_paging, step_asset_config_clawback(),       {"Clawback",       text});

ALGO_UX_STEP(37, pbb, NULL, 0, txn_approve(), NULL, {&C_icon_validate_14, "Sign",   "transaction"});
ALGO_UX_STEP(38, pbb, NULL, 0, txn_deny(),    NULL, {&C_icon_crossmark,   "Cancel", "signature"});

const ux_flow_step_t * const ux_txn_flow [] = {
  &txn_flow_0,
  &txn_flow_1,
  &txn_flow_2,
  &txn_flow_3,
  &txn_flow_4,
  &txn_flow_5,
  &txn_flow_6,
  &txn_flow_7,
  &txn_flow_8,
  &txn_flow_9,
  &txn_flow_10,
  &txn_flow_11,
  &txn_flow_12,
  &txn_flow_13,
  &txn_flow_14,
  &txn_flow_15,
  &txn_flow_16,
  &txn_flow_17,
  &txn_flow_18,
  &txn_flow_19,
  &txn_flow_20,
  &txn_flow_21,
  &txn_flow_22,
  &txn_flow_23,
  &txn_flow_24,
  &txn_flow_25,
  &txn_flow_26,
  &txn_flow_27,
  &txn_flow_28,
  &txn_flow_29,
  &txn_flow_30,
  &txn_flow_31,
  &txn_flow_32,
  &txn_flow_33,
  &txn_flow_34,
  &txn_flow_35,
  &txn_flow_36,
  &txn_flow_37,
  &txn_flow_38,
  FLOW_END_STEP,
};
#endif // TARGET_NANOX

#if defined(TARGET_NANOS)
struct ux_step {
  // The display callback returns a non-zero value if it placed information
  // about the associated caption into lineBuffer, which should be displayed.
  // If it returns 0, the approval flow moves on to the next step.  The
  // callback is invoked only if the transaction type matches txtype.
  int txtype;
  const char *caption;
  int (*display)(void);
};

static unsigned int ux_current_step;
static const struct ux_step ux_steps[] = {
  { ALL_TYPES,    "Txn type",         &step_txn_type },
  { ALL_TYPES,    "Sender",           &step_sender },
  { ALL_TYPES,    "RekeyTo",          &step_rekey },
  { ALL_TYPES,    "Fee (uAlg)",       &step_fee },
  { ALL_TYPES,    "First valid",      &step_firstvalid },
  { ALL_TYPES,    "Last valid",       &step_lastvalid },
  { ALL_TYPES,    "Genesis ID",       &step_genesisID },
  { ALL_TYPES,    "Genesis hash",     &step_genesisHash },
  { ALL_TYPES,    "Note",             &step_note },
  { PAYMENT,      "Receiver",         &step_receiver },
  { PAYMENT,      "Amount (uAlg)",    &step_amount },
  { PAYMENT,      "Close to",         &step_close },
  { KEYREG,       "Vote PK",          &step_votepk },
  { KEYREG,       "VRF PK",           &step_vrfpk },
  { KEYREG,       "Vote first",       &step_votefirst },
  { KEYREG,       "Vote last",        &step_votelast },
  { KEYREG,       "Nonparticipating", &step_nonpart },
  { ASSET_XFER,   "Asset ID",         &step_asset_xfer_id },
  { ASSET_XFER,   "Asset amt",        &step_asset_xfer_amount },
  { ASSET_XFER,   "Asset src",        &step_asset_xfer_sender },
  { ASSET_XFER,   "Asset dst",        &step_asset_xfer_receiver },
  { ASSET_XFER,   "Asset close",      &step_asset_xfer_close },
  { ASSET_FREEZE, "Asset ID",         &step_asset_freeze_id },
  { ASSET_FREEZE, "Asset account",    &step_asset_freeze_account },
  { ASSET_FREEZE, "Freeze flag",      &step_asset_freeze_flag },
  { ASSET_CONFIG, "Asset ID",         &step_asset_config_id },
  { ASSET_CONFIG, "Total units",      &step_asset_config_total },
  { ASSET_CONFIG, "Default frozen",   &step_asset_config_default_frozen },
  { ASSET_CONFIG, "Unit name",        &step_asset_config_unitname },
  { ASSET_CONFIG, "Decimals",         &step_asset_config_decimals },
  { ASSET_CONFIG, "Asset name",       &step_asset_config_assetname },
  { ASSET_CONFIG, "URL",              &step_asset_config_url },
  { ASSET_CONFIG, "Metadata hash",    &step_asset_config_metadata_hash },
  { ASSET_CONFIG, "Manager",          &step_asset_config_manager },
  { ASSET_CONFIG, "Reserve",          &step_asset_config_reserve },
  { ASSET_CONFIG, "Freezer",          &step_asset_config_freeze },
  { ASSET_CONFIG, "Clawback",         &step_asset_config_clawback },

  { APPLICATION, "Apprv (sha256)", &step_application_approve_prog},
  { APPLICATION, "Clear (sha256)", &step_application_clear_prog},
};

static const bagl_element_t bagl_ui_approval_nanos[] = {
  { {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
    NULL, 0, 0, 0, NULL, NULL, NULL, },
  { {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
    "Sign transaction", 0, 0, 0, NULL, NULL, NULL, },
  { {BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
    NULL, 0, 0, 0, NULL, NULL, NULL, },
  { {BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK},
    NULL, 0, 0, 0, NULL, NULL, NULL, },
};

static unsigned int
bagl_ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter)
{
  switch (button_mask) {
  case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
    txn_approve();
    break;

  case BUTTON_EVT_RELEASED | BUTTON_LEFT:
    txn_deny();
    break;
  }
  return 0;
}

static char captionBuffer[32];

static const bagl_element_t bagl_ui_step_nanos[] = {
  { {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,
     0, 0},
    NULL, 0, 0, 0, NULL, NULL, NULL, },

  /* Caption */
  { {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000,
     BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
    captionBuffer, 0, 0, 0, NULL, NULL, NULL, },

  /* Value */
  { {BAGL_LABELINE, 0x02, 23, 26, 82, 11, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
     BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
    lineBuffer, 0, 0, 0, NULL, NULL, NULL, },

  { {BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000,
     0, BAGL_GLYPH_ICON_CROSS},
    NULL, 0, 0, 0, NULL, NULL, NULL, },
  { {BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000,
     0, BAGL_GLYPH_ICON_RIGHT},
    NULL, 0, 0, 0, NULL, NULL, NULL, },
};

static void bagl_ui_step_nanos_display();

static unsigned int
bagl_ui_step_nanos_button(unsigned int button_mask, unsigned int button_mask_counter)
{
  switch (button_mask) {
  case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
    if (ui_text_more()) {
      UX_REDISPLAY();
      return 0;
    }

    ux_current_step++;
    bagl_ui_step_nanos_display();
    return 0;

  case BUTTON_EVT_RELEASED | BUTTON_LEFT:
    txn_deny();
    return 0;
  }

  return 0;
}

static void
bagl_ui_step_nanos_display()
{
  while (1) {
    if (ux_current_step >= sizeof(ux_steps) / sizeof(ux_steps[0])) {
      UX_DISPLAY(bagl_ui_approval_nanos, NULL);
      return;
    }

    int txtype = ux_steps[ux_current_step].txtype;
    if (txtype == ALL_TYPES || txtype == current_txn.type) {
      const char* step_caption = (const char*) PIC(ux_steps[ux_current_step].caption);
      int (*step_display)(void) = (int (*)(void)) PIC(ux_steps[ux_current_step].display);
      if (step_display()) {
        snprintf(captionBuffer, sizeof(captionBuffer), "%s", step_caption);
        ui_text_more();
        UX_DISPLAY(bagl_ui_step_nanos, NULL);
        return;
      }
    }

    ux_current_step++;
  }
}
#endif // TARGET_NANOS

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

#if defined(TARGET_NANOS)
  ux_current_step = 0;
  bagl_ui_step_nanos_display();
#endif

#if defined(TARGET_NANOX)
  ux_last_step = 0;
  if (G_ux.stack_count == 0) {
    ux_stack_push();
  }
  ux_flow_init(0, ux_txn_flow, NULL);
#endif
}

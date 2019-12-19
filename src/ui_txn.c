#include <string.h>
#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"
#include "algo_tx.h"
#include "algo_addr.h"
#include "algo_keys.h"
#include "base64.h"

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

  ui_text_putn(current_txn.genesisID, sizeof(current_txn.genesisID));
  return 1;
}

static int step_genesisHash() {
  if (all_zero_key(current_txn.genesisHash)) {
    return 0;
  }

  if (strncmp(current_txn.genesisID, default_genesisID, sizeof(current_txn.genesisID)) == 0) {
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
  { ALL_TYPES,    "Txn type",       &step_txn_type },
  { ALL_TYPES,    "Sender",         &step_sender },
  { ALL_TYPES,    "Fee (uAlg)",     &step_fee },
  { ALL_TYPES,    "First valid",    &step_firstvalid },
  { ALL_TYPES,    "Last valid",     &step_lastvalid },
  { ALL_TYPES,    "Genesis ID",     &step_genesisID },
  { ALL_TYPES,    "Genesis hash",   &step_genesisHash },
  { ALL_TYPES,    "Note",           &step_note },
  { PAYMENT,      "Receiver",       &step_receiver },
  { PAYMENT,      "Amount (uAlg)",  &step_amount },
  { PAYMENT,      "Close to",       &step_close },
  { KEYREG,       "Vote PK",        &step_votepk },
  { KEYREG,       "VRF PK",         &step_vrfpk },
  { ASSET_XFER,   "Asset ID",       &step_asset_xfer_id },
  { ASSET_XFER,   "Asset amt",      &step_asset_xfer_amount },
  { ASSET_XFER,   "Asset src",      &step_asset_xfer_sender },
  { ASSET_XFER,   "Asset dst",      &step_asset_xfer_receiver },
  { ASSET_XFER,   "Asset close",    &step_asset_xfer_close },
  { ASSET_FREEZE, "Asset ID",       &step_asset_freeze_id },
  { ASSET_FREEZE, "Asset account",  &step_asset_freeze_account },
  { ASSET_FREEZE, "Freeze flag",    &step_asset_freeze_flag },
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

  ux_current_step = 0;
  bagl_ui_step_nanos_display();
}

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
  if (current_txn.type != PAYMENT) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.receiver, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_amount() {
  if (current_txn.type != PAYMENT) {
    return 0;
  }

  ui_text_put(u64str(current_txn.amount));
  return 1;
}

static int step_close() {
  if (current_txn.type != PAYMENT) {
    return 0;
  }

  if (all_zero_key(current_txn.close)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.close, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_votepk() {
  if (current_txn.type != KEYREG) {
    return 0;
  }

  char buf[45];
  base64_encode((const char*) current_txn.votepk, sizeof(current_txn.votepk), buf, sizeof(buf));
  ui_text_put(buf);
  return 1;
}

static int step_vrfpk() {
  if (current_txn.type != KEYREG) {
    return 0;
  }

  char buf[45];
  base64_encode((const char*) current_txn.vrfpk, sizeof(current_txn.vrfpk), buf, sizeof(buf));
  ui_text_put(buf);
  return 1;
}

static int step_asset_xfer_id() {
  if (current_txn.type != ASSET_XFER) {
    return 0;
  }

  ui_text_put(u64str(current_txn.asset_xfer_id));
  return 1;
}

static int step_asset_xfer_amount() {
  if (current_txn.type != ASSET_XFER) {
    return 0;
  }

  ui_text_put(u64str(current_txn.asset_xfer_amount));
  return 1;
}

static int step_asset_xfer_sender() {
  if (current_txn.type != ASSET_XFER) {
    return 0;
  }

  if (all_zero_key(current_txn.asset_xfer_sender)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.asset_xfer_sender, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_asset_xfer_receiver() {
  if (current_txn.type != ASSET_XFER) {
    return 0;
  }

  if (all_zero_key(current_txn.asset_xfer_receiver)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.asset_xfer_receiver, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_asset_xfer_close() {
  if (current_txn.type != ASSET_XFER) {
    return 0;
  }

  if (all_zero_key(current_txn.asset_xfer_close)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.asset_xfer_close, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_asset_freeze_id() {
  if (current_txn.type != ASSET_FREEZE) {
    return 0;
  }

  ui_text_put(u64str(current_txn.asset_freeze_id));
  return 1;
}

static int step_asset_freeze_account() {
  if (current_txn.type != ASSET_FREEZE) {
    return 0;
  }

  if (all_zero_key(current_txn.asset_freeze_account)) {
    return 0;
  }

  char checksummed[65];
  checksummed_addr(current_txn.asset_freeze_account, checksummed);
  ui_text_put(checksummed);
  return 1;
}

static int step_asset_freeze_flag() {
  if (current_txn.type != ASSET_FREEZE) {
    return 0;
  }

  if (current_txn.asset_freeze_flag) {
    ui_text_put("Frozen");
  } else {
    ui_text_put("Unfrozen");
  }
  return 1;
}

struct ux_step {
  // The display callback returns a non-zero value if it placed information
  // about the associated caption into lineBuffer, which should be displayed.
  // If it returns 0, the approval flow moves on to the next step.
  const char *caption;
  int (*display)(void);
};

static unsigned int ux_current_step;
static const struct ux_step ux_steps[] = {
  { "Txn type",       &step_txn_type },
  { "Sender",         &step_sender },
  { "Fee (uAlg)",     &step_fee },
  { "First valid",    &step_firstvalid },
  { "Last valid",     &step_lastvalid },
  { "Genesis ID",     &step_genesisID },
  { "Genesis hash",   &step_genesisHash },
  { "Note",           &step_note },
  { "Receiver",       &step_receiver },
  { "Amount (uAlg)",  &step_amount },
  { "Close to",       &step_close },
  { "Vote PK",        &step_votepk },
  { "VRF PK",         &step_vrfpk },
  { "Asset ID",       &step_asset_xfer_id },
  { "Asset amt",      &step_asset_xfer_amount },
  { "Asset src",      &step_asset_xfer_sender },
  { "Asset dst",      &step_asset_xfer_receiver },
  { "Asset close",    &step_asset_xfer_close },
  { "Asset ID",       &step_asset_freeze_id },
  { "Asset account",  &step_asset_freeze_account },
  { "Freeze flag",    &step_asset_freeze_flag },
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

    const char* step_caption = (const char*) PIC(ux_steps[ux_current_step].caption);
    int (*step_display)(void) = (int (*)(void)) PIC(ux_steps[ux_current_step].display);
    if (step_display()) {
      snprintf(captionBuffer, sizeof(captionBuffer), "%s", step_caption);
      ui_text_more();
      UX_DISPLAY(bagl_ui_step_nanos, NULL);
      return;
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
  PRINTF("  Receiver: %.*h\n", 32, current_txn.receiver);
  PRINTF("  Amount: %s\n", u64str(current_txn.amount));
  PRINTF("  Close to: %.*h\n", 32, current_txn.close);
  PRINTF("  Vote PK: %.*h\n", 32, current_txn.votepk);
  PRINTF("  VRF PK: %.*h\n", 32, current_txn.vrfpk);

  ux_current_step = 0;
  bagl_ui_step_nanos_display();
}

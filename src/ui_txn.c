#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"
#include "algo_tx.h"
#include "algo_addr.h"
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

#define DISPLAY_ELEMENTS(header) {                                              \
  { {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,  \
     0, 0},                                                                     \
    NULL, 0, 0, 0, NULL, NULL, NULL, },                                         \
  { {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000,          \
     BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},         \
    header, 0, 0, 0, NULL, NULL, NULL, },                                       \
  { {BAGL_LABELINE, 0x02, 23, 26, 82, 11, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,  \
     BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},      \
    lineBuffer, 0, 0, 0, NULL, NULL, NULL, },                                   \
  { {BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000,                 \
     0, BAGL_GLYPH_ICON_CROSS},                                                 \
    NULL, 0, 0, 0, NULL, NULL, NULL, },                                         \
  { {BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000,               \
     0, BAGL_GLYPH_ICON_RIGHT},                                                 \
    NULL, 0, 0, 0, NULL, NULL, NULL, },                                         \
}

#define DISPLAY_HANDLER(after_func)                                             \
  switch (button_mask) {                                                        \
  case BUTTON_EVT_RELEASED | BUTTON_RIGHT:                                      \
    if (ui_text_more()) {                                                       \
      UX_REDISPLAY();                                                           \
    } else {                                                                    \
      after_func();                                                             \
    }                                                                           \
    break;                                                                      \
  case BUTTON_EVT_RELEASED | BUTTON_LEFT:                                       \
    txn_deny();                                                                 \
    break;                                                                      \
  }                                                                             \
  return 0;

static void after_vrfpk(void) {
  UX_DISPLAY(bagl_ui_approval_nanos, NULL);
}

static const bagl_element_t bagl_ui_vrfpk_nanos[] = DISPLAY_ELEMENTS("VRF PK");
static unsigned int bagl_ui_vrfpk_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_vrfpk)
}

static void after_votepk(void) {
  char buf[45];
  base64_encode((const char*) current_txn.vrfpk, sizeof(current_txn.vrfpk), buf, sizeof(buf));
  ui_text_put(buf);
  ui_text_more();
  UX_DISPLAY(bagl_ui_vrfpk_nanos, NULL);
}

static const bagl_element_t bagl_ui_votepk_nanos[] = DISPLAY_ELEMENTS("Vote PK");
static unsigned int bagl_ui_votepk_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_votepk)
}

static void after_close(void) {
  UX_DISPLAY(bagl_ui_approval_nanos, NULL);
}

static const bagl_element_t bagl_ui_close_nanos[] = DISPLAY_ELEMENTS("Close to");
static unsigned int bagl_ui_close_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_close)
}

static void after_amount(void) {
  if (all_zero_key(current_txn.close)) {
    after_close();
  } else {
    char checksummed[65];
    checksummed_addr(current_txn.close, checksummed);
    ui_text_put(checksummed);
    ui_text_more();
    UX_DISPLAY(bagl_ui_close_nanos, NULL);
  }
}

static const bagl_element_t bagl_ui_amount_nanos[] = DISPLAY_ELEMENTS("Amount (uAlg)");
static unsigned int bagl_ui_amount_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_amount)
}

static void after_receiver(void) {
  ui_text_put(u64str(current_txn.amount));
  ui_text_more();
  UX_DISPLAY(bagl_ui_amount_nanos, NULL);
}

static const bagl_element_t bagl_ui_receiver_nanos[] = DISPLAY_ELEMENTS("Receiver");
static unsigned int bagl_ui_receiver_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_receiver)
}

static void after_genesisHash(void) {
  char checksummed[65];
  if (current_txn.type == PAYMENT) {
    checksummed_addr(current_txn.receiver, checksummed);
    ui_text_put(checksummed);
    ui_text_more();
    UX_DISPLAY(bagl_ui_receiver_nanos, NULL);
  } else if (current_txn.type == KEYREG) {
    base64_encode((const char*) current_txn.votepk, sizeof(current_txn.votepk), checksummed, sizeof(checksummed));
    ui_text_put(checksummed);
    ui_text_more();
    UX_DISPLAY(bagl_ui_votepk_nanos, NULL);
  } else {
    UX_DISPLAY(bagl_ui_approval_nanos, NULL);
  }
}

static const bagl_element_t bagl_ui_genesisHash_nanos[] = DISPLAY_ELEMENTS("Genesis hash");
static unsigned int bagl_ui_genesisHash_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_genesisHash)
}

static void after_genesisID(void) {
  if (all_zero_key(current_txn.genesisHash)) {
    after_genesisHash();
  } else {
    char buf[45];
    base64_encode((const char*) current_txn.genesisHash, sizeof(current_txn.genesisHash), buf, sizeof(buf));
    ui_text_put(buf);
    ui_text_more();
    UX_DISPLAY(bagl_ui_genesisHash_nanos, NULL);
  }
}

static const bagl_element_t bagl_ui_genesisID_nanos[] = DISPLAY_ELEMENTS("Genesis ID");
static unsigned int bagl_ui_genesisID_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_genesisID)
}

static void after_lastValid(void) {
  ui_text_put(current_txn.genesisID);
  ui_text_more();
  UX_DISPLAY(bagl_ui_genesisID_nanos, NULL);
}

static const bagl_element_t bagl_ui_lastValid_nanos[] = DISPLAY_ELEMENTS("Last valid");
static unsigned int bagl_ui_lastValid_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_lastValid)
}

static void after_firstValid(void) {
  ui_text_put(u64str(current_txn.lastValid));
  ui_text_more();
  UX_DISPLAY(bagl_ui_lastValid_nanos, NULL);
}

static const bagl_element_t bagl_ui_firstValid_nanos[] = DISPLAY_ELEMENTS("First valid");
static unsigned int bagl_ui_firstValid_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_firstValid)
}

static void after_fee(void) {
  ui_text_put(u64str(current_txn.firstValid));
  ui_text_more();
  UX_DISPLAY(bagl_ui_firstValid_nanos, NULL);
}

static const bagl_element_t bagl_ui_fee_nanos[] = DISPLAY_ELEMENTS("Fee (uAlg)");
static unsigned int bagl_ui_fee_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_fee)
}

static void after_sender(void) {
  ui_text_put(u64str(current_txn.fee));
  ui_text_more();
  UX_DISPLAY(bagl_ui_fee_nanos, NULL);
}

static const bagl_element_t bagl_ui_sender_nanos[] = DISPLAY_ELEMENTS("Sender");
static unsigned int bagl_ui_sender_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_sender)
}

static void after_txtype(void) {
  char checksummed[65];
  checksummed_addr(current_txn.sender, checksummed);
  ui_text_put(checksummed);
  ui_text_more();
  UX_DISPLAY(bagl_ui_sender_nanos, NULL);
}

static const bagl_element_t bagl_ui_txtype_nanos[] = DISPLAY_ELEMENTS("Txn type");
static unsigned int bagl_ui_txtype_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
  DISPLAY_HANDLER(after_txtype)
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
  PRINTF("  Genesis ID: %s\n", current_txn.genesisID);
  PRINTF("  Genesis hash: %.*h\n", 32, current_txn.genesisHash);
  PRINTF("  Receiver: %.*h\n", 32, current_txn.receiver);
  PRINTF("  Amount: %s\n", u64str(current_txn.amount));
  PRINTF("  Close to: %.*h\n", 32, current_txn.close);
  PRINTF("  Vote PK: %.*h\n", 32, current_txn.votepk);
  PRINTF("  VRF PK: %.*h\n", 32, current_txn.vrfpk);

  switch (current_txn.type) {
  case PAYMENT:
    ui_text_put("Payment");
    break;

  case KEYREG:
    ui_text_put("Key reg");
    break;

  default:
    ui_text_put("Unknown");
  }
  ui_text_more();
  UX_DISPLAY(bagl_ui_txtype_nanos, NULL);
}

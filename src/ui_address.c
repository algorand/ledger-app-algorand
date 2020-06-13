#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"
#include "algo_keys.h"
#include "algo_addr.h"

#if defined(TARGET_NANOS)
static const bagl_element_t
bagl_ui_address_nanos[] = {
  { {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,
     0, 0},
    NULL,
    0, 0, 0, NULL, NULL, NULL, },
  { {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000,
     BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
    "Public address",
    0, 0, 0, NULL, NULL, NULL, },
  { {BAGL_LABELINE, 0x02, 23, 26, 82, 11, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
     BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
    lineBuffer,
    0, 0, 0, NULL, NULL, NULL, },
  { {BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
     BAGL_GLYPH_ICON_CROSS},
    NULL,
    0, 0, 0, NULL, NULL, NULL, },
  { {BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
     BAGL_GLYPH_ICON_RIGHT},
    NULL,
    0, 0, 0, NULL, NULL, NULL, },
};

static unsigned int
bagl_ui_address_nanos_button(unsigned int button_mask, unsigned int button_mask_counter)
{
  switch (button_mask) {
  case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
    if (ui_text_more()) {
      UX_REDISPLAY();
    } else {
      ui_idle();
    }
    break;

  case BUTTON_EVT_RELEASED | BUTTON_LEFT:
    ui_idle();
    break;
  }
  return 0;
}

void
ui_address()
{
  step_address();
  if (ui_text_more()) {
    UX_DISPLAY(bagl_ui_address_nanos, NULL);
  } else {
    ui_idle();
  }
}
#endif

void
step_address()
{
  char checksummed[65];
  uint8_t publicKey[32];
  cx_ecfp_private_key_t privateKey;

  algorand_key_derive(0, &privateKey);
  algorand_public_key(&privateKey, publicKey);
  checksummed_addr(publicKey, checksummed);
  ui_text_put(checksummed);
}

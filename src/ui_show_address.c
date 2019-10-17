#include "ui_show_address.h"
#include "ui_idle.h"

//------------------------------------------------------------------------------

static void
ui_menu_public_address_action_goback(unsigned int value);

//------------------------------------------------------------------------------

static const ux_menu_entry_t ui_menu_public_address[] = {
  {NULL,  NULL,                                  0,  NULL,          "Address",                                        (const char*)ui_strings.show_address.address[0],   0,   0},
  {NULL,  NULL,                                  0,  NULL,          (const char*)ui_strings.show_address.address[1],  (const char*)ui_strings.show_address.address[2],   0,   0},
  {NULL,  NULL,                                  0,  NULL,          (const char*)ui_strings.show_address.address[3],  (const char*)ui_strings.show_address.address[4],   0,   0},
  {NULL,  NULL,                                  0,  NULL,          (const char*)ui_strings.show_address.address[5],  "",                                                0,   0},
  {NULL,  ui_menu_public_address_action_goback,  1,  &C_icon_back,  "Back",                                           NULL,                                             61,  40},
  UX_MENU_END
};

//------------------------------------------------------------------------------

void
ui_menu_show_address_display(unsigned int value)
{
  char address[65];
  uint8_t publicKey[32];
  unsigned int i;

  UNUSED(value);

  algorand_public_key(publicKey);
  checksummed_addr(publicKey, address);

  for (i = 0; i < 5; i++) {
    os_memmove(ui_strings.show_address.address[i], address + i * 10, 10);
    ui_strings.show_address.address[i][10] = 0;
  }
  os_memmove(ui_strings.show_address.address[5], address + 50, 8);
  ui_strings.show_address.address[5][8] = 0;

  UX_MENU_DISPLAY(0, ui_menu_public_address, NULL);
  return;
}

static void
ui_menu_public_address_action_goback(unsigned int value)
{
  ui_menu_main_display(0);
  return;
}

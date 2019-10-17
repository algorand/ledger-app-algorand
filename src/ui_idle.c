#include "ui_idle.h"
#include "ui_show_address.h"

//------------------------------------------------------------------------------

static const ux_menu_entry_t menu_main[];
static const ux_menu_entry_t menu_about[];

static const ux_menu_entry_t menu_main[] = {
  {NULL,        ui_menu_show_address_display,  0,  NULL,               "Address",   NULL,   0,   0},
  {menu_about,  NULL,                          0,  NULL,               "About",     NULL,   0,   0},
  {NULL,        os_sched_exit,                 0,  &C_icon_dashboard,  "Quit app",  NULL,  50,  29},
  UX_MENU_END
};

static const ux_menu_entry_t menu_about[] = {
  {NULL,       NULL,  0,  NULL,          "Version",  APPVERSION,   0,   0},
  {menu_main,  NULL,  1,  &C_icon_back,  "Back",     NULL,        61,  40},
  UX_MENU_END
};

//------------------------------------------------------------------------------

void
ui_idle()
{
  ui_menu_main_display(0);
  return;
}

void
ui_menu_main_display(unsigned int value)
{
  UX_MENU_DISPLAY(value, menu_main, NULL);
  return;
}

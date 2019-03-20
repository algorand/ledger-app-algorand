#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"

static const ux_menu_entry_t menu_top[];
static const ux_menu_entry_t menu_about[];

static const ux_menu_entry_t menu_about[] = {
  {NULL,        NULL,           0, NULL, "Version",  APPVERSION, 0, 0},
  {menu_top,    NULL,           1, NULL, "Back",     NULL,       0, 0},
  UX_MENU_END
};

static const ux_menu_entry_t menu_top[] = {
  {NULL,        ui_address,     0, NULL, "Address",  NULL,       0, 0},
  {menu_about,  NULL,           0, NULL, "About",    NULL,       0, 0},
  {NULL,        os_sched_exit,  0, NULL, "Quit app", NULL,       0, 0},
  UX_MENU_END
};

void
ui_idle()
{
  uiState = UI_IDLE;
  UX_MENU_DISPLAY(0, menu_top, NULL);
}

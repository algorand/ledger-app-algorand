#include "os.h"
#include "os_io_seproxyhal.h"
#include "algo_ui.h"

#if defined(TARGET_NANOX)
#include "algo_keys.h"
#include "ux.h"
#endif

#if defined(TARGET_NANOS)
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
#endif

#if defined(TARGET_NANOX)
UX_STEP_NOCB(ux_idle_flow_1_step, bn, {"Version", APPVERSION});
UX_STEP_NOCB_INIT(ux_idle_flow_2_step, bnnn_paging, step_address(), {"Address", text});
UX_STEP_VALID(ux_idle_flow_3_step, pb, os_sched_exit(-1), {&C_icon_dashboard, "Quit"});

const ux_flow_step_t * const ux_idle_flow [] = {
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step,
  FLOW_END_STEP,
};
#endif

void
ui_idle()
{
#if defined(TARGET_NANOX)
  // reserve a display stack slot if none yet
  if(G_ux.stack_count == 0) {
    ux_stack_push();
  }
  ux_flow_init(0, ux_idle_flow, NULL);
#endif
#if defined(TARGET_NANOS)
  UX_MENU_DISPLAY(0, menu_top, NULL);
#endif
}

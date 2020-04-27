#include "os.h"
#include "os_io_seproxyhal.h"
#include "algo_ui.h"

UX_STEP_NOCB(ux_idle_flow_1_step, bn, {"Version", APPVERSION});
UX_STEP_NOCB_INIT(ux_idle_flow_2_step, bnnn_paging, step_address(), {"Address", text});
UX_STEP_VALID(ux_idle_flow_3_step, pb, os_sched_exit(-1), {&C_icon_dashboard, "Quit"});

const ux_flow_step_t * const ux_idle_flow [] = {
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step,
  FLOW_END_STEP,
};

UX_STEP_NOCB(ux_loading_1_step, bn, {"Loading...", "Please wait"});

const ux_flow_step_t * const ux_loading_flow [] = {
  &ux_loading_1_step,
  FLOW_END_STEP,
};

void
ui_loading()
{
  // reserve a display stack slot if none yet
  if(G_ux.stack_count == 0) {
    ux_stack_push();
  }
  ux_flow_init(0, ux_loading_flow, NULL);
}

void
ui_idle()
{
  // reserve a display stack slot if none yet
  if(G_ux.stack_count == 0) {
    ux_stack_push();
  }
  ux_flow_init(0, ux_idle_flow, NULL);
}

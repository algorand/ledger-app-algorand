#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"

#include "algo_ui.h"
#include "algo_tx.h"
#include "algo_keys.h"
#include "algo_addr.h"


UX_FLOW_DEF_NOCB(
    ux_display_public_flow_1_step,
    pnn,
    {
      &C_icon_eye,
      "Verify",
      "address",
    });
UX_FLOW_DEF_NOCB(
    ux_display_public_flow_2_step,
    bnnn_paging,
    {
      .title = "Address",
      .text = text,
    });
UX_FLOW_DEF_VALID(
    ux_display_public_flow_3_step,
    pbb,
    address_approve(),
    {
      &C_icon_validate_14,
      "Approve",
      "address",
    });
UX_FLOW_DEF_VALID(
    ux_display_public_flow_4_step,
    pb,
    user_approval_denied(),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_address_approval_flow,
  &ux_display_public_flow_1_step,
  &ux_display_public_flow_2_step,
  &ux_display_public_flow_3_step,
  &ux_display_public_flow_4_step
);

void ui_address_approval(void)
{
  ux_flow_relayout();
  if (G_ux.stack_count == 0) {
    ux_stack_push();
  }
  ux_flow_init(0, ux_address_approval_flow, NULL);
}

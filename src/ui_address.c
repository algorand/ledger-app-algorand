#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"

#include "algo_ui.h"
#include "algo_tx.h"
#include "algo_keys.h"
#include "algo_addr.h"

static void address_approve(void)
{
  unsigned int tx = ALGORAND_PUBLIC_KEY_SIZE;
  memmove(G_io_apdu_buffer, public_key.data, ALGORAND_PUBLIC_KEY_SIZE);

  G_io_apdu_buffer[tx++] = 0x90;
  G_io_apdu_buffer[tx++] = 0x00;

  explicit_bzero(public_key.data, ALGORAND_PUBLIC_KEY_SIZE);
  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

  // Display back the original UX
  ui_idle();
}

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

#include <string.h>
#include "os.h"
#include "ux.h"

#include "algo_ui.h"
#include "algo_tx.h"
#include "algo_addr.h"
#include "algo_keys.h"
#include "algo_asa.h"
#include "base64.h"
#include "glyphs.h"
#include "ui_txn.h"
#include "str.h"

#define INSIDE_BORDERS 0
#define OUT_OF_BORDERS 1

static uint8_t current_state;
static int8_t current_data_index;

static void txn_approve(void)
{
  int sign_size = 0;
  unsigned int msg_len;

  msgpack_buf[0] = 'T';
  msgpack_buf[1] = 'X';
  msg_len = 2 + tx_encode(&current_txn, msgpack_buf+2, sizeof(msgpack_buf)-2);

  PRINTF("Signing message: %.*h\n", msg_len, msgpack_buf);
  PRINTF("Signing message: accountId:%d\n", current_txn.accountId);

  int error = algorand_sign_message(current_txn.accountId, &msgpack_buf[0], msg_len, G_io_apdu_buffer, &sign_size);
  if (error) {
    THROW(error);
  }

  G_io_apdu_buffer[sign_size++] = 0x90;
  G_io_apdu_buffer[sign_size++] = 0x00;


  // we've just signed the txn so we clear the static struct
  explicit_bzero(&current_txn, sizeof(current_txn));
  msgpack_next_off = 0;

  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, sign_size);

  // Display back the original UX
  ui_idle();
}

static bool set_state_data(bool forward) {
  // verify that we don't move backwards if the posinion is outside the array (before the start)
  if (current_data_index == -1 && forward == false){
    return false;
  }
  // verify that we don't move forward if the posinion is outside the array (after the end)
  if (current_data_index == screen_num && forward == true){
    return false;
  }

  do
  {
    current_data_index =  forward ? current_data_index+1 : current_data_index -1;
    if(screen_table[current_data_index].type == ALL_TYPES || screen_table[current_data_index].type == current_txn.type){
      format_function_t val_setter_func_ptr = (format_function_t)PIC(screen_table[current_data_index].value_setter);
      if (val_setter_func_ptr() == 0){
        continue;
      }
      if (screen_table[current_data_index].caption != SCREEN_DYN_CAPTION) {
              strncpy(caption,
                      (char*)PIC(screen_table[current_data_index].caption),
                      sizeof(caption));
      }
      PRINTF("caption: %s\n", caption);
      PRINTF("details: %s\n\n", text);
      return true;  
      
    }
  } while (current_data_index >= 0 &&
            current_data_index < screen_num);
  return false;
}

static void display_next_state(bool is_upper_border) {

    if(is_upper_border){
        if(current_state == OUT_OF_BORDERS){ // -> from first screen
            current_state = INSIDE_BORDERS;
            set_state_data(true);
            ux_flow_next();
        }
        else{
            if(set_state_data(false)){ // <- from middle, more screens available
                ux_flow_next();
            }
            else{ // <- from middle, no more screens available
                current_state = OUT_OF_BORDERS;
                ux_flow_prev();
            }
        }
    }
    else // walking over the second border
    {
        if(current_state == OUT_OF_BORDERS){ // <- from last screen
            current_state = INSIDE_BORDERS;
            set_state_data(false);
            ux_flow_prev();
        }
        else{
            if(set_state_data(true)){ // -> from middle, more screens available
                /*dirty hack to have coherent behavior on bnnn_paging when there are multiple screens*/
                G_ux.flow_stack[G_ux.stack_count-1].prev_index = G_ux.flow_stack[G_ux.stack_count-1].index-2;
                G_ux.flow_stack[G_ux.stack_count-1].index--;
                ux_flow_relayout();
                /*end of dirty hack*/
            }
            else{ // -> from middle, no more screens available
                current_state = OUT_OF_BORDERS;
                ux_flow_next();
            }
        }
    }

}

UX_STEP_NOCB(
    ux_confirm_tx_init_flow_step,
    pnn,
    {
      &C_icon_eye,
      "Review",
      "Transaction",
    });

UX_STEP_INIT(
    ux_init_upper_border,
    NULL,
    NULL,
    {
        display_next_state(true);
    });
UX_STEP_NOCB(
    ux_variable_display,
    bnnn_paging,
    {
      .title = caption,
      .text = text,
    });
UX_STEP_INIT(
    ux_init_lower_border,
    NULL,
    NULL,
    {
        display_next_state(false);
    });

UX_FLOW_DEF_VALID(
    ux_confirm_tx_finalize_step,
    pnn,
    txn_approve(),
    {
      &C_icon_validate_14,
      "Sign",
      "Transaction",
    });

UX_FLOW_DEF_VALID(
    ux_reject_tx_flow_step,
    pnn,
    user_approval_denied(),
    {
      &C_icon_crossmark,
      "Cancel",
      "Transaction"
    });

UX_FLOW(ux_txn_flow,
  &ux_confirm_tx_init_flow_step,

  &ux_init_upper_border,
  &ux_variable_display,
  &ux_init_lower_border,

  &ux_confirm_tx_finalize_step,
  &ux_reject_tx_flow_step
);

void ui_txn(void) {
  PRINTF("Transaction:\n");
  PRINTF("  Type: %d\n", current_txn.type);
  PRINTF("  Sender: %.*h\n", 32, current_txn.sender);
  PRINTF("  Fee: %s\n", amount_to_str(current_txn.fee, ALGORAND_DECIMALS));
  PRINTF("  First valid: %s\n", u64str(current_txn.firstValid));
  PRINTF("  Last valid: %s\n", u64str(current_txn.lastValid));
  PRINTF("  Genesis ID: %.*s\n", 32, current_txn.genesisID);
  PRINTF("  Genesis hash: %.*h\n", 32, current_txn.genesisHash);
  PRINTF("  Group ID: %.*h\n", 32, current_txn.groupID);
  if (current_txn.type == PAYMENT) {
    PRINTF("  Receiver: %.*h\n", 32, current_txn.payment.receiver);
    PRINTF("  Amount: %s\n", amount_to_str(current_txn.payment.amount, ALGORAND_DECIMALS));
    PRINTF("  Close to: %.*h\n", 32, current_txn.payment.close);
  }
  if (current_txn.type == ASSET_XFER) {
    PRINTF("  Sender: %.*h\n", 32, current_txn.asset_xfer.sender);
    PRINTF("  Receiver: %.*h\n", 32, current_txn.asset_xfer.receiver);
    PRINTF("  Amount: %s\n", u64str(current_txn.asset_xfer.amount));
    PRINTF("  Close to: %.*h\n", 32, current_txn.asset_xfer.close);
  }
  if (current_txn.type == KEYREG) {
    PRINTF("  Vote PK: %.*h\n", 32, current_txn.keyreg.votepk);
    PRINTF("  VRF PK: %.*h\n", 32, current_txn.keyreg.vrfpk);
    PRINTF("  Stateproof PK: %.*h\n", 64, current_txn.keyreg.sprfkey);
  }

  current_data_index = -1;
  current_state = OUT_OF_BORDERS;
  ux_flow_relayout();
  if (G_ux.stack_count == 0) {
    ux_stack_push(); 
  }
  ux_flow_init(0, ux_txn_flow, NULL);
}

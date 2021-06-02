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

#define INSIDE_BORDERS 0
#define OUT_OF_BORDERS 1

extern screen_t const screen_table[];
extern const size_t screen_num;

uint8_t current_state;
uint8_t current_data_index;

bool set_state_data(bool forward){
    // Apply last formatter to fill the screen's buffer
    do{
      current_data_index = forward ? current_data_index+1 : current_data_index-1;
      if(screen_table[current_data_index].type == ALL_TYPES ||
         screen_table[current_data_index].type == current_txn.type){
           if(((format_function_t)PIC(screen_table[current_data_index].value_setter))() != 0){
             break;
           }
         }
    } while(current_data_index >= 0 &&
            current_data_index < screen_num);

    if(current_data_index < 0 || current_data_index >= screen_num){
      return false;
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

void display_next_state(bool is_upper_border){

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
  }

  current_data_index = -1;
  current_state = OUT_OF_BORDERS;
  ux_flow_relayout();
  if (G_ux.stack_count == 0) {
    ux_stack_push(); 
  }
  ux_flow_init(0, ux_txn_flow, NULL);
}

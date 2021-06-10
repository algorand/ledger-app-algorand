#include "command_handler.h"
#include "apdu_protocol_defines.h"
#include "algo_ui.h"
#include "algo_addr.h"
#include "algo_keys.h"




/*
* this function validated the input of the APDU command buffer.
* and extract the account id from the buffer.
* if the input doesn't contain an account id, the returend account id is 0
*/

int parse_input_for_get_public_key_command(const uint8_t* buffer, const uint32_t buffer_len, uint32_t* output_account_id)
{
  *output_account_id = 0;

  if (buffer_len <= OFFSET_LC)
  {
    PRINTF("using default account id 0 ");
    return 0;
  }

  uint8_t lc = buffer[OFFSET_LC];
  if (lc == 0)
  {
    PRINTF("using default account id 0 ");
    return 0;
  }

  if (lc < sizeof(uint32_t))
  {
    return 0x6a86;
  }

  if (buffer_len < lc + OFFSET_CDATA)
  {
    return 0x6a87;
  }
  *output_account_id = U4BE(buffer, OFFSET_CDATA);

  return 0;
}


/*
* This function takes a binary 32 bytes public key, converts it to Algorand address,
* and send it to the UI.
* this function fails if the public_key buffer is smaller than 32 bytes.
*/
void send_address_to_ui(const struct pubkey_s *public_key)
{
  char public_address[65];
  explicit_bzero(public_address, 65);
  convert_to_public_address(public_key->data, public_address);
  ui_text_put(public_address);
}


/*
* This function parses the input buffer (from the application) and tries to construct 
* the txn_output.
* this function might be called multiple times. each time the function will fill the 
* current_txn_buffer untill the end of the input.
* the function will throw 0x9000 when more data is needed to decode the transaction.
* if a decode error occucrs the fuction returns non null value.
*/

int parse_input_for_msgpack_command(const uint8_t* data_buffer, const uint32_t buffer_len,
                                    uint8_t* current_txn_buffer, const uint32_t current_txn_buffer_size,
                                    uint32_t *current_txn_buffer_offset, txn_t* txn_output,
                                    char **error_msg)
{
  const uint8_t *cdata = data_buffer + OFFSET_CDATA;
  uint8_t lc = data_buffer[OFFSET_LC];

  if (lc == 0) {
    return 0x6a84;
  }

  if (buffer_len < lc + OFFSET_CDATA) {
    return 0x6a85;
  }

  if ((data_buffer[OFFSET_P1] & 0x80) == P1_FIRST)
  {
    memset(txn_output, 0, sizeof(*txn_output));
    *current_txn_buffer_offset = 0;
    txn_output->accountId = 0;
    if (data_buffer[OFFSET_P1] & P1_WITH_ACCOUNT_ID)
    {
      parse_input_for_get_public_key_command(data_buffer, buffer_len, &txn_output->accountId);
      PRINTF("signing the transaction using account id: %d\n",txn_output->accountId);
      cdata += sizeof(uint32_t);
      lc -= sizeof(uint32_t);
    }
  }

  if (*current_txn_buffer_offset + lc > current_txn_buffer_size) {
    return 0x6700;
  }

  memmove(current_txn_buffer + *current_txn_buffer_offset, cdata, lc);
  *current_txn_buffer_offset += lc;

  switch (data_buffer[OFFSET_P2]) {
    case P2_LAST:
      break;
    case P2_MORE:
      return 0x9000;
    default:
      return 0x6B00;
  }

  *error_msg = tx_decode(current_txn_buffer, *current_txn_buffer_offset, txn_output);
  if (*error_msg != NULL) {
    PRINTF("got error from decoder:\n");
    PRINTF("%s\n", *error_msg);
  }

  return 0;
}

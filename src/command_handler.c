#include "command_handler.h"
#include "apdu_protocol_defines.h"
#include "algo_ui.h"
#include "algo_addr.h"



void parse_input_get_public_key(const uint8_t* buffer, const uint32_t buffer_len, uint32_t* output_account_id )
{
  *output_account_id = 0;


  if (buffer_len <= OFFSET_LC) 
  {
    PRINTF("using default account id 0 ");
    return;
  }

  uint8_t lc = buffer[OFFSET_LC];
  if (lc == 0)
  {
    PRINTF("using default account id 0 ");
    return ;
  }

  if (lc < sizeof(uint32_t)) 
  {
    THROW(0x6a86);
  } 
  
  if (buffer_len < lc + OFFSET_CDATA)
  {
    THROW(0x6a87);
  } 
  *output_account_id = U4BE(buffer, OFFSET_CDATA);
}


/*
* This function takes a binary public key, converts it to Algorand public address,
* and send it to the UI note.
* The function assumes that the public_key is 32 bytes long.
*/
void send_address_to_ui(const uint8_t* public_key)
{
  char public_address[65];
  explicit_bzero(public_address, 65);
  convert_to_public_address(public_key, public_address);
  ui_text_put(public_address);
}



char* parse_input_msgpack(const uint8_t * data_buffer, const uint32_t buffer_len, 
                        uint8_t* current_txn_buffer, const uint32_t current_txn_buffer_size, 
                        uint32_t *current_txn_buffer_offset, txn_t* txn_output)
{
  const uint8_t *cdata = data_buffer + OFFSET_CDATA;
  uint8_t lc = data_buffer[OFFSET_LC];

  if (lc == 0 )
  {
    THROW(0x6a84);
  }
  if (buffer_len < lc + OFFSET_CDATA)
  {
    THROW(0x6a85);
  }

  if ((data_buffer[OFFSET_P1] & 0x80) == P1_FIRST)
  {
    explicit_bzero(txn_output, sizeof(txn_output));
    *current_txn_buffer_offset = 0;
    txn_output->accountId = 0;
    if (data_buffer[OFFSET_P1] & P1_WITH_ACCOUNT_ID)
    {
      parse_input_get_public_key(data_buffer, buffer_len, &txn_output->accountId);
      PRINTF("signing the transaction using account id: %d\n",txn_output->accountId);
      cdata += sizeof(uint32_t);
      lc -= sizeof(uint32_t);
    }
  }

  if (*current_txn_buffer_offset + lc > current_txn_buffer_size) 
  {
      THROW(0x6700);
  }

  os_memmove(current_txn_buffer + *current_txn_buffer_offset, cdata, lc);
  *current_txn_buffer_offset += lc;

  switch (data_buffer[OFFSET_P2]) 
  {
    case P2_LAST:
    {
      char *err = tx_decode(current_txn_buffer, *current_txn_buffer_offset, txn_output);
      if (err != NULL) 
      {
        PRINTF("got error from decoder:\n");
        PRINTF("%s\n",err);
        return err;
      }
      return NULL;
    }
    break;
    case P2_MORE:
      THROW(0x9000);
    default:
      THROW(0x6B00);
  }

}

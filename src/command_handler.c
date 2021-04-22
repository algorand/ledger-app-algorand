#include "command_handler.h"
#include "apdu_protocol_defines.h"
#include "algo_ui.h"
#include "algo_addr.h"



void parse_input_get_public_key(const uint8_t* buffer, const uint32_t buffer_len, uint32_t* account_id )
{
  *account_id = 0;


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
  *account_id = U4BE(buffer, OFFSET_CDATA);
}


void send_pubkey_to_ui(const uint8_t* buffer)
{
  char checksummed[65];
  explicit_bzero(checksummed, 65);
  checksummed_addr(buffer, checksummed);
  ui_text_put(checksummed);
}


char* parse_input_msgpack(const uint8_t * data_buffer, const uint32_t buffer_len, 
                        uint8_t* current_tnx_buffer, const uint32_t current_tnx_buffer_size, uint32_t *current_tnx_buffer_offset,
                        txn_t* current_tnx)
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
    explicit_bzero(&current_txn, sizeof(current_txn));
    *current_tnx_buffer_offset = 0;
    current_txn.accountId = 0;
    if (data_buffer[OFFSET_P1] & P1_WITH_ACCOUNT_ID)
    {
      parse_input_get_public_key(data_buffer, buffer_len, &current_txn.accountId);
      PRINTF("signing the transaction using account id: %d\n",current_txn.accountId);
      cdata += sizeof(uint32_t);
      lc -= sizeof(uint32_t);
    }
  }

  if (*current_tnx_buffer_offset + lc > current_tnx_buffer_size) 
  {
      THROW(0x6700);
  }

  os_memmove(current_tnx_buffer + *current_tnx_buffer_offset, cdata, lc);
  *current_tnx_buffer_offset += lc;

  switch (data_buffer[OFFSET_P2]) 
  {
    case P2_LAST:
    {
      char *err = tx_decode(current_tnx_buffer, *current_tnx_buffer_offset, &current_txn);
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

#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"
#include "algo_tx.h"

#define FETCH_MORE_DATA 1
#define TNX_DECODE_ERROR 2



void parse_input_get_public_key(const uint8_t* buffer, int buffer_len, uint32_t* account_id);
void send_pubkey_to_ui(const uint8_t* buffer);
char* parse_input_msgpack(const uint8_t * data_buffer, const uint8_t buffer_len, 
                        uint8_t* current_tnx_buffer, const uint32_t current_tnx_buffer_size, uint32_t *current_tnx_buffer_offset,
                        txn_t* current_tnx, uint8_t* need_more_data);


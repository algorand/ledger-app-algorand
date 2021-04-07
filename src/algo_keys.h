#ifndef __ALGO_KEYS_H__
#define __ALGO_KEYS_H__
#include "cx.h"
#include "stdbool.h"


int algorand_sign_message(uint32_t account_id, const uint8_t* msg_to_sign , const uint32_t msg_len, uint8_t* out_buffer);
void fetch_public_key(uint32_t accountId, uint8_t* pubkey);

#endif


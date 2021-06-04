#ifndef __ALGO_KEYS_H__
#define __ALGO_KEYS_H__
#include "cx.h"
#include "stdbool.h"


#define ALGORAND_PUBLIC_KEY_SIZE 32

int algorand_sign_message(uint32_t account_id, const uint8_t* msg_to_sign , 
                          const uint32_t msg_len, uint8_t* out_signature_buffer);
void fetch_public_key(uint32_t account_id, uint8_t* out_pub_key, const uint32_t out_pub_key_size);

#endif


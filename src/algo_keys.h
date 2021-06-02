#ifndef __ALGO_KEYS_H__
#define __ALGO_KEYS_H__
#include "cx.h"
#include <stdbool.h>


#define ALGORAND_PUBLIC_KEY_SIZE 32

struct pubkey_s {
  uint8_t data[ALGORAND_PUBLIC_KEY_SIZE];
};

extern struct pubkey_s public_key;

int algorand_sign_message(uint32_t account_id, const uint8_t* msg_to_sign,
                          const uint32_t msg_len, uint8_t* out_signature_buffer,
                          int *sign_size);
int fetch_public_key(uint32_t account_id, struct pubkey_s* out_pub_key);

#endif


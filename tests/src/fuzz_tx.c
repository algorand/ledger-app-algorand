#include <malloc.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "algo_tx.h"
#include "ui_txn.h"

txn_t current_txn;

extern screen_t const screen_table[];
extern const size_t screen_num;
extern char caption[20];

static void display_tx(void)
{
  for (size_t i = 0; i < screen_num; i++) {
    if (screen_table[i].type == ALL_TYPES || screen_table[i].type == current_txn.type) {
      if (screen_table[i].value_setter() != 0) {
        if (screen_table[i].caption != SCREEN_DYN_CAPTION) {
          strncpy(caption,
                  screen_table[i].caption,
                  sizeof(caption));
        }
      }
    }
  }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 2) {
    return 0;
  }

#if 0
  size_t msglen = (data[0] << 8) | data[1];
#else
  size_t msglen = 900;
#endif

  if (tx_decode((uint8_t *)data + 2, size - 2, &current_txn) != NULL) {
    return 0;
  }

  uint8_t *msgpack_buf = malloc(msglen);
  if (msgpack_buf == NULL) {
    return 0;
  }

  unsigned int ret = tx_encode(&current_txn, msgpack_buf, msglen);
  if (ret > msglen) {
    // it should never happen
    *(volatile int *)NULL = 0;
  }

  display_tx();

  free(msgpack_buf);

  return 0;
}

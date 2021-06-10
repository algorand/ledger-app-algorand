#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "algo_tx.h"
#include "algo_ui.h"
#include "ui_txn.h"

txn_t current_txn;

extern char caption[20];

static void display_tx(void)
{
  for (size_t i = 0; i < SCREEN_NUM; i++) {
    if (screen_table[i].type != ALL_TYPES && screen_table[i].type != current_txn.type) {
      continue;
    }

    if (screen_table[i].value_setter() == 0) {
      continue;
    }

    if (screen_table[i].caption != SCREEN_DYN_CAPTION) {
      strncpy(caption, screen_table[i].caption, sizeof(caption));
      for (size_t i = 0; i < strlen(caption); i++) {
        assert(isprint(caption[i]));
      }
    }

    //printf("[%s][%s]\n", caption, text);
    for (size_t i = 0; i < strlen(text); i++) {
      assert(isprint(text[i]));
    }
  }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  memset(&current_txn, 0, sizeof(current_txn));

  if (tx_decode((uint8_t *)data, size, &current_txn) != NULL) {
    return 0;
  }

  const size_t msglen = 900;
  uint8_t *msgpack_buf = malloc(msglen);
  if (msgpack_buf == NULL) {
    return 0;
  }

  unsigned int ret = tx_encode(&current_txn, msgpack_buf, msglen);
  assert(ret <= msglen);

  display_tx();

  free(msgpack_buf);

  return 0;
}

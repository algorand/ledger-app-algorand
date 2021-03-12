#include "os.h"

#include "algo_addr.h"
#include "algo_ui.h"

#define MAX_CHARS_PER_LINE 8

char text[128];
static int lineBufferPos;

// 2 extra bytes for ".." on continuation
// 1 extra byte for the null termination
char lineBuffer[MAX_CHARS_PER_LINE+2+1];

char *u64str(uint64_t v)
{
  static char buf[27];

  char *p = &buf[sizeof(buf)];
  *(--p) = '\0';

  if (v == 0) {
    *(--p) = '0';
    return p;
  }

  while (v > 0) {
    *(--p) = '0' + (v % 10);
    v = v/10;
  }

  return p;
}

void
ui_text_putn(const char *msg, size_t maxlen)
{
  for (unsigned int i = 0; i < sizeof(text) && i < maxlen; i++) {
    text[i] = msg[i];

    if (msg[i] == '\0') {
      break;
    }
  }

  text[sizeof(text)-1] = '\0';
  lineBufferPos = 0;

  PRINTF("ui_text_putn: text %s\n", &text[0]);
}

void
ui_text_put(const char *msg)
{
  ui_text_putn(msg, SIZE_MAX);

  /* Caller should invoke ui_text_more() after ui_text_put(). */
}

/* TODO: change public_key type to struct pubkey_s */
void
ui_text_put_addr(const uint8_t *public_key)
{
  struct addr_s checksummed;
  checksummed_addr((const struct pubkey_s *)public_key, &checksummed);
  ui_text_put(checksummed.data);
}

void ui_text_put_u64(uint64_t v)
{
  char *p = u64str(v);
  ui_text_put(p);
}

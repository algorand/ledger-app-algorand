#include <string.h>
#include <stdint.h>

#include "algo_ui.h"
#include "os.h"
#include "os_io_seproxyhal.h"

#define MAX_CHARS_PER_LINE 128

char text[MAX_CHARS_PER_LINE];

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

void ui_text_put(const char *msg)
{
  strncpy(text, msg, MAX_CHARS_PER_LINE);
  text[sizeof(text)-1] = '\0';

  PRINTF("ui_text_putn: text %s\n", &text[0]);

  /* Caller should invoke ui_text_more() after ui_text_put(). */
}

void ui_text_put_u64(uint64_t v)
{
  const char *p = u64str(v);
  ui_text_put(p);
}

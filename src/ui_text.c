#include <string.h>
#include <stdint.h>

#include "algo_ui.h"
#include "str.h"

#define MAX_CHARS_PER_LINE 128

char text[MAX_CHARS_PER_LINE];

void ui_text_put(const char *msg)
{
  strncpy(text, msg, MAX_CHARS_PER_LINE);
  text[sizeof(text)-1] = '\0';

  /* Caller should invoke ui_text_more() after ui_text_put(). */
}

void ui_text_put_u64(uint64_t v)
{
  const char *p = u64str(v);
  ui_text_put(p);
}

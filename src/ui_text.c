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

static bool isprint(char c)
{
  return (c > 31 && c < 127);
}

void ui_text_put_str(const char *msg)
{
  size_t len = strlen(msg);
  size_t i, j = 0;

  if (len > MAX_CHARS_PER_LINE) {
    len = MAX_CHARS_PER_LINE;
  }

  for (i = 0; i < len; i++) {
    if (j >= sizeof(text)-1) {
      break;
    }
    text[j] = isprint(msg[i]) ? msg[i] : '?';
    j++;
  }

  if (i != len || len >= MAX_CHARS_PER_LINE) {
    strncpy(text + MAX_CHARS_PER_LINE - 6, "[...]", 6);
  } else {
    text[len] = '\0';
  }
}

void ui_text_put_u64(uint64_t v)
{
  const char *p = u64str(v);
  ui_text_put(p);
}

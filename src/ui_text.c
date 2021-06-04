#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"

#define MAX_CHARS_PER_LINE 128

char text[MAX_CHARS_PER_LINE];

void ui_text_put(const char *msg)
{
  strncpy(text, msg, MAX_CHARS_PER_LINE);
  text[sizeof(text)-1] = '\0';

  PRINTF("ui_text_putn: text %s\n", &text[0]);

  /* Caller should invoke ui_text_more() after ui_text_put(). */
}

#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"

#define MAX_CHARS_PER_LINE 8

char text[128];
static int lineBufferPos;

// 2 extra bytes for ".." on continuation
// 1 extra byte for the null termination
char lineBuffer[MAX_CHARS_PER_LINE+2+1];

int
ui_text_more()
{
  int linePos;

  if (text[lineBufferPos] == '\0') {
    lineBuffer[0] = '\0';
    return 0;
  }

  for (linePos = 0; linePos < MAX_CHARS_PER_LINE; linePos++) {
    if (text[lineBufferPos] == '\0') {
      break;
    }

    if (text[lineBufferPos] == '\n') {
      lineBufferPos++;
      break;
    }

    lineBuffer[linePos] = text[lineBufferPos];
    lineBufferPos++;
  }

  if (text[lineBufferPos] != '\0') {
    lineBuffer[linePos++] = '.';
    lineBuffer[linePos++] = '.';
  }

  lineBuffer[linePos++] = '\0';
  return 1;
}

void
ui_text_put(const char *msg)
{
  for (unsigned int i = 0; i < sizeof(text); i++) {
    text[i] = msg[i];

    if (msg[i] == '\0') {
      break;
    }
  }

  text[sizeof(text)-1] = '\0';
  lineBufferPos = 0;

  PRINTF("ui_text_put: text %s\n", &text[0]);

  /* Caller should invoke ui_text_more() after ui_text_put(). */
}

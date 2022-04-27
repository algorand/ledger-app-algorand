#pragma once

#include <stdint.h>

#define SCREEN_DYN_CAPTION    NULL

typedef int (*format_function_t)(void);

typedef struct {
  char* caption;
  format_function_t value_setter;
  uint8_t type;
} screen_t;

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define TNX_BUFFER_SIZE 2048
#else
#define TNX_BUFFER_SIZE 900
#endif
extern uint8_t msgpack_buf[TNX_BUFFER_SIZE];
extern unsigned int msgpack_next_off;

extern screen_t const screen_table[];
extern const uint8_t screen_num;

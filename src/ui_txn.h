#pragma once

#include <stdint.h>

#define SCREEN_DYN_CAPTION    NULL

typedef int (*format_function_t)(void);

typedef struct {
  char* caption;
  format_function_t value_setter;
  uint8_t type;
} screen_t;

#if defined(TARGET_NANOX)
#define TNX_BUFFER_SIZE 2048
#else
#define TNX_BUFFER_SIZE 900
#endif
extern uint8_t msgpack_buf[TNX_BUFFER_SIZE];
extern unsigned int msgpack_next_off;

#define SCREEN_NUM 48
extern screen_t const screen_table[SCREEN_NUM];

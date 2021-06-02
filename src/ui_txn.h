#pragma once

#include <stdint.h>

#define SCREEN_DYN_CAPTION    NULL

typedef int (*format_function_t)(void);

typedef struct {
  char* caption;
  format_function_t value_setter;
  uint8_t type;
} screen_t;

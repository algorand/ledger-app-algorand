#pragma once

typedef int (*format_function_t)();
typedef struct{
  char* caption;
  format_function_t value_setter;
  uint8_t type;
} screen_t;

#define SCREEN_DYN_CAPTION    NULL

#pragma once

#include <stdint.h>
#include <stdbool.h>

bool adjustDecimals(char *src, uint32_t srcLength, char *target, uint32_t targetLength, uint8_t decimals);
char *u64str(uint64_t v);
char* amount_to_str(uint64_t amount, uint8_t decimals);
void ui_text_put_str(const char *msg);

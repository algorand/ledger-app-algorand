#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool adjustDecimals(char *src, uint32_t srcLength, char *target,
                    uint32_t targetLength, uint8_t decimals);
char *amount_to_str(uint64_t amount, uint8_t decimals);
char *u64str(uint64_t v);

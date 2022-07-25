/*******************************************************************************
*   (c) 2018 - 2022 Zondax AG
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#pragma once
#include "stdint.h"
#include "stdbool.h"
#include "parser_common.h"

uint8_t encodePubKey(uint8_t *buffer, uint16_t bufferLen, const uint8_t *publicKey);

parser_error_t b64hash_data(unsigned char *data, size_t data_len, char *b64hash, size_t b64hashLen);

parser_error_t _toStringBalance(uint64_t* amount, uint8_t decimalPlaces, char postfix[], char prefix[],
                                char* outValue, uint16_t outValueLen, uint8_t pageIdx, uint8_t* pageCount);

parser_error_t _toStringAddress(uint8_t* address, char* outValue, uint16_t outValueLen, uint8_t pageIdx, uint8_t* pageCount);

parser_error_t _toStringSchema(const state_schema *schema, char* outValue, uint16_t outValueLen, uint8_t pageIdx, uint8_t* pageCount);

bool all_zero_key(uint8_t *buff);
bool is_opt_in_tx(parser_tx_t *tx_obj);

/*******************************************************************************
*  (c) 2018 - 2022 Zondax AG
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

#include "parser_common.h"
#include <zxmacros.h>
#include "zxtypes.h"
#include "parser_txdef.h"

#ifdef __cplusplus
extern "C" {
#endif

// Checks that there are at least SIZE bytes available in the buffer
#define CTX_CHECK_AVAIL(CTX, SIZE) \
    if ( (CTX) == NULL || ((CTX)->offset + (SIZE)) > (CTX)->bufferLen) { return parser_unexpected_buffer_end; }

#define CTX_CHECK_AND_ADVANCE(CTX, SIZE) \
    CTX_CHECK_AVAIL((CTX), (SIZE))   \
    (CTX)->offset += (SIZE);

#define DEF_READARRAY(SIZE) \
    v->_ptr = c->buffer + c->offset; \
    CTX_CHECK_AND_ADVANCE(c, SIZE) \
    return parser_ok;

#define DEF_READFIX_UNSIGNED(BITS) parser_error_t _readUInt ## BITS(parser_context_t *ctx, uint ## BITS ##_t *value)
#define DEC_READFIX_UNSIGNED(BITS) parser_error_t _readUInt ## BITS(parser_context_t *ctx, uint ## BITS ##_t *value) \
{                                                                                           \
    if (value == NULL)  return parser_no_data;                                              \
    *value = 0u;                                                                            \
    for(uint8_t i=0u; i < (BITS##u>>3u); i++, ctx->offset++) {                              \
        if (ctx->offset >= ctx->bufferLen) return parser_unexpected_buffer_end;             \
        *value = (*value << 8) | (uint ## BITS ##_t) *(ctx->buffer + ctx->offset);          \
    }                                                                                       \
    return parser_ok;                                                                       \
}

parser_error_t _readBytes(parser_context_t *c, uint8_t *buff, uint16_t bufLen);

parser_error_t parser_init(parser_context_t *ctx,
                           const uint8_t *buffer,
                           uint16_t bufferSize);

uint8_t _getNumItems();
uint8_t _getCommonNumItems();
uint8_t _getTxNumItems();

parser_error_t _read(parser_context_t *c, parser_tx_t *v);

parser_error_t _readMapSize(parser_context_t *c, size_t *mapItems);
parser_error_t _readArraySize(parser_context_t *c, size_t *mapItems);
parser_error_t _readString(parser_context_t *c, uint8_t *buff, uint16_t buffLen);
parser_error_t _readInteger(parser_context_t *c, uint64_t* value);
parser_error_t _readBool(parser_context_t *c, uint8_t *value);
parser_error_t _readArray_args(parser_context_t *c, uint8_t args[][MAX_ARGLEN], size_t args_len[], uint8_t *argsSize, uint8_t maxArgs);
parser_error_t _readBinFixed(parser_context_t *c, uint8_t *buff, uint16_t bufferLen);
parser_error_t _findKey(parser_context_t *c, const char *key);

DEF_READFIX_UNSIGNED(8);
DEF_READFIX_UNSIGNED(16);
DEF_READFIX_UNSIGNED(32);
DEF_READFIX_UNSIGNED(64);


#ifdef __cplusplus
}
#endif

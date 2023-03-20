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

#include "parser_encoding.h"
#include "base64.h"
#include "zxformat.h"
#include "crypto.h"

#include "sha512.h"
#include "base32.h"
#define CX_SHA512_SIZE 64

 #if defined(TARGET_NANOS) || defined(TARGET_NANOS2) || defined(TARGET_NANOX) || defined(TARGET_STAX)

#else
#include "picohash.h"

#endif

uint8_t encodePubKey(uint8_t *buffer, uint16_t bufferLen, const uint8_t *publicKey)
{
    if(bufferLen < (2 * PK_LEN_25519 + 1)) {
        return 0;
    }
    uint8_t messageDigest[CX_SHA512_SIZE];
    SHA512_256(publicKey, 32, messageDigest);

    uint8_t checksummed[36];
    memmove(&checksummed[0], publicKey, 32);
    memmove(&checksummed[32], &messageDigest[28], 4);

    return base32_encode(checksummed, sizeof(checksummed), (char*)buffer, bufferLen);
}

parser_error_t b64hash_data(unsigned char *data, size_t data_len, char *b64hash, size_t b64hashLen)
{
    unsigned char hash[32];
#if defined(TARGET_NANOS) || defined(TARGET_NANOS2) || defined(TARGET_NANOX) || defined(TARGET_STAX)
    // Hash program and b64 encode for display
    cx_sha256_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    cx_sha256_init(&ctx);
    cx_hash(&ctx.header, CX_LAST, data, data_len, hash, sizeof(hash));
#else
    picohash_ctx_t ctx;
    picohash_init_sha256(&ctx);
    picohash_update(&ctx, data, data_len);
    picohash_final(&ctx, hash);
#endif
    base64_encode((const char *)hash, sizeof(hash), b64hash, b64hashLen);
    return parser_ok;
}

parser_error_t _toStringBalance(uint64_t* amount, uint8_t decimalPlaces, char postfix[], char prefix[],
                                char* outValue, uint16_t outValueLen, uint8_t pageIdx, uint8_t* pageCount)
{
    char bufferUI[200] = {0};
    if (uint64_to_str(bufferUI, sizeof(bufferUI), *amount) != NULL) {
        return parser_unexpected_value;
    }

    if (intstr_to_fpstr_inplace(bufferUI, sizeof(bufferUI), decimalPlaces) == 0) {
        return parser_unexpected_value;
    }

    if (z_str3join(bufferUI, sizeof(bufferUI), prefix, postfix) != zxerr_ok) {
        return parser_unexpected_buffer_end;
    }

    number_inplace_trimming(bufferUI, 1);

    pageString(outValue, outValueLen, bufferUI, pageIdx, pageCount);
    return parser_ok;
}

parser_error_t _toStringAddress(uint8_t* address, char* outValue, uint16_t outValueLen, uint8_t pageIdx, uint8_t* pageCount)
{
    if (all_zero_key(address)) {
        snprintf(outValue, outValueLen, "Zero");
        *pageCount = 1;
    } else {
        char buff[65] = {0};
        if (!encodePubKey((uint8_t*)buff, sizeof(buff), address)){
            return parser_unexpected_buffer_end;
        }
        pageString(outValue, outValueLen, buff, pageIdx, pageCount);
    }
    return parser_ok;
}

parser_error_t _toStringSchema(const state_schema *schema, char* outValue, uint16_t outValueLen, uint8_t pageIdx, uint8_t* pageCount)
{
    // Don't display if nonzero schema cannot be valid
    // if (current_txn.application.id != 0) {
    //     return 0;
    // }
    char schm_repr[65] = {0};
    char uint_part[32] = {0};
    char byte_part[32] = {0};
    char bufferUI[200] = {0};

    if (uint64_to_str(bufferUI, sizeof(bufferUI), schema->num_uint) != NULL) {
        return parser_unexpected_value;
    }
    snprintf(uint_part, sizeof(uint_part), "uint: %s", bufferUI);

    if (uint64_to_str(bufferUI, sizeof(bufferUI), schema->num_byteslice) != NULL) {
        return parser_unexpected_value;
    }
    snprintf(byte_part, sizeof(byte_part), "byte: %s", bufferUI);

    snprintf(schm_repr, sizeof(schm_repr), "%s, %s",   uint_part, byte_part);
    pageString(outValue, outValueLen, schm_repr, pageIdx, pageCount);

    return parser_ok;
}

bool is_opt_in_tx(parser_tx_t *tx_obj) {

    if(tx_obj->type == TX_ASSET_XFER && tx_obj->asset_xfer.amount == 0 && tx_obj->asset_xfer.id != 0 &&
        memcmp(tx_obj->asset_xfer.receiver, tx_obj->asset_xfer.sender, sizeof(tx_obj->asset_xfer.receiver)) == 0)
    {
            return true;
    }
    return false;
}

bool all_zero_key(uint8_t *buff) {
  for (int i = 0; i < 32; i++) {
    if (buff[i] != 0) {
      return false;
    }
  }
  return true;
}

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

#include "crypto.h"
#include "coin.h"
#include "cx.h"
#include "zxmacros.h"
#include "parser_encoding.h"

uint32_t hdPath[HDPATH_LEN_DEFAULT];

zxerr_t crypto_extractPublicKey(uint8_t *pubKey, uint16_t pubKeyLen) {
    if (pubKey == NULL || pubKeyLen < PK_LEN_25519) {
        return zxerr_invalid_crypto_settings;
    }

    zxerr_t error = zxerr_unknown;
    cx_ecfp_public_key_t cx_publicKey;
    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[SK_LEN_25519] = {0};

    // Generate keys
    CATCH_CXERROR(os_derive_bip32_with_seed_no_throw(HDW_NORMAL,
                                                     CX_CURVE_Ed25519,
                                                     hdPath,
                                                     HDPATH_LEN_DEFAULT,
                                                     privateKeyData,
                                                     NULL,
                                                     NULL,
                                                     0));

    CATCH_CXERROR(cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519, privateKeyData, 32, &cx_privateKey));
    CATCH_CXERROR(cx_ecfp_init_public_key_no_throw(CX_CURVE_Ed25519, NULL, 0, &cx_publicKey));
    CATCH_CXERROR(cx_ecfp_generate_pair_no_throw(CX_CURVE_Ed25519, &cx_publicKey, &cx_privateKey, 1));

    for (unsigned int i = 0; i < PK_LEN_25519; i++) {
        pubKey[i] = cx_publicKey.W[64 - i];
    }

    if ((cx_publicKey.W[PK_LEN_25519] & 1) != 0) {
        pubKey[31] |= 0x80;
    }
    error = zxerr_ok;

catch_cx_error:
    MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
    MEMZERO(privateKeyData, SK_LEN_25519);

    if (error != zxerr_ok) {
        MEMZERO(pubKey, pubKeyLen);
    }

    return error;
}

zxerr_t crypto_sign(uint8_t *signature, uint16_t signatureMaxlen, const uint8_t *message, uint16_t messageLen) {
    if (signature == NULL || message == NULL || signatureMaxlen < ED25519_SIGNATURE_SIZE || messageLen == 0) {
        return zxerr_unknown;
    }

    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[SK_LEN_25519] = {0};

    zxerr_t error = zxerr_unknown;

    CATCH_CXERROR(os_derive_bip32_with_seed_no_throw(HDW_NORMAL,
                                                     CX_CURVE_Ed25519,
                                                     hdPath,
                                                     HDPATH_LEN_DEFAULT,
                                                     privateKeyData,
                                                     NULL,
                                                     NULL,
                                                     0));

    CATCH_CXERROR(cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519, privateKeyData, 32, &cx_privateKey));
    CATCH_CXERROR(cx_eddsa_sign_no_throw(&cx_privateKey,
                                         CX_SHA512,
                                         message,
                                         messageLen,
                                         signature,
                                         signatureMaxlen));
    error = zxerr_ok;

catch_cx_error:
    MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
    MEMZERO(privateKeyData, SK_LEN_25519);

    if (error != zxerr_ok) {
        MEMZERO(signature, signatureMaxlen);
    }

    return error;
}

zxerr_t crypto_fillAddress(uint8_t *buffer, uint16_t bufferLen, uint16_t *addrResponseLen)
{
    if (bufferLen < PK_LEN_25519 + SS58_ADDRESS_MAX_LEN) {
        return zxerr_unknown;
    }

    MEMZERO(buffer, bufferLen);
    CHECK_ZXERR(crypto_extractPublicKey(buffer, bufferLen))

    const uint32_t outLen = encodePubKey(buffer + PK_LEN_25519,
                                         bufferLen - PK_LEN_25519,
                                         buffer);

    if (outLen == 0 || (outLen + PK_LEN_25519) > UINT16_MAX) {
        MEMZERO(buffer, bufferLen);
        return zxerr_encoding_failed;
    }

    *addrResponseLen = PK_LEN_25519 + outLen;
    return zxerr_ok;
}

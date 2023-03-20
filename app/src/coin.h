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

#ifdef __cplusplus
extern "C" {
#endif

#define CLA                             0x80

#include <stdint.h>
#include <stddef.h>

#define HDPATH_LEN_DEFAULT   5
#define HDPATH_0_DEFAULT     (0x80000000u | 0x2c)   //44
#define HDPATH_1_DEFAULT     (0x80000000u | 0x11b)  //283

#define HDPATH_2_DEFAULT     (0x80000000u | 0u)
#define HDPATH_3_DEFAULT     (0u)
#define HDPATH_4_DEFAULT     (0u)

#define SECP256K1_PK_LEN            65u

#define SK_LEN_25519 64u
#define SCALAR_LEN_ED25519 32u
#define SIG_PLUS_TYPE_LEN 65u

#define PK_LEN_25519 32u
#define SS58_ADDRESS_MAX_LEN 60u

#define MAX_SIGN_SIZE 256u
#define BLAKE2B_DIGEST_SIZE 32u

#define COIN_AMOUNT_DECIMAL_PLACES 6
#define COIN_TICKER "ALGO "

#define MENU_MAIN_APP_LINE1 "Algorand"
#define MENU_MAIN_APP_LINE2 "Ready"
#define MENU_MAIN_APP_LINE2_SECRET          "???"
#define APPVERSION_LINE1 "Version"
#define APPVERSION_LINE2 "v" APPVERSION

#define REVIEW_SCREEN_TITLE "Review"
#define REVIEW_SCREEN_TXN_VALUE "Transaction"
#define REVIEW_SCREEN_ADDR_VALUE "address"

#define APDU_MIN_LENGTH                 5
#define ACCOUNT_ID_LENGTH               4

#define P1_SINGLE_CHUNK                10  //< P1

#define P1_FIRST 0x00
#define P1_FIRST_ACCOUNT_ID 0x01
#define P1_MORE  0x80
#define P1_WITH_REQUEST_USER_APPROVAL  0x80

#define P2_LAST  0x00
#define P2_MORE  0x80

#define INS_GET_VERSION     0x00
#define INS_GET_PUBLIC_KEY  0x03
#define INS_GET_ADDRESS     0x04
#define INS_SIGN_MSGPACK    0x08

#ifdef __cplusplus
}
#endif

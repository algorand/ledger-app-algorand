/*******************************************************************************
*   (c) 2018 - 2022 Zondax AG
*   (c) 2016 Ledger
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

#include "app_main.h"

#include <string.h>
#include <os_io_seproxyhal.h>
#include <os.h>
#include <ux.h>

#include "view.h"
#include "view_internal.h"
#include "actions.h"
#include "tx.h"
#include "addr.h"
#include "crypto.h"
#include "coin.h"
#include "zxmacros.h"

static bool tx_initialized = false;
static const unsigned char tmpBuff[] = {'T', 'X'};

__Z_INLINE void extractHDPath() {
    hdPath[0] = HDPATH_0_DEFAULT;
    hdPath[1] = HDPATH_1_DEFAULT;
    hdPath[3] = HDPATH_3_DEFAULT;
    hdPath[4] = HDPATH_4_DEFAULT;

    if (G_io_apdu_buffer[OFFSET_DATA_LEN] == 0) {
        hdPath[2] = HDPATH_2_DEFAULT;
    } else {
        hdPath[2] = HDPATH_2_DEFAULT | U4BE(G_io_apdu_buffer, OFFSET_DATA);
    }
}

__Z_INLINE uint8_t convertP1P2(const uint8_t p1, const uint8_t p2)
{
    if (p1 <= P1_FIRST_ACCOUNT_ID && p2 == P2_MORE) {
        return P1_INIT;
    } else if (p1 == P1_MORE && p2 == P2_MORE) {
        return P1_ADD;
    } else if (p1 == P1_MORE && p2 == P2_LAST) {
        return P1_LAST;
    } else if (p1 <= P1_FIRST_ACCOUNT_ID && p2 == P2_LAST) {
        // Transaction fits in one chunk
        return P1_SINGLE_CHUNK;
    }
    return 0xFF;
}

__Z_INLINE bool process_chunk(__Z_UNUSED volatile uint32_t *tx, uint32_t rx)
{
    const uint8_t P1 = G_io_apdu_buffer[OFFSET_P1];
    const uint8_t P2 = G_io_apdu_buffer[OFFSET_P2];
    const uint8_t payloadType = convertP1P2(P1, P2);

    if (rx < OFFSET_DATA) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    uint32_t added;
    uint8_t accountIdSize = 0;

    switch (payloadType) {
        case P1_INIT:
            tx_initialize();
            tx_reset();
            tx_initialized = true;
            if (P1 == P1_FIRST_ACCOUNT_ID) {
                extractHDPath();
                accountIdSize = ACCOUNT_ID_LENGTH;
            }
            tx_append((unsigned char*)tmpBuff, 2);
            added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA + accountIdSize]), rx - (OFFSET_DATA + accountIdSize));
            if (added != rx - (OFFSET_DATA + accountIdSize)) {
                tx_initialized = false;
                THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
            }
            return false;

        case P1_ADD:
            if (!tx_initialized) {
                THROW(APDU_CODE_TX_NOT_INITIALIZED);
            }
            added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);
            if (added != rx - OFFSET_DATA) {
                tx_initialized = false;
                THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
            }
            return false;

        case P1_LAST:
            if (!tx_initialized) {
                THROW(APDU_CODE_TX_NOT_INITIALIZED);
            }
            added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);
            tx_initialized = false;
            if (added != rx - OFFSET_DATA) {
                tx_initialized = false;
                THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
            }
            tx_initialized = false;
            return true;

        case P1_SINGLE_CHUNK:
            tx_initialize();
            tx_reset();
            if (P1 == P1_FIRST_ACCOUNT_ID) {
                extractHDPath();
                accountIdSize = ACCOUNT_ID_LENGTH;
            }
            tx_append((unsigned char*)tmpBuff, 2);
            added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA + accountIdSize]), rx - (OFFSET_DATA + accountIdSize));
            tx_initialized = false;
            if (added != rx - (OFFSET_DATA + accountIdSize)) {
                THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
            }
            return true;
    }

    THROW(APDU_CODE_INVALIDP1P2);
}

__Z_INLINE void handle_sign_msgpack(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx)
{
    if (!process_chunk(tx, rx)) {
        THROW(APDU_CODE_OK);
    }

    const char *error_msg = tx_parse();
    CHECK_APP_CANARY()

    if (error_msg != NULL) {
        int error_msg_length = strlen(error_msg);
        memcpy(G_io_apdu_buffer, error_msg, error_msg_length);
        *tx += (error_msg_length);
        THROW(APDU_CODE_DATA_INVALID);
    }

    view_review_init(tx_getItem, tx_getNumItems, app_sign);
    view_review_show(REVIEW_TXN);
    *flags |= IO_ASYNCH_REPLY;
}

__Z_INLINE void handle_get_public_key(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx)
{
    const uint8_t requireConfirmation = G_io_apdu_buffer[OFFSET_P1];
    const bool u2f_compatibility = G_io_apdu_buffer[OFFSET_INS] == INS_GET_PUBLIC_KEY;
    extractHDPath();

    zxerr_t err = app_fill_address();
    if (err != zxerr_ok) {
        THROW(APDU_CODE_UNKNOWN);
    }

    if (requireConfirmation) {
        view_review_init(addr_getItem, addr_getNumItems, app_reply_address);
        view_review_show(REVIEW_ADDRESS);
        *flags |= IO_ASYNCH_REPLY;
        return;
    }

    //U2F compatibility: return only pubkey
    if (u2f_compatibility) {
        action_addrResponseLen = PK_LEN_25519;
    }

    *tx = action_addrResponseLen;
    THROW(APDU_CODE_OK);
}

__Z_INLINE void handle_getversion(volatile uint32_t *flags, volatile uint32_t *tx)
{
    G_io_apdu_buffer[0] = 0;

#if defined(APP_TESTING)
    G_io_apdu_buffer[0] = 0x01;
#endif

    G_io_apdu_buffer[1] = (LEDGER_MAJOR_VERSION >> 8) & 0xFF;
    G_io_apdu_buffer[2] = (LEDGER_MAJOR_VERSION >> 0) & 0xFF;

    G_io_apdu_buffer[3] = (LEDGER_MINOR_VERSION >> 8) & 0xFF;
    G_io_apdu_buffer[4] = (LEDGER_MINOR_VERSION >> 0) & 0xFF;

    G_io_apdu_buffer[5] = (LEDGER_PATCH_VERSION >> 8) & 0xFF;
    G_io_apdu_buffer[6] = (LEDGER_PATCH_VERSION >> 0) & 0xFF;

    G_io_apdu_buffer[7] = !IS_UX_ALLOWED;

    G_io_apdu_buffer[8] = (TARGET_ID >> 24) & 0xFF;
    G_io_apdu_buffer[9] = (TARGET_ID >> 16) & 0xFF;
    G_io_apdu_buffer[10] = (TARGET_ID >> 8) & 0xFF;
    G_io_apdu_buffer[11] = (TARGET_ID >> 0) & 0xFF;

    *tx += 12;
    THROW(APDU_CODE_OK);
}

void handleApdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    uint16_t sw = 0;

    BEGIN_TRY
    {
        TRY
        {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(APDU_CODE_CLA_NOT_SUPPORTED);
            }

            if (rx < APDU_MIN_LENGTH) {
                THROW(APDU_CODE_WRONG_LENGTH);
            }

            const uint8_t ins = G_io_apdu_buffer[OFFSET_INS];
            switch (ins) {
                case INS_SIGN_MSGPACK:
                    CHECK_PIN_VALIDATED()
                    handle_sign_msgpack(flags, tx, rx);
                    break;

                case INS_GET_ADDRESS:
                case INS_GET_PUBLIC_KEY: {
                    CHECK_PIN_VALIDATED()
                    handle_get_public_key(flags, tx, rx);
                    break;
                }

                case INS_GET_VERSION: {
                    handle_getversion(flags, tx);
                    THROW(APDU_CODE_OK);
                    break;
                }

                default:
                    THROW(APDU_CODE_INS_NOT_SUPPORTED);
            }
        }
        CATCH(EXCEPTION_IO_RESET)
        {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e)
        {
            switch (e & 0xF000) {
                case 0x6000:
                case APDU_CODE_OK:
                    sw = e;
                    break;
                default:
                    sw = 0x6800 | (e & 0x7FF);
                    break;
            }
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY
        {
        }
    }
    END_TRY;
}

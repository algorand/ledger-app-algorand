/*******************************************************************************
*   (c) 2018, 2019 Zondax GmbH
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

#include "view.h"
#include "coin.h"
#include "view_internal.h"
#include "crypto.h"

#include "actions.h"
#include "apdu_codes.h"
#include "ux.h"
#include "bagl.h"
#include "zxmacros.h"
#include "view_templates.h"
#include "tx.h"
#include "addr.h"
#include "app_mode.h"
#include "zxerror.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

view_t viewdata;
unsigned int mustReply = 0;

void h_approve(__Z_UNUSED unsigned int _) {
    zemu_log_stack("h_approve");

    view_idle_show(0, NULL);
    UX_WAIT();
    if (viewdata.viewfuncAccept != NULL) {
        viewdata.viewfuncAccept();
    }
}

void h_reject(unsigned int requireReply) {
    zemu_log_stack("h_reject");

    view_idle_show(0, NULL);
    UX_WAIT();
    if(requireReply > 0) {
        app_reject();
    }
}

void h_error_accept(__Z_UNUSED unsigned int _) {
    view_idle_show(0, NULL);
    UX_WAIT();
    app_reply_error();
}

void h_initialize(__Z_UNUSED unsigned int _) {
    view_idle_show(0, NULL);
    UX_WAIT();
}

///////////////////////////////////
// Paging related

void h_paging_init() {
    zemu_log_stack("h_paging_init");

    viewdata.itemIdx = 0;
    viewdata.pageIdx = 0;
    viewdata.pageCount = 1;
    viewdata.itemCount = 0xFF;
}

bool h_paging_can_increase() {
    if (viewdata.pageIdx + 1 < viewdata.pageCount) {
        zemu_log_stack("h_paging_can_increase");
        return true;
    }

    // passed page count, go to next index
    if (viewdata.itemCount > 0 && viewdata.itemIdx < (viewdata.itemCount - 1 + INCLUDE_ACTIONS_COUNT)) {
        zemu_log_stack("h_paging_can_increase");
        return true;
    }

    zemu_log_stack("h_paging_can_increase NO");
    return false;
}

void h_paging_increase() {
    zemu_log_stack("h_paging_increase");

    if (viewdata.pageIdx + 1 < viewdata.pageCount) {
        // increase page
        viewdata.pageIdx++;
        return;
    }

    // passed page count, go to next index
    if (viewdata.itemCount > 0 && viewdata.itemIdx < (viewdata.itemCount - 1 + INCLUDE_ACTIONS_COUNT)) {
        viewdata.itemIdx++;
        viewdata.pageIdx = 0;
    }
}

bool h_paging_can_decrease() {
    if (viewdata.pageIdx != 0) {
        zemu_log_stack("h_paging_can_decrease");
        return true;
    }

    if (viewdata.itemIdx > 0) {
        zemu_log_stack("h_paging_can_decrease");
        return true;
    }

    zemu_log_stack("h_paging_can_decrease NO");
    return false;
}

void h_paging_decrease() {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "h_paging_decrease Idx %d", viewdata.itemIdx);
    zemu_log_stack(buffer);

    if (viewdata.pageIdx != 0) {
        viewdata.pageIdx--;
        zemu_log_stack("page--");
        return;
    }

    if (viewdata.itemIdx > 0) {
        viewdata.itemIdx--;
        zemu_log_stack("item--");
        // jump to last page. update will cap this value
        viewdata.pageIdx = 255;
    }
}

uint8_t getIntroPages() {
#if defined(APP_BLIND_MODE_ENABLED)
    if (mustReply == REVIEW_ADDRESS || !app_mode_blind()) {
        #if defined(TARGET_NANOS)
            return INTRO_PAGES - 1;
        #else
            return INTRO_PAGES;
        #endif
    } else {
        return INTRO_PAGES;
    }
#else
    return INTRO_PAGES;
#endif
}

bool h_paging_intro_screen() {
    return viewdata.itemIdx < getIntroPages();
}

///////////////////////////////////
// Paging related

#ifdef INCLUDE_ACTIONS_AS_ITEMS
bool is_accept_item(){
    return viewdata.itemIdx == viewdata.itemCount - 1;
}

void set_accept_item(){
    viewdata.itemIdx = viewdata.itemCount - 1;
    viewdata.pageIdx = 0;
}

bool is_reject_item(){
    return viewdata.itemIdx == viewdata.itemCount;
}
#endif

void h_review_action(unsigned int requireReply) {
#ifdef INCLUDE_ACTIONS_AS_ITEMS
    if( is_accept_item() ){
        zemu_log_stack("action_accept");
        h_approve(1);
        return;
    }

    if( is_reject_item() ){
        zemu_log_stack("action_reject");
        h_reject(requireReply);
        return;
    }

    // I need to use this shortcut for BlindSigning
    zemu_log_stack("quick accept");
    if (app_mode_expert()) {
        set_accept_item();
        h_review_update();
        return;
    }
#endif
}

zxerr_t h_review_update_data() {
    if (viewdata.viewfuncGetNumItems == NULL) {
        zemu_log_stack("h_review_update_data - GetNumItems==NULL");
        return zxerr_no_data;
    }
    if (viewdata.viewfuncGetItem == NULL) {
        zemu_log_stack("h_review_update_data - GetItem==NULL");
        return zxerr_no_data;
    }

    if (viewdata.viewfuncGetItem == NULL) {
        zemu_log_stack("h_review_update_data - GetItem==NULL");
        return zxerr_no_data;
    }

    if (viewdata.viewfuncGetItem == NULL) {
        zemu_log_stack("h_review_update_data - GetItem==NULL");
        return zxerr_no_data;
    }

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "update Idx %d/%d", viewdata.itemIdx, viewdata.pageIdx);
    zemu_log_stack(buffer);

#ifdef INCLUDE_ACTIONS_AS_ITEMS
    viewdata.pageCount = 1;
    if( is_accept_item() ){
        snprintf(viewdata.key, MAX_CHARS_PER_KEY_LINE, "%s","");
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "%s", APPROVE_LABEL);
        splitValueField();
        zemu_log_stack("show_accept_action - accept item");
        viewdata.pageIdx = 0;
        return zxerr_ok;
    }

    if( is_reject_item() ){
        snprintf(viewdata.key, MAX_CHARS_PER_KEY_LINE, "%s", "");
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "%s", REJECT_LABEL);
        splitValueField();
        zemu_log_stack("show_reject_action - reject item");
        viewdata.pageIdx = 0;
        return zxerr_ok;
    }


    if (h_paging_intro_screen()) {
        char* intro_key = NULL;
        char* intro_value = NULL;

    #if defined(REVIEW_SCREEN_ENABLED) && defined(APP_BLIND_MODE_ENABLED)
        switch (viewdata.itemIdx) {
            case 0:
                intro_key = PIC(review_key);
                if(mustReply == REVIEW_ADDRESS) {
                    intro_value = PIC(review_addrvalue);
                } else {
                    intro_value = PIC(review_txvalue);
                }
                break;
            case 1:
                intro_key = PIC(blindsigning_key);
                intro_value = PIC(blindsigning_value);
                break;
            default:
                return zxerr_no_data;
        }
    #elif defined(REVIEW_SCREEN_ENABLED)
        intro_key = PIC(review_key);
        if(mustReply == REVIEW_ADDRESS) {
            intro_value = PIC(review_addrvalue);
        } else {
            intro_value = PIC(review_txvalue);
        }
    #elif defined(APP_BLIND_MODE_ENABLED)
        intro_key = PIC(blindsigning_key);
        intro_value = PIC(blindsigning_value);
    #else
        return zxerr_no_data;
    #endif

        snprintf(viewdata.key, MAX_CHARS_PER_KEY_LINE, "%s", intro_key);
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "%s", intro_value);
        splitValueField();
        viewdata.pageIdx = 0;
        return zxerr_ok;
    }
#endif

    do {
        CHECK_ZXERR(viewdata.viewfuncGetNumItems(&viewdata.itemCount))
        viewdata.itemCount+= getIntroPages();

        //Verify how many chars fit in display (nanos)
        CHECK_ZXERR(viewdata.viewfuncGetItem(
                viewdata.itemIdx - getIntroPages(),
                viewdata.key, MAX_CHARS_PER_KEY_LINE,
                viewdata.value, MAX_CHARS_PER_VALUE1_LINE,
                0, &viewdata.pageCount))
        viewdata.pageCount = 1;
        const max_char_display dyn_max_char_per_line1 = get_max_char_per_line();

        // be sure we are not out of bounds
        CHECK_ZXERR(viewdata.viewfuncGetItem(
                viewdata.itemIdx - getIntroPages(),
                viewdata.key, MAX_CHARS_PER_KEY_LINE,
                viewdata.value, dyn_max_char_per_line1,
                0, &viewdata.pageCount))
        if (viewdata.pageCount != 0 && viewdata.pageIdx > viewdata.pageCount) {
            // try again and get last page
            viewdata.pageIdx = viewdata.pageCount - 1;
        }
        CHECK_ZXERR(viewdata.viewfuncGetItem(
                viewdata.itemIdx - getIntroPages(),
                viewdata.key, MAX_CHARS_PER_KEY_LINE,
                viewdata.value, dyn_max_char_per_line1,
                viewdata.pageIdx, &viewdata.pageCount))

        viewdata.itemCount++;

        if (viewdata.pageCount > 1) {
            uint8_t keyLen = strlen(viewdata.key);
            if (keyLen < MAX_CHARS_PER_KEY_LINE) {
                snprintf(viewdata.key + keyLen,
                         MAX_CHARS_PER_KEY_LINE - keyLen,
                         " [%d/%d]",
                         viewdata.pageIdx + 1,
                         viewdata.pageCount);
            }
        }

        if (viewdata.pageCount == 0) {
            h_paging_increase();
        }
    } while (viewdata.pageCount == 0);

    splitValueAddress();
    return zxerr_ok;
}

///////////////////////////////////
// General

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

void view_init(void) {
    UX_INIT();
#ifdef APP_SECRET_MODE_ENABLED
    viewdata.secret_click_count = 0;
#endif
}

void view_idle_show(uint8_t item_idx, char *statusString) {
    view_idle_show_impl(item_idx, statusString);
}

void view_message_show(char *title, char *message) {
    view_message_impl(title, message);
}

void view_review_init(viewfunc_getItem_t viewfuncGetItem,
                      viewfunc_getNumItems_t viewfuncGetNumItems,
                      viewfunc_accept_t viewfuncAccept) {
    viewdata.viewfuncGetItem = viewfuncGetItem;
    viewdata.viewfuncGetNumItems = viewfuncGetNumItems;
    viewdata.viewfuncAccept = viewfuncAccept;
}

void view_review_show(unsigned int requireReply) {
    // Set > 0 to reply apdu message
    view_review_show_impl(requireReply);
}

void view_error_show() {
    snprintf(viewdata.key, MAX_CHARS_PER_KEY_LINE, "ERROR");
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "SHOWING DATA");
    splitValueField();
    view_error_show_impl();
}

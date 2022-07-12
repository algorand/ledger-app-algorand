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

#include "app_mode.h"
#include "view.h"
#include "view_internal.h"
#include "apdu_codes.h"
#include "ux.h"
#include "bagl.h"
#include "zxmacros.h"
#include "view_templates.h"
#include "zxutils_ledger.h"

#if defined(TARGET_NANOS)
#define BAGL_WIDTH 128
#define BAGL_HEIGHT 32
#define BAGL_WIDTH_MARGIN 10

static bool exceed_pixel_in_display(const uint8_t length);

void h_initialize();
void account_enabled();
void blind_enabled(){};

static void h_expert_toggle();
static void h_expert_update();
static void h_review_button_left();
static void h_review_button_right();
static void h_review_button_both();

#ifdef APP_SECRET_MODE_ENABLED
static void h_secret_click();
#endif

#ifdef APP_ACCOUNT_MODE_ENABLED
static void h_account_toggle();
static void h_account_update();
#endif

#ifdef APP_BLIND_MODE_ENABLED
static void h_blind_toggle();
static void h_blind_update();
#endif

ux_state_t ux;
extern ux_menu_state_t ux_menu;
extern unsigned int mustReply;

void os_exit(uint32_t id) {
    (void)id;
    os_sched_exit(0);
}

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, &C_icon_app, MENU_MAIN_APP_LINE1, viewdata.key, 33, 12},
    {NULL, h_expert_toggle, 0, &C_icon_app, "Expert mode:", viewdata.value, 33, 12},

#ifdef APP_ACCOUNT_MODE_ENABLED
    {NULL, h_account_toggle, 0, &C_icon_app, "Account:", viewdata.value, 33, 12},
#endif

#ifdef APP_BLIND_MODE_ENABLED
    {NULL, h_blind_toggle, 0, &C_icon_app, "Blind signing:", viewdata.value, 33, 12},
#endif

    {NULL, NULL, 0, &C_icon_app, APPVERSION_LINE1, APPVERSION_LINE2, 33, 12},

    {NULL,
#ifdef APP_SECRET_MODE_ENABLED
     h_secret_click,
#else
     NULL,
#endif
     0, &C_icon_app, "Developed by:", "Zondax.ch", 33, 12},

    {NULL, NULL, 0, &C_icon_app, "License: ", "Apache 2.0", 33, 12},
    {NULL, os_exit, 0, &C_icon_dashboard, "Quit", NULL, 50, 29},
    UX_MENU_END
};

const ux_menu_entry_t menu_initialize[] = {
    {NULL, NULL, 0, &C_icon_app, MENU_MAIN_APP_LINE1, viewdata.key, 33, 12},
    {NULL, h_initialize, 0, &C_icon_app, "Click to", "Initialize", 33, 12},
    {NULL, h_expert_toggle, 0, &C_icon_app, "Expert mode:", viewdata.value, 33, 12},
    {NULL, NULL, 0, &C_icon_app, APPVERSION_LINE1, APPVERSION_LINE2, 33, 12},
    {NULL, NULL, 0, &C_icon_app, "Developed by:", "Zondax.ch", 33, 12},
    {NULL, NULL, 0, &C_icon_app, "License: ", "Apache 2.0", 33, 12},
    {NULL, os_exit, 0, &C_icon_dashboard, "Quit", NULL, 50, 29},
    UX_MENU_END
};

static const bagl_element_t view_message[] = {
    UI_BACKGROUND,
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
    UI_LabelLine(UIID_LABEL + 1, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
};

static const bagl_element_t view_review[] = {
    UI_BACKGROUND_LEFT_RIGHT_ICONS,
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
    UI_LabelLine(UIID_LABEL + 1, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
    UI_LabelLine(UIID_LABEL + 2, 0, 30, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value2),
};

static const bagl_element_t view_error[] = {
    UI_FillRectangle(0, 0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, 0x000000, 0xFFFFFF),
    UI_Icon(0, 128 - 7, 0, 7, 7, BAGL_GLYPH_ICON_CHECK),
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
    UI_LabelLine(UIID_LABEL + 0, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
    UI_LabelLineScrolling(UIID_LABELSCROLL, 0, 30, 128, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value2),
};

static unsigned int view_error_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            h_error_accept(0);
            break;
    }
    return 0;
}

static unsigned int view_message_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            break;
    }
    return 0;
}

static unsigned int view_review_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            h_review_button_both();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Press left to progress to the previous element
            h_review_button_left();
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Press right to progress to the next element
            h_review_button_right();
            break;
    }
    return 0;
}

const bagl_element_t* idle_preprocessor(const ux_menu_entry_t* entry, bagl_element_t* element) {
    switch(ux_menu.current_entry) {
        case 0:
            break;
        case 1:
            h_expert_update();
            break;
        case 2:
#ifdef APP_ACCOUNT_MODE_ENABLED
            h_account_update();
#endif
            break;
        case 3:
#ifdef APP_BLIND_MODE_ENABLED
            h_blind_update();
#endif
            break;
        default:
            break;
    }
    return element;
}

const bagl_element_t *view_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONLEFT:
            if (!h_paging_can_decrease()){
                return NULL;
            }
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case UIID_ICONRIGHT:
            if (!h_paging_can_increase()){
                return NULL;
            }
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case UIID_ICONREVIEW:
            if (!h_paging_intro_screen()){
                return NULL;
            }
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case UIID_LABELSCROLL:
            UX_CALLBACK_SET_INTERVAL(
                MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7))
            );
            break;
    }
    return element;
}

const bagl_element_t *view_prepro_idle(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONLEFT:
        case UIID_ICONRIGHT:
            return NULL;
    }
    return element;
}

void h_review_update() {
    zxerr_t err = h_review_update_data();
    switch(err) {
        case zxerr_ok:
            UX_DISPLAY(view_review, view_prepro);
            break;
        default:
            view_error_show();
            UX_WAIT();
            break;
    }
}

void h_review_button_left() {
    zemu_log_stack("h_review_button_left");
    h_paging_decrease();
    h_review_update();
}

void h_review_button_right() {
    zemu_log_stack("h_review_button_right");
    h_paging_increase();
    h_review_update();
}

void h_review_button_both() {
    zemu_log_stack("h_review_button_both");
    h_review_action(mustReply);
}

//////////////////////////
//////////////////////////
//////////////////////////
//////////////////////////
//////////////////////////

void view_initialize_show_impl(uint8_t item_idx, char *statusString) {
    if (statusString == NULL ) {
        snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", MENU_MAIN_APP_LINE2);
    } else {
        snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", statusString);
    }
    UX_MENU_DISPLAY(item_idx, menu_initialize, NULL);
}

void view_idle_show_impl(uint8_t item_idx, char *statusString) {
    if (statusString == NULL ) {
        snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", MENU_MAIN_APP_LINE2);
#ifdef APP_SECRET_MODE_ENABLED
        if (app_mode_secret()) {
            snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", MENU_MAIN_APP_LINE2_SECRET);
        }
#endif
    } else {
        snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", statusString);
    }
    UX_MENU_DISPLAY(item_idx, menu_main, idle_preprocessor);
}

void view_message_impl(char *title, char *message) {
    snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", title);
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE_LINE, "%s", message);
    UX_DISPLAY(view_message, view_prepro_idle);
}

void view_error_show_impl() {
    UX_DISPLAY(view_error, view_prepro);
}

void h_expert_toggle() {
    app_mode_set_expert(!app_mode_expert());
    view_idle_show(1, NULL);
}

void h_expert_update() {
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE_LINE, "disabled");
    if (app_mode_expert()) {
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE_LINE, "enabled");
    }
}

#ifdef APP_ACCOUNT_MODE_ENABLED
void h_account_toggle() {
    if(app_mode_expert()) {
        account_enabled();
    } else {
        view_idle_show(2, NULL);
    }
}

void h_account_update() {
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, ACCOUNT_DEFAULT);
    if (app_mode_account()) {
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, ACCOUNT_SECONDARY);
    }
}
#endif

#ifdef APP_BLIND_MODE_ENABLED
void h_blind_toggle() {
    if(app_mode_expert()) {
        blind_enabled();
    } else {
        view_idle_show(3, NULL);
    }
}

void h_blind_update() {
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "Disabled");
    if (app_mode_blind()) {
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "Enabled");
    }
}
#endif

#ifdef APP_SECRET_MODE_ENABLED
void h_secret_click() {
    if (COIN_SECRET_REQUIRED_CLICKS == 0) {
        // There is no secret mode
        return;
    }

    viewdata.secret_click_count++;

    char buffer[50];
    snprintf(buffer, sizeof(buffer), "secret click %d\n", viewdata.secret_click_count);
    zemu_log(buffer);

    if (viewdata.secret_click_count >= COIN_SECRET_REQUIRED_CLICKS) {
        secret_enabled();
        viewdata.secret_click_count = 0;
    }
}
#endif

void view_review_show_impl(unsigned int requireReply) {
    zemu_log_stack("view_review_show_impl");
    mustReply = requireReply;

    h_paging_init();

    zxerr_t err = h_review_update_data();
    switch(err) {
        case zxerr_ok:
            UX_DISPLAY(view_review, view_prepro);
            break;
        default:
            view_error_show();
            break;
    }
}

void splitValueField() {
    print_value2("");
    const uint16_t vlen = strlen(viewdata.value);
    if (vlen > MAX_CHARS_PER_VALUE2_LINE - 1) {
        snprintf(viewdata.value2, MAX_CHARS_PER_VALUE2_LINE, "%s", viewdata.value + MAX_CHARS_PER_VALUE_LINE);
        viewdata.value[MAX_CHARS_PER_VALUE_LINE] = 0;
    }
}
void splitValueAddress() {
    uint8_t len = MAX_CHARS_PER_VALUE_LINE;
    bool exceeding_max = exceed_pixel_in_display(len);
    while(exceeding_max && len--) {
        exceeding_max = exceed_pixel_in_display(len);
    }
    print_value2("");
    const uint16_t vlen = strlen(viewdata.value);
    //if viewdata.value == NULL --> len = 0
    if (vlen > len && len > 0) {
        snprintf(viewdata.value2, MAX_CHARS_PER_VALUE2_LINE, "%s", viewdata.value + len);
        viewdata.value[len] = 0;
    }
}

max_char_display get_max_char_per_line() {
    uint8_t len = MAX_CHARS_PER_VALUE_LINE;
    bool exceeding_max = exceed_pixel_in_display(len);
    while(exceeding_max && len--) {
        exceeding_max = exceed_pixel_in_display(len);
    }
    //MAX_CHARS_PER_VALUE1_LINE is defined this way
    return (len > 0) ? (2 * len + 1) : len;
}

bool exceed_pixel_in_display(const uint8_t length) {
    const unsigned short strWidth = zx_compute_line_width_light(viewdata.value, length);
    return (strWidth >= (BAGL_WIDTH - BAGL_WIDTH_MARGIN));
}
#endif

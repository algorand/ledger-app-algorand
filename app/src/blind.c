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

#include "app_main.h"
#include "app_mode.h"
#include "coin.h"
#include "cx.h"
#include "os.h"
#include "tx.h"
#include "view.h"
#include "view_internal.h"
#include "zxformat.h"

void blind_toggle() {
#ifdef APP_BLIND_MODE_ENABLED
  app_mode_set_blind(!app_mode_blind());
  view_idle_show(0, NULL);
#endif
}

static const char *blindMessage =
    "You are about to enable blind signing mode. If you are not sure why you "
    "are here, reject or unplug your device immediately. Activating this mode "
    "will allow you to sign transactions without reviewing each transaction "
    "field.";

zxerr_t blind_getNumItems(uint8_t *num_items) {
  zemu_log_stack("blind_getNumItems");
  *num_items = 1;
  return zxerr_ok;
}

zxerr_t blind_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen,
                      char *outVal, uint16_t outValLen, uint8_t pageIdx,
                      uint8_t *pageCount) {
  if (displayIdx != 0) {
    return zxerr_no_data;
  }

  snprintf(outKey, outKeyLen, "Warning!");

  pageString(outVal, outValLen, (char*)PIC(blindMessage), pageIdx, pageCount);
  return zxerr_ok;
}

zxerr_t blind_enabled() {
#ifdef APP_BLIND_MODE_ENABLED
  view_review_init(blind_getItem, blind_getNumItems, blind_toggle);
  view_review_show(REVIEW_UI);
#endif
  return zxerr_ok;
}

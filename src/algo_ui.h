#ifndef _ALGO_UI_H_
#define _ALGO_UI_H_

#include <stdint.h>

extern char caption[20];
extern char text[128];

void ui_idle();
void ui_address_approval();
void ui_txn();
void ux_approve_txn();

void ui_text_put(const char *msg);
void ui_text_put_u64(uint64_t v);

#define ALGORAND_DECIMALS 6
#endif

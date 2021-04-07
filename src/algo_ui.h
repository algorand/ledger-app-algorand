#ifndef _ALGO_UI_H_
#define _ALGO_UI_H_
#include "ux.h"

extern char lineBuffer[];
extern char caption[20];
extern char text[128];

void ui_idle();
void ui_address_approval();
void ui_txn();
void ux_approve_txn();

void ui_text_put(const char *msg);

#define ALGORAND_PUBLIC_KEY_SIZE 32
#define ALGORAND_DECIMALS 6
#endif
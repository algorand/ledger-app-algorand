#include "ux.h"

extern char lineBuffer[];
extern char caption[20];
extern char text[128];

void ui_idle();
void ui_address_approval();
void ui_txn();
void ux_approve_txn();

void ui_text_put(const char *msg);
void ui_text_putn(const char *msg, size_t maxlen);

#define ALGORAND_PUBLIC_KEY_SIZE 32

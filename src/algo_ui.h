#include <stdint.h>

extern char lineBuffer[];
extern char caption[20];
extern char text[128];

void ui_idle();
void ui_address_approval();
void ui_txn();
void ux_approve_txn();

char *u64str(uint64_t v);

void ui_text_put(const char *msg);
void ui_text_putn(const char *msg, size_t maxlen);
void ui_text_put_addr(const uint8_t *public_key);
void ui_text_put_u64(uint64_t v);

#define ALGORAND_PUBLIC_KEY_SIZE 32
#define ALGORAND_DECIMALS 6

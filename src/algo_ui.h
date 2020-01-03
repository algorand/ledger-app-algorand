extern char lineBuffer[];
extern char text[128];

void ui_idle();
void ui_address();
void ui_txn();

void ui_text_put(const char *msg);
void ui_text_putn(const char *msg, size_t maxlen);
int  ui_text_more();

enum UI_STATE {
  UI_IDLE,
  UI_ADDRESS,
  UI_TXN,
  UI_APPROVAL
};

extern enum UI_STATE uiState;
extern char lineBuffer[];

void ui_idle();
void ui_address();
void ui_txn();

void ui_text_put(const char *msg);
int  ui_text_more();

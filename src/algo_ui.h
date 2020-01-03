#if defined(TARGET_NANOX)
#include "ux.h"
#endif // TARGET_NANOX

extern char lineBuffer[];
extern char text[128];

void ui_idle();
void ui_address();
void ui_txn();

void ui_text_put(const char *msg);
void ui_text_putn(const char *msg, size_t maxlen);
int  ui_text_more();

// Override some of the Ledger X UI macros to enable step skipping
#if defined(TARGET_NANOX)

// If going backwards, skip backwards. Otherwise, skip forwards.
#define SKIPEMPTY \
        if (G_button_mask & BUTTON_LEFT) { \
                ux_flow_prev(); \
        } else { \
                ux_flow_next(); \
        } \
        return

#define ALGO_UX_STEP_NOCB_INIT(txtype, stepname, layoutkind, preinit, ...) \
        void stepname ##_init (unsigned int stack_slot) { \
                if (txtype != ALL_TYPES && txtype != current_txn.type) { SKIPEMPTY; }; \
                if (preinit == 0) { SKIPEMPTY; }; \
                ux_layout_ ## layoutkind ## _init(stack_slot); \
        } \
        const ux_layout_ ## layoutkind ## _params_t stepname ##_val = __VA_ARGS__; \
        const ux_flow_step_t stepname = { \
          stepname ##  _init, \
          & stepname ## _val, \
          NULL, \
          NULL, \
        }
#endif // TARGET_NANOX

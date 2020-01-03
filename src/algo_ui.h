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

void step_address();

// Override some of the Ledger X UI macros to enable step skipping
#if defined(TARGET_NANOX)

// If going backwards, skip backwards. Otherwise, skip forwards.
#define SKIPEMPTY(stepnum) \
        if (stepnum < ux_last_step) { \
                ux_flow_prev(); \
        } else { \
                ux_flow_next(); \
        } \
        return

#define ALGO_UX_STEP_NOCB_INIT(txtype, stepnum, layoutkind, preinit, ...) \
        void txn_flow_ ## stepnum ##_init (unsigned int stack_slot) { \
                if (txtype != ALL_TYPES && txtype != current_txn.type) { SKIPEMPTY(stepnum); }; \
                if (preinit == 0) { SKIPEMPTY(stepnum); }; \
		ux_last_step = stepnum; \
                ux_layout_ ## layoutkind ## _init(stack_slot); \
        } \
        const ux_layout_ ## layoutkind ## _params_t txn_flow_ ## stepnum ##_val = __VA_ARGS__; \
        const ux_flow_step_t txn_flow_ ## stepnum = { \
          txn_flow_ ## stepnum ##  _init, \
          & txn_flow_ ## stepnum ## _val, \
          NULL, \
          NULL, \
        }

#define ALGO_UX_STEP(stepnum, layoutkind, preinit, timeout_ms, validate_cb, error_flow, ...) \
        UX_FLOW_CALL(txn_flow_ ## stepnum ## _validate, { validate_cb; }) \
        void txn_flow_ ## stepnum ##_init (unsigned int stack_slot) { \
                preinit; \
		ux_last_step = stepnum; \
                ux_layout_ ## layoutkind ## _init(stack_slot); \
                ux_layout_set_timeout(stack_slot, timeout_ms);\
        } \
        const ux_layout_ ## layoutkind ## _params_t txn_flow_ ## stepnum ##_val = __VA_ARGS__; \
        const ux_flow_step_t txn_flow_ ## stepnum = { \
          txn_flow_ ## stepnum ##  _init, \
          & txn_flow_ ## stepnum ## _val, \
          txn_flow_ ## stepnum ## _validate, \
          error_flow, \
        }

#endif // TARGET_NANOX

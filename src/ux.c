#include "os.h"
#include "os_io_seproxyhal.h"

#ifdef TARGET_NANOS
// This seems to be implicitly required by the SDK at link-time.
ux_state_t ux;
#endif

#ifdef TARGET_NANOX
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
#endif

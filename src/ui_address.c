#include "os.h"
#include "os_io_seproxyhal.h"

#include "algo_ui.h"
#include "algo_keys.h"
#include "algo_addr.h"

void
step_address()
{
  char checksummed[65];
  checksummed_addr(publicKey, checksummed);
  ui_text_put(checksummed);
}

#ifndef _UI_COMMON_H
#define _UI_COMMON_H

#include "main.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "glyphs.h"

//------------------------------------------------------------------------------

typedef union ui_strings_t {
  struct {
    char address[6][11];
  } show_address;
  struct {
    char rcvAddress[60];
    char closeAddress[60];
    char amount[50];
    char fee[50];
  } tx_approval;
} ui_strings_t;

//------------------------------------------------------------------------------

extern ui_strings_t ui_strings;
extern int ux_step, ux_steps_count;

//------------------------------------------------------------------------------

void ui_common_init();

//------------------------------------------------------------------------------

#endif //_UI_COMMON_H

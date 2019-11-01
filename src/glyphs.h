#ifndef GLYPH_logo_BPP
#define GLYPH_logo_WIDTH 16
#define GLYPH_logo_HEIGHT 16
#define GLYPH_logo_BPP 1
extern
unsigned int const C_logo_colors[]
;
extern	
unsigned char const C_logo_bitmap[];
#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
extern
const bagl_icon_details_t C_logo;
#endif // GLYPH_logo_BPP
#endif // OS_IO_SEPROXYHAL

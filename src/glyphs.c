#include "glyphs.h"
unsigned int const C_app_logo_colors[]
 = {
  0x00262626, 
  0x00f3f3f3, 
};
	
unsigned char const C_app_logo_bitmap[] = {
0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xf8, 0x7f, 0xf0, 0x7f, 0xe2, 0x3f, 0xe2, 0x3f, 0xf3, 
  0x9f, 0xe1, 0x9f, 0xe1, 0xcf, 0xe4, 0x47, 0xe4, 0x67, 0xce, 0x33, 0xce, 0xff, 0xff, 0xff, 0xff, 
  };

#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
const bagl_icon_details_t C_app_logo = { GLYPH_app_logo_WIDTH, GLYPH_app_logo_HEIGHT, 1, C_app_logo_colors, C_app_logo_bitmap };
#endif // OS_IO_SEPROXYHAL
#include "glyphs.h"
unsigned int const C_icon_down_colors[]
 = {
  0x00000000, 
  0x00ffffff, 
};
	
unsigned char const C_icon_down_bitmap[] = {
0x41, 0x11, 0x05, 0x01, };

#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
const bagl_icon_details_t C_icon_down = { GLYPH_icon_down_WIDTH, GLYPH_icon_down_HEIGHT, 1, C_icon_down_colors, C_icon_down_bitmap };
#endif // OS_IO_SEPROXYHAL
#include "glyphs.h"
unsigned int const C_icon_left_colors[]
 = {
  0x00000000, 
  0x00ffffff, 
};
	
unsigned char const C_icon_left_bitmap[] = {
0x48, 0x12, 0x42, 0x08, };

#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
const bagl_icon_details_t C_icon_left = { GLYPH_icon_left_WIDTH, GLYPH_icon_left_HEIGHT, 1, C_icon_left_colors, C_icon_left_bitmap };
#endif // OS_IO_SEPROXYHAL
#include "glyphs.h"
unsigned int const C_icon_right_colors[]
 = {
  0x00000000, 
  0x00ffffff, 
};
	
unsigned char const C_icon_right_bitmap[] = {
0x21, 0x84, 0x24, 0x01, };

#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
const bagl_icon_details_t C_icon_right = { GLYPH_icon_right_WIDTH, GLYPH_icon_right_HEIGHT, 1, C_icon_right_colors, C_icon_right_bitmap };
#endif // OS_IO_SEPROXYHAL
#include "glyphs.h"
unsigned int const C_icon_up_colors[]
 = {
  0x00000000, 
  0x00ffffff, 
};
	
unsigned char const C_icon_up_bitmap[] = {
0x08, 0x8a, 0x28, 0x08, };

#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
const bagl_icon_details_t C_icon_up = { GLYPH_icon_up_WIDTH, GLYPH_icon_up_HEIGHT, 1, C_icon_up_colors, C_icon_up_bitmap };
#endif // OS_IO_SEPROXYHAL

#include "glyphs.h"

#define GLYPH_icon_left_WIDTH 4
#define GLYPH_icon_left_HEIGHT 7
#define GLYPH_icon_left_BPP 1

unsigned int const C_icon_left_colors[] = {
  0x00000000,
  0x00ffffff,
};

unsigned char const C_icon_left_bitmap[] = {
  0x48, 0x12, 0x42, 0x08,
};

const bagl_icon_details_t C_icon_left = { GLYPH_icon_left_WIDTH, GLYPH_icon_left_HEIGHT, 1, C_icon_left_colors, C_icon_left_bitmap };

#define GLYPH_icon_right_WIDTH 4
#define GLYPH_icon_right_HEIGHT 7
#define GLYPH_icon_right_BPP 1

unsigned int const C_icon_right_colors[] = {
  0x00000000,
  0x00ffffff,
};

unsigned char const C_icon_right_bitmap[] = {
  0x21, 0x84, 0x24, 0x01,
};

const bagl_icon_details_t C_icon_right = { GLYPH_icon_right_WIDTH, GLYPH_icon_right_HEIGHT, 1, C_icon_right_colors, C_icon_right_bitmap };

#define GLYPH_icon_down_WIDTH 7
#define GLYPH_icon_down_HEIGHT 4
#define GLYPH_icon_down_BPP 1

unsigned int const C_icon_down_colors[] = {
  0x00000000,
  0x00ffffff,
};

unsigned char const C_icon_down_bitmap[] = {
  0x41, 0x11, 0x05, 0x01,
};

const bagl_icon_details_t C_icon_down = { GLYPH_icon_down_WIDTH, GLYPH_icon_down_HEIGHT, 1, C_icon_down_colors, C_icon_down_bitmap };

#define GLYPH_icon_up_WIDTH 7
#define GLYPH_icon_up_HEIGHT 4
#define GLYPH_icon_up_BPP 1

unsigned int const C_icon_up_colors[] = {
  0x00000000,
  0x00ffffff,
};

unsigned char const C_icon_up_bitmap[] = {
  0x08, 0x8a, 0x28, 0x08,
};

const bagl_icon_details_t C_icon_up = { GLYPH_icon_up_WIDTH, GLYPH_icon_up_HEIGHT, 1, C_icon_up_colors, C_icon_up_bitmap };

#ifndef FONT_H
#define FONT_H

#include <stdlib.h>


typedef struct font_char {
	uint8_t		fc_width;
	uint32_t	fc_bitmap_offs;
	uint8_t		fc_bitmap_siz;
} font_char_t;

typedef struct font {
	uint8_t		fo_line_height;
	uint8_t		fo_ascii_min;
	uint8_t		fo_ascii_max;
	font_char_t	*fo_chars;
	uint8_t		*fo_bitmap;
} font_t;

#endif

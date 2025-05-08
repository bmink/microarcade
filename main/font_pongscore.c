#include "font.h"
#include "font_pongscore.h"

uint8_t	font_pongscore_bitmaps[] = {

	/* "ASCII 0x30 '0'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0xff, 0x81, 0x81, 0xff, 0x00,

	/* "ASCII 0x31 '1'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x00, 0x00, 0x00, 0xff, 0x00,

	/* "ASCII 0x32 '2'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0xf9, 0x89, 0x89, 0x8f, 0x00,

	/* "ASCII 0x33 '3'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x89, 0x89, 0x89, 0xff, 0x00,

	/* "ASCII 0x34 '4'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x0f, 0x08, 0x08, 0xff, 0x00,

	/* "ASCII 0x35 '5'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x8f, 0x89, 0x89, 0xf9, 0x00,

	/* "ASCII 0x36 '6'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0xff, 0x88, 0x88, 0xf8, 0x00,

	/* "ASCII 0x37 '7'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x01, 0x01, 0x01, 0xff, 0x00,

	/* "ASCII 0x38 '8'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0xff, 0x89, 0x89, 0xff, 0x00,

	/* "ASCII 0x39 '9'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x0f, 0x09, 0x09, 0xff, 0x00
};


font_char_t font_pongscore_chars[] = {

	/* ASCII 0x30 '0' */
	{ 5,	0,	5},

	/* ASCII 0x31 '1' */
	{ 5,	5,	5},

	/* ASCII 0x32 '2' */
	{ 5,	10,	5},

	/* ASCII 0x33 '3' */
	{ 5,	15,	5},

	/* ASCII 0x34 '4' */
	{ 5,	20,	5},

	/* ASCII 0x35 '5' */
	{ 5,	25,	5},

	/* ASCII 0x36 '6' */
	{ 5,	30,	5},

	/* ASCII 0x37 '7' */
	{ 5,	35,	5},

	/* ASCII 0x38 '8' */
	{ 5,	40,	5},

	/* ASCII 0x39 '9' */
	{ 5,	45,	5}
};


font_t font_pongscore = {
	9,
	0x30,
	0x39,
	font_pongscore_chars,
	font_pongscore_bitmaps
};

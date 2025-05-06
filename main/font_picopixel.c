#include "font.h"
#include "font_picopixel.h"

uint8_t	font_picopixel_bitmaps[] = {

	/* "ASCII 0x20 ' '" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x00, 0x00, 0x00,

	/* "ASCII 0x21 '!'" (2x8): vertical mapping, 16 pixels, 2 bytes */
	   0x17, 0x00,

	/* "ASCII 0x22 '"'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x03, 0x00, 0x03, 0x00,

	/* "ASCII 0x23 '#'" (6x8): vertical mapping, 48 pixels, 6 bytes */
	   0x0a, 0x1f, 0x0a, 0x1f, 0x0a, 0x00,

	/* "ASCII 0x24 '$'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x14, 0x3b, 0x1a, 0x00,

	/* "ASCII 0x25 '%'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x19, 0x04, 0x13, 0x00,

	/* "ASCII 0x26 '&'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x0a, 0x15, 0x0a, 0x14, 0x00,

	/* "ASCII 0x27 ''" (2x8): vertical mapping, 16 pixels, 2 bytes */
	   0x03, 0x00,

	/* "ASCII 0x28 '('" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x0e, 0x11, 0x00,

	/* "ASCII 0x29 ')'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x11, 0x0e, 0x00,

	/* "ASCII 0x2a '*'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0a, 0x04, 0x0a, 0x00,

	/* "ASCII 0x2b '+'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x04, 0x0e, 0x04, 0x00,

	/* "ASCII 0x2c ','" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x20, 0x10, 0x00,

	/* "ASCII 0x2d '-'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x04, 0x04, 0x04, 0x00,

	/* "ASCII 0x2e '.'" (2x8): vertical mapping, 16 pixels, 2 bytes */
	   0x10, 0x00,

	/* "ASCII 0x2f '/'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x18, 0x04, 0x03, 0x00,

	/* "ASCII 0x30 '0'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0e, 0x11, 0x0e, 0x00,

	/* "ASCII 0x31 '1'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x02, 0x1f, 0x00,

	/* "ASCII 0x32 '2'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x19, 0x15, 0x12, 0x00,

	/* "ASCII 0x33 '3'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x11, 0x15, 0x0a, 0x00,

	/* "ASCII 0x34 '4'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x07, 0x04, 0x1e, 0x00,

	/* "ASCII 0x35 '5'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x17, 0x15, 0x09, 0x00,

	/* "ASCII 0x36 '6'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0e, 0x15, 0x08, 0x00,

	/* "ASCII 0x37 '7'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x19, 0x05, 0x03, 0x00,

	/* "ASCII 0x38 '8'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0a, 0x15, 0x0a, 0x00,

	/* "ASCII 0x39 '9'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x02, 0x15, 0x0e, 0x00,

	/* "ASCII 0x3a ':'" (2x8): vertical mapping, 16 pixels, 2 bytes */
	   0x0a, 0x00,

	/* "ASCII 0x3b ';'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x10, 0x0a, 0x00,

	/* "ASCII 0x3c '<'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x04, 0x0a, 0x00,

	/* "ASCII 0x3d '='" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0a, 0x0a, 0x0a, 0x00,

	/* "ASCII 0x3e '>'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x0a, 0x04, 0x00,

	/* "ASCII 0x3f '?'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x01, 0x15, 0x02, 0x00,

	/* "ASCII 0x40 '@'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0e, 0x11, 0x16, 0x00,

	/* "ASCII 0x41 'A'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1e, 0x05, 0x1e, 0x00,

	/* "ASCII 0x42 'B'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x15, 0x0a, 0x00,

	/* "ASCII 0x43 'C'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0e, 0x11, 0x11, 0x00,

	/* "ASCII 0x44 'D'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x11, 0x0e, 0x00,

	/* "ASCII 0x45 'E'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x15, 0x15, 0x00,

	/* "ASCII 0x46 'F'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x05, 0x01, 0x00,

	/* "ASCII 0x47 'G'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0e, 0x11, 0x0d, 0x00,

	/* "ASCII 0x48 'H'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x04, 0x1f, 0x00,

	/* "ASCII 0x49 'I'" (2x8): vertical mapping, 16 pixels, 2 bytes */
	   0x1f, 0x00,

	/* "ASCII 0x4a 'J'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x08, 0x10, 0x0f, 0x00,

	/* "ASCII 0x4b 'K'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x06, 0x19, 0x00,

	/* "ASCII 0x4c 'L'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x10, 0x10, 0x00,

	/* "ASCII 0x4d 'M'" (6x8): vertical mapping, 48 pixels, 6 bytes */
	   0x1f, 0x02, 0x0c, 0x02, 0x1f, 0x00,

	/* "ASCII 0x4e 'N'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x1f, 0x02, 0x04, 0x1f, 0x00,

	/* "ASCII 0x4f 'O'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0e, 0x11, 0x0e, 0x00,

	/* "ASCII 0x50 'P'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x3f, 0x05, 0x02, 0x00,

	/* "ASCII 0x51 'Q'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x2e, 0x11, 0x0e, 0x00,

	/* "ASCII 0x52 'R'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x05, 0x1a, 0x00,

	/* "ASCII 0x53 'S'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x12, 0x15, 0x09, 0x00,

	/* "ASCII 0x54 'T'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x01, 0x1f, 0x01, 0x00,

	/* "ASCII 0x55 'U'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0f, 0x10, 0x0f, 0x00,

	/* "ASCII 0x56 'V'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x07, 0x18, 0x07, 0x00,

	/* "ASCII 0x57 'W'" (6x8): vertical mapping, 48 pixels, 6 bytes */
	   0x0f, 0x10, 0x0c, 0x10, 0x0f, 0x00,

	/* "ASCII 0x58 'X'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1b, 0x04, 0x1b, 0x00,

	/* "ASCII 0x59 'Y'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x03, 0x1c, 0x03, 0x00,

	/* "ASCII 0x5a 'Z'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x19, 0x15, 0x13, 0x00,

	/* "ASCII 0x5b '['" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x1f, 0x11, 0x00,

	/* "ASCII 0x5c '\'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x03, 0x04, 0x18, 0x00,

	/* "ASCII 0x5d ']'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x11, 0x1f, 0x00,

	/* "ASCII 0x5e '^'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x02, 0x01, 0x02, 0x00,

	/* "ASCII 0x5f '_'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x20, 0x20, 0x20, 0x20, 0x00,

	/* "ASCII 0x60 '`'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x01, 0x02, 0x00,

	/* "ASCII 0x61 'a'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1a, 0x1a, 0x1c, 0x00,

	/* "ASCII 0x62 'b'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x14, 0x18, 0x00,

	/* "ASCII 0x63 'c'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x08, 0x14, 0x14, 0x00,

	/* "ASCII 0x64 'd'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x18, 0x14, 0x1f, 0x00,

	/* "ASCII 0x65 'e'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0c, 0x16, 0x14, 0x00,

	/* "ASCII 0x66 'f'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x1e, 0x05, 0x00,

	/* "ASCII 0x67 'g'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x2c, 0x2a, 0x1e, 0x00,

	/* "ASCII 0x68 'h'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x04, 0x18, 0x00,

	/* "ASCII 0x69 'i'" (2x8): vertical mapping, 16 pixels, 2 bytes */
	   0x1d, 0x00,

	/* "ASCII 0x6a 'j'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x20, 0x1d, 0x00,

	/* "ASCII 0x6b 'k'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1f, 0x08, 0x14, 0x00,

	/* "ASCII 0x6c 'l'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x0f, 0x10, 0x00,

	/* "ASCII 0x6d 'm'" (6x8): vertical mapping, 48 pixels, 6 bytes */
	   0x1c, 0x04, 0x18, 0x04, 0x18, 0x00,

	/* "ASCII 0x6e 'n'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x1c, 0x04, 0x18, 0x00,

	/* "ASCII 0x6f 'o'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x08, 0x14, 0x08, 0x00,

	/* "ASCII 0x70 'p'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x3c, 0x14, 0x08, 0x00,

	/* "ASCII 0x71 'q'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x08, 0x14, 0x3c, 0x00,

	/* "ASCII 0x72 'r'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x1c, 0x04, 0x00,

	/* "ASCII 0x73 's'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x14, 0x1a, 0x0a, 0x00,

	/* "ASCII 0x74 't'" (3x8): vertical mapping, 24 pixels, 3 bytes */
	   0x0f, 0x12, 0x00,

	/* "ASCII 0x75 'u'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0c, 0x10, 0x1c, 0x00,

	/* "ASCII 0x76 'v'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x0c, 0x10, 0x0c, 0x00,

	/* "ASCII 0x77 'w'" (6x8): vertical mapping, 48 pixels, 6 bytes */
	   0x0c, 0x10, 0x08, 0x10, 0x0c, 0x00,

	/* "ASCII 0x78 'x'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x14, 0x08, 0x14, 0x00,

	/* "ASCII 0x79 'y'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x24, 0x28, 0x1c, 0x00,

	/* "ASCII 0x7a 'z'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x12, 0x1a, 0x16, 0x00,

	/* "ASCII 0x7b '{'" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x04, 0x1b, 0x11, 0x00,

	/* "ASCII 0x7c '|'" (2x8): vertical mapping, 16 pixels, 2 bytes */
	   0x3f, 0x00,

	/* "ASCII 0x7d '}" (4x8): vertical mapping, 32 pixels, 4 bytes */
	   0x11, 0x1b, 0x04, 0x00,

	/* "ASCII 0x7e '~'" (5x8): vertical mapping, 40 pixels, 5 bytes */
	   0x04, 0x02, 0x04, 0x02, 0x00
};


font_char_t font_picopixel_chars[] = {

	/* ASCII 0x20 ' ' */
	{ 3,	0,	3},

	/* ASCII 0x21 '!' */
	{ 2,	3,	2},

	/* ASCII 0x22 '"' */
	{ 4,	5,	4},

	/* ASCII 0x23 '#' */
	{ 6,	9,	6},

	/* ASCII 0x24 '$' */
	{ 4,	15,	4},

	/* ASCII 0x25 '%' */
	{ 4,	19,	4},

	/* ASCII 0x26 '&' */
	{ 5,	23,	5},

	/* ASCII 0x27 '' */
	{ 2,	28,	2},

	/* ASCII 0x28 '(' */
	{ 3,	30,	3},

	/* ASCII 0x29 ')' */
	{ 3,	33,	3},

	/* ASCII 0x2a '*' */
	{ 4,	36,	4},

	/* ASCII 0x2b '+' */
	{ 4,	40,	4},

	/* ASCII 0x2c ',' */
	{ 3,	44,	3},

	/* ASCII 0x2d '-' */
	{ 4,	47,	4},

	/* ASCII 0x2e '.' */
	{ 2,	51,	2},

	/* ASCII 0x2f '/' */
	{ 4,	53,	4},

	/* ASCII 0x30 '0' */
	{ 4,	57,	4},

	/* ASCII 0x31 '1' */
	{ 3,	61,	3},

	/* ASCII 0x32 '2' */
	{ 4,	64,	4},

	/* ASCII 0x33 '3' */
	{ 4,	68,	4},

	/* ASCII 0x34 '4' */
	{ 4,	72,	4},

	/* ASCII 0x35 '5' */
	{ 4,	76,	4},

	/* ASCII 0x36 '6' */
	{ 4,	80,	4},

	/* ASCII 0x37 '7' */
	{ 4,	84,	4},

	/* ASCII 0x38 '8' */
	{ 4,	88,	4},

	/* ASCII 0x39 '9' */
	{ 4,	92,	4},

	/* ASCII 0x3a ':' */
	{ 2,	96,	2},

	/* ASCII 0x3b ';' */
	{ 3,	98,	3},

	/* ASCII 0x3c '<' */
	{ 3,	101,	3},

	/* ASCII 0x3d '=' */
	{ 4,	104,	4},

	/* ASCII 0x3e '>' */
	{ 3,	108,	3},

	/* ASCII 0x3f '?' */
	{ 4,	111,	4},

	/* ASCII 0x40 '@' */
	{ 4,	115,	4},

	/* ASCII 0x41 'A' */
	{ 4,	119,	4},

	/* ASCII 0x42 'B' */
	{ 4,	123,	4},

	/* ASCII 0x43 'C' */
	{ 4,	127,	4},

	/* ASCII 0x44 'D' */
	{ 4,	131,	4},

	/* ASCII 0x45 'E' */
	{ 4,	135,	4},

	/* ASCII 0x46 'F' */
	{ 4,	139,	4},

	/* ASCII 0x47 'G' */
	{ 4,	143,	4},

	/* ASCII 0x48 'H' */
	{ 4,	147,	4},

	/* ASCII 0x49 'I' */
	{ 2,	151,	2},

	/* ASCII 0x4a 'J' */
	{ 4,	153,	4},

	/* ASCII 0x4b 'K' */
	{ 4,	157,	4},

	/* ASCII 0x4c 'L' */
	{ 4,	161,	4},

	/* ASCII 0x4d 'M' */
	{ 6,	165,	6},

	/* ASCII 0x4e 'N' */
	{ 5,	171,	5},

	/* ASCII 0x4f 'O' */
	{ 4,	176,	4},

	/* ASCII 0x50 'P' */
	{ 4,	180,	4},

	/* ASCII 0x51 'Q' */
	{ 4,	184,	4},

	/* ASCII 0x52 'R' */
	{ 4,	188,	4},

	/* ASCII 0x53 'S' */
	{ 4,	192,	4},

	/* ASCII 0x54 'T' */
	{ 4,	196,	4},

	/* ASCII 0x55 'U' */
	{ 4,	200,	4},

	/* ASCII 0x56 'V' */
	{ 4,	204,	4},

	/* ASCII 0x57 'W' */
	{ 6,	208,	6},

	/* ASCII 0x58 'X' */
	{ 4,	214,	4},

	/* ASCII 0x59 'Y' */
	{ 4,	218,	4},

	/* ASCII 0x5a 'Z' */
	{ 4,	222,	4},

	/* ASCII 0x5b '[' */
	{ 3,	226,	3},

	/* ASCII 0x5c '\' */
	{ 4,	229,	4},

	/* ASCII 0x5d ']' */
	{ 3,	233,	3},

	/* ASCII 0x5e '^' */
	{ 4,	236,	4},

	/* ASCII 0x5f '_' */
	{ 5,	240,	5},

	/* ASCII 0x60 '`' */
	{ 3,	245,	3},

	/* ASCII 0x61 'a' */
	{ 4,	248,	4},

	/* ASCII 0x62 'b' */
	{ 4,	252,	4},

	/* ASCII 0x63 'c' */
	{ 4,	256,	4},

	/* ASCII 0x64 'd' */
	{ 4,	260,	4},

	/* ASCII 0x65 'e' */
	{ 4,	264,	4},

	/* ASCII 0x66 'f' */
	{ 3,	268,	3},

	/* ASCII 0x67 'g' */
	{ 4,	271,	4},

	/* ASCII 0x68 'h' */
	{ 4,	275,	4},

	/* ASCII 0x69 'i' */
	{ 2,	279,	2},

	/* ASCII 0x6a 'j' */
	{ 3,	281,	3},

	/* ASCII 0x6b 'k' */
	{ 4,	284,	4},

	/* ASCII 0x6c 'l' */
	{ 3,	288,	3},

	/* ASCII 0x6d 'm' */
	{ 6,	291,	6},

	/* ASCII 0x6e 'n' */
	{ 4,	297,	4},

	/* ASCII 0x6f 'o' */
	{ 4,	301,	4},

	/* ASCII 0x70 'p' */
	{ 4,	305,	4},

	/* ASCII 0x71 'q' */
	{ 4,	309,	4},

	/* ASCII 0x72 'r' */
	{ 3,	313,	3},

	/* ASCII 0x73 's' */
	{ 4,	316,	4},

	/* ASCII 0x74 't' */
	{ 3,	320,	3},

	/* ASCII 0x75 'u' */
	{ 4,	323,	4},

	/* ASCII 0x76 'v' */
	{ 4,	327,	4},

	/* ASCII 0x77 'w' */
	{ 6,	331,	6},

	/* ASCII 0x78 'x' */
	{ 4,	337,	4},

	/* ASCII 0x79 'y' */
	{ 4,	341,	4},

	/* ASCII 0x7a 'z' */
	{ 4,	345,	4},

	/* ASCII 0x7b '{' */
	{ 4,	349,	4},

	/* ASCII 0x7c '|' */
	{ 2,	353,	2},

	/* ASCII 0x7d '} */
	{ 4,	355,	4},

	/* ASCII 0x7e '~' */
	{ 5,	359,	5}
};


font_t font_picopixel = {
	7,
	0x20,
	0x7e,
	font_picopixel_chars,
	font_picopixel_bitmaps
};


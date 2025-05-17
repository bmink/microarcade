#ifndef DISP_H
#define DISP_H

#include "font.h"

#define FRAMESIZ        1024
#define FRAME_WIDTH	128
#define FRAME_HEIGHT	64

extern uint8_t *curframe;
extern uint8_t *lastframe;

typedef struct disp_conf {
	uint8_t	dc_pin_clk;	/* Clock */
	uint8_t	dc_pin_mosi;
	uint8_t	dc_pin_cs;	/* Client Select */
	uint8_t	dc_pin_dc;	/* Data / Command */
	uint8_t	dc_pin_reset;


	int32_t	dc_spi_hz;	/* SPI bus freq */

} disp_conf_t;

int disp_init(disp_conf_t *);

typedef enum disp_mode {
	DISP_MODE_ADHOC,
	DISP_MODE_FPS
} disp_mode_t;

void disp_set_mode(disp_mode_t, int);
void disp_get_mode(disp_mode_t *, int *);

typedef enum disp_overlay {
	DISP_DRAW_ON,
	DISP_DRAW_OFF,
	DISP_DRAW_INVERT
} disp_overlay_t;

void disp_puttext(uint8_t *, const char *, const font_t *, uint16_t, uint16_t);
void disp_sendswapcurframe(void);
void disp_sleep_sendswapcurframe(void);
void disp_clearcurframe(void);
void disp_blt(uint8_t *, uint8_t *, uint32_t, int8_t, int, int);
int disp_contactblt(uint8_t *, uint8_t *, uint32_t, int8_t, int, int);

void disp_scrollframe(uint8_t *, int, uint8_t *, int);

void disp_drawbox(uint8_t *, int, int, int, int, disp_overlay_t);

void disp_transframe(uint8_t *, int);

uint8_t *disp_getframebuf(void);
void disp_releaseframebuf(uint8_t *);

#endif

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

typedef enum disp_overlay {
	DISP_DRAW_ON,
	DISP_DRAW_OFF,
	DISP_DRAW_INVERT
} disp_overlay_t;

extern disp_mode_t	disp_mode;
extern int		disp_fps;

void puttext(uint8_t *, const char *, const font_t *, uint16_t, uint16_t);
void sendswapcurframe(void);
void sleep_sendswapcurframe(void);
void clearcurframe(void);
void blt(uint8_t *, uint8_t *, uint32_t, int8_t, int, int);

void scrollframe(uint8_t *, int, uint8_t *, int);

void drawbox(uint8_t *, int x1, int y1, int x2, int y2, disp_overlay_t);

void transframe(uint8_t *, int);

#endif

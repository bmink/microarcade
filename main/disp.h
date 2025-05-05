#ifndef DISP_H
#define DISP_H

#include "font.h"


#define FRAMESIZ        1024
extern uint8_t *curframe;

typedef struct disp_conf {
	uint8_t	dc_pin_clk;	/* Clock */
	uint8_t	dc_pin_mosi;
	uint8_t	dc_pin_cs;	/* Client Select */
	uint8_t	dc_pin_dc;	/* Data / Command */
	uint8_t	dc_pin_reset;


	int32_t	dc_spi_hz;	/* SPI bus freq */

} disp_conf_t;

int disp_init(disp_conf_t *);

#define DISP_MODE_ADHOC 0
#define DISP_MODE_FPS   1

int disp_set_mode(int);

void puttext(uint8_t *, const char *, const font_t *, uint16_t, uint16_t);
void sendswapcurframe();



#endif

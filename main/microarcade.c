#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_rotary.h"
#include "font.h"
#include "font_c64.h"
#include "font_picopixel.h"
#include "microarcade.h"
#include "disp.h"
#include "ui_menu.h"

static const char *ltag = "microarcade";

#define GPIO_BLINK	2

uint8_t	ball_buf[8] = {
	/* "ball" (8x8): vertical mapping, 64 pixels, 8 bytes */
	   0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c };


typedef struct ball {
	int16_t	xpos;
	int16_t	ypos;
	int8_t	xvel;
	int8_t	yvel;
} ball_t;


#define MAX_BALLCNT 50
ball_t balls[MAX_BALLCNT] = { 0 };
int ballcnt = 10;

void mod_rotarytest_start(void);
void mod_pong_start(void);
void mod_asteroidbelt_start(void);


#define MAIN_MENU_ITEMCNT	2


ui_menu_item_t	main_menu[] = {

	{ "Rotary Test", MIT_CALLFUNC, mod_rotarytest_start, 0, NULL },
	{ "Gfx Perf Test", MIT_NONE, NULL, 0, NULL },
	{ "Pong", MIT_CALLFUNC, mod_pong_start, 0, NULL },
	{ "Etch-a-sketch", MIT_NONE, NULL, 0, NULL },
	{ "Clock", MIT_NONE, NULL, 0, NULL },
	{ "Asteroid Belt", MIT_CALLFUNC, mod_asteroidbelt_start, 0, NULL },
	{ "Lander", MIT_NONE, NULL, 0, NULL },
	{ "Minesweeper", MIT_NONE, NULL, 0, NULL },
	{ "Test1", MIT_NONE, NULL, 0, NULL },
	{ "Test2", MIT_NONE, NULL, 0, NULL },
	{ "Test3", MIT_NONE, NULL, 0, NULL },
	{ "Test4", MIT_NONE, NULL, 0, NULL },
	{ "Test5", MIT_NONE, NULL, 0, NULL },
	{  NULL }
};


#define MAIN_MENU_LINEMAXLEN	16
#define MAIN_MENU_MAXLINECNT	8


void splash(void);

void
app_main(void)
{
	esp_err_t			ret;
	gpio_config_t			gpioconf;
	rotary_config_t			rconf[ROTARY_CNT];
	disp_conf_t			dconf;

	ret = ESP_OK;

	/* Set up onboard LED -- will blink on error */
	memset(&gpioconf, 0, sizeof(gpio_config_t));

	gpioconf.mode = GPIO_MODE_OUTPUT;
	gpioconf.intr_type = GPIO_INTR_DISABLE;
	gpioconf.pin_bit_mask = 1ULL << GPIO_BLINK;
	ret = gpio_config(&gpioconf);
	if(ret != ESP_OK) {
		ESP_LOGE(ltag, "Could not config gpio\n");
		goto err_label;
		
	}

	/* Set up display */
	memset(&dconf, 0, sizeof(dconf));
	dconf.dc_pin_clk = 4;
	dconf.dc_pin_mosi = 5;
	dconf.dc_pin_reset = 6;
	dconf.dc_pin_dc = 7;
	dconf.dc_pin_cs = 15;
	dconf.dc_spi_hz = 10 * 1000 * 1000;

	ret = disp_init(&dconf);
	if(ret != ESP_OK)
		goto err_label;

	memset(rconf, 0, sizeof(rotary_config_t) * ROTARY_CNT);
	rconf[ROTARY_LEFT].rc_pin_a = 45;
	rconf[ROTARY_LEFT].rc_pin_b = 36;
	rconf[ROTARY_LEFT].rc_pin_button = 35;

	rconf[ROTARY_RIGHT].rc_pin_a = 21;
	rconf[ROTARY_RIGHT].rc_pin_b = 47;
	rconf[ROTARY_RIGHT].rc_pin_button = 20;

	if(rotary_config(rconf, ROTARY_CNT) != ESP_OK) {
		ESP_LOGE(ltag, "Could not configure rotary encoders\n");
		goto err_label;
	}

	/* Do splash screen only when first powered on */
	if(esp_reset_reason() == ESP_RST_POWERON)
		splash();

	ui_showmenu(main_menu, MAIN_MENU_MAXLINECNT, MAIN_MENU_LINEMAXLEN,
	    MENU_FULL_SCROLL);

	/* Not reached */
	ESP_LOGE(ltag, "Should not be here, main menu returned");

err_label:

	ESP_LOGE(ltag, "Error: %s\n", esp_err_to_name(ret));

	while (1) {
       		gpio_set_level(GPIO_BLINK, 0);
		vTaskDelay(pdMS_TO_TICKS(100));
		gpio_set_level(GPIO_BLINK, 1);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

	
#if 0


int idx = 0;

int xpos = 25;
int ypos = 0;
int xspeed = 1;
int yspeed = 1;

srand((unsigned int)xTaskGetTickCount());

ballcnt = 10;
for(int i = 0; i < ballcnt; ++i) {
	balls[i].xvel = rand() % 6;
	balls[i].ypos = rand() % 50;
}

int ydirchange;
while(1) {
	
	memset(curframe, 0, 1024);

//	puttext("hello, world", &font_c64, 0, 0);
//	puttext("hello, world", &font_picopixel, 0, 8);

	puttext("hello, world", &font_c64, ypos, 0);
	puttext("hello, world", &font_picopixel, 0, 50-ypos);

	blt(domy_buf[idx], 128, 32, xpos, ypos);
	
	xpos += xspeed;
	ypos += yspeed;
	
	if((xspeed > 0 && xpos >= 128 - 32) ||
	    (xspeed < 0 && xpos <= 0))
		xspeed = -xspeed;

	if((yspeed > 0 && ypos >= 64 - 32) ||
	    (yspeed < 0 && ypos <= 0)) {
		yspeed = -yspeed; 
	}

	for(int i = 0; i < ballcnt; ++i) {
		ball_t *b = &balls[i];
		b->xpos += b->xvel;
		if(b->xpos > 120) {
			b->xpos = 120 - (b->xpos - 120);
			b->xvel = -b->xvel;
		} else
		if(b->xpos < 0) {
			b->xpos = -b->xpos;
			b->xvel = -b->xvel;
		}
			


		ydirchange = 0;
		b->ypos += b->yvel;
		if(b->ypos >= 56) {
//			b->ypos = 56 - (b->ypos - 56) * .7;
			b->ypos = 56;
//printf("old yvel=%d\n", b->yvel);
			b->yvel = -b->yvel * .8;
//			b->yvel = -b->yvel * .6;
//printf("new yvel=%d\n", b->yvel);
			++ydirchange;
			if(abs(b->yvel) < 2) {
				b->ypos = 0;
				b->xpos = 0;
				b->xvel = rand() % 20;
				b->yvel = 0;
			}
		}

		blt(ball_buf, 8, 8, b->xpos, b->ypos);

		if(!ydirchange)
			b->yvel += 2;
//printf("yvel=%d, ypos=%d\n", b->yvel, b->ypos);
	}

	memset(&transact, 0, sizeof(spi_transaction_t));
	transact.length = 1024 * 8;
	transact.tx_buffer = curframe;
	ret = spi_device_transmit(devhand, &transact);
	if(ret != ESP_OK) {
		printf("Could not send data\n");
		goto err_label;
	}


	vTaskDelay(pdMS_TO_TICKS(100));
	++idx;
	if(idx > 23)
		idx = 0;
}




#endif





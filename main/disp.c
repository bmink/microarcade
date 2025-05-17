#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "disp.h"

static const char *ltag = "disp";	/* Tag for logging */

static spi_device_handle_t	     devhand;
static disp_conf_t	dconf;


static uint8_t	frame1[FRAMESIZ]; /* Our ping-pong frames */
static uint8_t	frame2[FRAMESIZ];

uint8_t *curframe = frame1; /* The frame currently being drawn in */

static uint8_t	lframe[FRAMESIZ]; /* Last sent frame (ie. what's currently
				   * being displayed on the screen) */
uint8_t	*lastframe = lframe;

static SemaphoreHandle_t       sendframe_mutex;
static uint8_t		*sendframe = NULL;

static TaskHandle_t	frame_sender_task;

#define DISP_ADHOC_SENDTRY_WAITMS 10

static disp_mode_t disp_mode = DISP_MODE_ADHOC;
static int disp_fps;

static int disp_ticks_per_frame;
static TickType_t disp_last_frame_at;

int disp_dropped_framecnt = 0;

#define FRAMEBUDGET_RAVG_CNT	1000
/* Circular buffer of tick budget (ie. the remaining ticks we had left over
 * per frame) to calculate running average. */
uint32_t framebudget_val[FRAMEBUDGET_RAVG_CNT];
uint32_t framebudget_cur_idx;
uint32_t framebudget_cnt;
uint32_t framebudget_sum;



#define MAX_FPS		100

void
disp_set_mode(disp_mode_t newmode, int fps)
{

	switch(newmode) {
	case DISP_MODE_ADHOC:
		disp_fps = 0;
		disp_ticks_per_frame = 0;

		break;

	case DISP_MODE_FPS:
		disp_fps = fps;

		if(disp_fps >= MAX_FPS)
			disp_fps = MAX_FPS;

		disp_ticks_per_frame = pdMS_TO_TICKS(1000 / disp_fps);
		disp_last_frame_at = xTaskGetTickCount();

		break;
	}
	
	disp_mode = newmode;
}


static void
config_gpio(int pinnr)
{
        gpio_config_t   config;
        int             ret;

        memset(&config, 0, sizeof(gpio_config_t));

        config.mode = GPIO_MODE_OUTPUT;
        config.intr_type = GPIO_INTR_DISABLE;
        config.pin_bit_mask = 1ULL << pinnr;

        ret = gpio_config(&config);
        if(ret != ESP_OK)
                ESP_LOGE(ltag, "Could not config gpio %d", pinnr);
}


static int
sendspi(uint8_t *buf, uint32_t bufsiz)
{
	spi_transaction_t	transact;
	int			ret;

	memset(&transact, 0, sizeof(spi_transaction_t));
	transact.tx_buffer = buf;
	transact.length =  bufsiz * 8;

	ret = spi_device_transmit(devhand, &transact);
	if(ret != ESP_OK) {
		ESP_LOGE(ltag, "Could not send data on SPI");
	}

	return ret;
}


static void
frame_sender_loop(void *arg)
{
	int			had_frame_to_send;

	while(ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
		had_frame_to_send = 0;
		xSemaphoreTake(sendframe_mutex, portMAX_DELAY);
		if(sendframe == NULL) {
			goto next_iter;
		}

		/* Only we can set sendframe back to NULL, and the writer
		 * will not notify as long as it it not NULL, so
		 * it's safe to release the mutex here */
		xSemaphoreGive(sendframe_mutex);

		sendspi(sendframe, FRAMESIZ);

		++had_frame_to_send;	
		xSemaphoreTake(sendframe_mutex, portMAX_DELAY);
		sendframe = NULL;

next_iter:
		xSemaphoreGive(sendframe_mutex);

		if(!had_frame_to_send)
			ESP_LOGE(ltag, "Was woken up but nothing to send!\n");
		
	}
	
}


void
disp_clearcurframe(void)
{
	memset(curframe, 0, FRAMESIZ);
}


void
disp_sendswapcurframe(void)
{
	xSemaphoreTake(sendframe_mutex, portMAX_DELAY);

	if(sendframe != NULL) {
		if(disp_mode == DISP_MODE_FPS) {

			/* We are in fps mode. Frame has to be dropped */

			ESP_LOGE(ltag, "Dropping frame!\n");
			++disp_dropped_framecnt;
			goto end_label;

		} else { /* DISP_MODE_ADHOC */

			/* We are in ad-hoc / slow update mode. Wait until
			 * sendframe is clear and send our frame */

			while(sendframe != NULL) {
				xSemaphoreGive(sendframe_mutex);
				vTaskDelay(pdMS_TO_TICKS(
				    DISP_ADHOC_SENDTRY_WAITMS));
				xSemaphoreTake(sendframe_mutex, portMAX_DELAY);
			}
		}
	}

	sendframe = curframe;
	if(curframe == frame1)
		curframe = frame2;
	else
		curframe = frame1;

	memcpy(lframe, sendframe, FRAMESIZ);

	xTaskNotifyGive(frame_sender_task);

end_label:
	xSemaphoreGive(sendframe_mutex);
	disp_clearcurframe();
}


void
disp_sleep_sendswapcurframe(void)
{

	vTaskDelayUntil(&disp_last_frame_at, disp_ticks_per_frame);

	disp_sendswapcurframe();
}


void
disp_blt(uint8_t *frame, uint8_t *buf, uint32_t buflen, int8_t spritewidth,
	int xpos, int ypos)
{
	/*
	 * Can safely be called with negative / out of bound xpos and ypos
	 * (sprites will be cut off)
	 */
	int	x;
	int	y;
	int	ybase;	
	int	ymod;	
	uint8_t	*framecur;
	uint8_t	*spritecur;
	int	i;
	uint8_t	byte;

	/* Screen buffer is vertically mapped so for y positions that aren't
	 * divisible by 8, we have to do some bit shifting */
		
	ybase = ypos / 8;
	ymod = ypos % 8;
	if(ypos < 0) {
		--ybase;
		ymod = 8 + ymod;
	}

	spritecur = buf;
	framecur = frame + FRAME_WIDTH * ybase + xpos;
	for(i = 0, x = xpos, y = ybase; i < buflen + spritewidth; ++i) {

		if(x >= 0 && x < FRAME_WIDTH && y >= 0 && y < 8) {

			if(i < buflen)
				byte = *spritecur << ymod;
			else	/* Overflow row */
				byte = 0;

			if(i >= spritewidth)
				byte |= *(spritecur - spritewidth) >>
				    (8 - ymod);

			*framecur |= byte;
		}

		++x;
		if(((i+1) % spritewidth) == 0) {
			x = xpos;
			++y;
			framecur = frame + FRAME_WIDTH * y + xpos;
		} else
			++framecur;

		++spritecur;
	}


}


int
disp_contactblt(uint8_t *frame, uint8_t *buf, uint32_t buflen,
	int8_t spritewidth, int xpos, int ypos)
{
	/*
	 * Same as disp_blt(), except doesn't overlay the sprite, but checks
	 * for contact with any previously drawn pixels in the frame.
	 * Returns 0 if the sprite does not contact anything, or nonzero
	 * if it does.
	 */
	int	x;
	int	y;
	int	ybase;	
	int	ymod;	
	uint8_t	*framecur;
	uint8_t	*spritecur;
	int	i;
	uint8_t	byte;

	/* Screen buffer is vertically mapped so for y positions that aren't
	 * divisible by 8, we have to do some bit shifting */
		
	ybase = ypos / 8;
	ymod = ypos % 8;
	if(ypos < 0) {
		--ybase;
		ymod = 8 + ymod;
	}

	spritecur = buf;
	framecur = frame + FRAME_WIDTH * ybase + xpos;
	for(i = 0, x = xpos, y = ybase; i < buflen + spritewidth; ++i) {

		if(x >= 0 && x < FRAME_WIDTH && y >= 0 && y < 8) {

			if(i < buflen)
				byte = *spritecur << ymod;
			else	/* Overflow row */
				byte = 0;

			if(i >= spritewidth)
				byte |= *(spritecur - spritewidth) >>
				    (8 - ymod);

			if(*framecur & byte)
				return 1;
		}

		++x;
		if(((i+1) % spritewidth) == 0) {
			x = xpos;
			++y;
			framecur = frame + FRAME_WIDTH * y + xpos;
		} else
			++framecur;

		++spritecur;
	}

	return 0;
}


void
disp_puttext(uint8_t *frame, const char *text, const font_t *fo, uint16_t x, uint16_t y)
{
	const char 	*ch;
	int		xpos;
	int		ypos;
	uint8_t		*bitmap;
	font_char_t	*fch;

	xpos = x;
	ypos = y;

	for(ch = text; *ch; ++ch) {
		if(*ch == '\n') {
			xpos = x;
			ypos += fo->fo_line_height;
			
			continue;
		}

		if(*ch < fo->fo_ascii_min || *ch > fo->fo_ascii_max) {
			ESP_LOGW(ltag, "Font does not support character: %02x",
			    *ch);
		} else {
			fch = &fo->fo_chars[*ch - fo->fo_ascii_min];
			bitmap = fo->fo_bitmap + fch->fc_bitmap_offs;
			disp_blt(frame, bitmap, fch->fc_bitmap_siz,
			    fch->fc_width, xpos, ypos);
			xpos += fch->fc_width;
		}


	}
}


#define DISP_FRAME_SENDER_LOOP_PRI	20
#define DISP_FRAME_SENDER_LOOP_HEAPSIZ	4096

static uint8_t init_cmds[] = {
	0x20, 0x00,	/* Horizontal addressing mode */
	//0xa5,		/* All pixels on */
	//0xa7,		/* Inverse */
	0xaf		/* Display on */
};

#define SPI_DEV_QUEUE_SIZ	16


int
disp_init(disp_conf_t *conf)
{
	esp_err_t			ret;
	spi_bus_config_t		spiconf;
	spi_device_interface_config_t	devconf;

	ESP_LOGI(ltag, "init called");

	ret = ESP_OK;

	if(conf == NULL) {
		ret = ESP_FAIL;
		goto end_label;
	}

	/* Set up SPI for display */
	memset(&spiconf, 0, sizeof(spi_bus_config_t));
	spiconf.sclk_io_num = conf->dc_pin_clk;
	spiconf.mosi_io_num = conf->dc_pin_mosi;

	spiconf.quadwp_io_num = -1;
	spiconf.quadhd_io_num = -1;

	ret = spi_bus_initialize(SPI2_HOST, &spiconf, SPI_DMA_CH_AUTO);
	if(ret != ESP_OK) {
		ESP_LOGE(ltag, "Could not initialize SPI bus\n");
		goto end_label;
	}

	memset(&devconf, 0, sizeof(spi_device_interface_config_t));
	devconf.clock_speed_hz = conf->dc_spi_hz;
	devconf.spics_io_num = conf->dc_pin_cs;
	devconf.queue_size = SPI_DEV_QUEUE_SIZ;
	
	ret = spi_bus_add_device(SPI2_HOST, &devconf, &devhand);
	if(ret != ESP_OK) {
		ESP_LOGE(ltag, "Could not add device\n");
		goto end_label;
	}

	config_gpio(conf->dc_pin_dc);
	config_gpio(conf->dc_pin_reset);

	dconf = *conf;

	/* Reset display */
	gpio_set_level(dconf.dc_pin_reset, 0);
	vTaskDelay(pdMS_TO_TICKS(100));
	gpio_set_level(dconf.dc_pin_reset, 1);

	/* Send initializing commands */
	gpio_set_level(dconf.dc_pin_dc, 0);
	ret = sendspi(init_cmds, sizeof(init_cmds));
	
	/* Set command/data pin to "data" and leave it there. From now on,
	 * we only send data bytes (ie. frame data) */
	gpio_set_level(dconf.dc_pin_dc, 1);

	ret = xTaskCreate(frame_sender_loop, "frame_sender_loop",
	    DISP_FRAME_SENDER_LOOP_HEAPSIZ, NULL, DISP_FRAME_SENDER_LOOP_PRI,
	    &frame_sender_task);
	if(ret != pdPASS)
		goto end_label;
	else
		ret = ESP_OK;

	sendframe_mutex = xSemaphoreCreateMutex();

end_label:
	return ret;

}


void
disp_scrollframe(uint8_t *frame, int xoffs, uint8_t *newframe, int left)
{
	/* Scrolls frame xoffs pixels to the left or right. If newframe is
	 * not NULL then the new space will be filled in from whatever is in
	 * newframe */

	int x;
	int line;
	uint8_t *framecur;
	uint8_t *newframecur;
	int myoffs;

	myoffs = xoffs;
	if(myoffs > FRAME_WIDTH)
		myoffs = FRAME_WIDTH;

	for(line = 0; line < 8; ++line) {

		if(left) {
			framecur = frame + line * 128;
			for(x = 0; x < FRAME_WIDTH - myoffs; ++x) {
				*framecur = *(framecur + myoffs);
				++framecur;
			}

			if(newframe)
				newframecur = newframe + line * 128;
			else
				newframecur = NULL;

			for(x = 0; x < myoffs; ++x) {
				if(*newframecur)
					*framecur = *newframecur;
				else
					*framecur = 0;
		
				++framecur;
				if(newframecur)	
					++newframecur;
			}
		} else { /* right */
			framecur = frame + line * 128 + FRAME_WIDTH - 1;

			for(x = 0; x < FRAME_WIDTH - myoffs; ++x) {
				*framecur = *(framecur - myoffs);
				--framecur;
			}

			if(newframe)
				newframecur = newframe + line * 128 +
				    FRAME_WIDTH - myoffs;
			else
				newframecur = NULL;

			framecur = frame + line * 128;
			for(x = 0; x < myoffs; ++x) {
				if(*newframecur)
					*framecur = *newframecur;
				else
					*framecur = 0;
		
				++framecur;
				if(newframecur)	
					++newframecur;
			}
		}
	}
}


void
disp_drawbox(uint8_t *frame, int x1, int y1, int x2, int y2,
	disp_overlay_t overlay)
{
	int	x;
	int	y;
	int	boxwidth;
	uint8_t	column[8];
	int	ybase;
	int	ymod;
	int	line;
	uint8_t	*framecur;
	uint8_t	byte;

	if(x2 < x1 || y2 < y1)
		return;
	
	/* Create a 1xFRAME_HEIGHT pixel column representing the box */
	memset(column, 0, 8);
	for(y = y1; y <= y2; ++y) {
		ybase = y / 8;
		ymod = y % 8;
		column[ybase] |= 0x1 << ymod;
	}

	boxwidth = x2 - x1 + 1;

	for(line = 0; line < 8; ++line) {
		byte = column[line];
		if(byte == 0)
			continue;

		framecur = frame + 128 * line + x1;
		for(x = 0; x < boxwidth; ++x) {
			switch(overlay) {
			case DISP_DRAW_ON:
				*framecur |= byte;

				break;

			case DISP_DRAW_OFF:
				*framecur &= ~byte;

				break;

			case DISP_DRAW_INVERT:
				*framecur ^= byte;

				break;
			}
			++framecur;
		}
	}
}


#define TRANSFRAME_FPS               35
#define TRANSFRAME_SCROLL_SPEED      8 /* Must be divisor of FRAME_WIDTH */

void
disp_transframe(uint8_t *newframe, int left)
{

	disp_mode_t     savedmode;
	int             savedfps;
	uint8_t         *nowframe;
	int             xscroll;

	savedmode = disp_mode;
	savedfps = disp_fps;

	nowframe = disp_getframebuf();
	memcpy(nowframe, lastframe, FRAMESIZ);

	disp_set_mode(DISP_MODE_FPS, TRANSFRAME_FPS);

	for(xscroll = 0; xscroll < FRAME_WIDTH;
	    xscroll += TRANSFRAME_SCROLL_SPEED) {
		memcpy(curframe, nowframe, FRAMESIZ);

		disp_scrollframe(curframe, xscroll, newframe, left);

		disp_sleep_sendswapcurframe();
	}

        disp_set_mode(savedmode, savedfps);
	disp_releaseframebuf(nowframe);
}


/* The framebuf functions allow allow applications request / release frame
 * buffers for temporary use. For example, applications may want to hold
 * on to display contents so they can later restore / transition back to it.
 *
 * We implement a pool of 8 statically allocated frame buffers. If no more
 * buffers are free, we abort: there simply shouldn't ever
 * be a need for an application to hold that many frames concurrently,
 * so we have very likely gotten ourselves in a very bad state. */
static uint8_t	framebufs[8][FRAMESIZ];
static uint8_t	framebufs_busy = 0;

uint8_t *
disp_getframebuf(void)
{
	int	i;
	uint8_t	busybyte;

	for(busybyte = 0x01, i = 0; i < 8; busybyte <<= 1, ++i) {
		if((framebufs_busy & busybyte) == 0) {
			framebufs_busy |= busybyte;
			return framebufs[i];
		}
	}

	esp_system_abort("Out of framebufs");
}


void
disp_releaseframebuf(uint8_t *buf)
{
	int	i;
	uint8_t	busybyte;

	for(busybyte = 0x01, i = 0; i < 8; busybyte <<= 1, ++i) {
		if(buf == framebufs[i]) {
			framebufs_busy &= ~busybyte;
			memset(framebufs[i], 0, FRAMESIZ);
			return;
		}
	}

	ESP_LOGE(ltag, "Trying to release an unknown framebuf");
}


void
disp_get_mode(disp_mode_t *modep, int *fpsp)
{
	*modep = disp_mode;
	*fpsp = disp_fps;
}

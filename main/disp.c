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

uint8_t *curframe = frame1;

static SemaphoreHandle_t       sendframe_mutex;
static uint8_t		*sendframe = NULL;

static TaskHandle_t	frame_sender_task;

#define DISP_ADHOC_SENDTRY_WAITMS 10

int disp_mode = DISP_MODE_ADHOC;

static int disp_fps;
static int disp_ticks_per_frame;

int dropped_framecnt = 0;

#define FRAMEBUDGET_RAVG_CNT	1000
/* Circular buffer of tick budget (ie. the remaining ticks we had left over
 * per frame) to calculate running average. */
uint32_t framebudget_val[FRAMEBUDGET_RAVG_CNT];
uint32_t framebudget_cur_idx;
uint32_t framebudget_cnt;
uint32_t framebudget_sum;


void
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


int
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
clearcurframe(void)
{
	memset(curframe, 0, FRAMESIZ);
}


void
sendswapcurframe(void)
{
	xSemaphoreTake(sendframe_mutex, portMAX_DELAY);

	if(sendframe != NULL) {
		if(disp_mode == DISP_MODE_FPS) {

			/* We are in fast mode. Frame has to be dropped */

			printf("Dropping frame!\n");
			++dropped_framecnt;
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

	xTaskNotifyGive(frame_sender_task);

end_label:
	xSemaphoreGive(sendframe_mutex);
	clearcurframe();
}

void
blt(uint8_t *frame, uint8_t *buf, uint32_t buflen, int8_t spritewidth, int xpos, int ypos)
{
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

	spritecur = buf;
	framecur = frame + 128 * ybase + xpos;
	for(i = 0, x = xpos, y = ybase; i < buflen + spritewidth; ++i) {

		if(x < 128 && y < 8) {

			if(i < buflen)
				byte = *spritecur << ymod;
			else	/* Overflow row */
				byte = 0;

			if(i >= spritewidth)
				byte |= *(spritecur - spritewidth) >> (8 - ymod);

			*framecur |= byte;
		}

		++x;
//		if(i > 0 && ((i+1) % spritewidth) == 0) {
		if(((i+1) % spritewidth) == 0) {
			x = xpos;
			++y;
			framecur = frame + 128 * y + xpos;
		} else
			++framecur;

		++spritecur;
	}


}


void
puttext(uint8_t *frame, const char *text, const font_t *fo, uint16_t x, uint16_t y)
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
			/* Unsupported char */
			fch = &fo->fo_chars['?' - fo->fo_ascii_min];
		} else {
			fch = &fo->fo_chars[*ch - fo->fo_ascii_min];
		}

		bitmap = fo->fo_bitmap + fch->fc_bitmap_offs;

		blt(frame, bitmap, fch->fc_bitmap_siz, fch->fc_width,
		    xpos, ypos);

		xpos += fch->fc_width;

	}
}


#define DISP_FRAME_SENDER_LOOP_PRI	20
#define DISP_FRAME_SENDER_LOOP_HEAPSIZ	4096

uint8_t init_cmds[] = {
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
		printf("Could not add device\n");
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

	

#if 0




#endif












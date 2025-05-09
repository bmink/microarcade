#include <string.h>
#include "microarcade.h"
#include "esp_log.h"
#include "esp_rotary.h"
#include "disp.h"
#include "font.h"
#include "font_c64.h"
#include "local_context.h"

static const char *ltag = "rotarytest";

#define MAXVALSTRLEN	5

#define PRESSTEXT	"Press"

void
mod_rotarytest_start(void)
{
	rotary_config_t	rconf[ROTARY_CNT];	
	int		ret;
	char		valstr[MAXVALSTRLEN];
	rotary_event_t	rev;
	int32_t		val;
	int		pressedcnt;
	int		maxpressed;
	local_context_t	savedlc;

	ESP_LOGI(ltag, "Rotary Test staring");

	save_lcontext(&savedlc);

	memset(rconf, 0, sizeof(rotary_config_t) * ROTARY_CNT);
	rconf[ROTARY_LEFT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_LEFT].rc_min = 0;
	rconf[ROTARY_LEFT].rc_max = FRAME_WIDTH - 1;
	rconf[ROTARY_LEFT].rc_start = 0;
	rconf[ROTARY_LEFT].rc_enable_speed_boost = 1;

	rconf[ROTARY_RIGHT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_RIGHT].rc_min = 0;
	rconf[ROTARY_RIGHT].rc_max = FRAME_WIDTH - 1;
	rconf[ROTARY_RIGHT].rc_start = FRAME_WIDTH - 1;
	rconf[ROTARY_RIGHT].rc_enable_speed_boost = 1;

	ret = rotary_reconfig(rconf, ROTARY_CNT);	
	if(ret != ESP_OK) {
		ESP_LOGE(ltag, "Could not reconfigure rotarys");
		goto end_label;
	}

	maxpressed = 0;
	while(1) {
		val = rotary_get_value(ROTARY_LEFT);
		snprintf(valstr, MAXVALSTRLEN, "%"PRIu32, val);
		puttext(curframe, valstr, &font_c64, 0, 22);
		drawbox(curframe, 0, 12, val, 19, DISP_DRAW_ON);

		val = rotary_get_value(ROTARY_RIGHT);
		snprintf(valstr, MAXVALSTRLEN, "%"PRIu32, val);
		puttext(curframe, valstr, &font_c64,
		    FRAME_WIDTH - 1 - strlen(valstr) * 8, 24);
		drawbox(curframe, val, 33, FRAME_WIDTH - 1, 40, DISP_DRAW_ON);

		pressedcnt = 0;
		if(rotary_get_button_state(ROTARY_LEFT) == BUTTON_PRESSED) {
			puttext(curframe, PRESSTEXT, &font_c64, 0, 4);
			++pressedcnt;
		}

		if(rotary_get_button_state(ROTARY_RIGHT) == BUTTON_PRESSED) {
			puttext(curframe, PRESSTEXT, &font_c64,
			    FRAME_WIDTH - 1 - strlen(PRESSTEXT) * 8, 42);
			++pressedcnt;
		}

		if(pressedcnt > maxpressed)
			maxpressed = pressedcnt;

		sendswapcurframe();

		/* Bail when both buttons were pressed *and* released */
		if(pressedcnt == 0 && maxpressed == ROTARY_CNT)
			goto end_label;

		/* Wait until something happens with he rotarys */
		xQueueReceive(rotary_event_queue, &rev, portMAX_DELAY);
	}


end_label:

	restore_lcontext(&savedlc);

	ESP_LOGI(ltag, "Rotary Test exiting");


}

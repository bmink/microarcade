#include <string.h>
#include "microarcade.h"
#include "esp_log.h"
#include "esp_rotary.h"
#include "disp.h"
#include "font.h"
#include "font_c64.h"


const char *ltag = "rotarytest";

#define MAXVALSTRLEN	5

void
mod_rotarytest_start(void)
{
	rotary_config_t	rconf[ROTARY_CNT];	
	int		ret;
	char		valstr[MAXVALSTRLEN];
	rotary_event_t	rev;

	ESP_LOGI(ltag, "mod_rotarytest_start() called");

	memset(rconf, 0, sizeof(rotary_config_t) * ROTARY_CNT);
	rconf[ROTARY_LEFT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_LEFT].rc_min = 0;
	rconf[ROTARY_LEFT].rc_max = 127;
	rconf[ROTARY_LEFT].rc_start = 0;
	rconf[ROTARY_LEFT].rc_enable_speed_boost = 1;

	rconf[ROTARY_RIGHT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_RIGHT].rc_min = 0;
	rconf[ROTARY_RIGHT].rc_max = 127;
	rconf[ROTARY_RIGHT].rc_start = 127;
	rconf[ROTARY_RIGHT].rc_enable_speed_boost = 1;

	ret = rotary_reconfig(rconf, ROTARY_CNT);	
	if(ret != ESP_OK) {
		ESP_LOGE(ltag, "mod_rotarytest_start() called");
		goto end_label;
	}

	while(1) {
		clearcurframe();
	
		snprintf(valstr, MAXVALSTRLEN, "%"PRIu32,
		    rotary_get_value(ROTARY_LEFT));

		puttext(curframe, valstr, &font_c64, 0, 24);

		snprintf(valstr, MAXVALSTRLEN, "%"PRIu32,
		    rotary_get_value(ROTARY_RIGHT));

		puttext(curframe, valstr, &font_c64, 128 - strlen(valstr) * 8,
		    24);

		sendswapcurframe();

		/* Wait until something happens with he rotarys */
		xQueueReceive(rotary_event_queue, &rev, portMAX_DELAY);

		if(rotary_get_button_state(ROTARY_LEFT) == BUTTON_PRESSED && 
		    rotary_get_button_state(ROTARY_RIGHT) == BUTTON_PRESSED)
			goto end_label;
	}


end_label:

	ESP_LOGI(ltag, "Restarting");

	esp_restart();

	/* Not reached */
}

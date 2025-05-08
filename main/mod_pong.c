#include <string.h>
#include <stdlib.h>
#include "microarcade.h"
#include "esp_log.h"
#include "esp_rotary.h"
#include "disp.h"
#include "font.h"
#include "font_c64.h"
#include "font_pongscore.h"


static const char *ltag = "pong";

static uint8_t ball[4] = { 0x0f, 0x0f, 0x0f, 0x0f  };

#define BALL_WIDTH	4
#define BALL_HEIGHT	4


static uint8_t middle_line[16] = { 0x33, 0x33, 0x33, 0x33,
				   0x33, 0x33, 0x33, 0x33,
				   0x33, 0x33, 0x33, 0x33,
				   0x33, 0x33, 0x33, 0x33 };
#define MIDDLE_LINE_WIDTH	2	
#define MIDDLE_LINE_XPOS	63

static uint8_t paddle[6] = { 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f };
#define PADDLE_WIDTH	3
#define PADDLE_HEIGHT	12	
#define PADDLE_XOFFS	3

#define PONG_MODE_SINGLE	0
#define PONG_MODE_MULTI		1

static int pong_mode = PONG_MODE_SINGLE;

#define PONG_FPS	30

#define BALL_DECIMAL_FACTOR	10

#define WINSCORE	11

#define SCORETXT_MAXLEN	5

static void
pong_newgame(void)
{
	rotary_config_t	rconf[ROTARY_CNT];	
	int		ret;
	int		pressedcnt;
	int		maxpressed;
	float		ballxpos;
	float		ballypos;
	float		ballxvel;
	float		ballyvel;
	int		firsthorizbounce;
	int		rpaddleypos;
	int		lpaddleypos;
	int		newserve;
	int		lscore;
	int		rscore;
	char		scoretxt[SCORETXT_MAXLEN];


	memset(rconf, 0, sizeof(rotary_config_t) * ROTARY_CNT);
	rconf[ROTARY_LEFT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_LEFT].rc_min = 0;
	rconf[ROTARY_LEFT].rc_max = FRAME_HEIGHT - PADDLE_HEIGHT - 1;
	rconf[ROTARY_LEFT].rc_start = (FRAME_HEIGHT - PADDLE_HEIGHT) / 2;
	rconf[ROTARY_LEFT].rc_step_value = 2;
	rconf[ROTARY_LEFT].rc_enable_speed_boost = 1;

	rconf[ROTARY_RIGHT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_RIGHT].rc_min = 0;
	rconf[ROTARY_RIGHT].rc_max = FRAME_HEIGHT - PADDLE_HEIGHT - 1;
	rconf[ROTARY_RIGHT].rc_start = (FRAME_HEIGHT - PADDLE_HEIGHT) / 2;
	rconf[ROTARY_RIGHT].rc_step_value = 2;
	rconf[ROTARY_RIGHT].rc_enable_speed_boost = 1;

	ret = rotary_reconfig(rconf, ROTARY_CNT);	
	if(ret != ESP_OK) {
		ESP_LOGE(ltag, "Could not reconfigure rotarys");
		goto end_label;
	}

	srand((unsigned int)xTaskGetTickCount());

	disp_set_mode(DISP_MODE_FPS, PONG_FPS);

	maxpressed = 0;
	lscore = 0;
	rscore = 0;
	newserve = 1;
	while(1) {

		if(newserve) {
			ballxpos = FRAME_WIDTH / 2;	
			ballypos = (rand() % (FRAME_HEIGHT)) - 1 - BALL_HEIGHT;
			ballxvel = (rand() % 2) ? 1 : -1;
			ballyvel = 0;
			firsthorizbounce = 1;
			newserve = 0;
		}

		blt(curframe, middle_line, sizeof(middle_line),
		    MIDDLE_LINE_WIDTH, MIDDLE_LINE_XPOS, 0);

		snprintf(scoretxt, SCORETXT_MAXLEN, "%d", lscore);
		puttext(curframe, scoretxt, &font_pongscore,
		    MIDDLE_LINE_XPOS - 2 - strlen(scoretxt) * 5, 0);

		snprintf(scoretxt, SCORETXT_MAXLEN, "%d", rscore);
		puttext(curframe, scoretxt, &font_pongscore,
		    MIDDLE_LINE_XPOS + MIDDLE_LINE_WIDTH + 2, 0);

		lpaddleypos = rotary_get_value(ROTARY_LEFT);
		blt(curframe, paddle, sizeof(paddle), PADDLE_WIDTH,
		    PADDLE_XOFFS, lpaddleypos);

		rpaddleypos = FRAME_HEIGHT - rotary_get_value(ROTARY_RIGHT) -
		    1 - PADDLE_HEIGHT;
		blt(curframe, paddle, sizeof(paddle), PADDLE_WIDTH,
		    FRAME_WIDTH - 1 - PADDLE_XOFFS - PADDLE_WIDTH, rpaddleypos);

		blt(curframe, ball, sizeof(ball), BALL_WIDTH,
		    ballxpos, ballypos);

		ballxpos += ballxvel;
		
		if(ballxpos < 0) {
			++rscore;
			++newserve;
		} else
		if (ballxpos > FRAME_WIDTH - 1 - BALL_WIDTH) {
			++lscore;
			++newserve;
		} else {
			if((ballxpos <= PADDLE_XOFFS + PADDLE_WIDTH) &&
			    (ballypos + BALL_HEIGHT - 1 >= lpaddleypos) &&
			    (ballypos <= lpaddleypos + PADDLE_HEIGHT)) {
				ballxpos = PADDLE_XOFFS + PADDLE_WIDTH;
				if(firsthorizbounce) {
					ballxvel = -ballxvel * 2;
					firsthorizbounce = 0;
				} else
					ballxvel = -ballxvel;

				ballyvel = (ballypos + BALL_HEIGHT / 2 -
				    (lpaddleypos + PADDLE_HEIGHT / 2)) / 2;
			} else
			if((ballxpos >= FRAME_WIDTH - 1 - PADDLE_XOFFS -
			    PADDLE_WIDTH - BALL_WIDTH) &&
			    (ballypos + BALL_HEIGHT - 1 >= rpaddleypos) &&
		    (ballypos <= rpaddleypos + PADDLE_HEIGHT)) {
				ballxpos = FRAME_WIDTH - 1 - PADDLE_XOFFS -
				    PADDLE_WIDTH - BALL_WIDTH;
				if(firsthorizbounce) {
					ballxvel = -ballxvel * 2;
					firsthorizbounce = 0;
				} else
					ballxvel = -ballxvel;

				ballyvel = (ballypos + BALL_HEIGHT / 2 -
				    (rpaddleypos + PADDLE_HEIGHT / 2)) / 2;
			}
		}

		ballypos += ballyvel;
		if(ballypos < 0) {
			ballypos = 0;
			ballyvel = -ballyvel;
		} else
		if(ballypos > FRAME_HEIGHT - 1 - BALL_HEIGHT) {
			ballypos = FRAME_HEIGHT - 1 - BALL_HEIGHT;
			ballyvel = -ballyvel;
		}

		pressedcnt = 0;
		if(rotary_get_button_state(ROTARY_LEFT) == BUTTON_PRESSED) {
			++pressedcnt;
		}

		if(rotary_get_button_state(ROTARY_RIGHT) == BUTTON_PRESSED) {
			++pressedcnt;
		}

		if(pressedcnt > maxpressed)
			maxpressed = pressedcnt;

		/* Bail when both buttons were pressed *and* released */
		if(pressedcnt == 0 && maxpressed == ROTARY_CNT)
			goto end_label;

		/* Wait until time to send the next frame, then send
		 * the frame. */
		sleep_sendswapcurframe();
	}


end_label:

}


#define MENU_ITEM_SINGLEPLAYER	0
#define MENU_ITEM_MULTIPLAYER	1
#define MENU_ITEM_CNT		2	

static const char *menu_text[MENU_ITEM_CNT] = {
	"Single Player",
	"Multi Player"
};


void
mod_pong_start(void)
{
	ESP_LOGI(ltag, "Pong starting");

	uint8_t prevframe[FRAMESIZ];

	pong_newgame();

	ESP_LOGI(ltag, "Exiting Pong");
}



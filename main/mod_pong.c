#include <string.h>
#include <stdlib.h>
#include "microarcade.h"
#include "esp_log.h"
#include "esp_rotary.h"
#include "disp.h"
#include "font_c64.h"
#include "font_pongscore.h"
#include "ui_menu.h"
#include "local_context.h"


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

#define PONG_FPS	30

#define WINSCORE	11

#define SCORETXT_MAXLEN	15

#define GO_MENU_VAL_PLAY_AGAIN	1
#define GO_MENU_VAL_EXIT	2
#define GO_MENU_LINELEN 	10

static ui_menu_item_t	game_over_menu[] = {
	{ "Play Again", MIT_RETURN_VAL, NULL, GO_MENU_VAL_PLAY_AGAIN, NULL },
	{ "Exit", MIT_RETURN_VAL, NULL, GO_MENU_VAL_EXIT, NULL },
	{ NULL }
};


#define PA_MENU_CONTINUE	1
#define PA_MENU_ABORT		2

static ui_menu_item_t	pause_menu[] = {
	{ "Continue", MIT_RETURN_VAL, NULL, PA_MENU_CONTINUE, NULL },
	{ "End Game", MIT_RETURN_VAL, NULL, PA_MENU_ABORT, NULL },
	{ NULL }
};


#define SECLEN	2

void
show_countdown()
{
	int		i;
	local_context_t	savedlc;
	char		sec[SECLEN];

	save_lcontext(&savedlc);	

	disp_set_mode(DISP_MODE_ADHOC, 0);
	for(i = 3; i > 0; --i) {
		puttext(curframe, "Get Ready!", &font_c64,
		    (FRAME_WIDTH - 10 * 8) / 2, 10);
		sendswapcurframe();
		vTaskDelay(pdMS_TO_TICKS(100));

		puttext(curframe, "Get Ready!", &font_c64,
		    (FRAME_WIDTH - 10 * 8) / 2, 10);
		snprintf(sec, SECLEN, "%d", i);
		puttext(curframe, sec, &font_c64, (FRAME_WIDTH - 8) / 2, 28);
		sendswapcurframe();
		vTaskDelay(pdMS_TO_TICKS(900));
	}

	restore_lcontext(&savedlc);	

}

#define COMP_ACTION_DISTANCE	110

static void
pong_newgame(int lplayercomp, int rplayercomp)
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
	int		ydist;
	local_context_t	savedlc;

	save_lcontext(&savedlc);	

	memset(rconf, 0, sizeof(rotary_config_t) * ROTARY_CNT);
	rconf[ROTARY_LEFT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_LEFT].rc_min = 0;
	rconf[ROTARY_LEFT].rc_max = FRAME_HEIGHT - PADDLE_HEIGHT - 1;
	rconf[ROTARY_LEFT].rc_start = (FRAME_HEIGHT - PADDLE_HEIGHT) / 2;
	rconf[ROTARY_LEFT].rc_step_value = 2;

	rconf[ROTARY_RIGHT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_RIGHT].rc_min = 0;
	rconf[ROTARY_RIGHT].rc_max = FRAME_HEIGHT - PADDLE_HEIGHT - 1;
	rconf[ROTARY_RIGHT].rc_start = (FRAME_HEIGHT - PADDLE_HEIGHT) / 2;
	rconf[ROTARY_RIGHT].rc_step_value = 2;

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
	show_countdown();
	lpaddleypos = rpaddleypos = (FRAME_HEIGHT - PADDLE_HEIGHT) / 2;

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

		if(!lplayercomp) {
			lpaddleypos = rotary_get_value(ROTARY_LEFT);
		} else {
			if(ballxpos < COMP_ACTION_DISTANCE) {
				ydist = (ballypos + BALL_HEIGHT / 2) -
				    (lpaddleypos + PADDLE_HEIGHT / 2);
				if(ydist < -2 && lpaddleypos > 1)
					lpaddleypos -= 2;
				else
				if(ydist > 2 && lpaddleypos < (FRAME_HEIGHT -
				    PADDLE_HEIGHT - 2))
					lpaddleypos += 2;
			}
		}
		blt(curframe, paddle, sizeof(paddle), PADDLE_WIDTH,
		    PADDLE_XOFFS, lpaddleypos);

		if(!rplayercomp) {
			rpaddleypos = FRAME_HEIGHT -
			    rotary_get_value(ROTARY_RIGHT) - 1 - PADDLE_HEIGHT;
		} else {
			if(ballxpos > FRAME_WIDTH - COMP_ACTION_DISTANCE - 1) {
				ydist = (ballypos + BALL_HEIGHT / 2) -
				    (rpaddleypos + PADDLE_HEIGHT / 2);
				if(ydist < -2 && rpaddleypos > 1)
					rpaddleypos -= 2;
				else
				if(ydist > 2 && rpaddleypos < (FRAME_HEIGHT -
				    PADDLE_HEIGHT - 2))
					rpaddleypos += 2;
			}
		}
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

		snprintf(scoretxt, SCORETXT_MAXLEN, "%d", lscore);
		puttext(curframe, scoretxt, &font_pongscore,
		    MIDDLE_LINE_XPOS - 2 - strlen(scoretxt) * 5, 0);

		snprintf(scoretxt, SCORETXT_MAXLEN, "%d", rscore);
		puttext(curframe, scoretxt, &font_pongscore,
		    MIDDLE_LINE_XPOS + MIDDLE_LINE_WIDTH + 2, 0);

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

		/* Show game menu when both buttons were pressed *and*
		 * released */
		if(pressedcnt == 0 && maxpressed == ROTARY_CNT) {
			ret = ui_showmenu(pause_menu, 2, 8,
			    MENU_CENTER_OVERLAY);
			if(ret == PA_MENU_ABORT) {
				goto end_label;
			}
			pressedcnt = maxpressed = 0;
		}

		/* Wait until time to send the next frame, then send
		 * the frame. */
		sleep_sendswapcurframe();

		if(lscore >= WINSCORE || rscore >= WINSCORE) {
			ret = ui_showmenu(game_over_menu, 2, 10,
			    MENU_CENTER_OVERLAY);
			if(ret == GO_MENU_VAL_PLAY_AGAIN) {
				lscore = rscore = 0;
				++newserve;
				show_countdown();
			} else
			if(ret == GO_MENU_VAL_EXIT) {
				goto end_label;
			}
		}
	}

end_label:

	restore_lcontext(&savedlc);	

}

void
pong_newgame_h_h(void)
{
	pong_newgame(0, 0);
}

void
pong_newgame_c_h(void)
{
	pong_newgame(1, 0);
}

void
pong_newgame_h_c(void)
{
	pong_newgame(0, 1);
}

void
pong_newgame_c_c(void)
{
	pong_newgame(1, 1);
}


static ui_menu_item_t	pong_menu[] = {
	{ "Human vs Human", MIT_CALLFUNC, pong_newgame_h_h, 0, NULL },
	{ "Comp. vs Human", MIT_CALLFUNC, pong_newgame_c_h, 0, NULL },
	{ "Human vs Comp.", MIT_CALLFUNC, pong_newgame_h_c, 0, NULL },
	{ "Comp. vs Comp.", MIT_CALLFUNC, pong_newgame_c_c, 0, NULL },
	{ "Back", MIT_RETURN_VAL, NULL, 0, NULL },
	{ NULL }
};


void
mod_pong_start(void)
{
	local_context_t	savedlc;

	ESP_LOGI(ltag, "Pong starting");

	save_lcontext(&savedlc);	

	ui_showmenu(pong_menu, 8, 16, MENU_FULL_SCROLL);
	
	/* If we are here, the user selected "Back" */

	restore_lcontext(&savedlc);	

	ESP_LOGI(ltag, "Exiting Pong");
}



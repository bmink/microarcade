#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "microarcade.h"
#include "esp_log.h"
#include "esp_rotary.h"
#include "disp.h"
#include "font.h"
#include "font_picopixel.h"
#include "font_c64.h"
#include "ui_menu.h"
#include "local_context.h"


static const char *ltag = "abelt";

static uint8_t	ship_buf[20][8];	/* See end of file for sprite data */
static uint8_t	obs_00_buf[32];
static uint8_t	obs_01_buf[128];
static uint8_t	obs_02_buf[128];
static uint8_t	obs_03_buf[96];
static uint8_t	obs_04_buf[96];
static uint8_t	obs_05_buf[72];

static uint8_t	exhaust_buf[2][2] = { { 0x08, 0x00 }, { 0x00, 0x10 } };

static uint8_t	station_buf[2][96];

static uint8_t radar_buf[8][22];

static uint8_t debris_buf[10][8];

typedef struct ab_obstacle_type {
	uint8_t		*at_buf;
	uint16_t	at_bufsiz;
	uint8_t		at_width;
} ab_obstacle_type_t;

#define OBS_TYPE_CNT	6

ab_obstacle_type_t	ab_obstacle_type[OBS_TYPE_CNT] ={
	{ obs_00_buf,	32,	16 },
	{ obs_01_buf,	128,	32 },
	{ obs_02_buf,	128,	32 },
	{ obs_03_buf,	96,	32 },
	{ obs_04_buf,	96,	32 },
	{ obs_05_buf,	72,	24 }
};


//#define OBS_CNT	150
#define OBS_CNT	200
//#define OBS_CNT	500

typedef struct ab_obstacle {
	ab_obstacle_type_t *ao_type;
	int16_t	ao_xpos;
	int16_t	ao_ypos;
} ab_obstacle_t;


#define UNIVERSE_WIDTH	FRAME_WIDTH * 15 
#define UNIVERSE_HEIGHT	FRAME_HEIGHT * 5


#define ABELT_FPS	30

#define SHIP_SPRITESIZ	32
#define SHIP_WIDTH	8
#define SHIP_HALFWIDTH	8
#define SHIP_HEIGHT	8
#define SHIP_HALFHEIGHT	8
#define SHIP_MAXABSVEL	3
#define SHIP_VELINC	.06

#define SHIP_STARTXPOS	FRAME_WIDTH * 3.5
#define SHIP_STARTYPOS	UNIVERSE_HEIGHT / 2

#define SHIP_DISP_XPOS	(FRAME_WIDTH - SHIP_WIDTH) / 2
#define SHIP_DISP_YPOS	(FRAME_HEIGHT - SHIP_HEIGHT) / 2

#define OBS_MINDIST_X	35
#define OBS_MINDIST_Y	35

#define EXHAUST_MAXCNT		6
#define EXHAUST_MAXAGE		ABELT_FPS / 2	/* Half a second */		
#define EXHAUST_BUFSIZ		2
#define EXHAUST_WIDTH		2
#define EXHAUST_HEIGHT		8
#define EXHAUST_FRAMEINTERVAL	ABELT_FPS / 10	/* If throttle on, output
						 * 10 per sec */		
#define EXHAUST_VELFACTOR	2

typedef struct position {
	uint8_t	ap_ttl;		/* Time to live */
	uint8_t	*ap_buf;	/* Sprite buf */
	float	ap_xpos;
	float	ap_ypos;
	float	ap_xvel;
	float	ap_yvel;
} position_t;


#define FUEL_MAXLEVEL		1000
#define FUEL_GAUGE_WIDTH	40
#define FUEL_GAUGE_HEIGHT	8 
#define FUEL_FRAMES_PER_REGEN	30
#define FUEL_GAUGESTRMAX	20

#define RADAR_BUFSIZ	22
#define RADAR_WIDTH	11
#define RADAR_HEIGHT	9

#define STATION_BUFSIZ		96
#define STATION_WIDTH		32
#define STATION_HEIGHT		24
#define STATION_FLIP_FRAMECNT	ABELT_FPS / 2	/* Switch station sprites
						 * twice per second */

#define DEBRIS_SPRITECNT	10
#define DEBRIS_BUFSIZ		8
#define DEBRIS_WIDTH		8	
#define DEBRIS_HEIGHT		8
#define DEBRIS_MAXCNT		5

#define EXPLODE_SPRITECHFRCNT	100 / (1000 / ABELT_FPS)
						 /* Change debris sprite
						  * every ~80 ms */
#define	EXPLODE_DEBRISVELRANDABS	.5

#define END_FRAMECNT		ABELT_FPS * 5	/* End animations last 3 sec */

#define SHIP_WINDIST		10
#define MSG_GAMELOST		"You lose!"
#define MSG_GAMELOST_WIDTH	9 * 8
#define MSG_GAMEWON		"Success!"
#define MSG_GAMEWON_WIDTH	8 * 8

static void
abelt_newgame(void)
{
	rotary_config_t rconf[ROTARY_CNT];
	int		ret;
	int		pressedcnt;
	int		maxpressed;
	float		shipxpos;
	float		shipypos;
	int		ship_angle;
	float		shipxvel;
	float		shipyvel;
	float		shipabsvel;
	float		rad;
	uint8_t		*sbuf;
	ab_obstacle_t	*obstacle;
	ab_obstacle_t	*obs;
	int		i;
	position_t	exhaust[EXHAUST_MAXCNT];
	position_t	*exh;
	int		newexhaust_ttl;
	int		fuel_level;
	char		fuel_gaugestr[FUEL_GAUGESTRMAX];
	int		fuel_gauge_xpos;
	int		fuel_gauge_ypos;
	int		fuel_perc;
	int		radar_xpos;
	int		radar_ypos;
	int		station_xpos;
	int		station_ypos;
	int		station_flipcnt;
	int		station_xdiff;
	int		station_ydiff;
	uint8_t		*stbuf;
	float		radar_rad;
	int		radar_angle;
	int		radar_bufidx;
	position_t	debris[DEBRIS_MAXCNT];
	position_t	*deb;
	int		explode;
	int		gamewon;
	int		explode_sprite_ttl;
	int		end_ttl;

	/* TODO -- convert all X, Y positions to the new position_t
	 * struct */
	
	obstacle = NULL;

	memset(rconf, 0, sizeof(rotary_config_t) * ROTARY_CNT);
	rconf[ROTARY_LEFT].rc_style = ROT_STYLE_WRAPAROUND;
	rconf[ROTARY_LEFT].rc_min = 0;
	rconf[ROTARY_LEFT].rc_max = 342;	
	rconf[ROTARY_LEFT].rc_step_value = 18;
	rconf[ROTARY_LEFT].rc_start = 90;

        ret = rotary_reconfig(rconf, ROTARY_CNT);
        if(ret != ESP_OK) {
                ESP_LOGE(ltag, "Could not reconfigure rotarys");
                goto end_label;
        }

        srand((unsigned int)xTaskGetTickCount());

	obstacle = malloc(OBS_CNT * sizeof(ab_obstacle_t));
	if(obstacle == NULL) {
		ESP_LOGE(ltag, "Could not allocate obstacle array");
		return;
	}

	shipxpos = SHIP_STARTXPOS;
	shipypos = SHIP_STARTYPOS;
	shipxvel = shipyvel = 0;

	station_xpos = UNIVERSE_WIDTH - FRAME_WIDTH - rand() % FRAME_WIDTH;
	station_ypos = UNIVERSE_HEIGHT / 2 - rand() % FRAME_HEIGHT -
	    FRAME_HEIGHT / 2;
	station_flipcnt = 0;
	stbuf = station_buf[0];

	maxpressed = 0;
	ship_angle = 0;
	fuel_level = FUEL_MAXLEVEL;
	radar_xpos = (FRAME_WIDTH - FUEL_GAUGE_WIDTH - RADAR_WIDTH) / 2;
	radar_ypos = FRAME_HEIGHT - RADAR_HEIGHT;
	fuel_gauge_xpos = radar_xpos + RADAR_WIDTH - 1;
	fuel_gauge_ypos = FRAME_HEIGHT - FUEL_GAUGE_HEIGHT - 1;

	explode = 0;
	gamewon = 0;
	end_ttl = 0;
	explode_sprite_ttl = 0;

	for(i = 0; i < OBS_CNT; ++i) {
		obs = &obstacle[i];
		obs->ao_type = &ab_obstacle_type[rand() % OBS_TYPE_CNT];

		while(1) {
			obs->ao_xpos = rand() % UNIVERSE_WIDTH;
			obs->ao_ypos = rand() % UNIVERSE_HEIGHT;

			if(abs((int)shipxpos - obs->ao_xpos) < OBS_MINDIST_X &&
			    abs((int)shipypos - obs->ao_ypos) < OBS_MINDIST_Y)
				continue;

			if(abs(station_xpos - obs->ao_xpos) < OBS_MINDIST_X && 
			    abs(station_ypos - obs->ao_ypos) < OBS_MINDIST_Y)
				continue;

			break;
		}	
	}

	memset(exhaust, 0, sizeof(position_t) * EXHAUST_MAXCNT);
	newexhaust_ttl = 0;

	memset(debris, 0, sizeof(position_t) * DEBRIS_MAXCNT);

	disp_set_mode(DISP_MODE_FPS, ABELT_FPS);

	while(1) {

		ship_angle = 360 - rotary_get_value(ROTARY_LEFT);
		if(ship_angle == 360)
			ship_angle = 0;

		sbuf = ship_buf[ship_angle / 18];
		if((rotary_get_button_state(ROTARY_RIGHT) == BUTTON_PRESSED || 
		    rotary_get_button_state(ROTARY_LEFT) == BUTTON_PRESSED) &&
		    fuel_level > 0 && !explode){
			rad = ship_angle * M_PI / 180;
			shipabsvel = cos(rad) * shipyvel;
			if(shipabsvel < SHIP_MAXABSVEL) {
				shipxvel -= sin(rad) * SHIP_VELINC;
				shipyvel -= cos(rad) * SHIP_VELINC;
			}

			if(newexhaust_ttl == 0) {
				/* Look for free exhaust slot */
				for(i = 0, exh = exhaust; i < EXHAUST_MAXCNT;
				    ++i, ++exh) {
					if(!exh->ap_ttl)
						break;
				}
				if(i < EXHAUST_MAXCNT) {
					exh->ap_xvel = shipxvel +
					    sin(rad) * EXHAUST_VELFACTOR;
					exh->ap_yvel = shipyvel +
					    cos(rad) * EXHAUST_VELFACTOR;
					exh->ap_ttl = EXHAUST_MAXAGE;

					exh->ap_xpos = shipxpos +
					    sin(rad) * SHIP_WIDTH / 2;
					exh->ap_ypos = shipypos +
					    cos(rad) * SHIP_HEIGHT / 2;

				}
				newexhaust_ttl = EXHAUST_FRAMEINTERVAL;
			}

			--fuel_level;
		}

		if(!gamewon) {
			shipxpos += shipxvel;
			shipypos += shipyvel;
		}

#if 0
		if(shipxpos < 0)
			shipxpos = FRAME_WIDTH - 1 - SHIP_HALFWIDTH;
		else
		if(shipxpos > FRAME_WIDTH - 1 + SHIP_HALFWIDTH)
			shipxpos = SHIP_HALFWIDTH;

		if(shipypos < 0)
			shipypos = FRAME_HEIGHT - 1 - SHIP_HALFHEIGHT;
		else
		if(shipypos > FRAME_HEIGHT - 1 + SHIP_HALFHEIGHT)
			shipypos = SHIP_HALFHEIGHT;
#endif

		for(i = 0, obs = obstacle; i < OBS_CNT; ++i, ++obs) {

			/* Bail if no chance of obstacle being shown */
			if(obs->ao_xpos < (int)shipxpos - FRAME_WIDTH)
				continue;
			if(obs->ao_xpos > (int)shipxpos + FRAME_WIDTH)
				continue;
			if(obs->ao_ypos < (int)shipypos - FRAME_HEIGHT)
				continue;
			if(obs->ao_ypos > (int)shipypos + FRAME_HEIGHT)
				continue;

			/* It's OK if obstacle still not visible or will
			 * be partially cut off. blt() will do the right
			 * thing */

			disp_blt(curframe, obs->ao_type->at_buf,
			    obs->ao_type->at_bufsiz, obs->ao_type->at_width,
			    obs->ao_xpos - (int)shipxpos + SHIP_DISP_XPOS,
			    obs->ao_ypos - (int)shipypos + SHIP_DISP_YPOS);
		}

		/* Check if we are making contact with anything */
		if(!explode && disp_contactblt(curframe, sbuf, 8, 8,
		    SHIP_DISP_XPOS, SHIP_DISP_YPOS)) {
			/* Explode. */

			for(i = 0, deb = debris; i < DEBRIS_MAXCNT;
			    ++i, ++deb) {
				deb->ap_xpos = shipxpos + SHIP_WIDTH / 2 -
					DEBRIS_WIDTH / 2;
				deb->ap_ypos = shipypos + SHIP_HEIGHT / 2 -
					DEBRIS_HEIGHT / 2;


				deb->ap_xvel = shipxvel +
				    (float)(rand() %
				    (int)(EXPLODE_DEBRISVELRANDABS * 10)) / 10 *
				    ((rand() % 2) ? 1 : -1);
				deb->ap_yvel = shipyvel +
				    (float)(rand() %
				    (int)(EXPLODE_DEBRISVELRANDABS * 10)) / 10 *
				    ((rand() % 2) ? 1 : -1);

				deb->ap_buf = debris_buf[
				    rand() % DEBRIS_SPRITECNT];

printf("Setup debris %d: xvel=%f yel=%f\n", i, deb->ap_xvel, deb->ap_yvel);

			}

			++explode;
			explode_sprite_ttl = EXPLODE_SPRITECHFRCNT;
			end_ttl = END_FRAMECNT;
		}

		if(!explode && !gamewon) {
			disp_blt(curframe, sbuf, 8, 8,
			    SHIP_DISP_XPOS, SHIP_DISP_YPOS);
		} else {
			--end_ttl;
			if(!end_ttl)
				goto end_label;
		}

		if(!gamewon &&
		    (fabs((station_xpos + STATION_WIDTH / 2) -
		    (shipxpos + SHIP_WIDTH / 2)) < SHIP_WINDIST) &&
		    (fabs((station_ypos + STATION_HEIGHT / 2) -
		    (shipypos + SHIP_HEIGHT / 2)) < SHIP_WINDIST)) {
			++gamewon;
			end_ttl = END_FRAMECNT;
		}


		if(explode) {	
			for(i = 0, deb = debris; i < DEBRIS_MAXCNT;
			    ++i, ++deb) {

				deb->ap_xpos += deb->ap_xvel;
				deb->ap_ypos += deb->ap_yvel;

				disp_blt(curframe, deb->ap_buf, DEBRIS_BUFSIZ,
			            DEBRIS_WIDTH,
				    deb->ap_xpos - (int)shipxpos +
				    SHIP_DISP_XPOS,
				    deb->ap_ypos - (int)shipypos +
				    SHIP_DISP_YPOS);
			}

			--explode_sprite_ttl;
			if(!explode_sprite_ttl) {
				/* Change debris sprites */
				
				for(i = 0, deb = debris; i < DEBRIS_MAXCNT;
				    ++i, ++deb) {
					deb->ap_buf = debris_buf[
					    rand() % DEBRIS_SPRITECNT];
				}
				explode_sprite_ttl = EXPLODE_SPRITECHFRCNT;
			}
		
			disp_drawbox(curframe,
			    (FRAME_WIDTH - MSG_GAMELOST_WIDTH) / 2 - 2, 0,
			    (FRAME_WIDTH + MSG_GAMELOST_WIDTH) / 2 + 2, 12,
			    DISP_DRAW_OFF);
			disp_puttext(curframe, MSG_GAMELOST, &font_c64,
			(FRAME_WIDTH - MSG_GAMELOST_WIDTH) / 2, 2);
			
		} else
		if(gamewon) {
			disp_drawbox(curframe,
			    (FRAME_WIDTH - MSG_GAMEWON_WIDTH) / 2 - 2, 0,
			    (FRAME_WIDTH + MSG_GAMEWON_WIDTH) / 2 + 2, 12,
			    DISP_DRAW_OFF);
			disp_puttext(curframe, MSG_GAMEWON, &font_c64,
			(FRAME_WIDTH - MSG_GAMEWON_WIDTH) / 2, 2);
		}



		
		if(station_xpos > (int)shipxpos - FRAME_WIDTH &&
		    station_xpos < (int)shipxpos + FRAME_WIDTH &&
		    station_ypos > (int)shipypos - FRAME_HEIGHT &&
		    station_ypos < (int)shipypos + FRAME_HEIGHT) {
	
			/* Draw station */

			disp_blt(curframe, stbuf, STATION_BUFSIZ,
			    STATION_WIDTH, 
			    station_xpos - (int)shipxpos + SHIP_DISP_XPOS,
			    station_ypos - (int)shipypos + SHIP_DISP_YPOS);
		}

		++station_flipcnt;
		if(station_flipcnt >= STATION_FLIP_FRAMECNT) {
			if(stbuf == station_buf[0])
				stbuf = station_buf[1];
			else
				stbuf = station_buf[0];

			station_flipcnt = 0;
		}
		
		for(i = 0, exh = exhaust; i < EXHAUST_MAXCNT; ++i, ++exh) {
			if(!exh->ap_ttl)
				continue;

			exh->ap_xpos += exh->ap_xvel;
			exh->ap_ypos += exh->ap_yvel;

			disp_blt(curframe, exhaust_buf[i%2], EXHAUST_BUFSIZ,
			    EXHAUST_WIDTH,
			    (int)exh->ap_xpos - (int)shipxpos + SHIP_DISP_XPOS +
			    + SHIP_WIDTH / 2 - EXHAUST_WIDTH / 2,
                            (int)exh->ap_ypos - (int)shipypos + SHIP_DISP_YPOS +
			    + SHIP_HEIGHT / 2 - EXHAUST_HEIGHT / 2);

			--exh->ap_ttl;
		}

		if(newexhaust_ttl)
			--newexhaust_ttl;

		/* Draw fuel gauge */
		fuel_perc = fuel_level * 100 / FUEL_MAXLEVEL;
		disp_drawbox(curframe, fuel_gauge_xpos, fuel_gauge_ypos,
		    fuel_gauge_xpos + FUEL_GAUGE_WIDTH, FRAME_HEIGHT - 1,
		    DISP_DRAW_ON);
		disp_drawbox(curframe, fuel_gauge_xpos + 1, fuel_gauge_ypos + 1,
		    fuel_gauge_xpos + FUEL_GAUGE_WIDTH - 1, FRAME_HEIGHT - 2,
		    DISP_DRAW_OFF);
		snprintf(fuel_gaugestr, FUEL_GAUGESTRMAX, "FUEL: %d%%",
		    fuel_perc);
		disp_puttext(curframe, fuel_gaugestr, &font_picopixel,
		    fuel_gauge_xpos + 2, fuel_gauge_ypos + 2);
#if 0
		disp_drawbox(curframe, fuel_gauge_xpos + 1, fuel_gauge_ypos + 1,
		    fuel_gauge_xpos + 1 +
		    (fuel_perc * (FUEL_GAUGE_WIDTH - 2) / 100),
		    FRAME_HEIGHT - 2, DISP_DRAW_INVERT);
#endif


		station_xdiff = station_xpos + STATION_WIDTH / 2 -
                    ((int)shipxpos + SHIP_WIDTH / 2);
		station_ydiff = station_ypos + STATION_HEIGHT / 2 -
                    ((int)shipypos + SHIP_HEIGHT / 2);

		/* Quick and dirty way to avoid division by zero etc */
		if(abs(station_ydiff) == 0)
			station_ydiff = 1;
		if(abs(station_xdiff) == 0)
			station_xdiff = 1;
			
		radar_rad = atan(abs(station_xdiff) / abs(station_ydiff));
		radar_angle = (int)(radar_rad / M_PI * 180);
		radar_bufidx = (radar_angle + 22) / 45;
		if(station_xdiff >= 0) {
			if(station_ydiff >= 0)
				radar_bufidx = 4 + radar_bufidx;
			else
				radar_bufidx = (8 - radar_bufidx) % 8;
		} else {
			if(station_ydiff >= 0)
				radar_bufidx = 4 - radar_bufidx;
			else
				radar_bufidx = radar_bufidx;
		}

		disp_drawbox(curframe, radar_xpos, radar_ypos,
		    radar_xpos + RADAR_WIDTH - 1, FRAME_HEIGHT - 1,
		    DISP_DRAW_OFF);
		

		disp_blt(curframe, radar_buf[radar_bufidx],
		    RADAR_BUFSIZ, RADAR_WIDTH, radar_xpos, radar_ypos);


		pressedcnt = 0;
		if(rotary_get_button_state(ROTARY_LEFT) == BUTTON_PRESSED) {
			++pressedcnt;
		}

		if(rotary_get_button_state(ROTARY_RIGHT) == BUTTON_PRESSED) {
			++pressedcnt;
		}

		if(pressedcnt > maxpressed)
			maxpressed = pressedcnt;

		/* Both buttons were pressed and released */
		if(pressedcnt == 0 && maxpressed == ROTARY_CNT)
			goto end_label;

		disp_sleep_sendswapcurframe();

	}


end_label:

	if(obstacle) {
		free(obstacle);
		obstacle = NULL;
	}

}


void
mod_asteroidbelt_start(void)
{
	local_context_t savedlc;

	ESP_LOGI(ltag, "Asteroid Belt starting");	

	save_lcontext(&savedlc);

	abelt_newgame();

	restore_lcontext(&savedlc);

	ESP_LOGI(ltag, "Asteroid Belt exiting");	
}


static uint8_t	ship_buf[20][8] = {
	/* "Ship_0" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0xbc, 0xbc, 0x18, 0x00, 0x00},

	/* "Ship_1" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0xbc, 0x98, 0x00, 0x00},

	/* "Ship_2" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0x3c, 0x98, 0x40, 0x00},

	/* "Ship_3" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x40, 0x20},

	/* "Ship_4" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x30},

	/* "Ship_5" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x18},

	/* "Ship_6" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x0c},

	/* "Ship_7" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x02, 0x04},

	/* "Ship_8" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0x3c, 0x19, 0x02, 0x00},

	/* "Ship_9" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3c, 0x3d, 0x19, 0x00, 0x00},

	/* "Ship_10" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x18, 0x3d, 0x3d, 0x18, 0x00, 0x00},

	/* "Ship_11" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x19, 0x3d, 0x3c, 0x18, 0x00, 0x00},

	/* "Ship_12" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x02, 0x19, 0x3c, 0x3c, 0x18, 0x00, 0x00},

	/* "Ship_13" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x04, 0x02, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x00},

	/* "Ship_14" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x0c, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x00},

	/* "Ship_15" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x18, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x00},

	/* "Ship_16" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x30, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x00},

	/* "Ship_17" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x20, 0x40, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x00},

	/* "Ship_18" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x40, 0x98, 0x3c, 0x3c, 0x18, 0x00, 0x00},

	/* "Ship_19" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x98, 0xbc, 0x3c, 0x18, 0x00, 0x00}
};


static uint8_t	obs_00_buf[32] = {
    /* "Space_Obstacle_01" (16x16): vertical mapping, 256 pixels, 32 bytes */
	   0xec, 0xf4, 0x9e, 0xcf, 0x8b, 0xf9, 0xfb, 0x7f,
	   0xfe, 0xfc, 0xec, 0xa8, 0xfc, 0xbc, 0xf0, 0xc0,
	   0x03, 0x07, 0x1d, 0x3f, 0x3c, 0x63, 0x77, 0xf2,
	   0xfe, 0xde, 0xfe, 0x95, 0xf8, 0x39, 0x3f, 0x03 };

static uint8_t	obs_01_buf[128] = {
    /* "Space_Obstacle_02" (32x32): vertical mapping, 1024 pixels, 128 bytes */
	   0x00, 0x80, 0x60, 0xf0, 0x98, 0xfc, 0x7c, 0x7c,
	   0x9a, 0xce, 0x7e, 0xb1, 0xf3, 0xfc, 0xfc, 0xbe,
	   0xae, 0xbe, 0x3e, 0xfb, 0xf3, 0xdb, 0x1e, 0x1c,
	   0xec, 0x70, 0xf0, 0x38, 0xdc, 0xec, 0xd8, 0xe0,
	   0x0f, 0x1f, 0xb8, 0x60, 0xe0, 0xc3, 0x8f, 0xde,
	   0xff, 0x3f, 0xff, 0x9f, 0x9f, 0x9f, 0xf3, 0xf7,
	   0x9b, 0x0f, 0xff, 0xff, 0xee, 0xfd, 0xff, 0xef,
	   0xef, 0xfc, 0x7f, 0xd9, 0x9d, 0x87, 0x02, 0x00,
	   0x00, 0x12, 0x3f, 0x6d, 0xe3, 0xfb, 0xef, 0x43,
	   0x7f, 0x3f, 0x3f, 0xee, 0xff, 0xf7, 0x9b, 0xdb,
	   0x56, 0x3f, 0xf7, 0xbf, 0x7f, 0x97, 0xee, 0xdf,
	   0xfb, 0xff, 0xff, 0xf3, 0xff, 0xdf, 0x96, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x04,
	   0x07, 0x06, 0x0f, 0x0b, 0x1f, 0x1f, 0x3e, 0x37,
	   0x17, 0x1f, 0x1f, 0x5f, 0xfe, 0xff, 0xcb, 0x5a,
	   0x3f, 0x1e, 0x3f, 0x2e, 0x27, 0x3b, 0x1f, 0x0c };

static uint8_t	obs_02_buf[128] = {
    /* "Space_Obstacle_02_fliph" (32x32): vertical mapping,
     * 1024 pixels, 128 bytes */
	   0xe0, 0xd8, 0xec, 0xdc, 0x38, 0xf0, 0x70, 0xec,
	   0x1c, 0x1e, 0xdb, 0xf3, 0xfb, 0x3e, 0xbe, 0xae,
	   0xbe, 0xfc, 0xfc, 0xf3, 0xb1, 0x7e, 0xce, 0x9a,
	   0x7c, 0x7c, 0xfc, 0x98, 0xf0, 0x60, 0x80, 0x00,
	   0x00, 0x02, 0x87, 0x9d, 0xd9, 0x7f, 0xfc, 0xef,
	   0xef, 0xff, 0xfd, 0xee, 0xff, 0xff, 0x0f, 0x9b,
	   0xf7, 0xf3, 0x9f, 0x9f, 0x9f, 0xff, 0x3f, 0xff,
	   0xde, 0x8f, 0xc3, 0xe0, 0x60, 0xb8, 0x1f, 0x0f,
	   0x00, 0x96, 0xdf, 0xff, 0xf3, 0xff, 0xff, 0xfb,
	   0xdf, 0xee, 0x97, 0x7f, 0xbf, 0xf7, 0x3f, 0x56,
	   0xdb, 0x9b, 0xf7, 0xff, 0xee, 0x3f, 0x3f, 0x7f,
	   0x43, 0xef, 0xfb, 0xe3, 0x6d, 0x3f, 0x12, 0x00,
	   0x0c, 0x1f, 0x3b, 0x27, 0x2e, 0x3f, 0x1e, 0x3f,
	   0x5a, 0xcb, 0xff, 0xfe, 0x5f, 0x1f, 0x1f, 0x17,
	   0x37, 0x3e, 0x1f, 0x1f, 0x0b, 0x0f, 0x06, 0x07,
	   0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 };

static uint8_t	obs_03_buf[96] = {
    /* "Space_Obstacle_03" (32x24): vertical mapping, 768 pixels, 96 bytes */
	   0x80, 0x60, 0xa0, 0xf0, 0x68, 0xd4, 0xbc, 0xcc,
	   0xd4, 0xbe, 0x8a, 0x8f, 0x9b, 0x7f, 0x97, 0x72,
	   0x77, 0xb9, 0x3f, 0xee, 0xaa, 0xca, 0xf6, 0x2c,
	   0xf8, 0xcc, 0xd4, 0xf6, 0x6c, 0xf8, 0xf0, 0x10,
	   0x00, 0x03, 0x02, 0x13, 0x3f, 0x3d, 0x77, 0xb8,
	   0x30, 0xf9, 0xff, 0x37, 0xdb, 0xa4, 0x6e, 0xcb,
	   0xf8, 0xfd, 0xd7, 0xff, 0x8c, 0x9f, 0xff, 0xd7,
	   0xbf, 0x76, 0x7f, 0x4d, 0xff, 0x7f, 0xee, 0x04,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x01, 0x01, 0x01, 0x02, 0x0f, 0x1d, 0x37, 0x3f,
	   0x38, 0xd7, 0xf0, 0xd9, 0xfe, 0xaf, 0xd5, 0x7e,
	   0x3f, 0x1a, 0x0e, 0x09, 0x07, 0x02, 0x01, 0x00 };


static uint8_t	obs_04_buf[96] = {
	/* "Space_Obstacle_02_fliph" (32x24): vertical mapping,
	 * 768 pixels, 96 bytes */
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x80, 0x80, 0x80, 0x40, 0xf0, 0xb8, 0xec, 0xfc,
	   0x1c, 0xeb, 0x0f, 0x9b, 0x7f, 0xf5, 0xab, 0x7e,
	   0xfc, 0x58, 0x70, 0x90, 0xe0, 0x40, 0x80, 0x00,
	   0x00, 0xc0, 0x40, 0xc8, 0xfc, 0xbc, 0xee, 0x1d,
	   0x0c, 0x9f, 0xff, 0xec, 0xdb, 0x25, 0x76, 0xd3,
	   0x1f, 0xbf, 0xeb, 0xff, 0x31, 0xf9, 0xff, 0xeb,
	   0xfd, 0x6e, 0xfe, 0xb2, 0xff, 0xfe, 0x77, 0x20,
	   0x01, 0x06, 0x05, 0x0f, 0x16, 0x2b, 0x3d, 0x33,
	   0x2b, 0x7d, 0x51, 0xf1, 0xd9, 0xfe, 0xe9, 0x4e,
	   0xee, 0x9d, 0xfc, 0x77, 0x55, 0x53, 0x6f, 0x34,
	   0x1f, 0x33, 0x2b, 0x6f, 0x36, 0x1f, 0x0f, 0x08 };


static uint8_t	obs_05_buf[72] = {
    /* "Space_Obstacle_04" (24x24): vertical mapping, 576 pixels, 72 bytes */
	   0x00, 0xc0, 0xfc, 0x34, 0xbc, 0xef, 0xf7, 0x8f,
	   0xf6, 0xde, 0xea, 0xe2, 0xfe, 0xff, 0xfb, 0x9a,
	   0x76, 0x7e, 0x3c, 0xba, 0x70, 0xfc, 0xd8, 0x00,
	   0x7e, 0x7f, 0xe3, 0x7b, 0x9f, 0xa7, 0xfe, 0xbe,
	   0xff, 0xff, 0xc7, 0xdb, 0xdd, 0xfd, 0x7a, 0x5b,
	   0x1c, 0x9e, 0xbd, 0xfc, 0xf0, 0x9f, 0x4f, 0x7e,
	   0x00, 0x06, 0x25, 0x7d, 0x3b, 0x37, 0x71, 0x6f,
	   0x5f, 0xfd, 0xee, 0xe7, 0xfd, 0xd7, 0xd9, 0x79,
	   0x7e, 0x1f, 0x3b, 0x3f, 0x12, 0x1f, 0x0e, 0x04 };


static uint8_t	station_buf[2][96] = {

	/* "Space Station 01" (32x24): vertical mapping, 768 pixels, 96 bytes */
	{  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0xe0, 0x30, 0xf8, 0x18, 0x0c,
	   0x0c, 0x18, 0xf8, 0x30, 0xe0, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x1f, 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11,
	   0x11, 0x1f, 0x04, 0xff, 0x93, 0xbb, 0xfe, 0xbe,
	   0xfa, 0xfe, 0xbb, 0x93, 0xff, 0x04, 0x1f, 0x11,
	   0x11, 0x11, 0x1f, 0x11, 0x11, 0x11, 0x1f, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x2b, 0x36,
	   0x36, 0x2b, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

	/* "Space Station 02" (32x24): vertical mapping, 768 pixels, 96 bytes */
	{  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0xe0, 0x30, 0xf8, 0x18, 0x0c,
	   0x0c, 0x18, 0xf8, 0x30, 0xe0, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x1f, 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11,
	   0x11, 0x1f, 0x04, 0xff, 0x93, 0xbb, 0xfe, 0xfa,
	   0xbe, 0xfe, 0xbb, 0x93, 0xff, 0x04, 0x1f, 0x11,
	   0x11, 0x11, 0x1f, 0x11, 0x11, 0x11, 0x1f, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x2b, 0x36,
	   0x36, 0x2b, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};


static uint8_t radar_buf[8][22] = {

	/* "Radar_0_deg" (11x16): vertical mapping, 176 pixels, 22 bytes */
	{  0xff, 0x01, 0x01, 0x01, 0x05, 0x13, 0x05, 0x01,
	   0x01, 0x01, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01,
	   0x01, 0x01, 0x01, 0x01, 0x01, 0x01},

	/* "Radar_45_deg" (11x16): vertical mapping, 176 pixels, 22 bytes */
	{  0xff, 0x11, 0x29, 0x01, 0x05, 0x13, 0x05, 0x01,
	   0x01, 0x01, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01,
	   0x01, 0x01, 0x01, 0x01, 0x01, 0x01},

	/* "Radar_160_deg" (11x16): vertical mapping, 176 pixels, 22 bytes */
	{  0xff, 0x11, 0x29, 0x01, 0x01, 0x11, 0x01, 0x01,
	   0x01, 0x01, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01,
	   0x01, 0x01, 0x01, 0x01, 0x01, 0x01},

	/* "Radar_135_deg" (11x16): vertical mapping, 176 pixels, 22 bytes */
	{  0xff, 0x11, 0x29, 0x01, 0x41, 0x91, 0x41, 0x01,
	   0x01, 0x01, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01,
	   0x01, 0x01, 0x01, 0x01, 0x01, 0x01},

	/* "Radar_1160_deg" (11x16): vertical mapping, 176 pixels, 22 bytes */
	{  0xff, 0x01, 0x01, 0x01, 0x41, 0x91, 0x41, 0x01,
	   0x01, 0x01, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01,
	   0x01, 0x01, 0x01, 0x01, 0x01, 0x01},

	/* "Radar_225_deg" (11x16): vertical mapping, 176 pixels, 22 bytes */
	{  0xff, 0x01, 0x01, 0x01, 0x41, 0x91, 0x41, 0x01,
	   0x29, 0x11, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01,
	   0x01, 0x01, 0x01, 0x01, 0x01, 0x01},

	/* "Radar_270_deg" (11x16): vertical mapping, 176 pixels, 22 bytes */
	{  0xff, 0x01, 0x01, 0x01, 0x01, 0x11, 0x01, 0x01,
	   0x29, 0x11, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01,
	   0x01, 0x01, 0x01, 0x01, 0x01, 0x01},

	/* "Radar_315_deg" (11x16): vertical mapping, 176 pixels, 22 bytes */
	{  0xff, 0x01, 0x01, 0x01, 0x05, 0x13, 0x05, 0x01,
	   0x29, 0x11, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01,
	   0x01, 0x01, 0x01, 0x01, 0x01, 0x01}
};


static uint8_t debris_buf[10][8] = {

	/* "Space_Debris_0" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x00, 0x04, 0x0c, 0x00, 0x00, 0x00},

	/* "Space_Debris_1" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x00, 0x08, 0x04, 0x04, 0x00, 0x00},

	/* "Space_Debris_2" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x00, 0x0c, 0x08, 0x00, 0x00, 0x00},

	/* "Space_Debris_3" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x00, 0x30, 0x18, 0x00, 0x00, 0x00},

	/* "Space_Debris_4" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00},

	/* "Space_Debris_5" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x08, 0x10, 0x10, 0x00, 0x00, 0x00},

	/* "Space_Debris_6" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x08, 0x08, 0x04, 0x00, 0x00, 0x00},

	/* "Space_Debris_7" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x08, 0x14, 0x00, 0x00, 0x00, 0x00},

	/* "Space_Debris_8" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x00, 0x10, 0x08, 0x04, 0x00, 0x00},

	/* "Space_Debris_9" (8x8): vertical mapping, 64 pixels, 8 bytes */
	{  0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x00, 0x00}
};

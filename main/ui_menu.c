#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "microarcade.h"
#include "esp_rotary.h"
#include "esp_log.h"
#include "ui_menu.h"
#include "disp.h"
#include "font_c64.h"
#include "local_context.h"

static const char *ltag = "ui_menu";


#define MENU_MAXLINEBUFSIZ         128

static void
drawmenu(uint8_t *frame, ui_menu_item_t *mitem, int mitem_cnt, int mselidx,
        int maxlinecnt, int maxlinelen, const font_t *font, uint16_t xpos,
        uint16_t ypos)
{
        int     	i;
        int     	mdispidx;
        char    	displin[MENU_MAXLINEBUFSIZ];
        int     	lmaxlen;

        mdispidx = ((int)(mselidx / maxlinecnt)) * maxlinecnt;
        for(i = 0; i < maxlinecnt; ++i) {
                if(i + mdispidx >= mitem_cnt)
                        break;

                lmaxlen = maxlinelen + 2; /* Account for selector ('*') and
					   * '\0' */
                if(lmaxlen > MENU_MAXLINEBUFSIZ)
                        lmaxlen = MENU_MAXLINEBUFSIZ;

                snprintf(displin, lmaxlen, "%c%s",
                    mdispidx + i == mselidx ? '*' : ' ',
		    mitem[mdispidx + i].mi_text);

                puttext(frame, displin, font, xpos,
                    ypos + i * font->fo_line_height);
        }
}



int
ui_showmenu(ui_menu_item_t *mitems, int maxlinecnt, int maxlinelen,
	ui_menu_visual_mode_t vmode)
{
	int		mitemscnt;
	ui_menu_item_t	*mitem;
	rotary_config_t	rconf[ROTARY_CNT];
	uint8_t		*tempframe;
	int		mselidx;
	rotary_event_t	rev;
	local_context_t	savedlc;
	int		retval;
	int		xpos;
	int		ypos;
	int		menuwidth;
	int		menuheight;
	uint8_t		*bgframe;

	/* Shows a menu and when one of the items are selected, executes the
	 * action associated with the item.
	 * Will do a nice left/right scrolling transition from the currently
	 * displayed frame into the menu and vice versa */

	if(mitems == 0)
		return -1;

	for(mitemscnt = 0, mitem = mitems; mitem->mi_text;
	    ++mitem, ++mitemscnt);

	if(mitemscnt == 0)
		return -1;

	tempframe = NULL;
	bgframe = NULL;
	retval = -1;

	save_lcontext(&savedlc);

	memset(&rconf, 0, sizeof(rotary_config_t) * ROTARY_CNT);
	rconf[ROTARY_LEFT].rc_style = ROT_STYLE_BOUND;
	rconf[ROTARY_LEFT].rc_min = 0;
	rconf[ROTARY_LEFT].rc_max = mitemscnt - 1;
	if(rotary_reconfig(rconf, ROTARY_CNT) != ESP_OK) {
		ESP_LOGE(ltag, "Could not reconfigure rotarys");
		return -1;
	}

	rotary_event_queue_reset();

	disp_set_mode(DISP_MODE_ADHOC, 0);
	mselidx = 0;

	xpos = ypos = 0;

	menuwidth = (maxlinelen + 1) * 8;
	menuheight = maxlinecnt * 8;

	if(vmode == MENU_CENTER_OVERLAY) {
		xpos = (FRAME_WIDTH - menuwidth) / 2;
		ypos = (FRAME_HEIGHT - menuheight) / 2;
		/* Save the current background */
		bgframe = getframebuf();
		memcpy(bgframe, lastframe, FRAMESIZ);
	} else
	if(vmode == MENU_FULL_SCROLL) {
		/* Draw menu into temp frame, for transition */
		tempframe = getframebuf();
		drawmenu(tempframe, mitems, mitemscnt, mselidx,
		    maxlinecnt, maxlinelen, &font_c64, 0, 0);
		transframe(tempframe, 1);
		releaseframebuf(tempframe);
		tempframe = NULL;
	}

	/* Enter main loop */
        while(1) {	

		if(vmode == MENU_CENTER_OVERLAY) {
			/* Fill with background */
			memcpy(curframe, bgframe, FRAMESIZ);

			/* Draw a frame */
			drawbox(curframe, xpos - 2, ypos - 2,
			    xpos + menuwidth + 2, ypos + menuheight + 2,
			    DISP_DRAW_ON);
			drawbox(curframe, xpos - 1, ypos - 1,
			    xpos + menuwidth + 1, ypos + menuheight + 1,
			    DISP_DRAW_OFF);
		}

		drawmenu(curframe, mitems, mitemscnt, mselidx,
		    maxlinecnt, maxlinelen, &font_c64, xpos, ypos);

		sendswapcurframe();

                if(xQueueReceive(rotary_event_queue, &rev, portMAX_DELAY)
                    != pdPASS)
                        continue;

                if(rev.re_idx != ROTARY_LEFT)
                        continue;

                switch(rev.re_type) {
                case ROT_EVENT_INCREMENT:
                case ROT_EVENT_DECREMENT:
                        if(rev.re_value >=0 && rev.re_value < mitemscnt)
                                mselidx = rev.re_value;

                        break;

                case ROT_EVENT_BUTTON_RELEASE:
			switch(mitems[mselidx].mi_action) {
			case MIT_CALLFUNC:
				if(mitems[mselidx].mi_callfunc == NULL)
					break;

				/* Save current displayed frame for
				 * transitioning back */
				tempframe = getframebuf();
                                memcpy(tempframe, lastframe, FRAMESIZ);

				/* Call the function */
				mitems[mselidx].mi_callfunc();

				/* Transition back */
                                transframe(tempframe, 0);
				releaseframebuf(tempframe);
				tempframe = NULL;
				
				break;

			case MIT_RETURN_VAL:
				retval = mitems[mselidx].mi_returnval;
				goto return_label;

			default:
				break;
                        }
                        break;

                default:
                        break;
                }
	}

return_label:

	if(bgframe != NULL) {
		releaseframebuf(bgframe);
		bgframe = NULL;
	}	

	restore_lcontext(&savedlc);

	return retval;
}

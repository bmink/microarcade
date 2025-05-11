#include "local_context.h"

void
save_lcontext(local_context_t *lc)
{
	int	i;

	rotary_getconfig(lc->lc_rconf, ROTARY_CNT);

	for(i = 0; i < ROTARY_CNT; ++i) {
		lc->lc_rval[i] = rotary_get_value(i);
	}

	disp_get_mode(&lc->lc_disp_mode, &lc->lc_disp_fps);
}


void
restore_lcontext(local_context_t *lc)
{
	int	i;


	rotary_reconfig(lc->lc_rconf, ROTARY_CNT);

	for(i = 0; i < ROTARY_CNT; ++i) {
		rotary_set_value(i, lc->lc_rval[i]);
	}

	/* Clear the rotary event queue. The previous user may not have been
	 * using the queue (but using real-time calls to get rotary state)
	 * so there may be junk in there */
	rotary_event_queue_reset();

	disp_set_mode(lc->lc_disp_mode, lc->lc_disp_fps);
}

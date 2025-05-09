#ifndef LOCAL_CONTEXT_H
#define LOCAL_CONTEXT_H

/* microarcade is a collection of menus and modules. Menus will call modules
 * (games) and modules themselves will display their own menus at times.
 * Whenever such a switch occurs, the callee will very likely change things
 * like display mode, display refresh rate (fps) and rotary configuration.
 * 
 * It such cases it is up to the callee to restore all settings to the way it
 * found them when it was called. The local_context struct and associated
 * functions are provided to help with easily backing up / restoring context.
 */

#include <inttypes.h>
#include "esp_rotary.h"
#include "microarcade.h"
#include "disp.h"

typedef struct local_context {
	rotary_config_t	lc_rconf[ROTARY_CNT];
	int32_t		lc_rval[ROTARY_CNT];	/* Rotary current values */

	disp_mode_t	lc_disp_mode;
	int		lc_disp_fps;
} local_context_t;

void save_lcontext(local_context_t *);
void restore_lcontext(local_context_t *);


#endif

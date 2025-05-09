#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdint.h>

typedef void (*mi_callfunc_t)(void);

typedef enum mi_action {
	MIT_NONE,
	MIT_CALLFUNC,
	MIT_SHOW_SUBMENU,
	MIT_RETURN_FROM_SUBMENU,
	MIT_RETURN_VAL
} mi_action_t;


typedef struct ui_menu_item {
	char			*mi_text;
	
	mi_action_t		mi_action;
	mi_callfunc_t		mi_callfunc;
	int			mi_returnval;	/* Has to be unique across all
						 * the entire menu */
	struct ui_menu_item	*mi_submenu;

} ui_menu_item_t;


typedef enum ui_menu_visual_mode {
	MENU_FULL_SCROLL,
	MENU_FULL_IMMEDIATE,
	MENU_CENTER_OVERLAY,
} ui_menu_visual_mode_t;


int ui_showmenu(ui_menu_item_t *, int, int, ui_menu_visual_mode_t);


#endif

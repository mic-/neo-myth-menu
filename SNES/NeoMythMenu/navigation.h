#ifndef _NAVIGATION_H_
#define _NAVIGATION_H_

#include "snes.h"

#define NUMBER_OF_GAMES_TO_SHOW 9

// Menu IDs
#define MID_MAIN_MENU 0			// Main menu (games list)
#define MID_EXT_RUN_MENU 1		// Extended run menu (ROM type selection etc)
#define MID_GG_ENTRY_MENU 2		// Game Genie code entry menu
#define MID_GG_EDIT_MENU 3		// Same screen as GG_ENTRY_MENU, but now the user is editing a code


extern u8 currentMenu;

extern u16 read_joypad();

extern void navigation_init();

extern void move_to_next_game();
extern void move_to_next_page();
extern void move_to_previous_game();
extern void move_to_previous_page();

extern int can_games_list_scroll(scrollDirection_t);

extern void main_menu_process_keypress(u16);
extern void extended_run_menu_process_keypress(u16);
extern void gg_code_entry_menu_process_keypress(u16);
extern void gg_code_edit_menu_process_keypress(u16);

extern void switch_to_menu(u8, u8);

#endif

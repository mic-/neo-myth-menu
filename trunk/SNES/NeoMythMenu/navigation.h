#ifndef _NAVIGATION_H_
#define _NAVIGATION_H_

#include "snes.h"
#include "pff.h"

#define NUMBER_OF_GAMES_TO_SHOW 9

// Menu IDs
enum
{
	MID_MAIN_MENU = 0,			// Main menu (games list).
	MID_EXT_RUN_MENU,			// Extended run menu (ROM type selection etc).
	MID_GG_ENTRY_MENU,			// Game Genie code entry menu.
	MID_GG_EDIT_MENU,			// Same screen as GG_ENTRY_MENU, but now the user is editing a code.
	MID_AR_ENTRY_MENU,			// Action Replay code entry menu.
	MID_AR_EDIT_MENU,			// Same screen as AR_ENTRY_MENU, but now the user is editing a code.
	MID_CHEAT_DB_MENU,
	MID_CHEAT_DB_NO_CODES_MENU,	// The menu shown when the user presses Select and no cheats are found in the database for
								// the highlighted game.
	MID_ROM_INFO_MENU,

	MID_SD_ERROR_MENU,
	MID_SD_INFO_MENU,

	MID_VGM_PLAY_MENU,

	MID_LAST_MENU
};


extern u8 currentMenu;
extern u8 cheatGameIdx;
extern u8 gameFoundInDb;
extern u8 highlightedOption[MID_LAST_MENU];
extern DIR sdDir;
extern FILINFO sdFileInfo;
extern int lastSdError, lastSdOperation;

extern u16 read_joypad();

extern void navigation_init();

extern void move_to_next_game();
extern void move_to_next_page();
extern void move_to_previous_game();
extern void move_to_previous_page();

extern int can_games_list_scroll(scrollDirection_t);
extern int can_cheat_list_scroll(scrollDirection_t);

extern void switch_to_menu(u8, u8);

#endif

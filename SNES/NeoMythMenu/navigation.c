// Menu navigation code for the SNES Myth
// Mic, 2010

#include "snes.h"
#include "neo2.h"
#include "hw_math.h"
#include "navigation.h"
#include "common.h"
#include "action_replay.h"
#include "game_genie.h"
#include "ppu.h"
#include "string.h"
#include "cheats/cheat.h"


// Define the top-left corner for some of the text labels and the marker sprite
#define MARKER_LEFT 12
#define MARKER_TOP 67
#define CODE_LEFT 2


u8 currentMenu = MID_MAIN_MENU;
u8 highlightedOption[MID_LAST_MENU];
u8 cheatGameIdx = 0;
u8 gameFoundInDb = 0;

extern ggCode_t ggCodes[MAX_GG_CODES * 2];
extern const cheatDbEntry_t cheatDatabase[];

oamEntry_t marker;

typedef struct
{
	char *label;
	char *optionValue;
	u8 row;
	u8 optionColumn;
} menuOption_t;

menuOption_t extRunMenuOptions[5] =
{
	{"Game Genie", 0, 9, 0},
	{"Action Replay", 0, 10, 0},
	{"Mode:", 0, 12, 8},
	{"Autofix region:", 0, 13, 18},
	{0,0,0,0}	// Terminator
};


// Prototypes
void main_menu_process_keypress(u16);
void extended_run_menu_process_keypress(u16);
void gg_code_entry_menu_process_keypress(u16);
void gg_code_edit_menu_process_keypress(u16);
void ar_code_entry_menu_process_keypress(u16);
void ar_code_edit_menu_process_keypress(u16);
void cheat_db_menu_process_keypress(u16);
void cheat_db_no_codes_menu_process_keypress(u16);



// Return an unsigned short that contains the status of all 8 buttons and the dpad
// Note that this function is blocking.
//
u16 read_joypad()
{
	int i;
	u8 b, lo, hi;
	u16 retVal = 0;

	for (;;)
	{
		// Delay
		for (i = 0; i < 32; i++);

		REG_NMI_TIMEN = 1;

		while (!((b = REG_HVB_JOY) & 1));
		while ((b = REG_HVB_JOY) & 1);
		lo = REG_JOY1L;
		hi = REG_JOY1H;

		retVal &= 0xff00;
		retVal |= lo;
		if (!lo)
		{
			retVal &= 0x00ff;
			retVal |= hi << 8;
			if (!hi)
			{
				continue;
			}
		}

		b = 1;
		while (b)
		{
			REG_NMI_TIMEN = 1;
			while (!((b = REG_HVB_JOY) & 1));
			while ((b = REG_HVB_JOY) & 1);
			b = REG_JOY1L | REG_JOY1H;
		}

		break;
	}

	return retVal;
}


// Update the "GAME (nnn)" string.
// E.g. update_game_number_string(-2) to decrease nnn by 2.
//
void update_game_number_string(int offset)
{
	if (offset > 0)
	{
		for (; offset > 0; offset--)
		{
			MS4[0xd]++;
			if (MS4[0xd] > '9')
			{
				MS4[0xd] = '0';
				MS4[0xc]++;
				if (MS4[0xc] > '9')
				{
					MS4[0xc] = '0';
					MS4[0xb]++;
				}
			}
		}
	}
	else if (offset < 0)
	{
		offset = -offset;
		for (; offset > 0; offset--)
		{
			MS4[0xd]--;
			if (MS4[0xd] < '0')
			{
				MS4[0xd] = '9';
				MS4[0xc]--;
				if (MS4[0xc] < '0')
				{
					MS4[0xc] = '9';
					MS4[0xb]--;
				}
			}
		}
	}
}


// Return the number of games stored on the GBA card
//
u16 count_games_on_gba_card()
{
	u16 cnt;
	u8 *pGames;

	set_full_pointer((void**)&pGames, GAME_LIST_BANK, 0xc800);

	// Loop until a chunk is found that doesn't begin with the value 0xff
	for (cnt = 0; *pGames == 0xff; pGames += 0x40)
	{
		if (++cnt == 500)
		{
			break;
		}
	}

	return cnt;
}



void swap(u16 *a, u16 *b)
{
  	int t=*a; *a=*b; *b=t;
}

// Quicksort implementation taken from Wikipedia, with some modifications
//
void quick_sort(u16 *arr, int beg, int end, int (*cmp)(u16,u16))
{
	int piv, l, r;

	if (end > beg + 1)
  	{
    	piv = arr[beg], l = beg + 1, r = end;
    	while (l < r)
    	{
			if (cmp(arr[l], piv) <= 0)
			{
				l++;
			}
			else
			{
				swap(&arr[l], &arr[--r]);
			}
		}
    	swap(&arr[--l], &arr[beg]);
    	quick_sort(arr, beg, l, cmp);
    	quick_sort(arr, r, end, cmp);
  	}
}

// Game title string comparator used when sorting
//
int cmp_game_titles(u16 a, u16 b)
{
	u8 *pGameA, *pGameB;

	set_full_pointer((void**)&pGameA, GAME_LIST_BANK, 0xc800 + (a << 6));
	set_full_pointer((void**)&pGameB, GAME_LIST_BANK, 0xc800 + (b << 6));

	return strcmp(&pGameA[0xc], &pGameB[0xc]);
}


// Assumes that count_games_on_gba_card() has already been called
void create_alphabetical_index()
{
	int i;
	char c;
	u8 *pGames;

	set_full_pointer((void**)&pGames, GAME_LIST_BANK, 0xc800);

	for (i = 0; i < gamesList.count; i++)
	{
		gbaCardAlphabeticalIdx[i] = i;
	}

	quick_sort(gbaCardAlphabeticalIdx, 0, gamesList.count, cmp_game_titles);
}


void navigation_init()
{
	int i;

	for (i = 0; i < 16; i++) highlightedOption[i] = 0;

	gamesList.highlighted = gamesList.firstShown = 0;
	gamesList.count = count_games_on_gba_card();
	create_alphabetical_index();

	marker.chr = 200;
	marker.x = MARKER_LEFT;
	marker.y = MARKER_TOP;
	marker.palette = SHELL_OBJPAL_DARK_OLIVE;
	marker.prio = 3;
	marker.flip = 0;
}


// Returns non-zero if the games list can scroll in the given direction
//
int can_games_list_scroll(scrollDirection_t direction)
{
	if ((direction == DIRECTION_DOWN) &&
		(gamesList.firstShown + NUMBER_OF_GAMES_TO_SHOW < gamesList.count))
	{
			return 1;
	}
	else if ((direction == DIRECTION_UP) &&
			 (gamesList.firstShown))
	{
		return 1;
	}

	return 0;
}


// Move to the next entry in the games list
//
void move_to_next_game()
{
	if (++gamesList.highlighted >= gamesList.count)
	{
		gamesList.highlighted--;
	}
	else
	{
		update_game_number_string(1);

		// Check if the games list needs to be scrolled
		if ((gamesList.firstShown + ((NUMBER_OF_GAMES_TO_SHOW / 2) - 1) < gamesList.highlighted) &&
			(gamesList.firstShown + NUMBER_OF_GAMES_TO_SHOW < gamesList.count))
		{
			gamesList.firstShown++;
		}
	}

	update_game_params();
	print_games_list();
}


// Move to the previous entry in the games list
//
void move_to_previous_game()
{
	if (gamesList.highlighted)
	{
		gamesList.highlighted--;

		update_game_number_string(-1);

		// Check if the games list needs to be scrolled
		if ((gamesList.firstShown) &&
			(gamesList.firstShown + ((NUMBER_OF_GAMES_TO_SHOW / 2) - 1) >= gamesList.highlighted))
		{
			gamesList.firstShown--;
		}
	}

	update_game_params();
	print_games_list();
}


// Move to the next page in the games list (NUMBER_OF_GAMES_TO_SHOW entries down)
//
void move_to_next_page()
{
	if (gamesList.highlighted + NUMBER_OF_GAMES_TO_SHOW < gamesList.count)
	{
		gamesList.highlighted += NUMBER_OF_GAMES_TO_SHOW;
		update_game_number_string(NUMBER_OF_GAMES_TO_SHOW);

		// firstShown = highlighted - (highlighted % NUMBER_OF_GAMES_TO_SHOW)
		gamesList.firstShown = gamesList.highlighted - hw_div16_8_rem16(gamesList.highlighted, NUMBER_OF_GAMES_TO_SHOW);

		// Make sure firstShown is in range
		for (; gamesList.firstShown + NUMBER_OF_GAMES_TO_SHOW > gamesList.count; gamesList.firstShown--);
		for (; gamesList.firstShown > gamesList.highlighted; gamesList.firstShown--);
	}

	update_game_params();
	print_games_list();
}


// Move to the previous page in the games list (NUMBER_OF_GAMES_TO_SHOW entries up)
//
void move_to_previous_page()
{
	if (gamesList.highlighted >= NUMBER_OF_GAMES_TO_SHOW)
	{
		gamesList.highlighted -= NUMBER_OF_GAMES_TO_SHOW;
		update_game_number_string(-NUMBER_OF_GAMES_TO_SHOW);

		// firstShown = highlighted - (highlighted % NUMBER_OF_GAMES_TO_SHOW)
		gamesList.firstShown = gamesList.highlighted - hw_div16_8_rem16(gamesList.highlighted, NUMBER_OF_GAMES_TO_SHOW);

		// Make sure firstShown is in range
		for (; gamesList.firstShown + NUMBER_OF_GAMES_TO_SHOW > gamesList.count; gamesList.firstShown--);
		for (; gamesList.firstShown > gamesList.highlighted; gamesList.firstShown--);
	}

	update_game_params();
	print_games_list();

}



// Returns non-zero if the cheat list can scroll in the given direction
//
int can_cheat_list_scroll(scrollDirection_t direction)
{
	cheat_t const *cheats = cheatDatabase[cheatGameIdx].cheats;

	if ((direction == DIRECTION_DOWN) &&
		(gamesList.firstShown + NUMBER_OF_GAMES_TO_SHOW < gamesList.count))
	{
			return 1;
	}
	else if ((direction == DIRECTION_UP) &&
			 (highlightedOption[MID_CHEAT_DB_MENU]))
	{
		return 1;
	}

	return 0;
}


// Move to the next entry in the cheat list
//
void move_to_next_cheat()
{
	if (++cheatList.highlighted >= cheatList.count)
	{
		cheatList.highlighted--;
	}
	else
	{
		// Check if the games list needs to be scrolled
		if ((gamesList.firstShown + ((NUMBER_OF_GAMES_TO_SHOW / 2) - 1) < gamesList.highlighted) &&
			(gamesList.firstShown + NUMBER_OF_GAMES_TO_SHOW < gamesList.count))
		{
			gamesList.firstShown++;
		}
	}

	print_cheat_list();
}


// Move to the previous entry in the games list
//
void move_to_previous_cheat()
{
	if (cheatList.highlighted)
	{
		cheatList.highlighted--;

		// Check if the games list needs to be scrolled
		if ((gamesList.firstShown) &&
			(gamesList.firstShown + ((NUMBER_OF_GAMES_TO_SHOW / 2) - 1) >= gamesList.highlighted))
		{
			gamesList.firstShown--;
		}
	}

	print_cheat_list();
}



void switch_to_menu(u8 newMenu, u8 reusePrevScreen)
{
	int i;
	u8 y;
	cheat_t const *cheats;
	void (*get_info)(void);

	if (!reusePrevScreen)
	{
		if (newMenu != currentMenu)
		{
			mosaic_up();
			clear_screen();
		}

		print_hw_card_rev();
		print_meta_string(MS_VERSION_COPYRIGHT);
	}

	switch (newMenu)
	{
		case MID_EXT_RUN_MENU:
			keypress_handler = extended_run_menu_process_keypress;
			REG_BGCNT = 3;			// Enable BG0 and BG1 (disable OBJ)
			print_meta_string(74);	// Print instructions

			extRunMenuOptions[2].optionValue = &(metaStrings[48 + romRunMode][4]);
			extRunMenuOptions[3].optionValue = (doRegionPatch) ? "On " : "Off";

			for (i = 0; i < 16; i++)
			{
				if (extRunMenuOptions[i].label == 0) break;
				printxy(extRunMenuOptions[i].label,
				        2,
				        extRunMenuOptions[i].row,
				        (highlightedOption[MID_EXT_RUN_MENU]==i) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE) : TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
				        32);
				if (extRunMenuOptions[i].optionValue)
				{
					printxy(extRunMenuOptions[i].optionValue,
					        extRunMenuOptions[i].optionColumn,
					        extRunMenuOptions[i].row,
					        (highlightedOption[MID_EXT_RUN_MENU]==i) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE) : TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
					        32);
				}
			}
			highlightedOption[MID_GG_ENTRY_MENU] = 0;
			break;


		case MID_GG_ENTRY_MENU:
			keypress_handler = gg_code_entry_menu_process_keypress;
			REG_BGCNT = 0x13;		// Enable BG0, BG1 and OBJ
			print_meta_string(MS_GG_ENTRY_MENU_INSTRUCTIONS);
			printxy("0 1 2 3 4 5 6 7", CODE_LEFT, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
			printxy("8 9 A B C D E F", CODE_LEFT, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
			for (i = 0; i < MAX_GG_CODES; i++)
			{
				print_gg_code(&ggCodes[i],
				              CODE_LEFT,
				              14 + i,
				              (i==highlightedOption[MID_GG_ENTRY_MENU]) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE) : TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			}
			marker.x = MARKER_LEFT;
			marker.y = MARKER_TOP;
			marker.palette = SHELL_OBJPAL_DARK_OLIVE;
			break;


		case MID_GG_EDIT_MENU:
			keypress_handler = gg_code_edit_menu_process_keypress;
			clear_status_window();
			print_meta_string(MS_GG_EDIT_MENU_INSTRUCTIONS);
			marker.palette = SHELL_OBJPAL_WHITE;
			highlightedOption[MID_GG_EDIT_MENU] = 0;
			break;


		case MID_AR_ENTRY_MENU:
			keypress_handler = ar_code_entry_menu_process_keypress;
			REG_BGCNT = 0x13;		// Enable BG0, BG1 and OBJ
			print_meta_string(MS_GG_ENTRY_MENU_INSTRUCTIONS);
			printxy("0 1 2 3 4 5 6 7", CODE_LEFT, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
			printxy("8 9 A B C D E F", CODE_LEFT, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
			for (i = 0; i < MAX_GG_CODES; i++)
			{
				print_ar_code(&ggCodes[i+MAX_GG_CODES],
				              CODE_LEFT,
				              14 + i,
				              (i == highlightedOption[MID_AR_ENTRY_MENU]) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE): TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			}
			marker.x = MARKER_LEFT;
			marker.y = MARKER_TOP;
			marker.palette = SHELL_OBJPAL_DARK_OLIVE;
			break;


		case MID_AR_EDIT_MENU:
			keypress_handler = ar_code_edit_menu_process_keypress;
			clear_status_window();
			print_meta_string(MS_GG_EDIT_MENU_INSTRUCTIONS);
			marker.palette = SHELL_OBJPAL_WHITE;
			highlightedOption[MID_AR_EDIT_MENU] = 0;
			break;


		case MID_CHEAT_DB_MENU:
			keypress_handler = cheat_db_menu_process_keypress;
			print_meta_string(78);
			highlightedOption[MID_CHEAT_DB_MENU] = 0;

			// Get ROM info
			get_info = get_rom_info & 0x7fff;
			add_full_pointer((void**)&get_info, 0x7d, 0x8000);
			get_info();

			gameFoundInDb = 0;
			for (cheatGameIdx = 0; ; cheatGameIdx++)
			{
				if (cheatDatabase[cheatGameIdx].cheats)
				{
					if ((cheatDatabase[cheatGameIdx].romChecksum == (snesRomInfo[0x1e] + (snesRomInfo[0x1f] << 8))) &&
					    (cheatDatabase[cheatGameIdx].romChecksumCompl == (snesRomInfo[0x1c] + (snesRomInfo[0x1d] << 8))))
					{
						gameFoundInDb = 1;
						break;
					}
				}
				else
				{
					break;
				}
			}

			if (gameFoundInDb)
			{
				y = 10;
				cheats = cheatDatabase[cheatGameIdx].cheats;
				for (i = 0; i < cheatDatabase[cheatGameIdx].numCheats; i++)
				{
					printxy(cheats[i].description,
							2,
							y,
							(i == highlightedOption[MID_CHEAT_DB_MENU]) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE):TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
							128);
					y += hw_div16_8_quot16(strlen(cheats[i].description), 27) + 1;
					if (y > 17) break;
				}

				printxy(snesRomInfo, 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);	// ROM title
				printxy("Remaining code slots: 16", 3, 23, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
				break;
			}
			else
			{
				newMenu = MID_CHEAT_DB_NO_CODES_MENU;
				// Fall through..
			}
		case MID_CHEAT_DB_NO_CODES_MENU:
			keypress_handler = cheat_db_no_codes_menu_process_keypress;
			printxy(snesRomInfo, 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);	// ROM title
			printxy("No cheats found in the database for this ROM. Pick one of the menues below to manually enter a cheat code, or press Y to go back to the main menu.",
			        2,
			        10,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        200);
			break;


		default:					// Main menu
			keypress_handler = main_menu_process_keypress;
			print_meta_string(MS_MAIN_MENU_INSTRUCTIONS);
			print_meta_string(MS_GAME_NUMBER);
			print_games_list();
			update_game_params();
			break;
	}

	if (!reusePrevScreen)
	{
		if (newMenu != currentMenu)
		{
			update_screen();
			mosaic_down();
		}
	}

	currentMenu = newMenu;
	show_scroll_indicators();
}



void main_menu_process_keypress(u16 keys)
{
	if (keys & JOY_B)
	{
		// B
		run_game_from_gba_card_c();
	}
	else if (keys & JOY_Y)
	{
		// Y
		run_secondary_cart_c();
	}
	if (keys & JOY_X)
	{
		// X
		switch_to_menu(MID_EXT_RUN_MENU, 0);
	}
	else if (keys & JOY_SELECT)
	{
		// Select
		switch_to_menu(MID_CHEAT_DB_MENU, 0);

	}
	else if (keys & JOY_UP)
	{
		// Up
		move_to_previous_game();
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		move_to_next_game();
	}
	else if (keys & JOY_L)
	{
		// L
		sortOrder = (sortOrder == SORT_LOGICALLY) ? SORT_ALPHABETICALLY : SORT_LOGICALLY;
		print_games_list();
	}
	else if (keys & JOY_RIGHT)
	{
		// Right
		move_to_next_page();
	}
	else if (keys & JOY_LEFT)
	{
		// Left
		move_to_previous_page();
	}
}



void extended_run_menu_process_keypress(u16 keys)
{
	if (keys & JOY_B)
	{
		// B
		if (highlightedOption[MID_EXT_RUN_MENU] == 2)
		{
			// Switch HIROM/LOROM
			romRunMode ^= 1;
			extRunMenuOptions[2].optionValue = &(metaStrings[48 + romRunMode][4]);
			printxy(extRunMenuOptions[2].optionValue,
			        extRunMenuOptions[2].optionColumn,
			        extRunMenuOptions[2].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == 3)
		{
			doRegionPatch ^= 1;
			extRunMenuOptions[3].optionValue = (doRegionPatch) ? "On " : "Off";
			printxy(extRunMenuOptions[3].optionValue,
			        extRunMenuOptions[3].optionColumn,
			        extRunMenuOptions[3].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == 1)
		{
			// Go to the action replay screen
			switch_to_menu(MID_AR_ENTRY_MENU, 0);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == 0)
		{
			// Go to the game genie screen
			switch_to_menu(MID_GG_ENTRY_MENU, 0);
		}
	}
	else if (keys & JOY_START)
	{
		// B
		run_game_from_gba_card_c();
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_EXT_RUN_MENU])
		{
			// Un-highlight the previously highlighted string(s), and highlight the new one(s)
			printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].label,
			        2,
			        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
			        32);
			if (extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].optionValue)
			{
				printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].optionValue,
				        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].optionColumn,
				        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].row,
				        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
				        32);
			}
			printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]-1].label,
			        2,
			        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]-1].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);
			if (extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]-1].optionValue)
			{
				printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]-1].optionValue,
				        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]-1].optionColumn,
				        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]-1].row,
				        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
				        32);
			}
			highlightedOption[MID_EXT_RUN_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (highlightedOption[MID_EXT_RUN_MENU] < 3)
		{
			printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].label,
			        2,
			        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
			        32);
			if (extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].optionValue)
			{
				printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].optionValue,
				        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].optionColumn,
				        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]].row,
				        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
				        32);
			}
			printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]+1].label,
			        2,
			        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]+1].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);
			if (extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]+1].optionValue)
			{
				printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]+1].optionValue,
				        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]+1].optionColumn,
				        extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]+1].row,
				        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
				        32);
			}

			highlightedOption[MID_EXT_RUN_MENU]++;
		}
	}
	else if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU, 0);
	}
}



void gg_code_entry_menu_process_keypress(u16 keys)
{
	if (keys & JOY_B)
	{
		// A
		switch_to_menu(MID_GG_EDIT_MENU, 1);
	}
	else if (keys & JOY_START)
	{
		// B
		run_game_from_gba_card_c();
	}
	else if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_EXT_RUN_MENU, 0);
	}
	else if (keys & JOY_X)
	{
		// X
		ggCodes[highlightedOption[MID_GG_ENTRY_MENU]].used = CODE_UNUSED;
		print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]],
		              CODE_LEFT,
		              14 + highlightedOption[MID_GG_ENTRY_MENU],
		              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_GG_ENTRY_MENU])
		{
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]],
			              CODE_LEFT,
			              14 + highlightedOption[MID_GG_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]-1],
			              CODE_LEFT,
			              13 + highlightedOption[MID_GG_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			highlightedOption[MID_GG_ENTRY_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (highlightedOption[MID_GG_ENTRY_MENU] < MAX_GG_CODES - 1)
		{
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]],
			              CODE_LEFT,
			              14 + highlightedOption[MID_GG_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]+1],
			              CODE_LEFT,
			              15 + highlightedOption[MID_GG_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			highlightedOption[MID_GG_ENTRY_MENU]++;
		}
	}
}


void gg_code_edit_menu_process_keypress(u16 keys)
{
	u8 b;
	u16 whichCode;

	if (keys & JOY_A)
	{
		// A
		switch_to_menu(MID_GG_ENTRY_MENU, 1);
	}
	else if (keys & JOY_B)
	{
		// B
		whichCode = highlightedOption[MID_GG_ENTRY_MENU];
		b = ((marker.x - MARKER_LEFT) >> 4) + ((marker.y - MARKER_TOP) >> 1);
		ggCodes[whichCode].code[highlightedOption[MID_GG_EDIT_MENU]] = b;
		b += '0'; if (b > '9') b += 7;
		printxy(&b,
		        CODE_LEFT + highlightedOption[MID_GG_EDIT_MENU] + ((highlightedOption[MID_GG_EDIT_MENU]>3)?1:0),
		        14 + whichCode,
		        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
		        1);
		if (++highlightedOption[MID_GG_EDIT_MENU] == 8)
		{
			gg_decode(ggCodes[whichCode].code,
			          &(ggCodes[whichCode].bank),
			          &(ggCodes[whichCode].offset),
			          &(ggCodes[whichCode].val));
			ggCodes[whichCode].used = ((ggCodes[whichCode].bank == 0x7e) || (ggCodes[whichCode].bank == 0x7f)) ? CODE_TARGET_RAM : CODE_TARGET_ROM;
			switch_to_menu(MID_GG_ENTRY_MENU, 1);
		}
	}
	else if (keys & JOY_Y)
	{
		// Y
		if (highlightedOption[MID_GG_EDIT_MENU])
		{
			highlightedOption[MID_GG_EDIT_MENU]--;
			whichCode = highlightedOption[MID_GG_ENTRY_MENU];
			printxy("_",
			        CODE_LEFT + highlightedOption[MID_GG_EDIT_MENU] + ((highlightedOption[MID_GG_EDIT_MENU]>3)?1:0),
			        14 + whichCode,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        1);
			ggCodes[whichCode].used = CODE_UNUSED;
		}
	}
	else if (keys & JOY_UP)
	{
		// Up
		marker.y -= 16;
		if (marker.y < MARKER_TOP) marker.y = MARKER_TOP+16;
		update_screen();
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		marker.y += 16;
		if (marker.y > MARKER_TOP+16) marker.y = MARKER_TOP;
		update_screen();
	}
	else if (keys & JOY_LEFT)
	{
		// Left
		marker.x -= 16;
		if ((marker.x < MARKER_LEFT) || (marker.x > MARKER_LEFT + 112)) marker.x = MARKER_LEFT + 112;
		update_screen();
	}
	else if (keys & JOY_RIGHT)
	{
		// Left
		marker.x += 16;
		if (marker.x > MARKER_LEFT + 112) marker.x = MARKER_LEFT;
		update_screen();
	}
}



void ar_code_entry_menu_process_keypress(u16 keys)
{
	if (keys & JOY_B)
	{
		// A
		switch_to_menu(MID_AR_EDIT_MENU, 1);
	}
	else if (keys & JOY_START)
	{
		// B
		run_game_from_gba_card_c();
	}
	else if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_EXT_RUN_MENU, 0);
	}
	else if (keys & JOY_X)
	{
		// X
		ggCodes[MAX_GG_CODES+highlightedOption[MID_AR_ENTRY_MENU]].used = CODE_UNUSED;

		print_ar_code(&ggCodes[MAX_GG_CODES+highlightedOption[MID_AR_ENTRY_MENU]],
		              CODE_LEFT,
		              14 + highlightedOption[MID_AR_ENTRY_MENU],
		              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_AR_ENTRY_MENU])
		{
			print_ar_code(&ggCodes[MAX_GG_CODES + highlightedOption[MID_AR_ENTRY_MENU]],
			              CODE_LEFT,
			              14 + highlightedOption[MID_AR_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));

			print_ar_code(&ggCodes[MAX_GG_CODES + highlightedOption[MID_AR_ENTRY_MENU]-1],
			              CODE_LEFT,
			              13 + highlightedOption[MID_AR_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

			highlightedOption[MID_AR_ENTRY_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (highlightedOption[MID_AR_ENTRY_MENU] < MAX_GG_CODES - 1)
		{
			print_ar_code(&ggCodes[MAX_GG_CODES + highlightedOption[MID_AR_ENTRY_MENU]],
			              CODE_LEFT,
			              14 + highlightedOption[MID_AR_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));

			print_ar_code(&ggCodes[MAX_GG_CODES + highlightedOption[MID_AR_ENTRY_MENU]+1],
			              CODE_LEFT,
			              15 + highlightedOption[MID_AR_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

			highlightedOption[MID_AR_ENTRY_MENU]++;
		}
	}
}


void ar_code_edit_menu_process_keypress(u16 keys)
{
	u8 b;
	u16 whichCode;

	if (keys & JOY_A)
	{
		// A
		switch_to_menu(MID_AR_ENTRY_MENU, 1);
	}
	else if (keys & JOY_B)
	{
		// B
		whichCode = MAX_GG_CODES + highlightedOption[MID_AR_ENTRY_MENU];
		b = ((marker.x - MARKER_LEFT) >> 4) + ((marker.y - MARKER_TOP) >> 1);
		ggCodes[whichCode].code[highlightedOption[MID_AR_EDIT_MENU]] = b;
		b += '0'; if (b > '9') b += 7;
		printxy(&b,
		        CODE_LEFT + highlightedOption[MID_AR_EDIT_MENU],
		        14 + whichCode - MAX_GG_CODES,
		        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
		        1);
		if (++highlightedOption[MID_AR_EDIT_MENU] == 8)	// Has the user entered an entire 8-character code?
		{
			ar_decode(ggCodes[whichCode].code,
			          &(ggCodes[whichCode].bank),
			          &(ggCodes[whichCode].offset),
			          &(ggCodes[whichCode].val));
			ggCodes[whichCode].used = ((ggCodes[whichCode].bank == 0x7e) || (ggCodes[whichCode].bank == 0x7f)) ? CODE_TARGET_RAM : CODE_TARGET_ROM;
			switch_to_menu(MID_AR_ENTRY_MENU, 1);
		}
	}
	else if (keys & JOY_Y)
	{
		// Y
		if (highlightedOption[MID_AR_EDIT_MENU])
		{
			highlightedOption[MID_AR_EDIT_MENU]--;
			whichCode = highlightedOption[MID_AR_ENTRY_MENU];
			printxy("_",
			        CODE_LEFT + highlightedOption[MID_AR_EDIT_MENU],
			        14 + whichCode,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        1);
			ggCodes[MAX_GG_CODES + whichCode].used = CODE_UNUSED;
		}
	}
	else if (keys & JOY_UP)
	{
		// Up
		marker.y -= 16;
		if (marker.y < MARKER_TOP) marker.y = MARKER_TOP + 16;
		update_screen();
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		marker.y += 16;
		if (marker.y > MARKER_TOP + 16) marker.y = MARKER_TOP;
		update_screen();
	}
	else if (keys & JOY_LEFT)
	{
		// Left
		marker.x -= 16;
		if ((marker.x < MARKER_LEFT) || (marker.x > MARKER_LEFT + 112)) marker.x = MARKER_LEFT + 112;
		update_screen();
	}
	else if (keys & JOY_RIGHT)
	{
		// Left
		marker.x += 16;
		if (marker.x > MARKER_LEFT + 112) marker.x = MARKER_LEFT;
		update_screen();
	}
}


void cheat_db_menu_process_keypress(u16 keys)
{
	if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU, 0);
	}
}


void cheat_db_no_codes_menu_process_keypress(u16 keys)
{
	if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU, 0);
	}
}

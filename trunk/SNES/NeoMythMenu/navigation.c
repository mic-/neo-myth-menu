// Menu navigation code for the SNES Myth
// Mic, 2010

#include "snes.h"
#include "neo2.h"
#include "hw_math.h"
#include "navigation.h"
#include "common.h"
#include "game_genie.h"
#include "ppu.h"
#include "string.h"


u8 currentMenu = MID_MAIN_MENU;
u8 highlightedOption[16];

oamEntry_t marker;

char *extRunMenuOptions[] =
{
	// Name			Value
	"Game Genie",	0,
	"Mode:",		0,
	0
};


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
	marker.x = 44;
	marker.y = 67;
	marker.palette = 0;
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


void switch_to_menu(u8 newMenu, u8 reusePrevScreen)
{
	int i;

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
			extRunMenuOptions[3] = &(metaStrings[48 + romRunMode][4]);
			for (i = 0; i < 16; i+=2)
			{
				if (extRunMenuOptions[i] == 0) break;
				printxy(extRunMenuOptions[i], 2, 9+(i>>1), (highlightedOption[MID_EXT_RUN_MENU]==(i>>1))?8:24, 32);
				if (extRunMenuOptions[i+1]) printxy(extRunMenuOptions[i+1], 13, 9+(i>>1), (highlightedOption[MID_EXT_RUN_MENU]==(i>>1))?8:24, 32);
			}
			highlightedOption[MID_GG_ENTRY_MENU] = 0;

			////// DEBUG
			/*print_hex(ggCodes[0].val, 2, 13, 8);
			print_hex(ggCodes[0].bank, 5, 13, 8);
			print_hex((u8)(ggCodes[0].offset>>8), 8, 13, 8);
			print_hex((u8)(ggCodes[0].offset), 10, 13, 8);
			print_hex(ggCodes[0].used, 13, 13, 8);*/
			////////

			break;

		case MID_GG_ENTRY_MENU:
			keypress_handler = gg_code_entry_menu_process_keypress;
			REG_BGCNT = 0x13;		// Enable BG0, BG1 and OBJ
			print_meta_string(MS_GG_ENTRY_MENU_INSTRUCTIONS);
			printxy("0 1 2 3 4 5 6 7", 6, 9, 7*4, 32);
			printxy("8 9 A B C D E F", 6, 11, 7*4, 32);
			for (i = 0; i < MAX_GG_CODES; i++)
			{
				print_gg_code(&ggCodes[i], 6, 14+i, (i==highlightedOption[MID_GG_ENTRY_MENU])?8:24);
			}
			marker.x = 44;
			marker.y = 67;
			marker.palette = 0;
			break;

		case MID_GG_EDIT_MENU:
			keypress_handler = gg_code_edit_menu_process_keypress;
			clear_status_window();
			print_meta_string(MS_GG_EDIT_MENU_INSTRUCTIONS);
			marker.palette = 1;
			highlightedOption[MID_GG_EDIT_MENU] = 0;
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
		// A
		if (highlightedOption[MID_EXT_RUN_MENU] == 1)
		{
			// Switch HIROM/LOROM
			romRunMode ^= 1;
			extRunMenuOptions[3] = &(metaStrings[48 + romRunMode][4]);
			printxy(extRunMenuOptions[3], 13, 10, (highlightedOption[MID_EXT_RUN_MENU]==1)?8:24, 32);
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
			printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]<<1], 2, 9+highlightedOption[MID_EXT_RUN_MENU], 24, 32);
			if (extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)+1])
				printxy(extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)+1], 13, 9+highlightedOption[MID_EXT_RUN_MENU], 24, 32);

			printxy(extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)-2], 2, 8+highlightedOption[MID_EXT_RUN_MENU], 8, 32);
			if (extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)-1])
				printxy(extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)-1], 13, 8+highlightedOption[MID_EXT_RUN_MENU], 8, 32);

			highlightedOption[MID_EXT_RUN_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (highlightedOption[MID_EXT_RUN_MENU] < 2)
		{
			printxy(extRunMenuOptions[highlightedOption[MID_EXT_RUN_MENU]<<1], 2, 9+highlightedOption[MID_EXT_RUN_MENU], 24, 32);
			if (extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)+1])
				printxy(extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)+1], 13, 9+highlightedOption[MID_EXT_RUN_MENU], 24, 32);

			printxy(extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)+2], 2, 10+highlightedOption[MID_EXT_RUN_MENU], 8, 32);
			if (extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)+3])
				printxy(extRunMenuOptions[(highlightedOption[MID_EXT_RUN_MENU]<<1)+3], 13, 10+highlightedOption[MID_EXT_RUN_MENU], 8, 32);

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
		ggCodes[highlightedOption[MID_GG_ENTRY_MENU]].used = 0;
		print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]], 6, 14+highlightedOption[MID_GG_ENTRY_MENU], 8);
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_GG_ENTRY_MENU])
		{
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]], 6, 14+highlightedOption[MID_GG_ENTRY_MENU], 6*4);
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]-1], 6, 13+highlightedOption[MID_GG_ENTRY_MENU], 2*4);
			highlightedOption[MID_GG_ENTRY_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (highlightedOption[MID_GG_ENTRY_MENU] < MAX_GG_CODES - 1)
		{
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]], 6, 14+highlightedOption[MID_GG_ENTRY_MENU], 6*4);
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]+1], 6, 15+highlightedOption[MID_GG_ENTRY_MENU], 2*4);
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
		b = ((marker.x - 44) >> 4) + ((marker.y - 67) >> 1);
		ggCodes[whichCode].code[highlightedOption[MID_GG_EDIT_MENU]] = b;
		b += '0'; if (b > '9') b += 7;
		printxy(&b, 6+highlightedOption[MID_GG_EDIT_MENU]+((highlightedOption[MID_GG_EDIT_MENU]>3)?1:0), 14+whichCode, 8, 1);
		if (++highlightedOption[MID_GG_EDIT_MENU] == 8)
		{
			gg_decode(ggCodes[whichCode].code, &(ggCodes[whichCode].bank), &(ggCodes[whichCode].offset), &(ggCodes[whichCode].val));
			ggCodes[whichCode].used = 1;
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
			printxy("_", 6+highlightedOption[MID_GG_EDIT_MENU]+((highlightedOption[MID_GG_EDIT_MENU]>3)?1:0), 14+whichCode, 8, 1);
			ggCodes[whichCode].used = 0;
		}
	}
	else if (keys & JOY_UP)
	{
		// Up
		marker.y -= 16;
		if (marker.y < 67) marker.y = 83;
		update_screen();
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		marker.y += 16;
		if (marker.y > 83) marker.y = 67;
		update_screen();
	}
	else if (keys & JOY_LEFT)
	{
		// Left
		marker.x -= 16;
		if (marker.x < 44) marker.x = 44+112;
		update_screen();
	}
	else if (keys & JOY_RIGHT)
	{
		// Left
		marker.x += 16;
		if (marker.x > 44+112) marker.x = 44;
		update_screen();
	}
}


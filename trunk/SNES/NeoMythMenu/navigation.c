// Menu navigation code for the SNES Myth
// Mic, 2010

#include "snes.h"
#include "neo2.h"
#include "hw_math.h"
#include "navigation.h"
#include "common.h"
#include "game_genie.h"
#include "string.h"


u8 currentMenu;
u8 highlightedGgCode;


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


void switch_to_menu(u8 newMenu)
{
	switch (newMenu)
	{
		case MID_EXT_RUN_MENU:
			mosaic_up();
			clear_screen();
			print_hw_card_rev();
			print_meta_string(0);
			print_meta_string(74);
			printxy("MODE:", 2, 9, 2*4, 32);
			printxy(&(metaStrings[48 + romRunMode][4]), 8, 9, 2*4, 32);
			dma_bg0_buffer();
			mosaic_down();
			break;

		case MID_GG_ENTRY_MENU:
			mosaic_up();
			clear_screen();
			highlightedGgCode = 0;
			print_meta_string(0);
			print_meta_string(76);
			print_hw_card_rev();
			printxy("0 1 2 3 4 5 6 7", 6, 9, 7*4, 32);
			printxy("8 9 A B C D E F", 6, 11, 7*4, 32);
			print_gg_code(&ggCodes[0], 6, 14, 2*4);
			print_gg_code(&ggCodes[1], 6, 15, 6*4);
			print_gg_code(&ggCodes[2], 6, 16, 6*4);
			print_gg_code(&ggCodes[3], 6, 17, 6*4);
			dma_bg0_buffer();
			mosaic_down();
			break;

		default:
			mosaic_up();
			clear_screen();
			print_meta_string(0);
			print_meta_string(75);
			print_meta_string(4);
			print_hw_card_rev();
			print_games_list();
			update_game_params();
			dma_bg0_buffer();
			mosaic_down();
			break;
	}

	currentMenu = newMenu;
}


void main_menu_process_keypress(u16 keys)
{
	if (keys & 0x8000)
	{
		// B
		run_game_from_gba_card_c();
	}
	else if (keys & 0x4000)
	{
		// Y
		run_secondary_cart_c();
	}
	if (keys & 0x0040)
	{
		// X
		switch_to_menu(MID_EXT_RUN_MENU);
	}
	else if (keys & 0x2000)
	{
		// Select
	}
	else if (keys & 0x0800)
	{
		// Up
		move_to_previous_game();
	}
	else if (keys & 0x0400)
	{
		// Down
		move_to_next_game();
	}
	else if (keys & 0x0020)
	{
		// L
		sortOrder = (sortOrder == SORT_LOGICALLY) ? SORT_ALPHABETICALLY : SORT_LOGICALLY;
		print_games_list();
	}
	else if (keys & 0x0100)
	{
		// Right
		move_to_next_page();
	}
	else if (keys & 0x0200)
	{
		// Left
		move_to_previous_page();
	}
}


void extended_run_menu_process_keypress(u16 keys)
{
	if (keys & 0x0040)
	{
		// X
		switch_to_menu(MID_GG_ENTRY_MENU);
	}
	else if (keys & 0x4000)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU);
	}
}


void gg_code_entry_menu_process_keypress(u16 keys)
{
	if (keys & 0x4000)
	{
		// Y
		switch_to_menu(MID_EXT_RUN_MENU);
	}
	else if (keys & 0x0800)
	{
		// Up
		if (highlightedGgCode)
		{
			print_gg_code(&ggCodes[highlightedGgCode], 6, 14+highlightedGgCode, 6*4);
			print_gg_code(&ggCodes[highlightedGgCode-1], 6, 13+highlightedGgCode, 2*4);
			highlightedGgCode--;
		}
	}
	else if (keys & 0x0400)
	{
		// Down
		if (highlightedGgCode < MAX_GG_CODES - 1)
		{
			print_gg_code(&ggCodes[highlightedGgCode], 6, 14+highlightedGgCode, 6*4);
			print_gg_code(&ggCodes[highlightedGgCode+1], 6, 15+highlightedGgCode, 2*4);
			highlightedGgCode++;
		}
	}
}



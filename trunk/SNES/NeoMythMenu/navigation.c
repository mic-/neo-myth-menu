// Menu navigation code for the SNES Myth
// Mic, 2010-2013

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
#include "sd_utils.h"
#include "bg_buffer.h"
#include "diskio.h"

//#define CART_TESTS

// Define the top-left corner for some of the text labels and the marker sprite
#define MARKER_LEFT 12
#define MARKER_TOP 67
#define CODE_LEFT_MARGIN 2

u8 currentMenu = MID_MAIN_MENU;
u8 highlightedOption[MID_LAST_MENU];
u8 cheatGameIdx = 0;
u8 gameFoundInDb = 0;
u8 resetType = 0;
u8 dumpType = 0;
u8 dumping = 0;
extern void dump_to_sd();
char highlightedFileName[100];
long long highlightedFileSize;
u16 highlightedIsDir;
extern FATFS *FatFs;
extern DWORD compressedVgmSize;
extern u8 diskioCrcbuf[8];
extern ggCode_t ggCodes[MAX_GG_CODES * 2];
extern const cheatDbEntry_t cheatDatabase[];

//DEBUG
#ifdef EMULATOR
static char psramTestData[24] = {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0, 2,3,7,11,13,17,19,23};
#endif

const char * const sdFrStrings[] =
{
	"Ok",
	"Disk error",
	"Not ready",
	"No file",
	"No path",
	"Invalid name",
	"Stream error",
	"Invalid object",
	"Not enabled",
	"No filesystem"
};

const char * const sdOpErrorStrings[] =
{
	"Mount error",
	"Open error",
	"Read error",
	"Write error",
	"Seek error",
	"Opendir error",
	"Readdir error",
	"Unknown error"
};

const char * const countryCodeStrings[] =
{
	"(Japan)",
	"(USA/Canada)",
	"(Europe)",
	"(Scandinavia)",
	0,
	0,
	"(France)",
	"(Netherlands)",
	"(Spain)",
	"(Germany)",
	"(Italy)",
	"(China)",
	0,
	"(Korea)",
	0,
	"(Canada)",
	"(Brazil)",
	"(Australia)",
};

const char * const sdManufacturerStrings[] =
{
	"Unknown",
	"Panasonic",
	"Toshiba",
	"SanDisk",
};

const char * const romRamSizeStrings[] =
{
	"(None)    ", "(16 kbit) ", "(32 kbit) ",
	"(64 kbit) ", "(128 kbit)", "(256 kbit)",
	"(512 kbit)", "(1 Mbit)  ", "(2 Mbit)  ",
	"(4 Mbit)  ", "(8 Mbit)  ", "(16 Mbit) ",
	"(32 Mbit) ", "(64 Mbit) ",
};

const char * const dumpTypeStrings[] =
{
	"ROM     ", "SRAM    ", "Neo SRAM",
};

const char * const sramAddrStrings[] =
{
	"$700000",	// LoROM
	"$200000",	// HiROM
};

const char * const neoSramAddrStrings[] =
{
	"$000000",
	"$002000",
	"$004000",
	"$006000",
	"$008000",
	"$00A000",
	"$00C000",
	"$00E000",
	"$010000",
	"$012000",
	"$014000",
	"$016000",
	"$018000",
	"$01A000",
	"$01C000",
	"$01E000",
	"$020000",
	"$022000",
	"$024000",
	"$026000",
	"$028000",
	"$02A000",
	"$02C000",
	"$02E000",
	"$030000",
	"$032000",
	"$034000",
	"$036000",
	"$038000",
	"$03A000",
	"$03C000",
	"$03E000",
};
const char * const regionPatchStrings[] =
{
	"Off     ", "Quick   ", "Complete",
};

const char * const resetTypeStrings[] =
{
	"To game         ",
	"To menu after 3s",
	"To menu         "
	//"Disabled        "
};

const char * const sramBankOverrideStrings[] =
{
	"Auto",
	"0   ",
	"1   ",
	"2   ",
	"3   "
};

oamEntry_t marker;

typedef struct
{
	char *label;
	char *optionValue;
	u8 row;
	u8 optionColumn;
} menuOption_t;
////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
	MENU1_ITEM_GAME_GENIE = 0,
	MENU1_ITEM_ACTION_REPLAY = 1,
	MENU1_ITEM_ROM_INFO = 2,
    MENU1_ITEM_SD_INFO = 3,
    MENU1_ITEM_DUMP_TO_SD = 4,
	MENU1_ITEM_RUN_MODE = 5,
	MENU1_ITEM_FIX_REGION = 6,
	MENU1_ITEM_RESET_TYPE = 7,
#ifdef CART_TESTS
	MENU1_ITEM_TEST_CART = 8,
	MENU1_ITEM_SRAM_BANK = 9,
#else
	MENU1_ITEM_SRAM_BANK = 8,
#endif
	MENU1_ITEM_LAST
};

menuOption_t extRunMenuItems[MENU1_ITEM_LAST + 1] =
{
	{"Game Genie", 0, 9, 0},
	{"Action Replay", 0, 10, 0},
	{"ROM info", 0, 11, 0},
	{"SD info", 0, 12, 0},
	{"Dump to SD", 0, 13, 0},
	{"Mode:", 0, 14, 8},
	{"Autofix region:", 0, 15, 18},
	{"Reset:", 0, 16, 9},
#ifdef CART_TESTS
	{"Cart self-test", 0, 17, 0},
#else
	{"SRAM bank:", 0, 17, 13},
#endif
	{0,0,0,0}	// Terminator
};

////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
	MENU2_ITEM_TEST_MYTH_PSRAM = 0,
	MENU2_ITEM_TEST_GBAC_PSRAM = 1,
	MENU2_ITEM_TEST_SRAM = 2,
    MENU2_ITEM_WRITE_SRAM = 3,
	MENU2_ITEM_READ_SRAM = 4,
	MENU2_ITEM_LAST
};

menuOption_t cartTestMenuItems[MENU2_ITEM_LAST + 1] =
{
	{"Test Myth PSRAM", 0, 9, 0},
	{"Test GBAC PSRAM", 0, 10, 0},
	{"Test SRAM (full)", 0, 11, 0},
	{"Test SRAM (write)", 0, 12, 0},
	{"Test SRAM (read)", 0, 13, 0},
	{0,0,0,0}	// Terminator
};

////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
	MENU3_ITEM_DUMP_TYPE = 0,
	MENU3_ITEM_ROM_LAYOUT = 1,
	MENU3_ITEM_ROM_SIZE = 2,
	MENU3_ITEM_SRAM_ADDR = 3,
	MENU3_ITEM_SRAM_SIZE = 4,
	MENU3_ITEM_NEO_SRAM_ADDR = 5,
	MENU3_ITEM_NEO_SRAM_SIZE = 6,
	MENU3_ITEM_LAST
};
menuOption_t dumpMenuItems[MENU3_ITEM_LAST + 1] =
{
	{"Dump:", 0, 9, 8},
	{"ROM layout:", 0, 11, 14},
	{"ROM size:", 0, 12, 14},
	{"SRAM addr:", 0, 14, 14},
	{"SRAM size:", 0, 15, 14},
	{"Neo SRAM bank:", 0, 17, 17},
	{"Neo SRAM size:", 0, 18, 17},
	{0,0,0,0}	// Terminator
};
////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
	MENU7_ITEM_GAME_GENIE = 0,
	MENU7_ITEM_ACTION_REPLAY = 1,
	MENU7_ITEM_LAST
};
menuOption_t noCodesMenuItems[MENU7_ITEM_LAST + 1] =
{
	{"Game Genie", 0, 17, 0},
	{"Action Replay", 0, 18, 0},
	{0,0,0,0}	// Terminator
};
////////////////////////////////////////////////////////////////////////////////////////////////
// Prototypes
void main_menu_process_keypress(u16);
void extended_run_menu_process_keypress(u16);
void cart_test_menu_process_keypress(u16);
void gg_code_entry_menu_process_keypress(u16);
void gg_code_edit_menu_process_keypress(u16);
void ar_code_entry_menu_process_keypress(u16);
void ar_code_edit_menu_process_keypress(u16);
void cheat_db_menu_process_keypress(u16);
void cheat_db_no_codes_menu_process_keypress(u16);
void rom_info_menu_process_keypress(u16);
void dump_menu_process_keypress(u16);
void sd_error_menu_process_keypress(u16);
void vgm_play_menu_process_keypress(u16);
////////////////////////////////////////////////////////////////////////////////////////////////
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
		retVal = lo | (hi<<8);
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

char toupper(char c)
{
	if ((c >= 'a') && (c <= 'z'))
	{
		c -= ' ';
	}
	return c;
}
int strstri(char *lookFor, char *lookIn)
{
	int len1,len2,i,j;
	static int x = 2;
	static int y = 9;
	char c,d;
	len1 = strlen(lookFor);
	len2 = strlen(lookIn);
	if (len2 >= len1)
	{
		for (i = 0; i < len2; i++)
		{
			for (j = 0; j < len1; j++)
			{
				if (i+j >= len2)
				{
					break;
				} else //if ((char)toupper(lookIn[i+j]) != (char)toupper(lookFor[j]))
				{
					c = toupper(lookFor[j]);
					d = toupper(lookIn[j+i]);
					if (c != d)
					{
						break;
					}
				}
			}
			if (j == len1)
			{
				return i;
			}
		}
	}
	return -1;
}

int is_file_to_list(char *fname)
{
	if ((strstri(".SMC", fname) > 0) ||
	    (strstri(".BIN", fname) > 0) ||
	    (strstri(".SPC", fname) > 0) ||
	    (strstri(".VGM", fname) > 0))
	{
		return 1;
	}
	return 0;
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
	lastSdOperation = SD_OP_UNKNOWN;
	lastSdError = FR_OK;
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
			if (sourceMedium == SOURCE_SD)
			{
				//pf_readdir(&sdDir, &sdFileInfo);
			}
		}
	}
	update_game_params(0);
	print_games_list();
}


// Move to the previous entry in the games list
//
void move_to_previous_game()
{
	int i;
	if (gamesList.highlighted)
	{
		gamesList.highlighted--;
		update_game_number_string(-1);
		// Check if the games list needs to be scrolled
		if ((gamesList.firstShown) &&
			(gamesList.firstShown + ((NUMBER_OF_GAMES_TO_SHOW / 2) - 1) >= gamesList.highlighted))
		{
			gamesList.firstShown--;
			if (sourceMedium == SOURCE_SD)
			{
				/*pf_opendir(&sdDir, sdRootDir);
				for (i = 0; i < gamesList.firstShown; i++)
				{
					pf_readdir(&sdDir, &sdFileInfo);
				}*/
			}
		}
	}
	update_game_params(0);
	print_games_list();
}


// Move to the next page in the games list (NUMBER_OF_GAMES_TO_SHOW entries down)
//
void move_to_next_page()
{
	int i;
	if (gamesList.highlighted + NUMBER_OF_GAMES_TO_SHOW < gamesList.count)
	{
		gamesList.highlighted += NUMBER_OF_GAMES_TO_SHOW;
		update_game_number_string(NUMBER_OF_GAMES_TO_SHOW);
		i = gamesList.firstShown;
		gamesList.firstShown = gamesList.highlighted - hw_div16_8_rem16(gamesList.highlighted, NUMBER_OF_GAMES_TO_SHOW);

		// Make sure firstShown is in range
		for (; gamesList.firstShown + NUMBER_OF_GAMES_TO_SHOW > gamesList.count; gamesList.firstShown--);
		for (; gamesList.firstShown > gamesList.highlighted; gamesList.firstShown--);

		if (sourceMedium == SOURCE_SD)
		{
			/*i = gamesList.firstShown - i;
            while (i > 0)
			{
				pf_readdir(&sdDir, &sdFileInfo);
				i--;
			}*/
		}
	}
	update_game_params(0);
	print_games_list();
}


// Move to the previous page in the games list (NUMBER_OF_GAMES_TO_SHOW entries up)
//
void move_to_previous_page()
{
	int i;
	if (gamesList.highlighted >= NUMBER_OF_GAMES_TO_SHOW)
	{
		gamesList.highlighted -= NUMBER_OF_GAMES_TO_SHOW;
		update_game_number_string(-NUMBER_OF_GAMES_TO_SHOW);
		gamesList.firstShown = gamesList.highlighted - hw_div16_8_rem16(gamesList.highlighted, NUMBER_OF_GAMES_TO_SHOW);

		// Make sure firstShown is in range
		for (; gamesList.firstShown + NUMBER_OF_GAMES_TO_SHOW > gamesList.count; gamesList.firstShown--);
		for (; gamesList.firstShown > gamesList.highlighted; gamesList.firstShown--);

		if (sourceMedium == SOURCE_SD)
		{
			/*pf_opendir(&sdDir, sdRootDir);
			for (i = 0; i < gamesList.firstShown; i++)
			{
				pf_readdir(&sdDir, &sdFileInfo);
			}*/
		}
	}
	update_game_params(0);
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
	u16 y1, y2, i, d;
	cheat_t const *cheats;
	cheats = cheatDatabase[cheatGameIdx].cheats;
	if (++cheatList.highlighted >= cheatList.count)
	{
		cheatList.highlighted--;
	}
	else
	{
		// Check if the games list needs to be scrolled
		if (cheatList.firstShown < (cheatList.count - 1))
		{
			y1 = y2 = 10;
			for (i = cheatList.firstShown; i < cheatList.count; i++)
			{
				d = hw_div16_8_quot16(strlen(cheats[i].description), 27) + 1;
				if (i < cheatList.highlighted)
				{
					y1 += d;
				}
				y2 += d;
				if (y2 > 17) break;
			}
			if ((cheatList.highlighted > ((cheatList.firstShown + i) >> 1)) && (i < (cheatList.count - 1))) cheatList.firstShown++;
		}
		hide_cheat_list();
	}
	print_cheat_list();
}

// Move to the previous entry in the games list
//
void move_to_previous_cheat()
{
	u16 y, i;
	cheat_t const *cheats;
	cheats = cheatDatabase[cheatGameIdx].cheats;
	if (cheatList.highlighted)
	{
		cheatList.highlighted--;
		// Check if the games list needs to be scrolled
		if (cheatList.firstShown)
		{
			y = 10;
			for (i = cheatList.firstShown; i < cheatList.count; i++)
			{
				y += hw_div16_8_quot16(strlen(cheats[i].description), 27) + 1;
				if (i == cheatList.highlighted) break;
			}
			if (y < 14) cheatList.firstShown--;
		}
		hide_cheat_list();
	}
	print_cheat_list();
}

void print_menu_item(menuOption_t *items, u16 idx, u16 attribs)
{
	printxy(items[idx].label,
	        2,
	        items[idx].row,
			attribs,
			32);
	if (items[idx].optionValue)
	{
		printxy(items[idx].optionValue,
		        items[idx].optionColumn,
		        items[idx].row,
		        attribs,
		        32);
	}
}


void run_highlighted_game()
{
	if (sourceMedium == SOURCE_GBAC)
	{
		run_game_from_gba_card_c();
	}
	else if (sourceMedium == SOURCE_SD)
	{
		run_game_from_sd_card_c(HIGHLIGHTED_GAME);
	}
}

void run_last_played_game()
{
	if (sourceMedium == SOURCE_SD)
	{
		run_game_from_sd_card_c(LAST_PLAYED_GAME);
	}
}


extern DWORD pffbcs;
extern WORD pffclst;
extern unsigned char pfmountbuf[36];

void switch_to_menu(u8 newMenu, u8 reusePrevScreen)
{
	int i;
	DWORD ofs;
	u8 y;
	u8 *p;
	WORD bytesRead;
	static char strBuf[128];
	cheat_t const *cheats;
	romLayout_t romLayout;
	void (*get_info)(char *, u16, u16, u16);
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
			extRunMenuItems[MENU1_ITEM_RUN_MODE].optionValue = (char*)&(metaStrings[48 + romRunMode][4]);
			extRunMenuItems[MENU1_ITEM_FIX_REGION].optionValue = (char*)regionPatchStrings[doRegionPatch];
			extRunMenuItems[MENU1_ITEM_RESET_TYPE].optionValue = (char*)resetTypeStrings[resetType];
			extRunMenuItems[MENU1_ITEM_SRAM_BANK].optionValue = (char*)sramBankOverrideStrings[sramBankOverride+1];
			for (i = 0; i < MENU1_ITEM_LAST; i++)
			{
				print_menu_item(extRunMenuItems,
				                i,
				                (highlightedOption[MID_EXT_RUN_MENU]==i) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE) : TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			}
			highlightedOption[MID_GG_ENTRY_MENU] = 0;
			break;

		case MID_DUMP_MENU:
			keypress_handler = dump_menu_process_keypress;
			print_meta_string(MS_DUMP_MENU_INSTRUCTIONS);
			dumping = 0;
			MAKE_RAM_FPTR(get_info, neo2_myth_bootcart_rom_read);
			get_info(snesRomInfo, 0, 0xffc0, 0x40);
			romDumpLayout = snesRomInfo[0x15] & LAYOUT_HIROM;
			sramDumpAddr = romDumpLayout;
			romDumpSize = snesRomInfo[0x17];

			if (romDumpSize > 0x0C) romDumpSize = 0x0C;
			if (romDumpSize < 0x05) romDumpSize = 0x05;

			sramDumpSize = snesRomInfo[0x18];
			if (sramDumpSize > 0x0C) sramDumpSize = 0x0C;

			neoSramDumpAddr = 0;
			neoSramDumpSize = 0;

			dumpMenuItems[MENU3_ITEM_DUMP_TYPE].optionValue     = (char*)dumpTypeStrings[dumpType];
			dumpMenuItems[MENU3_ITEM_ROM_LAYOUT].optionValue    = (char*)&(metaStrings[48 + (romDumpLayout^1)][4]);
			dumpMenuItems[MENU3_ITEM_ROM_SIZE].optionValue      = (char*)romRamSizeStrings[romDumpSize];
			dumpMenuItems[MENU3_ITEM_SRAM_ADDR].optionValue     = (char*)sramAddrStrings[sramDumpAddr];
			dumpMenuItems[MENU3_ITEM_SRAM_SIZE].optionValue     = (char*)romRamSizeStrings[sramDumpSize];
			dumpMenuItems[MENU3_ITEM_NEO_SRAM_ADDR].optionValue = (char*)neoSramAddrStrings[neoSramDumpAddr];
			dumpMenuItems[MENU3_ITEM_NEO_SRAM_SIZE].optionValue = (char*)romRamSizeStrings[neoSramDumpSize];

			highlightedOption[MID_DUMP_MENU] = 0;

			for (i = 0; i < MENU3_ITEM_LAST; i++)
			{
				print_menu_item(dumpMenuItems,
				                i,
				                (highlightedOption[MID_DUMP_MENU]==i) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE) : TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			}
			break;

#ifdef CART_TESTS
		case MID_CART_TEST_MENU:
			keypress_handler = cart_test_menu_process_keypress;
			print_meta_string(MS_CART_TEST_MENU_INSTRUCTIONS);
			highlightedOption[MID_CART_TEST_MENU] = 0;
			for (i = 0; i < MENU2_ITEM_LAST; i++)
			{
				print_menu_item(cartTestMenuItems,
				                i,
				                (i==0) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE) : TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			}
			break;
#endif
		case MID_GG_ENTRY_MENU:
			keypress_handler = gg_code_entry_menu_process_keypress;
			REG_BGCNT = 0x13;		// Enable BG0, BG1 and OBJ
			print_meta_string(MS_GG_ENTRY_MENU_INSTRUCTIONS);
			printxy("0 1 2 3 4 5 6 7", CODE_LEFT_MARGIN, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
			printxy("8 9 A B C D E F", CODE_LEFT_MARGIN, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
			for (i = 0; i < MAX_GG_CODES; i++)
			{
				print_gg_code(&ggCodes[i],
				              CODE_LEFT_MARGIN,
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
			printxy("0 1 2 3 4 5 6 7", CODE_LEFT_MARGIN, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
			printxy("8 9 A B C D E F", CODE_LEFT_MARGIN, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
			for (i = 0; i < MAX_GG_CODES; i++)
			{
				print_ar_code(&ggCodes[i+MAX_GG_CODES],
				              CODE_LEFT_MARGIN,
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
			if (sourceMedium == SOURCE_GBAC)
			{
				// Get ROM info
				MAKE_RAM_FPTR(get_info, neo2_myth_current_rom_read);
				if (romRunMode)
				{
					// LoROM
					get_info(snesRomInfo, 0, 0x7fc0, 0x40);
				}
				else
				{
					// HiROM
					get_info(snesRomInfo, 0, 0xffc0, 0x40);
				}
			}
			else
			{
				strcpy(strBuf, sdRootDir);
				if (sdRootDirLength > 1)
				{
					strcpy(&strBuf[sdRootDirLength], "/");
					strcpy(&strBuf[sdRootDirLength+1], highlightedFileName);
					strBuf[sdRootDirLength+1+strlen(highlightedFileName)] = 0;
				}
				else
				{
					strcpy(&strBuf[sdRootDirLength], highlightedFileName);
					strBuf[sdRootDirLength+strlen(highlightedFileName)] = 0;
				}
				if ((romLayout = get_rom_info_sd(strBuf, snesRomInfo)) == LAYOUT_UNKNOWN)
				{
					switch_to_menu(MID_SD_ERROR_MENU, 1);
					break;
				}
			}
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
			// DEBUG
#ifdef EMULATOR
			gameFoundInDb = 1; cheatGameIdx = 5;
#endif
			cheatList.count = cheatDatabase[cheatGameIdx].numCheats;
			cheatList.firstShown = cheatList.highlighted = 0;
			if (gameFoundInDb)
			{
				//for (i = 0; i < 128; i++) cheatApplied[i] = 0;
				print_cheat_list();
				printxy(snesRomInfo, 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);	// ROM title
				printxy("Remaining code slots:   ", 3, 23, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
				print_dec(freeCodeSlots, 25, 23, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				break;
			}
			else
			{
				newMenu = MID_CHEAT_DB_NO_CODES_MENU;
                // Fall through..
			}
		case MID_CHEAT_DB_NO_CODES_MENU:
			keypress_handler = cheat_db_no_codes_menu_process_keypress;
			clear_status_window();
			print_meta_string(79);
			printxy(snesRomInfo, 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);	// ROM title
			printxy("No cheats found in the database for this ROM. Pick one of the menues below to manually enter a cheat code, or press Y to go back to the  main menu.",
			        2,
			        10,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE),
			        200);
			highlightedOption[MID_CHEAT_DB_NO_CODES_MENU] = MENU7_ITEM_GAME_GENIE;
			for (i = 0; i < 2; i++)
			{
				printxy(noCodesMenuItems[i].label,
				        2,
				        noCodesMenuItems[i].row,
				        (highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]==i) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE) : TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
				        32);
			}
			break;
		case MID_VGM_PLAY_MENU:
			keypress_handler = vgm_play_menu_process_keypress;
			print_meta_string(MS_VGM_PLAY_MENU_INSTRUCTIONS);
			printxy("Playing ", 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			printxy(highlightedFileName, 10, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			i = 10;
			if (isVgz)
			{
				printxy("Zipped size:", 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
				print_dec(vgzSize, 21, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
				i++;
			}
			printxy("Uncompressed size:", 2, i, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
			print_dec(highlightedFileSize, 21, i, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			printxy("Compressed size:", 2, i+1, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
			print_dec(compressedVgmSize, 21, i+1, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			break;

		case MID_SD_INFO_MENU:
			keypress_handler = sd_error_menu_process_keypress;
			print_meta_string(MS_SD_INFO_MENU_INSTRUCTIONS);
			if (sourceMedium == SOURCE_GBAC)
			{
				printxy("No SD card mounted", 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
				printxy("Press R if you want to", 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 24);
				printxy("mount the SD card now", 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 24);
				break;
			}
			printxy("SD card info", 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			printxy("Card type:", 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
            print_hex(cardType>>8, 13, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
            print_hex(cardType, 15, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			//printxy("Speed class:", 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
            //print_hex(sdSpeedClass, 15, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

			printxy("Manufacturer:", 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
            print_hex(sdManufId, 16, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			printxy("Manufactured: 2000/", 2, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			print_dec((sdManufDate >> 4)+2000, 16, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			print_dec(sdManufDate & 0x0F, 21, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

			printxy("Sectors:", 2, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
            print_hex(num_sectors>>24, 11, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
            print_hex(num_sectors>>16, 13, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
            print_hex(num_sectors>>8, 15, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
            print_hex(num_sectors, 17, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			printxy("File system:", 2, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			printxy("Cluster size:", 2, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			if (FatFs)
			{
				if (FatFs->fs_type == FS_FAT12)
					printxy("FAT12", 15, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
				else if (FatFs->fs_type == FS_FAT16)
					printxy("FAT16", 15, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
				else if (FatFs->fs_type == FS_FAT32)
					printxy("FAT32", 15, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
				else
					printxy("??? ", 15, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
	            print_hex(FatFs->csize, 16, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			}
			else
			{
				printxy("??? ", 15, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
				printxy("??? ", 16, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			}
			break;

		case MID_SD_ERROR_MENU:
			keypress_handler = sd_error_menu_process_keypress;
			print_meta_string(MS_SD_ERROR_MENU_INSTRUCTIONS);
#if 1
			printxy("SD card error", 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			printxy(sdOpErrorStrings[lastSdOperation], 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			printxy(sdFrStrings[lastSdError], 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			if (lastSdParam)
			{
				printxy(lastSdParam, 2, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
				lastSdParam = NULL;
			} else
			{
				printxy("CRC16:", 2, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
				print_hex(diskioCrcbuf[7], 9, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(diskioCrcbuf[6], 11, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(diskioCrcbuf[5], 13, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(diskioCrcbuf[4], 15, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(diskioCrcbuf[3], 17, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(diskioCrcbuf[2], 19, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(diskioCrcbuf[1], 21, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(diskioCrcbuf[0], 23, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			}
			printxy("Packet:", 2, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			for (i = 0; i < 7; i++)
			{
				print_hex(diskioPacket[i], 2+i+i, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			}
			printxy("Response:", 2, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			for (i = 0; i < 6; i++)
			{
				print_hex(diskioResp[i], 2+i+i, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			}
			printxy("Type:", 2, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			print_hex(pffclst>>8, 8, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			print_hex(pffclst, 10, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			printxy("Sectors:", 2, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			print_hex(num_sectors>>24, 11, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			print_hex(num_sectors>>16, 13, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			print_hex(num_sectors>>8, 15, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			print_hex(num_sectors, 17, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
#else
            // DEBUG
			printxy(sdOpErrorStrings[lastSdOperation], 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			printxy(sdFrStrings[lastSdError], 2, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 21);
			p = (u8*)&sdDir;
			for (i = 0; i < 32; i++)
			{
				print_hex(*p, 2 + ((i&7)<<1), 10 + (i>>3), TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
				p++;
			}
			for (i = 0; i < 8; i++)
			{
				print_hex(sdDir.fn[i], 2+i+i, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			}
#endif
			break;
		case MID_ROM_INFO_MENU:
			keypress_handler = rom_info_menu_process_keypress;
			print_meta_string(MS_ROM_INFO_MENU_INSTRUCTIONS);
			if (sourceMedium == SOURCE_GBAC)
			{
				// Get ROM info
				MAKE_RAM_FPTR(get_info, neo2_myth_current_rom_read);
				if (romRunMode)
				{
					// LoROM
					get_info(snesRomInfo, 0, 0x7fc0, 0x40);
				}
				else
				{
					// HiROM
					get_info(snesRomInfo, 0, 0xffc0, 0x40);
				}
			}
			else
			{
				strcpy(strBuf, sdRootDir);
				if (sdRootDirLength > 1)
				{
					strcpy(&strBuf[sdRootDirLength], "/");
					strcpy(&strBuf[sdRootDirLength+1], highlightedFileName);
					strBuf[sdRootDirLength+1+strlen(highlightedFileName)] = 0;
				}
				else
				{
					strcpy(&strBuf[sdRootDirLength], highlightedFileName);
					strBuf[sdRootDirLength+strlen(highlightedFileName)] = 0;
				}
				if ((romLayout = get_rom_info_sd(strBuf, snesRomInfo)) == LAYOUT_UNKNOWN)
				{
					switch_to_menu(MID_SD_ERROR_MENU, 1);
					break;
				}
			}
			printxy(snesRomInfo, 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);	// ROM title
			printxy("Layout:   $", 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x15], 13, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			printxy((snesRomInfo[0x15] & 1) ? "(HIROM)" : "(LOROM)", 16, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			printxy("Type:     $", 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x16], 13, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			if (snesRomInfo[0x16] == 0x02)
			{
				printxy("(SRAM)", 16, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			}
			else if (snesRomInfo[0x16] == 0x03)
			{
				printxy("(DSP)", 16, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			}
			else if (snesRomInfo[0x16] == 0x05)
			{
				printxy("(DSP+SRAM)", 16, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			}
			printxy("ROM size: $", 2, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x17], 13, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			if ((snesRomInfo[0x17] >= 0x9) && (snesRomInfo[0x17] <= 0xD))
			{
				printxy((char*)romRamSizeStrings[snesRomInfo[0x17]], 16, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			}
			printxy("RAM size: $", 2, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x18], 13, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
            if (snesRomInfo[0x18] <= 7)
			{
				printxy((char*)romRamSizeStrings[snesRomInfo[0x18]], 16, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			}
			printxy("Country:  $", 2, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x19], 13, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			if (snesRomInfo[0x19] < 0x12)
			{
				if (countryCodeStrings[snesRomInfo[0x19]])
				{
					printxy((char*)countryCodeStrings[snesRomInfo[0x19]], 16, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 25);
				}
			}
			printxy("Checksum: $", 2, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x1f], 13, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			print_hex(snesRomInfo[0x1e], 15, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			printxy(", $", 17, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x1d], 20, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			print_hex(snesRomInfo[0x1c], 22, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			printxy("RESET:    $", 2, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x3d], 13, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			print_hex(snesRomInfo[0x3c], 15, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			printxy("NMI:      $", 2, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			print_hex(snesRomInfo[0x2b], 13, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			print_hex(snesRomInfo[0x2a], 15, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			break;
		default:					// Main menu
			keypress_handler = main_menu_process_keypress;
			print_meta_string(MS_MAIN_MENU_INSTRUCTIONS);
			print_meta_string(MS_GAME_NUMBER);
			print_games_list();
			update_game_params(0);
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
	int i;
	void (*psram_read)(char*,u16,u16,u16);
	void (*psram_write)(char*,u16,u16,u16);
	if (keys & JOY_B)
	{
		// B
		run_highlighted_game();
	}
	else if (keys & JOY_Y)
	{
		// Y
		run_secondary_cart_c();
	}
	// DEBUG
#ifdef EMULATOR
	else if (keys & JOY_START)
	{
		psram_read = neo2_myth_psram_read & 0x7fff;
		psram_write = neo2_myth_psram_write & 0x7fff;
		add_full_pointer((void**)&psram_read, 0x7d, 0x8000);
		add_full_pointer((void**)&psram_write, 0x7d, 0x8000);
		psram_write(&psramTestData[16], 0x12, 0x0000, 8);
		psram_read(&psramTestData[2], 0x12, 0x0000, 6);
		for (i = 0; i < 10; i++)
		{
			print_hex(psramTestData[i], 2+i+i, 4, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
		}
	}
#else
	else if (keys & JOY_START)
	{
		run_last_played_game();
	}
#endif
	else if (keys & JOY_X)
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
		if (sourceMedium == SOURCE_GBAC)
		{
			sortOrder = (sortOrder == SORT_LOGICALLY) ? SORT_ALPHABETICALLY : SORT_LOGICALLY;
			print_games_list();
		}
		else if (sourceMedium == SOURCE_SD)
		{
			// Jump back to the top of the current directory
			lastSdOperation = SD_OP_OPEN_DIR;
			if ((lastSdError = pf_opendir(&sdDir, sdRootDir)) == FR_OK)
			{
				gamesList.firstShown = gamesList.highlighted = 0;
				MS4[0xd] = '1'; MS4[0xc] = MS4[0xb] = '0';	// Reset the "Game (001)" string
				print_games_list();
			}
			else
			{
				switch_to_menu(MID_SD_ERROR_MENU, 0);
			}
		}
	}
	else if (keys & JOY_R)
	{
		// R
		if (sourceMedium == SOURCE_GBAC)
		{
			set_source_medium(SOURCE_SD, 0);
		}
		else if (sourceMedium == SOURCE_SD)
		{
			set_source_medium(SOURCE_GBAC, 0);
		}
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
void print_option(menuOption_t *items, u8 idx)
{
	printxy(items[idx].optionValue,
            items[idx].optionColumn,
            items[idx].row,
            TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
            32);
}

void extended_run_menu_process_keypress(u16 keys)
{
	if (keys & JOY_B)
	{
		// B
		if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_RUN_MODE)
		{
			// Switch HIROM/LOROM
			romRunMode ^= 1;
			extRunMenuItems[MENU1_ITEM_RUN_MODE].optionValue = (char*)&(metaStrings[48 + romRunMode][4]);
			/*printxy(extRunMenuItems[MENU1_ITEM_RUN_MODE].optionValue,
			        extRunMenuItems[MENU1_ITEM_RUN_MODE].optionColumn,
			        extRunMenuItems[MENU1_ITEM_RUN_MODE].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);*/
			print_option(extRunMenuItems, MENU1_ITEM_RUN_MODE);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_FIX_REGION)
		{
			doRegionPatch++; if (doRegionPatch > 2) doRegionPatch = 0;
			extRunMenuItems[MENU1_ITEM_FIX_REGION].optionValue = (char*)regionPatchStrings[doRegionPatch];
			print_option(extRunMenuItems, MENU1_ITEM_FIX_REGION);
			/*printxy(extRunMenuItems[MENU1_ITEM_FIX_REGION].optionValue,
			        extRunMenuItems[MENU1_ITEM_FIX_REGION].optionColumn,
			        extRunMenuItems[MENU1_ITEM_FIX_REGION].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);*/
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_RESET_TYPE)
		{
			resetType++; if (resetType > 2) resetType = 0;
			extRunMenuItems[MENU1_ITEM_RESET_TYPE].optionValue = (char*)resetTypeStrings[resetType];
			printxy(extRunMenuItems[MENU1_ITEM_RESET_TYPE].optionValue,
			        extRunMenuItems[MENU1_ITEM_RESET_TYPE].optionColumn,
			        extRunMenuItems[MENU1_ITEM_RESET_TYPE].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_SRAM_BANK)
		{
			sramBankOverride++; if (sramBankOverride > 3) sramBankOverride = -1;
			extRunMenuItems[MENU1_ITEM_SRAM_BANK].optionValue = (char*)sramBankOverrideStrings[sramBankOverride+1];
			printxy(extRunMenuItems[MENU1_ITEM_SRAM_BANK].optionValue,
			        extRunMenuItems[MENU1_ITEM_SRAM_BANK].optionColumn,
			        extRunMenuItems[MENU1_ITEM_SRAM_BANK].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_ROM_INFO)
		{
			if (!highlightedIsDir)
			{
				switch_to_menu(MID_ROM_INFO_MENU, 0);
			}
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_SD_INFO)
		{
			switch_to_menu(MID_SD_INFO_MENU, 0);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_DUMP_TO_SD)
		{
			// Go to the memory dumping screen
			switch_to_menu(MID_DUMP_MENU, 0);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_ACTION_REPLAY)
		{
			// Go to the action replay screen
			switch_to_menu(MID_AR_ENTRY_MENU, 0);
		}
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_GAME_GENIE)
		{
			// Go to the game genie screen
			switch_to_menu(MID_GG_ENTRY_MENU, 0);
		}
#ifdef CART_TESTS
		else if (highlightedOption[MID_EXT_RUN_MENU] == MENU1_ITEM_TEST_CART)
		{
			// Go to the game genie screen
			switch_to_menu(MID_CART_TEST_MENU, 0);
		}
#endif
	}
	else if (keys & JOY_START)
	{
		// B
		run_highlighted_game();
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_EXT_RUN_MENU])
		{
			// Un-highlight the previously highlighted string(s), and highlight the new one(s)
			print_menu_item(extRunMenuItems,
			                highlightedOption[MID_EXT_RUN_MENU],
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_menu_item(extRunMenuItems,
			                highlightedOption[MID_EXT_RUN_MENU] - 1,
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			highlightedOption[MID_EXT_RUN_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (highlightedOption[MID_EXT_RUN_MENU] < MENU1_ITEM_LAST - 1)
		{
			print_menu_item(extRunMenuItems,
			                highlightedOption[MID_EXT_RUN_MENU],
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_menu_item(extRunMenuItems,
			                highlightedOption[MID_EXT_RUN_MENU] + 1,
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			highlightedOption[MID_EXT_RUN_MENU]++;
		}
	}
	else if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU, 0);
	}
}
void dump_menu_process_keypress(u16 keys)
{
	if (keys & JOY_B)
	{
		// B
		if (dumping) return;
		if (highlightedOption[MID_DUMP_MENU] == MENU3_ITEM_DUMP_TYPE)
		{
			dumpType++; if (dumpType > DUMP_NEO_SRAM) dumpType = DUMP_ROM;
			dumpMenuItems[MENU3_ITEM_DUMP_TYPE].optionValue = (char*)dumpTypeStrings[dumpType];
			print_option(dumpMenuItems, MENU3_ITEM_DUMP_TYPE);
		}
		else if (highlightedOption[MID_DUMP_MENU] == MENU3_ITEM_ROM_LAYOUT)
		{
			romDumpLayout ^= 1;
			dumpMenuItems[MENU3_ITEM_ROM_LAYOUT].optionValue = (char*)&(metaStrings[48 + (romDumpLayout^1)][4]);
			print_option(dumpMenuItems, MENU3_ITEM_ROM_LAYOUT);
		}
		else if (highlightedOption[MID_DUMP_MENU] == MENU3_ITEM_ROM_SIZE)
		{
			romDumpSize++; if (romDumpSize > 0x0C) romDumpSize = 0x05;
			dumpMenuItems[MENU3_ITEM_ROM_SIZE].optionValue = (char*)romRamSizeStrings[romDumpSize];
			print_option(dumpMenuItems, MENU3_ITEM_ROM_SIZE);
		}
		else if (highlightedOption[MID_DUMP_MENU] == MENU3_ITEM_SRAM_SIZE)
		{
			sramDumpSize++; if (sramDumpSize > 0x05) sramDumpSize = 0x00;
			dumpMenuItems[MENU3_ITEM_SRAM_SIZE].optionValue = (char*)romRamSizeStrings[sramDumpSize];
			print_option(dumpMenuItems, MENU3_ITEM_SRAM_SIZE);
		}
		else if (highlightedOption[MID_DUMP_MENU] == MENU3_ITEM_SRAM_ADDR)
		{
			sramDumpAddr ^= 1;
			dumpMenuItems[MENU3_ITEM_SRAM_ADDR].optionValue = (char*)sramAddrStrings[sramDumpAddr];
			print_option(dumpMenuItems, MENU3_ITEM_SRAM_ADDR);
		}
		else if (highlightedOption[MID_DUMP_MENU] == MENU3_ITEM_NEO_SRAM_SIZE)
		{
			neoSramDumpSize++; if (neoSramDumpSize > 0x05) neoSramDumpSize = 0x00;
			dumpMenuItems[MENU3_ITEM_NEO_SRAM_SIZE].optionValue = (char*)romRamSizeStrings[neoSramDumpSize];
			print_option(dumpMenuItems, MENU3_ITEM_NEO_SRAM_SIZE);
		}
		else if (highlightedOption[MID_DUMP_MENU] == MENU3_ITEM_NEO_SRAM_ADDR)
		{
			neoSramDumpAddr++; if (neoSramDumpAddr > 31) neoSramDumpAddr = 0;
			dumpMenuItems[MENU3_ITEM_NEO_SRAM_ADDR].optionValue = (char*)neoSramAddrStrings[neoSramDumpAddr];
			print_option(dumpMenuItems, MENU3_ITEM_NEO_SRAM_ADDR);
		}
	}
	else if (keys & JOY_START)
	{
		// Start
		if (dumping) return;
		dumping = 1;
		dump_to_sd();
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (dumping) return;
		if (highlightedOption[MID_DUMP_MENU])
		{
			// Un-highlight the previously highlighted string(s), and highlight the new one(s)
			print_menu_item(dumpMenuItems,
			                highlightedOption[MID_DUMP_MENU],
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_menu_item(dumpMenuItems,
			                highlightedOption[MID_DUMP_MENU] - 1,
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			highlightedOption[MID_DUMP_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (dumping) return;
		if (highlightedOption[MID_DUMP_MENU] < MENU3_ITEM_LAST - 1)
		{
			print_menu_item(dumpMenuItems,
			                highlightedOption[MID_DUMP_MENU],
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_menu_item(dumpMenuItems,
			                highlightedOption[MID_DUMP_MENU] + 1,
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
				highlightedOption[MID_DUMP_MENU]++;
		}
	}
	else if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_EXT_RUN_MENU, 0);
	}
}


void cart_test_menu_process_keypress(u16 keys)
{
#ifdef CART_TESTS
	void        (*psram_read)(char*, u16, u16, u16);
	void        (*psram_write)(char*, u16, u16, u16);
	static u16  buf[0x20];
	u16         prbank, proffs, i;
	u16         banks, bnum, chk;
	u16         checkOk;

	if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_CART_TEST_MENU])
		{
			// Un-highlight the previously highlighted string(s), and highlight the new one(s)
			print_menu_item(cartTestMenuItems,
			                highlightedOption[MID_CART_TEST_MENU],
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_menu_item(cartTestMenuItems,
			                highlightedOption[MID_CART_TEST_MENU] - 1,
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			highlightedOption[MID_CART_TEST_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (highlightedOption[MID_CART_TEST_MENU] < MENU2_ITEM_LAST - 1)
		{
			print_menu_item(cartTestMenuItems,
			                highlightedOption[MID_CART_TEST_MENU],
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_menu_item(cartTestMenuItems,
			                highlightedOption[MID_CART_TEST_MENU] + 1,
			                TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			highlightedOption[MID_CART_TEST_MENU]++;
		}
	}
	else if (keys & JOY_B)
	{
		// B
		if ((highlightedOption[MID_CART_TEST_MENU] == MENU2_ITEM_TEST_MYTH_PSRAM)  ||
		    (highlightedOption[MID_CART_TEST_MENU] == MENU2_ITEM_TEST_GBAC_PSRAM))
		{
			if (highlightedOption[MID_CART_TEST_MENU] == MENU2_ITEM_TEST_MYTH_PSRAM)
			{
				MAKE_RAM_FPTR(psram_write, neo2_myth_psram_write_test_data);
				MAKE_RAM_FPTR(psram_read, neo2_myth_psram_read);
				banks = 0x80;
				bnum  = 0x50;
			}
			else
			{
				MAKE_RAM_FPTR(psram_write, neo2_gbac_psram_write_test_data);
				MAKE_RAM_FPTR(psram_read, neo2_gbac_psram_read);
				banks = 0x100;
				bnum  = 0x40;
			}
			clear_screen();
			print_hw_card_rev();
			print_meta_string(MS_VERSION_COPYRIGHT);
			printxy("Writing ", 2, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			update_screen();
			checkOk = 1;
			for (prbank = 0x00; prbank < banks; prbank++)
			{
				print_hex(prbank>>8, 10, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(prbank,    12, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				update_screen();
				psram_write(0, prbank, 0x0000, 0x8000);
				psram_write(0, prbank, 0x8000, 0x8000);
			}
			printxy("Reading ", 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			update_screen();
			for (prbank = 0x00; prbank < banks; prbank++)
			{
				print_hex(prbank>>8, 10, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(prbank,    12, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				update_screen();
				proffs = 0;
				do
				{
					psram_read((char*)buf, prbank, proffs, 0x40);
					if (bnum == 0x40) {
						chk = prbank + (proffs>>1);
						for (i = 0; i < 0x20; i++)
						{
							if (buf[i] != chk)
							{
								checkOk = 0;
								break;
							}
							chk++;
						}
					}
					else
					{
						chk = proffs + (bnum | (prbank&0x0F));
						for (i = 0; i < 0x20; i++)
						{
							if (buf[i] != chk)
							{
								checkOk = 0;
								break;
							}
							chk += 2;
						}
					}
					if (checkOk==0) break;
					proffs += 0x40;
				} while (proffs);
				if (checkOk==0) break;
			}
            if (checkOk)
			{
				printxy("OK", 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			}
			else
			{
				printxy("FAIL", 	2, 11, 4, 21);
				printxy("Bank", 	2, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				print_hex(prbank, 	7, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				printxy("Offset", 	2, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				print_hex((proffs+i)>>8, 9, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex((proffs+i), 11, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				printxy("Expected", 2, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				print_hex(chk>>8, 11, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(chk, 13, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				printxy("Actual", 	2, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				print_hex(buf[i]>>8,11, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(buf[i], 	13, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(buf[0]>>8,15, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(buf[0], 	17, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(buf[1]>>8,19, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(buf[1], 	21, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			}
		    update_screen();
		}
		else if ((highlightedOption[MID_CART_TEST_MENU] == MENU2_ITEM_TEST_SRAM) ||
		         (highlightedOption[MID_CART_TEST_MENU] == MENU2_ITEM_WRITE_SRAM) ||
		         (highlightedOption[MID_CART_TEST_MENU] == MENU2_ITEM_READ_SRAM))
		{
            MAKE_RAM_FPTR(psram_read, neo2_sram_read);
			MAKE_RAM_FPTR(psram_write, neo2_sram_write);
			clear_screen();
			print_hw_card_rev();
			print_meta_string(MS_VERSION_COPYRIGHT);
			checkOk = 1;
			if (highlightedOption[MID_CART_TEST_MENU] != MENU2_ITEM_READ_SRAM)
			{
				printxy("Writing ", 2, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				update_screen();
				for (i=0; i<0x10; i++)
				{
					buf[i+i] = 0xDEAD;
					buf[i+i+1] = 0xBEEF;
				}
				for (prbank=0; prbank<4; prbank++)
				{
					proffs = 0x0000;
					do
					{
						psram_write((char*)buf, prbank, proffs, 0x40);
						proffs += 0x40;
					} while (proffs);
				}
			}
			if (highlightedOption[MID_CART_TEST_MENU] != MENU2_ITEM_WRITE_SRAM)
			{
				printxy("Reading ", 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				update_screen();
				for (prbank=0; prbank<4; prbank++)
				{
					proffs = 0x0000;
					do
					{
						psram_read((char*)buf, prbank, proffs, 0x40);
						for (i=0; i<0x10; i++)
						{
							if ((buf[i+i] != 0xDEAD) ||
							    (buf[i+i+1] != 0xBEEF)) {
								checkOk = 0;
								break;
							}
						}
						if (checkOk == 0) break;
						proffs += 0x40;
					} while (proffs);
					if (checkOk == 0) break;
				}
			}
			if (checkOk)
			{
				printxy("OK", 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			}
			else
			{
				printxy("FAIL", 	2, 11, 4, 21);
				printxy("Bank", 	2, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				print_hex(prbank,11, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				printxy("Offset", 	2, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				print_hex(proffs>>8,11, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
				print_hex(proffs, 	13, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
			}
		}
	}
	else if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU, 0);
	}
#endif
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
		run_highlighted_game();
	}
	else if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_EXT_RUN_MENU, 0);
	}
	else if (keys & JOY_X)
	{
		// X
		if (ggCodes[highlightedOption[MID_GG_ENTRY_MENU]].used) freeCodeSlots++;
		ggCodes[highlightedOption[MID_GG_ENTRY_MENU]].used = CODE_UNUSED;
		print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]],
		              CODE_LEFT_MARGIN,
		              14 + highlightedOption[MID_GG_ENTRY_MENU],
		              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_GG_ENTRY_MENU])
		{
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]],
			              CODE_LEFT_MARGIN,
			              14 + highlightedOption[MID_GG_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]-1],
			              CODE_LEFT_MARGIN,
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
			              CODE_LEFT_MARGIN,
			              14 + highlightedOption[MID_GG_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_gg_code(&ggCodes[highlightedOption[MID_GG_ENTRY_MENU]+1],
			              CODE_LEFT_MARGIN,
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
		        CODE_LEFT_MARGIN + highlightedOption[MID_GG_EDIT_MENU] + ((highlightedOption[MID_GG_EDIT_MENU]>3)?1:0),
		        14 + whichCode,
		        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
		        1);
		if (++highlightedOption[MID_GG_EDIT_MENU] == 8)
		{
			gg_decode(ggCodes[whichCode].code,
			          &(ggCodes[whichCode].bank),
			          &(ggCodes[whichCode].offset),
			          &(ggCodes[whichCode].val));
			if (ggCodes[whichCode].used == CODE_UNUSED) freeCodeSlots--;
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
			        CODE_LEFT_MARGIN + highlightedOption[MID_GG_EDIT_MENU] + ((highlightedOption[MID_GG_EDIT_MENU]>3)?1:0),
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

// Action Replay code entry
//
void ar_code_entry_menu_process_keypress(u16 keys)
{
	if (keys & JOY_B)
	{
		// A
		switch_to_menu(MID_AR_EDIT_MENU, 1);
	}
	else if (keys & JOY_START)
	{
		// Start
		run_highlighted_game();
	}
	else if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_EXT_RUN_MENU, 0);
	}
	else if (keys & JOY_X)
	{
		// X
		if (ggCodes[MAX_GG_CODES+highlightedOption[MID_AR_ENTRY_MENU]].used) freeCodeSlots++;
		ggCodes[MAX_GG_CODES+highlightedOption[MID_AR_ENTRY_MENU]].used = CODE_UNUSED;
		print_ar_code(&ggCodes[MAX_GG_CODES+highlightedOption[MID_AR_ENTRY_MENU]],
		              CODE_LEFT_MARGIN,
		              14 + highlightedOption[MID_AR_ENTRY_MENU],
		              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_AR_ENTRY_MENU])
		{
			print_ar_code(&ggCodes[MAX_GG_CODES + highlightedOption[MID_AR_ENTRY_MENU]],
			              CODE_LEFT_MARGIN,
			              14 + highlightedOption[MID_AR_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_ar_code(&ggCodes[MAX_GG_CODES + highlightedOption[MID_AR_ENTRY_MENU]-1],
			              CODE_LEFT_MARGIN,
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
			              CODE_LEFT_MARGIN,
			              14 + highlightedOption[MID_AR_ENTRY_MENU],
			              TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
			print_ar_code(&ggCodes[MAX_GG_CODES + highlightedOption[MID_AR_ENTRY_MENU]+1],
			              CODE_LEFT_MARGIN,
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
		        CODE_LEFT_MARGIN + highlightedOption[MID_AR_EDIT_MENU],
		        14 + whichCode - MAX_GG_CODES,
		        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
		        1);
		if (++highlightedOption[MID_AR_EDIT_MENU] == 8)	// Has the user entered an entire 8-character code?
		{
			ar_decode(ggCodes[whichCode].code,
			          &(ggCodes[whichCode].bank),
			          &(ggCodes[whichCode].offset),
			          &(ggCodes[whichCode].val));
			          if (ggCodes[whichCode].used == CODE_UNUSED) freeCodeSlots--;
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
			        CODE_LEFT_MARGIN + highlightedOption[MID_AR_EDIT_MENU],
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
	u16 i, j, n;
	cheat_t const *cheats;
	cheats = cheatDatabase[cheatGameIdx].cheats;
	if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU, 0);
	}
	else if (keys & JOY_START)
	{
		// Start
		run_highlighted_game();
	}
	else if (keys & JOY_B)
	{
		n = strlen(cheats[cheatList.highlighted].codes) >> 3;	// Number of codes for this cheat
		if ((freeCodeSlots >= n) && (cheatApplied[cheatList.highlighted] == 0))
		{
			while (n)
			{
				for (i = 0; i < MAX_GG_CODES * 2; i++)
				{
					if (ggCodes[i].used == CODE_UNUSED)
					{
						for (j = 0; j < 8; j++)
						{
							ggCodes[i].code[j] = cheats[cheatList.highlighted].codes[((n - 1) << 3) + j] - '0';
							if (ggCodes[i].code[j] > 0xF) ggCodes[i].code[j] -= 7;
						}
						if (cheats[cheatList.highlighted].codeType == CODE_TYPE_GG)
						{
							gg_decode(ggCodes[i].code,
						              &(ggCodes[i].bank),
						              &(ggCodes[i].offset),
						              &(ggCodes[i].val));
							ggCodes[i].used = ((ggCodes[i].bank == 0x7e) || (ggCodes[i].bank == 0x7f)) ? CODE_TARGET_RAM : CODE_TARGET_ROM;
						}
						else if (cheats[cheatList.highlighted].codeType == CODE_TYPE_AR)
						{
							ar_decode(ggCodes[i].code,
						              &(ggCodes[i].bank),
						              &(ggCodes[i].offset),
						              &(ggCodes[i].val));
							ggCodes[i].used = ((ggCodes[i].bank == 0x7e) || (ggCodes[i].bank == 0x7f)) ? CODE_TARGET_RAM : CODE_TARGET_ROM;
						}
						freeCodeSlots--;
						break;
					}
				}
				n--;
			}
			cheatApplied[cheatList.highlighted] = 1;
			print_cheat_list();
			printxy("  ", 25, 23, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 2);
			print_dec(freeCodeSlots, 25, 23, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
		}
	}
	else if (keys & JOY_UP)
	{
		move_to_previous_cheat();
	}
	else if (keys & JOY_DOWN)
	{
		move_to_next_cheat();
	}
}
void cheat_db_no_codes_menu_process_keypress(u16 keys)
{
	if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU, 0);
	}
	else if (keys & JOY_B)
	{
		if (highlightedOption[MID_CHEAT_DB_NO_CODES_MENU] == MENU7_ITEM_GAME_GENIE)
		{
			switch_to_menu(MID_GG_ENTRY_MENU, 0);
		}
		else if (highlightedOption[MID_CHEAT_DB_NO_CODES_MENU] == MENU7_ITEM_ACTION_REPLAY)
		{
			switch_to_menu(MID_AR_ENTRY_MENU, 0);
		}
	}
	else if (keys & JOY_UP)
	{
		// Up
		if (highlightedOption[MID_CHEAT_DB_NO_CODES_MENU])
		{
			// Un-highlight the previously highlighted string(s), and highlight the new one(s)
			printxy(noCodesMenuItems[highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]].label,
			        2,
			        noCodesMenuItems[highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
			        32);
			printxy(noCodesMenuItems[highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]-1].label,
			        2,
			        noCodesMenuItems[highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]-1].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);
			highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]--;
		}
	}
	else if (keys & JOY_DOWN)
	{
		// Down
		if (highlightedOption[MID_CHEAT_DB_NO_CODES_MENU] < 1)
		{
			printxy(noCodesMenuItems[highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]].label,
			        2,
			        noCodesMenuItems[highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE),
			        32);
			printxy(noCodesMenuItems[highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]+1].label,
			        2,
			        noCodesMenuItems[highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]+1].row,
			        TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE),
			        32);
			highlightedOption[MID_CHEAT_DB_NO_CODES_MENU]++;
		}
	}
}
void rom_info_menu_process_keypress(u16 keys)
{
	if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_EXT_RUN_MENU, 0);
	}
}

void sd_error_menu_process_keypress(u16 keys)
{
	if (keys & JOY_Y)
	{
		// Y
		switch_to_menu(MID_MAIN_MENU, 0);
	}
	else if (keys & JOY_R)
	{
		// R
		if ((currentMenu == MID_SD_INFO_MENU) &&
		    (sourceMedium == SOURCE_GBAC))
		{
			set_source_medium(SOURCE_SD, 0);
		}
	}
}
void vgm_play_menu_process_keypress(u16 keys)
{
	u16 i;
	if (keys & JOY_Y)
	{
		// Y
		stop_vgm();
		init_sd();
		pf_opendir(&sdDir, sdRootDir);
		for (i = 0; i < gamesList.firstShown; i++)
		{
			pf_readdir(&sdDir, &sdFileInfo);
		}
		switch_to_menu(MID_MAIN_MENU, 0);
	}
	else if (keys & JOY_X)
	{
		// X
		vgm_echo();
	}
}
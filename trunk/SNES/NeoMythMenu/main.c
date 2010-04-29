// SNES Myth Menu
// C version 0.21
//
// Mic, 2010

#include "snes.h"
#include "dma.h"
#include "hw_math.h"
#include "lzss_decode.h"
#include "myth.h"
#include "neo2.h"
#include "navigation.h"
#include "common.h"


// Some resources defined in separate source files
extern char bg_patterns, bg_patterns_end;
extern char bg_palette[];
extern char bg_map, bg_map_end;

extern u8 font[];


// Myth-related variables
u8 cardModel, cpID;
u16 romAddressPins;
u8 gameMode;
u8 romSize, romRunMode, sramSize, sramBank, sramMode;
u8 extDsp, extSram;

sortOrder_t sortOrder = SORT_LOGICALLY;
char sortLetter = 'A';

u16 gbaCardAlphabeticalIdx[500];

gamesList_t gamesList;

// All the text strings are written to this buffer and then sent to VRAM using DMA in order to
// improve performance.
//
char bg0Buffer[0x800];
u8 bg0BufferDirty;

// These three strings are modified at runtime by the menu, so they are declared separately in order to get them into
// the .data section rather than .rodata.
//
char MS2[] = "\xff\x15\x02\x02 LOADING......(  )";
char MS3[] = "\xff\x0b\x02\x02                        \xff\x09\x02\x07SECONDARY CART:";
char MS4[] = "\xff\x15\x02\x02 GAME (001)";

// Each of these metastrings are onm the format 0xff,row,column,palette,"actual text". A single metastring can go on
// indefinitely until a null-terminator is reached.
//
char *metaStrings[] =
{
    "\xff\x03\x01\x07 MENU V 0.21\xff\x02\x01\x03 NEO POWER SNES MYTH CARD (A)\xff\x1a\x04\x05\x22 2010 WWW.NEOFLASH.COM     ",
	"\xff\x01\xfe\x0f\x0a\x68\x69\x6A\x20\x71\x72\x73\x20\x7a\x7b\x7c\x83\x84\x85\xfe\x10\x0a\x6b\x6c\x6d\x20\x74\x75 \
	 \x76\x20\x7d\x7e\x7f\x86\x87\x88xfe\x11\x0a\x6e\x6f\x70\x20\x77\x78\x79\x20\x80\x81\x82\x89\x8a\x8b\xfe\x17\x06 \
	 \x06\xff\x04                             ",
	MS2,
	MS3,
	MS4,
	"\xff\x02\x1b\x03(B)",
	"\xff\x02\x1b\x03(C)",
	"\xff\x03\x14\x07(HARD 0.1)",
	"\xff\x03\x14\x07(HARD 0.1)",
	"\xff\x03\x14\x07(HARD 0.1)",
    "\xff\x16\x03\x02          ",
    "\xff\x16\x03\x02SAVE 2KB  ",
    "\xff\x16\x03\x02SAVE 8KB  ",
    "\xff\x16\x03\x02SAVE 32KB ",
    "\xff\x16\x03\x02SAVE 64KB ",
    "\xff\x16\x03\x02SAVE 128KB",
    "\xff\x15\x01\x02 DSP      ",
	" ",
	" ",
	" ",
	"\xff\x09\x14\x05 8M/64M",
	"\xff\x09\x14\x05\x31\x36M/64M",
	"\xff\x09\x14\x05\x32\x34M/64M",
	"\xff\x09\x14\x05\x33\x32M/64M",
	"\xff\x09\x14\x05\x34\x30M/64M",
	"\xff\x09\x14\x05\x34\x38M/64M",
	"\xff\x09\x14\x05\x35\x36M/64M",
	"\xff\x09\x14\x05\x36\x34M/64M",
	"\xff\x0a\x01\x05 A24 & A25 TEST OK !  ",
	" ",
	"\xff\x09\x01\x02 TESTING PSRAM... ",
	"\xff\x0b\x01\x03 PSRAM TEST OK    ",
	"\xff\x0b\x01\x01 PSRAM TEST ERROR!",
	"\xff\x08\x10\x07\x66",		// Up arrow
	"\xff\x12\x10\x07\x67",		// Down arrow
	"\xff\x08\x10\x07 ",		// Up arrow clear
	"\xff\x12\x10\x07 ",		// Down arrow clear
	// Game sizes (strings nbr 37-47)
	"\xff\x15\x0f\x02\x34M ",
	"\xff\x15\x0f\x02?M ",
	"\xff\x15\x0f\x02?M ",
	"\xff\x15\x0f\x02?M ",
	"\xff\x15\x0f\x02\x38M ",
	"\xff\x15\x0f\x02\x31\x36M",
	"\xff\x15\x0f\x02\x32\x34M",
	"\xff\x15\x0f\x02\x33\x32M",
	"\xff\x15\x0f\x02\x34\x30M",
	"\xff\x15\x0f\x02\x34\x38M",
	"\xff\x15\x0f\x02\x36\x34M",
	// ROM types
	"\xff\x15\x13\x02HIROM",
	"\xff\x15\x13\x02LOROM",
	// DSP types (50-57)
	"\xff\x15\x19\x02    ",
	"\xff\x15\x19\x02\x44SP1",
	"\xff\x15\x19\x02\x44SP2",
	"\xff\x15\x19\x02\x44SP3",
	"\xff\x15\x19\x02\x44SP4",
	"\xff\x15\x19\x02\x44SP5",
	"\xff\x15\x19\x02\x44SP6",
	"\xff\x15\x19\x02SFX ",
	"\xff\x06\x02\x07GAME  \xff\x17\x03\x03\x42\xff\x17\x04\x07: RUN, \xff\x17\x0c\x03Y\xff\x17\x0d\x07: GO BACK\xff\x0d\x01\x07 SYSTEM INFO:",
	// Regions (59-60)
    "\xff\x0f\x01\x02 NTSC (60HZ)",
    "\xff\x0f\x01\x02 PAL  (50HZ)",
	// CPU versions (61-64)
    "\xff\x10\x01\x02 S-CPU  V0",
    "\xff\x10\x01\x02 S-CPU  V1",
    "\xff\x10\x01\x02 S-CPU  V2",
    "\xff\x10\x01\x02 S-CPU  V3",
    // PPU1 versions
    "\xff\x11\x01\x02 S-PPU1 V0",
    "\xff\x11\x01\x02 S-PPU1 V1",
    "\xff\x11\x01\x02 S-PPU1 V2",
    "\xff\x11\x01\x02 S-PPU1 V3",
    // PPU2 versions
    "\xff\x12\x01\x02 S-PPU2 V0",
    "\xff\x12\x01\x02 S-PPU2 V1",
    "\xff\x12\x01\x02 S-PPU2 V2",
    "\xff\x12\x01\x02 S-PPU2 V3",
    // 73
	"\xff\x06\x02\x07GAMES\xff\x17\x03\x03Y\xff\x17\x04\x07: GO BACK\xff\x09\x01\x07 SPC LOAD TEST",
	"\xff\x06\x02\x07OPTION\xff\x17\x03\x03Y\xff\x17\x04\x07: GO BACK",
    "\xff\x06\x02\x07GAMES\xff\x17\x03\x03\x42\xff\x17\x04\x07: RUN, \xff\x17\x0c\x03Y\xff\x17\x0d\x07: RUN 2ND CART",
    "\xff\x06\x02\x07\x43ODES\xff\x17\x03\x03\x42\xff\x17\x04\x07: RUN, \xff\x17\x0f\x03Y\xff\x17\x10\x07: GO BACK\xff\x16\x03\x03\x44PAD\xff\x16\x07\x07: PICK, \xff\x16\x0f\x03\x41\xff\x16\x10\x07: EDIT",
};


ggCode_t ggCodes[MAX_GG_CODES];

////// DEBUG
u8 ggTestCode[] = {0x2,0x2,0xB,0xB,0xA,0xD,0x0,0x1};  // Infinite lives in Contra 3
////////////


const u8 ppuRegData1[12] =
{
     3,		// OBJ data select
	 0,		// OAM address low
	 0,		// OAM address high
	 0,		// OAM data
	 1,		// BG mode
	 0,		// Mosaic (off)
	 0x20,	// BG0 name table size/address
	 0x30,	// BG1 name table size/address
	 0,		// BG2 name table size/address
	 0,		// BG3 name table size/address
	 0x40,	// BG0/1 pattern table address
	 0		// BG2/3 pattern table address
};

const u8 ppuRegData2[9] =
{
     2,		// Enable BG1. BG0 is enabled separately.
	 0,		// Sub screen
	 0,		// Window mask
	 0,		// Window mask
	 2,		// Color addition setting
	 0x20,	// Color addition setting
	 0,
	 0,
	 0
};

const u16 fontColors[] =
{
	0x7fff, 0x3c7f, 0x7fff, 0x47f1,
	0x1fc6, 0x7f18, 0x31ed, 0x4292 //0x718f
};



void wait_nmi()
{
	while (REG_RDNMI & 0x80);
	while (!(REG_RDNMI & 0x80));
}



// Set the value of a pointer to a full 24-bit bank:offset pair
//
// E.g. set_full_pointer((void**)&a_pointer, 0x7f, 0x8000) will make
// a_pointer point to 0x7f8000.
//
void set_full_pointer(void **pptr, u8 bank, u16 offset)
{
	u8 *bp = (u8*)pptr;
	u16 *wp = (u16*)pptr;

	bp[2] = bank;
	*wp = offset;
}


void add_full_pointer(void **pptr, u8 bank, u16 offset)
{
	u8 *bp = (u8*)pptr;
	u16 *wp = (u16*)pptr;
	u16 w;

	w = *wp;
	*wp += offset;

	bp[2] += bank;
	if (*wp < w)
	{
		bp[2]++;
	}
}


void dma_bg0_buffer()
{
	wait_nmi();
	load_vram(bg0Buffer, 0x2000, 0x800);
	bg0BufferDirty = 0;
}


void clear_screen()
{
	int i;

	for (i = 0; i < 0x400; i++)
	{
		bg0Buffer[i + i] = ' ';
		bg0Buffer[i + i + 1] = 0;
	}
	bg0BufferDirty = 1;
}



// Convert 1-bit font data to 4-bit (the three remaining bitplanes are all zeroed).
//
void expand_font_data()
{
	int i, j;
	u8 *bp = font;

	REG_VRAM_ADDR_L = 0x00;
	REG_VRAM_ADDR_H = 0x02;
	REG_VRAM_INC = VRAM_WORD_ACCESS;

	for (i = 0; i < 0xd2; i++)
	{
		// Lower two planes
		for (j = 0; j < 8; j++)
		{
			REG_VRAM_DATAW1 = *bp++;
			REG_VRAM_DATAW2 = 0;
		}
		// Upper two planes
		for (j = 0; j < 8; j++)
		{
			REG_VRAM_DATAW1 = 0;
			REG_VRAM_DATAW2 = 0;
		}
	}
}


void load_font_colors()
{
	u8 i;

	for (i = 0; i < 8; i++)
	{
		REG_CGRAM_ADDR = (i << 4) + 1;
		REG_CGRAM_DATAW = (u8)fontColors[i];
		REG_CGRAM_DATAW = fontColors[i] >> 8;
	}
}


void update_game_params()
{
	int i;
	u8 *pGame;

	if (sortOrder == SORT_LOGICALLY)
	{
		set_full_pointer((void**)&pGame, GAME_LIST_BANK, 0xc800 + (gamesList.highlighted << 6));
	}
	else
	{
		set_full_pointer((void**)&pGame, GAME_LIST_BANK,
		                 0xc800 + (gbaCardAlphabeticalIdx[gamesList.highlighted] << 6));
	}

	if (pGame[0] == 0xff)
	{
		gameMode = pGame[1];

		romSize = pGame[2] >> 4;

		romAddressPins = pGame[3];
		romAddressPins |= (pGame[2] & 0x0f) << 8;

		sramBank = pGame[4] >> 4;
		sramSize = pGame[4] & 0x0f;
		sramMode = pGame[6] & 0x0f;

		extDsp = pGame[5] >> 4;

		extSram = pGame[5] & 0x0f;

		romRunMode = pGame[6] >> 4;
	}
}



// Print a given metastring. E.g. print_meta_string(3) to print MS3.
//
void print_meta_string(u16 msNum)
{
	int i;
	u8 *pStr = (u8*)metaStrings[msNum];
	u8 attribs;
	u16 vramOffs;

	for (;;)
	{
		if (*pStr == 0)
		{
			// We've reached the null-terminator
			break;
		}
		else if (*pStr == 0xff)
		{
			// The next three bytes contain row, column and palette number
			vramOffs = *(++pStr) << 5;
			vramOffs |= *(++pStr);
			vramOffs <<= 1;
			attribs = *(++pStr) << 2;
		}
		else
		{
			// This byte was a character
			bg0Buffer[vramOffs++] = *pStr;
			bg0Buffer[vramOffs++] = attribs;
		}
		pStr++;
	}
	bg0BufferDirty = 1;
}


void printxy(char *pStr, u16 x, u16 y, u16 attribs, u16 maxChars)
{
	int i = x;
	u16 printed = 0;
	u16 vramOffs = (y << 6) + x + x;

	for (;;)
	{
		if (*pStr == 0)
		{
			// We've reached the null-terminator
			break;
		}
		bg0Buffer[vramOffs++] = *pStr;
		bg0Buffer[vramOffs++] = attribs;
		pStr++;
		if (++printed >= maxChars) break;
		if (++i >= 28)
		{
			i = x;
			vramOffs = ((++y) << 6) + x + x;
		}
	}
	bg0BufferDirty = 1;
}


void puts_game_title(u16 gameNum, u16 vramOffs, u8 attributes)
{
	u8 *pGame;
	int i;

	if (sortOrder == SORT_LOGICALLY)
	{
		set_full_pointer((void**)&pGame, GAME_LIST_BANK, 0xc800 + (gameNum << 6));
	}
	else
	{
		set_full_pointer((void**)&pGame, GAME_LIST_BANK,
		                 0xc800 + (gbaCardAlphabeticalIdx[gameNum] << 6));
	}

	// Make sure that the chunk begins with 0xff
	//
	if (*pGame == 0xff)
	{
		pGame += 0x0c;		// Add 0x0c to the address, since that's where the actual string is located
		for (i = 0; i < 28; i++)
		{
			if (pGame[i])
			{
				bg0Buffer[vramOffs++] = pGame[i];
				bg0Buffer[vramOffs++] = attributes;
			}
			else
			{
				break;
			}
		}
	}
	bg0BufferDirty = 1;
}



// Prints info about the highlighted game (size, type, dsp etc.)
//
void print_highlighted_game_info()
{
	u8 *pGame;

	if (sortOrder == SORT_LOGICALLY)
	{
		set_full_pointer((void**)&pGame, GAME_LIST_BANK, 0xc800 + (gamesList.highlighted << 6));
	}
	else
	{
		set_full_pointer((void**)&pGame, GAME_LIST_BANK,
		                 0xc800 + (gbaCardAlphabeticalIdx[gamesList.highlighted] << 6));
	}

	// Print "GAME (nnn)"
	print_meta_string(4);

	// Print game size
	print_meta_string((pGame[2] >> 4) + 33);

	// Print save size
	print_meta_string((pGame[4] & 0xf) + 10);

	// Print ROM type
	print_meta_string(((pGame[6] >> 4) & 1) + 48);

	// Print DSP type
	print_meta_string((pGame[5] >> 4) + 50);
}


void show_scroll_indicators()
{
	// Metastring 33 and 34 are the up/down arrows. 35 and 36 are spaces with the same positions
	print_meta_string(35 - (can_games_list_scroll(DIRECTION_UP) << 1));
	print_meta_string(36 - (can_games_list_scroll(DIRECTION_DOWN) << 1));
}


void print_games_list()
{
	int i;
	u16 vramOffs = 0x0244;

	print_highlighted_game_info();
	show_scroll_indicators();

	for (i = 0; i < NUMBER_OF_GAMES_TO_SHOW; i++)
	{
		puts_game_title(gamesList.firstShown + i,
		                vramOffs,
		                (gamesList.highlighted == gamesList.firstShown + i)?8:24);
		vramOffs += 0x40;
	}

}


void hide_games_list()
{
	int i;

	// Hide the top and bottom arrows
	print_meta_string(35);
	print_meta_string(36);

	for (i = 0; i < 320; i++)
	{
		bg0Buffer[0x240 + i + i] = ' ';
		bg0Buffer[0x240 + i + i + 1] = 0;
	}
	bg0BufferDirty = 1;
}


// Clears the status window (the one showing the ROM size, save size etc.)
//
void clear_status_window()
{
	int i;

	for (i = 0; i < 96; i++)
	{
		bg0Buffer[0x540 + i + i] = ' ';
		bg0Buffer[0x540 + i + i + 1] = 0;
	}
	bg0BufferDirty = 1;
}


// Show hardware and card revision
void print_hw_card_rev()
{
	if (cardModel == 1)
	{
		print_meta_string(5);
	}
	if (cardModel == 2)
	{
		print_meta_string(6);
	}
	if (cpID == 1)
	{
		print_meta_string(9);
	}
}


void run_game_from_gba_card_c()
{
	void (*run_game)(void);

	if (gameMode == 4)
	{
		run_game = run_game_from_gba_card & 0x7fff;
	}
	else if (gameMode == 32)
	{
		run_game = play_spc_from_gba_card & 0x7fff;
	}

	// The AND-operation above will not mask out bits 16-23, so we only add 0x7d to the bank here
	// to get the result we want (0x7e).
	add_full_pointer((void**)&run_game, 0x7d, 0x8000);

	run_game();
}


void run_secondary_cart_c()
{
	void (*run_cart)(void) = run_secondary_cart & 0x7fff;

	add_full_pointer((void**)&run_cart, 0x7d, 0x8000);

	run_cart();
}



int main()
{
	u8 *bp;
	int i;
	u16 keys;

	*(u8*)0x00c040 = 0;				// CPLD ROM

	if ((*(u8*)0x00ffe1 == 0x63) &&
	    (*(u8*)0x00ffe2 == 0x57))
	{
		cpID = *(u8*)0x00ffe0;
	}

	*(u8*)0x00c040 = 1;				// GBA card menu

	cardModel = *(u8*)0x00fff0;

	gamesList.highlighted = gamesList.firstShown = 0;
	gamesList.count = count_games_on_gba_card();
	create_alphabetical_index();

	copy_ram_code();

	REG_DISPCNT = 0x80;				// Turn screen off

	// Mark all Game Genie codes as unused
	for (i = 0; i < MAX_GG_CODES; i++)
	{
		ggCodes[i].used = 0;
	}

	expand_font_data();
	load_font_colors();

	// Load the background graphics data
	lzss_decode_vram(&bg_patterns, 0x4000, (&bg_patterns_end - &bg_patterns));
  	load_cgram(bg_palette, 0, 32);
   	load_vram(&bg_map, 0x3000,(&bg_map_end - &bg_map));

	// Setup various PPU registers
	bp = &(REG_OBSEL);
	for (i = 0; i < 12; i++)
	{
		*bp++ = ppuRegData1[i];
	}
	bp = &(REG_BGCNT);
	for (i = 0; i < 9; i++)
	{
		*bp++ = ppuRegData2[i];
	}

	// Setup color addition parameters
	REG_COLDATA = 0x22;
	REG_COLDATA = 0x41;
	REG_COLDATA = 0x82;

	REG_DISPCNT = 0x0f;			// Screen on, full brightness

	// Reset scrolling for BG0 and BG1
	REG_BG0VOFS = REG_BG0HOFS = 0;
	REG_BG0VOFS = REG_BG0HOFS = 0;
	REG_BG1VOFS = REG_BG1HOFS = 0;
	REG_BG1VOFS = REG_BG1HOFS = 0;

	REG_NMI_TIMEN = 1;

	switch_to_menu(MID_MAIN_MENU);

	dma_bg0_buffer();

	REG_BGCNT = 3;			// Enable BG0 and BG1


	////// DEBUG
	for (i = 0; i < 8; i++) ggCodes[0].code[i] = ggTestCode[i];
	gg_decode(ggCodes[0].code, &(ggCodes[0].bank), &(ggCodes[0].offset), &(ggCodes[0].val)); ggCodes[0].used = 1;
	////////////

	while (1)
	{
		if (bg0BufferDirty)
		{
			dma_bg0_buffer();
		}

		keys = read_joypad();

		switch (currentMenu)
		{
			case MID_EXT_RUN_MENU:
				extended_run_menu_process_keypress(keys);
				break;
			case MID_GG_ENTRY_MENU:
				gg_code_entry_menu_process_keypress(keys);
				break;
			default:
				main_menu_process_keypress(keys);
				break;
		}
	}


	return 0;
}


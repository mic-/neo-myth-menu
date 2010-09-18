// SNES Myth Shell
// C version 0.30
//
// Mic, 2010

#include "snes.h"
#include "ppu.h"
#include "hw_math.h"
#include "aplib_decrunch.h"
#include "myth.h"
#include "neo2.h"
#include "navigation.h"
#include "game_genie.h"
#include "common.h"
#include "cheats/cheat.h"
#include "string.h"
#include "diskio.h"
#include "pff.h"



// Some resources defined in separate source files
extern char bg_patterns, bg_patterns_end;
extern char bg_palette[];
extern char bg_map, bg_map_end;
extern char obj_marker;
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

itemList_t gamesList;

void (*keypress_handler)(u16);

// All the text strings are written to this buffer and then sent to VRAM using DMA in order to
// improve performance.
//
static char bg0Buffer[0x800];
static u8 bg0BufferDirty;

static char tempString[32];
static rect_t printxyClipRect;

FATFS sdFatFs;
extern u16 cardType;
extern long long num_sectors;
extern u8 diskioPacket[7];
extern u8 diskioResp[17];


// These three strings are modified at runtime by the shell, so they are declared separately in order to get them into
// the .data section rather than .rodata.
//
char MS2[] = "\xff\x15\x02\x02 Loading......(  )";
char MS3[] = "\xff\x0b\x02\x02                        \xff\x09\x02\x07Secondary cart:";
char MS4[] = "\xff\x15\x02\x02 Game (001)";

// Each of these metastrings are on the format 0xff,row,column,palette,"actual text". A single metastring can go on
// indefinitely until a null-terminator is reached.
//
const char * const metaStrings[] =
{
    "\xff\x03\x01\x07 Shell v 0.30\xff\x02\x01\x03 NEO POWER SNES MYTH CARD (A)\xff\x1a\x04\x05\x22 2010 WWW.NEOFLASH.COM     ",
	"\xff\x01\xfe\x0f\x0a\x68\x69\x6A\x20\x71\x72\x73\x20\x7a\x7b\x7c\x83\x84\x85\xfe\x10\x0a\x6b\x6c\x6d\x20\x74\x75 \
	 \x76\x20\x7d\x7e\x7f\x86\x87\x88xfe\x11\x0a\x6e\x6f\x70\x20\x77\x78\x79\x20\x80\x81\x82\x89\x8a\x8b\xfe\x17\x06 \
	 \x06\xff\x04                             ",
	MS2,
	MS3,
	MS4,
	"\xff\x02\x1b\x03(B)",
	"\xff\x02\x1b\x03(C)",
	"\xff\x03\x15\x07(H/W 0.1)",
	"\xff\x03\x15\x07(H/W 0.1)",
	"\xff\x03\x15\x07(H/W 0.1)",
    "\xff\x16\x03\x02          ",
    "\xff\x16\x03\x02Save 2KB  ",
    "\xff\x16\x03\x02Save 8KB  ",
    "\xff\x16\x03\x02Save 32KB ",
    "\xff\x16\x03\x02Save 64KB ",
    "\xff\x16\x03\x02Save 128KB",
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
	"\xff\x08\x10\x07\x86",		// Up arrow
	"\xff\x12\x10\x07\x87",		// Down arrow
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
	"\xff\x06\x02\x07Game  \xff\x17\x03\x03\x42\xff\x17\x04\x07: Run, \xff\x17\x0c\x03Y\xff\x17\x0d\x07: Go back\xff\x0d\x01\x07 System info:",
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
	"\xff\x06\x02\x07Music\xff\x17\x03\x03RESET\xff\x17\x08\x07: Go back\xff\x09\x01\x07 SPC Playback",
    "\xff\x06\x02\x07Option\xff\x17\x03\x03Start\xff\x17\x08\x07: Run, \xff\x17\x0f\x03Y\xff\x17\x10\x07: Go back\xff\x16\x03\x03\x42\xff\x16\x04\x07: Edit  ",
    "\xff\x06\x02\x07Games\xff\x17\x03\x03\x42/X\xff\x17\x06\x07: Run, \xff\x17\x0e\x03Y\xff\x17\x0f\x07: Run 2nd cart",
    "\xff\x06\x02\x07\x43odes\xff\x17\x03\x03Start\xff\x17\x08\x07: Run, \xff\x17\x0f\x03Y\xff\x17\x10\x07: Go back\xff\x16\x03\x03X\xff\x16\x04\x07: Delete, \xff\x16\x0f\x03\x42\xff\x16\x10\x07: Edit  ",
    "\xff\x17\x03\x03\x42\xff\x17\x04\x07: Add, \xff\x17\x0f\x03Y\xff\x17\x10\x07: Delete\xff\x16\x03\x03\x44pad\xff\x16\x07\x07: Pick, \xff\x16\x0f\x03\x41\xff\x16\x10\x07: Cancel",
    "\xff\x06\x02\x07\x43heats\xff\x16\x03\x03Start\xff\x16\x08\x07: Run, \xff\x15\x0f\x03\x42\xff\x15\x10\x07: Add \xff\x15\x03\x03\x44pad\xff\x15\x07\x07: Pick, \xff\x16\x0f\x03Y\xff\x16\x10\x07: Go back",
    "\xff\x17\x03\x03\x42\xff\x17\x04\x07: Edit, \xff\x17\x0f\x03Y\xff\x17\x10\x07: Go back",
    // 80
    "\xff\x06\x02\x07Info\xff\x17\x03\x03Y\xff\x17\x04\x07: Go back",
};

extern const cheatDbEntry_t cheatDatabase[];

// Allow for MAX_GG_CODES Game Genie codes, followed by an equal number of Action Replay codes
ggCode_t ggCodes[MAX_GG_CODES*2];
itemList_t cheatList;
u8 cheatApplied[128];
u8 anyRamCheats = 0;		// Do any of the cheats target RAM?
u8 freeCodeSlots = MAX_GG_CODES * 2;

u8 doRegionPatch = 0;		// Should we scan the game for region checks and patch them?

u8 snesRomInfo[0x40];


static const u8 ppuRegData1[12] =
{
     0x60,	// OBJ size (16x16) and pattern address (0x0000)
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

static const u8 ppuRegData2[9] =
{
     2,		// Enable BG1. BG0 and OBJ are enabled separately.
	 0,		// Sub screen
	 0,		// Window mask
	 0,		// Window mask
	 2,		// Color addition setting
	 0x20,	// Color addition setting
	 0,
	 0,
	 0
};

static const u16 fontColors[] =
{
	0x7fff, 0x3c7f, 0x7fff, 0x47f1,
	0x1fc6, 0x7f18, 0x31ed, 0x4292 //0x718f
};


static const u16 objColors[] =
{
	0x31ed, 0x7fff, 0x7fff, 0x47f1,
	0x1fc6, 0x7f18, 0x31ed, 0x4292
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


void testlongarg(unsigned long long foo)
{
	if (foo == 1) return 3;
	return 0;
}


void update_screen()
{
	testlongarg(0x10203040);
	wait_nmi();
	load_vram(bg0Buffer, 0x2000, 0x800);
	update_oam(&marker, 0, 1);
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
static void expand_font_data()
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
			REG_VRAM_DATAW1 = (*bp++); // ^ 0xff;
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


static void load_font_colors()
{
	u8 i;

	for (i = 0; i < 8; i++)
	{
		REG_CGRAM_ADDR = (i << 4) + 1;
		REG_CGRAM_DATAW = (u8)fontColors[i];
		REG_CGRAM_DATAW = fontColors[i] >> 8;
	}
}


static void load_obj_colors()
{
	u8 i;

	for (i = 0; i < 8; i++)
	{
		REG_CGRAM_ADDR = 128 + (i << 4) + 1;
		REG_CGRAM_DATAW = (u8)objColors[i];
		REG_CGRAM_DATAW = objColors[i] >> 8;
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

	for (; y < printxyClipRect.y2;)
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
		if (++i > printxyClipRect.x2) // 28
		{
			i = x;
			vramOffs = ((++y) << 6) + x + x;
			while (*pStr == ' ') pStr++;
		}
	}
	bg0BufferDirty = 1;
}


// Mainly for debugging purposes
//
void print_hex(u8 val, u16 x, u16 y, u16 attribs)
{
	u16 vramOffs = (y << 6) + x + x;
	int i;
	u8 b;

	for (i = 0; i < 2; i++)
	{
		b = (val >> 4) + '0';
		if (b > '9') b += 7;
		bg0Buffer[vramOffs++] = b;
		bg0Buffer[vramOffs++] = attribs;
		val <<= 4;
	}
	bg0BufferDirty = 1;
}


void print_dec(u16 val, u16 x, u16 y, u16 attribs)
{
	char *p = &tempString[32];
	char c;
	u16 chars = 0;

	do
	{
		*(--p) = hw_div16_8_rem16(val, 10) + '0';
		chars++;
		val = hw_div16_8_quot16(val, 10);
	} while (val != 0);
	printxy(p, x, y, attribs, chars);
}


void set_printxy_clip_rect(u16 x1, u16 y1, u16 x2, u16 y2)
{
	printxyClipRect.x1 = x1;
	printxyClipRect.y1 = y1;
	printxyClipRect.x2 = x2;
	printxyClipRect.y2 = y2;
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
	if (currentMenu == MID_MAIN_MENU)
	{
		// Metastring 33 and 34 are the up/down arrows. 35 and 36 are spaces with the same positions
		print_meta_string(35 - (can_games_list_scroll(DIRECTION_UP) << 1));
		print_meta_string(36 - (can_games_list_scroll(DIRECTION_DOWN) << 1));
	}
	/*else if (currentMenu == MID_CHEAT_DB_MENU)
	{
	}*/
}


void print_cheat_list()
{
	int i;
	u16 y, attribs;
	cheat_t const *cheats;

	set_printxy_clip_rect(2, 0, 28, 18);

	if (gameFoundInDb)
	{
		y = 10;
		cheats = cheatDatabase[cheatGameIdx].cheats;
		for (i = cheatList.firstShown; i < cheatList.count; i++)
		{
			attribs = TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE);
			if (i == cheatList.highlighted)
			{
				if (cheatApplied[i])
				{
					attribs = TILE_ATTRIBUTE_PAL(SHELL_BGPAL_LIGHT_GREEN);
				}
				else
				{
					attribs = TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE);
				}
			}
			else if (cheatApplied[i])
			{
				attribs = TILE_ATTRIBUTE_PAL(SHELL_BGPAL_TOS_GREEN);
			}

			printxy((char*)cheats[i].description,
					2,
					y,
					attribs,
					128);
			y += hw_div16_8_quot16(strlen(cheats[i].description), 27) + 1;
			if (y > 17) break;
		}
	}

	set_printxy_clip_rect(2, 0, 28, 31);
}


void print_games_list()
{
	int i;
	u16 vramOffs = 0x0244;

	print_highlighted_game_info();
	show_scroll_indicators();

	for (i = 0; ((i < NUMBER_OF_GAMES_TO_SHOW) && (i < gamesList.count)); i++)
	{
		puts_game_title(gamesList.firstShown + i,
		                vramOffs,
		                (gamesList.highlighted == gamesList.firstShown + i) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE) : TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE));
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


void hide_cheat_list()
{
	int i;

	for (i = 0; i < 288; i++)
	{
		bg0Buffer[0x280 + i + i] = ' ';
		bg0Buffer[0x280 + i + i + 1] = 0;
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
	int i;
	void (*run_game)(void);
	void (*read_rom)(char *, u16, u16, u16);

	//DEBUG
	/*print_hex(gameMode, 2, 5, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(romRunMode, 5, 5, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(anyRamCheats, 8, 5, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	update_screen();*/

	// Read 32 bytes from offset 0x10080 in the highlighted ROM
	MAKE_RAM_FPTR(read_rom, neo2_myth_current_rom_read);
	read_rom(tempString, 1, 0x0080, 32);
	tempString[31] = 0;

	// DEBUG
	/*for (i=0; i<12; i++) {
		print_hex(tempString[i], 2+i+i, 5, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	update_screen();
	return;*/

	if (strcmp(tempString, "SNES-SPC700 Sound File Data v0.") == 0)
	{
		gameMode = GAME_MODE_SPC;
	}

	if (gameMode == GAME_MODE_NORMAL_ROM)
	{
		MAKE_RAM_FPTR(run_game, run_game_from_gba_card);
	}
	else if (gameMode == GAME_MODE_SPC)
	{
		MAKE_RAM_FPTR(run_game, play_spc_from_gba_card);
	}
	else
	{
		clear_status_window();
		printxy("Error in game list!", 3, 21, 4, 32);
		update_screen();
		return;
	}

	for (i = 0; i < MAX_GG_CODES * 2; i++)
	{
		if (ggCodes[i].used == CODE_TARGET_RAM)
		{
			anyRamCheats = 1;
		}
		else
		{
			ggCodes[i].bank &= 0x3f;
		}
		if (romRunMode == 1)
		{
			// Convert LOROM addresses to file offsets
			if (ggCodes[i].used == CODE_TARGET_ROM)
			{
				ggCodes[i].offset &= 0x7fff;
				if (ggCodes[i].bank & 1) ggCodes[i].offset |= 0x8000;
				ggCodes[i].bank >>= 1;
			}
		}
	}


	run_game();
}


void run_secondary_cart_c()
{
	void (*run_cart)(void);

  	MAKE_RAM_FPTR(run_cart, run_secondary_cart);
	run_cart();
}



unsigned char crc8 (unsigned char *buf)
{
    int i;
    unsigned long long r4 = 0x80808080;
    unsigned char crc = 0;
    unsigned char c = 0;

    i = 5 * 8;
    do {
        if (r4 & 0x80) c = *buf++;
        crc = crc << 1;

        if (crc & 0x80) crc ^= 9;
        if (c & (r4>>24)) crc ^= 9;
        r4 = (r4 >> 1) | (r4 << 31);
      } while (--i > 0);

  return crc;
}



extern unsigned char pkt[6];

void sendCmd( unsigned char cmd, unsigned long long arg )
{
    pkt[0]=cmd|0x40;                         // b7 = 0 => start bit, b6 = 1 => host command
    pkt[1]=(unsigned char)(arg >> 24);
    pkt[2]=(unsigned char)(arg >> 16);
    pkt[3]=(unsigned char)(arg >> 8);
    pkt[4]=(unsigned char)(arg);
    pkt[5]=( crc8(pkt) << 1 ) | 1;             // b0 = 1 => stop bit
}

int init_sd()
{
	int mountResult = 0;

	cardType = 0;

	diskio_init();

	//neo2_enable_sd();
    if (mountResult = pf_mount(&sdFatFs))
    {
		print_hex(mountResult, 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
    	cardType = 0x8000;
        if (mountResult = pf_mount(&sdFatFs))
     	{
			print_hex(mountResult, 6, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			return -1;
        }
    }

    if (pf_open("/Map.spc"))
    {
		return -2;
    }

    return 0;
}


void setup_video()
{
	int i;
	u8 *bp;

	expand_font_data();
	load_font_colors();

	set_printxy_clip_rect(2, 0, 28, 31);

	// Load the background graphics data
	//lzss_decode_vram(&bg_patterns, 0x4000, (&bg_patterns_end - &bg_patterns));
	aplib_decrunch(&bg_patterns, 0x7f6000);
	load_vram(0x7f6000, 0x4000, 21152);

  	load_cgram(bg_palette, 0, 32);
   	load_vram(&bg_map, 0x3000,(&bg_map_end - &bg_map));

	// Load sprite patterns
	load_vram(&obj_marker, 200*16, 2*32);
	load_vram(&obj_marker+64, 216*16, 2*32);

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

	// Clear OAM
	REG_OAMADDL = 0;
	REG_OAMADDH = 0;
	for (i = 0; i < 512+32; i++)
	{
		REG_OAMDATA = 0;
	}

	load_obj_colors();

	navigation_init();

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
}


extern unsigned char pfmountbuf[36];

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

	copy_ram_code();

	REG_DISPCNT = 0x80;				// Turn screen off

	// Mark all Game Genie codes as unused
	for (i = 0; i < MAX_GG_CODES * 2; i++)
	{
		ggCodes[i].used = CODE_UNUSED;
	}

	setup_video();

	REG_NMI_TIMEN = 1;

	clear_screen();
	switch_to_menu(MID_MAIN_MENU, 0);
	update_screen();

	REG_BGCNT = 3;			// Enable BG0 and BG1

	if ((i = init_sd()) == 0)
	{
		//neo2_disable_sd();
		printxy("File open ok", 2, 7, 4, 32);
	}
	else
	{
		//neo2_disable_sd();
		if (i == -1)
		{
			printxy("Failed to mount root", 2, 7, 4, 32);
		}
		else if (i == -2)
		{
			printxy("Failed to open map.spc", 2, 7, 4, 32);
		}
		for (i = 0; i < 12; i++)
		{
			print_hex(pfmountbuf[i], 2+i+i, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
		}

		print_hex(cardType>>8, 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
		print_hex(cardType, 4, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

		print_hex(num_sectors>>24, 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
		print_hex(num_sectors>>16, 4, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
		print_hex(num_sectors>>8, 6, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
		print_hex(num_sectors, 8, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

		for (i = 0; i < 7; i++)
		{
			print_hex(diskioPacket[i], 2+i+i, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
		}
		for (i = 0; i < 6; i++)
		{
			print_hex(diskioResp[i], 2+i+i, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
		}

		sendCmd(8, 0x1AA);
		print_hex(pkt[5], 2, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	}

	while (1)
	{
		if (bg0BufferDirty)
		{
			update_screen();
		}

		keys = read_joypad();

		keypress_handler(keys);
	}


	return 0;
}


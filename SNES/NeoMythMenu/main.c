// SNES Myth Shell
// C version 0.52
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
#include "sd_utils.h"
#include "bg_buffer.h"


// Some resources defined in separate source files
extern char bg_patterns, bg_patterns_end;
extern char bg_palette[];
extern char bg_map, bg_map_end;
extern char obj_marker;
extern u8 font[];
//extern u8 vgzfile[];


// Myth-related variables
u8 cardModel, cpID;
u16 romAddressPins;
u8 gameMode;
u8 romSize, romRunMode, sramSize, sramBank, sramMode;
u8 extDsp, extSram;
u16 neo_mode;


sortOrder_t sortOrder = SORT_LOGICALLY;
char sortLetter = 'A';

static char tempString[200];

extern const cheatDbEntry_t cheatDatabase[];

// Allow for MAX_GG_CODES Game Genie codes, followed by an equal number of Action Replay codes
ggCode_t ggCodes[MAX_GG_CODES*2];
itemList_t cheatList;
u8 cheatApplied[128];
u8 anyRamCheats = 0;		// Do any of the cheats target RAM?
u8 freeCodeSlots = MAX_GG_CODES * 2;


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




///////////// DEBUG ///////////////////


extern unsigned char pkt[6];
extern u8 diskioTemp[8];
u8 cmdBitsWritten[16];
u8 cmdBitsRead[16];
u8 asicCommands[16];
extern unsigned char pfmountbuf[36];
unsigned char pfMountFmt;

extern unsigned long long sec_tags[16];
extern unsigned char sec_cache[16*512 + 8];


///////////////////////////////////////////////



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


// Add a 24-bit bank:offset pair to a pointer
//
// E.g. a_pointer = 0x500000; add_full_pointer((void**)&a_pointer, 0x12, 0x3456)
// will make a_pointer point to 0x623456
//
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


void update_game_params(int force)
{
	int i;
	u8 *pGame;

	if (sourceMedium == SOURCE_GBAC)
	{
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
	else if ((sourceMedium == SOURCE_SD) && force)
	{
		// This is handled before loading a game in the SD case
	}
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
	u16 attrib;
	static DIR dir;

	if (sourceMedium == SOURCE_GBAC)
	{
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
	else if (sourceMedium == SOURCE_SD)
	{
		// Print "GAME (nnn)"
		print_meta_string(4);

		show_scroll_indicators();

		memcpy(&dir, &sdDir, sizeof(DIR));
		highlightedIsDir = 0;

		for (i = 0; ((i < NUMBER_OF_GAMES_TO_SHOW) && (i < gamesList.count)); )
		{
			if (pf_readdir(&dir, &sdFileInfo) == FR_OK)
			{
				if (dir.sect != 0)
				{
					attrib = TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE);
					if (gamesList.highlighted == gamesList.firstShown + i)
					{
						strcpy(highlightedFileName, &sdFileInfo.fname[0]);
						highlightedFileSize = sdFileInfo.fsize;
						attrib = TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE);
						if (sdFileInfo.fattrib & AM_DIR)
						{
							highlightedIsDir = 1;
						}
					}
					printxy("            ", 2, 9 + i, 0, 28);
					if (sdFileInfo.fattrib & AM_DIR)
					{
						attrib = (attrib != TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE)) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_TOS_GREEN) : attrib;
						print_dir(&sdFileInfo.fname[0],
								2, 9 + i,
								attrib,
								28);
					}
					else
					{
						printxy(&sdFileInfo.fname[0],
								2, 9 + i,
								attrib,
								28);
					}
					i++;

				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
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
	else
	{
		printxy("CPLD ID:", 20, 3, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 32);
		print_hex(cpID, 28, 3, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
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



void play_spc_from_sd_card_c()
{
	static u8 *myth_pram_bio = (u8*)0xC006;
	static WORD prbank, proffs;
	WORD i, progress;
	void (*play_file)(void);

	if ((lastSdError = pf_open(tempString)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}


	clear_status_window();
	update_screen();
	loadProgress[14] = '6'; loadProgress[15] = '5';

	*myth_pram_bio = 0;
	prbank = 0x50;
	proffs = 0;
	progress = 1;
	for (i = 0; i < 129; i++)
	{
		if (progress)
		{
			show_loading_progress();
		}
		progress ^= 1;

		if ((lastSdError = pf_read_sect_to_psram(prbank, proffs, 1)) != FR_OK)
		{
			lastSdOperation = SD_OP_READ_FILE;
			switch_to_menu(MID_SD_ERROR_MENU, 0);
			return;
		}
		proffs += 512;
		if (proffs == 0)
		{
			prbank++;
		}
	}

	MAKE_RAM_FPTR(play_file, play_spc_from_sd_card);

	play_file();
}


void play_vgm_from_sd_card_c()
{
	static u8 *myth_pram_bio = (u8*)0xC006;
	static WORD prbank, proffs, sects, bytesRead;
	static char buf[4];
	WORD i, progress;
	long long (*play_file)(void);

	if (highlightedFileSize > 0x20000)
	{
		clear_status_window();
		printxy("VGM too large!", 3, 21, 4, 32);
		update_screen();
		return;
	}

	if ((lastSdError = pf_open(tempString)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}

	if ((lastSdError = pf_read(buf, 4, &bytesRead)) != FR_OK)
	{
		lastSdOperation = SD_OP_READ_FILE;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}
	if ((buf[0] != 'V') || (buf[1] != 'g') || (buf[2] != 'm'))
	{
		clear_status_window();
		printxy("Not a valid VGM!", 3, 21, 4, 32);
		update_screen();
		return;
	}

	if ((lastSdError = pf_open(tempString)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}

	clear_status_window();

	hide_games_list();
	printxy("Uncompressed size:", 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	print_dec(highlightedFileSize, 21, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	update_screen();

	*myth_pram_bio = 0;
	prbank = 0x50;
	proffs = 0;
	progress = 1;
	sects = highlightedFileSize >> 9;
	if (highlightedFileSize & 0x1FF)
	{
		sects++;
	}

	loadProgress[14] = (((sects>>1)+1)/10)+'0';
	loadProgress[15] = (((sects>>1)+1)%10)+'0';

	for (i = 0; i < sects; i++)
	{
		if (progress)
		{
			show_loading_progress();
		}
		progress ^= 1;

		if ((lastSdError = pf_read_sect_to_psram(prbank, proffs, 1)) != FR_OK)
		{
			lastSdOperation = SD_OP_READ_FILE;
			switch_to_menu(MID_SD_ERROR_MENU, 0);
			return;
		}
		proffs += 512;
		if (proffs == 0)
		{
			prbank++;
		}
	}

	MAKE_RAM_FPTR(play_file, play_vgm_from_sd_card);

	printxy("Compressing..", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	update_screen();
	compressedVgmSize = play_file();

	printxy("Compressed size:", 2, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	print_dec(compressedVgmSize, 21, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	if (compressedVgmSize <= 57856)
	{
		switch_to_menu(MID_VGM_PLAY_MENU, 0);
	}
	else
	{
		printxy("VGM too large!", 3, 21, 4, 32);
		update_screen();
		return;
	}
}



void run_game_from_sd_card_c()
{
	int i,j;
	void (*run_game)(void);
	void (*mirror_psram)(DWORD,DWORD,DWORD);
	static WORD mbits, sects, bytesRead, mythprbank, prbank, proffs, recalcSector;
	static u8 *myth_pram_bio = (u8*)0xC006;
	romLayout_t layout;

	if (highlightedIsDir)
	{
		change_directory(highlightedFileName);
		return;
	}

	strcpy(tempString, sdRootDir);
	if (sdRootDirLength > 1)
	{
		strcpy(&tempString[sdRootDirLength], "/");
		strcpy(&tempString[sdRootDirLength+1], highlightedFileName);
		tempString[sdRootDirLength+1+strlen(highlightedFileName)] = 0;
	}
	else
	{
		strcpy(&tempString[sdRootDirLength], highlightedFileName);
		tempString[sdRootDirLength+strlen(highlightedFileName)] = 0;
	}
	layout = get_rom_info_sd(tempString, snesRomInfo);
	romRunMode = layout ^ 1;

	if ((strstri(".SMC", highlightedFileName) > 0) ||
	    (strstri(".BIN", highlightedFileName) > 0))
	{
		gameMode = GAME_MODE_NORMAL_ROM;
	}
	else if (strstri(".SPC", highlightedFileName) > 0)
	{
		gameMode = GAME_MODE_SPC;
		play_spc_from_sd_card_c();
		return;
	}
	else if (strstri(".VGM", highlightedFileName) > 0)
	{
		gameMode = GAME_MODE_VGM;
		play_vgm_from_sd_card_c();
		return;
	}
	else
	{
		clear_status_window();
		printxy("Not a ROM!", 3, 21, 4, 32);
		update_screen();
		return;
	}

	if (romRunMode > 1)
	{
		clear_status_window();
		printxy("Unknown ROM type!", 3, 21, 4, 32);
		update_screen();
		return;
	}

	mbits = highlightedFileSize >> 17;
	switch (mbits)
	{
		case 2:
			romSize = 4; break;
		case 4:
			romSize = 4; break;
		case 6:
			romSize = 8; break;
		case 8:
			romSize = 8; break;
		case 12:
			romSize = 9; break;
		case 16:
			romSize = 9; break;
		case 24:
			romSize = 0x0A; break;
		case 32:
			romSize = 0x0B; break;
		case 40:
			romSize = 0x0C; break;
		case 48:
			romSize = 0x0D; break;
		case 64:
			romSize = 0x0E; break;
		default:
			romSize = 9; break;  // TODO: Treat this as an error?
	}

	loadProgress[14] = ((mbits+1)/10)+'0';
	loadProgress[15] = ((mbits+1)%10)+'0';

	MAKE_RAM_FPTR(run_game, run_game_from_sd_card);

	// Disable these for the time being
	extDsp = 0;
	extSram = 0;
	sramBank = 0;
	sramSize = 0;
	sramMode = 0;

	// Check SRAM size
	switch (snesRomInfo[0x18])
	{
		case 1:	 	// 2 kB
			sramSize = 1;
			break;
		case 3:		// 8 kB
			sramSize = 2;
			break;
		case 5:		// 32 kB
			sramSize = 3;
			break;
		case 6:		// 64 kB
			sramSize = 4;
			break;
		case 7:		// 128 kB
			sramSize = 5;
			break;
	}

	if (sramSize)
	{
		if (layout == LAYOUT_LOROM)
		{
			if (sramSize <= 5)	// <= 32 kB
			{
				sramMode = 5;
			}
		}
	}
	if (sramMode == 0) sramSize = 0;

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

	clear_status_window();
	update_screen();

	if ((lastSdError = pf_open(tempString)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}

	// Skip past the SMC header if present
	recalcSector = 1;
	if (highlightedFileSize & 0x3FF)
	{
		if ((lastSdError = pf_lseek(512)) != FR_OK)
		{
			lastSdOperation = SD_OP_SEEK;
			switch_to_menu(MID_SD_ERROR_MENU, 0);
			return;
		}
		// Skip the sector calculation on the first read after the seek
		recalcSector = 0;
	}

	prbank = 0x50;
	mythprbank = 0;
	*myth_pram_bio = mythprbank;
	proffs = 0;
	i = mbits;
	while (i)
	{
		show_loading_progress();

		if ((lastSdError = pf_read_1mbit_to_psram_asm(prbank, proffs, mythprbank, recalcSector)) != FR_OK)
		//if ((lastSdError = pf_read_1mbit_to_psram(prbank, proffs, recalcSector)) != FR_OK)
		{
			lastSdOperation = SD_OP_READ_FILE;
			switch_to_menu(MID_SD_ERROR_MENU, 0);
			return;
		}
		recalcSector = 1;

		prbank += 2;
		if (prbank == 0x60)
		{
			prbank = 0x50;
			mythprbank++;
			*myth_pram_bio = mythprbank;
		}
		i--;
	}

	// Special case for 20 Mbit ROMs: mirror the last 4 Mbit
	if (mbits == 20)
	{
		MAKE_RAM_FPTR(mirror_psram, neo2_myth_psram_copy);

		romSize = 0x0B;

		printxy("Mirroring..     ", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
		update_screen();

		// Mirror it 3 times to fill up 32 Mbit
		mirror_psram(0x780000, 0x700000, 0x080000);
		mirror_psram(0x800000, 0x700000, 0x080000);
		mirror_psram(0x880000, 0x700000, 0x080000);
	}

	run_game();
}



romLayout_t get_rom_info_sd(char *fname, u8 *romInfo)
{

	romLayout_t result = LAYOUT_UNKNOWN;
	DWORD ofs;
	WORD bytesRead;
	static u8 buf[0x40];

	lastSdOperation = SD_OP_OPEN_FILE;
	if ((lastSdError = pf_open(fname)) == FR_OK)
	{
		ofs = 0x7FC0;
		if (highlightedFileSize & 0x3FF)
		{
			ofs += 512;
		}
		lastSdOperation = SD_OP_SEEK;
		if ((lastSdError = pf_lseek(ofs)) == FR_OK)
		{
			lastSdOperation = SD_OP_READ_FILE;
			if ((lastSdError = pf_read(romInfo, 0x40, &bytesRead)) == FR_OK)
			{
				// Is the checksum correct?
				if ((((romInfo[0x1c] ^ romInfo[0x1e]) != 0xff) ||
					((romInfo[0x1d] ^ romInfo[0x1f]) != 0xff)) &&
					(highlightedFileSize >= 0x10000))
				{
					lastSdOperation = SD_OP_SEEK;
					ofs += 0x8000;
					if ((lastSdError = pf_lseek(ofs)) == FR_OK)
					{
						lastSdOperation = SD_OP_READ_FILE;
						if ((lastSdError = pf_read(buf, 0x40, &bytesRead)) == FR_OK)
						{
							if (((buf[0x1c] ^ buf[0x1e]) != 0xff) ||
								((buf[0x1d] ^ buf[0x1f]) != 0xff))
							{
								//result = LAYOUT_LOROM;
							}
							else
							{
								// HiROM
								memcpy(romInfo, buf, 0x40);
								result = LAYOUT_HIROM;
							}
						}
					}
				}
				else
				{
					// LoROM
					result = LAYOUT_LOROM;
				}
			}
		}
	}

	// Check the RESET vector if no valid checksum was found
	if (result == LAYOUT_UNKNOWN)
	{
		if (romInfo[0x3d] >= 0x80)
		{
			if (buf[0x3d] < 0x80)
			{
				result = LAYOUT_LOROM;
			}
		}
		else if (buf[0x3d] >= 0x80)
		{
			memcpy(romInfo, buf, 0x40);
			result = LAYOUT_HIROM;
		}
	}

	// Check the ROM type if no (or two) valid RESET vectors were found
	if (result == LAYOUT_UNKNOWN)
	{
		if (romInfo[0x15] >= 0x20)
		{
			result = LAYOUT_LOROM;
		}
		else if (buf[0x15] >= 0x20)
		{
			memcpy(romInfo, buf, 0x40);
			result = LAYOUT_HIROM;
		}
	}

	return result;
}





// DEBUG
void print_sd_status()
{
	int i;

	print_hex(cardType>>8, 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(cardType, 4, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	print_hex(num_sectors>>24, 2, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(num_sectors>>16, 4, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(num_sectors>>8, 6, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(num_sectors, 8, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	print_hex(pfMountFmt, 12, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(sdFatFs.n_rootdir, 15, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	for (i = 0; i < 7; i++)
	{
		print_hex(diskioPacket[i], 2+i+i, 12, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	for (i = 0; i < 6; i++)
	{
		print_hex(diskioResp[i], 2+i+i, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}

	//sendCmd(8, 0x1AA);
	print_hex(diskioTemp[4], 2, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(diskioTemp[5], 6, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	print_hex(asicCommands[2], 14, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(asicCommands[1], 16, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(asicCommands[0], 18, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	print_hex(asicCommands[6], 22, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(asicCommands[5], 24, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(asicCommands[4], 26, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	for (i = 0; i < 8; i++)
	{
		print_hex(cmdBitsWritten[8-i], 2+i+i, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	for (i = 0; i < 8; i++)
	{
		print_hex(cmdBitsRead[8-i], 2+i+i, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	for (i = 0; i < 12; i++)
	{
		print_hex(pfmountbuf[i], 2+i+i, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}

	print_hex(sec_tags[0]>>8, 2, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(sec_tags[0], 4, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(sec_tags[1]>>8, 7, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(sec_tags[1], 9, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	print_hex(diskioTemp[6], 12, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	for (i = 0; i < 10; i++)
	{
		print_hex(sec_cache[508+i], 2+i+i, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}

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


int main()
{
	u8 *bp;
	int i;
	u16 keys;

	//void (*pfninflate)(DWORD, u8 *);

	static DIR dir;
	static FILINFO fileInfo;

	*(u8*)0x00c040 = 0;				// CPLD ROM

	if ((*(u8*)0x00ffe1 == 0x63) &&
	    (*(u8*)0x00ffe2 == 0x57))
	{
		cpID = *(u8*)0x00ffe0;
	}

	*(u8*)0x00c040 = 1;				// GBA card menu

	// Use a faster SD->PSRAM transfer mode if the firmware supports it
	if (cpID >= 4)
	{
		MAKE_RAM_FPTR(recv_sd_psram_multi, neo2_recv_sd_psram_multi_hwaccel);
	}
	else
	{
		MAKE_RAM_FPTR(recv_sd_psram_multi, neo2_recv_sd_psram_multi);
	}

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

	/*MAKE_RAM_FPTR(pfninflate, inflate);
	inflate_start();
	pfninflate(0x7F4000, vgzfile+10);*/

	REG_BGCNT = 3;			// Enable BG0 and BG1

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

// SNES Myth Shell
// C version 0.65
//
// Mic, 2010-2013

//#include "snes.h"
//#include "ppu.h"
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

// Myth-related variables
u8  cardModel, cpID;
u8  gameMode;
u8  romSize, romRunMode=1, sramSize, sramBank, sramMode;
u8  extDsp, extSram;
u16 idType = 0;
u16 neo_mode = 0;
u16 romAddressPins;
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

const char * const romDumpNames[] = {
	"ROM256K.SMC","ROM512K.SMC","ROM1M.SMC",
	"ROM2M.SMC",  "ROM4M.SMC",  "ROM8M.SMC",
	"ROM16M.SMC", "ROM32M.SMC", "ROM64M.SMC"
};

const char * const ramDumpNames[] = {
	"RAM16K.SRM", "RAM32K.SRM","RAM64K.SRM",
	"RAM128K.SRM","RAM256K.SRM"
};

///////////// DEBUG ///////////////////
extern unsigned char pkt[6];
extern u8            diskioTemp[8];
u8                   cmdBitsWritten[16];
u8                   cmdBitsRead[16];
u8                   asicCommands[16];
extern unsigned char pfmountbuf[36];
unsigned char        pfMountFmt;
extern unsigned long long sec_tags[16];
extern unsigned char sec_cache[16*512 + 8];
///////////////////////////////////////////////

/**
* Convert 1-bit font data to 4-bit (the three remaining bitplanes are all zeroed).
*/
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
/* Compiler optimization tests
void opt_test3(unsigned int arg)
{
	int j;
	j /= 3;
	j = arg % 10;
}
void opt_test2(unsigned int arg)
{
	int j;
	j *= 7;
	j /= 4;
	j = arg % 16;
}

void opt_test(char *dst, char *src, unsigned int len)
{
	while (len > 0) {
		*dst++ = *src++;
		len--;
	}
}
*/

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
	//else if ((sourceMedium == SOURCE_SD) && force)
	//{
		// This is handled before loading a game in the SD case
	//}
}


/**
* Prints info about the highlighted game (size, type, dsp etc.)
*/
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
	// DEBUG
	/*print_hex(sramMode,4, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
	print_hex(extSram,4, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
	print_hex(extDsp,4, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
	print_hex(sramBank,4, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
	print_hex(romRunMode,7, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
	print_hex(romSize,10, 18, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));

	print_hex(idType>>8, 4, 6, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
	print_hex(idType, 6, 6, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));
	print_hex(hasGbacPsram,10, 6, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE));*/
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


/**
* Display the NUMBER_OF_GAMES_TO_SHOW entries in the game list
* around the currently highlighted game.
*/
void print_games_list()
{
	int i;
	u16 vramOffs = 0x0244;
	u16 attrib;
	static DIR dir;
	char *p;
	void (*psram_read)(char*, u16, u16, u16);
	u16 prbank,proffs;
	DWORD praddr;
	static fileInfoTable_t fit;
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
		MAKE_RAM_FPTR(psram_read, neo2_myth_psram_read);
		praddr = gamesList.firstShown;
		praddr <<= 6;
		proffs = praddr & 0xFFFF;
		prbank = praddr >> 16;
		prbank += 0x20;
		for (i = 0; ((i < NUMBER_OF_GAMES_TO_SHOW) && (i < gamesList.count)); )
		{
			psram_read((char*)&fit, prbank, proffs, 64);
			proffs += 64;
			if (proffs == 0) prbank++;
			set_printxy_clip_rect(2,0,28,10+i);
			p = fit.sfn; //&sdFileInfo.fname[0];
			attrib = TILE_ATTRIBUTE_PAL(SHELL_BGPAL_DARK_OLIVE);
			if (gamesList.highlighted == gamesList.firstShown + i)
			{
				strcpy(highlightedFileName, fit.sfn);
				highlightedFileSize = fit.fsize;
				attrib = TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE);
				if (fit.fattrib & AM_DIR)
				{
					highlightedIsDir = 1;
				}
#ifdef _USE_LFN
				p = fit.lfn; //if (sdFileInfo.lfname[0]) p = sdFileInfo.lfname;
#endif
			}
			printxy("                              ", 2, 9 + i, 0, 28);
			if (fit.fattrib & AM_DIR) //sdFileInfo.fattrib & AM_DIR)
			{
				attrib = (attrib != TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE)) ? TILE_ATTRIBUTE_PAL(SHELL_BGPAL_TOS_GREEN) : attrib;
				print_dir(fit.sfn, //&sdFileInfo.fname[0],
						2, 9 + i,
						attrib,
						28);
			}
			else
			{
				printxy(p,  //&sdFileInfo.fname[0],
						2, 9 + i,
						attrib,
						28);
			}
			i++;
		}
	}
	set_printxy_clip_rect(2,0,28,31);
}


/**
* Show hardware and card revision
*/
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
		//print_hex(useGbacPsram, 16, 24, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
}


/**
* Convert Game Genie code addresses into file offsets based on the ROM layout
* of the game they should be applied to.
*/
void calc_gg_code_addresses()
{
	WORD i;
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
	calc_gg_code_addresses();
	run_game();
}


void run_secondary_cart_c()
{
	void (*run_cart)(void);
  	MAKE_RAM_FPTR(run_cart, run_secondary_cart);
	run_cart();
}


void unzip_vgz()
{
	DWORD (*pInflate)(DWORD, DWORD);
	void (*psram_write)(char*, u16, u16, u16);
	DWORD deflatedDataAddr;
	DWORD unzippedSize;
	WORD i;
	deflatedDataAddr = 0x56000A;

	// Skip gzip embedded string
	if (compressVgmBuffer[3] & 8)
	{
		for (i = 10; i < 64; i++)
		{
			deflatedDataAddr++;
			if (compressVgmBuffer[i] == 0) break;
		}
	}

	printxy("Unzipping..  ", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	update_screen();

	MAKE_RAM_FPTR(pInflate, inflate);
	MAKE_RAM_FPTR(psram_write, neo2_myth_psram_write);
	psram_write((char*)0x7E9000, 0x0F, 0x9000, 0x3000);
	unzippedSize = pInflate(0x500000, deflatedDataAddr);

	printxy("Uncompressed size:", 2, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	print_dec(unzippedSize, 21, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));

	vgzSize = highlightedFileSize;
	highlightedFileSize = unzippedSize;
}


void play_vgm_from_sd_card_c()
{
	static u8   *myth_pram_bio = (u8*)0xC006;
	static WORD prbank, proffs, sects, bytesRead;
	WORD        i, progress;
	long long   (*play_file)(void);
	isVgz = 1;

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
		lastSdParam = tempString;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}

	if ((lastSdError = pf_read(compressVgmBuffer, 64, &bytesRead)) != FR_OK)
	{
		lastSdOperation = SD_OP_READ_FILE;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}

	if ((compressVgmBuffer[0] == 'V') || (compressVgmBuffer[1] == 'g') || (compressVgmBuffer[2] == 'm'))
	{
		isVgz = 0;
	}

	if ((lastSdError = pf_open(tempString)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		lastSdParam = tempString;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}

	clear_status_window();
	hide_games_list();
	if (isVgz)
	{
		printxy("Zipped size:", 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
		print_dec(highlightedFileSize, 15, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	else
	{
		printxy("Uncompressed size:", 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
		print_dec(highlightedFileSize, 21, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}
	update_screen();
	*myth_pram_bio = 0;
	prbank = (isVgz) ? 0x56 : 0x50;
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
	if (isVgz)
	{
		unzip_vgz();
	}
	MAKE_RAM_FPTR(play_file, play_vgm_from_sd_card);
	printxy("Compressing..", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	update_screen();
	compressedVgmSize = play_file();
	printxy("Compressed size:", 2, 9+isVgz, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	print_dec(compressedVgmSize, 21, 9+isVgz, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
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


/**
* Load an SPC file (absolute path in tempString) into PSRAM and
* play it.
*/
void play_spc_from_sd_card_c()
{
	static u8   *myth_pram_bio = (u8*)0xC006;
	static WORD prbank, proffs;
	WORD        i, progress;
	void        (*play_file)(void);

	if ((lastSdError = pf_open(tempString)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		lastSdParam = tempString;
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

	for (i = 0; i < 130; i++)
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


/**
 * Update sramSize and sramMode based on the ROM header and layout.
 */
void get_sram_size_mode(romLayout_t layout)
{
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
		else
		{
			sramMode = 1;
		}
	}
	if (sramMode == 0) sramSize = 0;

	// Check for DSP-1
	if (snesRomInfo[0x16] == 0x04) {
		extDsp = 1;
	} else if (snesRomInfo[0x16] == 0x03) {
		if (snesRomInfo[0x15] != 0x30) {
			if (snesRomInfo[0] == 'P' && snesRomInfo[1] == 'I' &&
			    snesRomInfo[2] == 'L' && snesRomInfo[3] == 'O') {
					// Special case for Pilotwings
					extDsp = 3;
			} else {
				extDsp = 1;
			}
		}
	} else if (snesRomInfo[0x16] == 0x05) {
		extDsp = 1;
	}
	if (extDsp == 1 && layout == LAYOUT_LOROM) {
		extDsp = 4;
	}
}


void get_rom_size(WORD sizeInMbits)
{
	if (sizeInMbits <= 4)
	{
		romSize = 4;
	}
	else if (sizeInMbits <= 8)
	{
		romSize = 8;
	}
	else if (sizeInMbits <= 16)
	{
		romSize = 9;
	}
	else if (sizeInMbits <= 24)
	{
		romSize = 0x0A;
	}
	else if (sizeInMbits <= 32)
	{
		romSize = 0x0B;
	}
	else if (sizeInMbits <= 40)
	{
		romSize = 0x0C;
	}
	else if (sizeInMbits <= 48)
	{
		romSize = 0x0D;
	}
	else if (sizeInMbits <= 64)
	{
		romSize = 0x0E;
	}
	else if (sizeInMbits <= 96)
	{
		romSize = 0x0E;
	}
	else
	{
		// TODO: Treat this as an error?
		romSize = 9;
	}
}


/**
* Dump data to the SD card from the Neo2/3 cart or the boot cart.
*/
void dump_to_sd()
{
	int     i;
	void    (*bootcart_read)(char *, u16, u16, u16);
	DWORD   sectors;
	u16     romBank,romOffs,romOffsReset;
	char    *dumpFileName;

	strcpy(tempString, "/MENU/SNES/DUMPS/");

	if (dumpType == DUMP_ROM)
	{
		dumpFileName = (char*)(romDumpNames[romDumpSize-0x05]);
		strcpy(&tempString[17], dumpFileName);
		tempString[17+strlen(dumpFileName)] = 0;
		if (romDumpSize == 0) return;
		sectors = 1 << (romDumpSize + 1);
		romOffsReset = 0x8000;
		romBank = 0x00; //80;
		if (romDumpLayout & LAYOUT_HIROM)
		{
			romOffsReset = 0;
			romBank = 0xC0;
		}
	}
	else if (dumpType == DUMP_SRAM)
	{
		dumpFileName = (char*)(ramDumpNames[sramDumpSize-1]);
		strcpy(&tempString[17], dumpFileName);
		tempString[17+strlen(dumpFileName)] = 0;
		if (sramDumpSize == 0) return;
		sectors = 1 << (sramDumpSize + 1);
		romOffsReset = 0;
		romBank = 0x70;
		if (sramDumpAddr & LAYOUT_HIROM)
		{
			romBank = 0x20;
		}
	}
	else
	{
		// Neo cart SRAM
		dumpFileName = (char*)(ramDumpNames[neoSramDumpSize-1]);
		strcpy(&tempString[17], dumpFileName);
		tempString[17+strlen(dumpFileName)] = 0;
		if (neoSramDumpSize == 0) return;
		sectors = 1 << (neoSramDumpSize + 1);
		// The neoSramDumpAddr is in 8 kB steps
		romOffs = neoSramDumpAddr;
		romBank = neoSramDumpAddr & 0x18;
		romOffs <<= 13;
		romBank >>= 3;
	}

	clear_status_window();
	hide_games_list();
	update_screen();

	printxy("Opening", 3, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy(dumpFileName, 11, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	update_screen();

	if ((lastSdError = pf_open(tempString)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		lastSdParam = tempString;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}

	printxy("Writing..", 3, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("PLEASE DO NOT REMOVE THE", 3, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("SD CARD OR TURN OFF THE ", 3, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("SNES UNTIL THE DUMP IS  ", 3, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("COMPLETE",                 3, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	update_screen();

	// Write the CRC LUTs to RAM
	for (i=0; i<256; i++)
	{
		sec_buf[i]     = (i & 0x0F) << 4;
		sec_buf[i+256] = (i & 0xF0) >> 4;
	}

	printxy("Dumping   ", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	print_hex(sectors>>8, 11, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(sectors, 13, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	update_screen();

	if ((dumpType == DUMP_ROM) || (dumpType == DUMP_SRAM))
	{
		MAKE_RAM_FPTR(bootcart_read, neo2_myth_bootcart_rom_read);
		romOffs = romOffsReset;
		while (sectors)
		{
			bootcart_read((char*)gbaCardAlphabeticalIdx, romBank, romOffs, 512);
			if ((lastSdError = pf_write(gbaCardAlphabeticalIdx, 512, &i)) != FR_OK)
			{
                lastSdOperation = SD_OP_WRITE_FILE;
				switch_to_menu(MID_SD_ERROR_MENU, 0);
				return;
			}
			sectors--;
			romOffs += 512;
			if (romOffs == 0)
			{
				romBank++;
				romOffs = romOffsReset;
				print_hex(sectors>>8, 11, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
				print_hex(sectors, 13, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
				update_screen();
			}
		}
	}
	else
	{
		// Neo cart SRAM
		MAKE_RAM_FPTR(bootcart_read, neo2_sram_read);
		while (sectors)
		{
			bootcart_read((char*)gbaCardAlphabeticalIdx, romBank, romOffs, 512);
			if ((lastSdError = pf_write(gbaCardAlphabeticalIdx, 512, &i)) != FR_OK)
			{
				lastSdOperation = SD_OP_WRITE_FILE;
				switch_to_menu(MID_SD_ERROR_MENU, 0);
				return;
			}
			sectors--;
			romOffs += 512;
			if (romOffs == 0)
			{
				romBank++;
				romOffs = 0;
				print_hex(sectors>>8, 11, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
				print_hex(sectors, 13, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
				update_screen();
			}
		}
	}
	printxy("Done        ", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("Y: Go back", 3, 22, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	update_screen();
}



void save_last_played_game_info(lastPlayedGame_t *info)
{
	void (*sram_write)(char*, u16, u16, u16);

	MAKE_RAM_FPTR(sram_write, neo2_sram_write);
	info->magic = LAST_GAME_MAGIC;
	sram_write((char*)info, LAST_GAME_SRAM_BANK, LAST_GAME_SRAM_OFFS, sizeof(lastPlayedGame_t));
}


int restore_last_played_game_info(lastPlayedGame_t *info)
{
	void (*sram_read)(char*, u16, u16, u16);

	MAKE_RAM_FPTR(sram_read, neo2_sram_read);
	memset((char*)info, 0, sizeof(lastPlayedGame_t));
	sram_read((char*)info, LAST_GAME_SRAM_BANK, LAST_GAME_SRAM_OFFS, sizeof(lastPlayedGame_t));
	if (info->magic == LAST_GAME_MAGIC)
	{
		return 1;
	}
	return 0;
}


void save_neo_sram_to_sd()
{
	int     i;
	void    (*sram_read)(char *, u16, u16, u16);
	DWORD   sectors;
	u16     ramBank,ramOffs,ramOffsReset;
	char    *dumpFileName;

	if (lastPlayedGame.sramStatus != LAST_GAME_SRAM_BACKUP_PENDING ||
	    lastPlayedGame.sramSize == 0)
	{
		return;
	}

	strcpy(tempString, "/SNES/SAVES/");

	switch (lastPlayedGame.sramSize)
	{
		case 1:	 	// 2 kB
			sectors = 4;
			break;
		case 2:		// 8 kB
			sectors = 16;
			break;
		case 3:		// 32 kB
			sectors = 64;
			break;
		case 4:		// 64 kB
			sectors = 128;
			break;
		case 5:		// 128 kB
			sectors = 256;
			break;
	}

	// Neo cart SRAM
	dumpFileName = (char*)(ramDumpNames[neoSramDumpSize-1]);
	strcpy(&tempString[12], dumpFileName);
	tempString[17+strlen(dumpFileName)] = 0;

	ramOffs = 0;
	ramBank = lastPlayedGame.sramBank;

	clear_status_window();
	hide_games_list();
	update_screen();

	if ((lastSdError = pf_open(tempString)) != FR_OK)
	{
		// Fail silently
		return;
	}

	printxy("Backing up SRAM..", 3, 11, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("PLEASE DO NOT REMOVE THE", 3, 13, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("SD CARD OR TURN OFF THE ", 3, 14, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("SNES UNTIL THE BACKUP IS", 3, 15, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	printxy("COMPLETE",                 3, 16, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	update_screen();

	// Write the CRC LUTs to RAM
	for (i=0; i<256; i++)
	{
		sec_buf[i]     = (i & 0x0F) << 4;
		sec_buf[i+256] = (i & 0xF0) >> 4;
	}

	printxy("Saving   ", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
	print_hex(sectors>>8, 10, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	print_hex(sectors, 12, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	update_screen();

	// Neo cart SRAM
	MAKE_RAM_FPTR(sram_read, neo2_sram_read);
	while (sectors)
	{
		sram_read((char*)gbaCardAlphabeticalIdx, ramBank, ramOffs, 512);
		if ((lastSdError = pf_write(gbaCardAlphabeticalIdx, 512, &i)) != FR_OK)
		{
			lastSdOperation = SD_OP_WRITE_FILE;
			switch_to_menu(MID_SD_ERROR_MENU, 0);
			return;
		}
		sectors--;
		ramOffs += 512;
		if (ramOffs == 0)
		{
			ramBank++;
			ramOffs = 0;
			print_hex(sectors>>8, 11, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			print_hex(sectors, 13, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
			update_screen();
		}
	}


	lastPlayedGame.sramStatus = LAST_GAME_SRAM_BACKUP_DONE;
	save_last_played_game_info(&lastPlayedGame);
	clear_screen();
}



void run_game_from_sd_card_c(int whichGame)
{
	int             i,j;
	void            (*run_game)(void);
	void            (*mirror_psram)(DWORD,DWORD,DWORD);
	DWORD           (*pInflate)(void);
	void            (*psram_write)(char*, u16, u16, u16);
	static WORD     mbits, sects, bytesRead, mythprbank, prbank, proffs, recalcSector;
	static DWORD    unzippedSize;
	static u8       *myth_pram_bio = (u8*)0xC006;
	static char     *fname;
	romLayout_t     layout;

	MAKE_RAM_FPTR(psram_write, neo2_myth_psram_write);

	if (whichGame == HIGHLIGHTED_GAME)
	{
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

		if ((strstri(".SMC", highlightedFileName) > 0) ||
		    (strstri(".SFC", highlightedFileName) > 0) ||
		    (strstri(".BIN", highlightedFileName) > 0))
		{
			gameMode = GAME_MODE_NORMAL_ROM;
			layout = get_rom_info_sd(tempString, snesRomInfo);
			romRunMode = layout ^ 1;
		} else if (strstri(".SPC", highlightedFileName) > 0)
		{
			gameMode = GAME_MODE_SPC;
			play_spc_from_sd_card_c();
			return;
		} else if ((strstri(".VGM", highlightedFileName) > 0) ||
		         (strstri(".VGZ", highlightedFileName) > 0))
		{
			gameMode = GAME_MODE_VGM;
			play_vgm_from_sd_card_c();
			return;
		} else if (strstri(".ZIP", highlightedFileName) > 0)
		{
			gameMode = GAME_MODE_ZIPPED_ROM;
		} else
		{
			clear_status_window();
			printxy("Not a ROM!", 3, 21, 4, 32);
			update_screen();
			return;
		}

		if ((romRunMode > 1) && (gameMode == GAME_MODE_NORMAL_ROM))
		{
			clear_status_window();
			printxy("Unknown ROM type!", 3, 21, 4, 32);
			update_screen();
			return;
		}

		fname = tempString;
		lastPlayedGame.gameMode = gameMode;

	} else if (whichGame == LAST_PLAYED_GAME)
	{
		if (lastPlayedGame.magic != LAST_GAME_MAGIC)
		{
			clear_status_window();
			printxy("Last game unknown", 3, 21, 4, 32);
			update_screen();
			return;
		}
		highlightedFileSize = lastPlayedGame.fsize;
		fname = lastPlayedGame.sfnAbs;
		//printxy(fname, 3, 20, 4, 32);
		//update_screen();
		//while (1) {}
		gameMode = lastPlayedGame.gameMode;
		if (gameMode = GAME_MODE_NORMAL_ROM) {
			layout = get_rom_info_sd(fname, snesRomInfo);
			romRunMode = layout ^ 1;
		}
	} else {
		return;
	}

	mbits = highlightedFileSize >> 17;
	sects = highlightedFileSize >> 9;
	if ((sects & 0xFF) > 1) mbits++;
	get_rom_size(mbits);

	loadProgress[14] = ((mbits+1)/10)+'0';
	loadProgress[15] = ((mbits+1)%10)+'0';

	if ((sects & 0xFF) > 1) mbits--;

	MAKE_RAM_FPTR(run_game, run_game_from_sd_card);

	// These are set by get_sram_size_mode
	extDsp = 0;
	extSram = 0;
	sramSize = 0;
	sramMode = 0;
	sramBank = 0;

	if (sramBankOverride != -1)
	{
		sramBank = sramBankOverride;
	}
	if (gameMode == GAME_MODE_NORMAL_ROM)
	{
		get_sram_size_mode(layout);
		calc_gg_code_addresses();
		if (allowLastGameSave) {
			if (whichGame != LAST_PLAYED_GAME) {
				strcpy(lastPlayedGame.sfnAbs, tempString);
				lastPlayedGame.fsize = highlightedFileSize;
			}
			lastPlayedGame.sramSize = sramSize;
			lastPlayedGame.sramMode = sramMode;
			lastPlayedGame.sramBank = sramBank;
			lastPlayedGame.dspMode = extDsp;
			lastPlayedGame.extSramMode = extSram;
			lastPlayedGame.sramStatus = LAST_GAME_SRAM_BACKUP_PENDING;
			save_last_played_game_info(&lastPlayedGame);
		}
	}

	clear_status_window();
	update_screen();

	if ((lastSdError = pf_open(fname)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		lastSdParam = fname;
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return;
	}

	recalcSector = 1;
	if (gameMode == GAME_MODE_NORMAL_ROM)
	{
		// Skip past the SMC header if present
		if (highlightedFileSize & 0x3FF) {
			if ((lastSdError = pf_lseek(512)) != FR_OK)
			{
				lastSdOperation = SD_OP_SEEK;
				switch_to_menu(MID_SD_ERROR_MENU, 0);
				return;
			}
			sects--;
			// Skip the sector calculation on the first read after the seek
			recalcSector = 0;
		}
		prbank = 0xD0; //0x50;
		mythprbank = 0;
	} else
	{
		prbank = 0x50;
		mythprbank = 1;
	}

	if (gameMode == GAME_MODE_ZIPPED_ROM) {
		if (highlightedFileSize & 0x1FF) sects++;
	}

	proffs = 0;
	i = mbits;

	// Copy some of the code from neo2.asm / diskio_asm.inc to PSRAM
	psram_write((char*)0x7E9000, (mythprbank<<4)|0x0F, 0x9000, 0x3000);
	*myth_pram_bio = mythprbank;
	while (i)
	{
		show_loading_progress();
		if ((lastSdError = pf_read_1mbit_to_psram_asm(prbank, proffs, mythprbank, recalcSector)) != FR_OK)
		{
			lastSdOperation = SD_OP_READ_FILE;
			switch_to_menu(MID_SD_ERROR_MENU, 0);
			return;
		}
		recalcSector = 1;
		sects -= 256;
		prbank += 2;
		if (prbank == 0xE0)
		{
			prbank = 0xD0;
			mythprbank++;
			if (sects) psram_write((char*)0x7E9000, (mythprbank<<4)|0x0F, 0x9000, 0x2800);
			*myth_pram_bio = mythprbank;
		}
		i--;
	}

	// Load any remaining sectors (for ROMs with odd sizes)
	if (sects)
	{
		prbank -= 0x80;
		show_loading_progress();
		// Load any remaining sectors
		for (i = 0; i < sects; i++)
		{
			if ((lastSdError = pf_read_sect_to_psram(prbank, proffs, recalcSector)) != FR_OK)
			{
				lastSdOperation = SD_OP_READ_FILE;
				switch_to_menu(MID_SD_ERROR_MENU, 0);
				return;
			}
			recalcSector = 1;
			proffs += 512;
			if (proffs == 0)
			{
				prbank++;
				if (prbank == 0x60)
				{
					prbank = 0x50;
					mythprbank++;
					*myth_pram_bio = mythprbank;
				}
			}
		}
	}
	if (gameMode == GAME_MODE_ZIPPED_ROM)
	{
		printxy("Unzipping..    ", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
		update_screen();
		MAKE_RAM_FPTR(pInflate, inflate_game);
		unzippedSize = pInflate();
		if (allowLastGameSave)
		{
			strcpy(lastPlayedGame.sfnAbs, tempString);
			lastPlayedGame.fsize = highlightedFileSize;
		}
		highlightedFileSize = unzippedSize;
		mbits = highlightedFileSize >> 17;
		sects = highlightedFileSize >> 9;
		if ((sects & 0xFF) > 1) mbits++;
		get_rom_size(mbits);
		if ((sects & 0xFF) > 1) mbits--;
		layout = get_rom_info_psram(tempString, snesRomInfo);
		romRunMode = layout ^ 1;
		get_sram_size_mode(layout);
		calc_gg_code_addresses();
		if (allowLastGameSave)
		{
			lastPlayedGame.sramSize = sramSize;
			lastPlayedGame.sramMode = sramMode;
			lastPlayedGame.sramBank = sramBank;
			lastPlayedGame.dspMode = extDsp;
			lastPlayedGame.extSramMode = extSram;
			lastPlayedGame.sramStatus = LAST_GAME_SRAM_BACKUP_PENDING;
			save_last_played_game_info(&lastPlayedGame);
		}
		printxy("Unzipped size:", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
		print_dec(unzippedSize, 18, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
	}

	// Special case for 20 Mbit ROMs: mirror the last 4 Mbit
	if (mbits == 20)
	{
		MAKE_RAM_FPTR(mirror_psram, neo2_myth_psram_copy);
		romSize = 0x0B;	// 0xB == 32Mbit
		printxy("Mirroring..     ", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
		update_screen();
		// Mirror it 3 times to fill up 32 Mbit
		mirror_psram(0x780000, 0x700000, 0x080000);
		mirror_psram(0x800000, 0x700000, 0x080000);
		mirror_psram(0x880000, 0x700000, 0x080000);
	}
	// Special case for ROMs <128 kB
	else if (mbits == 0)
	{
	}

	if (doRegionPatch != 0)
	{
		printxy("Fixing region.. ", 3, 21, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE), 32);
		update_screen();
	}

// DEBUG
/*MAKE_RAM_FPTR(psram_read, neo2_myth_psram_read);
psram_read(snesRomInfo, 0x00, 0x8000, 0x20);
print_hex(snesRomInfo[0], 6, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
print_hex(snesRomInfo[1], 8, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
print_hex(snesRomInfo[2], 10, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
print_hex(snesRomInfo[3], 12, 17, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_WHITE));
update_screen();
while (1) {}*/

	if (layout==LAYOUT_HIROM && mbits>32)
	{
		romRunMode = 2;  // ExHIROM
	}

	run_game();
}


romLayout_t get_rom_info_psram(char *fname, u8 *romInfo)
{
	romLayout_t     result = LAYOUT_UNKNOWN;
	DWORD           ofs;
	WORD            bytesRead;
	void            (*psram_read)(char*, u16, u16, u16);
	static u8       buf[0x40];
	MAKE_RAM_FPTR(psram_read, neo2_myth_psram_read);
	psram_read(romInfo, 0x00, 0x7fc0, 0x40);
	// Is the checksum correct?
	if ((((romInfo[0x1c] ^ romInfo[0x1e]) != 0xff) ||
		 ((romInfo[0x1d] ^ romInfo[0x1f]) != 0xff)) &&
		(highlightedFileSize >= 0x10000))
	{
		psram_read(buf, 0x00, 0xffc0, 0x40);
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
	else
	{
		// LoROM
		result = LAYOUT_LOROM;
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


/**
* Get info from a ROM stored on the SD card (LoROM/HiROM + header)
*/
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
/*void print_sd_status()
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
}*/

int has_zipram()
{
	static const char writeData[] = "CAN HAS ZIPRAM? ";
	static char       readData[16];
	void              (*psram_read)(char*, u16, u16, u16);
	void              (*psram_write)(char*, u16, u16, u16);
	int               i;

	MAKE_RAM_FPTR(psram_write, neo2_gbac_psram_write_test_data);
	MAKE_RAM_FPTR(psram_read, neo2_gbac_psram_read);

	psram_write(writeData, 0x42, 0x8000, 16);
	psram_read(readData, 0x42, 0x8000, 16);

	for (i = 0; i < 15; i++)
	{
		if (readData[i] != writeData[i])
		{
			return 0;
		}
	}

	return 1;
}

void setup_video()
{
	int i;
	u8 *bp;
	void (*decrunch)(char*, char*);

	REG_DISPCNT = 0x80;				// Turn screen off

	expand_font_data();
	load_font_colors();
	set_printxy_clip_rect(2, 0, 28, 31);

  	load_cgram(bg_palette, 0, 32);
   	load_vram(&bg_map, 0x3000,(&bg_map_end - &bg_map));

	// Load sprite patterns
	load_vram(&obj_marker, 200*16, 2*32);
	load_vram(&obj_marker+64, 216*16, 2*32);

	// Load the background graphics data
	//MAKE_RAM_FPTR(decrunch, aplib_decrunch);
	//decrunch(&bg_patterns, 0x7f6000);
	//load_vram(0x7f6000, 0x4000, 21152);
	load_vram(&bg_patterns, 0x4000, 21152);

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


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


int main()
{
	u8              *bp;
	int             i;
	u16             keys, keysTemp;
	u16             keysLast;
	u8              keyUpReptDelay;
	u8              keyDownReptDelay;
	void            (*check_gbac_psram)();
	static DIR      dir;
	static FILINFO  fileInfo;

	// Get the CPLD firmware version
	*(u8*)0x00c040 = 0;				// CPLD ROM
	if ((*(u8*)0x00ffe1 == 0x63) &&
	    (*(u8*)0x00ffe2 == 0x57))
	{
		cpID = *(u8*)0x00ffe0;
	}
	*(u8*)0x00c040 = 1;				// GBA card menu

	// ~75-80 kB/s
	MAKE_RAM_FPTR(recv_sd_psram_multi, neo2_recv_sd_psram_multi);

	sdLoaderMemSel  = 0;
	useSdAutoBuffer = 1;

	allowLastGameSave = 1;

	cardModel = *(u8*)0x00fff0;

	copy_ram_code();

	/*MAKE_RAM_FPTR(check_gbac_psram, neo2_check_gbac_psram);
	check_gbac_psram();
	useGbacPsram = hasGbacPsram;*/

	idType       = neo2_read_id();
	hasGbacPsram = has_zipram();

	sramBankOverride = -1;

	setup_video();

	// Mark all Game Genie codes as unused
	for (i = 0; i < MAX_GG_CODES * 2; i++)
    {
		ggCodes[i].used = CODE_UNUSED;
	}

	REG_NMI_TIMEN = 1;

	restore_last_played_game_info(&lastPlayedGame);

	clear_screen();
	update_screen();
	REG_BGCNT = 3;			// Enable BG0 and BG1

	if (set_source_medium(SOURCE_SD, 1) != SOURCE_SD)
	{
		clear_screen();
		switch_to_menu(MID_MAIN_MENU, 0);
		update_screen();
	}

	// Use a faster SD->PSRAM transfer mode if the firmware supports it
	if (cpID >= 4 && useSdAutoBuffer)
	{
		// ~160-175 kB/s
		MAKE_RAM_FPTR(recv_sd_psram_multi, neo2_recv_sd_psram_multi_hwaccel);
	}

	keys = keysLast = 0;
    keyUpReptDelay   = KEY_REPEAT_INITIAL_DELAY;
    keyDownReptDelay = KEY_REPEAT_INITIAL_DELAY;

	while (1)
	{
		if (bg0BufferDirty)
		{
			update_screen();
		}
		keysTemp = read_joypad();
        keys = keysTemp & ~keysLast;
        keysLast = keysTemp;
        if (!(keys & JOY_UP))
        {
        	if (keysLast & JOY_UP)
        	{
            	if (0 == --keyUpReptDelay)
            	{
					wait_nmi();
            	    keyUpReptDelay = KEY_REPEAT_DELAY;
            	    keys |= JOY_UP;
            	}
        	}
        	else
        	{
        	    keyUpReptDelay = KEY_REPEAT_INITIAL_DELAY;
        	}
		}
        if (!(keys & JOY_DOWN))
        {
        	if (keysLast & JOY_DOWN)
        	{
            	if (0 == --keyDownReptDelay)
            	{
					wait_nmi();
            	    keyDownReptDelay = KEY_REPEAT_DELAY;
            	    keys |= JOY_DOWN;
            	}
        	}
        	else
        	{
        	    keyDownReptDelay = KEY_REPEAT_INITIAL_DELAY;
        	}
		}
		keypress_handler(keys);
	}
	return 0;
}

#include "common.h"
#include "diskio.h"
#include "bg_buffer.h"
#include "pff.h"
#include "string.h"
#include "navigation.h"
#include "neo2.h"


FATFS sdFatFs;
DIR sdDir;
FILINFO sdFileInfo;

int lastSdError, lastSdOperation;

#ifdef _USE_LFN
char sdLfnBuf[80];
#endif

char sdRootDir[200] = "/SNES/ROMS";
u16 sdRootDirLength = 10;


int init_sd()
{
	int mountResult = 0;

	cardType = 0;
	sourceMedium = SOURCE_SD;

#ifdef _USE_LFN
	sdFileInfo.lfname = sdLfnBuf;
	sdFileInfo.lfsize = 80;
#endif

	diskio_init();

    if (mountResult = pf_mount(&sdFatFs))
    {
    	cardType = 0x8000;
        if (mountResult = pf_mount(&sdFatFs))
     	{
			sourceMedium = SOURCE_GBAC;
        }
    }

    return mountResult;
}



// Return the number of games stored on the SD card
//
u16 count_games_on_sd_card()
{
	u16 cnt = 0, i = 0;
	static DIR dir;
    u16 prbank, proffs;     // PSRAM bank/offset
    fileInfoTable_t *buf;
    char *fn;
	void (*psram_write)(char*, u16, u16, u16);

	MAKE_RAM_FPTR(psram_write, neo2_myth_psram_write);

   	if (pf_opendir(&dir, sdRootDir) != FR_OK)
   	{
		switch_to_menu(MID_SD_ERROR_MENU, 0);
		return 0;
	}

    // Start writing the file info table at offset 0x200000 in PSRAM
    prbank = 0x20;
    proffs = 0;

    buf = (fileInfoTable_t*)&compressVgmBuffer[0];

	while (cnt != 0xffff)
	{
		if (pf_readdir(&dir, &sdFileInfo) == FR_OK)
		{
			if (dir.sect != 0)
			{
				cnt++;
                fn = sdFileInfo.fname;
#ifdef _USE_LFN
                sdFileInfo.lfname[_MAX_LFN - 1] = 0;
                if (sdFileInfo.lfname[0]) fn = sdFileInfo.lfname;
#endif
                i = strlen(fn);
                if (i > 31) i = 31;
                memcpy(buf->sfn, sdFileInfo.fname, 13);
                memcpy(buf->lfn, fn, i);
                buf->lfn[i] = 0;
                buf->fsize = sdFileInfo.fsize;
                buf->fattrib = sdFileInfo.fattrib;

				if (buf->fattrib & AM_DIR)
				{
					buf->ftype = 0;
				}
                else if ((strstri(".SMC", sdFileInfo.fname) > 0) ||
                         (strstri(".SFC", sdFileInfo.fname) > 0) ||
                         (strstri(".BIN", sdFileInfo.fname) > 0))
                {
                    buf->ftype = GAME_MODE_NORMAL_ROM;
                }
                else if (strstri(".SPC", sdFileInfo.fname) > 0)
                {
                    buf->ftype = GAME_MODE_SPC;
                }
                else if (strstri(".VGM", sdFileInfo.fname) > 0)
                {
                    buf->ftype = GAME_MODE_VGM;
                }
                else if (strstri(".VGZ", sdFileInfo.fname) > 0)
                {
                    buf->ftype = GAME_MODE_VGZ;
                }
                else if (strstri(".ZIP", sdFileInfo.fname) > 0)
                {
                    buf->ftype = GAME_MODE_ZIPPED_ROM;
                }
                else
                {
                    buf->ftype = 0;
                }

                psram_write((char*)buf, prbank, proffs, 64);
                proffs += 64;
                if (proffs == 0) prbank++;

			} else
			{
				break;
			}
		} else
		{
			break;
		}
	}

	return cnt;
}


void read_sd_settings()
{
	char *buf = (char*)&compressVgmBuffer[0];
	WORD bytesRead;
	int pos, i;

	if (pf_open("/MENU/SNES/SETTINGS.TXT") == FR_OK)
	{
		if (pf_read(buf, 256, &bytesRead) == FR_OK)
		{
			buf[256] = 0;

			pos = strstri("START_DIR=", buf);
			if (pos >= 0)
			{
				pos += 10;
				for (i = 0; i < 200; i++)
				{
					if ((i + pos) >= 256) break;
					if (buf[i + pos] == 10 || buf[i + pos] == 13) break;
					sdRootDir[i] = buf[i + pos];
				}
				if ((i > 0) && (sdRootDir[i-1] == '/')) i--;
				sdRootDir[i] = 0;
				sdRootDirLength = i;
			}

			pos = strstri("RESET_MODE=", buf);
			if (pos >= 0)
			{
				if (buf[pos+11] == '0') resetType = 0;
				else if (buf[pos+11] == '1') resetType = 1;
				else if (buf[pos+11] == '2') resetType = 2;
			}

			pos = strstri("REGION_FIX=", buf);
			if (pos >= 0)
			{
				if (buf[pos+11] == '0') doRegionPatch = 0;
				else if (buf[pos+11] == '1') doRegionPatch = 1;
				else if (buf[pos+11] == '2') doRegionPatch = 2;
			}

		}
	}
}


// Change the SD card current directory
//
void change_directory(char *path)
{
	int i,j;

	if (strcmp(path, ".") == 0)
	{
		// Do nothing
		return;
	}

	hide_games_list();

	if (strcmp(path, "..") == 0)
	{
		// Up one level
		if (strlen(sdRootDir) > 1)
		{
			for (i = sdRootDirLength; i >= 0; i--)
			{
				if (sdRootDir[i] == '/') break;
			}
			if (i > 0) sdRootDir[i] = 0; else sdRootDir[1] = 0;
		}
		else return;
	}
	else if (path[0] != '/')	// Relative path
	{
		if (sdRootDirLength > 1)
		{
			sdRootDir[sdRootDirLength] = '/';
			sdRootDirLength++;
		}
		strcpy(&sdRootDir[sdRootDirLength], path);
		sdRootDir[sdRootDirLength + strlen(path)] = 0;
	}
	else						// Absolute path
	{
		strcpy(&sdRootDir[0], path);
		sdRootDir[strlen(path)] = 0;
	}

	sdRootDirLength = strlen(sdRootDir);

	printxy("Opening", 2, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
	printxy(sdRootDir, 10, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
	lastSdOperation = SD_OP_OPEN_DIR;
	if ((lastSdError = pf_opendir(&sdDir, sdRootDir)) == FR_OK)
	{
		printxy("Counting games", 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
		update_screen();
		gamesList.count = count_games_on_sd_card();
		gamesList.firstShown = gamesList.highlighted = 0;
		MS4[0xd] = '1'; MS4[0xc] = MS4[0xb] = '0';	// Reset the "Game (001)" string
		clear_screen();
		switch_to_menu(MID_MAIN_MENU, 0);
	}
	else
	{
		switch_to_menu(MID_SD_ERROR_MENU, 0);
	}
}


// Change source medium from GBAC to SD or vice versa
//
sourceMedium_t set_source_medium(sourceMedium_t newSource)
{
	static int firstSdMount = 1;

	if (newSource == SOURCE_SD)
	{
		lastSdOperation = SD_OP_MOUNT;
		hide_games_list();
		printxy("Mounting SD card", 2, 8, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
		update_screen();
		if ((lastSdError = init_sd()) == FR_OK)
		{
			if (firstSdMount)
			{
				read_sd_settings();
			}
			printxy("Opening ", 2, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			printxy(sdRootDir, 10, 9, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
			update_screen();
			lastSdOperation = SD_OP_OPEN_DIR;
			if ((lastSdError = pf_opendir(&sdDir, sdRootDir)) == FR_OK)
			{
				printxy("Counting games", 2, 10, TILE_ATTRIBUTE_PAL(SHELL_BGPAL_OLIVE), 21);
				update_screen();
				gamesList.count = count_games_on_sd_card();
				gamesList.firstShown = gamesList.highlighted = 0;

				MS4[0xd] = '1'; MS4[0xc] = MS4[0xb] = '0';	// Reset the "Game (001)" string
				clear_screen();
				switch_to_menu(MID_MAIN_MENU, 0);
				firstSdMount = 0;
			}
			else
			{
				switch_to_menu(MID_SD_ERROR_MENU, 0);
			}
		}
		else
		{
			switch_to_menu(MID_SD_ERROR_MENU, 0);
		}
	}
	else if (newSource == SOURCE_GBAC)
	{
		sourceMedium = SOURCE_GBAC;
		update_screen();
		gamesList.count = count_games_on_gba_card();
		gamesList.firstShown = gamesList.highlighted = 0;
		MS4[0xd] = '1'; MS4[0xc] = MS4[0xb] = '0';
		clear_screen();
		switch_to_menu(MID_MAIN_MENU, 0);
	}

	return sourceMedium;
}


/*
 * SD utility functions for the SMS Myth menu
 */
#include "shared.h"
#include "sd_utils.h"
#include "pff_map.h"
#include "pff.h"
#include "neo2_map.h"
#include "util.h"
#include "vdp.h"

extern FATFS sdFatFs;   
extern DIR sdDir,cdDir;
extern FILINFO sdFileInfo;
extern int lastSdError, lastSdOperation;
extern char sdRootDir[100];
extern uint16_t sdRootDirLength;
extern unsigned char pfmountbuf[36];
extern WCHAR LfnBuf[_MAX_LFN + 1];
extern void cls();
extern void print_hex(BYTE val, BYTE x, BYTE y);


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
	char c,d;

	len1 = strlen_asm(lookFor);
	len2 = strlen_asm(lookIn);

	if (len2 >= len1)
	{
		for (i = 0; i < len2; i++)
		{
			for (j = 0; j < len1; j++)
			{
				if (i+j >= len2)
				{
					break;
				} else 
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

int strcmp(char *a, char *b)
{
    int res,i;
    for (i=0; ;i++)
    {
        res = a[i] - b[i];
        if (res) break;
        if (a[i] == 0) break;
    }
    return res;
}

// Return the number of games stored on the SD card
//
uint16_t count_games_on_sd_card()
{
	uint16_t cnt = 0, i = 0, j;
    uint16_t prbank, proffs;     // PSRAM bank/offset
    FileInfoEntry *buf;
    char *fn;
    FRESULT (*p_pf_opendir)(DIR*, const char*) = pfn_pf_opendir;
    FRESULT (*p_pf_readdir)(DIR*, FILINFO*) = pfn_pf_readdir;
    
   	if (p_pf_opendir(&cdDir, sdRootDir) != FR_OK)
   	{
        // ToDo: Display error?
        return 0;
	}

    // Start writing the file info table at offset 0x200000 in PSRAM
    prbank = 0x20;
    proffs = 0x0000;

    vdp_wait_vblank();
    puts("Getting file info..", 3, 10, PALETTE1);
    
    buf = (FileInfoEntry*)0xD700;

	while (cnt < 1024)
	{
		if (p_pf_readdir(&cdDir, &sdFileInfo) == FR_OK)
		{
			if (cdDir.sect != 0)
			{
				cnt++;
                fn = sdFileInfo.fname;
#ifdef _USE_LFN
                sdFileInfo.lfname[_MAX_LFN - 1] = 0;
                if (sdFileInfo.lfname[0]) fn = sdFileInfo.lfname;
#endif
                i = strlen_asm(fn);
                if (i > 31) i = 31;
                for (j=0; j<i; j++) buf->lfn[j] = fn[j];    //memcpy_asm(buf->lfn, fn, 31);  // SDCC doesn't like this memcpy
                memcpy_asm(buf->sfn, sdFileInfo.fname, 13);
                buf->lfn[i] = 0;
                buf->fsize = sdFileInfo.fsize;
                buf->fattrib = sdFileInfo.fattrib;

                if ((strstri(".SMS", sdFileInfo.fname) > 0) ||
                    (strstri(".SG", sdFileInfo.fname) > 0) ||
                    (strstri(".BIN", sdFileInfo.fname) > 0))
                {
                    buf->ftype = GAME_MODE_NORMAL_ROM;
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

                pfn_neo2_ram_to_psram(prbank, proffs, (BYTE *)buf, 64);

                proffs += 64;
                if (proffs == 0)
                    prbank++;
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

// Change the SD card current directory
//
void change_directory(char *path)
{
	int i;
    FRESULT (*p_pf_opendir)(DIR*, const char*) = pfn_pf_opendir;

    Frame2 = BANK_PFF;
    
	if (strcmp(path, ".") == 0)
	{
		// Do nothing
		return;
	}

    cls();
    puts("Changing dir..", 3, 9, PALETTE1);

	if (strcmp(path, "..") == 0)
	{
		// Up one level
		if (strlen_asm(sdRootDir) > 1)
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
		strcpy_asm(&sdRootDir[sdRootDirLength], path);
		sdRootDir[sdRootDirLength + strlen_asm(path)] = 0;
	}
	else						// Absolute path
	{
		strcpy_asm(&sdRootDir[0], path);
		sdRootDir[strlen_asm(path)] = 0;
	}
    
	sdRootDirLength = strlen_asm(sdRootDir);
 
	lastSdOperation = SD_OP_OPEN_DIR;
  
	if ((lastSdError = p_pf_opendir(&sdDir, sdRootDir)) == FR_OK)
	{
		
        games.count = count_games_on_sd_card();
		games.firstShown = games.highlighted = 0;
	}
	else
	{
        // ToDo: Display error?
	}
}


void read_file_to_psram(FileInfoEntry *fi, BYTE prbank, WORD proffs)
{
    int sectorsPerUpdate, sectorsToNextUpdate;
    WORD sectorsInFile;
    WORD sectorsToRead;
    BYTE dotPos = 10;
    char *fullPath = (char *)0xDD00;    // Note: hardcoded
    FRESULT fr;
   
    Frame2 = BANK_PFF;
   
    sectorsInFile = fi->fsize >> 9;
    if (fi->fsize & 511) sectorsInFile++;
    
	strcpy_asm(fullPath, sdRootDir);
	if (sdRootDirLength > 1)
	{
		strcpy_asm(&fullPath[sdRootDirLength], "/");
		strcpy_asm(&fullPath[sdRootDirLength+1], highlightedFileName);
		fullPath[sdRootDirLength+1+strlen_asm(highlightedFileName)] = 0;
	}
	else
	{
		strcpy_asm(&fullPath[sdRootDirLength], highlightedFileName);
		fullPath[sdRootDirLength+strlen_asm(highlightedFileName)] = 0;
	}
 
    cls();
    puts("Opening ", 3, 9, PALETTE1);
    puts(highlightedFileName, 11, 9, PALETTE1);

	if ((lastSdError = pfn_pf_open(fullPath)) != FR_OK)
	{
		lastSdOperation = SD_OP_OPEN_FILE;
		return;
	}

	if ((GAME_MODE_NORMAL_ROM == fi->ftype) && ((fi->fsize & 0x3FF) == 0x200))
	{
		// strip header
		pfn_pf_read_sector(0xdb00);
		sectorsInFile--;
	}

    sectorsPerUpdate = sectorsInFile >> 3;
    sectorsToNextUpdate = sectorsPerUpdate;;
            
    puts("Reading", 3, 10, PALETTE1);
    while (sectorsInFile)
    {
        //pfn_pf_read_sector(0xDA08); // Note: hardcoded
        //pfn_neo2_ram_to_psram(prbank, proffs, 0xDA08, 512);
        
        sectorsToRead = 16;  // try to read at most 16 sectors at a time
        
        if (sectorsToRead > sectorsInFile)
            sectorsToRead = sectorsInFile;
        
        fr = pfn_pf_read_sectors(proffs, (WORD)prbank, sectorsToRead);

        if (fr != FR_OK)
            sectorsToRead = 0;
        
        proffs += sectorsToRead << 9;
        if (proffs == 0)
            prbank++;
              
        sectorsToNextUpdate -= sectorsToRead;
        if (sectorsToNextUpdate <= 0)
        {
            sectorsToNextUpdate += sectorsPerUpdate;
            puts(".", dotPos++, 10, PALETTE1);
        }
        sectorsInFile -= sectorsToRead;
    }
}

int init_sd()
{
	int mountResult = 0;

	cardType = 0;
    neoMode = 0x480;
	menu_state = MENU_STATE_GAME_SD;
    Frame2 = BANK_PFF;
    
#ifdef _USE_LFN
	sdFileInfo.lfname = (char*)LfnBuf;
	sdFileInfo.lfsize = _MAX_LFN;
#endif

    lastSdOperation = SD_OP_MOUNT;

    cls();
    puts("Mounting SD card..", 3, 9, PALETTE1);
    
    mountResult = pfn_pf_mount(&sdFatFs);
    if (mountResult)
    {
    	cardType = 0x8000;
        mountResult = pfn_pf_mount(&sdFatFs);
        if (mountResult)
     	{
			menu_state = MENU_STATE_GAME_GBAC;
            neoMode = 0;
        }
    }

    if (mountResult == FR_OK)
    {
        change_directory("/");
    }
    
    return mountResult;
}
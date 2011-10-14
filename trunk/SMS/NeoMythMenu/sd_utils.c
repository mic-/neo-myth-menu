#include "shared.h"
#include "sd_utils.h"
#include "pff_map.h"
#include "pff.h"
#include "neo2_map.h"
#include "util.h"
#include "vdp.h"

extern FATFS sdFatFs;   
extern DIR sdDir;
extern FILINFO sdFileInfo;
extern int lastSdError, lastSdOperation;
extern char sdRootDir[100];
extern uint16_t sdRootDirLength;
extern void cls();


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
	uint16_t cnt = 0, i = 0;
	DIR dir;
    uint16_t prbank, proffs;     // PSRAM bank/offset
    FileInfoEntry *buf;
    char *fn;
    FRESULT (*p_pf_opendir)(DIR*, const char*) = pfn_pf_opendir;
    FRESULT (*p_pf_readdir)(DIR*, FILINFO*) = pfn_pf_readdir;
    
   	if (p_pf_opendir(&dir, sdRootDir) != FR_OK)
   	{
        // ToDo: Display error?
        return 0;
	}

    // Start writing the file info table at offset 0x200000 in PSRAM
    prbank = 0; //0x20;
    proffs = 0x0000;

    puts("Getting file info..", 3, 10, PALETTE0);
    
    buf = (FileInfoEntry*)0xD600;

	while (cnt != 0xFFFF)
	{
		if (p_pf_readdir(&dir, &sdFileInfo) == FR_OK)
		{
			if (dir.sect != 0)
			{
				cnt++;
                //fn = sdFileInfo.fname;
/*#ifdef _USE_LFN
                sdFileInfo.lfname[_MAX_LFN - 1] = 0;
                if (sdFileInfo.lfname[0]) fn = sdFileInfo.lfname;
#endif*/
                //i = strlen_asm(fn);
                //if (i > 31) i = 31;
                memcpy_asm(buf->sfn, sdFileInfo.fname, 13);
                //memcpy_asm(buf->lfn, fn, i);
                buf->lfn[0] = 0; //buf->lfn[i] = 0;
                buf->fsize = sdFileInfo.fsize;
                buf->fattrib = sdFileInfo.fattrib;

                if ((strstri(".SMS", sdFileInfo.fname) > 0) ||
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
                if (proffs == 0x4000)
                {
                    prbank++;
                    proffs = 0;
                }
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
	int i,j;
    FRESULT (*p_pf_opendir)(DIR*, const char*) = pfn_pf_opendir;

	if (strcmp(path, ".") == 0)
	{
		// Do nothing
		return;
	}

    cls();
    puts("Changing dir..", 3, 9, PALETTE0);

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


int init_sd()
{
	int mountResult = 0;

	cardType = 0;
    neoMode = 0x480;
	menu_state = MENU_STATE_GAME_SD;

/*#ifdef _USE_LFN
	sdFileInfo.lfname = sdLfnBuf;
	sdFileInfo.lfsize = 80;
#endif*/

    lastSdOperation = SD_OP_MOUNT;

    cls();
    puts("Mounting SD card..", 3, 9, PALETTE0);
    
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
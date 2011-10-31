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
extern void puts_active_list();

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
    
    buf = (FileInfoEntry*)0xDA20;

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

                if (sdFileInfo.fattrib & AM_DIR)
                {
                    buf->ftype = 0;
                }
                else if ((strstri(".SMS", sdFileInfo.fname) > 0) ||
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
   /*
	int sectorsPerUpdate, sectorsToNextUpdate;
	WORD sectorsToRead;
	FRESULT fr;
	BYTE dotPos = 10;
	*/
    WORD sectorsInFile;
    char *fullPath = (char *)0xDD00;    // Note: hardcoded
    
   
    Frame2 = BANK_PFF;
   
    sectorsInFile = fi->fsize >> 9;
    if (fi->fsize & 511){ sectorsInFile++; }
    
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
		pfn_pf_read_sectors(0,0,1);
		sectorsInFile--;
	}
 
    puts("Reading...", 3, 10, PALETTE1);
	pfn_pf_read_sectors(proffs, (WORD)prbank, sectorsInFile);
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

void sdutils_sram_cls()
{
	BYTE* p = (BYTE*)0xdb00;
	BYTE blocks;
	WORD addr;
	WORD base;

	cls();
	puts("CLEARING SRAM", 8, 9, PALETTE1); vdp_wait_vblank();
	puts("Working", 3, 11, PALETTE1); vdp_wait_vblank();
	blocks = 0;
	addr = 0;
	base = 0;	
	memset_asm(p,0,512);

	while(blocks < 16)
	{
		++blocks;
		puts(".", 10+(blocks>>1), 11, PALETTE1);vdp_wait_vblank();
		pfn_neo2_ram_to_sram(0x00,base+addr,p,512);
		addr += 512;
	}

	cls();
	if(menu_state == MENU_STATE_GAME_SD){Frame2 = BANK_PFF;}
}

void sdutils_sd_to_sram(const char* filename)
{
	FRESULT (*f_write)(void*) = pfn_pf_write_sector;
	void (*grab_fs)(FATFS**) = pfn_pf_grab;
	FRESULT (*f_open)(const char*) = pfn_pf_open;
	BYTE* p = (BYTE*)0xdb00;
	FATFS* fs;
	BYTE blocks;
	WORD addr;
	WORD base;

	Frame2 = BANK_PFF;
	grab_fs(&fs);
	if(!fs){return;}

	if(f_open(filename) != FR_OK){return;}

	cls();
	puts("RESTORING SRAM", 8, 9, PALETTE1); vdp_wait_vblank();
	puts("Working", 3, 11, PALETTE1); vdp_wait_vblank();

	blocks = 0;
	addr = 0;
	base = 0;

	while(blocks < 16)
	{
		++blocks;
		puts(".", 10+(blocks>>1), 11, PALETTE1);vdp_wait_vblank();
		Frame2 = BANK_PFF;
		pfn_pf_read_sectors(0,0,1);
		pfn_neo2_ram_to_sram(0x00,base+addr,p,512);
		addr += 512;
	}

	sdutils_load_cfg();
	p[1] = 0x01;
	addr = strlen_asm(filename);
	p[256]= addr & 0xff;
	strcpy_asm(&p[257],filename);
	p[257+addr-1]='\0';
	sdutils_save_cfg();

	Frame2 = BANK_PFF;
	cls();
	change_directory("/");
	puts_active_list();
}

void sdutils_sram_to_sd(const char* filename)
{
	FRESULT (*f_write)(void*) = pfn_pf_write_sector;
	void (*grab_fs)(FATFS**) = pfn_pf_grab;
	FRESULT (*f_open)(const char*) = pfn_pf_open;
	BYTE* p = (BYTE*)0xdb00;
	FATFS* fs;
	BYTE blocks;
	WORD addr;
	WORD base;

	Frame2 = BANK_PFF;
	grab_fs(&fs);
	if(!fs){return;}
	if(f_open(filename) != FR_OK){return;}

	cls();
	puts("DUMPING SRAM", 8, 9, PALETTE1); vdp_wait_vblank();
	puts("Working", 3, 11, PALETTE1); vdp_wait_vblank();

	blocks = 0;
	addr = 0;
	base = 0;

	while(blocks < 16)
	{
		++blocks;
		puts(".", 10+(blocks>>1), 11, PALETTE1);vdp_wait_vblank();
		pfn_neo2_sram_to_ram(p, 0x00, base + addr,512);
		Frame2 = BANK_PFF;
		f_write(p);
		addr += 512;
	}

	cls();
	change_directory("/");
	puts_active_list();
}

/*
	MYTH.CFG 2x blocks of 256Bytes
	BLOCK #0
	$0000:1 : First run flag (1 = Yes/0 = No)
	$0001:1 : Save flag (1 = Backup game on next boot / = 0 Do nothing)
	$0002:1 : Reset flag (1 = Reset to Menu / 0 = Reset to Game)
	$0003:1 : FM Flag (1 = On / 0 = Off)
	$0004:1 : Dev Build (1 = Yes / 0 = No) (Use this if you need conditional debugging in final builds)

	BLOCK #1
	$0100:1 : If save flag is set , this byte represents the filename length of the last loaded game
	$0101:N : The next N bytes specified at $0100 make up the filename of the last loaded rom (MUST be null terminated)
*/
void sdutils_load_cfg()
{
	void (*grab_fs)(FATFS**) = pfn_pf_grab;
	FRESULT (*f_open)(const char*) = pfn_pf_open;
	BYTE* cfg = (BYTE*)0xdb00;
	FATFS* fs;
	WORD i;

	Frame2 = BANK_PFF;
	grab_fs(&fs);
	if(!fs){return;}
	if(f_open("/menu/sms/MYTH.CFG") != FR_OK){return;}

	cls();
	puts("Reading MYTH.CFG...", 8, 10, PALETTE1); vdp_wait_vblank();

	//Read one sector so that the data appear @ 0xdb00
	pfn_pf_read_sectors(0,0,1);

	options_count = 0;
    fm_enabled_option_idx = options_count;
    options_add("FM : ","off","on",OPTION_TYPE_SETTING,cfg[3]);
    reset_to_menu_option_idx = options_count;
    options_add("Reset to menu : ","off","on",OPTION_TYPE_SETTING,cfg[2]);

	if(cfg[1])
	{
		//Save sram to sd
		strcpy_asm(&cfg[256+128],"/menu/sms/save/");
		strncat_asm(&cfg[256],&cfg[256+128+15],cfg[256]);
		i = 256+128+15+cfg[256];

		while(i > 256 + 128)
		{
			if(cfg[i] == '.')
			{
				strncat_asm(&cfg[i],".SAV",4);
				i = 1;
				break;
			}
			--i;
		}

		if(i != 1){strncat_asm(&cfg[256+128+15+cfg[256]],".SAV",4);}

		sdutils_sram_to_sd(&cfg[256+128]);

		//Turn OFF SRAM manager
		cfg[1] = 0x00;
		sdutils_save_cfg();
	}
	cls();
	puts_active_list();
}

void sdutils_save_cfg()
{
	FRESULT (*f_write)(void*) = pfn_pf_write_sector;
	void (*grab_fs)(FATFS**) = pfn_pf_grab;
	FRESULT (*f_open)(const char*) = pfn_pf_open;
	BYTE* p = (BYTE*)0xdb00;
	FATFS* fs;

	Frame2 = BANK_PFF;
	grab_fs(&fs);
	if(!fs){return;}
	if(f_open("/menu/sms/MYTH.CFG") != FR_OK){return;}
	cls();
	puts("Writing MYTH.CFG...", 38, 10, PALETTE1); vdp_wait_vblank();
	f_write(p);

	cls();
	puts_active_list();
}

#if 0
void sdutils_import_ips(const char* filename)
{
	DWORD save;
	int written;
	int size;
	unsigned char prbank,a;
	WORD wr,proffs,len,step;
	unsigned char* buf;
	unsigned char c;
	FATFS (*grab_fs)(void) = pfn_pf_grab;
	FRESULT (*f_open)(const char*) = pfn_pf_open;
	FATFS* fs;

	fs = grab_fs();
	if(!fs){return;}
	if(f_open(filename) != FR_OK){return;}

	buf = (unsigned char*)0xDB00;
	size = fs->fsize - 8; /*patch + eof*/
	written = 0;
	prbank = 0;
	proffs = 0;
	save = fs->fptr;		/*patch*/
	pf_read(buf,5,&wr);
	pf_lseek(save + 5);

	while(written < size)
	{
		/*
			This is SLOW actually.For every N bytes a whole sector is read
			A better solution would be to copy the WHOLE patch to some psram offset and then proccess it from there
		*/
		save = fs->fptr;
		pf_read(buf,5,&wr);
		pf_lseek(save + 5);
		written += 5;
	
		prbank = buf[2];
		#if 0
		proffs = buf[1] << 8;
		proffs |= buf[0];
		len = buf[4] << 8;
		len |= buf[3];
		#else
		proffs = *(WORD*)&buf[0];
		len = *(WORD*)&buf[2];
		#endif

		if(len)
		{
			written += len;
			pfn_neo2_ram_to_psram(prbank,proffs,(BYTE*)buf,len);
			continue;
		}
		else	/*run length encoded*/
		{
			save = fs->fptr;
			pf_read(buf,3,&wr);
			pf_lseek(save + 3);
			written += 3;

			#if 0
			len = buf[1] << 8;
			len |= buf[0];
			#else
			len = *(WORD*)&buf[0];
			#endif

			c = buf[2];
			memset_asm(buf,c,(len >= 128) ? 128 : len);/*largest step*/
			while(len > 0)
			{
				step = (len > 128) ? 128 : len;
				len -= step;

				if(step&1)
				{
					while(step > 0)
					{
						if( (!(step&1))){goto patch_ips_apply_aligned_block_found;}
						pfn_neo2_ram_to_psram(prbank,proffs,(BYTE*)buf,1);
						step--;
						proffs++;
						if(0==proffs){prbank++;}
					}
				}
				else
				{
					patch_ips_apply_aligned_block_found:
					pfn_neo2_ram_to_psram(prbank,proffs,(BYTE*)buf,step);
					proffs += step;
					if(0==proffs){prbank++;}
				}
			}
		}
	}
}
#endif



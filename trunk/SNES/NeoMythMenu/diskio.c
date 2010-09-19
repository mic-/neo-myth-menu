#include <string.h>
#include <stdio.h>

#include "diskio.h"
#include "snes.h"
#include "common.h"


typedef volatile unsigned short int vu16;


/* global variables and arrays */

u32 sec_last;

unsigned short cardType;                /* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */

unsigned char pkt[6];                    /* command packet */

unsigned long long num_sectors;

#define CACHE_SIZE 16                    /* number sectors in cache */
unsigned int sec_tags[CACHE_SIZE];
unsigned char sec_cache[CACHE_SIZE*512 + 8];
unsigned char sec_buf[520]; /* for uncached reads */

#define R1_LEN (48/8)
#define    R2_LEN (136/8)
#define R3_LEN (48/8)
#define R6_LEN (48/8)
#define R7_LEN (48/8)

unsigned char sd_csd[R2_LEN];


u8 diskioCrcbuf[8];
u8 diskioPacket[7];
u8 diskioResp[R2_LEN];
u16 diskioRegBackup[6];
u8 diskioTemp[8];


DSTATUS (*disk_initialize) (void);
DRESULT (*disk_read) (BYTE*, DWORD, BYTE);
DRESULT (*disk_readp) (void* dest, DWORD sector, WORD sofs, WORD count);

extern DSTATUS disk_initialize_asm(void);
extern DRESULT disk_read_asm(BYTE*, DWORD, BYTE);
extern DRESULT disk_readp_asm(void* dest, DWORD sector, WORD sofs, WORD count);



void diskio_init()
{
	MAKE_RAM_FPTR(disk_initialize, disk_initialize_asm);
	MAKE_RAM_FPTR(disk_read, disk_read_asm);
	MAKE_RAM_FPTR(disk_readp, disk_readp_asm);
}


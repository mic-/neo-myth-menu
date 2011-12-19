/*
 * DiskIO helper routines for the SMS Myth menu
 * Acts as a bridge between pff and diskio_asm
 */
#include "diskio.h"
#include "neo2.h"
#include "sms.h"

// External code (found in diskio_asm.s)
extern DSTATUS disk_initialize2(void);
extern DRESULT disk_readp2(void* dest, DWORD sector, WORD sofs, WORD count);
extern DRESULT disk_read_sector2(void* dest, DWORD sector);
extern DRESULT disk_read_sectors2(WORD destLo, DWORD sector, WORD destHi, WORD count);
extern DRESULT sdWriteSingleBlock(void* src,DWORD addr);

void diskio_bridge_prologue()
{
   __asm
    di
    __endasm;

    // neo2_disable_sram essentially serves as neo2_select_menu,
    // it's just used to enable the SD interface
    neo2_disable_sram();
}

void diskio_bridge_epilogue()
{
    Frame2 = 5;

    __asm
    ei
    __endasm;
}

DSTATUS disk_initialize(void)
{
    DSTATUS res;

	diskio_bridge_prologue();
    res = disk_initialize2();
	diskio_bridge_epilogue();

    return res;    
}

DRESULT disk_readp(void* dest, DWORD sector, WORD sofs, WORD count)
{
    DSTATUS res;
   
	diskio_bridge_prologue();;
    res = disk_readp2(dest, sector, sofs, count);
	diskio_bridge_epilogue();

    return res;        
}

DRESULT disk_read_sectors(WORD destLo, DWORD sector, WORD destHi, WORD count)
{
    DSTATUS res;
 
   __asm
    di
    __endasm;
    neo2_enable_psram();

    res = disk_read_sectors2(destLo, sector, destHi, count);
    
    neo2_disable_psram();
	diskio_bridge_epilogue();

    return res;        
}

DRESULT disk_read_sector(void* dest, DWORD sector)
{
    DSTATUS res;

	diskio_bridge_prologue();
    res = disk_read_sector2(dest, sector);
	diskio_bridge_epilogue();

    return res;        
}

DRESULT disk_writep ( //Writes a sector a time
	BYTE* buff,		
	DWORD sc		
)
{
	DRESULT res;

	diskio_bridge_prologue();
	res = sdWriteSingleBlock(buff,sc);
	diskio_bridge_epilogue();

	return res;
}


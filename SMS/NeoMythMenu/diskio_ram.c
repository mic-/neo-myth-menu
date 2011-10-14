#include "diskio.h"
#include "neo2.h"
#include "sms.h"

// External code (found in diskio_asm.s)
extern DSTATUS disk_initialize2(void);
extern DRESULT disk_readp2(void* dest, DWORD sector, WORD sofs, WORD count);
extern DRESULT disk_read_sector2(void* dest, DWORD sector);

DSTATUS disk_initialize(void)
{
    DSTATUS res;

   __asm
    di
    __endasm;

    // neo2_disable_sram essentially serves as neo2_select_menu,
    // it's just used to enable the SD interface
    neo2_disable_sram();
    res = disk_initialize2();
    Frame2 = 5;

    __asm
    ei
    __endasm;

    return res;    
}

DRESULT disk_readp(void* dest, DWORD sector, WORD sofs, WORD count)
{
    DSTATUS res;

   __asm
    di
    __endasm;

    neo2_disable_sram();
    res = disk_readp2(dest, sector, sofs, count);
    Frame2 = 5;

    __asm
    ei
    __endasm;

    return res;        
}

DRESULT disk_read_sector(void* dest, DWORD sector)
{
    DSTATUS res;

   __asm
    di
    __endasm;

    neo2_disable_sram();
    res = disk_read_sector2(dest, sector);
    Frame2 = 5;

    __asm
    ei
    __endasm;

    return res;        
}
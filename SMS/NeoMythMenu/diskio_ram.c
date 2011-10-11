#include "diskio.h"
#include "diskio_asm_map.h"
#include "neo2.h"
#include "sms.h"


DSTATUS disk_initialize(void)
{
    DSTATUS res;

   __asm
    di
    __endasm;

    neo2_enable_sram(0x0000);
    res = pfn_disk_initialize();
    neo2_disable_sram();
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

    neo2_enable_sram(0x0000);
    res = pfn_disk_readp(dest, sector, sofs, count);
    neo2_disable_sram();
    Frame2 = 5;

    __asm
    ei
    __endasm;

    return res;        
}
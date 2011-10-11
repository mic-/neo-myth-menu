#ifndef __DISKIO_RAM_H__
#define __DISKIO_RAM_H__

#include "diskio.h"

DSTATUS disk_initialize(void);
DRESULT disk_readp_ram(void* dest, DWORD sector, WORD sofs, WORD count);

#endif


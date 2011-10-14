/*-----------------------------------------------------------------------
/  Low level disk interface modlue include file  R0.07   (C)ChaN, 2009
/-----------------------------------------------------------------------*/

#ifndef _DISKIO

#define _READONLY   0   /* 1: Read-only mode */
#define _USE_IOCTL  1

#include <z80/types.h>
#include <stdint.h>

#define FALSE 0
#define TRUE (~FALSE)


typedef uint32_t DWORD;
typedef int BOOL;

/* Status of Disk Functions */
typedef BYTE    DSTATUS;

/* Results of Disk Functions */
typedef enum {
    RES_OK = 0,     /* 0: Successful */
    RES_ERROR,      /* 1: R/W Error */
    RES_WRPRT,      /* 2: Write Protected */
    RES_NOTRDY,     /* 3: Not Ready */
    RES_PARERR      /* 4: Invalid Parameter */
} DRESULT;


extern unsigned short cardType;
extern uint32_t num_sectors;

/*---------------------------------------*/
/* Prototypes for disk control functions */


BOOL assign_drives (int argc, char *argv[]);
DSTATUS disk_initialize(void);
DRESULT disk_read(BYTE*, DWORD, BYTE);
DRESULT disk_readp(void* dest, DWORD sector, WORD sofs, WORD count);
DRESULT disk_read_sector(void* dest, DWORD sector);

extern void diskio_init();

/* Disk Status Bits (DSTATUS) */

#define STA_NOINIT      0x01    /* Drive not initialized */
#define STA_NODISK      0x02    /* No medium in the drive */
#define STA_PROTECT     0x04    /* Write protected */


/* Command code for disk_ioctrl() */

/* Generic command */
#define CTRL_SYNC           0   /* Mandatory for write functions */
#define GET_SECTOR_COUNT    1   /* Mandatory for only f_mkfs() */
#define GET_SECTOR_SIZE     2
#define GET_BLOCK_SIZE      3   /* Mandatory for only f_mkfs() */
#define CTRL_POWER          4
#define CTRL_LOCK           5
#define CTRL_EJECT          6
/* MMC/SDC command */
#define MMC_GET_TYPE        10
#define MMC_GET_CSD         11
#define MMC_GET_CID         12
#define MMC_GET_OCR         13
#define MMC_GET_SDSTAT      14
/* ATA/CF command */
#define ATA_GET_REV         20
#define ATA_GET_MODEL       21
#define ATA_GET_SN          22


#define _DISKIO
#endif
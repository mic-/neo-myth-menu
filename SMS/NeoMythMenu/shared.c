// Shared data for all banks

#include <stdint.h>
#include "shared.h"
#include "pff.h"

/*
 * These variables need to be in this exact order, because they are
 * accessed using hardcoded addresses
 */
BYTE idLo;          // C001
BYTE idHi;          // C002
WORD neoMode;       // C003
BYTE vdpSpeed;      // C005
// Extra "virtual" registers for assembly routines
BYTE vregs[16];     // C006
WORD cardType;      // C016
/******************************************************************/

// The rest can be in any order..

BYTE reset_to_menu_option_idx;
BYTE fm_enabled_option_idx;
BYTE flash_mem_type;
BYTE hasZipram;
BYTE menu_state;
BYTE sd_fetch_info_timeout;
FileList games;
BYTE region;
BYTE pad, padLast;

BYTE options_count;
BYTE options_highlighted;
Option options[MAX_OPTIONS];

BYTE generic_list_buffer[LIST_BUFFER_SIZE];

BYTE diskioPacket[7];
BYTE diskioResp[17];
BYTE diskioTemp[8];
BYTE sd_csd[17];
uint32_t sec_tags[2];
uint32_t sec_last;
uint32_t numSectors;

FATFS *FatFs;
FATFS sdFatFs;   
WCHAR LfnBuf[_MAX_LFN + 1];
unsigned char pfmountbuf[36];
unsigned char pfMountFmt;
DWORD pffbcs;
WORD pffclst;

DIR sdDir;
FILINFO sdFileInfo;
int lastSdError, lastSdOperation;
/*#ifdef _USE_LFN
char sdLfnBuf[80];
#endif*/
char sdRootDir[100];
uint16_t sdRootDirLength;

// For testing purposes
#ifdef EMULATOR
const char dummyGameList[] =
{
    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    '4', '0', '0', '1', ' ', 'R', '-', 'T',
    'Y', 'P', 'E', '.', 'S', 'M', 'S', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'A', 'l', 'e', 'x', ' ', 'K', 'i', 'd',
    'd', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '1', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '2', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '3', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '4', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '5', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '6', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '7', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '8', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '9', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0x00,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'S', 'o', 'n', 'i', 'c', ' ', '1', '0',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

    0xFF,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'A', 'l', 'e', 'x', ' ', 'K', 'i', 'd',
    'd', ' ', ' ', '.', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0
};

const BYTE *gbacGameList = (char*)&dummyGameList[0];
#else
const BYTE *gbacGameList = (const BYTE*)0xB000;
#endif




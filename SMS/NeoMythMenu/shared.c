// Shared data for all banks

#include <stdint.h>
#include "shared.h"

BYTE idLo,idHi;
WORD neoMode;
BYTE hasZipram;
BYTE menu_state;
BYTE sd_fetch_info_timeout;
FileList games;
BYTE region;
BYTE pad, padLast;
BYTE vdpSpeed;

// Extra "virtual" registers for assembly routines
BYTE vregs[16];

BYTE diskioPacket[7];
BYTE diskioResp[17];
BYTE diskioTemp[8];
BYTE sd_csd[17];
WORD cardType;
uint32_t sec_tags[2];
uint32_t sec_last;
uint32_t numSectors;


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




// Shared data for all banks

#include "shared.h"


int foo, bar = 3;
FileList games;


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
    'd', ' ', ' ', '.', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0,

	0xFF,0x02,0x41,0x00,0x00,0x00,0x0A,0x0B,
    'A', 'l', 'e', 'x', ' ', 'K', 'i', 'd',
    'd', ' ', ' ', '.', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0
};
#endif


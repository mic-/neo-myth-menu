// Game Genie routines for the SNES Myth menu
// Mic, 2010

#include "snes.h"
#include "common.h"
#include "bg_buffer.h"
#include "cheats/cheat.h"

const u8 ggSubstCipher[16] = {0x4, 0x6, 0xD, 0xE,
                              0x2, 0x7, 0x8, 0x3,
                              0xB, 0x5, 0xC, 0x9,
							  0xA, 0x0, 0xF, 0x1};
char ggCodeTemp[9];


// Game Genie code decoder
// Takes a pointer to an array containing a code (e.g. {10,11,0,2,12,1,0,0} would be the code AB02-C100) and
// converts it into an 8-bit value and a 24-bit bank:offset pair.
//
void gg_decode(u8 *ggCode, u8 *bank, u16 *offset, u8* val)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		ggCodeTemp[i] = ggSubstCipher[ggCode[i]];
	}

	*val = (ggCodeTemp[0] << 4) | ggCodeTemp[1];

	for (i = 0; i < 3; i++)
	{
		ggCodeTemp[i] = (ggCodeTemp[i + i + 2] << 4) | ggCodeTemp[i + i + 3];
	}

	*bank = ((ggCodeTemp[1] & 0x3C) << 2) | ((ggCodeTemp[2] & 0x3C) >> 2);
	*offset = ((ggCodeTemp[0] & 0xF0) << 8) | ((ggCodeTemp[2] & 0x03) << 10) | ((ggCodeTemp[1] & 0xC0) << 2);
	*offset |= ((ggCodeTemp[0] & 0x0F) << 4) | ((ggCodeTemp[1] & 0x03) << 2) | ((ggCodeTemp[2] & 0xC0) >> 6);
}



// Print a Game Genie code at a given position on the screen
//
void print_gg_code(ggCode_t *ggCode, u16 x, u16 y, u16 attribs)
{
	int i, j;

	if (ggCode->used)
	{
		j = 0;
		for (i = 0; i < 8; i++)
		{
			ggCodeTemp[j] = ggCode->code[i] + '0';
			if (ggCodeTemp[j] > '9') ggCodeTemp[j] += 7;
			if (++j == 4) ggCodeTemp[j++] = '-';
		}

		printxy(ggCodeTemp, x, y, attribs, 9);
	}
	else
		printxy("____-____", x, y, attribs, 9);
}

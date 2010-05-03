// Action Replay routines for the SNES Myth menu
// Mic, 2010

#include "snes.h"
#include "common.h"

char arCodeTemp[9];


void ar_decode(u8 *arCode, u8 *bank, u16 *offset, u8* val)
{
	*bank = (arCode[0] << 4) | arCode[1];
	*val = (arCode[6] << 4) | arCode[7];
	*offset = (arCode[2] << 12) | (arCode[3] << 8) | (arCode[4] << 4) | arCode[5];
}



// Print an Action Replay code at a given position on the screen
//
void print_ar_code(arCode_t *arCode, u16 x, u16 y, u16 attribs)
{
	int i, j;

	if (arCode->used)
	{
		j = 0;
		for (i = 0; i < 8; i++)
		{
			arCodeTemp[j] = arCode->code[i] + '0';
			if (arCodeTemp[j] > '9') arCodeTemp[j] += 7;
		}

		printxy(arCodeTemp, x, y, attribs, 8);
	}
	else
		printxy("________", x, y, attribs, 8);
}

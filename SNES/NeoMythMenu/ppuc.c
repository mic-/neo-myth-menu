#include "ppu.h"


// Write one or more OBJ descriptors to OAM
void update_oam(oamEntry_t *oamEntries, int first, int numEntries)
{
	int i;

	first <<= 2;

	REG_OAMADDL = (u8)first;
	REG_OAMADDH = 0x80 | ((first >> 8) & 1);

	for (i = 0; i < numEntries; i++)
	{
		REG_OAMDATA = oamEntries->x;
		REG_OAMDATA = oamEntries->y;
		REG_OAMDATA = (u8)(oamEntries->chr);
		REG_OAMDATA = ((oamEntries->chr >> 8) & 1) | (oamEntries->palette << 1) | (oamEntries->prio << 4) | (oamEntries->flip << 6);
		oamEntries++;
	}
}


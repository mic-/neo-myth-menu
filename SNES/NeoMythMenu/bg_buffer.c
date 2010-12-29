// Functions dealing with the SNES Myth shell's BG buffer
// Mic, 2010

#include "common.h"
#include "ppu.h"


// All the text strings are written to this buffer and then sent to VRAM using DMA in order to
// improve performance.
//
static char bg0Buffer[0x800];
u8 bg0BufferDirty;


static rect_t printxyClipRect;


// Print a given metastring. E.g. print_meta_string(3) to print MS3.
//
void print_meta_string(u16 msNum)
{
	int i;
	u8 *pStr = (u8*)metaStrings[msNum];
	u8 attribs;
	u16 vramOffs;

	for (;;)
	{
		if (*pStr == 0)
		{
			// We've reached the null-terminator
			break;
		}
		else if (*pStr == 0xff)
		{
			// The next three bytes contain row, column and palette number
			vramOffs = *(++pStr) << 5;
			vramOffs |= *(++pStr);
			vramOffs <<= 1;
			attribs = *(++pStr) << 2;
		}
		else
		{
			// This byte was a character
			bg0Buffer[vramOffs++] = *pStr;
			bg0Buffer[vramOffs++] = attribs;
		}
		pStr++;
	}
	bg0BufferDirty = 1;
}


// Print a string at position x,y
//
void printxy(char *pStr, u16 x, u16 y, u16 attribs, u16 maxChars)
{
	int i = x;
	u16 printed = 0;
	u16 vramOffs = (y << 6) + x + x;

	for (; y < printxyClipRect.y2;)
	{
		if (*pStr == 0)
		{
			// We've reached the null-terminator
			break;
		}
		bg0Buffer[vramOffs++] = *pStr;
		bg0Buffer[vramOffs++] = attribs;
		pStr++;
		if (++printed >= maxChars) break;
		if (++i > printxyClipRect.x2) // 28
		{
			i = x;
			vramOffs = ((++y) << 6) + x + x;
			while (*pStr == ' ') pStr++;
		}
	}
	bg0BufferDirty = 1;
}


// Print a directory name (enclosed with []) at position x,y
// This function was written to avoid having to do a strlen every
// time a directory name is printed
//
void print_dir(char *pStr, u16 x, u16 y, u16 attribs, u16 maxChars)
{
	int i = x;
	u16 printed = 0;
	u16 vramOffs = (y << 6) + x + x;

	bg0Buffer[vramOffs++] = '[';
	bg0Buffer[vramOffs++] = attribs;
	i++;

	for (; y < printxyClipRect.y2;)
	{
		if (*pStr == 0)
		{
			// We've reached the null-terminator
			break;
		}
		bg0Buffer[vramOffs++] = *pStr;
		bg0Buffer[vramOffs++] = attribs;
		pStr++;
		if (++printed >= maxChars) break;
		if (++i >= printxyClipRect.x2)
		{
			i = x;
			vramOffs = ((++y) << 6) + x + x;
			while (*pStr == ' ') pStr++;
		}
	}

	bg0Buffer[vramOffs++] = ']';
	bg0Buffer[vramOffs++] = attribs;

	bg0BufferDirty = 1;
}



// Mainly for debugging purposes
//
void print_hex(u8 val, u16 x, u16 y, u16 attribs)
{
	u16 vramOffs = (y << 6) + x + x;
	int i;
	u8 b;

	for (i = 0; i < 2; i++)
	{
		b = (val >> 4) + '0';
		if (b > '9') b += 7;
		bg0Buffer[vramOffs++] = b;
		bg0Buffer[vramOffs++] = attribs;
		val <<= 4;
	}
	bg0BufferDirty = 1;
}


// Print a number in decimal base at position x,y
//
void print_dec(DWORD val, u16 x, u16 y, u16 attribs)
{
	static char printDecBuf[32];
	char *p = &printDecBuf[32];
	char c;
	u16 chars = 0;

	do
	{
		*(--p) = (val % 10) + '0';
		chars++;
		val = val / 10;
	} while (val != 0);
	printxy(p, x, y, attribs, chars);
}


// Set the clipping rectangle used by printxy
//
void set_printxy_clip_rect(u16 x1, u16 y1, u16 x2, u16 y2)
{
	printxyClipRect.x1 = x1;
	printxyClipRect.y1 = y1;
	printxyClipRect.x2 = x2;
	printxyClipRect.y2 = y2;
}


void puts_game_title(u16 gameNum, u16 vramOffs, u8 attributes)
{
	u8 *pGame;
	int i;

	if (sortOrder == SORT_LOGICALLY)
	{
		set_full_pointer((void**)&pGame, GAME_LIST_BANK, 0xc800 + (gameNum << 6));
	}
	else
	{
		set_full_pointer((void**)&pGame, GAME_LIST_BANK,
		                 0xc800 + (gbaCardAlphabeticalIdx[gameNum] << 6));
	}

	// Make sure that the chunk begins with 0xff
	//
	if (*pGame == 0xff)
	{
		pGame += 0x0c;		// Add 0x0c to the address, since that's where the actual string is located
		for (i = 0; i < 28; i++)
		{
			if (pGame[i])
			{
				bg0Buffer[vramOffs++] = pGame[i];
				bg0Buffer[vramOffs++] = attributes;
			}
			else
			{
				break;
			}
		}
	}
	bg0BufferDirty = 1;
}


void hide_games_list()
{
	int i;

	// Hide the top and bottom arrows
	print_meta_string(35);
	print_meta_string(36);

	for (i = 0; i < 320; i++)
	{
		bg0Buffer[0x240 + i + i] = ' ';
		bg0Buffer[0x240 + i + i + 1] = 0;
	}
	bg0BufferDirty = 1;
}


void hide_cheat_list()
{
	int i;

	for (i = 0; i < 288; i++)
	{
		bg0Buffer[0x280 + i + i] = ' ';
		bg0Buffer[0x280 + i + i + 1] = 0;
	}
	bg0BufferDirty = 1;
}


// Clears the status window (the one showing the ROM size, save size etc.)
//
void clear_status_window()
{
	int i;

	for (i = 0; i < 96; i++)
	{
		bg0Buffer[0x540 + i + i] = ' ';
		bg0Buffer[0x540 + i + i + 1] = 0;
	}
	bg0BufferDirty = 1;
}


void update_screen()
{
	wait_nmi();
	load_vram(bg0Buffer, 0x2000, 0x800);
	update_oam(&marker, 0, 1);
	bg0BufferDirty = 0;
}


void clear_screen()
{
	int i;

	for (i = 0; i < 0x400; i++)
	{
		bg0Buffer[i + i] = ' ';
		bg0Buffer[i + i + 1] = 0;
	}
	bg0BufferDirty = 1;
}



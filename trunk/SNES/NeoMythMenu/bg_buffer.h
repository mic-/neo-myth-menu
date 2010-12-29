#ifndef _BG_BUFFER_H_
#define _BG_BUFFER_H_

#include "common.h"
#include "integer.h"

extern u8 bg0BufferDirty;

extern void printxy(char *, u16, u16, u16, u16);
extern void print_hex(u8, u16, u16, u16);
extern void print_dec(DWORD, u16, u16, u16);
void print_dir(char *pStr, u16 x, u16 y, u16 attribs, u16 maxChars);
void puts_game_title(u16 gameNum, u16 vramOffs, u8 attributes);


#endif

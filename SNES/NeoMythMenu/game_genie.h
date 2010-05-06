#ifndef _GAME_GENIE_H_
#define _GAME_GENIE_H_

#include "snes.h"
#include "common.h"
#include "cheats/cheat.h"

extern void gg_decode(u8 *, u8 *, u16 *, u8*);

extern void print_gg_code(ggCode_t *, u16, u16, u16);

#endif

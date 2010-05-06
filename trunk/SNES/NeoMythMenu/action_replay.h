#ifndef _ACTION_REPLAY_H_
#define _ACTION_REPLAY_H_

#include "snes.h"
#include "cheats/cheat.h"

extern void ar_decode(u8 *, u8 *, u16 *, u8*);

extern void print_ar_code(arCode_t *, u16, u16, u16);

#endif

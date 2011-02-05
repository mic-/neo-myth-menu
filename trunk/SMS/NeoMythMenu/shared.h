#ifndef __SHARED_H__
#define __SHARED_H__

#include <z80/types.h>

#define GAMES_TO_SHOW 9

typedef struct
{
	WORD firstShown;
	WORD highlighted;
	WORD size;
} FileList;


extern int foo, bar;
extern FileList games;


#ifdef EMULATOR
extern const char dummyGameList[];
#endif

#endif

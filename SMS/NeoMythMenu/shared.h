#ifndef __SHARED_H__
#define __SHARED_H__

#include <z80/types.h>

#define NUMBER_OF_GAMES_TO_SHOW 9

typedef struct
{
    WORD firstShown;
    WORD highlighted;
    WORD count;
} FileList;

/*
 * Task enumerators for task dispatchers located in other
 * banks (obsolete?)
 */
enum
{
    TASK_LOAD_BG = 0,
};

/*
 * Menu states
 */
enum
{
	MENU_STATE_GAME_GBAC = 0,
	MENU_STATE_GAME_SD,
	MENU_STATE_OPTIONS
};

extern BYTE sd_fetch_info_timeout;
extern BYTE menu_state;
extern FileList games;
extern BYTE region;
extern BYTE pad, padLast;

extern BYTE idLo,idHi;
extern WORD neoMode;
extern BYTE hasZipram;
extern BYTE vdpSpeed;

#ifdef EMULATOR
extern const char dummyGameList[];
#endif

extern const BYTE *gbacGameList;

#endif

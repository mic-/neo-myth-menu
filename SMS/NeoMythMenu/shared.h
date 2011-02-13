#ifndef __SHARED_H__
#define __SHARED_H__

#include <z80/types.h>
#define NUMBER_OF_GAMES_TO_SHOW 9
#define MAX_OPTIONS 5

typedef struct
{
    WORD firstShown;
    WORD highlighted;
    WORD count;
} FileList;

typedef struct
{
	BYTE encoded_info;		/*msb = type,lsb = 0 or 1 (enabled/disabled)*/
	char name[16];
	BYTE user_data[4];
}Option;

/*
	Option types
*/
enum
{
	OPTION_TYPE_SETTING = 0,	/*Internal setting*/
	OPTION_TYPE_CHEAT,			/*Cheat*/
	OPTION_TYPE_ROUTINE,		/*A callback*/
};

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

extern Option options[MAX_OPTIONS];
extern BYTE options_count;
void options_set_state(Option* option,BYTE new_state);
void options_set_type(Option* option,BYTE new_type);
extern Option* options_add_ex(const char* name,BYTE type,BYTE state,WORD user_data0,WORD user_data1);
extern Option* options_add(const char* name,BYTE type,BYTE state);
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

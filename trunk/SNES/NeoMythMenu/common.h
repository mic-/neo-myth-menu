#ifndef _COMMON_H_
#define _COMMON_H_

#include "snes.h"


// The default header that comes with SNESC specifies a LOROM configuration. Remove this define if HIROM is used.
//#define LOROM

#ifdef LOROM
#define GAME_LIST_BANK 1
#else
#define GAME_LIST_BANK 0
#endif

#define MAX_GG_CODES 4


typedef struct
{
	u16 count;
	u16 firstShown;
	u16 highlighted;
} gamesList_t;


typedef struct
{
	u8 used;
	u8 bank;
	u8 val;
	u16 offset;
	u8 code[8];
} ggCode_t;

// For use with can_games_list_scroll
typedef enum
{
	DIRECTION_UP,
	DIRECTION_DOWN
} scrollDirection_t;


typedef enum
{
	SORT_LOGICALLY,
	SORT_ALPHABETICALLY
} sortOrder_t;

extern u8 romSize, romRunMode, sramSize, sramBank, sramMode;
extern u8 extDsp, extSram;
extern gamesList_t gamesList;
extern char MS4[];
extern char *metaStrings[];
extern ggCode_t ggCodes[MAX_GG_CODES];
extern u16 gbaCardAlphabeticalIdx[500];
extern sortOrder_t sortOrder;

extern void set_full_pointer(void **, u8, u16);
extern void dma_bg0_buffer();

extern void run_game_from_gba_card_c();
extern void play_spc_from_gba_card_c();
extern void run_secondary_cart_c();

extern void clear_screen();
extern void print_meta_string(u16);
extern void print_games_list();
extern void print_hw_card_rev();
extern void printxy(char *, u16, u16, u16, u16);
extern void update_game_params();

#endif

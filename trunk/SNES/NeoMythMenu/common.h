#ifndef _COMMON_H_
#define _COMMON_H_

#include "snes.h"
#include "ppu.h"


// The default header that comes with SNESC specifies a LOROM configuration. Remove this define if HIROM is used.
//#define LOROM

#ifdef LOROM
#define GAME_LIST_BANK 1
#else
#define GAME_LIST_BANK 0
#endif

// Maximum number of Game Genie codes allowed
#define MAX_GG_CODES 4

// Metastring IDs
#define MS_VERSION_COPYRIGHT		0
#define MS_GAME_NUMBER				4
#define MS_MAIN_MENU_INSTRUCTIONS	75
#define MS_GG_ENTRY_MENU_INSTRUCTIONS 76
#define MS_GG_EDIT_MENU_INSTRUCTIONS 77


// Masks for the joypad data returned by read_joypad
#define JOY_R 		0x0010
#define JOY_L 		0x0020
#define JOY_X		0x0040
#define JOY_A 		0x0080
#define JOY_RIGHT 	0x0100
#define JOY_LEFT 	0x0200
#define JOY_DOWN 	0x0400
#define JOY_UP 		0x0800
#define JOY_START	0x1000
#define JOY_SELECT 	0x2000
#define JOY_Y		0x4000
#define JOY_B 		0x8000


#define SHELL_BGPAL_WHITE 2
#define SHELL_BGPAL_LIGHT_BLUE 5
#define SHELL_BGPAL_DARK_OLIVE 6
#define SHELL_BGPAL_OLIVE 7

#define SHELL_OBJPAL_DARK_OLIVE 0
#define SHELL_OBJPAL_WHITE 1


typedef struct
{
	u16 count;
	u16 firstShown;
	u16 highlighted;
} itemList_t;


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
extern itemList_t gamesList;
extern itemList_t cheatList;
extern char MS4[];
extern const char * const metaStrings[];
extern u16 gbaCardAlphabeticalIdx[500];
extern sortOrder_t sortOrder;
extern u8 snesRomInfo[0x40];
extern u8 doRegionPatch;
extern oamEntry_t marker;
extern void (*keypress_handler)(u16);

extern void set_full_pointer(void **, u8, u16);
extern void add_full_pointer(void **, u8, u16);

extern void update_screen();

extern void run_game_from_gba_card_c();
extern void play_spc_from_gba_card_c();
extern void run_secondary_cart_c();

extern void clear_screen();
extern void clear_status_window();
extern void print_meta_string(u16);
extern void print_games_list();
extern void print_cheat_list();
extern void print_hw_card_rev();
extern void printxy(char *, u16, u16, u16, u16);
extern void print_hex(u8, u16, u16, u16);
extern void show_scroll_indicators();
extern void update_game_params();

#endif

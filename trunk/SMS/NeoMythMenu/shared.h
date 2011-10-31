#ifndef __SHARED_H__
#define __SHARED_H__

#include <z80/types.h>
#include <stdint.h>

#define NUMBER_OF_GAMES_TO_SHOW 7
#define MAX_OPTIONS 6
#define MAX_CHEATS (9) //13
#define MENU_NAMETABLE 0x3000 

#define GAME_MODE_NORMAL_ROM 4
#define GAME_MODE_SPC 32
#define GAME_MODE_VGM 33
#define GAME_MODE_PACKED_VGM 34
#define GAME_MODE_ZIPPED_ROM 35
#define GAME_MODE_VGZ 36

typedef struct
{
    WORD firstShown;
    WORD highlighted;
    WORD count;
} FileList;

typedef struct		//30bytes x 6 options : 180bytes(  0xDA18(0xda08:16 : crc) ~ 0xDACC) ( 0xDB00 - 0xDACC : 52 bytes free ) (52/4 -> 13 cheat slots)
{
    BYTE encoded_info;      /*msnyb = type,lsnyb = 0 or 1 (enabled/disabled)*/
    char name[20];
    char cond0_bhv[4];
    char cond1_bhv[4];
    BYTE user_data;
}Option;

typedef struct
{
    char sfn[13];
    char lfn[32];
    uint32_t fsize;
    BYTE fattrib;
    BYTE ftype;
} FileInfoEntry;

/*
    Option types
*/
enum
{
    OPTION_TYPE_SETTING = 0,    /*Internal setting*/
    OPTION_TYPE_CHEAT,          /*Cheat*/
    OPTION_TYPE_ROUTINE,        /*A callback*/
};


enum
{
	OPTION_CB_SET_SRAM_BANK = 0xa0,
	OPTION_CB_IMPORT_IPS    = 0xa1,
	OPTION_CB_CLEAR_SRAM    = 0xa2,
	OPTION_CB_CHEAT_MGR		= 0xa3,
};

enum
{
	CT_RAM = 0,
	CT_ROM = 1
};

/*
 * Task enumerators for task dispatchers located in other
 * banks (obsolete?)
 */
enum
{
    TASK_LOAD_BG = 0,
	TASK_APPLY_OPTIONS = 1,
	TASK_EXEC_CHEAT_INPUTBOX = 2,
	TASK_PRINT_HEX = 3,
	TASK_DUMP_HEX = 4,
};

/*
 * Menu states
 */
enum
{
    MENU_STATE_TOP = 0,
    MENU_STATE_GAME_GBAC = 0,
    MENU_STATE_OPTIONS = 1,
    MENU_STATE_GAME_SD = 2,
    MENU_STATE_MEDIA_PLAYER = 3,
    MENU_STATES = 4,
};

/*
 * Code/data banks
 */
enum
{
    BANK_MAIN = 0,
    BANK_BG_GFX = 1,
    BANK_RAM_CODE = 2,
    //
    BANK_DISKIO = 4,
    BANK_PFF = 5,
    BANK_VGM_PLAYER = 6,
};

/*
 * SD card operations (for error handling)
 */
enum
{
	SD_OP_MOUNT,
	SD_OP_OPEN_FILE,
	SD_OP_READ_FILE,
	SD_OP_WRITE_FILE,
	SD_OP_SEEK,
	SD_OP_OPEN_DIR,
	SD_OP_READ_DIR,
	SD_OP_UNKNOWN
};

extern BYTE options_get_state(Option* option);
extern BYTE options_get_type(Option* option);
extern void options_set_state(Option* option,BYTE new_state);
extern void options_set_type(Option* option,BYTE new_type);
extern Option* options_add(const char* name,const char* cond0_bhv,const char* cond1_bhv,BYTE type,BYTE state);
extern Option* options_add_ex(const char* name,const char* cond0_bhv,const char* cond1_bhv,BYTE type,BYTE state,WORD user_data0,WORD user_data1);
extern void options_init();

extern BYTE flash_mem_type;

extern Option* options;//extern Option options[MAX_OPTIONS];
extern BYTE options_sram_bank;
extern BYTE options_sync;
extern BYTE options_count;
extern BYTE options_highlighted;
extern BYTE reset_to_menu_option_idx;
extern BYTE options_cheat_ptr;
extern BYTE sram_set_option_idx;
extern BYTE sram_cls_option_idx;
extern BYTE fm_enabled_option_idx;
extern BYTE import_ips_option_idx;
extern BYTE menu_state;
extern FileList games;
extern BYTE region;
extern BYTE pad, padLast;

extern BYTE idLo,idHi;
extern WORD neoMode;
extern BYTE hasZipram;
extern BYTE vdpSpeed;
extern BYTE vregs[16];
extern WORD cardType;

extern BYTE diskioPacket[7];
extern BYTE diskioResp[17];
extern BYTE diskioTemp[8];
extern uint32_t numSectors;
extern BYTE highlightedIsDir;
extern char highlightedFileName[13];

#define LIST_BUFFER_SIZE (32*2*NUMBER_OF_GAMES_TO_SHOW)
extern BYTE generic_list_buffer[LIST_BUFFER_SIZE];

#ifdef EMULATOR
extern const char dummyGameList[];
#endif

extern const BYTE *gbacGameList;

#endif

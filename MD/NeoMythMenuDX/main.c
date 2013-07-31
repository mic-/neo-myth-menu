/* Neo Super 32X/MD/SMS Flash Cart Menu by Chilly Willy, based on Dr. Neo's Menu code */
/* The license on this code is the same as the original menu code - MIT/X11 */

/*std*/
#include <string.h>
#include <stdio.h>

/*io*/
#include <diskio.h>
#include <ff.h>

/*M68000 optimized code*/
#include "deluxe/util_68k_interface.h"

/*lib config*/
#include "deluxe/conf.h"

/*For the GG/Hex cheats*/
#include "deluxe/cheat.h"
#include "deluxe/utility.h"

/*profiling ??*/
#include "deluxe/profiling.h"

//#include <u_memory.h>
//#include <zip_io.h>
#define f_open_zip f_open
#define f_close_zip f_close
#define f_lseek_zip f_lseek
#define f_read_zip f_read

#define min(x,y) (((x)<(y))?(x):(y))
#define max(x,y) (((x)>(y))?(x):(y))

#define STEP_INTO(S)
// {setStatusMessage(S); delay(100);}

/*tables*/
static const char* EEPROM_MAPPERS[] =
{
    "T-8104B",//new - nba jam 32x
    "GM G-4025",//new - wonder boy 3
    "GM G-4060",//new - wonder boy in MW

    //shared from genplus gx
    "T-120106",
    "T-50176",
    "T-50396",
    "T-50446",
    "T-50516",
    "T-50606",
    "T-12046",
    "T-12053",
    "MK-1215",
    "MK-1228",
    "G-5538",
    "PR-1993",
    "G-4060",
    "G-4060-00",
    "00001211-00",
    "00004076-00",
    "T-081326",
    "T-81033",
    "T-81406",
    "T-081276",
    "00000000-00",
    "T-081586",
    "T-81576",
    "T-81476",
    "T-120146-50",
    NULL,
    NULL
};

/* hardware definitions */
#define SEGA_CTRL_BUTTONS   0x0FFF
#define SEGA_CTRL_UP        0x0001
#define SEGA_CTRL_DOWN      0x0002
#define SEGA_CTRL_LEFT      0x0004
#define SEGA_CTRL_RIGHT     0x0008
#define SEGA_CTRL_B         0x0010
#define SEGA_CTRL_C         0x0020
#define SEGA_CTRL_A         0x0040
#define SEGA_CTRL_START     0x0080
#define SEGA_CTRL_Z         0x0100
#define SEGA_CTRL_Y         0x0200
#define SEGA_CTRL_X         0x0400
#define SEGA_CTRL_MODE      0x0800

#define SEGA_CTRL_TYPE      0xF000
#define SEGA_CTRL_THREE     0x0000
#define SEGA_CTRL_SIX       0x1000
#define SEGA_CTRL_NONE      0xF000

#define XFER_SIZE 16384

#define OPTION_ENTRIES 30               /* two screen's worth for now */

enum
{
    /*Save manager service status*/
    SMGR_STATUS_NULL = 0,
    SMGR_STATUS_BACKUP_SRAM,

    /*Save manager service mode*/
    SMGR_MODE_MD32X,
    SMGR_MODE_SMS,
    SMGR_MODE_BRAM
};

/*For the GG/Hex cheats*/
#define CHEAT_ENTRIES_COUNT (OPTION_ENTRIES - 6)
#define CHEAT_SUBPAIR_COUNT 12          //12 pairs per entry
enum
{
    CT_NULL   = 0x0, //null cheat entry
    CT_CHILD  = 0x1, //not used now
    CT_SELF   = 0x2, //normal cheat, not used now
    CT_MASTER = 0x4, //master code!
    CT_REGION = 0x8  //region code
};

typedef struct CheatEntry CheatEntry;
struct CheatEntry
{
    char name[32];
    CheatPair pair[CHEAT_SUBPAIR_COUNT];
    unsigned char active;
    unsigned char pairs;
    unsigned char type;
    unsigned char _pad0_;
};

#define CACHEBLK_VERS 2

typedef struct CacheBlock CacheBlock;
struct CacheBlock
{
    char sig[4];/*DXCS*/
    unsigned char version;/*what version?*/
    unsigned char sramType;/*eeprom or sram?*/
    unsigned char processed;/*Needs detection ? 0xff = processed*/
    unsigned char autoManageSaves;/*manage saves automatically ? */
    unsigned char autoManageSavesServiceStatus;/*1 = Backup SRAM using the above settings^ 0 = nothing to do*/
    unsigned char sramBank;/*which bank affects?*/
    short int sramSize;/*what's the sram size if type == sram?*/
    short int YM2413; /*fm on?*/
    short int resetMode;/*reset to menu or game?*/
    short int aliasMode;/*bank aliasing?*/
};

static CacheBlock gCacheBlock;/*common cache block*/

static CheatEntry cheatEntries[CHEAT_ENTRIES_COUNT];
static short registeredCheatEntries = 0;
static short gResponseMsgStatus = 0;

#define eeprom_inc_path "/menu/md/eeprom.inc"
#define dxcore_dir "/menu/md"
#define dxconf_cfg "/menu/md/DXCONF.CFG"
#define cheats_dir_default "menu/md/cheats"
#define ips_dir_default "menu/md/ips"
#define saves_dir_default "menu/md/saves"
#define cache_dir_default "menu/md/cache"
#define md_32x_save_ext_default ".srm"
#define sms_save_ext_default ".ssm"
#define brm_save_ext_default ".brm"

static char* CHEATS_DIR = cheats_dir_default;
static char* IPS_DIR = ips_dir_default;
static char* SAVES_DIR = saves_dir_default;
static char* CACHE_DIR = cache_dir_default;
static char* MD_32X_SAVE_EXT = md_32x_save_ext_default;
static char* SMS_SAVE_EXT = sms_save_ext_default;
static char* BRM_SAVE_EXT = brm_save_ext_default;

static short int gManageSaves = 0;
static short int gSRAMgrServiceStatus = SMGR_STATUS_NULL;
static short int gSRAMgrServiceMode = 0x0000;

#ifndef RUN_IN_PSRAM
static const char gAppTitle[] = "Neo Super 32X/MD/SMS Menu v3.0";
#else
static const char gAppTitle[] = "NEO Super 32X/MD/SMS Menu v3.0";
#endif

#define MB (0x20000)
#define KB (0x400)
#define printToScreen(_TEXT_,_X_,_Y_,_COLOR_) { gCursorX = _X_; gCursorY = _Y_; put_str(_TEXT_,_COLOR_);}
#define debugText printToScreen

/* Menu entry definitions */
#define PAGE_ENTRIES 15                 /* number of entries to show per screen page */
#define MAX_ENTRIES 401                 /* maximum number of menu entries in flash or per directory on SD card */
// note - the current flash menu can only fit 639 entries from 0xB000 to 0xFFE0

struct menuEntry {
    unsigned char meValid;              /* 0x00 = valid, 0xFF = invalid (end of entries) */
    unsigned char meType;               /* 0 = MD game, 1 = 32X game, 2 = SMS game */
    unsigned char meROMHi;              /* LSN = GBA flash ROM high (A27-A24), MSN = GBA flash ROM size */
    unsigned char meROMLo;              /* GBA flash ROM low (A23-A16) */
    unsigned char meSRAM;               /* LSN = GBA SRAM size, MSN = GBA SRAM bank */
    unsigned char mePad1;               /* reserved */
    unsigned char meRun;                /* run mode: 6 = MD/32X game, 8 = CD BIOS, 0x13 = SMS */
    unsigned char mePad2;               /* reserved */
    char meName[24];                    /* entry name string (null terminated) */
} __attribute__ ((packed));

typedef struct menuEntry menuEntry_t;

struct selEntry {
    unsigned char type;                 /* 0 = MD game, 1 = 32X game, 2 = SMS game, 128 = directory */
    unsigned char run;                  /* run mode: 6 = MD/32X game, 8 = CD BIOS, 0x13 = SMS */
    unsigned char bbank;                /* backup ram bank number */
    unsigned char bsize;                /* backup ram size in 8KB units */
    int offset;                         /* offset in media of data associated with entry */
    int length;                         /* amount of data associated with entry */
    WCHAR *name;                        /* entry name string (null terminated) */
} __attribute__ ((packed));

typedef struct selEntry selEntry_t;

/* global variables */
static unsigned int gSelectionSize;
static short int gPSRAM;                /* 0 = gba psram, 1 = myth psram */
static short int gShortenMode = 0;      /* 0 = show left side, 1 = show right side, 2 = try to show important parts */
static short int gHelp = 1;             /* 0 = show hardware info, 1 = show help messages */

short int gCpldVers;                    /* 3 = V11 hardware, 4 = V12 hardware, 5 = V5 hardware */
short int gCardType;                    /* 0 = 512 Mbit Neo2 Flash, 1 = other */
short int gCardOkay;                    /* 0 = okay, -1 = err */
short int gCursorX;                     /* range is 0 to 63 (only 0 to 39 onscreen) */
short int gCursorY;                     /* range is 0 to 31 (only 0 to 27 onscreen) */
short int gUpdate = -1;                 /* 0 = screen unchanged, 1 = minor change, -1 = major change */
short int gRomDly = 0;                  /* delay until load rom header */
short int gMaxEntry = 0;                /* number of entries in selection array */
short int gCurEntry = 0;                /* current entry number */
short int gStartEntry = 0;              /* starting entry for page of entries on display */
short int gCurMode = 0;                 /* current menu mode */
short int gCurUSB = 0;                  /* current USB mode (0 = off, 1 = on) */
short int gFileType = 0;                /* current file type (0 = bin, 1 = SMD_LSB_1ST, 2 = SMD_MSB_1ST) */
short int gMythHdr = 0;                 /* 1 = found "MYTH" header in memo */
unsigned short int gButtons = 0;        /* previous button state */
extern volatile int gTicks;             /* incremented every vblank */

/* global options table entry definitions */

typedef struct {
    char *name;                         /* option name string */
    char *value;                        /* option value string */
    void (*callback)(int index);        /* callback when C pressed */
    void (*patch)(int index);           /* callback after rom loaded */
    void* userData;                     /* added for cheat support*/
    short exclusiveFCall;               /* required for the new exclusive function calls */
} optionEntry;

typedef struct {
    unsigned char Magic[5];
    unsigned char Neo2;
    unsigned char Neo3;
    unsigned char Cpld;
    unsigned short MenuMan;
    unsigned short MenuDev;
    unsigned short GameMan;
    unsigned short GameDev;
} hwinfo;

hwinfo gCart;

#define IS_FLASH (gCart.MenuMan == 0x0089)
#define IS_NEO2SD (gCart.GameDev == 0x880D)
#define IS_NEO2PRO (gCart.GameDev == 0x8810)
#define IS_NEO3 (gCart.GameMan == 0x0000)

#define WORK_RAM_SIZE (XFER_SIZE*2)

/* global options */
short int gResetMode = 0x0000;          /* 0x0000 = reset to menu, 0x00FF = reset to game */
short int gGameMode = 0x00FF;           /* 0x0000 = menu mode, 0x00FF = game mode */
short int gWriteMode = 0x0000;          /* 0x0000 = write protected, 0x0002 = write-enabled */
short int gSRAMType;                    /* 0x0000 = sram, 0x0001 = eeprom */
short int gSRAMBank;                    /* sram bank = 0 to 15, constrained by size of sram */
short int gSRAMSize;                    /* size of sram in 8KB units */

short int gNoAlias = 0x0000;            /* 0x0000 = normal Myth PSRAM banks, 0x00FF = full size bank padded with 0xFF */

short int gYM2413 = 0x0001;             /* 0x0000 = YM2413 disabled, 0x0001 = YM2413 enabled */
short int gImportIPS = 0;
static int gLastEntryIndex = -1;

const char gEmptyLine[] = "                                      ";
const char gFEmptyLine[] = "\x7C                                    \x7C";
const char gLine[]  = "\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82";
const char gFBottomLine[] = "\x86\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x83";

const short int fsz_tbl[16] = { 0,1,2,0,4,5,6,0,8,16,24,32,40,0,0,0 };

WCHAR *lfnames = (WCHAR *)(0x400000 - MAX_ENTRIES * 512); /* space for long file names in PSRAM */
WCHAR *wstr_buf = (WCHAR *)0x380000;    /* space for WCHAR strings in PSRAM */
//WCHAR wstr_buf[512*4];
static unsigned int gWStrOffs = 0;

//unsigned char rtc[8];                   /* RTC from Neo2/3 flash cart */

static char gSRAMSizeStr[8];
static char gSRAMBankStr[8];

static char gStsLine[64];
static char entrySNameBuf[64];
static char gProgressBarStaticBuffer[64];

short int gDirectRead = 0;
short int gSdDetected = 0;              /* 0 - not detected, 1 - detected */

extern unsigned short cardType;         /* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */
extern unsigned int num_sectors;        /* number of sectors on SD card (or 0) */
extern unsigned char *sd_csd;           /* card specific data */

FATFS gSDFatFs;                         /* global FatFs structure for FF */
FIL gSDFile;                            /* global file structure for FF */

WCHAR ipsPath[512];                     /* ips path */
WCHAR path[512];                        /* SD card file path */

unsigned char __attribute__((aligned(16))) rom_hdr[256];             /* rom header from selected rom (if loaded) */
unsigned char __attribute__((aligned(16))) buffer[XFER_SIZE*2];      /* Work RAM buffer - big enough for SMD decoding */

optionEntry gOptions[OPTION_ENTRIES];   /* entries for global options */
selEntry_t gSelections[MAX_ENTRIES];    /* entries for flash or current SD directory */


/* misc definitions */

enum {
    MODE_FLASH,
    MODE_USB,
    MODE_SD,
    MODE_END
};

void updateConfig();
void loadConfig();
void do_options(void);
void cache_sync(int root);
void cache_load();
void cache_invalidate_pointers();

/* external support functions */

extern int set_sr(int sr);
extern void init_hardware(void);
extern unsigned short int get_pad(int pad);
extern void clear_screen(void);
extern void put_str(const char *str, int fcolor);
extern void set_usb(void);
extern short int neo_check_cpld(void);
extern short int neo_check_card(void);
extern void neo_hw_info(hwinfo *ptr);
extern void neo_run_game(int fstart, int fsize, int bbank, int bsize, int run);
extern void neo_run_psram(int pstart, int psize, int bbank, int bsize, int run);
extern void neo_run_myth_psram(int psize, int bbank, int bsize, int run);
extern void neo_copy_game(unsigned char *dest, int fstart, int len);
extern void neo_copyto_psram(unsigned char *src, int pstart, int len);
extern void neo_copyfrom_psram(unsigned char *dst, int pstart, int len);
extern void neo_copyto_myth_psram(unsigned char *src, int pstart, int len);
extern void neo_copyfrom_myth_psram(unsigned char *dst, int pstart, int len);
extern void neo_copyto_sram(unsigned char *src, int sstart, int len);
extern void neo_copyfrom_sram(unsigned char *dst, int sstart, int len);
extern void neo_get_rtc(unsigned char *rtc);
extern void neo2_enable_sd(void);
extern void neo2_disable_sd(void);
extern DSTATUS MMC_disk_initialize(void);

extern void ints_on();
extern void ints_off();
extern void PlayVGM(void);

int do_SDMgr(void);

void run_rom(int reset_mode);

int inputBox(char* result,const char* caption,const char* defaultText,short int  boxX,short int  boxY,
            short int  captionColor,short int boxColor,short int textColor,short int hlTextColor,short int maxChars);

void update_display(void);

static short int gChangedPage = 0;
static short int gCacheOutOfSync = 0;
static int inputboxDelay = 5;

#if 0
void dump_zipram(int len,int gen);
#endif
//macros
inline int is_space(char c)
{
    switch(c)
    {
        case 0x9:
        case 0xa:
        case 0xb:
        case 0xc:
        case 0xd:
        case 0x20:
            return 1;

        default:
            return 0;
    }

    return 0;
}

//0 = 7.67 MHz, and 1 = 7.60 MHz
inline int getClockType()
{
  return (*(char *)0xA10001 >> 6) & 1;
}

inline char systemRegion()
{
    char _od=(*(char *)0xA10001 >> 7) & 1;
    char _pn=(*(char *)0xA10001 >> 6) & 1;
    return _od ? (_pn ? 'E' : 'U') : 'J';
}

inline void delay(int count)
{
    int ticks;

    ints_on();

    ticks = gTicks + count;

    while (gTicks < ticks) ;
}

inline void setStatusMessage(const char* msg)
{
    printToScreen(gEmptyLine, 1, 23, 0x0000);
    printToScreen(msg, (40 - utility_strlen(msg))>>1, 23, 0x0000);
}

inline void clearStatusMessage()
{
    printToScreen(gEmptyLine, 1, 23, 0x0000);
}

int directoryExists(const XCHAR* fps)
{
    DIR dir;

    if(f_opendir(&dir,fps) != FR_NO_PATH)
        return 1;

    return 0;
}

int fileExists(const XCHAR* fss)
{
    f_close(&gSDFile);

    if(f_open(&gSDFile, fss, FA_OPEN_EXISTING | FA_READ) != FR_OK)
        return 0;

    f_close(&gSDFile);
    return 1;
}

void makeDir(const XCHAR* fps)
{
    int i,l,j,di;
    WCHAR* ss = &wstr_buf[gWStrOffs];
    gWStrOffs += 512;

    utility_strcpy(gStsLine,"Creating directory: ");
    utility_w2cstrcpy(&gStsLine[20],fps);
    setStatusMessage(gStsLine);

    i = j = 0;
    l = utility_wstrlen(fps);
    di = 1;

    if(fps[0] == (WCHAR)'/' )
    {
        ss[j++] = (WCHAR)'/';
        ++i;
    }

    while(i<l)
    {
        while((i<l) && (fps[i]!= (WCHAR)'/') )
        {
            ss[j++] = (WCHAR)fps[i++];
        }

        ss[j] = 0;

        if((di == 0)||(!directoryExists(ss)))
        {
            f_mkdir(ss);
            di = 0;
        }

        ss[j++] = (WCHAR)fps[i++];
    }

    clearStatusMessage();
    gWStrOffs -= 512;
}

int createDirectory(const XCHAR* fps)
{
    if(directoryExists(fps))
        return 1;

    makeDir(fps);

    return directoryExists(fps);
}

int createDirectoryFast(const XCHAR* fps)
{
    if(directoryExists(fps))
        return 1;

    utility_strcpy(gStsLine,"Creating directory: ");
    utility_w2cstrcpy(&gStsLine[20],fps);
    setStatusMessage(gStsLine);

    f_mkdir(fps);

    clearStatusMessage();
    return 1;
}

int deleteFile(const XCHAR* fss)
{
    if(!fileExists(fss))
        return FR_OK;

    return (f_unlink(fss) == FR_OK);
}

int deleteDirectory(const XCHAR* fps)
{
    if(!directoryExists(fps))
        return FR_OK;

    return (f_unlink(fps) == FR_OK);
}

UINT getFileSize(const XCHAR* fss)
{
    UINT r = 0;

    f_close(&gSDFile);

    if(f_open(&gSDFile, fss, FA_OPEN_EXISTING | FA_READ) != FR_OK)
        return 0;

    r = gSDFile.fsize;
    f_close(&gSDFile);

    return r;
}

int shortenName(char *dst, char *src, int max)
{
    short len,ix,iy,right;

    len = utility_strlen(src);

    if (len <= max)
    {
        // string fits, just copy it
        utility_strcpy(dst, src);
        return len;
    }

    if (gShortenMode == 0)
    {
        // copy left side
        utility_memcpy(dst, src, max-4);
        dst[max - 5] = '~';
        utility_memcpy(&dst[max - 4], &src[len-4], 4);
        dst[max] = '\0';
        return max;
    }
    else if (gShortenMode == 1)
    {
        // copy right side
        utility_memcpy(dst, src, 4);
        dst[4] = '~';
        utility_memcpy(&dst[5], &src[len-max+5], max-5);
        dst[max] = '\0';
        return max;
    }

    right = (max >> 1);

    // check right side of string for important name cues
    for (ix=iy=len-1; ix>(len-right); ix--)
    {
        switch(*(src + ix))
        {
            case '.':
            case '[':
            case '(':
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                iy = ix;
            break;
        }
    }

    right = (right > (len - iy)) ? (len - iy) : right;
    utility_memcpy(dst, src, max);
    dst[max - right - 1] = '~';
    utility_memcpy(&dst[max - right], &src[iy], right);
    dst[max] = '\0';
    return max;
}

/* cheat handling functions */
void cheat_invalidate()
{
    int a,b;

    registeredCheatEntries = 0;

    for(a = 0; a < CHEAT_ENTRIES_COUNT; a++)
    {
        cheatEntries[a].name[0] = cheatEntries[a].name[31] = '\0';
        cheatEntries[a].pairs = 0;
        cheatEntries[a].active = 0;
        cheatEntries[a].type = CT_NULL;

        for(b = 0; b < CHEAT_SUBPAIR_COUNT; b++)
        {
            cheatEntries[a].pair[b].addr = 0;
            cheatEntries[a].pair[b].data = 0;
        }
    }
}

void cheat_popEntry()
{
    if(registeredCheatEntries)
    {
        cheatEntries[registeredCheatEntries].pairs = 0;
        cheatEntries[registeredCheatEntries].type = CT_NULL;
        cheatEntries[registeredCheatEntries].active = 0;
        cheatEntries[registeredCheatEntries].name[0] =
        cheatEntries[registeredCheatEntries].name[31] = '\0';
    }

    --registeredCheatEntries;

    if(registeredCheatEntries < 0)
        registeredCheatEntries = 0;
}

CheatEntry* cheat_register()
{
    CheatEntry* e = NULL;

    if(registeredCheatEntries > CHEAT_ENTRIES_COUNT-1)
        return NULL;

    e = &cheatEntries[registeredCheatEntries++];

    if(e)
        e->pairs = 0;

    return e;
}

CheatPair* cheat_registerPair(CheatEntry* e)
{
    CheatPair* cp;

    if(!e)
        return NULL;

    if(e->pairs > CHEAT_SUBPAIR_COUNT-1)
        return NULL;

    cp = &e->pair[e->pairs++];

    return cp;
}


int cheat_fillPairs(const char* code,CheatEntry* ce)
{
    CheatPair* cp;
    int i = 0;
    int len = utility_strlen(code) - 1;
    int ind = 0;
    int prg = 0;
    char buf[16];

    UTIL_SetMemorySafe(buf,'0',16);

    while(*code)
    {
        if((code[i] == ',') || (ind >10) || (prg > len) )
        {
            buf[ind] = '\0';
            ind = 0;

            cp = cheat_registerPair(ce);

            if(!cp)//out of slots
                return 1;

            cp->addr = 0;
            cp->data = 0;

            if(cheat_decode(buf,cp))//failed
            {
                //cheat_popEntry();//remove already done
                return 1;
            }

            ++code;
        }
        else
            buf[ind++] = *(code++);

        ++prg;
    }

    //left overs ?? => ie : the user didn't put the last delim ','...
    {
        if(ind)
        {
            buf[ind] = '\0';
            ind = 0;

            cp = cheat_registerPair(ce);

            if(!cp)//out of slots
                return 1;

            cp->addr = 0;
            cp->data = 0;

            if(cheat_decode(buf,cp))//failed
            {
                //cheat_popEntry();//remove already done
                return 0;
            }
        }
    }

    return 0;
}

/* wide string functions */
/*
int wstrcmp(WCHAR *ws1, WCHAR *ws2)
{
    int ix = 0;
    while (ws1[ix] && ws2[ix] && (ws1[ix] == ws2[ix])) ix++;
    if (!ws1[ix] && ws2[ix])
        return -1; // ws1 < ws2
    if (ws1[ix] && !ws2[ix])
        return 1; // ws1 > ws2
    return (int)ws1[ix] - (int)ws2[ix];
}

int wstrlen(const WCHAR *ws)
{
    int ix = 0;
    while (ws[ix]) ix++;
    return ix;
}

WCHAR *wstrcat(WCHAR *ws1, WCHAR *ws2)
{
    int ix = wstrlen(ws1);
    int iy = 0;
    while (ws2[iy])
    {
        ws1[ix] = ws2[iy];
        ix++;
        iy++;
    }
    ws1[ix] = 0;
    return ws1;
}

WCHAR *wstrcpy(WCHAR *ws1, WCHAR *ws2)
{
    int ix = 0;
    while (ws2[ix])
    {
        ws1[ix] = ws2[ix];
        ix++;
    }
    ws1[ix] = 0;
    return ws1;
}

char *w2cstrcpy(char *s1, WCHAR *ws2)
{
    int ix = 0;
    while (ws2[ix])
    {
        s1[ix] = (char)ws2[ix]; // lazy conversion of wide chars
        ix++;
    }
    s1[ix] = 0;
    return s1;
}

WCHAR *c2wstrcpy(WCHAR *ws1, char *s2)
{
    int ix = 0;
    while (s2[ix])
    {
        ws1[ix] = (WCHAR)s2[ix];
        ix++;
    }
    ws1[ix] = 0;
    return ws1;
}

WCHAR *c2wstrcat(WCHAR *ws1, char *s2)
{
    int ix = wstrlen(ws1);
    int iy = 0;
    while (s2[iy])
    {
        ws1[ix] = (WCHAR)s2[iy];
        ix++;
        iy++;
    }
    ws1[ix] = 0;
    return ws1;
}
*/
/* support functions */
void neo_copy_sd(unsigned char *dest, int fstart, int len)
{
    UINT ts;
    ints_on();     /* enable interrupts */
    f_read_zip(&gSDFile, dest, len, &ts);
    ints_off();     /* disable interrupts */
}

void neo_sd_to_myth_psram(unsigned char *src, int pstart, int len)
{
    UINT ts;
    ints_on();     /* enable interrupts */
    gDirectRead = 1;
    f_read(&gSDFile, (unsigned char *)pstart, len, &ts);
    gDirectRead = 0;
    ints_off();     /* disable interrupts */
}


void sort_entries()
{
    int ix;
    short int a,b;
    selEntry_t temp;
    selEntry_t* pa,*pb;

    /*
        Not very efficient sorting algorithm , but at least is optimized.
        A better method would be to use a pointer swap table but that needs some work...
    */
    for (ix=0; ix<gMaxEntry-1; ix++)
    {
        pa = &gSelections[ix];
        pb = &gSelections[ix+1];
        a = (short int)pa->type;
        b = (short int)pb->type;

        if((a != 128) && (b == 128))
        {
            // directories go first
            utility_memcpy_entry_block((void*)&temp,(void*)pa);
            utility_memcpy_entry_block((void*)pa,(void*)pb);
            utility_memcpy_entry_block((void*)pb,(void*)&temp);
            ix = !ix ? -1 : ix-2;
        }
        else if(((a^b) & 0x80) == 0)
        {
            // both entries are directories, or both are files
            if(utility_wstrcmp(pa->name,pb->name) > 0)
            {
                utility_memcpy_entry_block((void*)&temp,(void*)pa);
                utility_memcpy_entry_block((void*)pa,(void*)pb);
                utility_memcpy_entry_block((void*)pb,(void*)&temp);
                ix = !ix ? -1 : ix-2;
            }
        }
    }
}

void get_menu_flash(void)
{
    menuEntry_t *p = NULL;
    char extension[8];
    int ix;

    gMaxEntry = 0;
    rom_hdr[0] = rom_hdr[1] = 0xFF;
    // count the number of entries present in menu flash
    p = (menuEntry_t *)0x00B000;            /* menu entries in menu flash */
    while (p->meValid != 0xFF)
    {
        // set entry in selections array
        gSelections[gMaxEntry].type = p->meType;
        gSelections[gMaxEntry].run = p->meRun;
        gSelections[gMaxEntry].bbank = (p->meSRAM & 0xF0) >> 4;
        gSelections[gMaxEntry].bsize = (p->meSRAM & 0x0F) ? 1 << ((p->meSRAM & 0x0F)-1) : 0;
        gSelections[gMaxEntry].offset = ((p->meROMHi & 0x0F)<<25)|(p->meROMLo<<17);
        gSelections[gMaxEntry].length = fsz_tbl[(p->meROMHi & 0xF0)>>4] * 131072;
        utility_c2wstrcpy(gSelections[gMaxEntry].name, p->meName);

        for (ix=min(24, strlen(p->meName)); ix>1; ix--)
            if (gSelections[gMaxEntry].name[ix-1] != (WCHAR)' ')
                break;
        gSelections[gMaxEntry].name[ix] = 0;

        // check for auto-boot extended menu
        if ((gSelections[gMaxEntry].run == 7) && !utility_memcmp(p->meName, "MDEBIOS", 7))
        {
            gCurEntry = gMaxEntry;
            run_rom(0x0000); // never returns
        }

        // check if SCD BRAM should be enabled
        if (((gSelections[gMaxEntry].run & 63) == 9) || (gSelections[gMaxEntry].run == 10))
        {
            if (gSelections[gMaxEntry].bsize != 16)
            {
                gSelections[gMaxEntry].bsize = 16; // 1Mbit
                gSelections[gMaxEntry].bbank = 3; // default to bank 3 to keep it out of the way of other saves
            }
        }

        // check for SMS override
        utility_strcpy(extension, &p->meName[utility_strlen(p->meName) - 4]);
        if (!utility_memcmp(extension, ".SMS", 4) || !utility_memcmp(extension, ".sms", 4))
        {
            // SMS ROM extension
            if (gSelections[gMaxEntry].type != 2)
            {
                gSelections[gMaxEntry].type = 2; // SMS
                gSelections[gMaxEntry].run = 0x13; // run mode = SMS + FM
            }
        }
        utility_strcpy(extension, &p->meName[utility_strlen(p->meName) - 3]);
        if (!utility_memcmp(extension, ".SG", 3) || !utility_memcmp(extension, ".sg", 3))
        {
            // SG-1000 ROM extension
            if (gSelections[gMaxEntry].type != 2)
            {
                gSelections[gMaxEntry].type = 2; // SMS
                gSelections[gMaxEntry].run = 0x12; // run mode = SMS
            }
        }

        // check for consistent run mode when sram enabled
        if (gSelections[gMaxEntry].type == 0 && gSelections[gMaxEntry].bsize && gSelections[gMaxEntry].run != 5  && gSelections[gMaxEntry].run != 9 && gSelections[gMaxEntry].run != 10)
        {
            if (gSelections[gMaxEntry].length <= 0x200000)
                gSelections[gMaxEntry].run = 1; // 16 Mbit + SRAM
            else if (gSelections[gMaxEntry].length <= 0x400000)
                gSelections[gMaxEntry].run = 2; // 32 Mbit + SRAM
        }

        // check for consistent run mode for 32X game
        if (gSelections[gMaxEntry].type == 1)
        {
            if (gSelections[gMaxEntry].run < 3)
                gSelections[gMaxEntry].run = 3; // 32X mode + SRAM
        }

        // next entry
        gMaxEntry++;
        p++;
        if (gMaxEntry == MAX_ENTRIES)
            break;
    }

    sort_entries();
}

void get_smd_hdr(unsigned char *jump)
{
    int ix;
    for (ix=0; ix<128; ix++)
    {
        rom_hdr[ix*2 + 1] = buffer[ix + 0x0280];
        rom_hdr[ix*2 + 0] = buffer[ix + 0x2280];
    }
    if (!utility_memcmp(rom_hdr, "SEGA", 4))
    {
        gFileType = 1; // SMD LSB 1ST
        jump[0] = buffer[0x2300];
        jump[3] = buffer[0x0301];
    }
    else
    {
        for (ix=0; ix<128; ix++)
        {
            rom_hdr[ix*2 + 0] = buffer[ix + 0x0280];
            rom_hdr[ix*2 + 1] = buffer[ix + 0x2280];
        }
        if (!utility_memcmp(rom_hdr, "SEGA", 4))
        {
            gFileType = 2; // SMD MSB 1ST
            jump[0] = buffer[0x0300];
            jump[3] = buffer[0x2301];
        }
    }
}


WCHAR *get_file_ext(WCHAR *src)
{
    WCHAR* s = src;
    while(*src != 0)
    {
        if(*src == (WCHAR)'.')
            return src;

        ++src;
    }

    return s;
}

void get_sd_ips(int entry)
{
    char* pa;
    WCHAR* fp;
    UINT bytesWritten = 0;

    gImportIPS = 0;

    utility_c2wstrcpy(ipsPath, "/");
    utility_c2wstrcat(ipsPath, IPS_DIR);
    utility_c2wstrcat(ipsPath, "/");
    utility_wstrcat(ipsPath, gSelections[entry].name);

    fp = get_file_ext(ipsPath);
    if(*fp == (WCHAR)'.')
        *fp = 0;
    utility_c2wstrcat(ipsPath, ".ips");

    f_close(&gSDFile);
    if(f_open(&gSDFile, ipsPath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        ipsPath[0] = 0; // disable
        return;
    }

    if(!gSDFile.fsize)
    {
        f_close(&gSDFile);
        ipsPath[0] = 0; // disable
        return;
    }

    //test
    pa = (char*)buffer;

    ints_on();
    if(f_read(&gSDFile, pa, 5, &bytesWritten) != FR_OK)
    {
        f_close(&gSDFile);
        ipsPath[0] = 0; // disable
        return;
    }

    f_close(&gSDFile);
    ints_off();
    pa[6] = 0; // make sure null terminated
    if(strncasecmp((const char *)pa, "patch", 5) == 0)
        return;

    ipsPath[0] = 0; // disable
}

void get_sd_cheat(WCHAR* sss)
{
    char *cheatBuf = (char*)&buffer[XFER_SIZE + 2];
    WCHAR* cheatPath = &wstr_buf[gWStrOffs];
    WCHAR* fp;
    CheatEntry* e = NULL;
    char* pb = (char*)&buffer[0];
    char* head,*sp;
    UINT bytesWritten;
    UINT bytesToRead;
    short lk,ll;
    char region = systemRegion();

    gWStrOffs += 512;
    ints_on();
    cheat_invalidate();

    utility_c2wstrcpy(cheatPath,"/");
    utility_c2wstrcat(cheatPath, CHEATS_DIR);
    utility_c2wstrcat(cheatPath, "/");
    utility_wstrcat(cheatPath, sss);

    fp = get_file_ext(cheatPath);

    if(*fp == (WCHAR)'.')
        *fp = 0; // cut off the extension

    utility_c2wstrcat(cheatPath, ".cht");

    f_close(&gSDFile);
    if(f_open(&gSDFile, cheatPath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        bytesWritten = 0;
        pb[0] = '\0';

        bytesToRead = gSDFile.fsize;

        if(!bytesToRead)
        {
            gWStrOffs -= 512;
            return;
        }

        if(bytesToRead > WORK_RAM_SIZE) //32K are more than enough
            bytesToRead = WORK_RAM_SIZE;

        ints_on();
        if(f_read(&gSDFile, pb, bytesToRead, &bytesWritten) == FR_OK)
        {
            if(!bytesWritten)
            {
                gWStrOffs -= 512;
                return;
            }

            pb[bytesWritten] = '\0';
            head = pb;

            //int y = 0;

            while(*head)
            {
                sp = UTIL_StringFind(head,"AddCheat\0");

                if(sp)
                {
                    sp += 8;//speedup string parsing

                    e = cheat_register();

                    if(!e)//all cheat slots used
                    {
                        gWStrOffs -= 512;
                        return;
                    }

                    utility_memset(e->name,'\0',32);//UTIL_SetMemorySafe(e->name,'\0',32);//remove possible garbage
                    UTIL_SubString(e->name,sp,"$Name(",")"); e->name[31] = '\0';

                    lk = utility_strlen(e->name);//UTIL_StringLengthConst(e->name);
                    sp += 7;//speedup string parsing
                    sp += lk;//speedup string parsing

                    utility_memset(cheatBuf,'\0',256);//UTIL_SetMemorySafe(cheatBuf,'\0',256);//remove possible garbage
                    UTIL_SubString(cheatBuf,sp,"$Code(",")");cheatBuf[255] = '\0';

                    //debugText(cheatBuf,2,y,0); y += 3;

                    ll = utility_strlen(cheatBuf);//UTIL_StringLengthConst(cheatBuf);

                    sp += 7;//speedup string parsing
                    sp += ll;//speedup string parsing

                    if(lk + ll > 10)
                    {
                        if(cheat_fillPairs(cheatBuf,e))
                            cheat_popEntry();//remove
                        else
                        {
                            e->active = 0;
                            e->type = CT_SELF;

                            if(UTIL_StringFind(sp,"$Flags"))
                            {
                                sp += 6;//speedup string parsing

                                if(UTIL_StringFind(sp,"CT_MASTER"))
                                {
                                    e->type = e->type | CT_MASTER;
                                    e->active = 0;
                                }
                                /*else*/ if(UTIL_StringFind(sp,"CT_REGION"))
                                {
                                    e->type = e->type | CT_REGION;
                                    if((region==rom_hdr[0xf0])||(region==rom_hdr[0xf1])||(region==rom_hdr[0xf2]))
                                    {
                                        e->active = 0;
                                    }
                                    else
                                    {
                                        e->active = 1;
                                    }
                                }
                                /*else*/ if(UTIL_StringFind(sp,"CT_CHILD"))
                                {
                                    e->type = e->type | CT_CHILD;
                                    e->active = 0;
                                }
                                /*else*/ if(UTIL_StringFind(sp,"CT_SELF"))
                                {
                                    e->type = e->type | CT_SELF;
                                    e->active = 0;
                                }
                            }
                        }
                    }
                    else
                        cheat_popEntry();//Bad script

                    while(*sp) //SKIP LN
                    {
                        if(*sp == '\r')
                        {
                            ++sp;

                            if(*sp == '\n')
                                break;
                        }
                        else if (*sp == '\n')
                            break;

                        ++sp;
                    }//!ln?

                    head = sp;
                }//sp?

                ++head;

            }//*head?

        }//f_read -> ok?
        //f_close(&gSDFile);
    }//open handle?
    gWStrOffs -= 512;
}

void get_sd_info(int entry)
{
    unsigned char jump[4];
    char extension[8];
    int eos = utility_wstrlen(path);
    UINT ts;

    gFileType = 0;
    gMythHdr = 0;

    printToScreen(gEmptyLine,1,20,0x0000);
    printToScreen(gEmptyLine,1,21,0x0000);
    printToScreen(gEmptyLine,1,22,0x0000);
    printToScreen(gEmptyLine,1,23,0x0000);

    if (gSelections[entry].type == 128)
    {
        gSelections[entry].run = 0xee;//hack for options
        return;
    }

    if (path[eos-1] != (WCHAR)'/')
        utility_c2wstrcat(path, "/");

    utility_wstrcat(path, gSelections[entry].name);
    f_close_zip(&gSDFile);
    if (f_open_zip(&gSDFile, path, FA_OPEN_EXISTING | FA_READ))
    {
        // couldn't open file
        char *temp = (char *)buffer;
        char *temp2 = (char *)&buffer[1024];
        utility_w2cstrcpy(temp2, path);
        temp2[32] = '\0';
        utility_strcpy(temp, "!open ");
        utility_strcat(temp, temp2);
        setStatusMessage(temp);
        path[eos] = 0;
        return;
    }
    path[eos] = 0;
    gSelections[entry].length = gSDFile.fsize;

    f_lseek_zip(&gSDFile, 0x7FF0);
    f_read_zip(&gSDFile, rom_hdr, 16, &ts);
    utility_w2cstrcpy(extension, &gSelections[entry].name[utility_wstrlen(gSelections[entry].name) - 4]);
    if (!utility_memcmp(rom_hdr, "TMR SEGA", 8))
    {
        // SMS ROM header
        gSelections[entry].type = 2; // SMS
        gSelections[entry].run = 0x13; // run mode = SMS + FM
        return;
    }
    f_lseek_zip(&gSDFile, 0x7FF0 + 0x0200); // skip possible header
    f_read_zip(&gSDFile, rom_hdr, 16, &ts);
    utility_w2cstrcpy(extension, &gSelections[entry].name[utility_wstrlen(gSelections[entry].name) - 4]);
    if (!utility_memcmp(rom_hdr, "TMR SEGA", 8) || !utility_memcmp(extension, ".sms", 4) || !utility_memcmp(extension, ".SMS", 4))
    {
        // SMS ROM header or file extension
        gSelections[entry].type = 2; // SMS
        gSelections[entry].run = 0x13; // run mode = SMS + FM
        return;
    }
    utility_w2cstrcpy(extension, &gSelections[entry].name[utility_wstrlen(gSelections[entry].name) - 3]);
    if (!utility_memcmp(extension, ".SG", 3) || !utility_memcmp(extension, ".sg", 3))
    {
        // SG-1000 ROM extension
        gSelections[entry].type = 2; // SMS
        gSelections[entry].run = 0x12; // run mode = SMS
        return;
    }

    if (gSelections[entry].length <= 0x200200)
        gSelections[entry].run = 1; // 16 Mbit + SRAM
    else if (gSelections[entry].length <= 0x400200)
        gSelections[entry].run = 2; // 32 Mbit + SRAM
    else
        gSelections[entry].run = 4; // 40 Mbit

    f_lseek_zip(&gSDFile, 0);
    f_read_zip(&gSDFile, buffer, 0x2400, &ts);
    if (!utility_memcmp(buffer, "Vgm ", 4))
    {
        int gd3;

        // VGM header
        gSelections[entry].type = 4; // VGM song file
        gSelections[entry].run = 7; // extended mode

        utility_memcpy(rom_hdr, buffer, 4);
        utility_memset(&rom_hdr[0x20], 0x20, 0x30); // fill name with spaces

        gd3 = buffer[20] | (buffer[21]<<8) | (buffer[22]<<16) | (buffer[23]<<24);
        if (gd3)
        {
            int i;
            f_lseek_zip(&gSDFile, gd3 + 20); // start of VGM tag data
            f_read_zip(&gSDFile, buffer, gSelections[entry].length - (gd3 + 20), &ts);
            for (i=0; i<0x30; i++)
            {
                // copy track name
                if (!buffer[12 + i*2])
                    break;
                rom_hdr[0x20 + i] = buffer[12 + i*2];
            }
        }
        else
            utility_memcpy((char *)&rom_hdr[0x20], "No GD3 Tag", 10);


        return;
    }
    else if ((buffer[8] == 0xAA) & (buffer[9] == 0xBB))
    {
        // SMD header
        gSelections[entry].length &= 0xFFFFF000;
        get_smd_hdr(jump);
    }
    else
    {
        // assume binary
        utility_memcpy(rom_hdr, &buffer[0x100], 0x100);
        utility_memcpy(jump, &buffer[0x200], 4);
    }

    if (!utility_memcmp(&rom_hdr[0xC8], "MYTH", 4))
    {
        gSelections[entry].run = (rom_hdr[0xCC]-'0')*10 + (rom_hdr[0xCD]-'0');
        gSelections[entry].bbank = 0;
        gSelections[entry].bsize = (rom_hdr[0xCE]-'0')*10 + (rom_hdr[0xCF]-'0');
        gSelections[entry].type = (jump[0]!=0)&&((jump[3]==0x88)||(jump[3]==0x90)||(jump[3]==0x91)||(memcmp(rom_hdr+0x20,"MARS",4)==0)) ? 1 : 0;
        gMythHdr = 1;
    }
    else if (utility_memcmp(rom_hdr, "SEGA", 4))
    {
        // not MD or 32X binary image
        gSelections[entry].type = 0; // MD
        if (((rom_hdr[0] == 0xFF) && (rom_hdr[1] == 0x04)) || ((rom_hdr[0] == 0xFF) && (rom_hdr[1] == 0x03)))
        {
            // CD BRAM
            gSelections[entry].run = 9;
            gSelections[entry].bbank = 3;
            gSelections[entry].bsize = 16; // 1Mbit
        }
    }
    else
    {
        if((jump[0]!=0)&&((jump[3]==0x88)||(jump[3]==0x90)||(jump[3]==0x91)||(utility_memcmp(rom_hdr+0x20,"MARS",4)==0)))
        {
            gSelections[entry].type = 1; // 32X
            if (gSelections[entry].run < 4)
                gSelections[entry].run = 3;
        }
        else
        {
            gSelections[entry].type = 0; // MD
            if (((rom_hdr[0x20] == 'C') && (rom_hdr[0x21] == 'D')) || ((rom_hdr[0x25] == 'C') && (rom_hdr[0x26] == 'D')))
            {
                // CD BIOS
                if (gSelections[entry].length <= 0x020200)
                {
                    // just CD BIOS
                    gSelections[entry].run = 8;
                }
                else
                {
                    // CD BIOS + BRAM
                    gSelections[entry].run = 10;
                    gSelections[entry].bbank = 3;
                    gSelections[entry].bsize = 16; // 1Mbit
                }
            }
        }
    }

}

void get_sd_directory(int entry)
{
    DIR dir;
    FILINFO fno;
    int ix;

    gSdDetected = 0;

    gMaxEntry = 0;
    if (entry == -1)
    {
        rom_hdr[0] = rom_hdr[1] = 0xFF;

        // get root
        path[0] = (WCHAR)'/';
        path[1] = 0;
        cardType = 0;
        // unmount SD card
        f_mount(1, NULL);
        // init SD card
        cardType = 0x0000;
        if (MMC_disk_initialize() == STA_NODISK)
            return;                     /* couldn't init SD card */
        // mount SD card
        if (f_mount(1, &gSDFatFs))
        {
            //printToScreen("Couldn't mount SD card.", 1, 1, 0x2000);
            //delay(180);
            return;                     /* couldn't mount SD card */
        }
        f_chdrive(1);                   /* make MMC current drive */
        // now check for funky timing by trying to opendir on the root
        if (f_opendir(&dir, path))
        {
            // failed, try funky timing
            cardType = 0x8000;          /* try funky read timing */
            if (MMC_disk_initialize() == STA_NODISK)
                return;                 /* couldn't init SD card */
        }
        gSdDetected = 1;
    }
    else
    {
        // get subdirectory
        if (gSelections[entry].name[0] == (WCHAR)'.')
        {
            // go back one level
            ix = utility_wstrlen(path) - 1;
            if (ix < 0)
                ix = 0;                 /* for safety */
            while (ix > 0)
            {
                if (path[ix] == (WCHAR)'/')
                    break;
                ix--;
            }
            if (ix < 1)
                ix = 1;                 /* for safety */
            path[ix] = 0;
        }
        else
        {
            // go forward one level - add entry name to path
            if (path[utility_wstrlen(path)-1] != (WCHAR)'/')
                utility_c2wstrcat(path, "/");

            utility_wstrcat(path, gSelections[entry].name);
        }
        gSdDetected = 1;
    }
    if (f_opendir(&dir, path))
    {
        gSdDetected = 0;
        if (do_SDMgr())                 /* we knows there's a card, but can't read it */
            return;                     /* user opted not to try to format the SD card */
        if (f_opendir(&dir, path))
            return;                     /* failed again... give up */
        gSdDetected = 1;
    }

    // add parent directory entry if not root
    if (path[1] != 0)
    {
        gSelections[gMaxEntry].type = 128; // directory entry
        utility_c2wstrcpy(gSelections[gMaxEntry].name, "..");
        gMaxEntry++;
    }

    // scan dirctory entries
    for(;;)
    {
        fno.lfname = (WCHAR*)buffer;
        fno.lfsize = 255;
        fno.lfname[0] = 0;
        if (f_readdir(&dir, &fno))
            break;                      /* no more entries in directory (or some other error) */
        if (!fno.fname[0])
            break;                      /* no more entries in directory (or some other error) */
        if (fno.fname[0] == '.')
            continue;                   /* skip links */
        if (fno.lfname[0] == (WCHAR)'.')
            continue;                   /* skip "hidden" files and directories */

        if (fno.fattrib & AM_DIR)
        {
            gSelections[gMaxEntry].type = 128; // directory entry
            if (fno.lfname[0])
                utility_wstrcpy(gSelections[gMaxEntry].name, fno.lfname);
            else
                utility_c2wstrcpy(gSelections[gMaxEntry].name, fno.fname);

            //w2cstrcpy((char*)buffer, fno.lfname);
            //printToScreen(fno.fname, 1, gMaxEntry % 28, 0);
            //printToScreen((char*)buffer, 14, gMaxEntry % 28, 0);
            //delay(60);
        }
        else
        {
            gSelections[gMaxEntry].type = 127; // unknown
            gSelections[gMaxEntry].bbank = 0;
            gSelections[gMaxEntry].bsize = 0;
            gSelections[gMaxEntry].offset = 0; // maybe later use this to skip a header
            gSelections[gMaxEntry].length = fno.fsize;
            if (fno.lfname[0])
                utility_wstrcpy(gSelections[gMaxEntry].name, fno.lfname);
            else
                utility_c2wstrcpy(gSelections[gMaxEntry].name, fno.fname);

            //w2cstrcpy((char*)buffer, fno.lfname);
            //printToScreen(fno.fname, 1, gMaxEntry % 28, 0);
            //printToScreen((char*)buffer, 14, gMaxEntry % 28, 0);
            //delay(60);

            //get_sd_info(gMaxEntry); // slows directory load
        }
        gMaxEntry++;
    }

    sort_entries();
}

int update_sd_display_make_name(int e)//single session
{
    //convert 16bit -> 8bit string
    utility_w2cstrcpy((char*)buffer, gSelections[e].name);

    if (gSelections[e].type == 128)
    {
        utility_strcpy(entrySNameBuf,"[");
        utility_strncat(entrySNameBuf,(const char*)buffer,34);
        entrySNameBuf[35] = '\0';
        utility_strcat(entrySNameBuf,"]");

        return utility_strlen(entrySNameBuf);
    }

    return shortenName(entrySNameBuf, (char*)buffer, 36);
}

void update_sd_display()//quick hack to remove flickering
{
    int x1,x2,x3;

    //Fast update not possible without " "statically" rendered tiles".Reload "map"
    if((gLastEntryIndex == -1) || (gCurEntry == -1) /*|| (gCurMode != MODE_SD)*/ || (gChangedPage)
    || (gLastEntryIndex == gCurEntry) || ((gCurEntry >= gStartEntry) && (gCurEntry <= gStartEntry + 1)) || (gCurEntry >= gMaxEntry) )
    {
        gResponseMsgStatus = 0;
        gUpdate = 1;
        gRomDly = (getClockType()) ? 27 : 37;

        if(gChangedPage)
        {
            for(x1 = 3; x1 < PAGE_ENTRIES + 3; x1++)
                printToScreen(gFEmptyLine,1,x1,0x2000);
        }

        update_display();
        gChangedPage = 0;
        gLastEntryIndex = -1;
        return;
    }

    gResponseMsgStatus = 0;

    x1 = ((gLastEntryIndex > PAGE_ENTRIES) ? (gLastEntryIndex % PAGE_ENTRIES) : gLastEntryIndex);
    x2 = ((gCurEntry > PAGE_ENTRIES) ? (gCurEntry % PAGE_ENTRIES) : gCurEntry);

    //prev
    x3 = update_sd_display_make_name(gLastEntryIndex);
    x1 += 3;
    printToScreen(gFEmptyLine,1,x1,0x2000);
    printToScreen(entrySNameBuf,20 - (x3 >> 1),x1,0x0000);

    //next
    x3 = update_sd_display_make_name(gCurEntry);
    x2 += 3;
    printToScreen(gFEmptyLine,1,x2,0x2000);
    printToScreen(entrySNameBuf,20 - (x3 >> 1),x2,0x2000);
}

void update_display(void)
{
    int ix;
    char temp[64];

    if (gUpdate < 0)
    {
        clear_screen();

        for(ix = 3; ix < PAGE_ENTRIES + 3; ix++)
        {
            //printToScreen(gFEmptyLine,1,ix,0x2000);
            //better render just the listbox border tiles
            printToScreen("\x7c",1,ix,0x2000);
            printToScreen("\x7c",38,ix,0x2000);
        }
    }

    ix = 0;
    gUpdate = 0;
    gCursorX = 1;
    gCursorY = 1;
    put_str("\x80\x82\x82 ", 0x2000);
    gCursorX = 5;
    put_str(gAppTitle, 0x4000);
    gCursorX = 35;
    put_str(" \x82\x82\x85", 0x2000);

    gCursorX = 1;
    gCursorY = 2;
    put_str(gFEmptyLine, 0x2000);

    // list area (if applicable)
    gCursorY = 3;

    if (gCurMode == MODE_USB)
    {
            for (ix=0; ix<PAGE_ENTRIES; ix++, gCursorY++)
                put_str(gFEmptyLine, 0x2000);
            gCursorX = 13;
            gCursorY = 3;
            if (gCurUSB)
                put_str(" USB Active ", 0x2000);
            else
                put_str("USB Inactive", 0x2000);
    }
    else
    {
        if (gMaxEntry)
        {
            int lines = ((gMaxEntry - gStartEntry) > PAGE_ENTRIES) ? PAGE_ENTRIES : (gMaxEntry - gStartEntry);
            int len;

            for (ix = 0; ix < lines; ix++)
            {
                utility_w2cstrcpy((char*)buffer, gSelections[gStartEntry + ix].name);

                // put centered name
                if (gSelections[gStartEntry + ix].type == 128)
                {
                    utility_strcpy(temp, "[");
                    utility_strncat(temp, (const char *)buffer, 34);
                    temp[35] = '\0';
                    utility_strcat(temp, "]"); // show directories in brackets
                    len = utility_strlen(temp);
                }
                else
                {
                    //file
                    len = shortenName(temp, (char *)buffer, 36);
                }

                printToScreen(temp,20 - (len >> 1),3 + ix,((gStartEntry + ix) == gCurEntry) ? 0x2000 : 0x0000);
                printToScreen("\x7c",1,3 + ix,0x2000);printToScreen("\x7c",38,3 + ix,0x2000);
            }

            //make sure that list border tiles are visible
            for(;lines < PAGE_ENTRIES; lines++,ix++)
            {
                printToScreen("\x7c",1,3 + ix,0x2000);
                printToScreen("\x7c",38,3 + ix,0x2000);
            }
        }
        else
        {
            for (ix=0; ix<PAGE_ENTRIES; ix++, gCursorY++)
                put_str(gFEmptyLine, 0x2000);

            gCursorX = 15;
            gCursorY = 3;
            put_str("No Entries", 0x2000);
        }
    }


//  sprintf(temp, " %02d:%01d%01d:%01d%01d ", rtc[4]&31, rtc[3]&7, rtc[2]&15, rtc[1]&7, rtc[0]&15);
//  gCursorX = 20 - utility_strlen(temp)/2;     /* center time */
//  put_str(temp, 0);

    {
#if 1
        utility_strcpy(temp, " CPLD V");
        UTIL_IntegerToString(&temp[7], gCpldVers, 10);
        if (IS_FLASH)
        {
            if (gCart.GameDev == 0x8810)
                utility_strcat(temp, " / 256Kb Type A ");
            else if (gCart.GameDev == 0x8815)
                utility_strcat(temp, " / 256Kb Type B ");
            else if (gCart.GameDev == 0x880D)
                utility_strcat(temp, " / 512Kb Type C ");
            else
                sprintf(temp[strlen(temp)], " / %04X:%04X ", gCart.GameMan, gCart.GameDev);
        }
        else if (IS_NEO2SD)
            utility_strcat(temp, " / Neo2-SD ");
        else if (IS_NEO2PRO)
            utility_strcat(temp, " / Neo2-Pro ");
        else if (IS_NEO3)
            utility_strcat(temp, " / Neo3-SD ");
        else
            sprintf(temp[strlen(temp)], " / %04X:%04X ", gCart.GameMan, gCart.GameDev);
#else
        sprintf(temp, "%02X%02X%02X%02X%02X:%02X:%02X:V%d:%04X/%04X:%04X/%04X",
                gCart.Magic[0], gCart.Magic[1], gCart.Magic[2], gCart.Magic[3], gCart.Magic[4], gCart.Neo2,
                gCart.Neo3, gCart.Cpld, gCart.MenuMan, gCart.MenuDev, gCart.GameMan, gCart.GameDev);
#endif
        //batch print
        gCursorX = 1;
        gCursorY = 18;
        put_str(gFEmptyLine, 0x2000);

        gCursorX = 1;
        gCursorY = 19;
        put_str(gFBottomLine, 0x2000);

        gCursorX = 20 - (utility_strlen(temp) >> 1);
        put_str(temp, 0);
    }

    // info area
    gCursorX = 1;
    gCursorY = 20;
    // erase line
    put_str(gEmptyLine, 0);
    gCursorX = 1;
    gCursorY = 21;
    // erase line
    put_str(gEmptyLine, 0);
    gCursorX = 1;
    gCursorY = 22;
    // erase line
    put_str(gEmptyLine, 0);
    gCursorX = 1;
    gCursorY = 23;
    // erase line
    put_str(gEmptyLine, 0);
    if (gCardOkay == -1)
    {
        // no Neo2 card found
        gCursorX = 6;
        put_str("Neo2 Flash Card not found!", 0x4000);
    }
    else
    {
        // menu entry info
        // get info for current entry
        if (gCurMode == MODE_SD)
        {
            if ( (gMaxEntry) && (!gRomDly))
            {
                printToScreen(gEmptyLine,1,20,0x0000);
                printToScreen(gEmptyLine,1,21,0x0000);
                printToScreen("Waiting for response...",20 - (utility_strlen("Waiting for response...") >> 1),21,0x2000);
                printToScreen(gEmptyLine,1,22,0x0000);
                printToScreen(gEmptyLine,1,23,0x0000);
                get_sd_info(gCurEntry); /* also loads rom_hdr */
            }
        }
        else if (gCurMode == MODE_FLASH)
        {
            if (gMaxEntry && !gRomDly)
            {
                int fstart;
                fstart = gSelections[gCurEntry].offset;
                gFileType = 0;
                // Get info for current game flash entry
                ints_off(); /* disable interrupts */
                if (gSelections[gCurEntry].type == 2)
                    neo_copy_game(rom_hdr, fstart+0x7FF0, 16);
                else if (gSelections[gCurEntry].type == 4)
                {
                    int gd3;

                    neo_copy_game(buffer, fstart, 0x2400);

                    utility_memcpy(rom_hdr, buffer, 4);
                    utility_memset(&rom_hdr[0x20], 0x20, 0x30); // fill name with spaces

                    gd3 = buffer[20] | (buffer[21]<<8) | (buffer[22]<<16) | (buffer[23]<<24);
                    if (gd3)
                    {
                        int i;
                        neo_copy_game(buffer, fstart + gd3 + 20, gSelections[gCurEntry].length - (gd3 + 20));
                        for (i=0; i<0x30; i++)
                        {
                            // copy track name
                            if (!buffer[12 + i*2])
                                break;
                            rom_hdr[0x20 + i] = buffer[12 + i*2];
                        }
                    }
                    else
                        utility_memcpy((char *)&rom_hdr[0x20], "No GD3 Tag", 10);
                }
                else
                    neo_copy_game(rom_hdr, fstart+0x100, 256);
                ints_on(); /* enable interrupts */
            }
        }
        else
        {
            gRomDly = 0; // always load info immediately in USB mode
            // Get info for USB transfer
            gFileType = 0;
            ints_off(); /* disable interrupts */
            neo_copy_game(rom_hdr, 0x7FF0, 16);
            if (!utility_memcmp(rom_hdr, "TMR SEGA", 8))
            {
                // SMS ROM header
                gSelections[0].type = 2; // SMS
                gSelections[0].run = 0x13; // run mode = SMS + FM
                gSelections[0].length = 0x100000; // 8Mbit
                gSelections[0].bsize = 0;
                gSelections[0].bbank = 0;
                utility_c2wstrcpy(gSelections[0].name, "SMS ROM");
            }
            else
            {
                gSelections[0].run = 6; // 32 Mbit no save
                gSelections[0].length = 0x400000; // 32Mbit
                gSelections[0].bsize = 0;
                gSelections[0].bbank = 0;
                neo_copy_game(rom_hdr, 0x100, 256);
                if (utility_memcmp(rom_hdr, "SEGA", 4))
                {
                    // not MD or 32X binary image - assume old MD
                    gSelections[0].type = 0; // MD
                    utility_c2wstrcpy(gSelections[0].name, "MD ROM");
                }
                else
                {
                    unsigned char jump[4];
                    neo_copy_game(jump, 0x200, 4);
                    if ((jump[0]!=0)&&((jump[3]==0x88)||(jump[3]==0x90)||(jump[3]==0x91)||(utility_memcmp(rom_hdr+0x20,"MARS",4)==0)))
                        gSelections[0].type = 1; // 32X
                    else
                        gSelections[0].type = 0; // MD
                    // get name from rom header
                    utility_memcpy(buffer, &rom_hdr[0x20], 0x30);
                    for (ix=47; ix>0; ix--)
                        if (buffer[ix] != 0x20)
                            break;
                    if (ix == 0)
                    {
                        // no domestic name, use export name
                        utility_memcpy(buffer, &rom_hdr[0x50], 0x30);
                        for (ix=47; ix>0; ix--)
                            if (buffer[ix] != 0x20)
                                break;
                    }
                    if (ix > 30)
                        ix = 30; // max string size
                    buffer[ix+1] = 0; // null terminate name
                    utility_c2wstrcpy(gSelections[0].name, (char*)buffer);
                }
            }
            ints_on(); /* enable interrupts */
        }

        if (gMaxEntry)
        {
            if (gSelections[gCurEntry].type == 127)
            {
                gCursorX = 1;
                gCursorY = 20;
                put_str("Type=Unknown", 0);
            }
            else if (gSelections[gCurEntry].type == 128)
            {
                gCursorX = 1;
                gCursorY = 20;
                put_str("Type=DIR", 0);
            }
            else if (gSelections[gCurEntry].type == 4)
            {
                gCursorX = 1;
                gCursorY = 20;
                put_str("Type=VGM", 0);
                utility_strcpy(temp, "Size=");
                UTIL_IntegerToString(&temp[5], gSelections[gCurEntry].length, 10);
                gCursorX = 39 - utility_strlen(temp);   // right justify string
                put_str(temp, 0);
            }
            else
            {
                gCursorX = 1;
                gCursorY = 20;
                utility_strcpy(temp, "Type=");
                utility_strcat(temp, (gSelections[gCurEntry].type == 0) ? "MD" : (gSelections[gCurEntry].type == 1) ? "32X" : "SMS");
                put_str(temp, 0);
                gCursorX = 12;
                utility_strcpy(temp, "Offset=0x");
                UTIL_IntegerToString(&temp[9], gSelections[gCurEntry].offset, 16);
                put_str(temp, 0);
                utility_strcpy(temp, "Size=");
                UTIL_IntegerToString(&temp[5], gSelections[gCurEntry].length/131072, 10);
                utility_strcat(temp, "Mb");
                gCursorX = 39 - utility_strlen(temp);   // right justify string
                put_str(temp, 0);

                printToScreen(gEmptyLine,1,21,0x0000);
                gCursorX = 1;
                gCursorY = 21;
                utility_strcpy(temp, "Run=0x");
                UTIL_IntegerToString(&temp[6], gSelections[gCurEntry].run, 16);
                put_str(temp, 0);
                gCursorX = 12;
                if (gSelections[gCurEntry].run == 5)
                    utility_strcpy(temp, "SRAM=EEPROM");
                else
                {
                    utility_strcpy(temp, "SRAM Bank=");
                    UTIL_IntegerToString(&temp[10],  gSelections[gCurEntry].bbank, 10);
                }
                put_str(temp, 0);
                utility_strcpy(temp, "Size=");
                UTIL_IntegerToString(&temp[5], (gSelections[gCurEntry].run == 5) ? 1: gSelections[gCurEntry].bsize*64, 10);
                utility_strcat(temp, "Kb");
                gCursorX = 39 - utility_strlen(temp);   // right justify string
                put_str(temp, 0);
            }
        }

        // rom header info (if loaded header)
        if (rom_hdr[0] != 0xFF)
        {
            int start, end;

            if (!utility_memcmp(rom_hdr, "TMR SEGA", 8))
            {
                // SMS ROM header
                utility_strcpy(temp, "Region=");
                switch ((rom_hdr[15] & 0xF0) >> 4)
                {
                    case 3:
                    utility_strcat(temp, "SMS Japan");
                    break;
                    case 4:
                    utility_strcat(temp, "SMS Export");
                    break;
                    case 5:
                    utility_strcat(temp, "GG Japan");
                    break;
                    case 6:
                    utility_strcat(temp, "GG Export");
                    break;
                    case 7:
                    utility_strcat(temp, "GG Intl");
                    break;
                    default:
                    utility_strcat(temp, "unknown");
                }
                gCursorX = 1;   // left justified
                gCursorY = 22;
                put_str(temp, 0);

                utility_strcpy(temp,"Reported Size=");
                switch (rom_hdr[15] & 0x0F)
                {
                    case 0:
                    utility_strcat(temp, "256KB");
                    break;
                    case 1:
                    utility_strcat(temp, "512KB");
                    break;
                    case 2:
                    utility_strcat(temp, "1MB");
                    break;
                    case 10:
                    utility_strcat(temp, "8KB");
                    break;
                    case 11:
                    utility_strcat(temp, "16KB");
                    break;
                    case 12:
                    utility_strcat(temp, "32KB");
                    break;
                    case 13:
                    utility_strcat(temp, "48KB");
                    break;
                    case 14:
                    utility_strcat(temp, "64KB");
                    break;
                    case 15:
                    utility_strcat(temp, "128KB");
                    break;
                    default:
                    utility_strcat(temp, "none");
                }
                gCursorX = 39 - utility_strlen(temp);   // right justify string
                put_str(temp, 0);
            }
            else if (!utility_memcmp(rom_hdr, "Vgm ", 4))
            {
                // VGM song file
                // print track name from header
                utility_memcpy(temp, &rom_hdr[0x20], 0x30);
                for (ix=47; ix>0; ix--)
                    if (temp[ix] != 0x20)
                        break;
                if (ix > 37)
                    ix = 37; // max string size to print
                temp[ix+1] = 0; // null terminate name
                gCursorY = 22;
                gCursorX = 20 - (utility_strlen(temp)>>1); // center name
                put_str(temp, 0);
            }
            else if (!utility_memcmp(rom_hdr, "SEGA", 4))
            {
                // MD/32X ROM header
                // print game name from header
                utility_memcpy(temp, &rom_hdr[0x20], 0x30);
                for (ix=47; ix>0; ix--)
                    if (temp[ix] != 0x20)
                        break;
                if (ix == 0)
                {
                    // no domestic name, use export name
                    utility_memcpy(temp, &rom_hdr[0x50], 0x30);
                    for (ix=47; ix>0; ix--)
                        if (temp[ix] != 0x20)
                            break;
                }
                if (ix > 37)
                    ix = 37; // max string size to print
                temp[ix+1] = 0; // null terminate name
                gCursorY = 22;
                gCursorX = 20 - (utility_strlen(temp)>>1); // center name
                put_str(temp, 0);

                // print rom size and sram size
                gCursorX = 1;
                gCursorY = 23;
                start = rom_hdr[0xA0]<<24|rom_hdr[0xA1]<<16|rom_hdr[0xA2]<<8|rom_hdr[0xA3];
                end = rom_hdr[0xA4]<<24|rom_hdr[0xA5]<<16|rom_hdr[0xA6]<<8|rom_hdr[0xA7];
                utility_strcpy(temp, "ROM Size=");
                UTIL_IntegerToString(&temp[9], (end - start + 1)/131072, 10);
                utility_strcat(temp, "Mbit");
                put_str(temp, 0);
                if ((rom_hdr[0xB0] == 'R') && (rom_hdr[0xB1] == 'A'))
                {
                    start = rom_hdr[0xB4]<<24|rom_hdr[0xB5]<<16|rom_hdr[0xB6]<<8|rom_hdr[0xB7];
                    end = rom_hdr[0xB8]<<24|rom_hdr[0xB9]<<16|rom_hdr[0xBA]<<8|rom_hdr[0xBB];
                }
                else
                {
                    start = end = 0;
                }
                utility_strcpy(temp, "SRAM Size=");
                UTIL_IntegerToString(&temp[10], (end - start + 2)/128, 10);
                utility_strcat(temp, "Kbit");
                gCursorX = 39 - utility_strlen(temp);   // right justify string
                put_str(temp, 0);
            }
            else
            {
                gCursorX = 1;
                gCursorY = 22;
                put_str("      CART header not recognized      ", 0x4000);
            }
        }
    }

    gCursorX = 1;
    gCursorY = 24;
    put_str(gLine, 0x2000);

    // help area
    printToScreen(gEmptyLine, 1, 25, 0x0000);
    printToScreen(gEmptyLine, 1, 26, 0x0000);
    if (gHelp)
    {
        gCursorY = 25;
        gCursorX = 1;
        put_str("A", 0x4000);
        gCursorX = 2;
        put_str((gCurMode == MODE_USB) ? "=Toggle USB" : "=Options", 0);
        gCursorX = 14;
        put_str("B", 0x4000);
        gCursorX = 15;
        put_str("=Run", 0);
        gCursorX = 20;
        put_str("C", 0x4000);
        gCursorX = 21;
        put_str("=Run2", 0);
        gCursorX = 28;
        put_str("St", 0x4000);
        gCursorX = 30;
        put_str((gCurMode == MODE_FLASH) ? "=USB Mode" : (gCurMode == MODE_USB) ? "=SDC Mode" : "=Flash Md", 0);

        if (gCurMode != MODE_USB)
        {
            gCursorY = 26;
            gCursorX = 1;
            put_str("Up", 0x4000);
            gCursorX = 3;
            put_str("=Prev", 0);
            gCursorX = 9;
            put_str("Dn", 0x4000);
            gCursorX = 11;
            put_str("=Next", 0);
            gCursorX = 18;
            put_str("Lt", 0x4000);
            gCursorX = 20;
            put_str("=Prev Pg", 0);
            gCursorX = 29;
            put_str("Rt", 0x4000);
            gCursorX = 31;
            put_str("=Next Pg", 0);
        }
    }
    else
    {
        sprintf(temp, "ASIC Magic: %02X%02X%02X%02X%02X PCB Type: %02X %02X",
                gCart.Magic[0], gCart.Magic[1], gCart.Magic[2], gCart.Magic[3], gCart.Magic[4],
                gCart.Neo2, gCart.Neo3);
        gCursorY = 25;
        gCursorX = 1;
        put_str(temp, 0x0000);
        sprintf(temp, "Flash: Menu[%04X/%04X] Game[%04X/%04X]", gCart.MenuMan, gCart.MenuDev, gCart.GameMan, gCart.GameDev);
        gCursorY = 26;
        gCursorX = 1;
        put_str(temp, 0x0000);
    }
}

void gen_bram(unsigned char *dest, int fstart, int len)
{
    int ix;
    ints_on();          /* enable interrupts */
    for (ix=0; ix<len; ix+=2)
    {
        dest[ix] = 0xFF;
        dest[ix+1] = 0x03;
    }
    ints_off();         /* disable interrupts */
}

void update_progress(char *str1, char *str2, int curr, int total)
{
    static char *cstr1 = NULL, *cstr2 = NULL;
    static int ctotal = 0, div, last;
    int this;

//  if ((cstr1 != str1) || (cstr2 != str2) || (ctotal != total) || (curr < last))
    if ((curr == 0) || (curr == total))
    {
        // new or complete progress bar, recompute divisor and start
        cstr1 = str1;
        cstr2= str2;
        ctotal = total;
        div = (total + 31) / 32;
        last = -1;

        // print empty progress bar
        printToScreen(gEmptyLine,1,20,0x0000);
        printToScreen(gEmptyLine,1,21,0x0000);
        printToScreen(gEmptyLine,1,22,0x0000);
        printToScreen(gEmptyLine,1,23,0x0000);
        printToScreen(str1,20-((strlen(str1)+strlen(str2))>>1),21,0x0000);
        printToScreen(str2,20-((strlen(str2)-strlen(str1))>>1),21,0x2000);
        memset(gProgressBarStaticBuffer, 0x87, 32);
        gProgressBarStaticBuffer[32] = 0;
        printToScreen(gProgressBarStaticBuffer,4,22,0x4000);
    }

    // now print progress
    this = curr/div;
    if (this != last)
    {
        memset(gProgressBarStaticBuffer, 0x87, this-last);
        gProgressBarStaticBuffer[this-last] = 0;
        printToScreen(gProgressBarStaticBuffer,4+last+1,22,0x2000);
        last = this;
    }
}

void copyGame(void (*dst)(unsigned char *buff, int offs, int len), void (*src)(unsigned char *buff, int offs, int len), int doffset, int soffset, int length, char *str1, char *str2)
{
    int ix, iy;

    for (iy=0; iy<length; iy+=XFER_SIZE)
    {
        // fetch data data from source
        if (src)
        {
            (src)(buffer, soffset + iy + (gFileType ? 512 : 0), XFER_SIZE);
            switch (gFileType)
            {
                case 1:
                    // SMD LSB first
                    for (ix=0; ix<8192; ix++)
                    {
                        buffer[XFER_SIZE + ix*2 + 1] = buffer[ix];
                        buffer[XFER_SIZE + ix*2 + 0] = buffer[8192 + ix];
                    }
                    break;
                case 2:
                    // SMD MSB first
                    for (ix=0; ix<8192; ix++)
                    {
                        buffer[XFER_SIZE + ix*2 + 0] = buffer[ix];
                        buffer[XFER_SIZE + ix*2 + 1] = buffer[8192 + ix];
                    }
                    break;
            }
        }
        // store data to destination
        (dst)(&buffer[gFileType ? XFER_SIZE : 0], doffset + iy, XFER_SIZE);
        update_progress(str1, str2, iy, length);
    }

    if ((length == 0x200000) && ((dst == &neo_copyto_myth_psram) || (dst == &neo_sd_to_myth_psram)))
    {
        // clear past end of rom for S&K
        utility_memset(buffer, 0x00, XFER_SIZE);
        for (iy=length; iy<(length + 0x020000); iy+=XFER_SIZE)
            neo_copyto_myth_psram(buffer, doffset + iy, XFER_SIZE);
    }
    else if (gNoAlias && (length < 0x400000) && ((dst == &neo_copyto_myth_psram) || (dst == &neo_sd_to_myth_psram)))
    {
        // pad rom to 4M with 0xFF
        utility_memset(buffer, 0xFF, XFER_SIZE);
        for (iy=length; iy<0x400000; iy+=XFER_SIZE)
            neo_copyto_myth_psram(buffer, doffset + iy, XFER_SIZE);
    }
}

void toggleResetMode(int index)
{
    if (gResetMode)
    {
        gResetMode = 0x0000;
        gOptions[index].value = "Reset to Menu";
    }
    else
    {
        gResetMode = 0x00FF;
        gOptions[index].value = "Reset to Game";
    }

    gCacheOutOfSync = 1;
}

void toggleBankAlias(int index)
{
    if (gNoAlias)
    {
        gNoAlias = 0x0000;
        gOptions[index].value = "ON";
    }
    else
    {
        gNoAlias = 0x00FF;
        gOptions[index].value = "OFF";
    }

    gCacheOutOfSync = 1;
}

void toggleSRAMType(int index)
{
    if (gSRAMType)
    {
        gSRAMType = 0;
        gOptions[index].value = "SRAM";
    }
    else
    {
        gSRAMType = 1;
        gSRAMBank = 0;
        gSRAMSize = 0;
        gOptions[index].value = "EEPROM";
    }

    gCacheOutOfSync = 1;
}
/*
int logtwo(int x)
{
    int result = -1;
    while (x > 0)
    {
        result++;
        x >>= 1;
    }
    return result;
}*/

void incSaveRAMSize(int index)
{
    gSRAMSize = gSRAMSize ? gSRAMSize<<1 : 1;

    if (gSRAMSize == 0x20)
        gSRAMSize = 0;

    UTIL_IntegerToString(gSRAMSizeStr, gSRAMSize*64, 10);
    utility_strcat(gSRAMSizeStr, "Kb");
    // remask bank setting
    gSRAMBank = gSRAMBank & (63 >> utility_logtwo(gSRAMSize));

    if (gSRAMBank > 31)
        gSRAMBank = 0; // max of 32 banks since minimum bank size is 8KB sram

    UTIL_IntegerToString(gSRAMBankStr, gSRAMBank, 10);
    gCacheOutOfSync = 1;
}

void incSaveRAMBank(int index)
{
    gSRAMBank = (gSRAMBank + 1) & (63 >> utility_logtwo(gSRAMSize));

    if (gSRAMBank > 31)
        gSRAMBank = 0; // max of 32 banks since minimum bank size is 8KB sram

    UTIL_IntegerToString(gSRAMBankStr, gSRAMBank, 10);
    gCacheOutOfSync = 1;
}

void toggleYM2413(int index)
{
    if (gYM2413)
    {
        gYM2413 = 0x0000;
        gOptions[index].value = "Off";
    }
    else
    {
        gYM2413 = 0x0001;
        gOptions[index].value = "On";
    }

    gCacheOutOfSync = 1;
}

//for the cheat support
void toggleCheat(int index)
{
    CheatEntry* e = (CheatEntry*)gOptions[index].userData;

    if(!e)
        return;

    if(e->active)
    {
        e->active = 0;
        gOptions[index].value = "Off";
    }
    else
    {
        e->active = 1;
        gOptions[index].value = "On";
    }
}

void importCheats(int index)
{
    CheatEntry* e;
    CheatPair* cp;
    short a,b,any_active;

    any_active = 0;

    if(!registeredCheatEntries)
        return;

    //Check for types that require master code
    for(a = 0; a < registeredCheatEntries; a++)
    {
        e = &cheatEntries[a];

        {
            //apply all master codes if exist
            for(b = 0; b < registeredCheatEntries; b++)
            {
                e = &cheatEntries[b];

                if(e->type & CT_MASTER)
                    e->active = 1;
            }
        }
    }

    //patch
    for(a = 0; a < registeredCheatEntries; a++)
    {
        e = &cheatEntries[a];

        if(e->active)
        {
            if(any_active == 0)
            {
                ints_on();
                setStatusMessage("Applying cheats...");
                any_active = 1;
            }
            for(b = 0; b < e->pairs; b++)
            {
                cp = &e->pair[b];

                buffer[0] =  (cp->data & 0xFF00) >> 8;
                buffer[1] =  (cp->data & 0xFF);

                ints_off();
                if (gPSRAM) neo_copyto_myth_psram(buffer,cp->addr,2);
                else neo_copyto_psram(buffer,cp->addr,2);
            }
        }
    }

    ints_on();
    if(any_active == 1)
    {
        setStatusMessage("Applying cheats...OK");
        clearStatusMessage();
    }
}

void toggleIPS(int index)
{
    if(gImportIPS)
    {
        gImportIPS = 0;
        gOptions[index].value = "Off";
    }
    else
    {
        gImportIPS = 1;
        gOptions[index].value = "On";
    }
}

//For the IPS support
void importIPS(int index)
{
    unsigned char in[8];
    unsigned int addr = 0 , len = 0;
    DWORD fsize,fbr;

    if(!gSelectionSize)
        return;

    if(!gImportIPS)
        return;

    if(ipsPath[0] == 0)
        return;

    ints_on();
    setStatusMessage("Importing patch...");

    f_close(&gSDFile);
    if(f_open(&gSDFile, ipsPath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        clearStatusMessage();
        return;
    }

    fsize = gSDFile.fsize;
    fsize -= 3; //adjust for eof marker

    //skip "PATCH"
    neo_copy_sd(in,0,5);
    fbr = 5;

    while(fbr < fsize)
    {
        unsigned int nb, ix = 0;

        neo_copy_sd(in,0,3);
        fbr += 3;
        addr = ((unsigned int)in[0] << 16) | ((unsigned int)in[1] << 8) | (unsigned int)in[2];

        neo_copy_sd(in,0,2);
        fbr += 2;
        len = ((unsigned int)in[0] << 8) | (unsigned int)in[1];

        if (len)
        {
            nb = len;
            if (addr & 1)
            {
                addr--;
                nb++;
                ix = 1;

                if (gPSRAM) neo_copyfrom_myth_psram(buffer, addr, 2);
                else neo_copyfrom_psram(buffer, addr, 2);
            }
            if (nb & 1)
            {
                if (gPSRAM) neo_copyfrom_myth_psram(&buffer[nb-1], addr+nb-1, 2);
                else neo_copyfrom_psram(&buffer[nb-1], addr+nb-1, 2);
                nb++;
            }

            neo_copy_sd(&buffer[ix], 0, len);
            fbr += len;
        }
        else
        {
            neo_copy_sd(in,0,2);
            fbr += 2;
            len = ((unsigned int)in[0] << 8) | (unsigned int)in[1];

            nb = len;
            if (addr & 1)
            {
                addr--;
                nb++;
                ix = 1;

                if (gPSRAM) neo_copyfrom_myth_psram(buffer, addr, 2);
                else neo_copyfrom_psram(buffer, addr, 2);
            }
            if (nb & 1)
            {
                if (gPSRAM) neo_copyfrom_myth_psram(&buffer[nb-1], addr+nb-1, 2);
                else neo_copyfrom_psram(&buffer[nb-1], addr+nb-1, 2);
                nb++;
            }

            neo_copy_sd(in,0,1);
            fbr += 1;
            utility_memset(&buffer[ix], in[0], len);
        }

        if((addr+nb-1) < gSelectionSize)
        {
            if ((addr & 0x00F00000) == ((addr+nb-1) & 0x00F00000))
            {
                // doesn't cross 1MB boundary, copy all at once
                if (gPSRAM) neo_copyto_myth_psram(buffer, addr, nb);
                else neo_copyto_psram(buffer, addr, nb);
            }
            else
            {
                unsigned int len1 = ((addr & 0x00F00000) + 0x00100000) - addr;

                // crosses 1MB boundary, copy up to 1MB boundary
                if (gPSRAM) neo_copyto_myth_psram(buffer, addr, len1);
                else neo_copyto_psram(buffer, addr, len1);

                // copy the rest after the boundary
                if (gPSRAM) neo_copyto_myth_psram(&buffer[len1], addr+len1, nb-len1);
                else neo_copyto_psram(&buffer[len1], addr+len1, nb-len1);
            }
        }
    }

    ints_on();
    setStatusMessage("Importing patch...OK");
    delay(60);
    clearStatusMessage();
}

int patchesNeeded(void)
{
    CheatEntry* e;
    short a;

    if(gImportIPS)
        return 1;

    if(!registeredCheatEntries)
        return 0;

    for(a = 0; a < registeredCheatEntries; a++)
    {
        e = &cheatEntries[a];

        if(e->active)
            return 1;
    }

    return 0;
}

/* CACHE */
    /*
        12B block
        DXCS 4B
        SRAMBANK 2B
        SRAMSIZE 2B
        YM2413 2B
        RESET MODE 2B
    */
void cache_invalidate_pointers()
{
    gSRAMBank = gSRAMSize = gResetMode = gNoAlias = 0x0000;
    gYM2413 = 0x0001;
    gManageSaves = gSRAMgrServiceStatus = 0;

    UTIL_IntegerToString(gSRAMSizeStr, gSRAMSize*64, 10);
    utility_strcat(gSRAMSizeStr, "Kb");
    UTIL_IntegerToString(gSRAMBankStr, gSRAMBank, 10);
}

int cache_process()
{
    unsigned int a = 0 , b = 0;
    int i = 0;
    short int found = 0;

    clearStatusMessage();

    switch(gSelections[gCurEntry].type)
    {
        case 4://vgm
        case 127://unknown
        case 128://dir
            return 0;
    }

    if(gSelections[gCurEntry].run == 0x27)
        return 0;

    if((rom_hdr[0] == 0xFF) && (rom_hdr[1] != 0x04))
    {
        setStatusMessage("Fetching header...");
        get_sd_info(gCurEntry);
        clearStatusMessage();

        if((rom_hdr[0] == 0xFF) && (rom_hdr[1] != 0x04)) //bad!
            return 0;

        switch(gSelections[gCurEntry].type)
        {
            case 4://vgm
            case 127://unknown
            case 128://dir
                return 0;
        }

        if(gSelections[gCurEntry].run == 0x27)
            return 0;
    }

    setStatusMessage("One-time detection in progress...");
    gCacheOutOfSync = 1;

    if ((rom_hdr[0xB0] == 'R') && (rom_hdr[0xB1] == 'A'))
    {
        a = rom_hdr[0xB4]<<24|rom_hdr[0xB5]<<16|rom_hdr[0xB6]<<8|rom_hdr[0xB7];
        b = rom_hdr[0xB8]<<24|rom_hdr[0xB9]<<16|rom_hdr[0xBA]<<8|rom_hdr[0xBB];
    }

    gSRAMSize = 0;

    if(b-a == 1)
    {
        gSRAMType = 0x0001;
        gSRAMSize = 0;
        gManageSaves = 0;
        gSRAMgrServiceStatus = SMGR_STATUS_NULL;//EEPROM unsupported : Use only the internal chip....
    }
    else
    {
        if((b-a) > 2)
        {
            if(((b-a)+2) <= 8192)
                gSRAMSize = 1;
            else
                gSRAMSize = (short int)(( ((b-a)+2) / 1024) / 8);

            gManageSaves = 1;
            gSRAMgrServiceStatus = SMGR_STATUS_BACKUP_SRAM;
        }
        else
        {
            gManageSaves = 0;
            gSRAMgrServiceStatus = SMGR_STATUS_NULL;
        }

        if (((rom_hdr[0] == 0xFF) && (rom_hdr[1] == 0x04)) || ((rom_hdr[0] == 0xFF) && (rom_hdr[1] == 0x03)))
            gSRAMSize = 16; // BRAM file

        gSRAMType = 0x0000;

        if(!gSRAMSize)//check for eeprom
        {
            //intense scan
            for(i = 0; ; i++)
            {
                if(EEPROM_MAPPERS[i] == NULL)
                    break;

                if(!utility_memcmp(rom_hdr + 0x83,EEPROM_MAPPERS[i],utility_strlen(EEPROM_MAPPERS[i])))
                {
                    found = 1;
                    gSRAMType = 0x0001;
                    break;
                }
            }

            if(!found)
            {
                {
                    unsigned char* token = (unsigned char*)&buffer[((XFER_SIZE << 1) - 256)];
                    unsigned char* code = (unsigned char*)&buffer[0];
                    WCHAR* eepInc = (WCHAR*)&buffer[0];
                    short tokenLen = 0;
                    unsigned int bytesToRead = 0;
                    unsigned int bytesWritten = 0;
                    unsigned int addr = 0;
                    FIL f;

                    utility_c2wstrcpy(eepInc,eeprom_inc_path);

                    if(f_open(&f,eepInc, FA_OPEN_EXISTING | FA_READ) == FR_OK)
                    {
                        bytesToRead = f.fsize;

                        if(bytesToRead > ((XFER_SIZE << 1) - 256))
                            bytesToRead = ((XFER_SIZE << 1) - 256);

                        if(f_read(&f,code, bytesToRead, &bytesWritten) == FR_OK)
                        {
                            while((addr < bytesWritten))
                            {
                                while(addr < bytesWritten)
                                {
                                    if(!is_space(code[addr]))
                                        break;

                                    ++addr;
                                }

                                if( ((code[addr] == '\r') || (code[addr] == '\n')) || (addr >= bytesWritten) )
                                {
                                    if(tokenLen)
                                    {
                                        if(!utility_memcmp(rom_hdr + 0x83,token,tokenLen))
                                        {
                                            gSRAMType = 0x0001;
                                            break;
                                        }
                                    }

                                    tokenLen = 0;
                                    ++addr;
                                    continue;
                                }

                                if(tokenLen < 255)
                                {
                                    token[tokenLen++] = code[addr++];
                                    continue;
                                }

                                ++addr;
                            }
                        }

                        f_close(&f);
                    }//parser sec
                }
            }
        }
    }

    UTIL_IntegerToString(gSRAMSizeStr, gSRAMSize*64, 10);
    utility_strcat(gSRAMSizeStr, "Kb");
    UTIL_IntegerToString(gSRAMBankStr, gSRAMBank, 10);

    setStatusMessage("One-time detection in progress...OK");

    clearStatusMessage();
    return 0xFF;
}

void cache_loadPA(WCHAR* sss,int skip_check)
{
    UINT fbr = 0;
    WCHAR* fnbuf;
    WCHAR* fnew;

    if(gCurMode != MODE_SD)
    {
        STEP_INTO("gCurMode != MODE_SD");
        return;
    }

    if(!skip_check)
    {
        switch(gSelections[gCurEntry].type)
        {
            case 4://vgm
            case 127://unknown
            case 128://dir
            {
                STEP_INTO("gSelections[gCurEntry].type == 4/127/128");
                return;
            }
        }

        if(gSelections[gCurEntry].run == 0x27)
        {
            STEP_INTO("gSelections[gCurEntry].run == 0x27");
            return;
        }
    }

    fnbuf = &wstr_buf[gWStrOffs];
    gWStrOffs += 512;

    //setStatusMessage("Reading cache...");
    utility_memset_psram(fnbuf,0,512);

    ints_off();
    utility_c2wstrcpy(fnbuf,"/");
    utility_c2wstrcat(fnbuf,CACHE_DIR);

    utility_c2wstrcat(fnbuf,"/");
    utility_wstrcat(fnbuf,sss);

    fnew = get_file_ext(fnbuf);
    if(*fnew == (WCHAR)'.')
        *fnew = 0;

    utility_c2wstrcat(fnbuf,".dxcs");

    //utility_w2cstrcpy((char*)&buffer[(XFER_SIZE*2)-256],fnbuf);
    //setStatusMessage((const char*)&buffer[(XFER_SIZE*2)-256]);
    //delay(100);
    ints_on();
    f_close(&gSDFile);
    ints_on();

    if(f_open(&gSDFile,fnbuf,FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        STEP_INTO("f_open(&gSDFile,fnbuf,FA_OPEN_EXISTING | FA_READ) != FR_OK");
        clearStatusMessage();
        cache_invalidate_pointers();
        gCacheBlock.processed = cache_process();
        cache_sync(0);
        gWStrOffs -= 512;
        return;
    }

    if(gSDFile.fsize != sizeof(CacheBlock))
    {
        STEP_INTO("gSDFile.fsize != sizeof(CacheBlock)");
        clearStatusMessage();
        f_close(&gSDFile);
        gWStrOffs -= 512;
        return;
    }
    ints_on();
    //changed it to block r/w
    f_read(&gSDFile,(char*)&gCacheBlock,sizeof(CacheBlock),&fbr);

    if(gCacheBlock.version != CACHEBLK_VERS)
    {
        STEP_INTO("Cache Block version mismatch");
        clearStatusMessage();
        f_close(&gSDFile);
        gWStrOffs -= 512;
        return;
    }

    gSRAMBank = (short int)gCacheBlock.sramBank;
    gSRAMSize = gCacheBlock.sramSize;
    gYM2413 = gCacheBlock.YM2413;
    gResetMode =  gCacheBlock.resetMode;
    gNoAlias =  gCacheBlock.aliasMode;
    gSRAMType = (short int)gCacheBlock.sramType;
    gManageSaves = (short int)gCacheBlock.autoManageSaves;
    gSRAMgrServiceStatus = (short int)gCacheBlock.autoManageSavesServiceStatus;

    f_close(&gSDFile);

    UTIL_IntegerToString(gSRAMSizeStr, gSRAMSize*64, 10);
    utility_strcat(gSRAMSizeStr, "Kb");
    UTIL_IntegerToString(gSRAMBankStr, gSRAMBank, 10);

    if(gCacheBlock.processed != 0xFF)
    {
        STEP_INTO("gCacheBlock.processed != 0xFF");
        clearStatusMessage();
        gCacheBlock.processed = cache_process();
        cache_sync(0);
    }

    clearStatusMessage();
    ints_on();
    gWStrOffs -= 512;
}

void cache_load()
{
    switch(gSelections[gCurEntry].type)
    {
        case 4://vgm
        case 127://unknown
        case 128://dir
            return;
    }

    if(gSelections[gCurEntry].run == 0x27)
        return;

    cache_loadPA(gSelections[gCurEntry].name,0);
}

void cache_sync(int root)
{
    UINT fbr = 0;
    WCHAR* fnbuf = &wstr_buf[gWStrOffs];
    WCHAR* fnew;

    if(gCurMode != MODE_SD)
        return;

    switch(gSelections[gCurEntry].type)
    {
        case 4://vgm
        case 127://unknown
        case 128://dir
            return;
    }

    if(gSelections[gCurEntry].run == 0x27)
        return;

    if(!gCacheOutOfSync)
        return;

    gCacheOutOfSync = 0;
    gWStrOffs += 512;

    ints_on();
    utility_memset_psram(fnbuf,0,512);
    utility_c2wstrcpy(fnbuf,"/");
    utility_c2wstrcat(fnbuf,CACHE_DIR);

    if(!createDirectory(fnbuf))
    {
        gWStrOffs -= 512;
        return;
    }

    setStatusMessage("Writing cache...");

    utility_c2wstrcat(fnbuf,"/");
    utility_wstrcat(fnbuf,gSelections[gCurEntry].name);

    fnew = get_file_ext(fnbuf);
    if(*fnew == (WCHAR)'.')
        *fnew = 0;

    utility_c2wstrcat(fnbuf,".dxcs");

    f_close(&gSDFile);
    deleteFile(fnbuf);

    if(f_open(&gSDFile,fnbuf,FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        clearStatusMessage();
        gWStrOffs -= 512;
        return;
    }

    ints_on();

    //gCacheBlock.processed = 0;
    gCacheBlock.sramBank = (unsigned char)gSRAMBank;

    if(gManageSaves)
        gCacheBlock.autoManageSaves = 1;
    else
        gCacheBlock.autoManageSaves = 0;

    if(gSRAMgrServiceStatus == SMGR_STATUS_NULL )
        gCacheBlock.autoManageSavesServiceStatus = SMGR_STATUS_NULL;
    else
        gCacheBlock.autoManageSavesServiceStatus = SMGR_STATUS_BACKUP_SRAM;

    //gCacheBlock.autoManageSaves = (unsigned char)gManageSaves;
    //gCacheBlock.autoManageSavesServiceStatus = (unsigned char)gSRAMgrServiceStatus;
    gCacheBlock.sramSize = gSRAMSize;
    gCacheBlock.YM2413 = gYM2413;
    gCacheBlock.resetMode = gResetMode;
    gCacheBlock.aliasMode = gNoAlias;
    gCacheBlock.sramType = (unsigned char)gSRAMType;

    f_write(&gSDFile,(char*)&gCacheBlock,sizeof(CacheBlock),&fbr);

    f_close(&gSDFile);

    UTIL_IntegerToString(gSRAMSizeStr, gSRAMSize*64, 10);
    utility_strcat(gSRAMSizeStr, "Kb");
    UTIL_IntegerToString(gSRAMBankStr, gSRAMBank, 10);

    ints_on();

    clearStatusMessage();

    if( (gCacheBlock.processed != 0xFF) && (root < 2) )
    {
        gCacheBlock.processed = cache_process();
        cache_sync(root + 1);
    }

    clearStatusMessage();
    gWStrOffs -= 512;
}

/*SRAM manager*/
void sram_mgr_toggleService(int index)
{
    ints_on();

    setStatusMessage("Updating configuration...");

    if(gManageSaves)
    {
        gOptions[index].value = "Disabled";
        gManageSaves = 0;
        gCacheOutOfSync = 1;
        //config_push("manageSaves","0");//will be replaced if exists
    }
    else
    {
        gOptions[index].value = "Enabled";
        gManageSaves = 1;
        gCacheOutOfSync = 1;
        //config_push("manageSaves","1");//will be replaced if exists
    }

    //updateConfig();

    ints_on();

    cache_sync(0);

    ints_on();
    setStatusMessage("Updating configuration...OK");
    clearStatusMessage();
}

void sram_mgr_saveGamePA(WCHAR* sss)
{
    UINT fbr = 0;
    WCHAR* fnbuf = &wstr_buf[gWStrOffs];
    WCHAR* fnew;
    int sramLength,sramBankOffs,k,i,tw;

    //dont let this happen
    if(!gSRAMSize)
    {
        STEP_INTO("GSRAMSIZE == 0");
        return;
    }

    gWStrOffs += 512;

    ints_on();
    utility_memset(fnbuf,0,512);

    sramLength = gSRAMSize * 4096;//actual myth space occupied, not counting even bytes
    if(gSRAMBank)
        sramBankOffs = gSRAMBank * max(sramLength,8192);//minimum bank size is 8KB, not 4KB
    else
        sramBankOffs = 0;

    utility_c2wstrcpy(fnbuf,"/");
    utility_c2wstrcat(fnbuf,SAVES_DIR);

    if(!createDirectory(fnbuf))
    {
        STEP_INTO("createDirectory(fnbuf) == 0");
        gWStrOffs -= 512;
        return;
    }

    utility_c2wstrcat(fnbuf,"/");
    utility_wstrcat(fnbuf,sss);

    fnew = get_file_ext(fnbuf);
    if(*fnew == (WCHAR)'.')
        *fnew = 0;

    if((gSRAMgrServiceMode==SMGR_MODE_SMS) || (gSRAMgrServiceMode==SMGR_MODE_BRAM))
    {
        STEP_INTO("(gSRAMgrServiceMode==SMGR_MODE_SMS||gSRAMgrServiceMode==SMGR_MODE_BRAM)");
        //sms or bram
        if(gSRAMgrServiceMode==SMGR_MODE_SMS)
        {
            utility_c2wstrcat(fnbuf,SMS_SAVE_EXT);
        }
        else
        {
            utility_c2wstrcat(fnbuf,BRM_SAVE_EXT);//or .crm
        }
    }
    else
    {
        STEP_INTO("NOT (gSRAMgrServiceMode==SMGR_MODE_SMS||gSRAMgrServiceMode==SMGR_MODE_BRAM)");
        utility_c2wstrcat(fnbuf,MD_32X_SAVE_EXT);
    }

    //if exists - delete
    deleteFile(fnbuf);

    f_close(&gSDFile);
    delay(80);
    ints_on();

    if(f_open(&gSDFile,fnbuf,FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        STEP_INTO("f_open(&gSDFile,fnbuf,FA_CREATE_ALWAYS | FA_WRITE) != FR_OK");
        gWStrOffs -= 512;
        return;
    }

    setStatusMessage("Backing up GAME sram...");

    if((gSRAMgrServiceMode==SMGR_MODE_SMS) || (gSRAMgrServiceMode==SMGR_MODE_BRAM))
    {
        STEP_INTO("(gSRAMgrServiceMode==SMGR_MODE_SMS||gSRAMgrServiceMode==SMGR_MODE_BRAM)");
        gSRAMgrServiceMode = 0x0000;

        //sms or bram - direct copy
        k=0;
        while(sramLength)
        {
            tw=min(sramLength,XFER_SIZE * 2);

            ints_off();
            neo_copyfrom_sram(buffer,sramBankOffs+k,tw);
            ints_on();
            f_write(&gSDFile,(char*)buffer,tw,&fbr);

            k+=tw;
            sramLength-=tw;
        }
    }
    else
    {
        STEP_INTO("NOT (gSRAMgrServiceMode==SMGR_MODE_SMS||gSRAMgrServiceMode==SMGR_MODE_BRAM)");
        gSRAMgrServiceMode = 0x0000;

        //md or 32x - store even bytes
        k=0;
        while(sramLength)
        {
            tw=min(sramLength,XFER_SIZE);

            ints_off();
            neo_copyfrom_sram(buffer,sramBankOffs+k,tw);
            ints_on();
            for(i=tw-1;i>=0;i--)
            {
                buffer[i*2+1]=buffer[i];
                buffer[i*2+0]=0;
            }
            f_write(&gSDFile,(char*)buffer,tw*2,&fbr);

            k+=tw;
            sramLength-=tw;
        }
    }

    ints_on();
    f_close(&gSDFile);

    setStatusMessage("Backing up GAME sram...OK!");
    clearStatusMessage();
    gWStrOffs -= 512;
}

void sram_mgr_saveGame(int index)
{
    gSRAMgrServiceMode = (gSelections[gCurEntry].type == 2) ? SMGR_MODE_SMS : (gSelections[gCurEntry].run < 8) ? SMGR_MODE_MD32X : SMGR_MODE_BRAM;
    sram_mgr_saveGamePA(gSelections[gCurEntry].name);
    gSRAMgrServiceMode = 0x0000;
}

void sram_mgr_restoreGame(int index)
{
    UINT fbr = 0;
    WCHAR* fnbuf = &wstr_buf[gWStrOffs];
    WCHAR* fnew;
    int sramLength,sramBankOffs,k,i,tr;

    if(!gSRAMSize)
        return;

    gWStrOffs += 512;

    ints_on();
    utility_memset_psram(fnbuf,0,512);
    utility_c2wstrcpy(fnbuf,"/");
    utility_c2wstrcat(fnbuf,SAVES_DIR);

    utility_c2wstrcat(fnbuf,"/");
    utility_wstrcat(fnbuf,gSelections[gCurEntry].name);

    sramLength = gSRAMSize * 4096;//actual myth space occupied, not counting even bytes
    if(gSRAMBank)
        sramBankOffs = gSRAMBank * max(sramLength,8192);//minimum bank size is 8KB, not 4KB
    else
        sramBankOffs = 0;

    fnew = get_file_ext(fnbuf);
    if(*fnew == (WCHAR)'.')
        *fnew = 0;

    if((gSelections[gCurEntry].run >= 9) && (gSelections[gCurEntry].run <= 13))
    {
        //sms or bram
        if(gSelections[gCurEntry].type==2)
        {
            utility_c2wstrcat(fnbuf,SMS_SAVE_EXT);
        }
        else
        {
            utility_c2wstrcat(fnbuf,BRM_SAVE_EXT);//or .crm
        }
    }
    else
    {
        utility_c2wstrcat(fnbuf,MD_32X_SAVE_EXT);
    }

    f_close(&gSDFile);

    if(f_open(&gSDFile,fnbuf,FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        gWStrOffs -= 512;
        return;
    }

    if(!gSDFile.fsize)
    {
        f_close(&gSDFile);
        gWStrOffs -= 512;
        return;
    }

    setStatusMessage("Restoring GAME sram...");

    if((gSelections[gCurEntry].run >= 9) && (gSelections[gCurEntry].run <= 13))
    {
        //sms or bram - direct copy
        sramLength = min(sramLength,gSDFile.fsize);
        k=0;
        while(sramLength)
        {
            tr=min(sramLength,XFER_SIZE * 2);

            f_read(&gSDFile,(char*)buffer,tr,&fbr);
            ints_off();
            neo_copyto_sram(buffer,sramBankOffs+k,tr);
            ints_on();

            k+=tr;
            sramLength-=tr;
        }
    }
    else
    {
        //md or 32x - skip even bytes
        sramLength = min(sramLength,gSDFile.fsize/2);
        k=0;
        while(sramLength)
        {
            tr=min(sramLength,XFER_SIZE);

            f_read(&gSDFile,(char*)buffer,tr*2,&fbr);
            for(i=0;i<tr;i++)
            {
                buffer[i]=buffer[i*2+1];
            }
            ints_off();
            neo_copyto_sram(buffer,sramBankOffs+k,tr);
            ints_on();

            k+=tr;
            sramLength-=tr;
        }
    }
    ints_on();
    f_close(&gSDFile);

    setStatusMessage("Restoring GAME sram...OK!");

    clearStatusMessage();
    gWStrOffs -= 512;
}

void sram_mgr_saveAll(int index)
{
    WCHAR* fss = &wstr_buf[gWStrOffs];
    UINT fsize = 0 , i = 0 , fbr = 0;

    gWStrOffs += 512;

    ints_on();
    setStatusMessage("Working...");

    utility_memset(fss,0,512);

    utility_c2wstrcpy(fss,"/");
    utility_c2wstrcat(fss,SAVES_DIR);

    if(!createDirectory(fss))
    {
        clearStatusMessage();
        gWStrOffs -= 512;
        return;
    }

    utility_c2wstrcat(fss,"/SRAM");
    utility_c2wstrcat(fss,MD_32X_SAVE_EXT);

    deleteFile(fss);//delete if exists

    f_close(&gSDFile);

    if(f_open(&gSDFile, fss , FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        gWStrOffs -= 512;
        return;
    }

    fsize = gSDFile.fsize;

    while( i < 2 * MB)
    {
        update_progress("Saving ALL SRAM.."," ",i,((XFER_SIZE * 2) * 4) * 2);

        ints_off();
        neo_copyfrom_sram(buffer,i,XFER_SIZE * 2);
        ints_on();
        f_write(&gSDFile,(char*)buffer,(XFER_SIZE * 2), &fbr);;

        i += (XFER_SIZE * 2);
    }
    ints_on();
    f_close(&gSDFile);
    update_progress("Saving ALL SRAM.."," ",100,100);
    clearStatusMessage();
    gWStrOffs -= 512;
}

void sram_mgr_restoreAll(int index)
{
    WCHAR* fss = &wstr_buf[gWStrOffs];
    UINT fsize = 0 , i = 0 , fbr = 0;

    gWStrOffs += 512;

    ints_on();
    setStatusMessage("Working...");

    utility_memset(fss,0,512);
    utility_c2wstrcpy(fss,"/");
    utility_c2wstrcat(fss,SAVES_DIR);

    utility_c2wstrcat(fss,"/SRAM");
    utility_c2wstrcat(fss,MD_32X_SAVE_EXT);

    f_close(&gSDFile);

    if(f_open(&gSDFile,fss, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        gWStrOffs -= 512;
        return;
    }

    fsize = gSDFile.fsize;

    if(fsize < 2 * MB)
    {
        f_close(&gSDFile);
        gWStrOffs -= 512;
        return;
    }

    while( i < 2 * MB)
    {
        ints_on();
        update_progress("Restoring ALL SRAM.."," ",i,2 * MB);
        f_read(&gSDFile,(char*)buffer,(XFER_SIZE * 2), &fbr);
        ints_off();
        neo_copyto_sram(buffer,i,XFER_SIZE * 2);

        i += (XFER_SIZE * 2);
    }
    ints_on();
    f_close(&gSDFile);
    update_progress("Restoring ALL SRAM.."," ",100,100);
    clearStatusMessage();
    gWStrOffs -= 512;
}

void sram_mgr_clearGame(int index)
{
    int i = 0,tw;
    int sramLength = gSRAMSize * 4096;//actual myth space occupied, not counting even bytes
    const int sramBankOffs = gSRAMBank * max(sramLength,8192);//minimum bank size is 8KB, not 4KB

    ints_on();
    setStatusMessage("Clearing GAME sram...");

    utility_memset((char*)buffer,0x0,XFER_SIZE * 2);

    while(sramLength)//write  32KB blocks
    {
        tw=min(sramLength,XFER_SIZE * 2);
        ints_off();
        neo_copyto_sram(buffer,sramBankOffs+i,tw);
        ints_on();

        sramLength-=tw;
        i+=tw;
    }

    ints_on();
    setStatusMessage("Clearing GAME sram...OK!");

    clearStatusMessage();
}

void sram_mgr_clearAll(int index)
{
    int i = 0;

    ints_on();
    setStatusMessage("Clearing ALL SRAM...");

    utility_memset((char*)buffer,0x0,XFER_SIZE * 2);

    while( i < 2 * MB)//write  32KB blocks
    {
        update_progress("Clearing ALL SRAM..."," ",i,2 * MB);

        ints_off();
        neo_copyto_sram(buffer,i,XFER_SIZE * 2);
        ints_on();

        i += (XFER_SIZE * 2);
    }

    ints_on();
    update_progress("Clearing ALL SRAM..."," ",100,100);
    setStatusMessage("Clearing ALL SRAM...OK");

    clearStatusMessage();
}

void sram_mgr_copyGameToNextBank(int index)
{
    int sramLength,sramBankOffs,oldBankOffs,k,d;
    short int targetBank;

    ints_on();

    targetBank = gSRAMBank + 1;
    // check for wrap-around
    targetBank = targetBank & (63 >> utility_logtwo(gSRAMSize));
    if (targetBank > 31)
        targetBank = 0; // max of 32 banks since minimum bank size is 8KB sram

    sramLength = gSRAMSize * 4096;//actual myth space occupied, not counting even bytes
    sramBankOffs = targetBank * max(sramLength,8192);
    oldBankOffs = gSRAMBank * max(sramLength,8192);

    //copy
    setStatusMessage("Copying SRAM to next bank...");


    if(sramLength <= XFER_SIZE * 2)
    {
        ints_off();
        neo_copyfrom_sram(buffer,oldBankOffs,sramLength);
        ints_off();
        neo_copyto_sram(buffer,sramBankOffs,sramLength);
    }
    else
    {
        if(sramLength <= (XFER_SIZE * 2) * 2)//64KB
        {
            k = XFER_SIZE * 2;
            d = sramLength  - k;

            //1st block
            ints_off();
            neo_copyfrom_sram(buffer,oldBankOffs,k);
            ints_off();
            neo_copyto_sram(buffer,sramBankOffs,k);

            //2nd block
            ints_off();
            neo_copyfrom_sram(buffer,oldBankOffs + k,d);
            ints_off();
            neo_copyto_sram(buffer,sramBankOffs + k,d);
        }
        else if( (sramLength > (XFER_SIZE * 2) * 2) && (sramLength <= (XFER_SIZE * 2) * 4) )//maximum 1Mb
        {
            k = XFER_SIZE * 2;
            d = sramLength  - (k * 2);

            //1st block
            ints_off();
            neo_copyfrom_sram(buffer,oldBankOffs,k);
            ints_off();
            neo_copyto_sram(buffer,sramBankOffs,k);

            //2nd block
            ints_off();
            neo_copyfrom_sram(buffer,oldBankOffs + k,k);
            ints_off();
            neo_copyto_sram(buffer,sramBankOffs + k,k);

            //3rd block
            ints_off();
            neo_copyfrom_sram(buffer,oldBankOffs + (k * 2),k);
            ints_off();
            neo_copyto_sram(buffer,sramBankOffs + (k * 2),k);

            //4th block
            ints_off();
            neo_copyfrom_sram(buffer,oldBankOffs + (k * 3),d);
            ints_off();
            neo_copyto_sram(buffer,sramBankOffs + (k * 3),d);
        }
    }

    ints_on();
    setStatusMessage("Copying SRAM to next bank...OK");

    clearStatusMessage();
}

void do_sramMgr(void)
{
    int i,y,x,elems,running,sync,selection,nameLen;
    unsigned short int buttons,changed;

//    if(gCurMode != MODE_SD)
//        return;

    clear_screen();
    //cache_load();

    //title
    printToScreen("\x80\x82\x82\x82\x82\x82\x82\x82\x82",1,1,0x2000);
    printToScreen("  Save RAM Manager  ",10,1,0x4000);
    printToScreen("\x82\x82\x82\x82\x82\x82\x82\x82\x85",30,1,0x2000);

    //sep line
    printToScreen(gFEmptyLine,1,2,0x2000);

    //listing rect
    y = 3;
    for(i=0; i<PAGE_ENTRIES; i++,y++)
    {
        //put line
        printToScreen(gFEmptyLine,1,y,0x2000);
    }

    //sep line
    printToScreen(gFEmptyLine,1,18,0x2000);

    printToScreen(gFBottomLine,1,19,0x2000);

    // info area
    printToScreen(gLine,1,24,0x2000);

    // help area
    printToScreen("A",1,25,0x4000);
    printToScreen("=Cancel",2,25,0x0000);

    printToScreen("C",18,25,0x4000);
    printToScreen("=Call current option",19,25,0x0000);

    //put options
    elems = 0;
    //loadConfig();
    //gManageSaves = config_getI("manageSaves");

    //sram mgr
    if (gCurMode == MODE_SD)
    {
        // SD oriented services (at least until we can write the flash)
        gOptions[elems].name = "Save RAM Manager Service: ";
        if(gManageSaves)
            gOptions[elems].value = "Enabled";
        else
            gOptions[elems].value = "Disabled";
        gOptions[elems].callback = &sram_mgr_toggleService;
        gOptions[elems].patch = gOptions[elems].userData = NULL;
        elems++;

        gOptions[elems].name = "Save GAME";
        gOptions[elems].value = " ";
        gOptions[elems].callback = &sram_mgr_saveGame;
        gOptions[elems].patch = gOptions[elems].userData = NULL;
        elems++;

        gOptions[elems].name = "Restore GAME";
        gOptions[elems].value = " ";
        gOptions[elems].callback = &sram_mgr_restoreGame;
        gOptions[elems].patch = gOptions[elems].userData = NULL;
        elems++;

        gOptions[elems].name = "Save ALL";
        gOptions[elems].value = " ";
        gOptions[elems].callback = &sram_mgr_saveAll;
        gOptions[elems].patch = gOptions[elems].userData = NULL;
        elems++;

        gOptions[elems].name = "Restore ALL";
        gOptions[elems].value = " ";
        gOptions[elems].callback = &sram_mgr_restoreAll;
        gOptions[elems].patch = gOptions[elems].userData = NULL;
        elems++;
    }

    // services for all modes
    gOptions[elems].name = "Clear GAME";
    gOptions[elems].value = " ";
    gOptions[elems].callback = &sram_mgr_clearGame;
    gOptions[elems].patch = gOptions[elems].userData = NULL;
    elems++;

    gOptions[elems].name = "Clear ALL";
    gOptions[elems].value = " ";
    gOptions[elems].callback = &sram_mgr_clearAll;
    gOptions[elems].patch = gOptions[elems].userData = NULL;
    elems++;

    gOptions[elems].name = "Copy GAME to NEXT bank";
    gOptions[elems].value = " ";
    gOptions[elems].callback = &sram_mgr_copyGameToNextBank;
    gOptions[elems].patch = gOptions[elems].userData = NULL;
    elems++;

    running = 1;
    sync = 1;
    selection = 0;

    while(running)
    {
        if (sync)
        {
            sync = 0;
            y = 3;

            for (i=0; i<elems; i++, y++)
            {
                // remove garbage from line
                printToScreen(gFEmptyLine,1,y,0x2000);

                // keep track of length
                nameLen = utility_strlen(gOptions[i].name);

                // print centered text
                x = (40 >> 1) - ( (nameLen + utility_strlen(gOptions[i].value) + 2) >> 1);

                if(selection == i) //highlight selection
                {
                    //put name
                    printToScreen(gOptions[i].name,x,y,0x2000);
                    x += nameLen;

                    //delim
                    //printToScreen(":",x,y,0x2000);
                    x += 1;//x += 2;

                    //value
                    printToScreen(gOptions[i].value,x,y,0x2000);
                }
                else
                {
                    //put name
                    printToScreen(gOptions[i].name,x,y,0x0000);
                    x += nameLen;

                    //delim
                    //printToScreen(":",x,y,0x0000);
                    x += 1;//x += 2;

                    //value
                    printToScreen(gOptions[i].value,x,y,0x0000);
                }
            }
        }//sync

        //input
        delay(2);
        buttons = get_pad(0);

        if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
        {
            buttons = get_pad(1);
            if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
            {
                // no controllers, loop until one plugged in
                delay(20);
                continue;
            }
        }

        //read buttons
        if ((buttons & SEGA_CTRL_BUTTONS) != gButtons)
        {
            changed = (buttons & SEGA_CTRL_BUTTONS) ^ gButtons;
            gButtons = buttons & SEGA_CTRL_BUTTONS;

            if( ((changed & SEGA_CTRL_UP) && (buttons & SEGA_CTRL_UP)) )
            {
                // UP/LEFT pressed, go one entry back
                selection--;

                if (selection < 0)
                    selection = elems - 1;

                sync = 1;
                continue;
            }

            if ( ((changed & SEGA_CTRL_DOWN) && (buttons & SEGA_CTRL_DOWN)) )
            {
                // DOWN/RIGHT pressed, go one entry forward
                selection++;

                if (selection >= elems)
                    selection = 0;

                sync = 1;
                continue;
            }

            if ((changed & SEGA_CTRL_C) && !(buttons & SEGA_CTRL_C))
            {
                // C released -> do option callback
                if (gOptions[selection].callback)
                {
                    clearStatusMessage();
                    (gOptions[selection].callback)(selection);
                    do_sramMgr();

                    return;
                }
            }

            if ((changed & SEGA_CTRL_A) && !(buttons & SEGA_CTRL_A))
                return;
        }//changed??
    }

    clear_screen();
}

void runSRAMMgr(int index)
{
//    if(gCurMode != MODE_SD)
//        return;

    ints_on();
    setStatusMessage("Working...");
    clearStatusMessage();
    cache_sync(0);
    clearStatusMessage();
    do_sramMgr();
    clearStatusMessage();
}

void runCheatEditor(int index)
{
    char* line = (char*)&buffer[XFER_SIZE];
    char* buf = (char*)&buffer[XFER_SIZE + 256 ];
    char* pb = (char*)&buffer[0];
    WCHAR* cheatPath = &wstr_buf[gWStrOffs];
    UINT fbr,read;
    int r;
    int added = 0;
    int running = 1;
    int idx = 0;
    unsigned short int buttons = 0;

    if(gCurMode != MODE_SD)
        return;

    gWStrOffs += 512;

    clear_screen();

    ints_on();
    printToScreen("Working...",(40 >> 1) - (utility_strlen("Working...") >>1),12,0x2000);

    cache_sync(0);

    utility_memset(buf,'\0',32);
    utility_memset(line,'\0',256);

    clear_screen();

    //printToScreen("Prepare to enter CHEAT NAME...",(40 >> 1) - (utility_strlen("Prepare to enter CHEAT NAME...") >>1),12,0x0000);
    //delay(120);

    utility_memset(buf,'\0',32);
    r = inputBox(buf,"Enter cheat NAME","Some name",40 >> 1,5,0x4000,0x2000,0x0000,0x2000,32);

    if(!r)
    {
        clear_screen();
        gWStrOffs -= 512;
        return;
    }

    utility_strcpy(line,"AddCheat( $Name(");
    utility_strcat(line,buf);
    utility_strcat(line,") , $Code(");

    clear_screen();
    //printToScreen("Prepare to enter CHEAT CODE...",(40 >> 1) - (utility_strlen("Prepare to enter CHEAT CODE...") >>1),12,0x0000);
    //delay(120);

    while(running)
    {
        r = inputBox(buf,"Enter cheat CODE","NNNN-NNNN",40 >> 1,5,0x4000,0x2000,0x0000,0x2000,12);

        if(!r)
            break;

        {
            utility_strcat(line,buf);
            utility_strcat(line,",");
        }

        clear_screen();
        printToScreen("START = add more",((40 >> 1) - (utility_strlen("START = add more") >> 1 ))  ,11,0x0000);
        printToScreen("A = exit",((40 >> 1) - (utility_strlen("A = exit") >> 1 ))  ,13,0x0000);

        while(1)
        {
            delay(2);
            buttons = get_pad(0);

            if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
            {
                buttons = get_pad(1);
                if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
                {
                    running = 0;
                    break;
                }
            }

            if ((buttons & SEGA_CTRL_BUTTONS) != gButtons)
            {
                unsigned short int changed = (buttons & SEGA_CTRL_BUTTONS) ^ gButtons;
                gButtons = buttons & SEGA_CTRL_BUTTONS;

                if ((changed & SEGA_CTRL_A) && (buttons & SEGA_CTRL_A))
                {
                    running = 0;
                    break;
                }

                if ((changed & SEGA_CTRL_START) && (buttons & SEGA_CTRL_START))
                {
                    if(added >= 11)
                    {
                        clear_screen();
                        printToScreen("Linked cheats limit reached!",(40 >> 1) - (utility_strlen("Linked cheats limit reached!") >> 1 ),11,0x4000);

                        delay(60);

                        running = 0;
                        break;
                    }
                    else
                    {

                        ++added;
                        break;
                    }
                }
            }
        }
    }

    idx = utility_strlen(line) - 1;

    if(line[idx] == ',')
    {
        utility_memcpy(line + idx,"))\r\n",4);
        line[idx + 4] = '\0';
    }
    else
        utility_strcat(line,") )\r\n");

    utility_c2wstrcpy(cheatPath,"/");
    utility_c2wstrcat(cheatPath, CHEATS_DIR); createDirectory(cheatPath);
    utility_c2wstrcat(cheatPath, "/");
    utility_wstrcat(cheatPath, gSelections[gCurEntry].name);

    WCHAR* fnew = get_file_ext(cheatPath);

    if(*fnew == (WCHAR)'.')
        *fnew = 0; // cut off the extension
    utility_c2wstrcat(cheatPath, ".cht");

    f_close(&gSDFile);

    read = 0;

    ints_on();
    if(f_open(&gSDFile, cheatPath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        ints_on();
        read = gSDFile.fsize;

        if(read > XFER_SIZE)
            read = XFER_SIZE;

        ints_on();
        f_read(&gSDFile, pb , read,&fbr);

        ints_on();
    }

    f_close(&gSDFile);

    clear_screen();
    printToScreen("Working...",(40 >> 1) - (utility_strlen("Working...") >>1),12,0x0000);
    //delay(30);

    deleteFile(cheatPath);

    //delay(30);

    if(f_open(&gSDFile, cheatPath, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
    {
        ints_on();
        f_write(&gSDFile, line ,utility_strlen(line),&fbr);

        ints_on();
        f_write(&gSDFile, pb ,read,&fbr);
    }

    f_close(&gSDFile);

    ints_on();
    // - not needed anymore - get_sd_cheat(gSelections[gCurEntry].name);//cheatPath);
    gButtons = SEGA_CTRL_NONE;
    delay(10);
    clear_screen();
    gWStrOffs -= 512;
}

void hw_tst_myth_psram_test(int selection);
void hw_tst_psram_test(int selection);
void hw_tst_sram_write(int selection);
void hw_tst_sram_read(int selection);

void do_options(void)
{
    int i;
    int ix;
    int maxOptions = 0;
    int currOption = 0;
    int update = 1;
    char ipsFPath[64];
    char optCheatFirst = 1;

    __options_EntryPoint:
    clearStatusMessage();
    setStatusMessage("Checking for cheats...");
    get_sd_cheat(gSelections[gCurEntry].name);
    clearStatusMessage();
    setStatusMessage("Checking for ips...");
    get_sd_ips(gCurEntry);
    clearStatusMessage();

    maxOptions = currOption = 0;
    update = 1;
    gSRAMBank = 0;
    gSRAMSize = 0;
    gSRAMType = 0;

    clear_screen();

    if (gSelections[gCurEntry].type == 64)
        get_sd_info(gCurEntry);

    cache_load();
    clear_screen();
    gSelectionSize = 0;
    gImportIPS = 0;

    gCursorX = 1;
    gCursorY = 1;
    put_str("\x80\x82\x82\x82\x82", 0x2000);
    gCursorX = 6;
    put_str(" Game Options Configuration ", 0x4000);
    gCursorX = 34;
    put_str("\x82\x82\x82\x82\x85", 0x2000);

    gCursorX = 1;
    gCursorY = 2;
    put_str(gFEmptyLine, 0x2000);

    // list area (if applicable)
    gCursorY = 3;
    for (ix=0; ix<PAGE_ENTRIES; ix++, gCursorY++)
        put_str(gFEmptyLine, 0x2000);

    gCursorX = 1;
    gCursorY = 18;
    put_str(gFEmptyLine, 0x2000);

    gCursorX = 1;
    gCursorY = 19;
    put_str(gFBottomLine, 0x2000);

    // info area

    gCursorX = 1;
    gCursorY = 24;
    put_str(gLine, 0x2000);

    // help area
    gCursorY = 25;
    gCursorX = 1;
    put_str("A", 0x4000);
    gCursorX = 2;
    put_str("=Cancel", 0);
    gCursorX = 11;
    put_str("B", 0x4000);
    gCursorX = 12;
    put_str("=Run", 0);
    gCursorX = 18;
    put_str("C", 0x4000);
    gCursorX = 19;
    put_str("=Call current option", 0);

    // insert options into gOptions array
    if (gSelections[gCurEntry].run < 7)
    {
        // options for MD/32X
        gOptions[maxOptions].exclusiveFCall = 0;
        gOptions[maxOptions].name = "Reset Mode";
        if(gResetMode == 0x0000)
            gOptions[maxOptions].value = "Reset to Menu";
        else
            gOptions[maxOptions].value = "Reset to Game";
        gOptions[maxOptions].callback = &toggleResetMode;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;

        gOptions[maxOptions].exclusiveFCall = 0;
        gOptions[maxOptions].name = "Bank Aliasing";
        gOptions[maxOptions].value = gNoAlias ? "OFF" : "ON";
        gOptions[maxOptions].callback = &toggleBankAlias;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;

        if(!gSRAMType)
            gSRAMType = (gSelections[gCurEntry].run == 5);
        gOptions[maxOptions].exclusiveFCall = 0;
        gOptions[maxOptions].name = "Save RAM Type";
        gOptions[maxOptions].value = gSRAMType ? "EEPROM" : "SRAM";
        gOptions[maxOptions].callback = &toggleSRAMType;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;
    }
    else if (gSelections[gCurEntry].run == 0x13)
    {
        // options for SMS
        gOptions[maxOptions].exclusiveFCall = 0;
        gOptions[maxOptions].name = "YM2413 FM";

        if(gYM2413)
            gOptions[maxOptions].value = "On";
        else
            gOptions[maxOptions].value = "Off";

        gOptions[maxOptions].callback = &toggleYM2413;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;
    }
    else if (((gSelections[gCurEntry].run & 63) == 9) || (gSelections[gCurEntry].run == 10))
    {
        // options for BRAM or CDBIOS+BRAM
        gOptions[maxOptions].exclusiveFCall = 0;
        gOptions[maxOptions].name = "Reset Mode";

        if(gResetMode == 0x0000)
            gOptions[maxOptions].value = "Reset to Menu";
        else
            gOptions[maxOptions].value = "Reset to Game";

        gOptions[maxOptions].callback = &toggleResetMode;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;

        gSRAMSize = gSelections[gCurEntry].bsize;
        if(!gSRAMBank)
            gSRAMBank = gSelections[gCurEntry].bbank;

        gOptions[maxOptions].exclusiveFCall = 0;
        UTIL_IntegerToString(gSRAMBankStr, gSRAMBank, 10);
        gOptions[maxOptions].name = "Save RAM Bank";
        gOptions[maxOptions].value = gSRAMBankStr;
        gOptions[maxOptions].callback = &incSaveRAMBank;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;

        gOptions[maxOptions].exclusiveFCall = 1; //Exclusive function! jump back to source caller
        gOptions[maxOptions].name = "Save RAM Manager";
        gOptions[maxOptions].value = NULL;
        gOptions[maxOptions].callback = &runSRAMMgr;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;
    }
    // MD, 32X, or SMS
    if ((gSelections[gCurEntry].run < 7) || (gSelections[gCurEntry].run == 0x13))
    {
        gOptions[maxOptions].exclusiveFCall = 0;

        if(!gSRAMSize)
            gSRAMSize = gSelections[gCurEntry].bsize;

        UTIL_IntegerToString(gSRAMSizeStr, gSRAMSize*64, 10);
        utility_strcat(gSRAMSizeStr, "Kb");
        gOptions[maxOptions].name = "Save RAM Size";
        gOptions[maxOptions].value = gSRAMSizeStr;
        gOptions[maxOptions].callback = &incSaveRAMSize;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;

        if(!gSRAMBank)
            gSRAMBank = gSelections[gCurEntry].bbank;

        gOptions[maxOptions].exclusiveFCall = 0;
        UTIL_IntegerToString(gSRAMBankStr, gSRAMBank, 10);
        gOptions[maxOptions].name = "Save RAM Bank";
        gOptions[maxOptions].value = gSRAMBankStr;
        gOptions[maxOptions].callback = &incSaveRAMBank;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;

        gOptions[maxOptions].exclusiveFCall = 1; //Exclusive function! jump back to source caller
        gOptions[maxOptions].name = "Save RAM Manager";
        gOptions[maxOptions].value = NULL;
        gOptions[maxOptions].callback = &runSRAMMgr;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;
    }

    //if(gCurMode == MODE_SD)
    {
        // insert options for game rom into gOptions array
        gOptions[maxOptions].exclusiveFCall = 1; //Exclusive function! jump back to source caller
        gOptions[maxOptions].name = "Add cheats manually";
        gOptions[maxOptions].value = NULL;
        gOptions[maxOptions].callback = &runCheatEditor;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;
    }

    //HW SELF TEST ROUTINES
    {
        gOptions[maxOptions].name = "TEST ONBOARD PSRAM";
        gOptions[maxOptions].value = NULL;
        gOptions[maxOptions].callback = &hw_tst_myth_psram_test;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        gOptions[maxOptions].exclusiveFCall = 1;
        maxOptions++;

        gOptions[maxOptions].name = "TEST NEO2    PSRAM";
        gOptions[maxOptions].value = NULL;
        gOptions[maxOptions].callback = &hw_tst_psram_test;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        gOptions[maxOptions].exclusiveFCall = 1;
        maxOptions++;

        gOptions[maxOptions].name = "TEST SRAM(write)";
        gOptions[maxOptions].value = NULL;
        gOptions[maxOptions].callback = &hw_tst_sram_write;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        gOptions[maxOptions].exclusiveFCall = 1;
        maxOptions++;

        gOptions[maxOptions].name = "TEST SRAM(read)";
        gOptions[maxOptions].value = NULL;
        gOptions[maxOptions].callback = &hw_tst_sram_read;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        gOptions[maxOptions].exclusiveFCall = 1;
        maxOptions++;
    }

    //List one ips patch
    if( (ipsPath[0] != 0) )
    {
        utility_w2cstrcpy((char*)buffer, ipsPath);
        strncpy(ipsFPath, (const char *)buffer, 36);
        ipsFPath[35] = '\0';
        gOptions[maxOptions].name = ipsFPath;

        if(!gImportIPS)
            gOptions[maxOptions].value = "Off";
        else
            gOptions[maxOptions].value = "On";

        gOptions[maxOptions].userData = NULL;
        gOptions[maxOptions].callback = &toggleIPS;
        gOptions[maxOptions].patch = &importIPS;

        maxOptions++;
    }

    //List cheats
    for(i = 0; (i < registeredCheatEntries) && (OPTION_ENTRIES > maxOptions); i++)
    {
        if(cheatEntries[i].name[0] != '\0')
        {
            gOptions[maxOptions].name = cheatEntries[i].name;

            if(!cheatEntries[i].active)
                gOptions[maxOptions].value = "Off";
            else
                gOptions[maxOptions].value = "On";

            gOptions[maxOptions].callback = &toggleCheat;
            gOptions[maxOptions].patch = optCheatFirst?&importCheats:0;
            optCheatFirst = 0; // we need only one call of importCheats
            gOptions[maxOptions].userData = (void*)(&cheatEntries[i]);

            maxOptions++;
        }
    }

    int start = 0;
    int end;

    if(maxOptions > PAGE_ENTRIES)
        end = PAGE_ENTRIES;
    else
        end = maxOptions;

    while(1)
    {
        unsigned short int buttons;

        if (update && maxOptions)
        {
            update = 0;
            gCursorY = 3;
            for (ix=0; ix<PAGE_ENTRIES; ix++, gCursorY++)
            {
                // erase line
                gCursorX = 1;
                put_str(gFEmptyLine, 0x2000);
                if (start + ix >= maxOptions)
                    continue;   // past end, skip line
                // put centered name
                if(gOptions[start+ix].value != NULL)
                    gCursorX = 20 - (utility_strlen(gOptions[start+ix].name) + utility_strlen(gOptions[start+ix].value) + 2)/2;
                else
                    gCursorX = 20 - (utility_strlen(gOptions[start+ix].name) /2);

                put_str(gOptions[start+ix].name, ((start+ix) == currOption) ? 0x2000 : 0);
                gCursorX += utility_strlen(gOptions[start+ix].name);

                if(gOptions[start+ix].value != NULL)
                {
                    put_str(":", ((start+ix) == currOption) ? 0x2000 : 0);
                    gCursorX += 2;
                    put_str(gOptions[start+ix].value, ((start+ix) == currOption) ? 0x2000 : 0);
                }
            }
        }

        delay(2);
        buttons = get_pad(0);
        if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
        {
            buttons = get_pad(1);
            if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
            {
                // no controllers, loop until one plugged in
                delay(20);
                continue;
            }
        }

        {
            if ((buttons & SEGA_CTRL_UP))
            {
                // UP pressed, go one entry back
                currOption--;
                if (currOption < 0)
                {
                    currOption = maxOptions - 1; // wrap around to bottom
                    start = maxOptions - (maxOptions % PAGE_ENTRIES);
                }
                if (currOption < start)
                {
                    start -= PAGE_ENTRIES;
                    if (start < 0)
                        start = 0;
                }
                update = 1;
                delay( (getClockType()) ? 4 : 6 );
                continue;
            }

            if ((buttons & SEGA_CTRL_LEFT))
            {
                // LEFT pressed, go one page back
                currOption -= PAGE_ENTRIES;
                if (currOption < 0)
                {
                    currOption = maxOptions - 1;
                    start = maxOptions - (maxOptions % PAGE_ENTRIES);
                }
                if (currOption < start)
                {
                    start -= PAGE_ENTRIES; // previous "page" of entries
                    if (start < 0)
                        start = 0;
                }
                update = 1;
                delay( (getClockType()) ? 10 : 14 );
                continue;
            }

            if ((buttons & SEGA_CTRL_DOWN))
            {
                // DOWN pressed, go one entry forward
                currOption++;
                if (currOption >= maxOptions)
                    currOption = start = 0; // wrap around to top
                if ((currOption - start) >= PAGE_ENTRIES)
                    start += PAGE_ENTRIES;
                update = 1;
                delay( (getClockType()) ? 4 : 6 );
                continue;
            }

            if ((buttons & SEGA_CTRL_RIGHT))
            {
                // RIGHT pressed, go one page forward
                currOption += PAGE_ENTRIES;
                if (currOption >= maxOptions)
                    currOption = start = 0;    // wrap around to top
                if ((currOption - start) >= PAGE_ENTRIES)
                    start += PAGE_ENTRIES; // next "page" of entries
                update = 1;
                delay( (getClockType()) ? 10 : 14 );
                continue;
            }

        }

        if ((buttons & SEGA_CTRL_BUTTONS) != gButtons)
        {
            unsigned short int changed = (buttons & SEGA_CTRL_BUTTONS) ^ gButtons;
            gButtons = buttons & SEGA_CTRL_BUTTONS;

            if ((changed & SEGA_CTRL_A) && !(buttons & SEGA_CTRL_A))
            {
                if(gCurMode == MODE_SD)
                {
                    cache_sync(0);
                    cache_invalidate_pointers();
                }

                gSelectionSize = 0;
                ipsPath[0] = 0;
                cheat_invalidate();
                delay( (getClockType()) ? 10 : 14 );
                break;  // A released -> cancel, break out of while loop
            }

            if ((changed & SEGA_CTRL_B) && !(buttons & SEGA_CTRL_B))
            {
                // B released
                char temp[64];

                utility_w2cstrcpy((char*)buffer, gSelections[gCurEntry].name);
                //strncpy(temp, (const char *)buffer, 30);
                //temp[30] = '\0';
                shortenName(temp, (char *)buffer, 30);

                // check for write-enable psram flag
                if (gSelections[gCurEntry].run & 64)
                {
                    gSelections[gCurEntry].run &= 63;
                    gWriteMode = 0x0002; // write-enable psram when run
                }

                if (gCurMode == MODE_FLASH)
                {
                    int fstart, fsize, bbank, bsize, runmode;
                    fstart = gSelections[gCurEntry].offset;
                    fsize = gSelections[gCurEntry].length;
                    bbank = gSRAMBank;
                    bsize = gSRAMSize * 8192;
                    runmode = gSelections[gCurEntry].run;
                    if ((runmode & 0x1F) < 7)
                        runmode = gSRAMType ? 5 : (fsize > 0x400200) ? 4 : !bsize ? 6 : (gSelections[gCurEntry].type == 1) ? 3 : (fsize > 0x200200) ? 2 : 1;
                    ints_off();     /* disable interrupts */
                    // Run selected rom
                    if ((gSelections[gCurEntry].type == 2) || (runmode == 7))
                    {
                        // if no cheats or patches enabled, run from flash
                        if (!patchesNeeded())
                            neo_run_game(fstart, fsize, bbank, bsize, runmode); // never returns

                        // copy flash to gba psram
                        copyGame(&neo_copyto_psram, &neo_copy_game, 0, fstart, fsize, "Loading ", temp);

                        gSelectionSize = fsize;
                        gPSRAM = 0; // gba psram
                        // do patch callbacks
                        for (ix=0; ix<maxOptions; ix++)
                        {
                            if (gOptions[ix].patch)
                                (gOptions[ix].patch)(ix);
                        }

                        neo_run_psram(0, fsize, bbank, bsize, runmode); // never returns
                    }
                    else
                    {
                        int pstart = (runmode == 0x27) ? 0x600000 : 0;

                        if (runmode == 9)
                        {
                            // generate bram pattern to myth psram
                            copyGame(&neo_copyto_myth_psram, &gen_bram, 0, 0, fsize, "Generating ", temp);
                            neo_run_myth_psram(fsize, bbank, bsize, runmode); // never returns
                        }
                        // copy flash to myth psram
                        copyGame(&neo_copyto_myth_psram, &neo_copy_game, pstart, fstart, fsize, "Loading ", temp);

                        // check for raw S&K
                        if (!utility_memcmp((void*)0x200180, "GM MK-1563 -00", 14) && (fsize == 0x200000))
                            fsize = 0x300000;

                        gSelectionSize = fsize;
                        gPSRAM = 1; // myth psram
                        // do patch callbacks
                        for (ix=0; ix<maxOptions; ix++)
                          if (gOptions[ix].patch)
                              (gOptions[ix].patch)(ix);

                        neo_run_myth_psram(fsize, bbank, bsize, runmode); // never returns
                    }
                }
                else if (gCurMode == MODE_SD)
                {
                    int eos = utility_wstrlen(path);
                    int fsize, bbank, bsize, runmode;
                    UINT ts;

                    fsize = gSelections[gCurEntry].length;
                    bbank = gSRAMBank;
                    bsize = gSRAMSize * 8192;
                    runmode = gSelections[gCurEntry].run;
                    // Run selected rom
                    if (path[utility_wstrlen(path)-1] != (WCHAR)'/')
                        utility_c2wstrcat(path, "/");
                    utility_wstrcat(path, gSelections[gCurEntry].name);

                    f_close_zip(&gSDFile);
                    if (f_open_zip(&gSDFile, path, FA_OPEN_EXISTING | FA_READ))
                    {
                        // couldn't open file
                        path[eos] = 0;
                        continue;
                    }
                    path[eos] = 0;

                    fsize = gSDFile.fsize;
                    if ((runmode & 0x1F) < 7)
                        runmode = gSRAMType ? 5 : (fsize > 0x400200) ? 4 : !bsize ? 6 : (gSelections[gCurEntry].type == 1) ? 3 : (fsize > 0x200200) ? 2 : 1;

                    if (gFileType)
                        f_read_zip(&gSDFile, buffer, 0x200, &ts);

                    if ((gSelections[gCurEntry].type == 2) || (runmode == 7))
                    {
                        // copy file to flash cart psram
                        copyGame(&neo_copyto_psram, &neo_copy_sd, 0, 0, fsize, "Loading ", temp);

                        gSelectionSize = fsize;
                        gPSRAM = 0; // gba psram
                        // do patch callbacks
                        for (ix=0; ix<maxOptions; ix++)
                        {
                            if (gOptions[ix].patch)
                                (gOptions[ix].patch)(ix);
                        }

                        //cache_load();
                        gCacheOutOfSync = 1;
                        if(gManageSaves)
                        {
                            f_close_zip(&gSDFile);
                            clear_screen();
                            gSRAMgrServiceStatus = SMGR_STATUS_BACKUP_SRAM;
                            cache_sync(0);

                            char* p = (char*)buffer;
                            utility_w2cstrcpy(p,gSelections[gCurEntry].name);
                            config_push("romName",p);

                            UTIL_IntegerToString(p, (gSelections[gCurEntry].type == 2) ? SMGR_MODE_SMS : (gSelections[gCurEntry].run < 8) ? SMGR_MODE_MD32X : SMGR_MODE_BRAM, 10);
                            config_push("romType",p);

                            updateConfig();

                            sram_mgr_restoreGame(-1);
                        }
                        else
                            cache_sync(0);

                        neo2_disable_sd();
                        ints_off();     /* disable interrupts */
                        neo_run_psram(0, fsize, bbank, bsize, runmode); // never returns
                    }
                    else
                    {
                        int pstart = (runmode == 0x27) ? 0x600000 : 0;

                        if (runmode == 9)
                        {
                            // generate bram pattern to myth psram
                            copyGame(&neo_copyto_myth_psram, &gen_bram, 0, 0, fsize, "Generating ", temp);
                        }
                        else
                        {
                            // copy file to myth psram
                            if (gFileType)
                                copyGame(&neo_copyto_myth_psram, &neo_copy_sd, pstart, 0, fsize, "Loading ", temp);
                            else
                                copyGame(&neo_sd_to_myth_psram, 0, pstart, 0, fsize, "Loading ", temp);
                        }

                        // check for raw S&K
                        if (!utility_memcmp((void*)0x200180, "GM MK-1563 -00", 14) && (fsize == 0x200000))
                            fsize = 0x300000;

                        gSelectionSize = fsize;
                        gPSRAM = 1; // myth psram
                        // do patch callbacks
                        for (ix=0; ix<maxOptions; ix++)
                        {
                            if (gOptions[ix].patch)
                                (gOptions[ix].patch)(ix);
                        }

                        //cache_load();
                        gCacheOutOfSync = 1;
                        if(gManageSaves)
                        {
                            f_close_zip(&gSDFile);
                            clear_screen();
                            gSRAMgrServiceStatus = SMGR_STATUS_BACKUP_SRAM;
                            cache_sync(0);

                            char* p = (char*)buffer;
                            utility_w2cstrcpy(p,gSelections[gCurEntry].name);
                            config_push("romName",p);

                            UTIL_IntegerToString(p, (gSelections[gCurEntry].type == 2) ? SMGR_MODE_SMS : (gSelections[gCurEntry].run < 8) ? SMGR_MODE_MD32X : SMGR_MODE_BRAM, 10);
                            config_push("romType",p);

                            updateConfig();

                            sram_mgr_restoreGame(-1);
                        }

                        ints_on();
                        neo2_disable_sd();
                        ints_off();     /* disable interrupts */
                        neo_run_myth_psram(fsize, bbank, bsize, runmode); // never returns
                    }
                }
            }

            if ((changed & SEGA_CTRL_C) && !(buttons & SEGA_CTRL_C))
            {
                //if(gOptions[currOption].exclusiveFCall)
                    //cache_sync(0);

                // C released -> do option callback
                if (gOptions[currOption].callback)
                    (gOptions[currOption].callback)(currOption);

                //exclusive function
                if(gOptions[currOption].exclusiveFCall)
                {
                    clear_screen();
                    delay( (getClockType()) ? 10 : 14 );
                    goto __options_EntryPoint;
                }

                update = 1;
                continue;
            }

        }
    }

    gUpdate = -1; // major update to screen
}

int do_SDMgr(void)
{
    int i, y;
    unsigned short int buttons,changed;
    char temp[64];

    clear_screen();

    //title
    printToScreen("\x80\x82\x82\x82\x82\x82\x82\x82\x82",1,1,0x2000);
    printToScreen("  SD Card Manager  ",10,1,0x4000);
    printToScreen("\x82\x82\x82\x82\x82\x82\x82\x82\x82\x85",29,1,0x2000);

    //sep line
    printToScreen(gFEmptyLine,1,2,0x2000);

    //listing rect
    y = 3;
    for(i=0; i<PAGE_ENTRIES; i++,y++)
    {
        //put line
        printToScreen(gFEmptyLine,1,y,0x2000);
    }

    //sep line
    printToScreen(gFEmptyLine,1,18,0x2000);

    printToScreen(gFBottomLine,1,19,0x2000);

    // info area
    printToScreen(gLine,1,24,0x2000);

    // help area
    printToScreen("A",1,25,0x4000);
    printToScreen("=Cancel",2,25,0x0000);

    printToScreen("C",23,25,0x4000);
    printToScreen("=Format SD Card",24,25,0x0000);

    utility_strcpy(temp, "Card has ");
    UTIL_IntegerToString(&temp[9], num_sectors, 10);
    utility_strcat(temp, " blocks (");
    UTIL_IntegerToString(&temp[utility_strlen(temp)], (num_sectors / 2048), 10);
    utility_strcat(temp, " MB)");
    printToScreen(temp, (40 - utility_strlen(temp))>>1, 3, 0x2000);

//    sprintf(temp, "CSD: %02X %02X %02X %02X %02X %02X", sd_csd[0], sd_csd[1], sd_csd[2], sd_csd[3], sd_csd[4], sd_csd[5]);
//    printToScreen(temp, (40 - utility_strlen(temp))>>1, 5, 0x2000);

    while(1)
    {
        //input
        delay(2);
        buttons = get_pad(0);
        if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
        {
            buttons = get_pad(1);
            if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
            {
                // no controllers, loop until one plugged in
                delay(20);
                continue;
            }
        }

        //read buttons
        if ((buttons & SEGA_CTRL_BUTTONS) != gButtons)
        {
            changed = (buttons & SEGA_CTRL_BUTTONS) ^ gButtons;
            gButtons = buttons & SEGA_CTRL_BUTTONS;

            if ((changed & SEGA_CTRL_C) && !(buttons & SEGA_CTRL_C))
            {
                // C released -> format SD card
                if (f_mkfs(1, 0, 0) == FR_OK)
                    return 0;
                return 1;
            }

            if ((changed & SEGA_CTRL_A) && !(buttons & SEGA_CTRL_A))
            {
                return 1;
            }
        }//changed??
    }

    // to suppress warning... shouldn't reach here
    return 1;
}

void run_rom(int reset_mode)
{
    char temp[64];

    utility_w2cstrcpy((char*)buffer, gSelections[gCurEntry].name);
    //strncpy(temp, (const char *)buffer, 30);
    //temp[30] = '\0';
    shortenName(temp, (char *)buffer, 30);

    gResetMode = reset_mode;

    printToScreen(gEmptyLine,1,20,0x0000);
    printToScreen(gEmptyLine,1,21,0x0000);
    printToScreen(gEmptyLine,1,22,0x0000);
    printToScreen(gEmptyLine,1,23,0x0000);

    if (gCurMode == MODE_FLASH)
    {
        int fstart, fsize, bbank, bsize;
        fstart = gSelections[gCurEntry].offset;
        fsize = gSelections[gCurEntry].length;
        bbank = gSelections[gCurEntry].bbank;
        bsize = gSelections[gCurEntry].bsize * 8192;

        if (gSelections[gCurEntry].type == 4)
        {
            int ix;
            char temp2[64];

            // Play VGM song
#ifndef RUN_IN_PSRAM
            if (fsize > 0x780000)
                return; // too big for current method of playing
#else
            if (fsize > 0x700000)
                return; // too big for current method of playing
#endif
            // copy file to myth psram
            ints_off();
            copyGame(&neo_copyto_myth_psram, &neo_copy_game, 0x000000, fstart, fsize, "Loading ", temp);
            ints_on();     /* enable interrupts */

            gCursorX = 1;
            gCursorY = 20;
            // erase line
            put_str(gEmptyLine, 0);
            gCursorX = 1;
            gCursorY = 21;
            // erase line
            put_str(gEmptyLine, 0);
            gCursorX = 1;
            gCursorY = 22;
            // erase line
            put_str(gEmptyLine, 0);
            gCursorX = 1;
            gCursorY = 23;
            // erase line
            put_str(gEmptyLine, 0);

            // print track name from header
            utility_memcpy(temp2, &rom_hdr[0x20], 0x30);
            for (ix=47; ix>0; ix--)
                if (temp2[ix] != 0x20)
                    break;
            if (ix > 37)
                ix = 37; // max string size to print
            temp2[ix+1] = 0; // null terminate name
            gCursorY = 22;
            gCursorX = 20 - utility_strlen(temp2)/2;    // center name
            put_str(temp2, 0);

            utility_strcpy(temp2, "Press C to Stop");
            gCursorY = 23;
            gCursorX = 20 - utility_strlen(temp2)/2;    // center name
            put_str(temp2, 0);

            delay(60);

            PlayVGM();
            gRomDly = 60;
            gResponseMsgStatus = 1;
            gUpdate = -1;               /* clear screen for major screen update */
            return;
        }

        // Run selected rom
        ints_off();     /* disable interrupts */
        if ((gSelections[gCurEntry].type == 2) || (gSelections[gCurEntry].run == 7))
        {
            neo_run_game(fstart, fsize, bbank, bsize, gSelections[gCurEntry].run); // never returns
        }
        else
        {
            int pstart = (gSelections[gCurEntry].run == 0x27) ? 0x600000 : 0;

            if (gSelections[gCurEntry].run == 9)
            {
                // generate bram pattern to myth psram
                copyGame(&neo_copyto_myth_psram, &gen_bram, 0, 0, fsize, "Generating ", temp);
                neo_run_myth_psram(fsize, bbank, bsize, gSelections[gCurEntry].run); // never returns
            }
            // copy flash to myth psram
            copyGame(&neo_copyto_myth_psram, &neo_copy_game, pstart, fstart, fsize, "Loading ", temp);

            // check for raw S&K
            if (!utility_memcmp((void*)0x200180, "GM MK-1563 -00", 14) && (fsize == 0x200000))
                fsize = 0x300000;

            neo_run_myth_psram(fsize, bbank, bsize, gSelections[gCurEntry].run); // never returns
        }
    }
    else if (gCurMode == MODE_USB)
    {
        // Run USB loaded image
        ints_off();     /* disable interrupts */
        if (gSelections[gCurEntry].type == 2)
        {
            neo_run_game(0, 0x100000, 0, 0, 0x13); // never returns
        }
        else
        {
            // copy flash to myth psram
            copyGame(&neo_copyto_myth_psram, &neo_copy_game, 0, 0, 0x400000, "Loading ", temp);
            neo_run_myth_psram(0x400000, 0, 0, gSelections[0].run); // never returns
        }
    }
    else if (gCurMode == MODE_SD)
    {
        int eos = utility_wstrlen(path);
        int fsize, bbank, bsize, runmode;
        UINT ts;

        // entry may be unknown
        if (gSelections[gCurEntry].type == 127)
            get_sd_info(gCurEntry);

        if (gSelections[gCurEntry].type == 128)
        {
            get_sd_directory(gCurEntry);
            return;
        }

        cache_invalidate_pointers();

        cache_loadPA(gSelections[gCurEntry].name,0);//don't forget to load the cache

        if(gSRAMSize || gSRAMType)//load cached info
        {
            fsize = gSelections[gCurEntry].length;
            bbank = gSRAMBank;
            bsize = gSRAMSize * 8192;
        }
        else
        {
            fsize = gSelections[gCurEntry].length;
            bbank = gSelections[gCurEntry].bbank;
            bsize = gSelections[gCurEntry].bsize * 8192;
        }

        runmode = gSelections[gCurEntry].run;

        if (!bsize && (runmode < 4))
            runmode = 6;

        // make sure file is open and ready to load
        if (path[utility_wstrlen(path)-1] != (WCHAR)'/')
            utility_c2wstrcat(path, "/");
        utility_wstrcat(path, gSelections[gCurEntry].name);
        f_close_zip(&gSDFile);
        if (f_open_zip(&gSDFile, path, FA_OPEN_EXISTING | FA_READ))
        {
            // couldn't open file
            setStatusMessage("Couldn't open game file");
            path[eos] = 0;
            return;
        }
        path[eos] = 0;

        fsize = gSDFile.fsize;

        if (gFileType || (((runmode == 0x12) || (runmode == 0x13)) && ((fsize & 0xFFF) == 0x200)))
            f_read_zip(&gSDFile, buffer, 0x200, &ts); // skip header on SMD file and (some) SMS files

        // Look for special file types
        if (gSelections[gCurEntry].type == 4)
        {
            int ix;
            char temp2[64];

            // Play VGM song
#ifndef RUN_IN_PSRAM
            if (fsize > 0x780000)
                return; // too big for current method of playing
#else
            if (fsize > 0x700000)
                return; // too big for current method of playing
#endif
            // copy file to myth psram
            copyGame(&neo_copyto_myth_psram, &neo_copy_sd, 0x000000, 0, fsize, "Loading ", temp);
            ints_on();     /* enable interrupts */

            gCursorX = 1;
            gCursorY = 20;
            // erase line
            put_str(gEmptyLine, 0);
            gCursorX = 1;
            gCursorY = 21;
            // erase line
            put_str(gEmptyLine, 0);
            gCursorX = 1;
            gCursorY = 22;
            // erase line
            put_str(gEmptyLine, 0);
            gCursorX = 1;
            gCursorY = 23;
            // erase line
            put_str(gEmptyLine, 0);

            // print track name from header
            utility_memcpy(temp2, &rom_hdr[0x20], 0x30);
            for (ix=47; ix>0; ix--)
                if (temp2[ix] != 0x20)
                    break;
            if (ix > 37)
                ix = 37; // max string size to print
            temp2[ix+1] = 0; // null terminate name
            gCursorY = 22;
            gCursorX = 20 - utility_strlen(temp2)/2;    // center name
            put_str(temp2, 0);

            utility_strcpy(temp2, "Press C to Stop");
            gCursorY = 23;
            gCursorX = 20 - utility_strlen(temp2)/2;    // center name
            put_str(temp2, 0);

            delay(60);

            PlayVGM();
            gRomDly = 60;
            gResponseMsgStatus = 1;
            gUpdate = -1;               /* clear screen for major screen update */
            return;
        }

        // Run selected rom
        if ((gSelections[gCurEntry].type == 2) || (runmode == 7))
        {
            // copy file to flash cart psram
            copyGame(&neo_copyto_psram, &neo_copy_sd, 0, 0, fsize, "Loading ", temp);
            cache_load();
            gCacheOutOfSync = 1;
            if(gManageSaves)
            {
                gSRAMgrServiceStatus = SMGR_STATUS_BACKUP_SRAM;
                cache_sync(0);

                char* p = (char*)buffer;
                utility_w2cstrcpy(p,gSelections[gCurEntry].name);
                config_push("romName",p);

                UTIL_IntegerToString(p, (gSelections[gCurEntry].type == 2) ? SMGR_MODE_SMS : (gSelections[gCurEntry].run < 8) ? SMGR_MODE_MD32X : SMGR_MODE_BRAM, 10);
                config_push("romType",p);

                updateConfig();

                sram_mgr_restoreGame(-1);
            }
            else
                cache_sync(0);

            clearStatusMessage();

            neo2_disable_sd();
            ints_off();     /* disable interrupts */
            neo_run_psram(0, fsize, bbank, bsize, runmode); // never returns
        }
        else
        {
            int pstart = (runmode == 0x27) ? 0x600000 : 0;

            if (runmode == 9)
            {
                // generate bram pattern to myth psram
                copyGame(&neo_copyto_myth_psram, &gen_bram, 0, 0, fsize, "Generating ", temp);
            }
            else
            {
                // copy file to myth psram
                if (gFileType)
                    copyGame(&neo_copyto_myth_psram, &neo_copy_sd, pstart, 0, fsize, "Loading ", temp);
                else
                    copyGame(&neo_sd_to_myth_psram, 0, pstart, 0, fsize, "Loading ", temp);
            }

            // check for raw S&K
            if (!utility_memcmp((void*)0x200180, "GM MK-1563 -00", 14) && (fsize == 0x200000))
                fsize = 0x300000;

            cache_load();
            gCacheOutOfSync = 1;
            if(gManageSaves)
            {
                gSRAMgrServiceStatus = SMGR_STATUS_BACKUP_SRAM;
                cache_sync(0);

                char* p = (char*)buffer;
                utility_w2cstrcpy(p,gSelections[gCurEntry].name);
                config_push("romName",p);

                UTIL_IntegerToString(p, (gSelections[gCurEntry].type == 2) ? SMGR_MODE_SMS : (gSelections[gCurEntry].run < 8) ? SMGR_MODE_MD32X : SMGR_MODE_BRAM, 10);
                config_push("romType",p);

                updateConfig();

                sram_mgr_restoreGame(-1);
            }
            else
                cache_sync(0);

            //patch bad headers ( based on genplus emu )
            {
                unsigned int a,b;
                unsigned char* psramBuf = (unsigned char*)&buffer[0];
                unsigned char* psramRaw = (unsigned char*)0x2001B4;

                setStatusMessage("Checking for bad header...");
                ints_off();

                if( (*(unsigned char*)0x2001B0 == 'R') && (*(unsigned char*)0x2001B1 == 'A') )
                {
                    {
                        a = psramRaw[0x00]<<24|psramRaw[0x01]<<16|psramRaw[0x02]<<8|psramRaw[0x03];
                        b = psramRaw[0x04]<<24|psramRaw[0x05]<<16|psramRaw[0x06]<<8|psramRaw[0x07];

                        if ((b > a) || ((b - a) >= 0x10000))
                        {
                            b = a + 0xffff;
                            a &= 0xfffffffe;
                            b |= 1;

                            psramBuf[0] = (a>>24)&0xff;psramBuf[1] = (a>>16)&0xff;
                            psramBuf[2] = (a>>8)&0xff;psramBuf[3] = (a&0xff);

                            psramBuf[4] = (b>>24)&0xff;psramBuf[5] = (b>>16)&0xff;
                            psramBuf[6] = (b>>8)&0xff;psramBuf[7] = (b&0xff);

                            ints_off();
                            neo_copyto_myth_psram(psramBuf,0x1B4,8);
                        }
                    }
                }

                //nba jam 32x te
                if( (utility_memcmp((void*)0x200180,"GM T-8104B",10) == 0) || (utility_memcmp((void*)0x200180,"GM T-81406",10) == 0) )
                {
                    *(unsigned short*)&psramBuf[0] = 0x714e;

                    ints_off();
                    neo_copyto_myth_psram(psramBuf,0xeec,2);
                }
            }

            clearStatusMessage();
            ints_on();
            neo2_disable_sd();
            ints_off();     /* disable interrupts */
            neo_run_myth_psram(fsize, bbank, bsize, runmode); // never returns
        }
    }
}

/* config */
void updateConfig()
{
    UINT fbr = 0;
    WCHAR* fss = &wstr_buf[gWStrOffs];
    gWStrOffs += 512;

    ints_on();

    setStatusMessage("Updating config...");

    if(config_saveToBuffer((char*)buffer))
    {
        clearStatusMessage();
        gWStrOffs -= 512;
        return;
    }

    utility_c2wstrcpy(fss,dxcore_dir); createDirectory(fss);
    utility_c2wstrcpy(fss,dxconf_cfg);

    f_close(&gSDFile);
    deleteFile(fss);

    if(f_open(&gSDFile, fss, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        clearStatusMessage();
        gWStrOffs -= 512;
        return;
    }

    setStatusMessage("Writing config...");

    ints_on();
    f_write(&gSDFile,(char*)buffer,utility_strlen((char*)buffer), &fbr);
    f_close(&gSDFile);
    ints_off();

    clearStatusMessage();
    ints_on();
    gWStrOffs -= 512;
}

void loadConfig()
{
    //The above code covers all cases just to make sure that we're not going to run into issues
    UINT fbr = 0,bytesToRead = 0,newConfig = 0;
    WCHAR* fss = &wstr_buf[gWStrOffs];

    if(!gSdDetected)
        return;

    gWStrOffs += 512;

    setStatusMessage("Loading config...");

    ints_on();

    config_init();

    utility_c2wstrcpy(fss,dxconf_cfg);

    f_close(&gSDFile);

    if(f_open(&gSDFile,fss, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        bytesToRead = gSDFile.fsize;

        if(bytesToRead > XFER_SIZE)
            bytesToRead = XFER_SIZE;

        ints_on();
        setStatusMessage("Reading config...");
        if(f_read(&gSDFile,(char*)buffer,bytesToRead, &fbr) == FR_OK)
        {
            setStatusMessage("Initializing configuration...");
            config_loadFromBuffer((char*)buffer,(int)fbr);

            gShortenMode = (short int)config_getI("shortenMode");

            CHEATS_DIR = config_getS("cheatsPath");
            IPS_DIR = config_getS("ipsPath");
            SAVES_DIR = config_getS("savesPath");
            CACHE_DIR = config_getS("cachePath");
            MD_32X_SAVE_EXT = config_getS("md32xSaveExt");
            SMS_SAVE_EXT = config_getS("smsSaveExt");
            BRM_SAVE_EXT = config_getS("brmSaveExt");

            if(CHEATS_DIR[0]=='/')CHEATS_DIR++;
            if(IPS_DIR[0]=='/')IPS_DIR++;
            if(SAVES_DIR[0]=='/')SAVES_DIR++;
            if(CACHE_DIR[0]=='/')CACHE_DIR++;
        }

        f_close(&gSDFile);
        ints_on();
    }
    else
    {
        newConfig = 1;

        setStatusMessage("Initializing configuration...");
        config_push("ipsPath",ips_dir_default);
        config_push("cheatsPath",cheats_dir_default);
        config_push("savesPath",saves_dir_default);
        config_push("cachePath",cache_dir_default);
        config_push("md32xSaveExt",md_32x_save_ext_default);
        config_push("smsSaveExt",sms_save_ext_default);
        config_push("brmSaveExt",brm_save_ext_default);
        config_push("shortenMode","0");
        config_push("romName","*");
        config_push("romType","0");

        utility_c2wstrcpy(fss,dxcore_dir); createDirectory(fss);
        utility_c2wstrcpy(fss,dxconf_cfg);

        if(f_open(&gSDFile, fss, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
        {
            config_saveToBuffer((char*)buffer);
            ints_on();

            f_write(&gSDFile,(char*)buffer,utility_strlen((char*)buffer), &fbr);

            ints_off();

            f_close(&gSDFile);
            ints_on();
        }
    }

    setStatusMessage("Validating configuration...");
    //BAd config? - fix it
    if(!CHEATS_DIR)
    {
        CHEATS_DIR = cheats_dir_default;
        config_push("cheatsPath",cheats_dir_default);
    }

    if(!IPS_DIR)
    {
        IPS_DIR = ips_dir_default;
        config_push("ipsPath",ips_dir_default);
    }

    if(!SAVES_DIR)
    {
        SAVES_DIR = saves_dir_default;
        config_push("savesPath",saves_dir_default);
    }

    if(!CACHE_DIR)
    {
        CACHE_DIR = cache_dir_default;
        config_push("cachePath",cache_dir_default);
    }

    if(!MD_32X_SAVE_EXT)
    {
        MD_32X_SAVE_EXT = md_32x_save_ext_default;
        config_push("md32xSaveExt",md_32x_save_ext_default);
    }

    if(!SMS_SAVE_EXT)
    {
        SMS_SAVE_EXT = sms_save_ext_default;
        config_push("smsSaveExt",sms_save_ext_default);
    }

    if(!BRM_SAVE_EXT)
    {
        BRM_SAVE_EXT = brm_save_ext_default;
        config_push("brmSaveExt",brm_save_ext_default);
    }

    setStatusMessage("Finalizing configuration...");
    WCHAR* buf = &wstr_buf[gWStrOffs];
    gWStrOffs += 512;

    //if config existed before, it's the responsibility of user to create directories
    //if it's new default config, we create directories here
    //if directories do not exist, they will be created by the code that saves to those directories
    utility_c2wstrcpy(buf,"/"); utility_c2wstrcat(buf,CHEATS_DIR); if(newConfig)createDirectoryFast(buf);
    utility_c2wstrcpy(buf,"/"); utility_c2wstrcat(buf,IPS_DIR); if(newConfig)createDirectoryFast(buf);
    utility_c2wstrcpy(buf,"/"); utility_c2wstrcat(buf,SAVES_DIR); if(newConfig)createDirectoryFast(buf);
    utility_c2wstrcpy(buf,"/"); utility_c2wstrcat(buf,CACHE_DIR); if(newConfig)createDirectoryFast(buf);

    clearStatusMessage();
    ints_on();
    gWStrOffs -= 1024;
}

/*Inputbox*/
int inputBox(char* result,const char* caption,const char* defaultText,short int  boxX,short int  boxY,
            short int  captionColor,short int boxColor,short int textColor,short int hlTextColor,short int maxChars)
{
    char* buf = result;
    char* in = (char*)&buffer[(XFER_SIZE*2) - 128];/*single character replacement*/
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$^&,-:\x83\0";
    unsigned short int buttons;
    const short int  numChars = utility_strlen(chars);
    short int cursorX,cursorY,cursorZ,cursorAbsoluteOffset,lastCursorAbsoluteOffset;
    short int len = utility_strlen(caption),len2;
    short int x = boxX - (len >> 1),x2;
    short int y = boxY;
    short int i;
    short int sync = 1;
    short int cx = x - 10;
    short int cy = y + 9;
    short int n = 0;
    short int nn = 0;
    short int lines = 0;
    short int visPerLine = (x + len + 10) - 2;
    short int inputOffs;

    ints_on();
    clear_screen();

    if(getClockType())
        inputboxDelay = 3;
    else
        inputboxDelay = 5;

    for(i = x - 10; i < x + 10; i++)
        printToScreen("\x82",i,y,boxColor);

    printToScreen(caption,x,y,captionColor);

    for(i = x + len; i <  x + len + 10; i++)
        printToScreen("\x82",i,y,boxColor);

    for(i = 0; i < numChars; i++)
    {
        if( (1 * n) >= visPerLine)
        {
            lines++;
            nn += n;

            buf[n] = '\0';

            printToScreen(buf,cx,cy,textColor);
            cx = x - 10;
            cy += 1;
            n = 0;
        }
        else
        {
            buf[n] = chars[n];
            ++n;
        }
    }

    //anything that doesn't fit
    if(numChars - nn)
    {
        lines++;
        cx = x - 10;
        n = 0;

        for(i = nn; i < numChars ; i++)
            buf[n++] = chars[i];

        buf[n] = '\0';
        printToScreen(buf,cx,cy,textColor);
    }

    //Fill rect
    for(i = x - 10; i < x + len + 10; i++)
    {
        printToScreen("\x82",i,y + 8,boxColor   & 0x000F);
        printToScreen("\x82",i,y + 11,boxColor  & 0x000F);
        printToScreen("\x82",i,y + 16,boxColor);//bottom line
    }

    /*cy = y + 9;

    for(i = 0; i < lines; i++)
    {
        printToScreen("\x82",(x - 10)-2,cy,boxColor);
        printToScreen("\x82",(x + len + 10)+2,cy,boxColor);
        ++cy;
    } */

    cursorX = x - 10;
    cursorY = y + 9;
    cursorZ = 0;
    cursorAbsoluteOffset = 0;
    lastCursorAbsoluteOffset = 0;

    len2 = utility_strlen(defaultText);
    if(len2 < maxChars)
        i = len2;
    else
        i = maxChars;
    x2 = (40 - i) >> 1;

    inputOffs = i;
    utility_memset(buf,'\0',maxChars);
    utility_memcpy(buf,defaultText,i);
    buf[i] = '\0';

    printToScreen("A",2 + 5,y + 17,0);  printToScreen("=Delete",3 + 5,y + 17,0x4000);
    printToScreen("B",11 + 5,y + 17,0); printToScreen("=Enter",12 + 5,y + 17,0x4000);
    printToScreen("C",19 + 5,y + 17,0); printToScreen("=Cancel",20 + 5,y + 17,0x4000);
    printToScreen("START",14,y + 18,0); printToScreen("=Submit",19,y + 18,0x4000);

    while(1)
    {
        delay(inputboxDelay);

        if(sync)
        {
            sync = 0;

            printToScreen(gEmptyLine,1,y+4,0);
            printToScreen(buf,(40 - utility_strlen(buf)) >> 1,y + 4,textColor);

            if(inputOffs < maxChars)
                printToScreen("_",(40 + inputOffs) >> 1,y + 4,0x4000);

            if(cursorAbsoluteOffset-lastCursorAbsoluteOffset)
            {
                in[0] = chars[cursorZ * (numChars >> 1) +( 1 * lastCursorAbsoluteOffset )];
                in[1] = '\0';
                printToScreen(in,cursorX + (1 * lastCursorAbsoluteOffset),cursorY + cursorZ,textColor);

                lastCursorAbsoluteOffset = cursorAbsoluteOffset;
            }

            in[0] = chars[cursorZ * (numChars >> 1) + (1 * cursorAbsoluteOffset)];
            in[1] = '\0';
            printToScreen(in,cursorX + (1 * cursorAbsoluteOffset),cursorY + cursorZ,hlTextColor);
        }

        buttons = get_pad(0);
        if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
        {
            buttons = get_pad(1);
            if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
            {
                // no controllers, loop until one plugged in
                delay(20);
                continue;
            }
        }
        // check if buttons changed
        if ((buttons & SEGA_CTRL_BUTTONS) != gButtons)
        {
            unsigned short int changed = (buttons & SEGA_CTRL_BUTTONS) ^ gButtons;
            gButtons = buttons & SEGA_CTRL_BUTTONS;

            if ((changed & SEGA_CTRL_A) && (buttons & SEGA_CTRL_A))
            {
                sync = 1;

                if(inputOffs <= 0)
                {
                    buf[0] = '\0';
                    inputOffs = 0;
                    continue;
                }

                if(inputOffs > 0)
                    buf[--inputOffs] = '\0';

                continue;
            }

            if ((changed & SEGA_CTRL_C) && (buttons & SEGA_CTRL_C))//cancel
                return 0;

            if ((changed & SEGA_CTRL_B) && (buttons & SEGA_CTRL_B))
            {
                sync = 1;

                if(inputOffs > maxChars - 1)
                    inputOffs = maxChars - 1;

                char c = chars[cursorZ * (numChars >> 1) + (1 * cursorAbsoluteOffset)];

                if(c != '\x83')
                    buf[inputOffs++] = c;
                else
                    buf[inputOffs++] = ' ';

                buf[inputOffs] = '\0';

                continue;
            }

            if ((changed & SEGA_CTRL_START) && (buttons & SEGA_CTRL_START))//start
                break;
        }

        // always check dpad, not just when changed
        {
            if (buttons & SEGA_CTRL_UP)
            {
                sync = 1;

                {
                    short int z = cursorZ;

                    if(cursorZ >-1)
                        cursorZ--;

                    if(cursorZ < 0)
                        cursorZ = 0;

                    in[0] = chars[z * (numChars >> 1) + (1 * lastCursorAbsoluteOffset )];
                    in[1] = '\0';
                    printToScreen(in,cursorX + (1 * lastCursorAbsoluteOffset),cursorY + z,textColor);

                    lastCursorAbsoluteOffset = cursorAbsoluteOffset =  cursorZ * (numChars >> 1) + (1 * lastCursorAbsoluteOffset);
                }

                continue;
            }//up

            if (buttons & SEGA_CTRL_DOWN)
            {
                sync = 1;

                {
                    short int z = cursorZ;

                    if(cursorZ < lines - 1)
                        cursorZ++;
                    else
                        continue;

                    in[0] = chars[z * (numChars >> 1) + (1 * lastCursorAbsoluteOffset )];
                    in[1] = '\0';
                    printToScreen(in,cursorX + (1 * lastCursorAbsoluteOffset),cursorY + z,textColor);

                    lastCursorAbsoluteOffset = cursorAbsoluteOffset =  z * (numChars >> 1) + (1 * lastCursorAbsoluteOffset);
                }

                continue;
            }//down

            if (buttons & SEGA_CTRL_LEFT)
            {
                sync = 1;

                cursorAbsoluteOffset--;

                if(cursorAbsoluteOffset < 0)
                {
                    short int z = cursorZ;

                    if(cursorZ < lines - 1)
                        cursorZ++;
                    else
                        cursorZ = 0;

                    in[0] = chars[z * (numChars >> 1) + (1 * lastCursorAbsoluteOffset )];
                    in[1] = '\0';
                    printToScreen(in,cursorX + (1 * lastCursorAbsoluteOffset),cursorY + z,textColor);

                    lastCursorAbsoluteOffset = cursorAbsoluteOffset =  visPerLine-1;
                }

                continue;
            }//left

            if (buttons & SEGA_CTRL_RIGHT)
            {
                sync = 1;

                cursorAbsoluteOffset++;

                if(cursorAbsoluteOffset > visPerLine-1)
                {
                    short int z = cursorZ;

                    if(cursorZ < lines - 1)
                        cursorZ++;
                    else
                        cursorZ = 0;

                    in[0] = chars[z * (numChars >> 1) + (1 * lastCursorAbsoluteOffset )];
                    in[1] = '\0';
                    printToScreen(in,cursorX + (1 * lastCursorAbsoluteOffset),cursorY + z,textColor);

                    lastCursorAbsoluteOffset = cursorAbsoluteOffset =  0;
                }

                continue;
            }//right
        }
    }

    buf[inputOffs] = '\0';
    return inputOffs;
}

/* main code entry point */

int main(void)
{
    int ix;
//    char temp[64];                      /* keep in sync with RTC print below! */

#ifndef RUN_IN_PSRAM
    init_hardware();                    /* set hardware to a consistent state, clears vram and loads the font */
    gCardOkay = neo_check_card();       /* check for Neo Flash card */
#else
    gCardOkay = 0;                      /* have Neo Flash card - duh! */
#endif
    gCpldVers = neo_check_cpld();       /* get CPLD version */
    gCardType = *(short int *)0x00FFF0; /* get flash card type from menu flash */

    neo_hw_info(&gCart);

    //ints_on();                          /* allow interrupts */

    // set long file name pointers
    for (ix=0; ix<MAX_ENTRIES; ix++)
    {
        gSelections[ix].name = &lfnames[ix * 256];
        lfnames[ix * 256] = (WCHAR)0;
    }

#ifndef RUN_IN_PSRAM
    {
        WCHAR* fss = &wstr_buf[gWStrOffs];
        gWStrOffs += 512;

        gCurEntry = 0;
        gStartEntry = 0;
        gMaxEntry = 0;

        neo2_enable_sd();
        get_sd_directory(-1);           /* get root directory of sd card */
        if(gSdDetected)
        {
            utility_c2wstrcpy(fss, "/menu/md/MDEBIOS.BIN");
            if(f_open(&gSDFile, fss, FA_OPEN_EXISTING | FA_READ) == FR_OK)
            {
                gCurEntry = 0;
                utility_c2wstrcpy(path, "/menu/md");
                utility_c2wstrcpy(gSelections[gCurEntry].name, "MDEBIOS.BIN");
                gSelections[gCurEntry].type = 0;
                gSelections[gCurEntry].bbank = 0;
                gSelections[gCurEntry].bsize = 0;
                gSelections[gCurEntry].offset = 0;
                gSelections[gCurEntry].length = gSDFile.fsize;
                gSelections[gCurEntry].run = 0x27;
                f_close(&gSDFile);
                gCurMode = MODE_SD;
                run_rom(0x0000);        /* never returns */
            }
        }
        //neo2_disable_sd();
        gWStrOffs -= 512;
    }
#endif

    #ifdef  __DO_PROFILING__
    unsigned int i;

    Z80_requestBus(1);

    for (i=0; i<0x2000; i++)
        *(unsigned char  *)(Z80_RAM + i) = 0;

    Z80_setBank(0);

    // upload Z80 driver
    for (i=0; i<sizeof(z80_thread); i++)
        *(unsigned char *)(Z80_RAM + i) = *(unsigned char*)(z80_thread + i);

    // reset Z80
    Z80_startReset();
    Z80_releaseBus();
    Z80_endReset();

    Z80_THREAD_IDLE();//idle state

    /*do_profilingPrint(
            "utility_memset",
            utility_memset((char*)&buffer[0],'\0',WORK_RAM_SIZE),
            1,
            1);
    delay(100);*/
    #endif


//  ints_off();                         /* disable interrupts */
//  neo_get_rtc(rtc);                   /* get current time from Neo2/3 flash cart */
//  ints_on();                          /* enable interrupts */

    ints_on();
    cache_invalidate_pointers();
    cheat_invalidate();                 /*Invalidate cheat list*/
    ipsPath[0] = 0;
    gImportIPS = 0;
    gSelectionSize = 0;
    gManageSaves = 0;
    gCurMode = MODE_SD;
#ifdef RUN_IN_PSRAM
    gCurEntry = 0;
    gStartEntry = 0;
    gMaxEntry = 0;
    neo2_enable_sd();
    get_sd_directory(-1);               /* get root directory of sd card */
#endif

    cache_invalidate_pointers();
    utility_memcpy(gCacheBlock.sig,"DXCS",4);
    gCacheBlock.processed = 0;
    gCacheBlock.version = CACHEBLK_VERS;

    if(gSdDetected)
    {
#if 0
    dump_zipram(16*1024,1);
#endif
        clear_screen();
        setStatusMessage("Loading cache & configuration...");
        loadConfig();

        char* p = config_getS("romName");
        STEP_INTO("Checking last loaded rom..");
        if(p)
        {
            ints_off();
            if(utility_strlen(p) > 2)
            {
                WCHAR* buf = &wstr_buf[gWStrOffs];
                gWStrOffs += 512;

                utility_c2wstrcpy(buf,p);
                cache_loadPA(buf,1);

                if(p[0] == '*')
                {
                    STEP_INTO("SMGR_STATUS_NULL");
                    gSRAMgrServiceStatus = SMGR_STATUS_NULL;
                }
                else if(gSRAMgrServiceStatus == SMGR_STATUS_BACKUP_SRAM)
                {
                    STEP_INTO("SMGR_STATUS_BACKUP_SRAM");
                    gSRAMgrServiceMode = (short int)config_getI("romType");
                    sram_mgr_saveGamePA(buf);
                    setStatusMessage("Loading cache & configuration...");

                    config_push("romName","*");
                    config_push("romType","0");

                    gSRAMgrServiceStatus = SMGR_STATUS_NULL; //just in case mute cache even if the config has been updated
                    gCacheOutOfSync = 1;
                    cache_sync(0);
                    updateConfig();
                }
                gWStrOffs -= 512;
            }
        }
        setStatusMessage("Loading cache & configuration...OK");
        clear_screen();
    }

    cache_invalidate_pointers();

    if(gSdDetected&&(gMaxEntry>0))
    {
        ints_on();
        clearStatusMessage();
    }
    else
    {
        if (!IS_NEO3)
        {
            neo2_disable_sd();

            ints_on();
            clearStatusMessage();

            /* starts in flash mode, so set gSelections from menu flash */
            gCurEntry = 0;
            gStartEntry = 0;
            gMaxEntry = 0;
            gCurMode = MODE_FLASH;
            get_menu_flash();
        }
    }

    unsigned short maxDL;
    gResponseMsgStatus = 0;

    *(unsigned short*)rom_hdr = 0xffff;
    maxDL = (getClockType()) ? 50 : 60;
    update_sd_display();
    //force load static map
    gChangedPage = 1; update_sd_display(); gChangedPage = 0;
    gRomDly = maxDL >> 1;
    gLastEntryIndex = -1;
    utility_memset(entrySNameBuf,'\0',64);
    utility_memset(gProgressBarStaticBuffer,0x87,36);
    gProgressBarStaticBuffer[32] = '\0';
    gCacheOutOfSync = 0;

    while(1)
    {
        unsigned short int buttons;
//      unsigned char now[8];

        delay(1);

        gRomDly = (gRomDly > maxDL) ? maxDL : gRomDly;

        if (gRomDly)
        {
            // decrement time to try loading rom header
            --gRomDly;

            if (gRomDly)
            {
                if(!gResponseMsgStatus)
                {
                    gResponseMsgStatus = 1;

                    if(gMaxEntry && gCurEntry)
                    {
                        if( (gSelections[gCurEntry].type != 4))
                        {
                            printToScreen(gEmptyLine,1,20,0x0000);
                            printToScreen(gEmptyLine,1,21,0x0000);
                            printToScreen("Waiting for response...",20 - (utility_strlen("Waiting for response...") >> 1),21,0x2000);
                            printToScreen(gEmptyLine,1,22,0x0000);
                            printToScreen(gEmptyLine,1,23,0x0000);
                        }
                    }
                }
                *(unsigned short*)rom_hdr = 0xffff;//rom_hdr[0] = rom_hdr[1] = 0xFF;
            }
            else
            {
                gResponseMsgStatus = 1;
                gRomDly = 0;
                gUpdate = 1;
            }
        }

        if (gUpdate)
            update_display();


//      ints_off();                 /* disable interrupts */
//      neo_get_rtc(now);               /* get current time from Neo2/3 flash cart */
//      ints_on();                 /* enable interrupts */
//      if (memcmp(rtc, now, 8))
//      {
//          memcpy(rtc, now, 8);
//          gCursorY = 19;
//          sprintf(temp, " %02d:%01d%01d:%01d%01d ", rtc[4]&31, rtc[3]&7, rtc[2]&15, rtc[1]&7, rtc[0]&15);
//          gCursorX = 20 - utility_strlen(temp)/2;     /* center time */
//          put_str(temp, 0);
//      }

        buttons = get_pad(0);
        if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
        {
            buttons = get_pad(1);
            if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
            {
                // no controllers, loop until one plugged in
                delay(20);
                continue;
            }
        }

        {
            if (  (buttons & SEGA_CTRL_UP))
            {
                // UP pressed, go one entry back
                gLastEntryIndex = gCurEntry;
                gCurEntry--;

                if (gCurEntry < 0)
                {
                    gChangedPage = 1;
                    gCurEntry = gMaxEntry - 1;
                    gStartEntry = gMaxEntry - (gMaxEntry % PAGE_ENTRIES);
                }
                if (gCurEntry < gStartEntry)
                {
                    gChangedPage = 1;
                    gStartEntry -= PAGE_ENTRIES; // previous "page" of entries

                    if (gStartEntry < 0)
                        gStartEntry = 0;
                }
                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                update_sd_display(); // quick hack to remove flickering
                gRomDly += (gCurMode == MODE_SD) ? 10 : 5;
                delay( (getClockType()) ? 4 : 6 );
                continue;
            }
            if ( (buttons & SEGA_CTRL_LEFT))
            {
                // LEFT pressed, go one page back
                gLastEntryIndex = gCurEntry;
                gCurEntry -= PAGE_ENTRIES;

                if (gCurEntry < 0)
                {
                    gChangedPage = 1;
                    gLastEntryIndex = gCurEntry = gMaxEntry - 1;
                    gStartEntry = gMaxEntry - (gMaxEntry % PAGE_ENTRIES);
                }

                if (gCurEntry < gStartEntry)
                {
                    gChangedPage = 1;
                    gStartEntry -= PAGE_ENTRIES; // previous "page" of entries

                    if (gStartEntry < 0)
                        gStartEntry = 0;
                }
                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                update_sd_display(); // quick hack to remove flickering
                gRomDly += (gCurMode == MODE_SD) ? 20 : 10;
                delay( (getClockType()) ? 10 : 14 );
                continue;
            }
            if (  (buttons & SEGA_CTRL_DOWN))
            {
                // DOWN pressed, go one entry forward
                gLastEntryIndex = gCurEntry;
                gCurEntry++;

                if (gCurEntry == gMaxEntry)
                {
                    gChangedPage = 1;
                    gLastEntryIndex = gCurEntry = gStartEntry = 0;    // wrap around to top
                }

                if ((gCurEntry - gStartEntry) == PAGE_ENTRIES)
                {
                    gChangedPage = 1;
                    gStartEntry += PAGE_ENTRIES; // next "page" of entries
                }

                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                update_sd_display(); // quick hack to remove flickering
                gRomDly += (gCurMode == MODE_SD) ? 10 : 5;
                delay( (getClockType()) ? 4 : 6 );
                continue;
            }
            if (  (buttons & SEGA_CTRL_RIGHT))
            {
                // RIGHT pressed, go one page forward
                gLastEntryIndex = gCurEntry;
                gCurEntry += PAGE_ENTRIES;

                if (gCurEntry >= gMaxEntry)
                {
                    gChangedPage = 1;
                    gLastEntryIndex = gCurEntry = gStartEntry = 0;    // wrap around to top
                }

                if ((gCurEntry - gStartEntry) >= PAGE_ENTRIES)
                {
                    gChangedPage = 1;
                    gStartEntry += PAGE_ENTRIES; // next "page" of entries
                }

                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                update_sd_display(); // quick hack to remove flickering
                gRomDly += (gCurMode == MODE_SD) ? 20 : 10;
                delay( (getClockType()) ? 10 : 14 );
                continue;
            }
        }

        // check if buttons changed
        if ((buttons & SEGA_CTRL_BUTTONS) != gButtons)
        {
            unsigned short int changed = (buttons & SEGA_CTRL_BUTTONS) ^ gButtons;
            gButtons = buttons & SEGA_CTRL_BUTTONS;

            if ((changed & SEGA_CTRL_START) && !(buttons & SEGA_CTRL_START))
            {
                // START released, change mode (unless Neo3-SD)
                if (!IS_NEO3)
                {
                    gCurEntry = 0;
                    gStartEntry = 0;
                    gMaxEntry = 0;

                    gCurMode++;
                    if (gCurMode == MODE_END)
                    {
                        neo2_disable_sd();
                        gCurMode = MODE_FLASH;
                        get_menu_flash();
                    }
                    if (gCurMode == MODE_SD)
                    {
                        if (IS_FLASH)
                        {
                            neo2_disable_sd();
                            gCurMode = MODE_FLASH;
                            get_menu_flash();
                        }
                        else
                        {
                            gCursorY = 0;
                            neo2_enable_sd();
                            get_sd_directory(-1);   /* get root directory of sd card */
                            loadConfig();
                        }
                    }
                }
                else
                {
                    gCurEntry = 0;
                    gStartEntry = 0;
                    gMaxEntry = 0;

                    // reload root dir on Neo3
                    gCursorY = 0;
                    neo2_enable_sd();
                    get_sd_directory(-1);   /* get root directory of sd card */
                    loadConfig();
                }

                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                gUpdate = -1;
                continue;
            }

            if ((changed & SEGA_CTRL_MODE) && !(buttons & SEGA_CTRL_MODE))
            {
                char p[4];
                // MODE released, change shorten filename mode
                gShortenMode = (gShortenMode < 2) ? gShortenMode + 1 : 0;
                gUpdate = -1;
                p[0] = 0x30 + gShortenMode;
                p[1] = 0;
                config_push("shortenMode", p);
                updateConfig();
                continue;
            }

            if ((changed & SEGA_CTRL_A) && !(buttons & SEGA_CTRL_A))
            {
                // A released
                if (gCurMode == MODE_FLASH)
                {
                    // call options screen, which will either return or run the rom
                   // if (gSelections[gCurEntry].type < 3)
                        do_options();
                }
                else if (gCurMode == MODE_USB)
                {
                    // activate USB and wait for user to press A
                    ints_off();     /* disable interrupts */
                    gCursorX = 13;
                    gCursorY = 3;
                    put_str(" USB Active ", 0x2000);
                    gCursorX = 10;
                    gCursorY = 4;
                    put_str("Press A to continue", 0);
                    set_usb();
                    ints_on();     /* enable interrupts */
                    gUpdate = -1;
                    continue;
                }
                else if (gCurMode == MODE_SD)
                {
                    // call options screen, which will either return or run the rom
                  //  if (gSelections[gCurEntry].type < 3)
                        do_options();
                }
            }
            if ((changed & SEGA_CTRL_B) && !(buttons & SEGA_CTRL_B))
            {
                // B released
                if ((gCurMode == MODE_SD) && (gSelections[gCurEntry].type == 128))
                {
                    // get selected subdirectory
                    get_sd_directory(gCurEntry);
                    gCurEntry = 0;
                    gStartEntry = 0;
                    gUpdate = -1;
                    continue;
                }

                if ((gCurMode == MODE_SD) && (gMaxEntry == 0))
                {
                    // no entries last time SD card checked... check again
                    gCursorY = 0;
                    neo2_enable_sd();
                    get_sd_directory(-1);   /* get root directory of sd card */
                    loadConfig();
                    gUpdate = -1;
                    continue;
                }

                // check for write-enable psram flag
                if (gSelections[gCurEntry].run & 64)
                {
                    gSelections[gCurEntry].run &= 63;
                    gWriteMode = 0x0002; // write-enable psram when run
                }

                run_rom(0x0000);
            }
            if ((changed & SEGA_CTRL_C) && !(buttons & SEGA_CTRL_C))
            {
                // C released
                if ((gCurMode == MODE_SD) && (gSelections[gCurEntry].type == 128))
                {
                    // get selected subdirectory
                    get_sd_directory(gCurEntry);
                    gCurEntry = 0;
                    gStartEntry = 0;
                    gUpdate = -1;
                    continue;
                }

                if ((gCurMode == MODE_SD) && (gMaxEntry == 0))
                {
                    // no entries last time SD card checked... check again
                    gCursorY = 0;
                    neo2_enable_sd();
                    get_sd_directory(-1);   /* get root directory of sd card */
                    loadConfig();
                    gUpdate = -1;           /* clear screen for major screen update */
                    continue;
                }

                // check for write-enable psram flag
                if (gSelections[gCurEntry].run & 64)
                {
                    gSelections[gCurEntry].run &= 63;
                    gWriteMode = 0x0002; // write-enable psram when run
                }

                run_rom(0x00FF);
            }
            if ((changed & SEGA_CTRL_Z) && !(buttons & SEGA_CTRL_Z))
            {
                // Z released
                gHelp ^= 1;
                gUpdate = 1;
            }

        }
    }

    return 0;
}

int wait_for_buttons(unsigned short initial)
{
    unsigned short int buttons;

    while(1)
    {
        delay(2);
        buttons = get_pad(0);

        if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
        {
            buttons = get_pad(1);
            if ((buttons & SEGA_CTRL_TYPE) == SEGA_CTRL_NONE)
            {
                // no controllers, loop until one plugged in
                delay(20);
                continue;
            }
        }

        if ((buttons & SEGA_CTRL_BUTTONS) != initial)
            break;
    }

    return buttons & SEGA_CTRL_BUTTONS;
}

unsigned char hw_tst_dbg_x,hw_tst_dbg_y;

int deci_to_tile_units(int input,int spacing,int unit)//0..999
{
    if (input < 10) {
        return (spacing * unit) + unit;
    } else if (input >= 100) {
        return (spacing * unit) + ((unit<<2) - unit);
    }

    return (spacing * unit) + (unit << 1);
}

void hw_tst_wait_event(unsigned short events)
{
    unsigned short buttons = 0;

    while (!buttons)
        buttons = wait_for_buttons(0) & events;
    while (buttons)
        buttons = wait_for_buttons(buttons) & events;
}

void hw_tst_new_ln()
{
    if((++hw_tst_dbg_y) <= 23)
    {
        return;
    }

    hw_tst_dbg_y = 5;
}

void hw_tst_follow(const char* msg,int color)
{
    printToScreen(msg,hw_tst_dbg_x,hw_tst_dbg_y,((unsigned short)color));
    hw_tst_new_ln();
}

void hw_gen_pattern_16KB(unsigned char* block,int* seed,int* f)
{
    register unsigned char* a0 = block;
    register unsigned char* a1 = a0 + (16 * 1024);
    register unsigned char d0 = (unsigned char)*seed;
    register int d1 = *f;

    do
    {
        *a0 = ((d1 << 8)) - (1 + (d0--));
        d1 ^= 1;
    }while((++a0) < a1);

    *f = d1;
    *seed = d0;
}

void hw_tst_prologue(const char* s)
{
    int ix;

    ints_off();
    neo2_disable_sd();

    ints_on();
    clear_screen();

    gCursorX = 1;
    gCursorY = 1;
    put_str("\x80\x82\x82\x82\x82", 0x2000);
    gCursorX = 6;
    put_str("  HARDWARE SELF TEST MODE ", 0x4000);
    gCursorX = 34;
    put_str("\x82\x82\x82\x82\x85", 0x2000);

    gCursorX = 1;
    gCursorY = 2;
    put_str(gFEmptyLine, 0x2000);

    // list area (if applicable)
    gCursorY = 3;
    for (ix=0; ix<22; ix++, gCursorY++)
        put_str(gFEmptyLine, 0x2000);

    gCursorX = 1;
    gCursorY = 24;
    put_str(gFEmptyLine, 0x2000);
    gCursorX = 1;
    gCursorY = 25;
    put_str(gFBottomLine, 0x2000);
    hw_tst_dbg_x = 9;
    hw_tst_dbg_y = 3;
    hw_tst_follow(s,0x2000);
    hw_tst_dbg_y = 5;
    hw_tst_dbg_x = 8;
}

void hw_tst_epilogue()
{
    hw_tst_new_ln();
    hw_tst_follow("Job finished!",0);
    hw_tst_follow("Press 'B' to exit",0x2000);

    ints_on();
    hw_tst_wait_event(SEGA_CTRL_A | SEGA_CTRL_B);

    gCurEntry = 0;
    gStartEntry = 0;
    gMaxEntry = 0;
    if (gCurMode == MODE_SD)
    {
        gCursorY = 0;
        neo2_enable_sd();
        get_sd_directory(-1);   /* get root directory of sd card */
        loadConfig();
    }
    else
    {
        gCurMode = MODE_FLASH;
        neo2_disable_sd();
        get_menu_flash();
    }

    clear_screen();
    gUpdate = -1;
    gButtons = 0;
    delay(5);
}

void hw_tst_dump()
{

}

void hw_tst_myth_psram_test(int selection)
{
    extern int neo_test_myth_psram(int pstart, int len, void *save);
    int i, e;
    char mbs[4];

    hw_tst_prologue("TESTING ONBOARD PSRAM");
    //==================================================

    hw_tst_dbg_x += 4;
    hw_tst_follow("Testing...",0x0000);
    hw_tst_new_ln();
    ints_off();

    int a = hw_tst_dbg_x - (6 + 4);
    int b = a + 30 + 5;
    int x = a;
    int y = hw_tst_dbg_y;
    int z = -1;

    for(e = 0,i = 0;i<64*(128*1024);)
    {
        if ((i>>17) > z)
        {
            z = i>>17;
            UTIL_IntegerToString(mbs,z+1,10);
            printToScreen(mbs,x,y,0);
            x += deci_to_tile_units(z + 1,1,1);
            if (x >= b) {x = a;++y;}
        }

#ifdef RUN_IN_PSRAM
        if (i >= (56*(128*1024))) //Don't overwrite RAM MENU
        {
            if(neo_test_myth_psram(i, WORK_RAM_SIZE, buffer) != 0)
            {
                e = 1;
                ints_on();
                hw_tst_follow("Testing...FAILED!",0x4000);
                break;
            }
            i += WORK_RAM_SIZE;
        }
        else
#endif
        {
            if(neo_test_myth_psram(i, WORK_RAM_SIZE, 0) != 0)
            {
                e = 1;
                ints_on();
                hw_tst_follow("Testing...FAILED!",0x4000);
                break;
            }
            i += WORK_RAM_SIZE;
        }
    }

    hw_tst_dbg_y = y + 2;
    ints_on();
    if(!e)
    {
        hw_tst_follow("Testing...PASSED!",0x2000);
    }

    hw_tst_dump();

    //==================================================
    hw_tst_epilogue();
}

void hw_tst_psram_test(int selection)
{
    extern int neo_test_psram(int pstart, int len);
    int i, e;
    char mbs[4];

    hw_tst_prologue("TESTING NEO2    PSRAM");
    //==================================================
    hw_tst_dbg_x += 4;
    hw_tst_follow("Testing...",0x0000);
    hw_tst_new_ln();
    ints_off();

    int a = hw_tst_dbg_x - (6 + 4);
    int b = a + 30 + 5;
    int x = a;
    int y = hw_tst_dbg_y;
    int z = -1;

    for(e = 0,i = 0;i<128*(128*1024);i += WORK_RAM_SIZE)
    {
        if ((i>>17) > z)
        {
            z = i>>17;
            UTIL_IntegerToString(mbs,z+1,10);
            printToScreen(mbs,x,y,0);
            x += deci_to_tile_units(z + 1,1,1);
            if (x >= b) {x = a;++y;}
        }

        if(neo_test_psram(i, WORK_RAM_SIZE) != 0)
        {
            e = 1;
            ints_on();
            hw_tst_follow("Testing...FAILED!",0x4000);
            break;
        }
    }

    hw_tst_dbg_y = y + 2;
    ints_on();
    if(!e)
    {
        hw_tst_follow("Testing...PASSED!",0x2000);
    }

    hw_tst_dump();

    //==================================================
    hw_tst_epilogue();
}

void hw_tst_sram_write(int selection)
{
    int i;
    int seed,f;

    hw_tst_prologue("TESTING SRAM(write)");
    //==================================================

    hw_tst_follow("Writing seed...",0x0000);
    hw_tst_new_ln();

    ints_off();
    for(seed = 0,f = 1,i = 0;i<2*(128*1024);i += 16 * 1024)
    {
        hw_gen_pattern_16KB(&buffer[0],&seed,&f);
        neo_copyto_sram(&buffer[0],i,16*1024);
    }
    ints_on();

    hw_tst_follow("Writing seed...FINISHED!",0x2000);

    //==================================================
    hw_tst_epilogue();
}

void hw_tst_sram_read(int selection)
{
    int i,e;
    int seed,f;

    hw_tst_prologue("TESTING SRAM(read)");
    //==================================================

    hw_tst_follow("Testing SRAM...",0x0000);
    hw_tst_new_ln();

    ints_off();
    for(e = 0,seed = 0,f = 1,i = 0;i<2*(128*1024);i += 16 * 1024)
    {
        hw_gen_pattern_16KB(&buffer[0],&seed,&f);
        neo_copyfrom_sram(&buffer[16*1024],i,16*1024);

        if(utility_word_cmp(&buffer[0],&buffer[16*1024],16*1024) != 0)
        {
            ints_on();
            e = 1;
            hw_tst_follow("Testing SRAM...FAILED!",0x4000);
            break;
        }
    }

    ints_on();

    if(!e)
    {
        hw_tst_follow("Testing SRAM...PASSED!",0x2000);
    }

    hw_tst_dump();

    //==================================================
    hw_tst_epilogue();
}
#if 0
void dump_zipram(int len,int gen)//Not a real dumping routine.Its more like response dumper :)
{
    int i,j;
    WCHAR* fnbuf = &wstr_buf[gWStrOffs];
    FIL f;
    UINT ts;

    len = (len > XFER_SIZE) ? XFER_SIZE : len;
    gWStrOffs += 512;

    utility_c2wstrcpy(fnbuf,"/menu/md/ZIP.BIN");
    memset(&f,0,sizeof(FIL));

    ints_on();
    if(f_open(&f,fnbuf,FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        gWStrOffs -= 512;
        return;
    }

    ints_off();
    if (gen)
    {
        for (i = 0;i < len;i += 256)
        {
            for (j = 0;j < 256;j++)
                buffer[i + j] = j;
        }
        neo_copyto_psram(&buffer[0],0,len);
    }

    neo_copyfrom_psram(&buffer[len],0,len);

    //write resp
    ints_on();
    f_write(&f,&buffer[len],len,&ts);

    f_close(&f);
    gWStrOffs -= 512;
}
#endif


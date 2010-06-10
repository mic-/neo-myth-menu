/* Neo Super 32X/MD/SMS Flash Cart Menu by Chilly Willy, based on Dr. Neo's Menu code */
/* The license on this code is the same as the original menu code - MIT/X11 */

/*std*/
#include <string.h>
#include <stdio.h>

/*timing*/
#define SD_READ_TIME_NTSC_MIN (32)
#define SD_READ_TIME_NTSC_MAX (42)
#define SD_READ_TIME_PAL_MIN (22)
#define SD_READ_TIME_PAL_MAX (32)

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

#define min(x,y) (((x)<(y))?(x):(y))
#define max(x,y) (((x)>(y))?(x):(y))

/*tables*/
#define EEPROM_MAPPERS_COUNT (26)
static const char* EEPROM_MAPPERS[EEPROM_MAPPERS_COUNT] =  //shared from genplus gx
{
    "T-8104B",//new - nba jam 32x
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
    "T-120146-50"
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

/*Save manager service status*/
enum
{
    SMGR_STATUS_NULL = 0,
    SMGR_STATUS_BACKUP_SRAM
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
};

static CacheBlock gCacheBlock;/*common cache block*/

static CheatEntry cheatEntries[CHEAT_ENTRIES_COUNT];
static short registeredCheatEntries = 0;

#define dxcore_dir "/.menu/md"
#define dxconf_cfg "/.menu/md/DXCONF.CFG"
#define cheats_dir_default ".menu/md/cheats"
#define ips_dir_default ".menu/md/ips"
#define saves_dir_default ".menu/md/saves"
#define cache_dir_default ".menu/md/cache"
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

#ifndef RUN_IN_PSRAM
static const char gAppTitle[] = "Neo Super 32X/MD/SMS Menu v2.3";
#else
static const char gAppTitle[] = "NEO Super 32X/MD/SMS Menu v2.3";
#endif

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
short int gCpldVers;			/* 3 = V11 hardware, 4 = V12 hardware, 5 = V5 hardware */
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

extern unsigned short cardType;         /* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */

extern unsigned int num_sectors;        /* number of sectors on SD card (or 0) */
extern unsigned char *sd_csd;           /* card specific data */

FATFS gSDFatFs;                         /* global FatFs structure for FF */
FIL gSDFile;                            /* global file structure for FF */

short int gSdDetected = 0;                   /* 0 - not detected, 1 - detected */

#define gRomDly_default_flash (0)

int gRomDly_default_sd = 20;
static int gTime1,gTime2;

/* global options table entry definitions */

typedef struct {
    char *name;                         /* option name string */
    char *value;                        /* option value string */
    void (*callback)(int index);        /* callback when C pressed */
    void (*patch)(int index);           /* callback after rom loaded */
    void* userData;                     /* added for cheat support*/
    short exclusiveFCall;               /* required for the new exclusive function calls */
} optionEntry;

#define WORK_RAM_SIZE (XFER_SIZE*2)

/* global options */
short int gResetMode = 0x0000;          /* 0x0000 = reset to menu, 0x00FF = reset to game */
short int gGameMode = 0x00FF;           /* 0x0000 = menu mode, 0x00FF = game mode */
short int gWriteMode = 0x0000;          /* 0x0000 = write protected, 0x0002 = write-enabled */
short int gSRAMType;                    /* 0x0000 = sram, 0x0001 = eeprom */
short int gSRAMBank;                    /* sram bank = 0 to 15, constrained by size of sram */
short int gSRAMSize;                    /* size of sram in 8KB units */

short int gYM2413 = 0x0001;             /* 0x0000 = YM2413 disabled, 0x0001 = YM2413 enabled */
short int gImportIPS = 0;

/* arrays */
const char *gEmptyLine = "                                      ";
const char *gFEmptyLine = "\x7C                                    \x7C";
const char *gLine  = "\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82";
const char *gFBottomLine = "\x86\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x83";

char gSRAMSizeStr[8];
char gSRAMBankStr[8];

optionEntry gOptions[OPTION_ENTRIES];   /* entries for global options */

WCHAR ipsPath[512];                     /* ips path */
WCHAR path[512];                        /* SD card file path */
WCHAR *lfnames = (WCHAR *)(0x400000 - MAX_ENTRIES * 512); /* space for long file names in PSRAM */

unsigned char rtc[8];                   /* RTC from Neo2/3 flash cart */

unsigned char __attribute__((aligned(16))) rom_hdr[256];             /* rom header from selected rom (if loaded) */

unsigned char __attribute__((aligned(16))) buffer[XFER_SIZE*2];      /* Work RAM buffer - big enough for SMD decoding */

selEntry_t gSelections[MAX_ENTRIES];    /* entries for flash or current SD directory */

const short int fsz_tbl[16] = { 0,1,2,0,4,5,6,0,8,16,24,32,40,0,0,0 };

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
void cache_sync();
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
extern void neo_run_game(int fstart, int fsize, int bbank, int bsize, int run);
extern void neo_run_psram(int pstart, int psize, int bbank, int bsize, int run);
extern void neo_run_myth_psram(int psize, int bbank, int bsize, int run);
extern void neo_copy_game(unsigned char *dest, int fstart, int len);
extern void neo_copyto_psram(unsigned char *src, int pstart, int len);
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

static int inputboxDelay = 5;

//macros
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

inline void setStatusMessage(const char* msg)
{
    printToScreen(gEmptyLine, 1, 23, 0x0000);
    printToScreen(msg, (40 - utility_strlen(msg))>>1, 23, 0x0000);
}

inline void clearStatusMessage()
{
    printToScreen(gEmptyLine, 1, 23, 0x0000);
}

inline int directoryExists(const XCHAR* fps)
{
    DIR dir;

    if(f_opendir(&dir,fps) != FR_NO_PATH)
        return 1;

    return 0;
}

inline int fileExists(const XCHAR* fss)
{
    f_close(&gSDFile);

    if(f_open(&gSDFile, fss, FA_OPEN_EXISTING | FA_READ) != FR_OK)
        return 0;

    f_close(&gSDFile);
    return 1;
}

void makeDir(const XCHAR* fps)
{
    WCHAR* ss = (WCHAR*)&buffer[XFER_SIZE >> 1];
    int i,l,j,di;

    utility_strcpy((char*)ss,"Creating directory: ");
    utility_w2cstrcpy((char*)(ss+10),fps);
    setStatusMessage((char*)ss);

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
}

inline int createDirectory(const XCHAR* fps)
{
    if(directoryExists(fps))
        return 1;

    makeDir(fps);

    return directoryExists(fps);
}

inline int createDirectoryFast(const XCHAR* fps)
{
    WCHAR* ss = (WCHAR*)&buffer[XFER_SIZE >> 1];

    if(directoryExists(fps))
        return 1;

    utility_strcpy((char*)ss,"Creating directory: ");
    utility_w2cstrcpy((char*)(ss+10),fps);
    setStatusMessage((char*)ss);

    f_mkdir(fps);

    clearStatusMessage();
    return 1;
}

inline int deleteFile(const XCHAR* fss)
{
    if(!fileExists(fss))
        return FR_OK;

    return (f_unlink(fss) == FR_OK);
}

inline int deleteDirectory(const XCHAR* fps)
{
    if(!directoryExists(fps))
        return FR_OK;

    return (f_unlink(fps) == FR_OK);
}

inline UINT getFileSize(const XCHAR* fss)
{
    UINT r = 0;

    f_close(&gSDFile);

    if(f_open(&gSDFile, fss, FA_OPEN_EXISTING | FA_READ) != FR_OK)
        return 0;

    r = gSDFile.fsize;
    f_close(&gSDFile);

    return r;
}

inline void shortenName(char *dst, char *src, int max)
{
    if (utility_strlen(src) <= max)
    {
        // string fits, just copy it
        utility_strcpy(dst, src);
        return;
    }

    // string must be shortened
    short ix, iy;
    short right = max/2;
    short len = utility_strlen(src);

    // check right side of string for important name cues
    for (ix=iy=len-1; ix>(len-right); ix--)
        if ((src[ix] == '.') || (src[ix] == '[') || (src[ix] == '(') || (src[ix] == '-') || ((src[ix] >= '0')&&(src[ix] <= '9')))
            iy = ix;
    if (right > (len - iy))
        right = len - iy; // split at last cue

    utility_memcpy(dst, src, max);
    dst[max - right - 1] = '~';
    utility_memcpy(&dst[max - right], &src[iy], right);
    dst[max] = '\0';
}

// needed for GCC 4.x libc
int atexit(void (*function)(void))
{
    return -1;
}

/* cheat handling functions */
void cheat_invalidate()
{
    int a,b;

    registeredCheatEntries = 0;

    for(a = 0; a < CHEAT_ENTRIES_COUNT; a++)
    {
        cheatEntries[a].name[0] = cheatEntries[a].name[32] = '\0';
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
        cheatEntries[registeredCheatEntries].name[32] = '\0';
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
inline void neo_copy_sd(unsigned char *dest, int fstart, int len)
{
    UINT ts;
    ints_on();     /* enable interrupts */
    f_read(&gSDFile, dest, len, &ts);
    ints_off();     /* disable interrupts */
}

void delay(int count)
{
    int ticks;

    ints_on();

    ticks = gTicks + count;

    while (gTicks < ticks) ;
}

void sort_entries()
{
    int ix;

    // Sort entries! Not very fast, but small and easy.
    for (ix=0; ix<gMaxEntry-1; ix++)
    {
        selEntry_t temp;

        if (((gSelections[ix].type != 128) && (gSelections[ix+1].type == 128)) || // directories first
            ((gSelections[ix].type == 128) && (gSelections[ix+1].type == 128) &&  utility_wstrcmp(gSelections[ix].name, gSelections[ix+1].name) > 0) ||
            ((gSelections[ix].type != 128) && (gSelections[ix+1].type != 128) &&  utility_wstrcmp(gSelections[ix].name, gSelections[ix+1].name) > 0))
        {
            utility_memcpy((void*)&temp, (void*)&gSelections[ix], sizeof(selEntry_t));
            utility_memcpy((void*)&gSelections[ix], (void*)&gSelections[ix+1], sizeof(selEntry_t));
            utility_memcpy((void*)&gSelections[ix+1], (void*)&temp, sizeof(selEntry_t));
            ix = !ix ? -1 : ix-2;
        }
    }
}

void get_menu_flash(void)
{
    menuEntry_t *p = NULL;

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

        // check for auto-boot extended menu
        if ((gSelections[gMaxEntry].run == 7) && !utility_memcmp(p->meName, "MDEBIOS", 7))
        {
            gCurEntry = gMaxEntry;
            run_rom(0x0000); // never returns
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
/*
WCHAR *get_file_ext(WCHAR *src)
{
    int ix = wstrlen(src) - 1;
    while (ix && (src[ix] != (WCHAR)'.')) ix--;
    if (!ix)
        ix = wstrlen(src); // no extension, use whole string
    return &src[ix];
}*/

void get_sd_ips(int entry)
{
    char* pa;
    UINT bytesWritten = 0;

    gImportIPS = 0;

    utility_c2wstrcpy(ipsPath, "/");
    utility_c2wstrcat(ipsPath, IPS_DIR);
    utility_c2wstrcat(ipsPath, "/");
    utility_wstrcat(ipsPath, gSelections[entry].name);
    *utility_getFileExtW(ipsPath) = 0;
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
    WCHAR* cheatPath = (WCHAR*)&buffer[XFER_SIZE + 18];
    CheatEntry* e = NULL;
    char* pb = (char*)&buffer[0];
    char* head,*sp;
    UINT bytesWritten;
    UINT bytesToRead;
    short lk,ll;
    char region = systemRegion();

    ints_on();
    cheat_invalidate();

    utility_c2wstrcpy(cheatPath,"/");
    utility_c2wstrcat(cheatPath, CHEATS_DIR);
    utility_c2wstrcat(cheatPath, "/");
    utility_wstrcat(cheatPath, sss);
    *utility_getFileExtW(cheatPath) = 0; // cut off the extension
    utility_c2wstrcat(cheatPath, ".cht");

    f_close(&gSDFile);
    if(f_open(&gSDFile, cheatPath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        bytesWritten = 0;
        pb[0] = '\0';

        bytesToRead = gSDFile.fsize;

        if(!bytesToRead)
            return;

        if(bytesToRead > WORK_RAM_SIZE) //32K are more than enough
            bytesToRead = WORK_RAM_SIZE;

        ints_on();
        if(f_read(&gSDFile, pb, bytesToRead, &bytesWritten) == FR_OK)
        {
            if(!bytesWritten)
                return;

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
                        return;

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
}

void get_sd_info(int entry)
{
    unsigned char jump[4];
    int eos = utility_wstrlen(path);
    UINT ts;

    gTime1 = gTime2 = 0;
    gFileType = 0;
    gMythHdr = 0;

    if(getClockType())
        gRomDly_default_sd = SD_READ_TIME_PAL_MIN;
    else
        gRomDly_default_sd = SD_READ_TIME_NTSC_MIN;

    //get_sd_cheat(gSelections[entry].name);
    //get_sd_ips(entry);

    //cache_invalidate_pointers();

    gTime1 = gTicks;
    if (path[eos-1] != (WCHAR)'/')
        utility_c2wstrcat(path, "/");

    utility_wstrcat(path, gSelections[entry].name);
    f_close(&gSDFile);
    if (f_open(&gSDFile, path, FA_OPEN_EXISTING | FA_READ))
    {
        // couldn't open file
        char *temp = (char *)buffer;
        char *temp2 = (char *)&buffer[1024];
        utility_w2cstrcpy(temp2, path);
        temp2[32] = '\0';
        sprintf(temp, "!open %s",temp2);
        setStatusMessage(temp);
        path[eos] = 0;
        gRomDly_default_sd = 1;
        return;
    }
    path[eos] = 0;

    f_lseek(&gSDFile, 0x7FF0);
    f_read(&gSDFile, rom_hdr, 16, &ts);
    if (!utility_memcmp(rom_hdr, "TMR SEGA", 8))
    {
        // SMS ROM header
        gSelections[entry].type = 2; // SMS
        gSelections[entry].run = 0x13; // run mode = SMS + FM
        gRomDly_default_sd = 1;
        return;
    }

    if (gSelections[entry].length <= 0x200200)
        gSelections[entry].run = 1; // 16 Mbit + SRAM
    else if (gSelections[entry].length <= 0x400200)
        gSelections[entry].run = 2; // 32 Mbit + SRAM
    else
        gSelections[entry].run = 4; // 40 Mbit

    f_lseek(&gSDFile, 0);
    f_read(&gSDFile, buffer, 0x2400, &ts);
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
            f_lseek(&gSDFile, gd3 + 20); // start of VGM tag data
            f_read(&gSDFile, buffer, gSelections[entry].length - (gd3 + 20), &ts);
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
        if ((rom_hdr[0] == 0xFF) && (rom_hdr[1] == 0x04))
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

    gTime2 = gTicks;

    if( !(gTime2-gTime1) )
    {
        gRomDly_default_sd = (getClockType()) ? SD_READ_TIME_PAL_MAX : SD_READ_TIME_NTSC_MAX;
        return;
    }

    gRomDly_default_sd = (int)( ((gTime2-gTime1) * gSelections[entry].length) / 0x20000);

    if(getClockType())
    {
        if(gRomDly_default_sd > SD_READ_TIME_PAL_MAX)
            gRomDly_default_sd = SD_READ_TIME_PAL_MAX;

        if(gRomDly_default_sd < SD_READ_TIME_PAL_MIN)
            gRomDly_default_sd = SD_READ_TIME_PAL_MIN;
    }
    else
    {
        if(gRomDly_default_sd > SD_READ_TIME_NTSC_MAX)
            gRomDly_default_sd = SD_READ_TIME_NTSC_MAX;

        if(gRomDly_default_sd < SD_READ_TIME_NTSC_MIN)
            gRomDly_default_sd = SD_READ_TIME_NTSC_MIN;
    }
}

void get_sd_directory(int entry)
{
    DIR dir;
    FILINFO fno;
    int ix;

    gSdDetected = 0;
    gRomDly_default_sd = 0;

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

void update_display(void)
{
    int ix;
    char temp[48];

    if (gUpdate < 0)
        clear_screen();

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
            for (ix=0; ix<PAGE_ENTRIES; ix++, gCursorY++)
            {
                utility_w2cstrcpy((char*)buffer, gSelections[gStartEntry + ix].name);
                // erase line
                gCursorX = 1;
                put_str(gFEmptyLine, 0x2000);
                if (gStartEntry + ix >= gMaxEntry)
                    continue;   // past end, skip line
                // put centered name
                if (gSelections[gStartEntry + ix].type == 128)
                {
                    utility_strcpy(temp, "[");
                    strncat(temp, (const char *)buffer, 34);
                    temp[35] = '\0';
                    utility_strcat(temp, "]"); // show directories in brackets
                }
                else
                {
                    //strncpy(temp, (const char *)buffer, 36);
                    //temp[36] = '\0';
                    shortenName(temp, (char *)buffer, 36);
                }
                gCursorX = 20 - utility_strlen(temp)/2; // center the name
                put_str(temp, ((gStartEntry + ix) == gCurEntry) ? 0x2000 : 0); // hilite name if currently selected entry
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

    gCursorX = 1;
    gCursorY = 18;
    put_str(gFEmptyLine, 0x2000);

    gCursorX = 1;
    gCursorY = 19;
    put_str(gFBottomLine, 0x2000);

//  sprintf(temp, " %02d:%01d%01d:%01d%01d ", rtc[4]&31, rtc[3]&7, rtc[2]&15, rtc[1]&7, rtc[0]&15);
//  gCursorX = 20 - utility_strlen(temp)/2;     /* center time */
//  put_str(temp, 0);

    sprintf(temp, " CPLD V%d / Flash Type %c ", gCpldVers, 0x41 + (gCardType & 0xFF));
    gCursorX = 20 - strlen(temp)/2;
    put_str(temp, 0);

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
        // no Neo2/3 card found
        gCursorX = 5;
        put_str("Neo2/3 Flash Card not found!", 0x4000);
    }
    else
    {
        // menu entry info
        // get info for current entry
        if (gCurMode == MODE_SD)
        {
            if (gMaxEntry && !gRomDly && (gSelections[gCurEntry].type != 128))
                get_sd_info(gCurEntry); /* also loads rom_hdr */
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
                sprintf(temp, "Size=%d", gSelections[gCurEntry].length);
                gCursorX = 39 - utility_strlen(temp);   // right justify string
                put_str(temp, 0);
            }
            else
            {
                gCursorX = 1;
                gCursorY = 20;
                sprintf(temp, "Type=%s", (gSelections[gCurEntry].type == 0) ? "MD" : (gSelections[gCurEntry].type == 1) ? "32X" : "SMS");
                put_str(temp, 0);
                gCursorX = 12;
                sprintf(temp, "Offset=0x%08X", gSelections[gCurEntry].offset);
                put_str(temp, 0);
                sprintf(temp, "Size=%dMb", gSelections[gCurEntry].length/131072);
                gCursorX = 39 - utility_strlen(temp);   // right justify string
                put_str(temp, 0);

                gCursorX = 1;
                gCursorY = 21;
                sprintf(temp, "Run=0x%02X", gSelections[gCurEntry].run);
                put_str(temp, 0);
                gCursorX = 12;
                if (gSelections[gCurEntry].run == 5)
                    utility_strcpy(temp, "SRAM=EEPROM");
                else
                    sprintf(temp, "SRAM Bank=%d", gSelections[gCurEntry].bbank);
                put_str(temp, 0);
                sprintf(temp, "Size=%dKb", (gSelections[gCurEntry].run == 5) ? 1: gSelections[gCurEntry].bsize*64);
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
                gCursorX = 20 - utility_strlen(temp)/2; // center name
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
                gCursorX = 20 - utility_strlen(temp)/2; // center name
                put_str(temp, 0);

                // print rom size and sram size
                gCursorX = 1;
                gCursorY = 23;
                start = rom_hdr[0xA0]<<24|rom_hdr[0xA1]<<16|rom_hdr[0xA2]<<8|rom_hdr[0xA3];
                end = rom_hdr[0xA4]<<24|rom_hdr[0xA5]<<16|rom_hdr[0xA6]<<8|rom_hdr[0xA7];
                sprintf(temp, "ROM Size=%dMbit",(end - start + 1)/131072);
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
                sprintf(temp, "SRAM Size=%dKbit",(end - start + 2)/128);
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

void update_progress(char *str1, char *str2, int curr, int total)
{
    int ix;

    gCursorX = 1;
    gCursorY = 20;
    // erase line
    put_str(gEmptyLine, 0);

    gCursorX = 1;
    gCursorY = 21;
    // erase line
    put_str(gEmptyLine, 0);
    gCursorX = 20 - (utility_strlen(str1) + utility_strlen(str2)) / 2;
    put_str(str1, 0x2000);              /* print first string in green */
    gCursorX += utility_strlen(str1);
    put_str(str2, 0);                   /* print first string in white */

    gCursorX = 1;
    gCursorY = 22;
    // draw progress bar
    put_str("   ", 0);
    gCursorX = 4;
    for (ix=0; ix<=(32*curr/total); ix++, gCursorX++)
        put_str("\x87", 0x2000);        /* part completed */
    while (gCursorX < 36)
    {
        put_str("\x87", 0x4000);        /* part left to go */
        gCursorX++;
    }
    put_str("   ", 0);

    gCursorX = 1;
    gCursorY = 23;
    // erase line
    put_str(gEmptyLine, 0);
}

void copyGame(void (*dst)(unsigned char *buff, int offs, int len), void (*src)(unsigned char *buff, int offs, int len), int doffset, int soffset, int length, char *str1, char *str2)
{
    int ix, iy;

    for (iy=0; iy<length; iy+=XFER_SIZE)
    {
        // fetch data data from source
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
        // store data to destination
        (dst)(&buffer[gFileType ? XFER_SIZE : 0], doffset + iy, XFER_SIZE);
        update_progress(str1, str2, iy, length);
    }
    if ((length == 0x200000) && (dst == &neo_copyto_myth_psram))
    {
	// clear past end of rom for S&K
	utility_memset(buffer, 0x00, XFER_SIZE);
	for (iy=length; iy<(length + 0x020000); iy+=XFER_SIZE)
	    (dst)(buffer, doffset + iy, XFER_SIZE);
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

    sprintf(gSRAMSizeStr, "%dKb", gSRAMSize*64);
    // remask bank setting
    gSRAMBank = gSRAMBank & (63 >> utility_logtwo(gSRAMSize));

    if (gSRAMBank > 31)
        gSRAMBank = 0; // max of 32 banks since minimum bank size is 8KB sram

    sprintf(gSRAMBankStr, "%d", gSRAMBank);
}

void incSaveRAMBank(int index)
{
    gSRAMBank = (gSRAMBank + 1) & (63 >> utility_logtwo(gSRAMSize));

    if (gSRAMBank > 31)
        gSRAMBank = 0; // max of 32 banks since minimum bank size is 8KB sram

    sprintf(gSRAMBankStr, "%d", gSRAMBank);
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

        if(e->active)
        {
            //apply all master codes if exist
            for(b = 0; b < registeredCheatEntries; b++)
            {
                e = &cheatEntries[b];

                if(e->type & CT_MASTER)
                    e->active = 1;
            }

            //one time is enough to enable all master codes -- exit
            break;
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
                neo_copyto_myth_psram(buffer,cp->addr,2);
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

    ints_on();
    //skip "PATCH"
    neo_copy_sd(in,0,5);
    fbr = 5;

    UTIL_SetMemorySafe((char *)in,'0',6); in[5] = '\0';

    while(fbr < fsize)
    {
        unsigned int nb, ix = 0;

        neo_copy_sd(in,0,3);
        fbr += 3;
        addr = (unsigned int)in[2];
        addr += (unsigned int)in[1] << 8;
        addr += (unsigned int)in[0] << 16;

        neo_copy_sd(in,0,2);
        fbr += 2;
        len = (unsigned int)in[1];
        len += (unsigned int)in[0] << 8;
        if (len)
        {
            nb = len;
            if (addr & 1)
            {
                addr--;
                nb++;
                ix = 1;

                ints_off();
                neo_copyfrom_myth_psram(buffer, addr, 2);
            }
            if (nb & 1)
            {
                ints_off();
                neo_copyfrom_myth_psram(&buffer[nb-1], addr+nb-1, 2);
                nb++;
            }

            ints_on();
            neo_copy_sd(&buffer[ix], 0, len);
            fbr += len;
        }
        else
        {
            neo_copy_sd(in,0,2);
            fbr += 2;
            len = (unsigned int)in[1];
            len += (unsigned int)in[0] << 8;
            nb = len;
            if (addr & 1)
            {
                addr--;
                nb++;
                ix = 1;

                ints_off();
                neo_copyfrom_myth_psram(buffer, addr, 2);
            }
            if (nb & 1)
            {
                ints_off();
                neo_copyfrom_myth_psram(&buffer[nb-1], addr+nb-1, 2);
                nb++;
            }

            ints_on();
            neo_copy_sd(in,0,1);
            fbr += 1;
            utility_memset(&buffer[ix], in[0], len);
        }

        if((addr+len-1) < gSelectionSize)
        {
            if ((addr & 0x00F00000) == ((addr+len-1) & 0x00F00000))
            {
                ints_off();
                // doesn't cross 1MB boundary, copy all at once
                neo_copyto_myth_psram(buffer, addr, nb);
            }
            else
            {
                unsigned int len1 = ((addr & 0x00F00000) + 0x00100000) - addr;

                ints_off();
                // crosses 1MB boundary, copy up to 1MB boundary
                neo_copyto_myth_psram(buffer, addr, len1);

                ints_off();
                // copy the rest after the boundary
                neo_copyto_myth_psram(&buffer[len1], addr+len1, nb-len1);
            }
        }
    }

    ints_on();
    setStatusMessage("Importing patch...OK");

    clearStatusMessage();
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
    gSRAMBank = gSRAMSize = gResetMode = 0x0000;
    gYM2413 = 0x0001;
    gManageSaves = gSRAMgrServiceStatus = 0;

    sprintf(gSRAMBankStr, "%d", gSRAMBank);
    sprintf(gSRAMSizeStr, "%dKb", gSRAMSize*64);
}

int cache_process()
{
    unsigned int a = 0 , b = 0;
    short int i;

    clearStatusMessage();

    if(gSelections[gCurEntry].type == 128) //dir
        return 0;
    else if(gSelections[gCurEntry].type == 127) //Uknown
        return 0;

    if(gSelections[gCurEntry].run == 0x27)
        return 0;

    if((rom_hdr[0] == 0xFF) && (rom_hdr[1] != 0x04))
    {
        setStatusMessage("Fetching header...");
        get_sd_info(gCurEntry);
        clearStatusMessage();

        if((rom_hdr[0] == 0xFF) && (rom_hdr[1] != 0x04)) //bad!
            return 0;

        if(gSelections[gCurEntry].run == 0x27)
            return 0;
    }

    setStatusMessage("One-time detection in progress...");

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
    }
    else
    {
        if((b-a) > 2)
        {
            if(b-a +2 <= 8192)
                gSRAMSize = 1;
            else
                gSRAMSize = (short int)(( (b-a+2) / 1024) / 8);
        }

        if((rom_hdr[0] == 0xFF) && (rom_hdr[1] == 0x04))
            gSRAMSize = 16; // BRAM file

        gSRAMType = 0x0000;

        if(!gSRAMSize)//check for eeprom
        {
            //intense scan
            for(i = 0; i < EEPROM_MAPPERS_COUNT; i++)
            {
                if(!utility_memcmp(rom_hdr + 0x83,EEPROM_MAPPERS[i],utility_strlen(EEPROM_MAPPERS[i])))
                {
                    gSRAMType = 0x0001;
                    break;
                }
            }
        }
    }

    sprintf(gSRAMBankStr, "%d", gSRAMBank);
    sprintf(gSRAMSizeStr, "%dKb", gSRAMSize*64);

    setStatusMessage("One-time detection in progress...OK");

    clearStatusMessage();
    return 0xFF;
}

void cache_loadPA(WCHAR* sss)
{
    UINT fbr = 0;
    WCHAR* fnbuf = (WCHAR*)&buffer[XFER_SIZE + 512];

    if(gCurMode != MODE_SD)
        return;

    if(*utility_getFileExtW(sss) != '.')
        return;

    if(gSelections[gCurEntry].run == 0x27)
        return;

    setStatusMessage("Reading cache...");
    utility_memset(fnbuf,0,512);

    ints_on();
    utility_c2wstrcpy(fnbuf,"/");
    utility_c2wstrcat(fnbuf,CACHE_DIR);

    utility_c2wstrcat(fnbuf,"/");
    utility_wstrcat(fnbuf,sss);

    *utility_getFileExtW(fnbuf) = 0;
    utility_c2wstrcat(fnbuf,".dxcs");

    f_close(&gSDFile);
    if(f_open(&gSDFile,fnbuf,FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        clearStatusMessage();
        cache_invalidate_pointers();
        gCacheBlock.processed = cache_process();
        cache_sync();
        return;
    }

    if(gSDFile.fsize != sizeof(CacheBlock))
    {
        clearStatusMessage();
        f_close(&gSDFile);
        return;
    }
    ints_on();
    //changed it to block r/w
    f_read(&gSDFile,(char*)&gCacheBlock,sizeof(CacheBlock),&fbr);

    gSRAMBank = (short int)gCacheBlock.sramBank;
    gSRAMSize = gCacheBlock.sramSize;
    gYM2413 = gCacheBlock.YM2413;
    gResetMode =  gCacheBlock.resetMode;
    gSRAMType = (short int)gCacheBlock.sramType;
    gManageSaves = (short int)gCacheBlock.autoManageSaves;
    gSRAMgrServiceStatus = (short int)gCacheBlock.autoManageSavesServiceStatus;

    f_close(&gSDFile);

    sprintf(gSRAMBankStr, "%d", gSRAMBank);
    sprintf(gSRAMSizeStr, "%dKb", gSRAMSize*64);

    if(gCacheBlock.processed != 0xFF)
    {
        clearStatusMessage();
        gCacheBlock.processed = cache_process();
        cache_sync();
    }

    clearStatusMessage();
    ints_on();
}

void cache_load()
{
    if(gSelections[gCurEntry].type == 128) //dir
        return;
    else if(gSelections[gCurEntry].type == 127) //Uknown
        return;

    if(gSelections[gCurEntry].run == 0x27)
        return;

    cache_loadPA(gSelections[gCurEntry].name);
}

void cache_sync()
{
    UINT fbr = 0;
    WCHAR* fnbuf = (WCHAR*)&buffer[XFER_SIZE + 512];

    if(gCurMode != MODE_SD)
        return;

    if(*utility_getFileExtW(gSelections[gCurEntry].name) != '.')
        return;

    if(gSelections[gCurEntry].run == 0x27)
        return;

    ints_on();
    utility_memset(fnbuf,0,512);
    utility_c2wstrcpy(fnbuf,"/");
    utility_c2wstrcat(fnbuf,CACHE_DIR);

    if(!createDirectory(fnbuf))
        return;

    setStatusMessage("Writing cache...");

    utility_c2wstrcat(fnbuf,"/");
    utility_wstrcat(fnbuf,gSelections[gCurEntry].name);

    *utility_getFileExtW(fnbuf) = 0;
    utility_c2wstrcat(fnbuf,".dxcs");

    f_close(&gSDFile);
    deleteFile(fnbuf);

    if(f_open(&gSDFile,fnbuf,FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        clearStatusMessage();
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
    gCacheBlock.sramType = (unsigned char)gSRAMType;

    f_write(&gSDFile,(char*)&gCacheBlock,sizeof(CacheBlock),&fbr);

    f_close(&gSDFile);

    sprintf(gSRAMBankStr, "%d", gSRAMBank);
    sprintf(gSRAMSizeStr, "%dKb", gSRAMSize*64);

    ints_on();

    clearStatusMessage();

    if(gCacheBlock.processed != 0xFF)
    {
        gCacheBlock.processed = cache_process();
        cache_sync();
    }

    clearStatusMessage();
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

        //config_push("manageSaves","0");//will be replaced if exists
    }
    else
    {
        gOptions[index].value = "Enabled";
        gManageSaves = 1;

        //config_push("manageSaves","1");//will be replaced if exists
    }

    //updateConfig();

    ints_on();

    cache_sync();

    ints_on();
    setStatusMessage("Updating configuration...OK");
    clearStatusMessage();
}

void sram_mgr_saveGamePA(WCHAR* sss)
{
    UINT fbr = 0;
    WCHAR* fnbuf = (WCHAR*)&buffer[XFER_SIZE + 512];
    int sramLength,sramBankOffs,k,i,tw;

    //dont let this happen
    if(!gSRAMSize)
        return;

    ints_on();
    utility_memset(fnbuf,0,512);

    sramLength = gSRAMSize * 4096;//actual myth space occupied, not counting even bytes
    sramBankOffs = gSRAMBank * max(sramLength,8192);//minimum bank size is 8KB, not 4KB

    utility_c2wstrcpy(fnbuf,"/");
    utility_c2wstrcat(fnbuf,SAVES_DIR);

    if(!createDirectory(fnbuf))
        return;

    utility_c2wstrcat(fnbuf,"/");
    utility_wstrcat(fnbuf,sss);

    *utility_getFileExtW(fnbuf) = 0;
    if(/*gSelections[gCurEntry].type==2||*/gSRAMSize==16)//sms will not work due to romtype not being hashed
    {
        //sms or bram
        /*if(gSelections[gCurEntry].type==2)
        {
            utility_c2wstrcat(fnbuf,SMS_SAVE_EXT);
        }
        else*/
        {
            utility_c2wstrcat(fnbuf,BRM_SAVE_EXT);//or .crm
        }
    }
    else
    {
        utility_c2wstrcat(fnbuf,MD_32X_SAVE_EXT);
    }

    //if exists - delete
    deleteFile(fnbuf);

    f_close(&gSDFile);

    if(f_open(&gSDFile,fnbuf,FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
        return;

    setStatusMessage("Backing up GAME sram...");

    if(/*gSelections[gCurEntry].type==2||*/gSRAMSize==16)//sms will not work due to romtype not being hashed
    {
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
}

void sram_mgr_saveGame(int index)
{
    sram_mgr_saveGamePA(gSelections[gCurEntry].name);
}

void sram_mgr_restoreGame(int index)
{
    UINT fbr = 0;
    WCHAR* fnbuf = (WCHAR*)&buffer[XFER_SIZE + 512];
    int sramLength,sramBankOffs,k,i,tr;

    ints_on();
    utility_memset(fnbuf,0,512);
    utility_c2wstrcpy(fnbuf,"/");
    utility_c2wstrcat(fnbuf,SAVES_DIR);

    utility_c2wstrcat(fnbuf,"/");
    utility_wstrcat(fnbuf,gSelections[gCurEntry].name);

    sramLength = gSRAMSize * 4096;//actual myth space occupied, not counting even bytes
    sramBankOffs = gSRAMBank * max(sramLength,8192);//minimum bank size is 8KB, not 4KB

    *utility_getFileExtW(fnbuf) = 0;
    if(gSelections[gCurEntry].type==2||gSRAMSize==16)
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
        return;

    if(!gSDFile.fsize)
    {
        f_close(&gSDFile);
        return;
    }

    setStatusMessage("Restoring GAME sram...");

    if(gSelections[gCurEntry].type==2||gSRAMSize==16)
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
}

void sram_mgr_saveAll(int index)
{
    WCHAR* fss = (WCHAR*)&buffer[XFER_SIZE + 512];
    UINT fsize = 0 , i = 0 , fbr = 0;

    ints_on();
    setStatusMessage("Working...");

    utility_memset(fss,0,512);

    utility_c2wstrcpy(fss,"/");
    utility_c2wstrcat(fss,SAVES_DIR);

    if(!createDirectory(fss))
    {
        clearStatusMessage();
        return;
    }

    utility_c2wstrcat(fss,"/SRAM");
    utility_c2wstrcat(fss,MD_32X_SAVE_EXT);

    deleteFile(fss);//delete if exists

    f_close(&gSDFile);

    if(f_open(&gSDFile, fss , FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
        return;

    fsize = gSDFile.fsize;

    while( i < ((XFER_SIZE * 2) * 4) * 2 )
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
}

void sram_mgr_restoreAll(int index)
{
    WCHAR* fss = (WCHAR*)&buffer[XFER_SIZE + 512];
    UINT fsize = 0 , i = 0 , fbr = 0;

    ints_on();
    setStatusMessage("Working...");

    utility_memset(fss,0,512);
    utility_c2wstrcpy(fss,"/");
    utility_c2wstrcat(fss,SAVES_DIR);

    utility_c2wstrcat(fss,"/SRAM");
    utility_c2wstrcat(fss,MD_32X_SAVE_EXT);

    f_close(&gSDFile);

    if(f_open(&gSDFile,fss, FA_OPEN_EXISTING | FA_READ) != FR_OK)
        return;

    fsize = gSDFile.fsize;

    if(fsize < ((XFER_SIZE * 2) * 4) * 2)
    {
        f_close(&gSDFile);
        return;
    }

    while( i < ((XFER_SIZE * 2) * 4) * 2)
    {
        ints_on();
        update_progress("Restoring ALL SRAM.."," ",i,((XFER_SIZE * 2) * 4) * 2);
        f_read(&gSDFile,(char*)buffer,(XFER_SIZE * 2), &fbr);
        ints_off();
        neo_copyto_sram(buffer,i,XFER_SIZE * 2);

        i += (XFER_SIZE * 2);
    }
    ints_on();
    f_close(&gSDFile);
    update_progress("Restoring ALL SRAM.."," ",100,100);
    clearStatusMessage();
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

    while( i < ((XFER_SIZE * 2) * 4) * 2)//write  32KB blocks
    {
        update_progress("Clearing ALL SRAM.."," ",i,((XFER_SIZE * 2) * 4) * 2);

        ints_off();
        neo_copyto_sram(buffer,i,XFER_SIZE * 2);
        ints_on();

        i += (XFER_SIZE * 2);
    }

    ints_on();
    update_progress("Clearing ALL SRAM.."," ",100,100);
    setStatusMessage("Clearing ALL SRAM...OK");
;
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
        //inlined code..no loops

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
    cache_sync();
    clearStatusMessage();
    do_sramMgr();
    clearStatusMessage();
}

void runCheatEditor(int index)
{
    char* line = (char*)&buffer[XFER_SIZE];
    char* buf = (char*)&buffer[XFER_SIZE + 256 ];
    char* pb = (char*)&buffer[0];
    WCHAR* cheatPath = (WCHAR*)&buffer[XFER_SIZE + 512 ];
    UINT fbr,read;
    int r;
    int added = 0;
    int running = 1;
    int idx = 0;
    unsigned short int buttons = 0;

    if(gCurMode != MODE_SD)
        return;

    clear_screen();

    ints_on();
    printToScreen("Working...",(40 >> 1) - (utility_strlen("Working...") >>1),12,0x2000);


    cache_sync();

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
    *utility_getFileExtW(cheatPath) = 0; // cut off the extension
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
}

void do_options(void)
{
    int i;
    int ix;
    int maxOptions = 0;
    int currOption = 0;
    int update = 1;
    char ipsFPath[40];
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
    else if ((gSelections[gCurEntry].run == 9) || (gSelections[gCurEntry].run == 10))
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

        gOptions[maxOptions].exclusiveFCall = 0;
        sprintf(gSRAMBankStr, "%d", gSRAMBank);
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

        sprintf(gSRAMSizeStr, "%dKb", gSRAMSize*64);
        gOptions[maxOptions].name = "Save RAM Size";
        gOptions[maxOptions].value = gSRAMSizeStr;
        gOptions[maxOptions].callback = &incSaveRAMSize;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;

        if(!gSRAMBank)
            gSRAMBank = gSelections[gCurEntry].bbank;

        gOptions[maxOptions].exclusiveFCall = 0;
        sprintf(gSRAMBankStr, "%d", gSRAMBank);
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

    if(gCurMode == MODE_SD)
    {
        // insert options for game rom into gOptions array
        gOptions[maxOptions].exclusiveFCall = 1; //Exclusive function! jump back to source caller
        gOptions[maxOptions].name = "Add cheats manually";
        gOptions[maxOptions].value = NULL;
        gOptions[maxOptions].callback = &runCheatEditor;
        gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
        maxOptions++;
    }

    /*
    //fake entry = separator
    gOptions[maxOptions].exclusiveFCall = 0;
    gOptions[maxOptions].name = "\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82\x82";
    gOptions[maxOptions].value = NULL;
    gOptions[maxOptions].callback = NULL;
    gOptions[maxOptions].patch = gOptions[maxOptions].userData = NULL;
    maxOptions++;
    */

    //For the cheat support
    for(i = 0; i < registeredCheatEntries; i++)
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

    if(ipsPath[0] != 0)
    {
        utility_w2cstrcpy((char*)buffer, ipsPath);
        strncpy(ipsFPath, (const char *)buffer, 36);
        ipsFPath[36] = '\0';
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
        // check if buttons changed
        if ((buttons & SEGA_CTRL_BUTTONS) != gButtons)
        {
            unsigned short int changed = (buttons & SEGA_CTRL_BUTTONS) ^ gButtons;
            gButtons = buttons & SEGA_CTRL_BUTTONS;

            if ((changed & SEGA_CTRL_UP) && (buttons & SEGA_CTRL_UP))
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
                continue;
            }

            if ((changed & SEGA_CTRL_LEFT) && (buttons & SEGA_CTRL_LEFT))
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
                continue;
            }

            if ((changed & SEGA_CTRL_DOWN) && (buttons & SEGA_CTRL_DOWN))
            {
                // DOWN pressed, go one entry forward
                currOption++;
                if (currOption >= maxOptions)
                    currOption = start = 0; // wrap around to top
                if ((currOption - start) >= PAGE_ENTRIES)
                    start += PAGE_ENTRIES;
                update = 1;
                continue;
            }

            if ((changed & SEGA_CTRL_RIGHT) && (buttons & SEGA_CTRL_RIGHT))
            {
                // RIGHT pressed, go one page forward
                currOption += PAGE_ENTRIES;
                if (currOption >= maxOptions)
                    currOption = start = 0;    // wrap around to top
                if ((currOption - start) >= PAGE_ENTRIES)
                    start += PAGE_ENTRIES; // next "page" of entries
                update = 1;
                continue;
            }

            if ((changed & SEGA_CTRL_A) && !(buttons & SEGA_CTRL_A))
            {
                if(gCurMode == MODE_SD)
                {
                    cache_sync();
                    cache_invalidate_pointers();
                }

                gSelectionSize = 0;
                ipsPath[0] = 0;
                cheat_invalidate();
                break;  // A released -> cancel, break out of while loop
            }

            if ((changed & SEGA_CTRL_B) && !(buttons & SEGA_CTRL_B))
            {
                // B released
                char temp[32];

                utility_w2cstrcpy((char*)buffer, gSelections[gCurEntry].name);
                //strncpy(temp, (const char *)buffer, 30);
                //temp[30] = '\0';
                shortenName(temp, (char *)buffer, 30);

                if (gCurMode == MODE_FLASH)
                {
                    int fstart, fsize, bbank, bsize, runmode;
                    fstart = gSelections[gCurEntry].offset;
                    fsize = gSelections[gCurEntry].length;
                    bbank = gSRAMBank;
                    bsize = gSRAMSize * 8192;
                    runmode = gSelections[gCurEntry].run;
                    if ((runmode & 0x1F) < 7)
                        runmode = gSRAMType ? 5 : !bsize ? 6 : (gSelections[gCurEntry].type == 1) ? 3 : (fsize > 0x200200) ? 2 : 1;
                    ints_off();     /* disable interrupts */
                    // Run selected rom
                    if ((gSelections[gCurEntry].type == 2) || (runmode == 7))
                    {
                        neo_run_game(fstart, fsize, bbank, bsize, runmode); // never returns
                    }
                    else
                    {
                        int pstart = (runmode == 0x27) ? 0x600000 : 0;
                        //int ix;
                        // copy flash to myth psram
                        copyGame(&neo_copyto_myth_psram, &neo_copy_game, pstart, fstart, fsize, "Loading ", temp);

                        // check for raw S&K
                        if (!utility_memcmp((void*)0x200180, "GM MK-1563 -00", 14) && (fsize == 0x200000))
                            fsize = 0x300000;

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
                    if ((runmode & 0x1F) < 7)
                        runmode = gSRAMType ? 5 : !bsize ? 6 : (gSelections[gCurEntry].type == 1) ? 3 : (fsize > 0x200200) ? 2 : 1;
                    // Run selected rom
                    if (path[utility_wstrlen(path)-1] != (WCHAR)'/')
                        utility_c2wstrcat(path, "/");
                    utility_wstrcat(path, gSelections[gCurEntry].name);

                    f_close(&gSDFile);
                    if (f_open(&gSDFile, path, FA_OPEN_EXISTING | FA_READ))
                    {
                        // couldn't open file
                        path[eos] = 0;
                        continue;
                    }
                    path[eos] = 0;
                    if (gFileType)
                        f_read(&gSDFile, buffer, 0x200, &ts);

                    if ((gSelections[gCurEntry].type == 2) || (runmode == 7))
                    {
                        // copy file to flash cart psram
                        copyGame(&neo_copyto_psram, &neo_copy_sd, 0, 0, fsize, "Loading ", temp);

                        if(gManageSaves)
                        {
                            f_close(&gSDFile);
                            clear_screen();
                            gSRAMgrServiceStatus = SMGR_STATUS_BACKUP_SRAM;
                            cache_sync();

                            char* p = (char*)buffer;
                            utility_w2cstrcpy(p,gSelections[gCurEntry].name);
                            config_push("romName",p);
                            updateConfig();

                            sram_mgr_restoreGame(0);
                        }
                        else
                            cache_sync();

                        neo2_disable_sd();
                        ints_off();     /* disable interrupts */
                        neo_run_psram(0, fsize, bbank, bsize, runmode); // never returns
                    }
                    else
                    {
                        int pstart = (runmode == 0x27) ? 0x600000 : 0;
                        //int ix;
                        // copy file to myth psram
                        copyGame(&neo_copyto_myth_psram, &neo_copy_sd, pstart, 0, fsize, "Loading ", temp);

                        // check for raw S&K
                        if (!utility_memcmp((void*)0x200180, "GM MK-1563 -00", 14) && (fsize == 0x200000))
                            fsize = 0x300000;

                        gSelectionSize = fsize;
                        // do patch callbacks
                         for (ix=0; ix<maxOptions; ix++)
                         {
                                if (gOptions[ix].patch)
                                 (gOptions[ix].patch)(ix);
                        }

                        if(gManageSaves)
                        {
                            f_close(&gSDFile);
                            clear_screen();
                            gSRAMgrServiceStatus = SMGR_STATUS_BACKUP_SRAM;
                            cache_sync();

                            char* p = (char*)buffer;
                            utility_w2cstrcpy(p,gSelections[gCurEntry].name);
                            config_push("romName",p);
                            updateConfig();

                            sram_mgr_restoreGame(0);
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
                    //cache_sync();

                // C released -> do option callback
                if (gOptions[currOption].callback)
                    (gOptions[currOption].callback)(currOption);

                //exclusive function
                if(gOptions[currOption].exclusiveFCall)
                {
                    clear_screen();
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
    char temp[40];

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

    sprintf(temp, "Card has %d blocks (%d MB)", num_sectors, (num_sectors / 2048));
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
    char temp[32];

    utility_w2cstrcpy((char*)buffer, gSelections[gCurEntry].name);
    //strncpy(temp, (const char *)buffer, 30);
    //temp[30] = '\0';
    shortenName(temp, (char *)buffer, 30);

    gResetMode = reset_mode;

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
            char temp2[48];

            // Play VGM song
#ifndef RUN_IN_PSRAM
            if (fsize > 0x1C0000)
                return; // too big for current method of playing
#else
            if (fsize > 0x100000)
                return; // too big for current method of playing
#endif
            // copy file to myth psram
            ints_off();
            copyGame(&neo_copyto_myth_psram, &neo_copy_game, 0x600000, fstart, fsize, "Loading ", temp);
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

        cache_loadPA(gSelections[gCurEntry].name);//don't forget to load the cache

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
        f_close(&gSDFile);
        if (f_open(&gSDFile, path, FA_OPEN_EXISTING | FA_READ))
        {
            // couldn't open file
            setStatusMessage("Couldn't open game file");
            path[eos] = 0;
            return;
        }
        path[eos] = 0;
        if (gFileType)
            f_read(&gSDFile, buffer, 0x200, &ts); // skip header on SMD file

        // Look for special file types
        if (gSelections[gCurEntry].type == 4)
        {
            int ix;
            char temp2[48];

            // Play VGM song
#ifndef RUN_IN_PSRAM
            if (fsize > 0x1C0000)
                return; // too big for current method of playing
#else
            if (fsize > 0x100000)
                return; // too big for current method of playing
#endif
            // copy file to myth psram
            copyGame(&neo_copyto_myth_psram, &neo_copy_sd, 0x600000, 0, fsize, "Loading ", temp);
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
            gUpdate = -1;               /* clear screen for major screen update */
            return;
        }

        // Run selected rom
        if ((gSelections[gCurEntry].type == 2) || (runmode == 7))
        {
            // copy file to flash cart psram
            copyGame(&neo_copyto_psram, &neo_copy_sd, 0, 0, fsize, "Loading ", temp);
            neo2_disable_sd();
            ints_off();     /* disable interrupts */
            neo_run_psram(0, fsize, bbank, bsize, runmode); // never returns
        }
        else
        {
            int pstart = (gSelections[gCurEntry].run == 0x27) ? 0x600000 : 0;

            // copy file to myth psram
            copyGame(&neo_copyto_myth_psram, &neo_copy_sd, pstart, 0, fsize, "Loading ", temp);

            // check for raw S&K
            if (!utility_memcmp((void*)0x200180, "GM MK-1563 -00", 14) && (fsize == 0x200000))
                fsize = 0x300000;

            if(gManageSaves)
            {
                gSRAMgrServiceStatus = SMGR_STATUS_BACKUP_SRAM;
                cache_sync();

                char* p = (char*)buffer;
                utility_w2cstrcpy(p,gSelections[gCurEntry].name);
                config_push("romName",p);
                updateConfig();

                sram_mgr_restoreGame(0);
            }
            else
                cache_sync();

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
    WCHAR* fss = (WCHAR*)&buffer[XFER_SIZE + 512];

    ints_on();

    setStatusMessage("Writing config...");

    if(config_saveToBuffer((char*)buffer))
    {
        clearStatusMessage();
        return;
    }

    utility_c2wstrcpy(fss,dxcore_dir); createDirectory(fss);
    utility_c2wstrcpy(fss,dxconf_cfg);

    f_close(&gSDFile);
    deleteFile(fss);

    if(f_open(&gSDFile, fss, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        clearStatusMessage();
        return;
    }

    ints_on();
    f_write(&gSDFile,(char*)buffer,utility_strlen((char*)buffer), &fbr);
    f_close(&gSDFile);
    ints_off();

    clearStatusMessage();
    ints_on();
}

void loadConfig()
{
    //The above code covers all cases just to make sure that we're not going to run into issues
    UINT fbr = 0,bytesToRead = 0,newConfig = 0;
    WCHAR* fss = (WCHAR*)&buffer[XFER_SIZE + 512];

    if(!gSdDetected)
        return;

    setStatusMessage("Reading config...");

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
        setStatusMessage("Initializing configuration...");
        if(f_read(&gSDFile,(char*)buffer,bytesToRead, &fbr) == FR_OK)
        {
            config_loadFromBuffer((char*)buffer,(int)fbr);

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
        config_push("romName","*");

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
    WCHAR* buf = (WCHAR*)&buffer[XFER_SIZE + 24];
    memset(buf,0,256);

    //if config existed before, it's the responsibility of user to create directories
    //if it's new default config, we create directories here
    //if directories do not exist, they will be created by the code that saves to those directories
    utility_c2wstrcpy(buf,"/"); utility_c2wstrcat(buf,CHEATS_DIR); if(newConfig)createDirectoryFast(buf);
    utility_c2wstrcpy(buf,"/"); utility_c2wstrcat(buf,IPS_DIR); if(newConfig)createDirectoryFast(buf);
    utility_c2wstrcpy(buf,"/"); utility_c2wstrcat(buf,SAVES_DIR); if(newConfig)createDirectoryFast(buf);
    utility_c2wstrcpy(buf,"/"); utility_c2wstrcat(buf,CACHE_DIR); if(newConfig)createDirectoryFast(buf);

    clearStatusMessage();
    ints_on();
}

/*Inputbox*/
int inputBox(char* result,const char* caption,const char* defaultText,short int  boxX,short int  boxY,
            short int  captionColor,short int boxColor,short int textColor,short int hlTextColor,short int maxChars)
{
    char* buf = result;
    char* in = (char*)&buffer[(XFER_SIZE*2) - 8];/*single character replacement*/
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
//    char temp[44];                      /* keep in sync with RTC print below! */

#ifndef RUN_IN_PSRAM
    init_hardware();                    /* set hardware to a consistent state, clears vram and loads the font */
    gCardOkay = neo_check_card();       /* check for Neo Flash card */
#else
    gCardOkay = 0;                      /* have Neo Flash card - duh! */
#endif
    gCpldVers = neo_check_cpld();	/* get CPLD version */
    gCardType = *(short int *)0x00FFF0; /* get flash card type from menu flash */

    ints_on();                          /* allow interrupts */

    // set long file name pointers
    for (ix=0; ix<MAX_ENTRIES; ix++)
    {
        gSelections[ix].name = &lfnames[ix * 256];
        lfnames[ix * 256] = (WCHAR)0;
    }

#ifndef RUN_IN_PSRAM
    {
        WCHAR* fss = (WCHAR*)&buffer[XFER_SIZE];

        gCurEntry = 0;
        gStartEntry = 0;
        gMaxEntry = 0;

        neo2_enable_sd();
        get_sd_directory(-1);           /* get root directory of sd card */
        if(gSdDetected)
        {
            utility_c2wstrcpy(fss, "/.menu/md/MDEBIOS.BIN");
            if(f_open(&gSDFile, fss, FA_OPEN_EXISTING | FA_READ) == FR_OK)
            {
                gCurEntry = 0;
                utility_c2wstrcpy(path, "/.menu/md");
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
    if(gSdDetected)
    {
        clear_screen();
        setStatusMessage("Loading cache & configuration...");
        loadConfig();
        char* p = config_getS("romName");
        if(p)
        {
            if(utility_strlen(p) > 2)
            {
                WCHAR* buf = (WCHAR*)&buffer[XFER_SIZE - 256];//remember loadPA() uses the block after XFER_SIZE...so move 256bytes back!

                utility_c2wstrcpy(buf,p);

                cache_loadPA(buf);

                if(p[0] == '*')
                    gSRAMgrServiceStatus = SMGR_STATUS_NULL;

                if(gSRAMgrServiceStatus == SMGR_STATUS_BACKUP_SRAM)
                {
                    sram_mgr_saveGamePA(buf);
                    setStatusMessage("Loading cache & configuration...");

                    config_push("romName","*");

                    gSRAMgrServiceStatus = SMGR_STATUS_NULL;
                    cache_sync();
                    updateConfig();
                }
            }
        }
        setStatusMessage("Loading cache & configuration...OK");
        clear_screen();
    }

    utility_memcpy(gCacheBlock.sig,"DXCS",4);
    gCacheBlock.sig[4] = '\0';
    gCacheBlock.processed = 0;
    gCacheBlock.version = 1;

    cache_invalidate_pointers();

    if(gSdDetected&&(gMaxEntry>0))
    {
        ints_on();
        clearStatusMessage();
    }
    else
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

    while(1)
    {
        unsigned short int buttons;
//      unsigned char now[8];

        if (gRomDly)
        {
            // decrement time to try loading rom header
            gRomDly--;
            if (gRomDly)
                rom_hdr[0] = rom_hdr[1] = 0xFF;
            else
                gUpdate = 1;            /* minor update - load rom header */
        }

        if (gUpdate)
            update_display();           /* if flag set, update the display */

        delay(1);

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
        // check if buttons changed
        if ((buttons & SEGA_CTRL_BUTTONS) != gButtons)
        {
            unsigned short int changed = (buttons & SEGA_CTRL_BUTTONS) ^ gButtons;
            gButtons = buttons & SEGA_CTRL_BUTTONS;

            if ((changed & SEGA_CTRL_UP) && (buttons & SEGA_CTRL_UP))
            {
                // UP pressed, go one entry back
                gCurEntry--;
                if (gCurEntry < 0)
                {
                    gCurEntry = gMaxEntry - 1;
                    gStartEntry = gMaxEntry - (gMaxEntry % PAGE_ENTRIES);
                }
                if (gCurEntry < gStartEntry)
                {
                    gStartEntry -= PAGE_ENTRIES; // previous "page" of entries
                    if (gStartEntry < 0)
                        gStartEntry = 0;
                }
                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                gUpdate = 1;            /* minor screen update */
                gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
                continue;
            }
            if ((changed & SEGA_CTRL_LEFT) && (buttons & SEGA_CTRL_LEFT))
            {
                // LEFT pressed, go one page back
                gCurEntry -= PAGE_ENTRIES;
                if (gCurEntry < 0)
                {
                    gCurEntry = gMaxEntry - 1;
                    gStartEntry = gMaxEntry - (gMaxEntry % PAGE_ENTRIES);
                }
                if (gCurEntry < gStartEntry)
                {
                    gStartEntry -= PAGE_ENTRIES; // previous "page" of entries
                    if (gStartEntry < 0)
                        gStartEntry = 0;
                }
                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                gUpdate = 1;            /* minor screen update */
                gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
                continue;
            }
            if ((changed & SEGA_CTRL_DOWN) && (buttons & SEGA_CTRL_DOWN))
            {
                // DOWN pressed, go one entry forward
                gCurEntry++;
                if (gCurEntry == gMaxEntry)
                    gCurEntry = gStartEntry = 0;    // wrap around to top
                if ((gCurEntry - gStartEntry) == PAGE_ENTRIES)
                    gStartEntry += PAGE_ENTRIES; // next "page" of entries
                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                gUpdate = 1;            /* minor screen update */
                gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
                continue;
            }
            if ((changed & SEGA_CTRL_RIGHT) && (buttons & SEGA_CTRL_RIGHT))
            {
                // RIGHT pressed, go one page forward
                gCurEntry += PAGE_ENTRIES;
                if (gCurEntry >= gMaxEntry)
                    gCurEntry = gStartEntry = 0;    // wrap around to top
                if ((gCurEntry - gStartEntry) >= PAGE_ENTRIES)
                    gStartEntry += PAGE_ENTRIES; // next "page" of entries
                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                gUpdate = 1;            /* minor screen update */
                gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
                continue;
            }

            if ((changed & SEGA_CTRL_START) && !(buttons & SEGA_CTRL_START))
            {
                // START released, change mode
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
                    gCursorY = 0;
                    neo2_enable_sd();
                    get_sd_directory(-1);   /* get root directory of sd card */
                    loadConfig();
                }

                //rom_hdr[0] = 0xFF;        /* rom header not loaded */
                gUpdate = -1;           /* clear screen for major screen update */
                gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
                continue;
            }

            if ((changed & SEGA_CTRL_A) && !(buttons & SEGA_CTRL_A))
            {
                // A released
                if (gCurMode == MODE_FLASH)
                {
                    // call options screen, which will either return or run the rom
                    if (gSelections[gCurEntry].type < 3)
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
                    gUpdate = 1;        /* minor screen update */
                    gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
                    continue;
                }
                else if (gCurMode == MODE_SD)
                {
                    // call options screen, which will either return or run the rom
                    if (gSelections[gCurEntry].type < 3)
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
                    gUpdate = 1;        /* minor screen update */
                    gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
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
                    gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
                    continue;
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
                    gUpdate = 1;        /* minor screen update */
                    gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
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
                    gRomDly = (gCurMode==MODE_FLASH)?gRomDly_default_flash:gRomDly_default_sd; /* delay before loading rom header */
                    continue;
                }

                run_rom(0x00FF);
            }
        }
    }

    return 0;
}

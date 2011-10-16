
#undef __DEBUG_PSRAM__

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>
#include <diskio.h>
#include <ff.h>
#include "neo_2_asm.h"
#include "configuration.h"
#include "interrupts.h"

typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile uint64_t vu64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef uint64_t u64;

/* hardware definitions */
// Pad buttons
#define A_BUTTON(a)     ((a) & 0x8000)
#define B_BUTTON(a)     ((a) & 0x4000)
#define Z_BUTTON(a)     ((a) & 0x2000)
#define START_BUTTON(a) ((a) & 0x1000)

// D-Pad
#define DU_BUTTON(a)    ((a) & 0x0800)
#define DD_BUTTON(a)    ((a) & 0x0400)
#define DL_BUTTON(a)    ((a) & 0x0200)
#define DR_BUTTON(a)    ((a) & 0x0100)

// Triggers
#define TL_BUTTON(a)    ((a) & 0x0020)
#define TR_BUTTON(a)    ((a) & 0x0010)

// Yellow C buttons
#define CU_BUTTON(a)    ((a) & 0x0008)
#define CD_BUTTON(a)    ((a) & 0x0004)
#define CL_BUTTON(a)    ((a) & 0x0002)
#define CR_BUTTON(a)    ((a) & 0x0001)

unsigned int gBootCic;
unsigned int gCpldVers;                 /* 0x81 = V1.2 hardware, 0x82 = V2.0 hardware, 0x83 = V3.0 hardware */
unsigned int gCardTypeCmd;
unsigned int gPsramCmd;
short int gCardType;                    /* 0x0000 = newer flash, 0x0101 = new flash, 0x0202 = old flash */
unsigned int gCardID;                   /* should be 0x34169624 for Neo2 flash carts */
short int gCardOkay;                    /* 0 = okay, -1 = err */
short int gCursorX;                     /* range is 0 to 63 (only 0 to 39 onscreen) */
short int gCursorY;                     /* range is 0 to 31 (only 0 to 27 onscreen) */


unsigned short gButtons = 0;
struct controller_data gKeys;

volatile unsigned int gTicks;           /* incremented every vblank */

sprite_t *pattern[3] = { NULL, NULL, NULL };

unsigned short boxes[16];
char boxart[14984*16];
char unknown[14984];

sprite_t *splash;
sprite_t *browser;
sprite_t *loading;

struct textColors {
    u32 title;
    u32 usel_game;
    u32 sel_game;
    u32 uswp_info;
    u32 swp_info;
    u32 usel_option;
    u32 sel_option;
    u32 hw_info;
    u32 help_info;
};
typedef struct textColors textColors_t;

textColors_t gTextColors;

u64 back_flags;                         /* 0xAA550mno - m = brwsr, n = bopt, o = bfill */

struct selEntry {
    u32 valid;                          /* 0 = invalid, ~0 = valid */
    u32 type;                           /* 64 = compressed , 128 = directory, 0xFF = N64 game */
    u32 swap;                           /* 0 = no swap, 1 = byte swap, 2 = word swap, 3 = long swap */
    u32 pad;
    u8 options[8];                      /* menu entry options */
    u8 hdr[0x20];                       /* copy of data from rom offset 0 */
    u8 rom[0x20];                       /* copy of data from rom offset 0x20 */
    char name[256];
};
typedef struct selEntry selEntry_t;

selEntry_t __attribute__((aligned(16))) gTable[1024];

extern unsigned short cardType;         /* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */
extern unsigned int num_sectors;        /* number of sectors on SD card (or 0) */
extern unsigned char *sd_csd;           /* card specific data */

FATFS gSDFatFs;                         /* global FatFs structure for FF */

short int gSdDetected = 0;              /* 0 - not detected, 1 - detected */

unsigned int fast_flag = 0;             /* 0 - use normal cart timing for SD read, 1 - use quick timing, 2 - use fast timing */

char path[1024];


#define ONCE_SIZE (256*1024)
static u8 __attribute__((aligned(16))) tmpBuf[ONCE_SIZE];

extern sprite_t *loadImageDFS(char *fname);
extern sprite_t *loadImageSD(char *fname);
extern void drawImage(display_context_t dcon, sprite_t *sprite);

extern void disk_io_set_mode(int mode,int dat_swap);
extern void neo2_enable_sd(void);
extern void neo2_disable_sd(void);
extern volatile void neo_sync_bus(void);
extern DSTATUS MMC_disk_initialize(void);

extern void neo_select_menu(void);
extern void neo_select_game(void);
extern void neo_select_psram(void);
extern void neo_psram_offset(int offs);

extern unsigned int neo_id_card(void);
extern unsigned int neo_get_cpld(void);
extern void neo_run_menu(void);
extern void neo_run_game(u8 *options, int reset);
extern void neo_run_psram(u8 *options, int reset);
extern void neo_copyfrom_game(void *dest, int fstart, int len);
extern void neo_copyfrom_menu(void *dest, int fstart, int len);
extern void neo_xferto_psram(void *src, int pstart, int len);
extern void neo_xferfrom_psram(void *dst, int pstart, int len);
extern void neo_copyto_psram(void *src, int pstart, int len);
extern void neo_copyfrom_psram(void *dst, int pstart, int len);
extern void neo_copyto_sram(void *src, int sstart, int len);
extern void neo_copyfrom_sram(void *dst, int sstart, int len);
extern void neo_copyto_nsram(void *src, int sstart, int len, int mode);
extern void neo_copyfrom_nsram(void *dst, int sstart, int len, int mode);
extern void neo_copyto_eeprom(void *src, int sstart, int len, int mode);
extern void neo_copyfrom_eeprom(void *dst, int sstart, int len, int mode);
extern void neo_copyto_fram(void *src, int sstart, int len, int mode);
extern void neo_copyfrom_fram(void *dst, int sstart, int len, int mode);
extern void neo_get_rtc(unsigned char *rtc);
extern void neo2_cycle_sd(void);

extern int disk_io_force_wdl;
extern int get_cic(unsigned char *buffer);
extern int get_swap(unsigned char *buffer);
extern int get_cic_save(char *cartid, int *cic, int *save);

extern void checksum_psram(void);

extern int doOSKeyb(char *title, char *deflt, char *buffer, int max, int bfill);
extern int RequestFile (char *title, char *initialPath, int bfill);

int f_force_open(FIL* f,XCHAR* s,unsigned int flags,int retries)
{
	int i,j,k;

	if( !(flags&FA_WRITE) )
		return f_open(f,s,flags) == FR_OK;

	i = j = 0;
	k = retries;
	disk_io_force_wdl = 0;

	while(i++ < k)
	{
		memset(f,0,sizeof(FIL));
		neo2_disable_sd();
		neo2_enable_sd();
		getSDInfo(-1);

		if(f_open(f,s,flags) == FR_OK)
		{
			j = 1;
			break;
		}

		disk_io_force_wdl += 200;
	}

	disk_io_force_wdl = 0;
	return j;
}

void w2cstrcpy(void *dst, void *src)
{
    int ix = 0;
    while (1)
    {
        *(char *)(dst + ix) = *(XCHAR *)(src + ix*2) & 0x00FF;
        if (*(char *)(dst + ix) == 0)
            break;
        ix++;
    }
}

void w2cstrcat(void *dst, void *src)
{
    int ix = 0, iy = 0;
    // find end of str in dst
    while (1)
    {
        if (*(char *)(dst + ix) == 0)
            break;
        ix++;
    }
    while (1)
    {
        *(char *)(dst + ix) = *(XCHAR *)(src + iy*2) & 0x00FF;
        if (*(char *)(dst + ix) == 0)
            break;
        ix++;
        iy++;
    }
}

void c2wstrcpy(void *dst, void *src)
{
    int ix = 0;
    while (1)
    {
        *(XCHAR *)(dst + ix*2) = *(char *)(src + ix) & 0x00FF;
        if (*(XCHAR *)(dst + ix*2) == (XCHAR)0)
            break;
        ix++;
    }
}

void c2wstrcat(void *dst, void *src)
{
    int ix = 0, iy = 0;
    // find end of str in dst
    while (1)
    {
        if (*(XCHAR *)(dst + ix*2) == (XCHAR)0)
            break;
        ix++;
    }
    while (1)
    {
        *(XCHAR *)(dst + ix*2) = *(char *)(src + iy) & 0x00FF;
        if (*(XCHAR *)(dst + ix*2) == (XCHAR)0)
            break;
        ix++;
        iy++;
    }
}

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

/* input - do getButtons() first, then getAnalogX() and/or getAnalogY() */
unsigned short getButtons(int pad)
{
    // Read current controller status
    memset(&gKeys, 0, sizeof(gKeys));
    controller_read(&gKeys);
    return (unsigned short)(gKeys.c[pad].data >> 16);
}

unsigned char getAnalogX(int pad)
{
    return (unsigned char)gKeys.c[pad].x;
}

unsigned char getAnalogY(int pad)
{
    return (unsigned char)gKeys.c[pad].y;
}

display_context_t lockVideo(int wait)
{
    display_context_t dc;

    if (wait)
        while (!(dc = display_lock()));
    else
        dc = display_lock();
    return dc;
}

void unlockVideo(display_context_t dc)
{
    if (dc)
        display_show(dc);
}

/* text functions */
void drawText(display_context_t dc, char *msg, int x, int y)
{
    if (dc)
        graphics_draw_text(dc, x, y, msg);
}

void printText(display_context_t dc, char *msg, int x, int y)
{
    if (x != -1)
        gCursorX = x;
    if (y != -1)
        gCursorY = y;

    if (dc)
        graphics_draw_text(dc, gCursorX*8, gCursorY*8, msg);

    gCursorY++;
    if (gCursorY > 29)
    {
        gCursorY = 0;
        gCursorX ++;
    }
}

/* vblank callback */
void vblCallback(void)
{
    gTicks++;
}

void delay(int cnt)
{
    int then = gTicks + cnt;
    while (then > gTicks) ;
}

/* debug helper function */
void debugText(char *msg, int x, int y, int d)
{
    display_context_t dcon = lockVideo(1);
    unlockVideo(dcon);
    dcon = lockVideo(1);
    printText(dcon, msg, x, y);
    unlockVideo(dcon);
    delay(d);
}

int wait_confirm(void)
{
    unsigned short buttons, previous = 0;

    while (1)
    {
        // get buttons
        buttons = getButtons(0);

        if (A_BUTTON(buttons ^ previous))
        {
            // A changed
            if (!A_BUTTON(buttons))
            {
                // A just released
                return 1;
            }
        }
        if (B_BUTTON(buttons ^ previous))
        {
            // B changed
            if (!B_BUTTON(buttons))
            {
                // B just released
                return 0;
            }
        }
        previous = buttons;
        delay(1);
    }
}

static void progress_screen(char *str1, char *str2, int frac, int total, int bfill)
{
    static display_context_t dcon;
    static char temp[40];

    // get next buffer to draw in
    dcon = lockVideo(1);

    if ((str1 != NULL) || (str2 != NULL) || (bfill != -1))
    {
        graphics_fill_screen(dcon, 0);

        if (loading && (bfill == 4))
            drawImage(dcon, loading);
        else if ((bfill < 3) && (pattern[bfill] != NULL))
        {
            rdp_sync(SYNC_PIPE);
            rdp_set_default_clipping();
            rdp_enable_texture_copy();
            rdp_attach_display(dcon);
            // Draw pattern
            rdp_sync(SYNC_PIPE);
            rdp_load_texture(0, 0, MIRROR_DISABLED, pattern[bfill]);
            for (int j=0; j<240; j+=pattern[bfill]->height)
                for (int i=0; i<320; i+=pattern[bfill]->width)
                    rdp_draw_sprite(0, i, j);
            rdp_detach_display();
        }

        graphics_set_color(gTextColors.sel_game, 0);
        printText(dcon, str1, 20 - strlen(str1)/2, 3);

        graphics_set_color(gTextColors.usel_game, 0);
        strncpy(temp, str2, 34);
        temp[34] = 0;
        printText(dcon, temp, 20-strlen(temp)/2, 5);
        for (int ix=34, iy=6; ix<strlen(str2); ix+=34, iy++)
        {
            strncpy(temp, &str2[ix], 34);
            temp[34] = 0;
            printText(dcon, temp, 20-strlen(temp)/2, iy);
        }

        // show display
        unlockVideo(dcon);

        // get next buffer to draw in
        dcon = lockVideo(1);
        graphics_fill_screen(dcon, 0);

        if (loading && (bfill == 4))
            drawImage(dcon, loading);
        else if ((bfill < 3) && (pattern[bfill] != NULL))
        {
            rdp_sync(SYNC_PIPE);
            rdp_set_default_clipping();
            rdp_enable_texture_copy();
            rdp_attach_display(dcon);
            // Draw pattern
            rdp_sync(SYNC_PIPE);
            rdp_load_texture(0, 0, MIRROR_DISABLED, pattern[bfill]);
            for (int j=0; j<240; j+=pattern[bfill]->height)
                for (int i=0; i<320; i+=pattern[bfill]->width)
                    rdp_draw_sprite(0, i, j);
            rdp_detach_display();
        }

        graphics_set_color(gTextColors.sel_game, 0);
        printText(dcon, str1, 20 - strlen(str1)/2, 3);

        graphics_set_color(gTextColors.usel_game, 0);
        strncpy(temp, str2, 34);
        temp[34] = 0;
        printText(dcon, temp, 20-strlen(temp)/2, 5);
        for (int ix=34, iy=6; ix<strlen(str2); ix+=34, iy++)
        {
            strncpy(temp, &str2[ix], 34);
            temp[34] = 0;
            printText(dcon, temp, 20-strlen(temp)/2, iy);
        }
    }

    if (frac)
        graphics_draw_box(dcon, 32, 160, 256*frac/total, 6, graphics_make_color(0x3F, 0xFF, 0x3F, 0xFF));
    if (frac<total)
        graphics_draw_box(dcon, 32+256*frac/total, 160, 256-256*frac/total, 6, graphics_make_color(0xFF, 0x3F, 0x3F, 0xFF));

    // show display
    unlockVideo(dcon);
}

void get_boxart(int brwsr, int entry)
{
    char boxpath[32];
    XCHAR fpath[32];
    FIL lSDFile;
    UINT ts;
    unsigned short cart = *(unsigned short *)&gTable[entry].rom[0x1C];
    int ix = (gTable[entry].rom[0x1C] ^ gTable[entry].rom[0x1D]) & 15;

    sprintf(boxpath, "/menu/n64/boxart/%c%c.sprite", gTable[entry].rom[0x1C], gTable[entry].rom[0x1D]);

    // default to unknown
    memcpy(&boxart[ix*14984], unknown, 14984);
    boxes[ix] = cart;
    if (brwsr)
    {
        c2wstrcpy(fpath, boxpath);
        if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
            return;                     // couldn't open file
        // read boxart sprite
        f_read(&lSDFile, &boxart[ix*14984], 14984, &ts);
        f_close(&lSDFile);
    }
    /* Invalidate data associated with sprite in cache */
    data_cache_writeback_invalidate( &boxart[ix*14984], 14984 );
}

void sort_entries(int max)
{
    // Sort entries! Not very fast, but small and easy.
    for (int ix=0; ix<max-1; ix++)
    {
        selEntry_t temp;

        if (((gTable[ix].type != 128) && (gTable[ix+1].type == 128)) || // directories first
            ((gTable[ix].type == 128) && (gTable[ix+1].type == 128) &&  strcmp(gTable[ix].name, gTable[ix+1].name) > 0) ||
            ((gTable[ix].type != 128) && (gTable[ix+1].type != 128) &&  strcmp(gTable[ix].name, gTable[ix+1].name) > 0))
        {
            memcpy((void*)&temp, (void*)&gTable[ix], sizeof(selEntry_t));
            memcpy((void*)&gTable[ix], (void*)&gTable[ix+1], sizeof(selEntry_t));
            memcpy((void*)&gTable[ix+1], (void*)&temp, sizeof(selEntry_t));
            ix = !ix ? -1 : ix-2;
        }
    }
}

int getGFInfo(void)
{
    int max = 0;
    u8 options[64];
    char cartid[4];

    neo_copyfrom_menu(options, 0x1D0000, 64);

    // go through entries present in menu flash
    while (options[0] == 0xFF)
    {
        int cic, save, ix;
        u32 rombank;

        // set entry in gTable
        gTable[max].valid = 1;
        gTable[max].type = options[0];
        memcpy(gTable[max].options, options, 8);
        memcpy(gTable[max].name, &options[8], 56);
        // make sure name is null-terminated (menu entry has name padded with spaces)
        for (ix=55; ix>=0; ix--)
            if (gTable[max].name[ix] != ' ')
                break;
        gTable[max].name[ix+1] = 0;

        rombank = (gTable[max].options[1]<<8) | gTable[max].options[2];
        if (rombank > 16)
            rombank = rombank / 64;
        rombank *= (8*1024*1024);
        // copy rom info - both hdr and rom arrays at once
        neo_copyfrom_game(gTable[max].hdr, rombank, 0x40);

        // now check if it needs to be byte-swapped
        gTable[max].swap = get_swap(gTable[max].hdr);
        switch (gTable[max].swap)
        {
            case 1:
            // words byte-swapped
            for (int ix=0; ix<0x20; ix+=4)
            {
                u32 tmp1 = *(u32*)&gTable[max].rom[ix];
                *(u32*)&gTable[max].rom[ix] = ((tmp1 & 0x00FF00FF) << 8) | ((tmp1 & 0xFF00FF00) >> 8);
            }
            break;
            case 2:
            // longs word-swapped
            for (int ix=0; ix<0x20; ix+=4)
            {
                u32 tmp1 = *(u32*)&gTable[max].rom[ix];
                *(u32*)&gTable[max].rom[ix] = ((tmp1 & 0x0000FFFF) << 16) | ((tmp1 & 0xFFFF0000) >> 16);
            }
            break;
            case 3:
            // longs byte-swapped
            for (int ix=0; ix<0x20; ix+=4)
            {
                u32 tmp1 = *(u32*)&gTable[max].rom[ix];
                *(u32*)&gTable[max].rom[ix] = (tmp1 << 24) | ((tmp1 & 0xFF00) << 8) | ((tmp1 & 0xFF0000) >> 8) | (tmp1 >> 24);
            }
            break;
        }

        // get cartid
        sprintf(cartid, "%c%c", gTable[max].rom[0x1C], gTable[max].rom[0x1D]);
        if (get_cic_save(cartid, &cic, &save))
        {
            // cart was found, use CIC and SaveRAM type
            gTable[max].options[5] = save;
            gTable[max].options[6] = cic;
        }

        // next entry
        max++;
        if (max == 1024)
            break;

        neo_copyfrom_menu(options, 0x1D0000 + max*64, 64);
    }
    if (max < 1024)
        gTable[max].valid = 0;

    if (max > 1)
        sort_entries(max);

    return max;
}

void check_fast(void)
{
    FIL lSDFile;
    XCHAR fpath[24];

    fast_flag = 0;
    c2wstrcpy(fpath, "/menu/n64/.quick");
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        fast_flag = 1;
        f_close(&lSDFile);
        return;
    }
    c2wstrcpy(fpath, "/menu/n64/.fast");
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        fast_flag = 2;
        f_close(&lSDFile);
        return;
    }
}

void check_dir(char *dname)
{
    DIR dir;
    XCHAR dpath[24];

    c2wstrcpy(dpath, dname);
    if (f_opendir(&dir, dpath) != FR_OK)
        f_mkdir(dpath);
}

int check_sd(void)
{
    DIR dir;
    XCHAR dpath[24];
    FIL lSDFile;
    XCHAR fpath[24];

    c2wstrcpy(dpath, "/menu");
    if (f_opendir(&dir, dpath) != FR_OK)
        return -1;
    c2wstrcpy(dpath, "/menu/n64");
    if (f_opendir(&dir, dpath) != FR_OK)
        return -1;
    c2wstrcpy(dpath, "/menu/n64/save");
    if (f_opendir(&dir, dpath) != FR_OK)
        return -1;
    c2wstrcpy(dpath, "/menu/n64/images");
    if (f_opendir(&dir, dpath) != FR_OK)
        return -1;
    c2wstrcpy(dpath, "/menu/n64/boxart");
    if (f_opendir(&dir, dpath) != FR_OK)
        return -1;

    c2wstrcpy(fpath, "/menu/n64/.fast");
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        f_close(&lSDFile);
        return 0;
    }
    c2wstrcpy(fpath, "/menu/n64/.quick");
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        f_close(&lSDFile);
        return 0;
    }
    c2wstrcpy(fpath, "/menu/n64/.slow");
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        f_close(&lSDFile);
        return 0;
    }

    return -1;
}

int do_sd_mgr(int state)
{
    char temp1[40];
    char temp2[40];
    XCHAR wname[24];
    FIL lSDFile;
    UINT ts;

    fast_flag = 0;

    if (!state)
    {
        // SD card not formatted
        sprintf(temp1, "Card has %d blocks (%d MB)", num_sectors, (num_sectors / 2048));
        strcpy(temp2, "Press A to format, or B to exit");
        progress_screen(temp1, temp2, 0, 100, (loading) ? 4 : 0);
        if (wait_confirm())
            f_mkfs(1, 0, 0);
        else
            return -1;

        strcpy(temp2, "Formatting complete");
        progress_screen(temp1, temp2, 100, 100, (loading) ? 4 : 0);
        delay(60);
    }

    strcpy(temp1, "Checking directory structure");
    strcpy(temp2, "checking /menu");
    progress_screen(temp1, temp2, 0, 100, (loading) ? 4 : 0);
    check_dir("/menu");
    delay(60);
    strcpy(temp2, "checking /menu/n64");
    progress_screen(temp1, temp2, 20, 100, (loading) ? 4 : 0);
    check_dir("/menu/n64");
    delay(60);
    strcpy(temp2, "checking /menu/n64/save");
    progress_screen(temp1, temp2, 40, 100, (loading) ? 4 : 0);
    check_dir("/menu/n64/save");
    delay(60);
    strcpy(temp2, "checking /menu/n64/images");
    progress_screen(temp1, temp2, 60, 100, (loading) ? 4 : 0);
    check_dir("/menu/n64/images");
    delay(60);
    strcpy(temp2, "checking /menu/n64/boxart");
    progress_screen(temp1, temp2, 80, 100, (loading) ? 4 : 0);
    check_dir("/menu/n64/boxart");
    delay(60);
    strcpy(temp2, "complete");
    progress_screen(temp1, temp2, 100, 100, (loading) ? 4 : 0);
    delay(60);

    strcpy(temp1, "Checking SD card read speed");
    strcpy(temp2, "opening test file");
    progress_screen(temp1, temp2, 0, 100, (loading) ? 4 : 0);
    c2wstrcpy(wname, "/.test");
    if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
    {
        strcpy(temp2, "writing test file");
        for (int i=0; i<ONCE_SIZE; i+=8192)
        {
            progress_screen(temp1, temp2, i*100/ONCE_SIZE, 100, (loading) ? 4 : 0);
            for (int j=0; j<8192; j+=4)
                *(int*)&tmpBuf[j] = (i+j);
            f_write(&lSDFile, tmpBuf, 8192, &ts);
        }
        f_close(&lSDFile);
        strcpy(temp2, "reading test file");
        progress_screen(temp1, temp2, 0, 100, (loading) ? 4 : 0);
        if (f_open(&lSDFile, wname, FA_OPEN_EXISTING | FA_READ) == FR_OK)
        {
            fast_flag = 2; // fast - full speed
            for (int i=0; i<ONCE_SIZE; i+=8192)
            {
                progress_screen(temp1, temp2, i*100/ONCE_SIZE, 100, (loading) ? 4 : 0);
                f_read(&lSDFile, tmpBuf, 8192, &ts);
                for (int j=0; j<8192; j+=4)
                    if (*(int*)&tmpBuf[j] != (i+j))
                        fast_flag = 0;
            }
            f_lseek(&lSDFile, 0);
            if (!fast_flag)
            {
                fast_flag = 1; // quick - slightly less than full speed
                for (int i=0; i<ONCE_SIZE; i+=8192)
                {
                    progress_screen(temp1, temp2, i*100/ONCE_SIZE, 100, (loading) ? 4 : 0);
                    f_read(&lSDFile, tmpBuf, 8192, &ts);
                    for (int j=0; j<8192; j+=4)
                        if (*(int*)&tmpBuf[j] != (i+j))
                            fast_flag = 0;
                }
                f_lseek(&lSDFile, 0);
            }
            f_close(&lSDFile);
            f_unlink(wname); // delete test file
        }
        sprintf(temp2, "SD card speed %s", (fast_flag == 2) ? "FAST" : (fast_flag == 1) ? "QUICK" : "SLOW");
        progress_screen(temp1, temp2, 100, 100, (loading) ? 4 : 0);
        delay(120);
        if (fast_flag == 2)
        {
            // the existence of this file implies fast read mode allowed
            c2wstrcpy(wname, "/menu/n64/.fast");
            if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                f_close(&lSDFile);
        }
        else if (fast_flag == 1)
        {
            // the existence of this file implies quick read mode allowed
            c2wstrcpy(wname, "/menu/n64/.quick");
            if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                f_close(&lSDFile);
        }
        else
        {
            c2wstrcpy(wname, "/menu/n64/.slow");
            if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                f_close(&lSDFile);
        }
    }
    return 0;
}

void get_sd_info(int entry)
{
    FIL lSDFile;
    UINT ts;
    u8 buffer[0x440];
    XCHAR fpath[1280];
    char cartid[4];
    int cic, save;

    c2wstrcpy(fpath, path);
    if (path[strlen(path)-1] != '/')
        c2wstrcat(fpath, "/");

    c2wstrcat(fpath, gTable[entry].name);
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        //char temp[1536];
        // couldn't open file
        //w2cstrcpy(temp, fpath);
        debugText("Couldn't open file: ", 2, 16, 1);
        debugText(gTable[entry].name, 22, 16, 180);
        return;
    }
    // read rom header
    f_read(&lSDFile, buffer, 0x440, &ts);
    f_close(&lSDFile);

    // check for NES rom
    if (!strncasecmp(buffer, "NES\x1A", 4))
    {
        // NES rom with header
        gTable[entry].options[0] = 0xFF; // game
        gTable[entry].options[1] = 0;
        gTable[entry].options[2] = 0;
        gTable[entry].options[3] = 0;
        gTable[entry].options[4] = 0x0F; // 64Mbit
        gTable[entry].options[5] = 0x0F; // Save off
        gTable[entry].options[6] = 2;    // CIC 6102
        gTable[entry].options[7] = 0;
        gTable[entry].type = 254;

        return;
    }

    // copy rom info - both hdr and rom arrays at once
    memcpy(gTable[entry].hdr, buffer, 0x40);

    // now check if it needs to be byte-swapped
    gTable[entry].swap = get_swap(gTable[entry].hdr);
    switch (gTable[entry].swap)
    {
        case 1:
        // words byte-swapped
        for (int ix=0; ix<0x20; ix+=4)
        {
            u32 tmp1 = *(u32*)&gTable[entry].rom[ix];
            *(u32*)&gTable[entry].rom[ix] = ((tmp1 & 0x00FF00FF) << 8) | ((tmp1 & 0xFF00FF00) >> 8);
        }
        break;
        case 2:
        // longs word-swapped
        for (int ix=0; ix<0x20; ix+=4)
        {
            u32 tmp1 = *(u32*)&gTable[entry].rom[ix];
            *(u32*)&gTable[entry].rom[ix] = ((tmp1 & 0x0000FFFF) << 16) | ((tmp1 & 0xFFFF0000) >> 16);
        }
        break;
        case 3:
        // longs byte-swapped
        for (int ix=0; ix<0x20; ix+=4)
        {
            u32 tmp1 = *(u32*)&gTable[entry].rom[ix];
            *(u32*)&gTable[entry].rom[ix] = (tmp1 << 24) | ((tmp1 & 0xFF00) << 8) | ((tmp1 & 0xFF0000) >> 8) | (tmp1 >> 24);
        }
        break;
    }

    gTable[entry].options[6] = get_cic(&buffer[0x40]);
    gTable[entry].type = 255;

    // get cartid
    sprintf(cartid, "%c%c", gTable[entry].rom[0x1C], gTable[entry].rom[0x1D]);
    if (get_cic_save(cartid, &cic, &save))
    {
        // cart was found, use CIC and SaveRAM type
        gTable[entry].options[5] = save;
        gTable[entry].options[6] = cic;
    }
}

int getSDInfo(int entry)
{
    DIR dir;
    FILINFO fno;
    int ix, max = 0;
    XCHAR lfnbuf[256];
    XCHAR fpath[1280];
    XCHAR privateName[8];

    c2wstrcpy(privateName, "menu");

    gSdDetected = 0;

    if (entry == -1)
    {
        // get root
        strcpy(path, "/");
        cardType = 0x0000;
        // unmount SD card
        f_mount(1, NULL);
        // init SD card
        cardType = 0x0000;
        if (MMC_disk_initialize() == STA_NODISK)
            return 0;                   /* couldn't init SD card */
        // mount SD card
        if (f_mount(1, &gSDFatFs))
        {
            debugText("Couldn't mount SD card", 9, 2, 180);
            return 0;                   /* couldn't mount SD card */
        }
        f_chdrive(1);                   /* make MMC current drive */
        // now check for funky timing by trying to opendir on the root
        c2wstrcpy(fpath, path);
        if (f_opendir(&dir, fpath))
        {
            // failed, try funky timing
            cardType = 0x8000;          /* try funky read timing */
            if (MMC_disk_initialize() == STA_NODISK)
                return 0;               /* couldn't init SD card */
        }
        gSdDetected = 1;
    }
    else
    {
        // get subdirectory
        if (gTable[entry].name[0] == '.')
        {
            // go back one level
            ix = strlen(path) - 1;
            if (ix < 0)
                ix = 0;                 /* for safety */
            while (ix > 0)
            {
                if (path[ix] == '/')
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
            if (path[strlen(path)-1] != '/')
                strcat(path, "/");

            strcat(path, gTable[entry].name);
        }
        gSdDetected = 1;
    }
    c2wstrcpy(fpath, path);
    if (f_opendir(&dir, fpath))
    {
        gSdDetected = 0;
        if (do_sd_mgr(0))               /* we know there's a card, but can't read it */
            return 0;                   /* user opted not to try to format the SD card */
        if (f_opendir(&dir, fpath))
            return 0;                   /* failed again... give up */
        gSdDetected = 1;
    }

    // if root, check directory structure
    if( (entry == -1) && (check_sd() == -1) )
        do_sd_mgr(1);

    // add parent directory entry if not root
    if (path[1] != 0)
    {
        gTable[max].valid = 1;
        gTable[max].type = 128;         // directory entry
        strcpy(gTable[max].name, "..");
        max++;
    }

    // scan dirctory entries
    f_opendir(&dir, fpath);
    for(;;)
    {
        FRESULT fres;
        fno.lfname = (XCHAR*)lfnbuf;
        fno.lfsize = 255;
        fno.lfname[0] = (XCHAR)0;
        fno.fname[0] = '\0';

        if ((fres = f_readdir(&dir, &fno)))
        {
            //char temp[40];
            //sprintf(temp, "Error %d reading directory", fres);
            //debugText(temp, 7, 2, 180);
            break;                      /* no more entries in directory (or some other error) */
        }

        if (!fno.fname[0])
        {
            //debugText("No more entries in directory", 6, 2, 180);
            break;                      /* no more entries in directory (or some other error) */
        }

        if (fno.fname[0] == '.')
            continue;                   /* skip links */

        if (fno.lfname[0] == (XCHAR)'.')
            continue;                   /* skip "hidden" files and directories */

        if (fno.fattrib & AM_DIR)
        {
            if(fno.lfname[0])
            {
                if( (wstrcmp(privateName,fno.lfname) == 0) )
                    continue;

                w2cstrcpy(gTable[max].name, fno.lfname);
            }
            else
            {
                /*Skip checking for length to allow prefixes/etc : menu , menu_ , menu_1234567 , menu_abcd..*/
                if( (memcmp(fno.fname,"menu",4) == 0 ) /*&& (strlen(fno.fname) == 4)*/ )
                    continue;

                strcpy(gTable[max].name, fno.fname);
            }

            gTable[max].valid = 1;
            gTable[max].type = 128;     // directory entry
        }
        else
        {
            u8* options = gTable[max].options;

            gTable[max].valid = 1;
            gTable[max].type = 127;     // unknown
            options[0] = 0xFF;
            options[1] = 0;
            options[2] = 0;
            options[3] = (fno.fsize / (128*1024)) >> 8;
            options[4] = (fno.fsize / (128*1024)) & 0xFF;
            options[5] = 0;
            options[6] = 2;
            options[7] = 0;

            if (fno.lfname[0])
                w2cstrcpy(gTable[max].name, fno.lfname);
            else
                strcpy(gTable[max].name, fno.fname);

            memset(gTable[max].rom, 0, 0x20);

            //get_sd_info(max); // slows directory load
        }
        max++;
        if (max == 1024)
            break;
    }
    if (max < 1024)
        gTable[max].valid = 0;

    if (max > 1)
        sort_entries(max);

    //{
    //  char temp[40];
    //  sprintf(temp, "%d entries found", max);
    //  debugText(temp, 12, 13, 180);
    //}
    return max;
}

/* Copy data from game flash to gba psram */
void copyGF2Psram(int bselect, int bfill)
{
    u32 rombank, romsize, gamelen;
    char temp[256];

    rombank = (gTable[bselect].options[1]<<8) | gTable[bselect].options[2];
    if (rombank > 16)
        rombank = rombank / 64;
    rombank *= (8*1024*1024);

    romsize = (gTable[bselect].options[3]<<8) | gTable[bselect].options[4];
    switch (romsize)
    {
        case 0x0000:
        // 1 Gbit
        gamelen = 128*1024*1024;
        break;
        case 0x0008:
        // 512 Mbit
        gamelen = 64*1024*1024;
        break;
        case 0x000C:
        // 256 Mbit
        gamelen = 32*1024*1024;
        break;
        case 0x000E:
        // 128 Mbit
        gamelen = 16*1024*1024;
        break;
        case 0x000F:
        // 64 Mbit
        gamelen = 8*1024*1024;
        break;
        default:
        // romsize is number of Mbits in rom
        gamelen = (romsize & 0xFFFF)*128*1024;
        break;
    }

    strcpy(temp, gTable[bselect].name);

    // copy the rom from file to ram, then ram to psram
    for(int ic=0; ic<gamelen; ic+=ONCE_SIZE)
    {
        progress_screen("Loading", temp, 100*ic/gamelen, 100, bfill);
        // copy game flash to buffer
        neo_copyfrom_game(tmpBuf, rombank+ic, ONCE_SIZE);
        // now check if it needs to be byte-swapped
        switch (gTable[bselect].swap)
        {
            case 1:
            // words byte-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=8)
            {
                u64 tmp1 = *(u64*)&tmpBuf[ix];
                *(u64*)&tmpBuf[ix] = ((tmp1 & 0x00FF00FF00FF00FFULL) << 8) | ((tmp1 & 0xFF00FF00FF00FF00ULL) >> 8);
            }
            break;
            case 2:
            // longs word-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=8)
            {
                u64 tmp1 = *(u64*)&tmpBuf[ix];
                *(u64*)&tmpBuf[ix] = ((tmp1 & 0x0000FFFF0000FFFFULL) << 16) | ((tmp1 & 0xFFFF0000FFFF0000ULL) >> 16);
            }
            break;
            case 3:
            // longs byte-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=4)
            {
                u32 tmp1 = *(u32*)&tmpBuf[ix];
                *(u32*)&tmpBuf[ix] = (tmp1 << 24) | ((tmp1 & 0xFF00) << 8) | ((tmp1 & 0xFF0000) >> 8) | (tmp1 >> 24);
            }
            break;
        }
        // copy to psram
        neo_copyto_psram(tmpBuf, ic, ONCE_SIZE);
    }

    progress_screen("Loading", temp, 100, 100, bfill);
    delay(30);
}

void sync_sd_stream(FIL* f,XCHAR* s)
{
    char tmp[2048];

	neo2_disable_sd();
	neo2_enable_sd();
	getSDInfo(-1);
	memset(f,0,sizeof(FIL));

	if(f_open(f,s,FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        w2cstrcpy(tmp,s);
        debugText("Couldn't open file: ", 2, 2, 0);
        debugText(tmp, 22, 2, 180);
    }
}

void copyfrom_sd_to_psram(FIL* f,int len,int disk_mode,int swap,int bfill,const char* msg,const char* fn)
{
	const int read = 256*1024;
	int i;
	UINT ts;

	if(len <= 0)
		return;

    progress_screen(msg,fn, 0, 100, bfill);
    disk_io_set_mode(disk_mode,swap);				//Also resets PSRAM pointer

	for(i = 0; i<len; i+=read)
	{
		progress_screen(NULL,NULL, 100*i/len, 100,-1);
		f_read_dummy(f,read,&ts);
	}

	progress_screen(NULL,NULL,100,100,-1);
	delay(5);
}

#ifdef __DEBUG_PSRAM__
#define MYTH_IO_BASE (0xA8040000)
#define ROM_BANK  *(vu32*)(MYTH_IO_BASE | 0x0C*2)
#define ROM_SIZE  *(vu32*)(MYTH_IO_BASE | 0x08*2) 

void test_psram(int m,const char* s)
{
	char* p,*p2;
	int l,k;
 
	l = strlen(s);
	p2 = (char*)&tmpBuf[1024];

	if( l & 3 )
	{
		//align 
		k = (l + 4) & (~3);
		p = (char*)tmpBuf;
		memcpy(p,s,l);
		memset(p + l,0,k-l);
		l = k;
	}
	else
		p = s;

	neo_psram_offset((m*1024*1024) / (32*1024));
	neo_copyto_psram(p,0,l);
	neo_copyfrom_psram(p2,0,l);
	debugText(p2,5,18,1);
	while(1){}
}

void test_psram2(const char* s)
{
	char* p,*p2;
	int l,k;
 	int m = 48;
	display_context_t ctx;

	l = strlen(s);
	p2 = (char*)&tmpBuf[1024];

	if( l & 3 )
	{
		//align 
		k = (l + 4) & (~3);
		p = (char*)tmpBuf;
		memcpy(p,s,l);
		memset(p + l,0,k-l);
		l = k;
	}
	else
		p = s;

	ctx = lockVideo(1);
	graphics_fill_screen(ctx,0);
	unlockVideo(ctx);

	neo_psram_offset((48*1024*1024) / (32*1024));
	neo_copyto_psram(p,0,l);
	neo_copyfrom_psram(p2,0,l);
	debugText(p2,4,4,1);

	//Now check to see if its getting "mirrored" (caused by range wrapping)
	neo_psram_offset(0);
	neo_copyfrom_psram(p2,0,l);
	debugText(p2,4,5,1);

	neo_psram_offset((32*1024*1024) / (32*1024));
	neo_copyfrom_psram(p2,0,l);
	debugText(p2,4,6,1);

	neo_psram_offset((16*1024*1024) / (32*1024));
	neo_copyfrom_psram(p2,0,l);
	debugText(p2,4,7,1);

	while(1){}
}

const char* align_text(char* s,char* back_buff,int* dl)
{
	int l,k;
	char* p;

	l = strlen(s);

	if( l & 3 )
	{
		//align 
		k = (l + 4) & (~3);
		p = (char*)back_buff;
		memcpy(p,s,l);
		memset(p + l,0,k-l);

		*dl = k;
		return p;
	}

	*dl = l;
	return s;
}

#define MYTH_IO_BASE (0xA8040000)
#define ROM_BANK  *(vu32*)(MYTH_IO_BASE | 0x0C*2)

void test_psram3(const char* a,const char* b,const char* c,const char* d)
{
	display_context_t	ctx;
	int 				ia,ib,ic,id;
	const char			*pa,*pb,*pc,*pd,*pe;
	char				*pf;

	//set up strings
	pa = align_text(a,&tmpBuf[0],&ia);
	pb = align_text(b,&tmpBuf[128],&ib);
	pc = align_text(c,&tmpBuf[256],&ic);
	pd = align_text(d,&tmpBuf[384],&id);
	pf = &tmpBuf[512];

	//Write each to its own region:bank
	ROM_BANK = 0; neo_sync_bus();
	neo_psram_offset(0);
	neo_copyto_psram(pa,0,ia);
	neo_psram_offset((16*1024*1024) / (32*1024));
	neo_copyto_psram(pb,0,ib);

	ROM_BANK = 0x00010001; neo_sync_bus();
	neo_psram_offset(0);
	neo_copyto_psram(pc,0,ic);
	neo_psram_offset((16*1024*1024) / (32*1024));
	neo_copyto_psram(pd,0,id);

	//clean up framebuffer
	ctx = lockVideo(1);
	graphics_fill_screen(ctx,0);
	unlockVideo(ctx);

	//Read each one and print it
	ROM_BANK = 0; neo_sync_bus();
	neo_psram_offset(0);
	neo_copyfrom_psram(pf,0,ia);
	debugText(pf,4,4,1);
	neo_psram_offset((16*1024*1024) / (32*1024));
	neo_copyfrom_psram(pf,0,ib);
	debugText(pf,4,5,1);

	ROM_BANK = 0x00010001; neo_sync_bus();
	neo_psram_offset(0);
	neo_copyfrom_psram(pf,0,ic);
	debugText(pf,4,6,1);
	neo_psram_offset((16*1024*1024) / (32*1024));
	neo_copyfrom_psram(pf,0,id);
	debugText(pf,4,7,1);

	while(1){}
}

int last_psram_addr = -1; 				//to avoid wasting cycles with asic ops

int neo_psram_offset_addr(unsigned int addr,int force)
{
	const int mb = 1024*1024;

	last_psram_addr = (force) ? -1 : last_psram_addr;

	
	if(addr <= (16*mb))//0..16MB				
	{
		if(last_psram_addr == 0)
			return 0;

		last_psram_addr = 0;
		neo_psram_offset(0);

		return 0;	//special case
	}
	else if(addr <= (32*mb))//16..32MB
	{
		if(last_psram_addr == 16)
			return 0;

		last_psram_addr = 16;
	}
	else if(addr <= (48*mb))//32..48MB
	{
		if(last_psram_addr == 32)
			return 0;

		last_psram_addr = 32;
	}
	else if(addr <= (64*mb))//48..64MB
	{
		if(last_psram_addr == 48)
			return 0;

		last_psram_addr = 48;
	}
	else	
		return 0;

	neo_psram_offset((last_psram_addr*mb) >> 15);

	//Yeah! we've just updated flash offset
	//The result returned allows you to decide when to reset PSRAM address :)
	return 1;
}
#endif

void fastCopySD2Psram(int bselect,int bfill)
{
    FIL f;
    u32 gamelen,rem,addr,a;
    XCHAR fpath[1280];
    char temp[256];
	int swp;
	int unit;

	#ifdef __DEBUG_PSRAM__
	test_psram3("Haha","Hehe","Hihi","Hoho");
	test_psram2("Can you see this ???");
	#endif
    disk_io_set_mode(0,0);

    if (gTable[bselect].type == 127)
        get_sd_info(bselect);
	
	swp = gTable[bselect].swap;
    strcpy(temp, gTable[bselect].name);
    c2wstrcpy(fpath, path);

    if (path[strlen(path)-1] != '/')
        c2wstrcat(fpath, "/");

    c2wstrcat(fpath, gTable[bselect].name);

    if(f_open(&f, fpath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        char temp2[1536];

        w2cstrcpy(temp2, fpath);
        debugText("Couldn't open file: ", 2, 2, 0);
        debugText(temp2, 22, 2, 180);

        return;
    }

	gamelen = f.fsize;
	neo_psram_offset(0);

	#if 1
	unit = 16*1024*1024;

	#if 0

	//Just to make sure 
	copyfrom_sd_to_psram(&f,unit,1,swp,bfill,"Loading",temp); 		//0-16
	neo_psram_offset((16*1024*1024) / (32*1024));
	copyfrom_sd_to_psram(&f,unit,1,swp,bfill,"Loading",temp);		//16-32
	neo_psram_offset((32*1024*1024) / (32*1024));			
	copyfrom_sd_to_psram(&f,unit,1,swp,bfill,"Loading",temp);		//32-48
	neo_psram_offset((48*1024*1024) / (32*1024));
	copyfrom_sd_to_psram(&f,unit,1,swp,bfill,"Loading",temp);		//48-64

	#else
	if(gamelen > (32 * 1024 * 1024))//Up to 512Mbit --- XXX READ XXX:DOES NOT WORK YET. The core wraps around anything > 32MB
	{
		//Write 1/2
		copyfrom_sd_to_psram(&f,unit,1,swp,bfill,"Loading",temp);

		//Switch offset
		neo_psram_offset((16*1024*1024) / (32*1024));

		//Write 2/2
		copyfrom_sd_to_psram(&f,unit,1,swp,bfill,"Loading",temp);

		//Switch offset
		neo_psram_offset((32*1024*1024) / (32*1024));

		//Subtract 32MB from count
		gamelen -= 32*1024*1024;

		if(gamelen <= unit)//Just up to 48MB ?
			copyfrom_sd_to_psram(&f,gamelen,1,swp,bfill,"Loading",temp);
		else//Image contains at least 1x16MB block
		{
			//Write 1/2 
			copyfrom_sd_to_psram(&f,unit,1,swp,bfill,"Loading",temp);

			//Switch offset
			neo_psram_offset((48*1024*1024) / (32*1024));

			//Write 2/2 and subtract unit from count
			copyfrom_sd_to_psram(&f,gamelen-unit,1,swp,bfill,"Loading",temp);
		}
	}
	else//Up to 256Mbit
	{
		if(gamelen <= unit)
			copyfrom_sd_to_psram(&f,gamelen,1,swp,bfill,"Loading",temp);
		else
		{
			//Write 1/2 
			copyfrom_sd_to_psram(&f,unit,1,swp,bfill,"Loading 1/2",temp);

			//Switch offset
			neo_psram_offset((16*1024*1024) / (32*1024));

			//Write 2/2 and subtract unit from count
			copyfrom_sd_to_psram(&f,gamelen-unit,1,swp,bfill,"Loading 2/2",temp);
		}
	}
	#endif
	#else
	disk_io_set_mode(1,swp);
	u32 i;
	UINT ts;

	neo_psram_offset_addr(0,1);

	for(i = 0;i<gamelen;i+=ONCE_SIZE)
	{
		if(neo_psram_offset_addr(i,0))
		{
			//reset psram addr
			disk_io_set_mode(1,swp);
		}

		progress_screen(NULL,NULL, 100*i/gamelen, 100,-1);
		f_read_dummy(&f,ONCE_SIZE,&ts);
	}

	#endif
	neo_psram_offset(0);
	disk_io_set_mode(0,0);
    f_close(&f);
}

/* Load neon64 emulator and append NES file in psram */
void loadNES2Psram(int bselect, int bfill)
{
    FIL lSDFile;
    UINT ts, emusize;
    u32 gamelen;
    XCHAR fpath[1280];
    char temp[256];

    disk_io_set_mode(0,0);

    progress_screen("Loading", "neon64 emulator", 0, 100, bfill);

    // clear psram
    memset(tmpBuf, 0, ONCE_SIZE);
    for(int ix=0; ix<1280*1024; ix+=ONCE_SIZE)
        neo_xferto_psram(tmpBuf, ix, ONCE_SIZE);

    c2wstrcpy(fpath, "/menu/n64/neon64/neon64cd.rom");
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        char temp[1536];
        // couldn't open file
        w2cstrcpy(temp, fpath);
        debugText("Couldn't open file: ", 2, 2, 0);
        debugText(temp, 22, 2, 180);
        return;
    }
    f_read(&lSDFile, tmpBuf, lSDFile.fsize, &emusize);
    neo_xferto_psram(tmpBuf, 0, emusize);
    f_close(&lSDFile);

    strcpy(temp, gTable[bselect].name);
    progress_screen("Loading", temp, 33, 100, bfill);

    c2wstrcpy(fpath, path);
    if (path[strlen(path)-1] != '/')
        c2wstrcat(fpath, "/");

    c2wstrcat(fpath, gTable[bselect].name);
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        char temp[1536];
        // couldn't open file
        w2cstrcpy(temp, fpath);
        debugText("Couldn't open file: ", 2, 2, 0);
        debugText(temp, 22, 2, 180);
        return;
    }

    // copy the rom from file to ram, then ram to psram
    gamelen = lSDFile.fsize;
    for(int ic=0; ic<gamelen; ic+=ONCE_SIZE)
    {
        memset(tmpBuf, 0, ONCE_SIZE);
        f_read(&lSDFile, tmpBuf, ONCE_SIZE, &ts);
        neo_xferto_psram(tmpBuf, ic + emusize, ONCE_SIZE);
    }
    f_close(&lSDFile);

    progress_screen("Checksumming", "...", 66, 100, bfill);
    checksum_psram();
#if 0
    {
        char temp[40];
        unsigned int cs1, cs2;
        debugText("checksum = ", 2, 2, 2);
        cs1 = *(unsigned int *)0xB0000010;
        cs2 = *(unsigned int *)0xB0000014;
        sprintf(temp, "%08X %08X", cs1, cs2);
        debugText(temp, 13, 2, 1200);
    }
#endif
    progress_screen(NULL,NULL,100,100,-1);
    delay(5);
}

/* Copy data from file to gba psram */
void copySD2Psram(int bselect, int bfill)
{
    FIL lSDFile;
    UINT ts;
    u32 romsize, gamelen, copylen;
    XCHAR fpath[1280];
    char temp[256];

    disk_io_set_mode(0,0);

    // load rom info if not already loaded
    if (gTable[bselect].type == 127)
        get_sd_info(bselect);

    if((gTable[bselect].swap == 0) || (gTable[bselect].swap == 1) )
    {
        fastCopySD2Psram(bselect,bfill);
        disk_io_set_mode(0,0);
        return;
    }

    romsize = (gTable[bselect].options[3]<<8) | gTable[bselect].options[4];
    // for SD card file, romsize is number of Mbits in rom
    gamelen = romsize*128*1024;

    strcpy(temp, gTable[bselect].name);

    c2wstrcpy(fpath, path);
    if (path[strlen(path)-1] != '/')
        c2wstrcat(fpath, "/");

    c2wstrcat(fpath, gTable[bselect].name);
    if (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        char temp[1536];
        // couldn't open file
        w2cstrcpy(temp, fpath);
        debugText("Couldn't open file: ", 2, 2, 0);
        debugText(temp, 22, 2, 180);
        return;
    }

    if (gamelen > (16*1024*1024))
        copylen = 16*1024*1024;
    else
        copylen = gamelen;

    progress_screen("Loading", temp, 0, 100, bfill);

    // copy the rom from file to ram, then ram to psram
    for(int ic=0; ic<copylen; ic+=ONCE_SIZE)
    {
        progress_screen(NULL,NULL, 100*ic/gamelen, 100,-1);
        f_read(&lSDFile, tmpBuf, ONCE_SIZE, &ts);

        // now check if it needs to be byte-swapped
        switch (gTable[bselect].swap)
        {
            case 1:
            // words byte-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=8)
            {
                u64 tmp1 = *(u64*)&tmpBuf[ix];
                *(u64*)&tmpBuf[ix] = ((tmp1 & 0x00FF00FF00FF00FFULL) << 8) | ((tmp1 & 0xFF00FF00FF00FF00ULL) >> 8);
            }
            break;
            case 2:
            // longs word-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=8)
            {
                u64 tmp1 = *(u64*)&tmpBuf[ix];
                *(u64*)&tmpBuf[ix] = ((tmp1 & 0x0000FFFF0000FFFFULL) << 16) | ((tmp1 & 0xFFFF0000FFFF0000ULL) >> 16);
            }
            break;
            case 3:
            // longs byte-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=4)
            {
                u32 tmp1 = *(u32*)&tmpBuf[ix];
                *(u32*)&tmpBuf[ix] = (tmp1 << 24) | ((tmp1 & 0xFF00) << 8) | ((tmp1 & 0xFF0000) >> 8) | (tmp1 >> 24);
            }
            break;
        }

        neo_xferto_psram(tmpBuf, ic, ONCE_SIZE);
    }

    if (gamelen <= (16*1024*1024))
    {
        f_close(&lSDFile);
        progress_screen(NULL,NULL,100,100,-1);
        delay(5);
        return;
    }

    // change the psram offset and copy the rest
    neo_psram_offset(copylen/(32*1024));

    for(int ic=0; ic<(gamelen-copylen); ic+=ONCE_SIZE)
    {
        progress_screen(NULL,NULL, 100*(copylen+ic)/gamelen, 100,-1);

        // read file to buffer
        f_read(&lSDFile, tmpBuf, ONCE_SIZE, &ts);

        // now check if it needs to be byte-swapped
        switch (gTable[bselect].swap)
        {
            case 1:
            // words byte-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=8)
            {
                u64 tmp1 = *(u64*)&tmpBuf[ix];
                *(u64*)&tmpBuf[ix] = ((tmp1 & 0x00FF00FF00FF00FFULL) << 8) | ((tmp1 & 0xFF00FF00FF00FF00ULL) >> 8);
            }
            break;
            case 2:
            // longs word-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=8)
            {
                u64 tmp1 = *(u64*)&tmpBuf[ix];
                *(u64*)&tmpBuf[ix] = ((tmp1 & 0x0000FFFF0000FFFFULL) << 16) | ((tmp1 & 0xFFFF0000FFFF0000ULL) >> 16);
            }
            break;
            case 3:
            // longs byte-swapped
            for (int ix=0; ix<ONCE_SIZE; ix+=4)
            {
                u32 tmp1 = *(u32*)&tmpBuf[ix];
                *(u32*)&tmpBuf[ix] = (tmp1 << 24) | ((tmp1 & 0xFF00) << 8) | ((tmp1 & 0xFF0000) >> 8) | (tmp1 >> 24);
            }
            break;
        }

        // copy to psram
        neo_xferto_psram(tmpBuf, ic, ONCE_SIZE);
    }
    neo_psram_offset(0);

    f_close(&lSDFile);
    progress_screen(NULL,NULL,100,100,-1);
    delay(5);
}

int selectGBASlot(int sel, u8 *blk, int stype, int bfill)
{
    display_context_t dcon;
    char temp[40];
    char names[20*256];
    u16 buttons, previous = 0;
    int i, ix = sel;
    int ssel, sfirst, slast;
    char str1[] = "Overwriting slot in GBA SRAM";
    char str2[] = "Select GBA SRAM slot for Save State";
    char str3[] = "A/B = Select slot   Z = Erase slot";

    neo_copyfrom_sram(names, 0x3E000, 20*256);

    if (stype == 4)
    {
        // FRAM - only one slot
        sfirst = 0;
        slast = 0;
    }
    else if (stype == 5)
    {
        // 4Kb EEPROM - eight slots
        sfirst = 12;
        slast = 19;
    }
    else if (stype == 6)
    {
        // 16Kb EEPROM - eight slots
        sfirst = 4;
        slast = 11;
    }
    else
    {
        // 32KB SRAM - three slots
        sfirst = 1;
        slast = 3;
    }

    if (sel == 20)
        ssel = sfirst;
    else
        ssel = -1;

    while (1)
    {
        // get next buffer to draw in
        dcon = lockVideo(1);
        graphics_fill_screen(dcon, 0);

        if (browser && (bfill == 4))
        {
            drawImage(dcon, browser);
        }
        else if ((bfill < 3) && (pattern[bfill] != NULL))
        {
            rdp_sync(SYNC_PIPE);
            rdp_set_default_clipping();
            rdp_enable_texture_copy();
            rdp_attach_display(dcon);
            // Draw pattern
            rdp_sync(SYNC_PIPE);
            rdp_load_texture(0, 0, MIRROR_DISABLED, pattern[bfill]);
            for (int j=0; j<240; j+=pattern[bfill]->height)
                for (int i=0; i<320; i+=pattern[bfill]->width)
                    rdp_draw_sprite(0, i, j);
            rdp_detach_display();
        }

        graphics_set_color(gTextColors.title, 0);
        if (ssel == -1)
            printText(dcon, str1, 20 - strlen(str1)/2, 3);
        else
            printText(dcon, str2, 20 - strlen(str2)/2, 3);

        for (int iy=0; iy<(slast - sfirst + 1); iy++)
        {
            if ((sfirst + iy) == sel)
                graphics_set_color(gTextColors.sel_game, 0);
            else
                graphics_set_color((sfirst + iy) == ssel ? gTextColors.sel_game : gTextColors.usel_game, 0);
            if (blk[sfirst + iy] == 0xAA)
            {
                strncpy(temp, &names[(sfirst + iy)*256], 36);
                temp[36] = 0;
                printText(dcon, temp, 20 - strlen(temp)/2, 5 + iy);
            }
            else
                printText(dcon, "empty", 19, 5 + iy);
        }
        graphics_set_color(gTextColors.usel_game, 0);
        printText(dcon, str3, 20 - strlen(str3)/2, 25);

        // show display
        unlockVideo(dcon);

        if (ssel == -1)
        {
            // just showing matching slot
            for (i=0; i<180; i++)
            {
                buttons = getButtons(0);
                if (Z_BUTTON(buttons))
                    break;
                previous = buttons;
                delay(1);
            }
            if (i == 180)
                break;
            ssel = sel;
            sel = 20;
        }

        // get buttons
        buttons = getButtons(0);

        if (DU_BUTTON(buttons))
        {
            // UP pressed, go one entry back
            ssel--;
            if (ssel < sfirst)
                ssel = slast;
            delay(7);
        }
        if (DD_BUTTON(buttons))
        {
            // DOWN pressed, go one entry forward
            ssel++;
            if (ssel > slast)
                ssel = sfirst;
            delay(7);
        }

        if (A_BUTTON(buttons ^ previous))
        {
            // A changed
            if (!A_BUTTON(buttons))
            {
                // A just released
                ix = ssel;
                break;
            }
        }
        if (B_BUTTON(buttons ^ previous))
        {
            // B changed
            if (!B_BUTTON(buttons))
            {
                // B just released
                ix = ssel;
                break;
            }
        }

        if (Z_BUTTON(buttons ^ previous))
        {
            // Z changed
            if (!Z_BUTTON(buttons))
            {
                // Z just released
                blk[ssel] = 0;
            }
        }

        previous = buttons;
        delay(1);
    }

    return ix;
}

#define SWAPLONG(i) (((uint32_t)((i) & 0xFF000000) >> 24) | ((uint32_t)((i) & 0x00FF0000) >>  8) | ((uint32_t)((i) & 0x0000FF00) <<  8) | ((uint32_t)((i) & 0x000000FF) << 24))

void byte_swap_buf(u8 *buf, int size)
{
    u32 *ptr = (u32*)buf;

    for (int ix=0; ix<(size/4); ix++)
        ptr[ix] = SWAPLONG(ptr[ix]);
}

 
void loadSaveState(int bsel, int bfill) //cleanup later
{
    int ix, ssize[16] = { 0, 32768, 65536, 131072, 131072, 512, 2048, 0, 262144, 0, 0, 0, 0, 0, 0, 0 };
    char *sext[16] = { 0, ".sra",  ".sra",  ".sra", ".fla", ".eep", ".eep", 0, ".sra", 0, 0, 0, 0, 0, 0, 0 };
    char temp[260];
    XCHAR wname[280];
    u64 flags;
    FIL lSDFile;
    UINT ts;
    selEntry_t entry;

    if (ssize[gTable[bsel].options[5] & 15] == 0)
        return; // ext cart, invalid, or off

    memcpy(&entry, &gTable[bsel], sizeof(selEntry_t)); // backup the entry

    memset(temp,'\0',260);

    strcpy(temp, entry.name);
    ix = strlen(temp)-1;
    while (ix && (temp[ix] != '.')) ix--;
    if (ix == 0)
        ix = strlen(temp);              // no extension - just add to end of name
    strcpy(&temp[ix], sext[entry.options[5]]);

    // check for SD card
    neo2_enable_sd();
    ix = getSDInfo(-1);             // try to get root

	flags = 0xAA550100LL | entry.options[5];

	if(ix)
	{
        c2wstrcpy(wname, "/menu/n64/save/last.run");
        f_force_open(&lSDFile,wname,FA_CREATE_ALWAYS | FA_WRITE,64);//f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE);
		f_write(&lSDFile,&flags,8,&ts);			
        c2wstrcpy(wname, "/menu/n64/save/");
        c2wstrcat(wname, temp);
        f_write(&lSDFile,wname,280*2,&ts); //waste space for alignment porpuses
        f_close(&lSDFile);
		memset(&lSDFile,0,sizeof(FIL));
	}
	else
	{
		neo_copyto_sram(temp, 0x3FE00, 256);
		neo_copyto_sram(&flags, 0x3FF00, 8);
	}

    if (!ix)
    {
        // no SD card, use GBA SRAM
        u8 blk[32];
        int ix;
        int soffs[20] = { 0x00000, // one slot for 128KB FRAM games
                          0x20000, 0x28000, 0x30000, // three slots for 32KB SRAM games
                          0x38000, 0x38800, 0x39000, 0x39800, 0x3A000, 0x3A800, 0x3B000, 0x3B800, // 8 slots for 16Kb EEPROM games
                          0x3C000, 0x3C200, 0x3C400, 0x3C600, 0x3C800, 0x3CA00, 0x3CC00, 0x3CE00 }; // 8 slots for 4Kb EEPROM games

        neo_copyfrom_sram(blk, 0x3FF20, 32);
        for (ix=0; ix<20; ix++)
        {
            char temp2[256];
            neo_copyfrom_sram(temp2, 0x3E000 | (ix<<8), 256);
            if ((blk[ix] == 0xAA) && !strcmp(temp, temp2))
                break;                  // found match in gba sram directory
        }
        ix = selectGBASlot(ix, blk, entry.options[5], bfill); // have user select a slot

        // init save ram to 0x00 or 0xFF in case there's no save file, or only a partial file
        memset(tmpBuf, (entry.options[5] == 4) ? 0xFF : 0x00, ssize[entry.options[5]]);

        // copy from valid slot in GBA SRAM
        if ((entry.options[5] == 4) && (blk[ix] == 0xAA))
        {
            // copy FRAM in two segments
            neo_copyfrom_sram(tmpBuf, soffs[ix], 65536);
            neo_copyfrom_sram(&tmpBuf[65536], soffs[ix]+65536, 65536);
            // "fix" for sram quirk
            neo_copyfrom_sram(tmpBuf, 0x3FFF0, 16);
        }
        else if (blk[ix] == 0xAA)
            neo_copyfrom_sram(tmpBuf, soffs[ix], ssize[entry.options[5]]);

        blk[ix] = 0xAA;
        neo_copyto_sram(blk, 0x3FF20, 32);
        neo_copyto_sram(temp, 0x3E000 | (ix<<8), 256);

        neo2_disable_sd();
        neo2_enable_sd();
        getSDInfo(-1);
    }
    else
    {
        // init save ram to 0x00 or 0xFF in case there's no save file, or only a partial file
        memset(tmpBuf, (entry.options[5] == 4) ? 0xFF : 0x00, ssize[entry.options[5]]);

        if(f_open(&lSDFile, wname, FA_OPEN_EXISTING | FA_READ) == FR_OK)
        {
            f_read(&lSDFile, tmpBuf, ssize[flags & 0xf], &ts);
            f_close(&lSDFile);
        }
    }
    neo2_disable_sd();

    switch(flags&15)
    {
        case 1:
        case 2:
        case 3:
            byte_swap_buf(tmpBuf, ssize[entry.options[5]]);
            neo_copyto_nsram(tmpBuf, 0, ssize[entry.options[5]], 8);
		break;

        case 4:
            byte_swap_buf(tmpBuf, ssize[entry.options[5]]);
            neo_copyto_nsram(tmpBuf, 0, ssize[entry.options[5]], 8);
		break;

        case 5:
        case 6:
            neo_copyto_eeprom(tmpBuf, 0, ssize[entry.options[5]], entry.options[5]);
		break;

        case 8:
            byte_swap_buf(tmpBuf, ssize[entry.options[5]]);
            neo_copyto_nsram(tmpBuf, 0, ssize[entry.options[5]], 8);
		break;
    }

    memcpy(&gTable[bsel], &entry, sizeof(selEntry_t)); // restore the entry
}

void saveSaveState()
{
    int ssize[16] = { 0, 32768, 65536, 131072, 131072, 512, 2048, 0, 262144, 0, 0, 0, 0, 0, 0, 0 };
    char temp[256];
    XCHAR wname[280];
	XCHAR wname2[280];
    u64 flags;
    FIL in,out;
    UINT ts;

    flags = 0;
	if(gSdDetected)
	{
        c2wstrcpy(wname2, "/menu/n64/save/last.run");
        if(f_open(&in, wname2, FA_OPEN_EXISTING | FA_READ) == FR_OK)
		{
			f_read(&in,&flags,8,&ts);	
            f_read(&in,wname,280*2,&ts);	
			f_close(&in);
		}
		else
		{
			//oops
			return;
		}
	}
	else
	{
		neo_copyfrom_sram(temp, 0x3FE00, 256);
		neo_copyfrom_sram(&flags, 0x3FF00, 8);
		neo_copyfrom_sram(&back_flags, 0x3FF08, 8);

		if ((flags & 0xFFFFFF00) != 0xAA550100)
		{
		    neo2_enable_sd();
		    getSDInfo(-1);
		    neo2_disable_sd();
		    return; // auto-save sram not needed
		}
	}

    switch(flags & 15)
    {
        case 1:
        case 2:
        case 3:
            neo_copyfrom_nsram(tmpBuf, 0, ssize[flags & 15], 8);
            byte_swap_buf(tmpBuf, ssize[flags & 15]);
            break;
        case 4:
            neo_copyfrom_nsram(tmpBuf, 0, ssize[flags & 15], 8);
            byte_swap_buf(tmpBuf, ssize[flags & 15]);
            break;
        case 5:
        case 6:
            neo_copyfrom_eeprom(tmpBuf, 0, ssize[flags & 15], flags & 15);
            break;
        case 8:
            neo_copyfrom_nsram(tmpBuf, 0, ssize[flags & 15], 8);
            byte_swap_buf(tmpBuf, ssize[flags & 15]);
            break;
    }

    if(gSdDetected)
    {
        if(f_force_open(&out,wname,FA_CREATE_ALWAYS | FA_WRITE,64) == 0)
		{
		    char temp[512];

			w2cstrcpy(temp,wname);
			debugText("Couldn't open file: ", 2, 2, 0);
			debugText(temp, 4, 8, 500);
			return;
		}

        f_write(&out,tmpBuf,ssize[flags & 15], &ts);
        f_close(&out);
		f_unlink(wname2);
        neo2_disable_sd();
        neo2_enable_sd();
        getSDInfo(-1);
    }
	else
    {
        // no SD card, use GBA SRAM
        u8 blk[32];
        int ix;
        int soffs[20] = { 0x00000, // one slot for 128KB FRAM games
                          0x20000, 0x28000, 0x30000, // three slots for 32KB SRAM games
                          0x38000, 0x38800, 0x39000, 0x39800, 0x3A000, 0x3A800, 0x3B000, 0x3B800, // 8 slots for 16Kb EEPROM games
                          0x3C000, 0x3C200, 0x3C400, 0x3C600, 0x3C800, 0x3CA00, 0x3CC00, 0x3CE00 }; // 8 slots for 4Kb EEPROM games

        neo_copyfrom_sram(blk, 0x3FF20, 32);
        for (ix=0; ix<20; ix++)
        {
            char temp2[256];
            neo_copyfrom_sram(temp2, 0x3E000 | (ix<<8), 256);
            if ((blk[ix] == 0xAA) && !strcmp(temp, temp2))
                break;                  // found match in gba sram directory
        }
        if (ix == 20)
        {
            // turn off auto-save
            flags = 0;
            neo_copyto_sram(&flags, 0x3FF00, 8);
            return;                     // no matching entry
        }

        // copy to slot in GBA SRAM
        if ((flags & 15) == 4)
        {
            // copy FRAM in two segments
            neo_copyto_sram(tmpBuf, soffs[ix], 65536);
            neo_copyto_sram(&tmpBuf[65536], soffs[ix]+65536, 65536);
            // "fix" for sram quirk
            neo_copyto_sram(tmpBuf, 0x3FFF0, 16);
        }
        else
            neo_copyto_sram(tmpBuf, soffs[ix], ssize[flags & 15]);

		// turn off auto-save
		flags = 0;
		neo_copyto_sram(&flags, 0x3FF00, 8);
    }
}

void saveBrowserFlags(int brwsr, int bopt, int bfill)
{
    back_flags = 0xAA550000LL | (brwsr<<8) | (bopt<<4) | bfill;
    neo_copyto_sram(&back_flags, 0x3FF08, 8);

    if (brwsr)
        neo2_enable_sd();               // make sure still in proper mode for SD
}

int getMPInfo(int bcon)
{
    int bmax, ix;

    strcpy(gTable[0].name, "4Kb EEPROM on Ext Card");
    gTable[0].valid = 1;
    strcpy(gTable[1].name, "16Kb EEPROM on Ext Card");
    gTable[1].valid = 1;
    strcpy(gTable[2].name, "32KB SRAM on Ext Card");
    gTable[2].valid = 1;
    strcpy(gTable[3].name, "128KB FRAM on Ext Card");
    gTable[3].valid = 1;
    bmax = 4;
    gTable[bmax].valid = 0;

    // add saves on current mempak here
    ix = get_controllers_present();
    if (((bcon == 0) && !(ix & CONTROLLER_1_INSERTED)) ||
        ((bcon == 1) && !(ix & CONTROLLER_2_INSERTED)) ||
        ((bcon == 2) && !(ix & CONTROLLER_3_INSERTED)) ||
        ((bcon == 3) && !(ix & CONTROLLER_4_INSERTED)))
    {
        gTable[bmax].valid = 0;
        return bmax;
    }

    get_accessories_present();
    if (identify_accessory(bcon) == ACCESSORY_MEMPAK)
    {
        if (validate_mempak(bcon))
        {
            // not formatted or some other error
            strcpy(gTable[bmax].name, "Raw MemPak");
            gTable[bmax].valid = 1;
            bmax++;
            strcpy(gTable[bmax].name, "Format MemPak");
            gTable[bmax].valid = 1;
            bmax++;
        }
        else
        {
            strcpy(gTable[bmax].name, "Raw MemPak");
            gTable[bmax].valid = 1;
            bmax++;
            for (ix=0; ix<16; ix++)
            {
                entry_structure_t entry;

                get_mempak_entry(bcon, ix, &entry );

                if (entry.valid)
                {
                    strcpy(gTable[bmax].name, entry.name);
                    gTable[bmax].valid = 1;
                    gTable[bmax].type = entry.blocks;
                }
                else
                {
                    strcpy(gTable[bmax].name, "Empty Slot");
                    gTable[bmax].valid = 1;
                    gTable[bmax].type = get_mempak_free_space(bcon);
                }
                bmax++;
            }
        }
        gTable[bmax].valid = 0;
    }
    return bmax;
}

void doSaveMgr(int bfill)
{
    display_context_t dcon;
    u16 previous = 0, buttons = 0;
    int bselect = 0, bstart = 0, bmax = 0, bcon = 0;
    FIL lSDFile;
    UINT ts;
    char temp[44];
    XCHAR wname[40];

    char *save_title = "Save Manager";
    char *save_help1 = "A=Save to SD          B=Load from SD";
    char *save_help2 = "DPad=Navigate    LT/RT=Change MemPak";
    char *save_help3 = "Z=                        START=Exit";

    bmax = getMPInfo(bcon);

    while (1)
    {
        // get next buffer to draw in
        dcon = lockVideo(1);
        graphics_fill_screen(dcon, 0);

        if (browser && (bfill == 4))
        {
            drawImage(dcon, browser);
        }
        else if ((bfill < 3) && (pattern[bfill] != NULL))
        {
            rdp_sync(SYNC_PIPE);
            rdp_set_default_clipping();
            rdp_enable_texture_copy();
            rdp_attach_display(dcon);
            // Draw pattern
            rdp_sync(SYNC_PIPE);
            rdp_load_texture(0, 0, MIRROR_DISABLED, pattern[bfill]);
            for (int j=0; j<240; j+=pattern[bfill]->height)
                for (int i=0; i<320; i+=pattern[bfill]->width)
                    rdp_draw_sprite(0, i, j);
            rdp_detach_display();
        }

        // show title
        graphics_set_color(gTextColors.title, 0);
        printText(dcon, save_title, 20 - strlen(save_title)/2, 1);

        // print save list (lines 3 to 12)
        if (bmax)
        {
            for (int ix=0; ix<10; ix++)
            {
                if ((bstart+ix) == 1024)
                    break; // end of table
                if (gTable[bstart+ix].valid)
                {
                    strncpy(temp, gTable[bstart+ix].name, 36);
                    temp[36] = 0;
                    graphics_set_color((bstart+ix) == bselect ? gTextColors.sel_game : gTextColors.usel_game, 0);
                    printText(dcon, temp, 20 - strlen(temp)/2, 3 + ix);
                }
                else
                    break; // stop on invalid entry
            }
        }

        // show help messages
        graphics_set_color(gTextColors.help_info, 0);
        printText(dcon, save_help1, 20 - strlen(save_help1)/2, 25);
        printText(dcon, save_help2, 20 - strlen(save_help2)/2, 26);
        printText(dcon, save_help3, 20 - strlen(save_help3)/2, 27);

        // show display
        unlockVideo(dcon);

        // get buttons
        previous = buttons;
        buttons = getButtons(0);

        if (DU_BUTTON(buttons))
        {
            // UP pressed, go one entry back
            bselect--;
            if (bselect < 0)
            {
                bselect = bmax - 1;
                bstart = bmax - (bmax % 10);
            }
            if (bselect < bstart)
            {
                bstart -= 10;
                if (bstart < 0)
                    bstart = 0;
            }
            delay(7);
        }
        if (DL_BUTTON(buttons))
        {
            // LEFT pressed, go one page back
            bselect -= 10;
            if (bselect < 0)
            {
                bselect = bmax - 1;
                bstart = bmax - (bmax % 10);
            }
            else
            {
                bstart -= 10;
                if (bstart < 0)
                    bstart = 0;
            }
            delay(15);
        }
        if (DD_BUTTON(buttons))
        {
            // DOWN pressed, go one entry forward
            bselect++;
            if (bselect == bmax)
            {
                bselect = bstart = 0;
            }
            else if ((bselect - bstart) == 10)
            {
                bstart += 10;
            }
            delay(7);
        }
        if (DR_BUTTON(buttons))
        {
            // RIGHT pressed, go one page forward
            bselect += 10;
            if (bselect >= bmax)
            {
                bselect = bstart = 0;
            }
            else
            {
                bstart += 10;
            }
            delay(15);
        }

        if (START_BUTTON(buttons ^ previous))
        {
            // START changed
            if (!START_BUTTON(buttons))
            {
                // START just released
                return;                 // exit back to SD browser
            }
        }

        if (Z_BUTTON(buttons ^ previous))
        {
            // Z changed
            if (!Z_BUTTON(buttons))
            {
                // Z just released

            }
        }

        if (A_BUTTON(buttons ^ previous) && bmax)
        {
            // A changed
            if (!A_BUTTON(buttons))
            {
                // A just released - save ext cart save to SD
                if (bselect == 0)
                {
                    // save 4Kb EEPROM
                    neo_copyfrom_eeprom(tmpBuf, 0, 512, 0);
                    neo2_enable_sd();
                    bmax = getSDInfo(-1);
                    if (!doOSKeyb("Enter Save Name", "Save-4Kb.eep", temp, 40, bfill))
                    {
                        bmax = getMPInfo(bcon);
                        continue;
                    }
                    c2wstrcpy(wname, "/menu/n64/save/");
                    c2wstrcat(wname, temp);
                    if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                    {
                        progress_screen("Saving 4Kb EEPROM", temp, 0, 100, bfill);
                        f_write(&lSDFile, tmpBuf, 512, &ts);
                        progress_screen(NULL, NULL, 100, 100, -1);
                        f_close(&lSDFile);
                        delay(30);
                    }
                    bmax = getMPInfo(bcon);
                }
                else if (bselect == 1)
                {
                    // save 16Kb EEPROM
                    neo_copyfrom_eeprom(tmpBuf, 0, 2048, 0);
                    neo2_enable_sd();
                    bmax = getSDInfo(-1);
                    if (!doOSKeyb("Enter Save Name", "Save-16Kb.eep", temp, 40, bfill))
                    {
                        bmax = getMPInfo(bcon);
                        continue;
                    }
                    c2wstrcpy(wname, "/menu/n64/save/");
                    c2wstrcat(wname, temp);
                    if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                    {
                        progress_screen("Saving 16Kb EEPROM", temp, 0, 100, bfill);
                        f_write(&lSDFile, tmpBuf, 2048, &ts);
                        progress_screen(NULL, NULL, 100, 100, -1);
                        f_close(&lSDFile);
                        delay(30);
                    }
                    bmax = getMPInfo(bcon);
                }
                else if (bselect == 2)
                {
                    int i;
                    // save 32KB SRAM
                    neo_copyfrom_nsram(tmpBuf, 0, 32768, 0);
                    byte_swap_buf(tmpBuf, 32768);
                    neo2_enable_sd();
                    bmax = getSDInfo(-1);
                    if (!doOSKeyb("Enter Save Name", "Save-32KB.sra", temp, 40, bfill))
                    {
                        bmax = getMPInfo(bcon);
                        continue;
                    }
                    c2wstrcpy(wname, "/menu/n64/save/");
                    c2wstrcat(wname, temp);
                    if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                    {
                        progress_screen("Saving 32KB SRAM", temp, 0, 100, bfill);
                        for (i=0; i<32768; i+=4096)
                        {
                            f_write(&lSDFile, &tmpBuf[i], 4096, &ts);
                            progress_screen(NULL, NULL, i*100/32768, 100, -1);
                        }
                        progress_screen(NULL, NULL, 100, 100, -1);
                        f_close(&lSDFile);
                        delay(30);
                    }
                    bmax = getMPInfo(bcon);
                }
                else if (bselect == 3)
                {
                    // save 128KB FRAM
                    int i;
                    neo_copyfrom_fram(tmpBuf, 0, 131072, 0); // 0 = EXT, 4 = FRAM
                    byte_swap_buf(tmpBuf, 131072);
                    neo2_enable_sd();
                    bmax = getSDInfo(-1);
                    if (!doOSKeyb("Enter Save Name", "Save-128KB.fla", temp, 40, bfill))
                    {
                        bmax = getMPInfo(bcon);
                        continue;
                    }
                    c2wstrcpy(wname, "/menu/n64/save/");
                    c2wstrcat(wname, temp);
                    if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                    {
                        progress_screen("Saving 128KB FRAM", temp, 0, 100, bfill);
                        for (i=0; i<131072; i+=4096)
                        {
                            f_write(&lSDFile, &tmpBuf[i], 4096, &ts);
                            progress_screen(NULL, NULL, i*100/131072, 100, -1);
                        }
                        progress_screen(NULL, NULL, 100, 100, -1);
                        f_close(&lSDFile);
                        delay(30);
                    }
                    bmax = getMPInfo(bcon);
                }
                else if (bselect == 4)
                {
                    // save 32KB raw mempak
                    int i;
                    for (i=0; i<128; i++)
                        read_mempak_sector(bcon, i, &tmpBuf[i*256]);
                    if (!doOSKeyb("Enter Save Name", "Save-32KB.mpk", temp, 40, bfill))
                    {
                        bmax = getMPInfo(bcon);
                        continue;
                    }
                    c2wstrcpy(wname, "/menu/n64/save/");
                    c2wstrcat(wname, temp);
                    if (f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                    {
                        progress_screen("Saving 32KB MemPak", temp, 0, 100, bfill);
                        for (i=0; i<128; i++)
                        {
                            f_write(&lSDFile, &tmpBuf[i*256], 256, &ts);
                            progress_screen(NULL, NULL, i*100/128, 100, -1);
                        }
                        progress_screen(NULL, NULL, 100, 100, -1);
                        f_close(&lSDFile);
                        delay(30);
                    }
                    bmax = getMPInfo(bcon);
                }
                else if ((bselect == 5) && (bmax == 6))
                {
                    // format mempak
                    progress_screen("Formatting", "MemPak", 0, 100, bfill);
                    format_mempak(bcon);
                    progress_screen(NULL, NULL, 100, 100, -1);
                    delay(30);
                    bmax = getMPInfo(bcon);
                }
                else
                {
                    // save MemPak entry to SD

                }
            }
        }
        if (B_BUTTON(buttons ^ previous) && bmax)
        {
            // B changed
            if (!B_BUTTON(buttons))
            {
                // B just released - load ext cart save from SD
                if (bselect == 0)
                {
                    // load 4Kb EEPROM
                }
                else if (bselect == 1)
                {
                    // load 16Kb EEPROM
                }
                else if (bselect == 2)
                {
                    // load 32KB SRAM
                }
                else if (bselect == 3)
                {
                    // load 128KB FRAM
                }
                else if (bselect == 4)
                {
                    // load 32KB raw mempak
                    int i, j;
                    j = RequestFile("Select MemPak file to load", "/menu/n64/save", bfill);
                    if (j == -1)
                    {
                        bmax = getMPInfo(bcon);
                        continue;
                    }
                    c2wstrcpy(wname, path);
                    if (path[strlen(path)-1] != '/')
                        c2wstrcat(wname, "/");
                    c2wstrcat(wname, gTable[j].name);
                    if (f_open(&lSDFile, wname, FA_OPEN_EXISTING | FA_READ) == FR_OK)
                    {
                        // handle DexDrive .n64 file
                        if (lSDFile.fsize > 32768)
                            f_lseek(&lSDFile, lSDFile.fsize - 32768);
                        progress_screen("Loading 32KB MemPak", gTable[j].name, 0, 100, bfill);
                        for (i=0; i<128; i++)
                        {
                            f_read(&lSDFile, &tmpBuf[i*256], 256, &ts);
                            progress_screen(NULL, NULL, i*100/128, 100, -1);
                        }
                        progress_screen(NULL, NULL, 100, 100, -1);
                        f_close(&lSDFile);
                        for (i=0; i<128; i++)
                            write_mempak_sector(bcon, i, &tmpBuf[i*256]);
                    }
                    bmax = getMPInfo(bcon);
                }
                else if ((bselect == 5) && (bmax == 6))
                {
                    // format mempak
                    progress_screen("Formatting", "MemPak", 0, 100, bfill);
                    format_mempak(bcon);
                    progress_screen(NULL, NULL, 100, 100, -1);
                    delay(30);
                    bmax = getMPInfo(bcon);
                }
                else
                {
                    // load MemPak entry from SD

                }
            }
        }

        if (TL_BUTTON(buttons ^ previous))
        {
            // TL changed
            if (!TL_BUTTON(buttons))
            {
                // TL just released
                bcon = (bcon == 0) ? 3 : bcon-1;
                bmax = getMPInfo(bcon);
            }
        }

        if (TR_BUTTON(buttons ^ previous))
        {
            // TR changed
            if (!TR_BUTTON(buttons))
            {
                // TR just released
                bcon = (bcon == 3) ? 0 : bcon+1;
                bmax = getMPInfo(bcon);
            }
        }
    }
}

void setTextColors(int bfill)
{
    if (bfill < 4)
    {
        // patterns
        gTextColors.title = graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF);
        gTextColors.usel_game = graphics_make_color(0x3F, 0x8F, 0x3F, 0xFF);
        gTextColors.sel_game = graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF);
        gTextColors.uswp_info = graphics_make_color(0x3F, 0xFF, 0x3F, 0xFF);
        gTextColors.swp_info = graphics_make_color(0x5F, 0x5F, 0xFF, 0xFF);
        gTextColors.usel_option = graphics_make_color(0x3F, 0xFF, 0x3F, 0xFF);
        gTextColors.sel_option = graphics_make_color(0xBF, 0x3F, 0x3F, 0xFF);
        gTextColors.hw_info = graphics_make_color(0xFF, 0x3F, 0x3F, 0xFF);
        gTextColors.help_info = graphics_make_color(0x3F, 0xFF, 0x3F, 0xFF);
    }
    else
    {
        // images
        gTextColors.title = graphics_make_color(0x3F, 0x3F, 0x9F, 0xFF);
        gTextColors.usel_game = graphics_make_color(0x3F, 0x8F, 0x3F, 0xFF);
        gTextColors.sel_game = graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF);
        gTextColors.uswp_info = graphics_make_color(0x3F, 0xBF, 0x3F, 0xFF);
        gTextColors.swp_info = graphics_make_color(0x3F, 0x3F, 0xBF, 0xFF);
        gTextColors.usel_option = graphics_make_color(0x3F, 0xBF, 0x3F, 0xFF);
        gTextColors.sel_option = graphics_make_color(0xBF, 0x3F, 0x3F, 0xFF);
        gTextColors.hw_info = graphics_make_color(0x5F, 0x1F, 0x1F, 0xFF);
        gTextColors.help_info = graphics_make_color(0x1F, 0x7F, 0x1F, 0xFF);
    }
}

/* initialize console hardware */
void init_n64(void)
{
    // enable MI interrupts (on the CPU)
    set_MI_interrupt(1,1);

    // Initialize display
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
    rdp_init();
   // rdp_set_texture_flush(FLUSH_STRATEGY_NONE);
    register_VI_handler(vblCallback);

    // Initialize controllers
    controller_init();

    // Initialize rom filesystem
    if (dfs_init(0xB0101000) == DFS_ESUCCESS)
    {
        // load pattern sprites
        int fp = dfs_open("/pattern0.sprite");
        pattern[0] = malloc(dfs_size(fp));
        dfs_read(pattern[0], 1, dfs_size(fp), fp);
        /* Invalidate data associated with sprite in cache */
        data_cache_writeback_invalidate( pattern[0], dfs_size(fp) );
        dfs_close(fp);
        fp = dfs_open("/pattern1.sprite");
        pattern[1] = malloc(dfs_size(fp));
        dfs_read(pattern[1], 1, dfs_size(fp), fp);
        /* Invalidate data associated with sprite in cache */
        data_cache_writeback_invalidate( pattern[1], dfs_size(fp) );
        dfs_close(fp);
        fp = dfs_open("/pattern2.sprite");
        pattern[2] = malloc(dfs_size(fp));
        dfs_read(pattern[2], 1, dfs_size(fp), fp);
        /* Invalidate data associated with sprite in cache */
        data_cache_writeback_invalidate( pattern[2], dfs_size(fp) );
        dfs_close(fp);
        // load unknown boxart sprite
        fp = dfs_open("/unknown.sprite");
        dfs_read(unknown, 1, 14984, fp);
        dfs_close(fp);
        // load backdrop images
        splash = loadImageDFS("/splash.png");
        if (!splash)
            splash = loadImageDFS("/splash.jpg");
        browser = loadImageDFS("/browser.png");
        if (!browser)
            browser = loadImageDFS("/browser.jpg");
        loading = loadImageDFS("/loading.png");
        if (!loading)
            loading = loadImageDFS("/loading.jpg");
    }
}

/* main code entry point */
int main(void)
{
    display_context_t dcon;
    u16 previous = 0, buttons;
    int bfill = 4, bselect = 0, bstart = 0, bmax = 0, btout = 60, bopt = 0, osel = 0;
#ifdef RUN_FROM_SD
    int brwsr = 1;                      // start in SD browser
#else
    int brwsr = 0;                      // start in game flash browser
#endif
    char temp[128];
#if defined RUN_FROM_U2
    char *menu_title = "Neo N64 Myth Menu v2.4 (U2)";
#elif defined RUN_FROM_SD
    char *menu_title = "Neo N64 Myth Menu v2.4 (SD)";
#else
    char *menu_title = "Neo N64 Myth Menu v2.4 (MF)";
#endif
    char *menu_help1 = "A=Run reset to menu  B=Reset to game";
    char *menu_help2 = "DPad=Navigate     CPad=change option";
    char *menu_help3[4] = { "Z=Show Options      START=SD browser",
                            "Z=Show ROM Info     START=SD browser",
                            "Z=Show Options   START=Flash browser",
                            "Z=Show ROM Info  START=Flash browser" };
    char cards[3] = { 'C', 'B', 'A' };
    int onext[2][16] = {
        { 1, 2, 3, 4, 5, 6, 8, 0, 15, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    int oprev[2][16] = {
        { 15, 0, 1, 2, 3, 4, 5, 0, 6, 0, 0, 0, 0, 0, 0, 8 },
        { 6, 0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    char *ostr[2][16] = {
        { "EXT CARD", "SRAM 32KB", "SRAM 64KB", "SRAM 128KB", "FRAM 128KB", "EEPROM 4Kb", "EEPROM 16Kb", "", "SRAM 256KB", "", "", "", "", "", "", "SAVE OFF" },
        { "EXT CARD", "6101/7102", "6102/7101", "6103/7103", "6104/7104", "6105/7105", "6106/7106", "", "", "", "", "", "", "", "", "" }
    };

    sprite_t *stemp;

    init_n64();
	gSdDetected = 0;
	disk_io_force_wdl = 0;
    gBootCic = get_cic((unsigned char *)0xB0000040);
    gCpldVers = neo_get_cpld();         // get Myth CPLD ID
    gCardID = neo_id_card();            // check for Neo Flash card

    neo_select_menu();                  // enable menu flash in cart space
    gCardType = *(vu32 *)(0xB01FFFF0) >> 16;
    switch(gCardType & 0x00FF)
    {
        case 0:
        gCardTypeCmd = 0x00DAAE44;      // set iosr = enable game flash for newest cards
        gPsramCmd = 0x00DAAF44;         // set iosr for Neo2-SD psram
        break;
        case 1:
        gCardTypeCmd = 0x00DA8E44;      // set iosr = enable game flash for new cards
        gPsramCmd = 0x00DA674E;         // set iosr for Neo2-Pro psram
        break;
        case 2:
        gCardTypeCmd = 0x00DA0E44;      // set iosr = enable game flash for old cards
        gPsramCmd = 0x00DA0F44;         // set iosr for psram
        break;
        default:
        gCardTypeCmd = 0x00DAAE44;      // set iosr = enable game flash for newest cards
        gPsramCmd = 0x00DAAF44;         // set iosr for psram
        break;
    }
    neo_select_game();


#ifdef RUN_FROM_U2
    // check for boot rom in menu
    neo_select_menu();                  // enable menu flash in cart space

    if (!memcmp((void *)0xB0000020, "N64 Myth Menu (MF)", 18))
        neo_run_menu();
#endif

	disk_io_set_mode(0,0);

#ifndef RUN_FROM_SD
    // check for boot rom on SD card
    neo2_enable_sd();
    bmax = getSDInfo(-1);               // get root directory of sd card
    if (bmax)
    {
        int load_ex_menu = 0;
        FIL lSDFile;
        XCHAR fpath[32];

        check_fast();                   // let SD read at full speed if allowed

        strcpy(path, "/menu/n64/");
        strcpy(gTable[0].name, "NEON64SD.z64");
        c2wstrcpy(fpath, path);
        c2wstrcat(fpath, gTable[0].name);

        load_ex_menu = (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) == FR_OK);

        if(load_ex_menu == 0)
        {
            memset(&lSDFile,0,sizeof(FIL));
            strcpy(path, "/menu/n64/");
            strcpy(gTable[0].name, "NEON64SD.v64");
            c2wstrcpy(fpath, path);
            c2wstrcat(fpath, gTable[0].name);
            load_ex_menu = (f_open(&lSDFile, fpath, FA_OPEN_EXISTING | FA_READ) == FR_OK);
        }

        if(load_ex_menu)
        {
            gTable[0].valid = 1;
            gTable[0].type = 127;
            gTable[0].options[0] = 0xFF;
            gTable[0].options[1] = 0;
            gTable[0].options[2] = 0;
            gTable[0].options[3] = 0;
            gTable[0].options[4] = (lSDFile.fsize > 2097152) ? lSDFile.fsize/131072 : 16;
            gTable[0].options[5] = 5;
            gTable[0].options[6] = 2;
            gTable[0].options[7] = 0;
            f_close(&lSDFile);
            get_sd_info(0);

            stemp = loadImageSD("/menu/n64/images/loading.png");
            if (!stemp)
                stemp = loadImageSD("/menu/n64/images/loading.jpg");
            if (stemp)
            {
                if (loading)
                    free(loading);
                loading = stemp;
            }

            setTextColors((loading) ? 4 : 0);
            copySD2Psram(0, (loading) ? 4 : 0);

            neo_run_psram(gTable[0].options, 1);
        }
        neo2_disable_sd();
    }
#endif

	if(gSdDetected == 0)
	{
		neo2_enable_sd();
		bmax = getSDInfo(-1);
	}

    if (bmax)
    {        
        check_fast();                   // let SD read at full speed if allowed

        // try for images on the SD card
        stemp = loadImageSD("/menu/n64/images/splash.png");
        if (!stemp)
            stemp = loadImageSD("/menu/n64/images/splash.jpg");
        if (stemp)
        {
            if (splash)
                free(splash);
            splash = stemp;
        }
        stemp = loadImageSD("/menu/n64/images/browser.png");
        if (!stemp)
            stemp = loadImageSD("/menu/n64/images/browser.jpg");
        if (stemp)
        {
            if (browser)
                free(browser);
            browser = stemp;
        }
        stemp = loadImageSD("/menu/n64/images/loading.png");
        if (!stemp)
            stemp = loadImageSD("/menu/n64/images/loading.jpg");
        if (stemp)
        {
            if (loading)
                free(loading);
            loading = stemp;
        }
    }

    if (splash)
    {
        display_context_t dcon;
        dcon = lockVideo(1);
        graphics_fill_screen(dcon, 0);
        drawImage(dcon, splash);
        unlockVideo(dcon);
        delay(120);
    }

	saveSaveState();
    neo_select_game();

    if ((back_flags & 0xFFFFF000) == 0xAA550000)
    {
        brwsr = (back_flags >> 8) & 1;
        bopt = (back_flags >> 4) & 1;
        bfill = back_flags & 7;
    }

    setTextColors(bfill);

    memset(boxes, 0xFF, 16 * sizeof(unsigned short)); // clear boxart cache
    if (brwsr)
    {
        neo2_enable_sd();
        bmax = getSDInfo(-1);           // preload SD root dir
        btout = 60;
    }
    else
        bmax = getGFInfo();             // preload flash menu entries

    config_init();
    config_load("/menu/n64/config.cfg");
    sys_set_boot_cic(6102);
    ints_setup();

    while (1)
    {
        // check if need to fetch SD file info
        if (btout)
            btout--;
        else if (gTable[bselect].type == 127)
            get_sd_info(bselect);

        // get next buffer to draw in
        dcon = lockVideo(1);
        graphics_fill_screen(dcon, 0);

        if (browser && (bfill == 4))
        {
            drawImage(dcon, browser);
        }
        else if ((bfill < 3) && (pattern[bfill] != NULL))
        {
            rdp_sync(SYNC_PIPE);
            rdp_set_default_clipping();
            rdp_enable_texture_copy();
            rdp_attach_display(dcon);
            // Draw pattern
            rdp_sync(SYNC_PIPE);
            rdp_load_texture(0, 0, MIRROR_DISABLED, pattern[bfill]);
            for (int j=0; j<240; j+=pattern[bfill]->height)
                for (int i=0; i<320; i+=pattern[bfill]->width)
                    rdp_draw_sprite(0, i, j);
            rdp_detach_display();
        }

        // show title
        graphics_set_color(gTextColors.title, 0);
        strcpy(temp, menu_title);
        if (*(vu32*)0x80000300 == 0)
        {
            // PAL
            strcpy(&temp[strlen(temp)-1], "/7101)");
            if (gBootCic == 1)
                temp[30] = '2';
            else if (gBootCic == 2)
                temp[30] = '1';
            else
                temp[30] = 0x30 + gBootCic;
        }
        else
        {
            // NTSC or MPAL
            strcpy(&temp[strlen(temp)-1], "/6102)");
            temp[30] = 0x30 + gBootCic;
        }
        printText(dcon, temp, 20 - strlen(temp)/2, 1);

        // print browser list (lines 3 to 12)
        if (bmax)
        {
            for (int ix=0; ix<10; ix++)
            {
                if ((bstart+ix) == 1024)
                    break; // end of table
                if (gTable[bstart+ix].valid)
                {
                    if (gTable[bstart+ix].type == 128)
                    {
                        // directory entry
                        strcpy(temp, "[");
                        strncat(temp, gTable[bstart+ix].name, 34);
                        temp[35] = 0;
                        strcat(temp, "]");
                    }
                    else
                        strncpy(temp, gTable[bstart+ix].name, 36);
                    temp[36] = 0;
                    graphics_set_color((bstart+ix) == bselect ? gTextColors.sel_game : gTextColors.usel_game, 0);
                    printText(dcon, temp, 20 - strlen(temp)/2, 3 + ix);
                }
                else
                    break; // stop on invalid entry
            }
        }

        if (bmax && gTable[bselect].valid)
        {
            if ((gTable[bselect].type == 255) && (bopt == 0))
            {
                graphics_set_color(gTable[bselect].swap ? gTextColors.swp_info : gTextColors.uswp_info, 0);
                // print selection info
                printText(dcon, (char *)gTable[bselect].rom, 2, 14);
                sprintf(temp, "Country: %02X (%c)", gTable[bselect].rom[0x1E], gTable[bselect].rom[0x1E]);
                printText(dcon, temp, 23, 14);
                sprintf(temp, "Manuf ID: %02X (%c)", gTable[bselect].rom[0x1B], gTable[bselect].rom[0x1B]);
                printText(dcon, temp, 2, 15);
                sprintf(temp, "Cart ID: %04X (%c%c)", gTable[bselect].rom[0x1C]<<8 | gTable[bselect].rom[0x1D], gTable[bselect].rom[0x1C], gTable[bselect].rom[0x1D]);
                printText(dcon, temp, 20, 15);

                graphics_set_color(gTextColors.title, 0);
                // print full selection name (lines 16 - 22)
                strncpy(temp, gTable[bselect].name, 36);
                temp[36] = 0;
                printText(dcon, temp, 20-strlen(temp)/2, 16);
                for (int ix=36, iy=17; ix<strlen(gTable[bselect].name); ix+=36, iy++)
                {
                    strncpy(temp, &gTable[bselect].name[ix], 36);
                    temp[36] = 0;
                    printText(dcon, temp, 20-strlen(temp)/2, iy);
                }
            }

            if ((gTable[bselect].type == 255) && (bopt == 1))
            {
                unsigned short cart = *(unsigned short *)&gTable[bselect].rom[0x1C];
                int ix = (gTable[bselect].rom[0x1C] ^ gTable[bselect].rom[0x1D]) & 15;

                graphics_set_color(gTable[bselect].swap ? gTextColors.swp_info : gTextColors.uswp_info, 0);
                // print selection options
                sprintf(temp, "Bank: %3d", (gTable[bselect].options[1]<<8) | gTable[bselect].options[2]);
                printText(dcon, temp, 4, 14);
                sprintf(temp, "Size: %3d", (gTable[bselect].options[3]<<8) | gTable[bselect].options[4]);
                printText(dcon, temp, 4, 15);
                graphics_set_color(osel ? gTextColors.usel_option : gTextColors.sel_option, 0);
                sprintf(temp, "Save: %s", ostr[0][gTable[bselect].options[5]]);
                printText(dcon, temp, 4, 16);
                graphics_set_color(osel ? gTextColors.sel_option : gTextColors.usel_option, 0);
                sprintf(temp, "CIC: %s", ostr[1][gTable[bselect].options[6]]);
                printText(dcon, temp, 4, 17);

                // do boxart
                if (boxes[ix] != cart)
                    get_boxart(brwsr, bselect);
                graphics_draw_sprite(dcon, 184, 112, (sprite_t*)&boxart[ix*14984]);
            }
        }

        // show cart info help messages
        graphics_set_color(gTextColors.hw_info, 0);
        sprintf(temp, "CPLD:V%d CART:0x%08X FLASH:%c", gCpldVers & 7, gCardID, cards[gCardType & 3]);
        printText(dcon, temp, 20 - strlen(temp)/2, 24);
        graphics_set_color(gTextColors.help_info, 0);
        printText(dcon, menu_help1, 20 - strlen(menu_help1)/2, 25);
        printText(dcon, menu_help2, 20 - strlen(menu_help2)/2, 26);
        printText(dcon, menu_help3[brwsr*2+bopt], 20 - strlen(menu_help3[brwsr*2+bopt])/2, 27);

        // show display
        unlockVideo(dcon);

        // get buttons
        buttons = getButtons(0);

        if (DU_BUTTON(buttons))
        {
            // UP pressed, go one entry back
            bselect--;
            if (bselect < 0)
            {
                bselect = bmax - 1;
                bstart = bmax - (bmax % 10);
            }
            if (bselect < bstart)
            {
                bstart -= 10;
                if (bstart < 0)
                    bstart = 0;
            }
            btout = 60;
            delay(7);
        }
        if (DL_BUTTON(buttons))
        {
            // LEFT pressed, go one page back
            bselect -= 10;
            if (bselect < 0)
            {
                bselect = bmax - 1;
                bstart = bmax - (bmax % 10);
            }
            else
            {
                bstart -= 10;
                if (bstart < 0)
                    bstart = 0;
            }
            btout = 60;
            delay(15);
        }
        if (DD_BUTTON(buttons))
        {
            // DOWN pressed, go one entry forward
            bselect++;
            if (bselect == bmax)
            {
                bselect = bstart = 0;
            }
            else if ((bselect - bstart) == 10)
            {
                bstart += 10;
            }
            btout = 60;
            delay(7);
        }
        if (DR_BUTTON(buttons))
        {
            // RIGHT pressed, go one page forward
            bselect += 10;
            if (bselect >= bmax)
            {
                bselect = bstart = 0;
            }
            else
            {
                bstart += 10;
            }
            btout = 60;
            delay(15);
        }

        if (START_BUTTON(buttons ^ previous))
        {
            // START changed
            if (!START_BUTTON(buttons))
            {
                // START just released
                memset(boxes, 0xFF, 16 * sizeof(unsigned short)); // clear boxart cache
                bselect = bstart = 0;
                brwsr ^= 1;             // toggle browser mode
                if (brwsr)
                {
                    // browse SD card
                    neo2_enable_sd();
                    bmax = getSDInfo(-1);
                }
                else
                {
                    // browse gba game flash
                    neo2_disable_sd();
                    bmax = getGFInfo();
                }
                saveBrowserFlags(brwsr, bopt, bfill);
            }
            btout = 60;
        }

        if (Z_BUTTON(buttons ^ previous))
        {
            // Z changed
            if (!Z_BUTTON(buttons))
            {
                // Z just released
                bopt ^= 1;              // toggle options display
                saveBrowserFlags(brwsr, bopt, bfill);
            }
        }

        if (A_BUTTON(buttons ^ previous) && bmax)
        {
            // A changed
            if (!A_BUTTON(buttons))
            {
                // A just released - run with reset to menu
                if (brwsr)
                {
                    if (gTable[bselect].type == 128)
                    {
                        // enter sub-directory
                        bmax = getSDInfo(bselect);
                        bselect = bstart = 0;
                    }
                    else
                    {
                        if (gTable[bselect].type == 127)
                        {
                            // load rom info if not already loaded
                            get_sd_info(bselect);
                        }
                        if (gTable[bselect].type == 254)
                            loadNES2Psram(bselect, bfill);
                        else
                            copySD2Psram(bselect, bfill);
                        loadSaveState(bselect, bfill);
                        neo_run_psram(gTable[bselect].options, 1);
                    }
                }
                else
                {
                    if (!gTable[bselect].swap)
                    {
                        loadSaveState(bselect, bfill);
                        neo_run_game(gTable[bselect].options, 1);
                    }
                    else
                    {
                        copyGF2Psram(bselect, bfill);
                        loadSaveState(bselect, bfill);
                        neo_run_psram(gTable[bselect].options, 1);
                    }
                }
            }
        }
        if (B_BUTTON(buttons ^ previous) && bmax)
        {
            // B changed
            if (!B_BUTTON(buttons))
            {
                // B just released - run with reset to game
                if (brwsr)
                {
                    if (gTable[bselect].type == 128)
                    {
                        // enter sub-directory
                        bmax = getSDInfo(bselect);
                        bselect = bstart = 0;
                    }
                    else
                    {
                        if (gTable[bselect].type == 127)
                        {
                            // load rom info if not already loaded
                            get_sd_info(bselect);
                        }
                        if (gTable[bselect].type == 254)
                            loadNES2Psram(bselect, bfill);
                        else
                            copySD2Psram(bselect, bfill);
                        loadSaveState(bselect, bfill);
                        neo_run_psram(gTable[bselect].options, 0);
                    }
                }
                else
                {
                    if (!gTable[bselect].swap)
                    {
                        loadSaveState(bselect, bfill);
                        neo_run_game(gTable[bselect].options, 0);
                    }
                    else
                    {
                        copyGF2Psram(bselect, bfill);
                        loadSaveState(bselect, bfill);
                        neo_run_psram(gTable[bselect].options, 0);
                    }
                }
            }
        }

        if (bopt == 1)
        {
            if (CR_BUTTON(buttons ^ previous))
            {
                // CR changed
                if (!CR_BUTTON(buttons))
                    gTable[bselect].options[5+osel] = onext[osel][gTable[bselect].options[5+osel]];
            }
            if (CL_BUTTON(buttons ^ previous))
            {
                // CL changed
                if (!CL_BUTTON(buttons))
                    gTable[bselect].options[5+osel] = oprev[osel][gTable[bselect].options[5+osel]];
            }
            if (CU_BUTTON(buttons ^ previous))
            {
                // CU changed
                if (!CU_BUTTON(buttons))
                    osel ^=1;
            }
            if (CD_BUTTON(buttons ^ previous))
            {
                // CD changed
                if (!CD_BUTTON(buttons))
                    osel ^=1;
            }
        }

        if (TL_BUTTON(buttons ^ previous))
        {
            // TL changed
            if (!TL_BUTTON(buttons))
            {
                // TL just released
                if (brwsr)
                {
                    doSaveMgr(bfill);
                    // browse SD card
                    neo2_enable_sd();
                    bmax = getSDInfo(-1);
                }
                else
                {
                    bfill = bfill ? bfill-1 : 4;
                    saveBrowserFlags(brwsr, bopt, bfill);
                    setTextColors(bfill);
                }
            }
        }

        if (TR_BUTTON(buttons ^ previous))
        {
            // TR changed
            if (!TR_BUTTON(buttons))
            {
                // TR just released
                bfill = (bfill == 4) ? 0 : bfill+1;
                saveBrowserFlags(brwsr, bopt, bfill);
                setTextColors(bfill);
            }
        }

        previous = buttons;
        delay(1);
    }

    config_shutdown();//will never reach here anyway
    return 0;
}

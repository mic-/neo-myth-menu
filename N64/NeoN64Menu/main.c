
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>

#include <diskio.h>
#include <ff.h>

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

sprite_t *splash;
int splash_w, splash_h;
sprite_t *browser;
int browser_w, browser_h;
sprite_t *loading;
int loading_w, loading_h;

u64 back_flags;                         /* 0xAA550mno - m = brwsr, n = bopt, o = bfill */

struct selEntry {
    u32 valid;                          /* 0 = invalid, ~0 = valid */
    u32 type;                           /* 128 = directory, 0xFF = N64 game */
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
FIL gSDFile;                            /* global file structure for FF */

short int gSdDetected = 0;              /* 0 - not detected, 1 - detected */

char path[1024];


#define ONCE_SIZE (256*1024)
static u8 __attribute__((aligned(16))) tmpBuf[ONCE_SIZE];

extern sprite_t *loadImageDFS(char *fname, int *w, int *h);
extern sprite_t *loadImageSD(char *fname, int *w, int *h);
extern void drawImage(display_context_t dcon, sprite_t *sprites, int w, int h);

extern void neo2_enable_sd(void);
extern void neo2_disable_sd(void);
extern DSTATUS MMC_disk_initialize(void);

extern void neo_select_menu(void);
extern void neo_select_game(void);
extern void neo_select_psram(void);
extern void neo_psram_offset(int offset);

extern unsigned int neo_id_card(void);
extern unsigned int neo_get_cpld(void);
extern void neo_run_menu(void);
extern void neo_run_game(u8 *options, int reset);
extern void neo_run_psram(u8 *options, int reset);
extern void neo_copyfrom_game(void *dest, int fstart, int len);
extern void neo_copyfrom_menu(void *dest, int fstart, int len);
extern void neo_xferto_psram(void *src, int pstart, int len);
extern void neo_copyto_psram(void *src, int pstart, int len);
extern void neo_copyfrom_psram(void *dst, int pstart, int len);
extern void neo_copyto_sram(void *src, int sstart, int len);
extern void neo_copyfrom_sram(void *dst, int sstart, int len);
extern void neo_copyto_nsram(void *src, int sstart, int len);
extern void neo_copyfrom_nsram(void *dst, int sstart, int len);
extern void neo_copyto_eeprom(void *src, int sstart, int len, int mode);
extern void neo_copyfrom_eeprom(void *dst, int sstart, int len, int mode);
extern void neo_get_rtc(unsigned char *rtc);

extern int get_cic(unsigned char *buffer);
extern int get_swap(unsigned char *buffer);
extern int get_cic_save(char *cartid, int *cic, int *save);

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

void progress_screen(char *str1, char *str2, int frac, int total, int bfill)
{
    display_context_t dcon;
    char temp[40];

    // get next buffer to draw in
    dcon = lockVideo(1);
    graphics_fill_screen(dcon, 0);

    if (loading && (bfill == 4))
    {
        drawImage(dcon, loading, loading_w, loading_h);
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

    graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), 0);
    printText(dcon, str1, 20 - strlen(str1)/2, 3);

    graphics_set_color(graphics_make_color(0x7F, 0xFF, 0x3F, 0xFF), 0);
    strncpy(temp, str2, 34);
    temp[34] = 0;
    printText(dcon, temp, 20-strlen(temp)/2, 5);
    for (int ix=34, iy=6; ix<strlen(str2); ix+=34, iy++)
    {
        strncpy(temp, &str2[ix], 34);
        temp[34] = 0;
        printText(dcon, temp, 20-strlen(temp)/2, iy);
    }

    if (frac)
        graphics_draw_box(dcon, 32, 160, 256*frac/total, 6, graphics_make_color(0x3F, 0xFF, 0x3F, 0xFF));
    if (frac<total)
        graphics_draw_box(dcon, 32+256*frac/total, 160, 256-256*frac/total, 6, graphics_make_color(0xFF, 0x3F, 0x3F, 0xFF));

    // show display
    unlockVideo(dcon);
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

void get_sd_info(int entry)
{
    UINT ts;
    u8 buffer[0x440];
    XCHAR fpath[1280];
    char cartid[4];
    int cic, save;

    c2wstrcpy(fpath, path);
    if (path[strlen(path)-1] != '/')
        c2wstrcat(fpath, "/");

    c2wstrcat(fpath, gTable[entry].name);
    f_close(&gSDFile);
    if (f_open(&gSDFile, fpath, FA_OPEN_EXISTING | FA_READ))
    {
        //char temp[1536];
        // couldn't open file
        //w2cstrcpy(temp, fpath);
        debugText("Couldn't open file: ", 2, 16, 1);
        debugText(gTable[entry].name, 22, 16, 180);
        return;
    }
    // read rom header
    f_read(&gSDFile, buffer, 0x440, &ts);

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
    WCHAR lfnbuf[256];
    XCHAR fpath[1280];

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
//        if (do_SDMgr())                 /* we know there's a card, but can't read it */
//            return;                     /* user opted not to try to format the SD card */
        if (f_opendir(&dir, fpath))
            return 0;                   /* failed again... give up */
        gSdDetected = 1;
    }

    // add parent directory entry if not root
    if (path[1] != 0)
    {
        gTable[max].valid = 1;
        gTable[max].type = 128;         // directory entry
        strcpy(gTable[max].name, "..");
        max++;
    }

    // scan dirctory entries
    for(;;)
    {
        FRESULT fres;
        fno.lfname = (WCHAR*)lfnbuf;
        fno.lfsize = 255;
        fno.lfname[0] = (WCHAR)0;
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
        if (fno.lfname[0] == (WCHAR)'.')
            continue;                   /* skip "hidden" files and directories */

        if (fno.fattrib & AM_DIR)
        {
            gTable[max].valid = 1;
            gTable[max].type = 128;     // directory entry
            if (fno.lfname[0])
                w2cstrcpy(gTable[max].name, fno.lfname);
            else
                strcpy(gTable[max].name, fno.fname);
        }
        else
        {
            gTable[max].valid = 1;
            gTable[max].type = 127;     // unknown
            gTable[max].options[0] = 0xFF;
            gTable[max].options[1] = 0;
            gTable[max].options[2] = 0;
            gTable[max].options[3] = (fno.fsize / (128*1024)) >> 8;
            gTable[max].options[4] = (fno.fsize / (128*1024)) & 0xFF;
            gTable[max].options[5] = 0;
            gTable[max].options[6] = 2;
            gTable[max].options[7] = 0;
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

/* Copy data from file to gba psram */
void copySD2Psram(int bselect, int bfill)
{
    u32 romsize, gamelen, copylen;
    XCHAR fpath[1280];
    char temp[256];

    // load rom info if not already loaded
    if (gTable[bselect].type == 127)
        get_sd_info(bselect);

    romsize = (gTable[bselect].options[3]<<8) | gTable[bselect].options[4];
    // for SD card file, romsize is number of Mbits in rom
    gamelen = romsize*128*1024;

    strcpy(temp, gTable[bselect].name);

    c2wstrcpy(fpath, path);
    if (path[strlen(path)-1] != '/')
        c2wstrcat(fpath, "/");

    c2wstrcat(fpath, gTable[bselect].name);
    f_close(&gSDFile);
    if (f_open(&gSDFile, fpath, FA_OPEN_EXISTING | FA_READ))
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

    // copy the rom from file to ram, then ram to psram
    for(int ic=0; ic<copylen; ic+=ONCE_SIZE)
    {
        UINT ts;
        progress_screen("Loading", temp, 100*ic/gamelen, 100, bfill);
        // read file to buffer
        f_read(&gSDFile, tmpBuf, ONCE_SIZE, &ts);
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

    if (gamelen <= (16*1024*1024))
    {
        progress_screen("Loading", temp, 100, 100, bfill);
        delay(30);
        return;
    }

    // change the psram offset and copy the rest
    neo_psram_offset(copylen/(32*1024));
    for(int ic=0; ic<(gamelen-copylen); ic+=ONCE_SIZE)
    {
        UINT ts;
        progress_screen("Loading", temp, 100*(copylen+ic)/gamelen, 100, bfill);
        // read file to buffer
        f_read(&gSDFile, tmpBuf, ONCE_SIZE, &ts);
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

    progress_screen("Loading", temp, 100, 100, bfill);
    delay(30);
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
            drawImage(dcon, browser, browser_w, browser_h);
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

        graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), 0);
        if (ssel == -1)
            printText(dcon, str1, 20 - strlen(str1)/2, 3);
        else
            printText(dcon, str2, 20 - strlen(str2)/2, 3);

        for (int iy=0; iy<(slast - sfirst + 1); iy++)
        {
            if ((sfirst + iy) == sel)
                graphics_set_color(graphics_make_color(0xEF, 0xEF, 0xEF, 0xFF), 0);
            else
                graphics_set_color((sfirst + iy) == ssel ? graphics_make_color(0xEF, 0xEF, 0xEF, 0xFF) : graphics_make_color(0x7F, 0xFF, 0x3F, 0xFF), 0);
            if (blk[sfirst + iy] == 0xAA)
            {
                strncpy(temp, &names[(sfirst + iy)*256], 36);
                temp[36] = 0;
                printText(dcon, temp, 20 - strlen(temp)/2, 5 + iy);
            }
            else
                printText(dcon, "empty", 19, 5 + iy);
        }
        graphics_set_color(graphics_make_color(0x7F, 0xFF, 0x3F, 0xFF), 0);
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

void loadSaveState(int bsel, int bfill)
{
    int ix, ssize[16] = { 0, 32768, 65536, 131072, 131072, 512, 2048, 0, 262144, 0, 0, 0, 0, 0, 0, 0 };
    char *sext[16] = { 0, ".sra",  ".sra",  ".sra", ".fla", ".eep", ".eep", 0, ".sra", 0, 0, 0, 0, 0, 0, 0 };
    char temp[260];
    XCHAR wname[280];
    u64 flags;
    UINT ts;
    selEntry_t entry;

    if (ssize[gTable[bsel].options[5] & 15] == 0)
        return; // ext cart, invalid, or off

    memcpy(&entry, &gTable[bsel], sizeof(selEntry_t)); // backup the entry

    strcpy(temp, entry.name);
    ix = strlen(temp)-1;
    while (ix && (temp[ix] != '.')) ix--;
    if (ix == 0)
        ix = strlen(temp);              // no extension - just add to end of name
    strcpy(&temp[ix], sext[entry.options[5]]);

    // check for SD card
    neo2_enable_sd();
    ix = getSDInfo(-1);             // try to get root
    if (!ix)
    {
        // no SD card, use GBA SRAM
        u8 blk[32];
        int ix;
        int soffs[20] = { 0x00000, // one slot for 128KB FPRAM games
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

        // copy from slot in GBA SRAM
        if (entry.options[5] == 4)
        {
            // copy FRAM in two segments
            neo_copyfrom_sram(tmpBuf, soffs[ix], 65536);
            neo_copyfrom_sram(&tmpBuf[65536], soffs[ix]+65536, 65536);
            // "fix" for sram quirk
            neo_copyfrom_sram(tmpBuf, 0x3FFF0, 16);
        }
        else
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
        // load save state from SD card
        c2wstrcpy(wname, "/.menu/n64/save/");
        c2wstrcat(wname, temp);
        f_close(&gSDFile);
        if (f_open(&gSDFile, wname, FA_OPEN_EXISTING | FA_READ) == FR_OK)
        {
            f_read(&gSDFile, tmpBuf, ssize[entry.options[5]], &ts);
            f_close(&gSDFile);
        }
        else
        {
            // no save state file - init save ram to all 0xFFs
            memset(tmpBuf, 0xFF, ssize[entry.options[5]]);
        }
    }
    neo2_disable_sd();

    switch(entry.options[5])
    {
        case 1:
        case 2:
        case 3:
            byte_swap_buf(tmpBuf, ssize[entry.options[5]]);
            neo_copyto_nsram(tmpBuf, 0, ssize[entry.options[5]]);
            break;
        case 4:
            byte_swap_buf(tmpBuf, ssize[entry.options[5]]);
            neo_copyto_nsram(tmpBuf, 0, ssize[entry.options[5]]);
            break;
        case 5:
        case 6:
            neo_copyto_eeprom(tmpBuf, 0, ssize[entry.options[5]], entry.options[5]);
            break;
        case 8:
            byte_swap_buf(tmpBuf, ssize[entry.options[5]]);
            neo_copyto_nsram(tmpBuf, 0, ssize[entry.options[5]]);
            break;
    }
    flags = 0xAA550100LL | entry.options[5];

    neo_copyto_sram(temp, 0x3FE00, 256);
    neo_copyto_sram(&flags, 0x3FF00, 8);

    memcpy(&gTable[bsel], &entry, sizeof(selEntry_t)); // restore the entry
}

void saveSaveState(void)
{
    int ssize[16] = { 0, 32768, 65536, 131072, 131072, 512, 2048, 0, 262144, 0, 0, 0, 0, 0, 0, 0 };
    char temp[256];
    XCHAR wname[280];
    u64 flags;
    UINT ts;
    int bmax;

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

    switch(flags & 15)
    {
        case 1:
        case 2:
        case 3:
            neo_copyfrom_nsram(tmpBuf, 0, ssize[flags & 15]);
            byte_swap_buf(tmpBuf, ssize[flags & 15]);
            break;
        case 4:
            neo_copyfrom_nsram(tmpBuf, 0, ssize[flags & 15]);
            byte_swap_buf(tmpBuf, ssize[flags & 15]);
            break;
        case 5:
        case 6:
            neo_copyfrom_eeprom(tmpBuf, 0, ssize[flags & 15], flags & 15);
            break;
        case 8:
            neo_copyfrom_nsram(tmpBuf, 0, ssize[flags & 15]);
            byte_swap_buf(tmpBuf, ssize[flags & 15]);
            break;
    }

    // check for SD card
    neo2_enable_sd();
    bmax = getSDInfo(-1);               // try to get root
    if (!bmax)
    {
        // no SD card, use GBA SRAM
        u8 blk[32];
        int ix;
        int soffs[20] = { 0x00000, // one slot for 128KB FPRAM games
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

        neo2_disable_sd();
        neo2_enable_sd();
        getSDInfo(-1);
    }
    else
    {
        c2wstrcpy(wname, "/.menu/n64/save/");
        c2wstrcat(wname, temp);
        f_close(&gSDFile);
        f_open(&gSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE);
        f_write(&gSDFile, tmpBuf, ssize[flags & 15], &ts);
        f_close(&gSDFile);

    }
    neo2_disable_sd();

    // turn off auto-save
    flags = 0;
    neo_copyto_sram(&flags, 0x3FF00, 8);
}

void saveBrowserFlags(int brwsr, int bopt, int bfill)
{
    back_flags = 0xAA550000LL | (brwsr<<8) | (bopt<<4) | bfill;
    neo_copyto_sram(&back_flags, 0x3FF08, 8);

    if (brwsr)
        neo2_enable_sd();               // make sure still in proper mode for SD
}

/* initialize console hardware */
void init_n64(void)
{
    // enable MI interrupts (on the CPU)
    set_MI_interrupt(1,1);

    // Initialize display
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
    rdp_init();
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
        dfs_close(fp);
        fp = dfs_open("/pattern1.sprite");
        pattern[1] = malloc(dfs_size(fp));
        dfs_read(pattern[1], 1, dfs_size(fp), fp);
        dfs_close(fp);
        fp = dfs_open("/pattern2.sprite");
        pattern[2] = malloc(dfs_size(fp));
        dfs_read(pattern[2], 1, dfs_size(fp), fp);
        dfs_close(fp);
        // load backdrop images
        splash = loadImageDFS("/splash.jpg", &splash_w, &splash_h);
        browser = loadImageDFS("/browser.jpg", &browser_w, &browser_h);
        loading = loadImageDFS("/loading.jpg", &loading_w, &loading_h);
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
    char *menu_title = "Neo N64 Myth Menu v1.8 (U2)";
#elif defined RUN_FROM_SD
    char *menu_title = "Neo N64 Myth Menu v1.8 (SD)";
#else
    char *menu_title = "Neo N64 Myth Menu v1.8 (MF)";
#endif
    char *menu_help1 = "A=Run reset to menu  B=Reset to game";
    char *menu_help2 = "DPad = Navigate CPad = change option";
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

    int stemp_w, stemp_h;
    sprite_t *stemp;

    init_n64();

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

#ifdef RUN_FROM_U2
    // check for boot rom in menu
    if (!memcmp((void *)0xB0000020, "N64 Myth Menu (MF)", 18))
        neo_run_menu();
#endif

#ifndef RUN_FROM_SD
    // check for boot rom on SD card
    neo2_enable_sd();
    bmax = getSDInfo(-1);               // get root directory of sd card
    if (bmax)
    {
        XCHAR fpath[32];

        strcpy(path, "/.menu/n64/");
        strcpy(gTable[0].name, "NEON64SD.v64");
        c2wstrcpy(fpath, path);
        c2wstrcat(fpath, gTable[0].name);
        if (f_open(&gSDFile, fpath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
        {
            stemp = loadImageSD("/.menu/n64/images/loading.jpg", &stemp_w, &stemp_h);
            if (stemp)
            {
                if (loading)
                    free(loading);
                loading = stemp;
                loading_w = stemp_w;
                loading_h = stemp_h;
            }

            gTable[0].valid = 1;
            gTable[0].type = 127;
            gTable[0].options[0] = 0xFF;
            gTable[0].options[1] = 0;
            gTable[0].options[2] = 0;
            gTable[0].options[3] = 0;
            gTable[0].options[4] = (gSDFile.fsize > 2097152) ? gSDFile.fsize/131072 : 16;
            gTable[0].options[5] = 5;
            gTable[0].options[6] = 2;
            gTable[0].options[7] = 0;
            get_sd_info(0);

            copySD2Psram(0, 0);
            neo_run_psram(gTable[0].options, 1);
        }
        neo2_disable_sd();
    }
#endif

    neo2_enable_sd();
    bmax = getSDInfo(-1);
    if (bmax)
    {
        // try for images on the SD card
        stemp = loadImageSD("/.menu/n64/images/splash.jpg", &stemp_w, &stemp_h);
        if (stemp)
        {
            if (splash)
                free(splash);
            splash = stemp;
            splash_w = stemp_w;
            splash_h = stemp_h;
        }
        stemp = loadImageSD("/.menu/n64/images/browser.jpg", &stemp_w, &stemp_h);
        if (stemp)
        {
            if (browser)
                free(browser);
            browser = stemp;
            browser_w = stemp_w;
            browser_h = stemp_h;
        }
        stemp = loadImageSD("/.menu/n64/images/loading.jpg", &stemp_w, &stemp_h);
        if (stemp)
        {
            if (loading)
                free(loading);
            loading = stemp;
            loading_w = stemp_w;
            loading_h = stemp_h;
        }
    }
    neo2_disable_sd();

    if (splash)
    {
        display_context_t dcon;
        dcon = lockVideo(1);
        graphics_fill_screen(dcon, 0);
        drawImage(dcon, splash, splash_w, splash_h);
        unlockVideo(dcon);
        delay(120);
    }

    // save save state if needed
    saveSaveState();

    if ((back_flags & 0xFFFFF000) == 0xAA550000)
    {
        brwsr = (back_flags >> 8) & 1;
        bopt = (back_flags >> 4) & 1;
        bfill = back_flags & 7;
    }

    if (brwsr)
    {
        neo2_enable_sd();
        bmax = getSDInfo(-1);           // preload SD root dir
        btout = 60;
    }
    else
        bmax = getGFInfo();             // preload flash menu entries

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
            drawImage(dcon, browser, browser_w, browser_h);
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
        if (browser && (bfill == 4))
            graphics_set_color(graphics_make_color(0x3F, 0x3F, 0x7F, 0xFF), 0);
        else
            graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), 0);
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
                    graphics_set_color((bstart+ix) == bselect ? graphics_make_color(0xEF, 0xEF, 0xEF, 0xFF) : graphics_make_color(0x7F, 0xFF, 0x3F, 0xFF), 0);
                    printText(dcon, temp, 20 - strlen(temp)/2, 3 + ix);
                }
                else
                    break; // stop on invalid entry
            }
        }

        graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), 0);

        if (bmax && gTable[bselect].valid)
        {
            if ((gTable[bselect].type == 255) && (bopt == 0))
            {
                if (gTable[bselect].swap)
                    graphics_set_color(graphics_make_color(0xFF, 0x3F, 0xBF, 0xFF), 0);
                // print selection info
                printText(dcon, (char *)gTable[bselect].rom, 2, 14);
                sprintf(temp, "Country: %02X (%c)", gTable[bselect].rom[0x1E], gTable[bselect].rom[0x1E]);
                printText(dcon, temp, 23, 14);
                sprintf(temp, "Manuf ID: %02X (%c)", gTable[bselect].rom[0x1B], gTable[bselect].rom[0x1B]);
                printText(dcon, temp, 2, 15);
                sprintf(temp, "Cart ID: %04X (%c%c)", gTable[bselect].rom[0x1C]<<8 | gTable[bselect].rom[0x1D], gTable[bselect].rom[0x1C], gTable[bselect].rom[0x1D]);
                printText(dcon, temp, 20, 15);
            }

            if ((gTable[bselect].type == 255) && (bopt == 1))
            {
                // print selection options
                sprintf(temp, "Bank: %3d", (gTable[bselect].options[1]<<8) | gTable[bselect].options[2]);
                printText(dcon, temp, 4, 14);
                sprintf(temp, "Size: %3d", (gTable[bselect].options[3]<<8) | gTable[bselect].options[4]);
                printText(dcon, temp, 27, 14);
                if (osel == 0)
                    graphics_set_color(graphics_make_color(0xFF, 0x3F, 0x3F, 0xFF), 0);
                else
                    graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), 0);
                sprintf(temp, "Save: %s", ostr[0][gTable[bselect].options[5]]);
                printText(dcon, temp, 4, 15);
                if (osel == 1)
                    graphics_set_color(graphics_make_color(0xFF, 0x3F, 0x3F, 0xFF), 0);
                else
                    graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), 0);
                sprintf(temp, "CIC: %s", ostr[1][gTable[bselect].options[6]]);
                printText(dcon, temp, 36-strlen(temp), 15);
            }

            graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), 0);

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

        // show cart info help messages
        if (browser && (bfill == 4))
            graphics_set_color(graphics_make_color(0x5F, 0x1F, 0x1F, 0xFF), 0);
        else
            graphics_set_color(graphics_make_color(0xFF, 0x4F, 0x4F, 0xFF), 0);
        sprintf(temp, "CPLD:V%d CART:0x%08X FLASH:%c", gCpldVers & 7, gCardID, cards[gCardType & 3]);
        printText(dcon, temp, 20 - strlen(temp)/2, 24);
        graphics_set_color(graphics_make_color(0x7F, 0xFF, 0x3F, 0xFF), 0);
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
                // START just released
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
            if (CU_BUTTON(buttons ^ previous))
            {
                // CU changed
                if (!CU_BUTTON(buttons))
                    gTable[bselect].options[5+osel] = onext[osel][gTable[bselect].options[5+osel]];
            }
            if (CD_BUTTON(buttons ^ previous))
            {
                // CD changed
                if (!CD_BUTTON(buttons))
                    gTable[bselect].options[5+osel] = oprev[osel][gTable[bselect].options[5+osel]];
            }
            if (CR_BUTTON(buttons ^ previous))
            {
                // CR changed
                if (!CR_BUTTON(buttons))
                    osel ^=1;
            }
            if (CL_BUTTON(buttons ^ previous))
            {
                // CL changed
                if (!CL_BUTTON(buttons))
                    osel ^=1;
            }
        }

        if (TL_BUTTON(buttons ^ previous))
        {
            // TL changed
            if (!TL_BUTTON(buttons))
            {
                // TL just released
                bfill = bfill ? bfill-1 : 4;
                saveBrowserFlags(brwsr, bopt, bfill);
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
            }
        }

        previous = buttons;
        delay(1);
    }

    return 0;
}

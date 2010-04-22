/* Neo Super 32X/MD/SMS Flash Cart Menu Loader by Chilly Willy              */
/* The license on this code is the same as the original menu code - MIT/X11 */

#include <string.h>
#include <stdio.h>

#include "pff.h"

#define XFER_SIZE 16384

/* Menu entry definitions */

/* global variables */

short int gCardOkay;                    /* 0 = okay, -1 = err */
short int gCursorX;						/* range is 0 to 63 (only 0 to 39 onscreen) */
short int gCursorY;						/* range is 0 to 31 (only 0 to 27 onscreen) */

extern volatile int gTicks;				/* incremented every vblank */

extern unsigned short cardType;			/* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */

FATFS gSDFatFs;							/* global FatFs structure for PFF */

/* arrays */

const char *gEmptyLine = "                                      ";

unsigned char __attribute__((aligned(16))) buffer[XFER_SIZE];		/* Work RAM buffer */

/* external support functions */

extern int set_sr(int sr);
extern void init_hardware(void);
extern void clear_screen(void);
extern void put_str(const char *str, int fcolor);
extern short int neo_check_card(void);
extern void neo_run_myth_psram(int psize, int bbank, int bsize, int run);
extern void neo_copyto_myth_psram(unsigned char *src, int pstart, int len);
extern void neo2_enable_sd(void);
extern void neo2_disable_sd(void);

// needed for GCC 4.x libc

int atexit(void (*function)(void))
{
    return -1;
}

/* support functions */ 

void neo_copy_sd(unsigned char *dest, int fstart, int len)
{
	WORD ts;
	set_sr(0x2000); 	/* enable interrupts */
	pf_read(dest, len, &ts);
	set_sr(0x2700); 	/* disable interrupts */
}

void delay(int count)
{
    int ticks = gTicks + count;

    while (gTicks < ticks) ;
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
	gCursorX = 20 - (strlen(str1) + strlen(str2)) / 2;
	put_str(str1, 0x2000);				/* print first string in green */
	gCursorX += strlen(str1);
	put_str(str2, 0);					/* print first string in white */

	gCursorX = 1;
	gCursorY = 22;
	// draw progress bar
	put_str("   ", 0);
	gCursorX = 4;
	for (ix=0; ix<=(32*curr/total); ix++, gCursorX++)
		put_str("\x87", 0x2000);		/* part completed */
	while (gCursorX < 36)
	{
		put_str("\x87", 0x4000);		/* part left to go */
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
	int iy;

	for (iy=0; iy<length; iy+=XFER_SIZE)
	{
		// fetch data data from source
		(src)(buffer, soffset + iy, XFER_SIZE);
		// store data to destination
		(dst)(buffer, doffset + iy, XFER_SIZE);
		update_progress(str1, str2, iy, length);
	}
}

void get_sd_menu(void)
{
    neo2_enable_sd();
	cardType = 0;
	// mount SD card
	if (pf_mount(&gSDFatFs))
	{
		cardType = 0x8000;  	 		/* try funky read timing */
		if (pf_mount(&gSDFatFs))
        {
            update_progress("Failed ", "Insert SD card", 100, 100);
			while (1);					/* couldn't mount SD card - hang */
        }
	}

	if (pf_open("/MDEBIOS.BIN"))
    {
        update_progress("Failed ", "Couldn't open MDEBIOS.BIN", 100, 100);
		while (1);						/* failed to open menu - hang */
    }
}

/* main code entry point */
 
int main(void)
{
	init_hardware();					/* set hardware to a consistent state, clears vram and loads the font */
    gCardOkay = neo_check_card();       /* check for Neo Flash card */
    set_sr(0x2000);						/* allow interrupts */

    get_sd_menu();
	// copy file to myth psram
	copyGame(&neo_copyto_myth_psram, &neo_copy_sd, 0x700000, 0, gSDFatFs.fsize, "Loading ", "MDEBIOS.BIN");

    neo2_disable_sd();
    delay(2);
	set_sr(0x2700); 	                /* disable interrupts */
	neo_run_myth_psram(gSDFatFs.fsize, 0, 0, 0x27); // never returns

	return 0;
}

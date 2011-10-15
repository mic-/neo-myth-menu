#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <libdragon.h>
#include <diskio.h>
#include <ff.h>


// reuse gTable for file requester

struct selEntry {
    uint32_t valid;                          /* 0 = invalid, ~0 = valid */
    uint32_t type;                           /* 64 = compressed , 128 = directory, 0xFF = N64 game */
    uint32_t swap;                           /* 0 = no swap, 1 = byte swap, 2 = word swap, 3 = long swap */
    uint32_t pad;
    uint8_t options[8];                      /* menu entry options */
    uint8_t hdr[0x20];                       /* copy of data from rom offset 0 */
    uint8_t rom[0x20];                       /* copy of data from rom offset 0x20 */
    char name[256];
};
typedef struct selEntry selEntry_t;

extern selEntry_t gTable[1024];

extern char path[1024];

extern int getSDInfo(int entry);

#define MAXFILES 1024
#define PAGESIZE 10

static int maxfiles;

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

extern unsigned short getButtons(int pad);

extern sprite_t *pattern[3];
extern sprite_t *splash;
extern sprite_t *browser;
extern sprite_t *loading;

extern void drawImage(display_context_t dcon, sprite_t *sprite);

struct textColors {
    uint32_t title;
    uint32_t usel_game;
    uint32_t sel_game;
    uint32_t uswp_info;
    uint32_t swp_info;
    uint32_t usel_option;
    uint32_t sel_option;
    uint32_t hw_info;
    uint32_t help_info;
};
typedef struct textColors textColors_t;

extern textColors_t gTextColors;

extern void delay(int cnt);
extern display_context_t lockVideo(int wait);
extern void unlockVideo(display_context_t dc);
extern void printText(display_context_t dc, char *msg, int x, int y);


/****************************************************************************
 * ParseDirectory
 *
 * Parse the directory, returning the number of files found
 ****************************************************************************/

int parse_dir (int entry)
{
    maxfiles = getSDInfo(entry);

	return maxfiles;
}

/****************************************************************************
 * ShowFiles
 *
 * Support function for FileSelector
 ****************************************************************************/

void ShowFiles( int offset, int selection, char * title, int bfill )
{
    display_context_t dcon;
    int bselect = selection, bstart = offset;
    char temp[40];
    char *req_help1 = "A=Select File       B=Cancel Request";
    char *req_help2 = "DPad=Navigate                       ";

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
    printText(dcon, title, 20 - strlen(title)/2, 1);

    // print browser list (lines 3 to 12)
    if (maxfiles)
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

    // show cart info help messages
    graphics_set_color(gTextColors.help_info, 0);
    printText(dcon, req_help1, 20 - strlen(req_help1)/2, 25);
    printText(dcon, req_help2, 20 - strlen(req_help2)/2, 26);

    // show display
    unlockVideo(dcon);
}

/****************************************************************************
 * FileSelector
 *
 * Press A to select, B to cancel
 ****************************************************************************/

int FileSelector(char *title, int bfill)
{
	int offset = 0;
	int selection = 0;
	int redraw = 1;
	unsigned short p = getButtons(0);

	while ( !B_BUTTON(p) )
	{
		if ( redraw )
			ShowFiles( offset, selection, title, bfill );
		redraw = 0;

		while (!(p = getButtons(0)))
			delay(10);

		if ( DD_BUTTON(p) )
		{
			selection++;
			if ( selection == maxfiles )
				selection = offset = 0;	// wrap around to top

			if ( ( selection - offset ) == PAGESIZE )
				offset += PAGESIZE; // next "page" of entries

			redraw = 1;
		}

		if ( DU_BUTTON(p) )
		{
			selection--;
			if ( selection < 0 )
			{
				selection = maxfiles - 1;
				offset = maxfiles > PAGESIZE ? selection - PAGESIZE + 1 : 0; // wrap around to bottom
			}

			if ( selection < offset )
			{
				offset -= PAGESIZE; // previous "page" of entries
				if ( offset < 0 )
					offset = 0;
			}

			redraw = 1;
		}

		if ( DR_BUTTON(p) )
		{
			selection += PAGESIZE;
			if ( selection >= maxfiles )
				selection = offset = 0;	// wrap around to top

			if ( ( selection - offset ) >= PAGESIZE )
				offset += PAGESIZE; // next "page" of entries

			redraw = 1;
		}

		if ( DL_BUTTON(p) )
		{
			selection -= PAGESIZE;
			if ( selection < 0 )
			{
				selection = maxfiles - 1;
				offset = maxfiles > PAGESIZE ? selection - PAGESIZE + 1 : 0; // wrap around to bottom
			}

			if ( selection < offset )
			{
				offset -= PAGESIZE; // previous "page" of entries
				if ( offset < 0 )
					offset = 0;
			}

			redraw = 1;
		}

		if ( A_BUTTON(p) )
		{
            while (getButtons(0))
                delay(10);
			if ( gTable[selection].type == 128 )	/*** This is directory ***/
			{
				offset = selection = 0;
				parse_dir(selection);
			}
			else
				return selection;

			redraw = 1;
		}

        delay(15);
	}

    while (getButtons(0))
        delay(10);
	return -1; // no file selected
}

/****************************************************************************
 * RequestFile
 *
 * return filename selector in gTable
 ****************************************************************************/

int RequestFile (char *title, char *initialPath, int bfill)
{
	int selection;

    strcpy(path, "/");
    strcpy(gTable[0].name, &initialPath[1]);
	if (!parse_dir(0))
		return -1;

	selection = FileSelector (title, bfill);
	return selection;
}

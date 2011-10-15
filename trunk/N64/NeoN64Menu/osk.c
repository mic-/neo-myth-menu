#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <libdragon.h>

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


int doOSKeyb(char *title, char *deflt, char *buffer, int max, int bfill)
{
    display_context_t dcon;
    uint16_t previous = 0, buttons;
    int crsr;
    short xcoord[16] = { 0, 23, 14, 0, 17, 20, 14, 0, 20, 23, 17, 0, 0, 0, 0, 0 };
    short ycoord[16] = { 0, 7, 5, 0, 7, 7, 7, 0, 5, 5, 5, 0, 0, 0, 0, 0 };
    char *shft_text1[16] = { "", ":\"|", "!@#", "", "FGH", "JKL", "ASD", "", "&*(", ")_+", "$%^", "", "", "", "", "" };
    char *shft_text2[16] = { "", "? ~", "QWE", "", "VBN", "M<>", "ZXC", "", "UIO", "P{}", "RTY", "", "", "", "", "" };
    char *norm_text1[16] = { "", ";'\\", "123", "", "fgh", "jkl", "asd", "", "789", "0-=", "456", "", "", "", "", "" };
    char *norm_text2[16] = { "", "/ `", "qwe", "", "vbn", "m,.", "zxc", "",  "uio", "p[]", "rty", "", "", "", "", "" };

    memset(buffer, 0, max);
    strncpy(buffer, deflt, max-1);
    buffer[max-1] = 0;
    crsr = strlen(buffer);

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
        printText(dcon, title, 20 - strlen(title)/2, 1);

        // print text line buffer (line 3)
        if (strlen(buffer))
        {
            graphics_set_color(gTextColors.usel_game, 0);
            printText(dcon, buffer, 20 - strlen(buffer)/2, 3);
        }

        {
            char temp[2];
            temp[0] = buffer[crsr];
            if ((temp[0] == 0) | (temp[0] == 0x20))
                temp[0] = '_';
            temp[1] = 0;
            graphics_set_color(gTextColors.usel_game, gTextColors.sel_game);
            printText(dcon, temp, 20 - strlen(buffer)/2 + crsr, 3);
        }

        // get buttons
        buttons = getButtons(0);

        // draw OSK (lines 5 to 8)
        if (TR_BUTTON(buttons))
        {
            // shift held - do shift characters
            graphics_set_color(gTextColors.usel_game, 0);
            printText(dcon, "!@#$%^&*()_+", xcoord[2], ycoord[2]);
            printText(dcon, "QWERTYUIOP{}", xcoord[2], ycoord[2]+1);
            printText(dcon, "ASDFGHJKL:\"|", xcoord[6], ycoord[6]);
            printText(dcon, "ZXCVBNM<>? ~", xcoord[6], ycoord[6]+1);

            if ((buttons & 0x0F00) && (buttons & 0xC00F))
            {
                short dx = -1, dy = -1;
                // hilite square behind button being pressed
                graphics_set_color(0, gTextColors.usel_game);
                if (B_BUTTON(buttons))
                {
                    dx = 0;
                    dy = 0;
                }
                else if (CL_BUTTON(buttons))
                {
                    dx = 1;
                    dy = 0;
                }
                else if (CU_BUTTON(buttons))
                {
                    dx = 2;
                    dy = 0;
                }
                else if (A_BUTTON(buttons))
                {
                    dx = 0;
                    dy = 1;
                }
                else if (CD_BUTTON(buttons))
                {
                    dx = 1;
                    dy = 1;
                }
                else if (CR_BUTTON(buttons))
                {
                    dx = 2;
                    dy = 0;
                }
                if (dx >= 0)
                    printText(dcon, "_", xcoord[(buttons>>8)&15]+dx, ycoord[(buttons>>8)&15]+dy);
            }

            graphics_set_color(gTextColors.sel_game, 0);
            if (xcoord[(buttons>>8)&15])
            {
                printText(dcon, shft_text1[(buttons>>8)&15], xcoord[(buttons>>8)&15], ycoord[(buttons>>8)&15]);
                printText(dcon, shft_text2[(buttons>>8)&15], xcoord[(buttons>>8)&15], ycoord[(buttons>>8)&15]+1);
            }

        }
        else
        {
            // no shift - do normal characters
            graphics_set_color(gTextColors.usel_game, 0);
            printText(dcon, "1234567890-=", xcoord[2], ycoord[2]);
            printText(dcon, "qwertyuiop[]", xcoord[2], ycoord[2]+1);
            printText(dcon, "asdfghjkl;'\\", xcoord[6], ycoord[6]);
            printText(dcon, "zxcvbnm,./ `", xcoord[6], ycoord[6]+1);

            if ((buttons & 0x0F00) && (buttons & 0xC00F))
            {
                short dx = -1, dy = -1;
                // hilite square behind button being pressed
                graphics_set_color(0, gTextColors.usel_game);
                if (B_BUTTON(buttons))
                {
                    dx = 0;
                    dy = 0;
                }
                else if (CL_BUTTON(buttons))
                {
                    dx = 1;
                    dy = 0;
                }
                else if (CU_BUTTON(buttons))
                {
                    dx = 2;
                    dy = 0;
                }
                else if (A_BUTTON(buttons))
                {
                    dx = 0;
                    dy = 1;
                }
                else if (CD_BUTTON(buttons))
                {
                    dx = 1;
                    dy = 1;
                }
                else if (CR_BUTTON(buttons))
                {
                    dx = 2;
                    dy = 0;
                }
                if (dx >= 0)
                    printText(dcon, "_", xcoord[(buttons>>8)&15]+dx, ycoord[(buttons>>8)&15]+dy);
            }

            graphics_set_color(gTextColors.sel_game, 0);
            if (xcoord[(buttons>>8)&15])
            {
                printText(dcon, norm_text1[(buttons>>8)&15], xcoord[(buttons>>8)&15], ycoord[(buttons>>8)&15]);
                printText(dcon, norm_text2[(buttons>>8)&15], xcoord[(buttons>>8)&15], ycoord[(buttons>>8)&15]+1);
            }
        }

        // show display
        unlockVideo(dcon);

        if (buttons & 0x0F00)
        {
            // pressing a direction => OSK
            char c = 0;

            if (B_BUTTON(buttons))
                c = TR_BUTTON(buttons) ? shft_text1[((buttons>>8)&15)][0] : norm_text1[((buttons>>8)&15)][0];
            else if (CL_BUTTON(buttons))
                c = TR_BUTTON(buttons) ? shft_text1[((buttons>>8)&15)][1] : norm_text1[((buttons>>8)&15)][1];
            else if (CU_BUTTON(buttons))
                c = TR_BUTTON(buttons) ? shft_text1[((buttons>>8)&15)][2] : norm_text1[((buttons>>8)&15)][2];
            else if (A_BUTTON(buttons))
                c = TR_BUTTON(buttons) ? shft_text2[((buttons>>8)&15)][0] : norm_text2[((buttons>>8)&15)][0];
            else if (CD_BUTTON(buttons))
                c = TR_BUTTON(buttons) ? shft_text2[((buttons>>8)&15)][1] : norm_text2[((buttons>>8)&15)][1];
            else if (CR_BUTTON(buttons))
                c = TR_BUTTON(buttons) ? shft_text2[((buttons>>8)&15)][2] : norm_text2[((buttons>>8)&15)][2];
            if (c && (crsr < (max-2)))
            {
                if (buffer[crsr] == 0)
                    buffer[crsr+1] = 0;
                buffer[crsr] = c;
                crsr++;
            }
        }
        else
        {
            if (A_BUTTON(buttons ^ previous))
            {
                // A changed
                if (!A_BUTTON(buttons))
                {
                    // A just released - Enter
                    return strlen(buffer);
                }
            }
            if (B_BUTTON(buttons ^ previous))
            {
                // B changed
                if (!B_BUTTON(buttons))
                {
                    // B just released - Escape
                    return 0;
                }
            }
            if (CL_BUTTON(buttons ^ previous))
            {
                // CL changed
                if (!CL_BUTTON(buttons))
                {
                    // CL just released - Cursor Left
                    if (crsr > 0)
                        crsr--;
                }
            }
            if (CR_BUTTON(buttons ^ previous))
            {
                // CR changed
                if (!CR_BUTTON(buttons))
                {
                    // CR just released - Cursor Right
                    if (crsr < (strlen(buffer)))
                        crsr++;
                }
            }
            if (CU_BUTTON(buttons ^ previous))
            {
                // CU changed
                if (!CU_BUTTON(buttons))
                {
                    // CU just released - Delete
                    if (crsr > 0)
                    {
                        crsr--;
                        if (buffer[crsr+1] == 0)
                            buffer[crsr] = 0;
                        else
                            strcpy(&buffer[crsr], &buffer[crsr+1]);
                    }
                }
            }
            if (CD_BUTTON(buttons ^ previous))
            {
                // CD changed
                if (!CD_BUTTON(buttons))
                {
                    // CD just released - Space
                    if (crsr < (max-2))
                    {
                        if (buffer[crsr] == 0)
                            buffer[crsr+1] = 0;
                        buffer[crsr] = ' ';
                        crsr++;
                    }
                }
            }
        }

        previous = buttons;
        delay(7);
    }

    return strlen(buffer);
}

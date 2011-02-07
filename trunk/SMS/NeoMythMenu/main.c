#include "util.h"
#include "sms.h"
#include "vdp.h"
#include "pad.h"
#include "shared.h"
#include "font.h"


#define MENU_VERSION_STRING "0.11"

/*
 * Use the plain single-colored background instead of the pattered one.
 * If you remove this define you should also modify the Makefile to
 * link against font2.rel instead of font.rel.
 */
#define PLAIN_BG

void mute_psg()
{
    PsgPort = 0x9F;     // Mute channel 0
    PsgPort = 0xBF;     // Mute channel 1
    PsgPort = 0xDF;     // Mute channel 2
    PsgPort = 0xFF;     // Mute channel 3
}


/*
 * Expand the 2-bit font and write to VRAM at
 * address 0000
 */
void load_font()
{
    WORD i;
    BYTE b,c;

    disable_ints;
    vdp_set_vram_addr(0x0000);

    for (i = 0; i < 960; i++)
    {
#ifdef PLAIN_BG
        b = font[i] ^ 0xFF;
        VdpData = b;
        VdpData = 0;
        VdpData = 0;
#else
        b = font[i+i];      // Bitplane 0
        VdpData = b;
        c = font[i+i+1];    // Bitplane 1
        VdpData = c;
        VdpData = b & c;
#endif
        VdpData = 0;
    }
    enable_ints;
}


/*
 * Initializes the Sprite Attribute Table
 */
void init_sat()
{
    BYTE i;

    vdp_set_vram_addr(0x2800);

    // Initialize the Y-coordinates. Place all sprites outside
    // of the visible area
    for (i = 0; i < 0x40; i++) VdpData = 240;
}


void setup_vdp()
{
    disable_ints();
    vdp_set_reg(REG_MODE_CTRL_1, 0x04);     // Set mode 4, Line ints off
    vdp_set_reg(REG_MODE_CTRL_2, 0xE0);     // Screen on, Frame ints on
    vdp_set_reg(REG_HSCROLL, 0x00);         // Reset scrolling
    vdp_set_reg(REG_VSCROLL, 0x00);         // ...
    vdp_set_reg(REG_NAME_TABLE_ADDR, 0x07); // Nametable at 0x1800
    vdp_set_reg(REG_OVERSCAN_COLOR, 0);
    vdp_set_reg(REG_BG_PATTERN_ADDR, 0xFF); // Needed for the SMS1 VDP, ignored for later versions
    vdp_set_reg(REG_COLOR_TABLE_ADDR, 0xFF); // Needed for the SMS1 VDP, ignored for later versions
    vdp_set_reg(REG_LINE_COUNT, 0xFF);      // Line ints off
    vdp_set_reg(REG_SAT_ADDR, 0x51);        // Sprite attribute table at 0x2800

#ifdef PLAIN_BG
    vdp_set_color(0, 0, 0, 0);
    vdp_set_color(1, 3, 3, 3);
    vdp_set_color(16, 0, 0, 2);
    vdp_set_color(17, 3, 3, 3);
#else
    vdp_set_color(16, 1, 0, 0);             // Color 16 (maroon)
    vdp_set_color(17, 1, 0, 0);             // Color 17 (maroon)
    vdp_set_color(18, 1, 0, 0);             // Color 18 (maroon)
    vdp_set_color(23, 3, 3, 3);             // Color 23 (white)
#endif

    init_sat();

    enable_ints();
}

void puts_game_list()
{
    BYTE *p = (BYTE*)gbacGameList;
    BYTE row, shown;

    vdp_wait_vblank();

    // Print the current directory name
    // TODO: Handle this properly when SD support has been
    //       implemented
    puts("/", 1, 5, PALETTE0);

    shown = 0;
    row = 7;
    p += games.firstShown << 5;
    // Loop until we've shown the desired number of games, or
    // there are no more games in the list
    while ((*p != 0xFF) && (shown < NUMBER_OF_GAMES_TO_SHOW) &&
           ((shown + games.firstShown) < games.count))
    {
        if (games.highlighted == shown + games.firstShown)
            putsn(&p[8], 1, row, PALETTE1, 22); // the palette bit is bit 2 of the attribute
        else
            putsn(&p[8], 1, row, PALETTE0, 22);
        row++;
        shown++;
        p += 0x20;
    }
}


/*
 * Move to the next entry in the games list
 */
void move_to_next_game()
{
    if (++games.highlighted >= games.count)
    {
        games.highlighted--;
    }
    else
    {
        // Check if the games list needs to be scrolled
        if ((games.firstShown + ((NUMBER_OF_GAMES_TO_SHOW >> 1) - 1) < games.highlighted) &&
            (games.firstShown + NUMBER_OF_GAMES_TO_SHOW < games.count))
        {
            games.firstShown++;
        }
    }

    puts_game_list();
}


/*
 * Move to the previous entry in the games list
 */
void move_to_previous_game()
{
    if (games.highlighted)
    {
        games.highlighted--;

        // Check if the games list needs to be scrolled
        if ((games.firstShown) &&
            (games.firstShown + ((NUMBER_OF_GAMES_TO_SHOW >> 1) - 1) >= games.highlighted))
        {
            games.firstShown--;
        }
    }

    puts_game_list();
}



/*
 * Count the number of games on the GBA card
 */
WORD count_games_on_gbac()
{
    BYTE *p = (BYTE*)gbacGameList;
    WORD count = 0;

    while (*p != 0xFF)
    {
        p += 0x20;
        count++;
    }

    return count;
}


/*
 * Check if the machine is Domestic (Japanese) or Exported (elsewhere)
 *
 * Set P1 TH and P2 TH as outputs and their levels high (for MD PBC
 * compatibility), then read back P1 TH and P2 TH. If they are the same
 * as written, the console is exported, else domestic.
 *
 * Note: This leaves TH for both ports as outputs and set high... which
 * is what we want for using MD pads on the SMS. Using a light gun would
 * require changing the appropriate TH back to an input.
 */
BYTE check_sms_region()
{
    JoyCtrl = 0xF5; // set P1 TH and P2 TH as output and levels high
    if ((JoyPort2 & 0xC0) == 0xC0)
        return EXPORTED;
    return JAPANESE;
}

void vdp_set_vram_addr2(WORD addr)
{
    VdpCtrl = (addr & 0xFF);
    VdpCtrl = (addr >> 8) | CMD_VRAM_WRITE;
}

void main()
{
    BYTE temp;
    void (*bank1_dispatcher)(WORD) = (void (*)(WORD))0x4000;

    Frame1 = 1;

    mute_psg();

    games.count = count_games_on_gbac();
    games.firstShown = games.highlighted = 0;

    region = check_sms_region();

    // Make sure the display is off before we write to VRAM
    vdp_set_reg(REG_MODE_CTRL_2, 0xA0);

    load_font();

#ifdef PLAIN_BG
    puts("Neo SMS Menu", 10, 1, PALETTE0);
    puts("(c) NeoTeam 2011", 8, 2, PALETTE0);
#else
    bank1_dispatcher(TASK_LOAD_BG);
#endif

    // Print software (menu) and firmware versions
    puts("SW ", 24, 21, PALETTE0);
    puts(MENU_VERSION_STRING, 27, 21, PALETTE0);
    puts("FW ", 24, 22, PALETTE0);
    puts("1.00", 27, 22, PALETTE0);     // TODO: read version from CPLD

    puts("[I]  Run", 1, 21, PALETTE0);
    puts("[II] More options", 1, 22, PALETTE0);

    setup_vdp();

    puts_game_list();

    pad = padLast = 0;

#if 0
    temp = 0;
    while (temp != PAD_START)
    {
        //temp = pad1_get_2button();
        //temp = pad2_get_2button();
        temp = pad1_get_3button();
        //temp = pad2_get_3button();
        if (temp & PAD_START)
            puts("S", 11, 0, 4);
        else
            puts(" ", 11, 0, 4);
        if (temp & PAD_A)
            puts("A", 12, 0, 4);
        else
            puts(" ", 12, 0, 4);
        if (temp & PAD_C)
            puts("C", 13, 0, 4);
        else
            puts(" ", 13, 0, 4);
        if (temp & PAD_B)
            puts("B", 14, 0, 4);
        else
            puts(" ", 14, 0, 4);
        if (temp & PAD_RIGHT)
            puts("R", 15, 0, 4);
        else
            puts(" ", 15, 0, 4);
        if (temp & PAD_LEFT)
            puts("L", 16, 0, 4);
        else
            puts(" ", 16, 0, 4);
        if (temp & PAD_DOWN)
            puts("D", 17, 0, 4);
        else
            puts(" ", 17, 0, 4);
        if (temp & PAD_UP)
            puts("U", 18, 0, 4);
        else
            puts(" ", 18, 0, 4);

        vdp_wait_vblank();
    }
#endif
#if 0
    {
    WORD temp;

    temp = 0;
    while (temp != PAD_MODE)
    {
        temp = pad1_get_6button();
        //temp = pad2_get_6button();
        if (temp & PAD_MODE)
            puts("M", 7, 0, 4);
        else
            puts(" ", 7, 0, 4);
        if (temp & PAD_X)
            puts("X", 8, 0, 4);
        else
            puts(" ", 8, 0, 4);
        if (temp & PAD_Y)
            puts("Y", 9, 0, 4);
        else
            puts(" ", 9, 0, 4);
        if (temp & PAD_Z)
            puts("Z", 10, 0, 4);
        else
            puts(" ", 10, 0, 4);
        if (temp & PAD_START)
            puts("S", 11, 0, 4);
        else
            puts(" ", 11, 0, 4);
        if (temp & PAD_A)
            puts("A", 12, 0, 4);
        else
            puts(" ", 12, 0, 4);
        if (temp & PAD_C)
            puts("C", 13, 0, 4);
        else
            puts(" ", 13, 0, 4);
        if (temp & PAD_B)
            puts("B", 14, 0, 4);
        else
            puts(" ", 14, 0, 4);
        if (temp & PAD_RIGHT)
            puts("R", 15, 0, 4);
        else
            puts(" ", 15, 0, 4);
        if (temp & PAD_LEFT)
            puts("L", 16, 0, 4);
        else
            puts(" ", 16, 0, 4);
        if (temp & PAD_DOWN)
            puts("D", 17, 0, 4);
        else
            puts(" ", 17, 0, 4);
        if (temp & PAD_UP)
            puts("U", 18, 0, 4);
        else
            puts(" ", 18, 0, 4);

        vdp_wait_vblank();
    }
    }
#endif

    while (1)
    {
        temp = pad1_get_2button();

        // Only those that were pressed since the last time
        pad = temp & ~padLast;
        padLast = temp;

        if (pad & PAD_UP)
        {
            move_to_previous_game();
        }
        else if (pad & PAD_DOWN)
        {
            move_to_next_game();
        }

        vdp_wait_vblank();
    }
}


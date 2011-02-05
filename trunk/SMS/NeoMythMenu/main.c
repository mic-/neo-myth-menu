#include "sms.h"
#include "vdp.h"
#include "shared.h"
#include "font.h"


void mute_psg()
{
    PsgPort = 0x9F;     // Mute channel 0
    PsgPort = 0xBF;     // Mute channel 1
    PsgPort = 0xDF;     // Mute channel 2
    PsgPort = 0xFF;     // Mute channel 3
}


/*
 * Expand the 1-bit font and write to VRAM at
 * address 0000
 */
void load_font()
{
    WORD i;

    vdp_set_vram_addr(0x0000);

    for (i = 0; i < 960; i++)
    {
        VdpData = font[i] ^ 0xFF;
        VdpData = 0;        // Fill in the empty bitplanes
        VdpData = 0;        // ...
        VdpData = 0;        // ...
    }
}


void setup_vdp()
{
    vdp_set_reg(REG_MODE_CTRL_1, 0x04);     // Set mode 4
    vdp_set_reg(REG_MODE_CTRL_2, 0xE0);     // Screen on, VBlank int on
    vdp_set_reg(REG_HSCROLL, 0x00);         // Reset scrolling
    vdp_set_reg(REG_VSCROLL, 0x00);         // ...
    vdp_set_reg(REG_NAME_TABLE_ADDR, 0x07); // Nametable at 0x1800
    vdp_set_reg(REG_OVERSCAN_COLOR, 0);
    vdp_set_reg(REG_BG_PATTERN_ADDR, 0x07); // Needed for the SMS1 VDP, ignored for later versions
    vdp_set_reg(REG_COLOR_TABLE_ADDR, 0xFF); // Needed for the SMS1 VDP, ignored for later versions
}


void puts(const char *str, BYTE x, BYTE y, BYTE attributes)
{
    vdp_set_vram_addr(0x1800 + x + x + (y << 6));
    while (*str)
    {
        VdpData = (*str++) - ' ';
        VdpData = (attributes << 1);
    }
}


void puts_game_list()
{
    BYTE *p = (BYTE*)gbacGameList;
    BYTE row, shown;

    vdp_wait_vblank();

    shown = 0;
    row = 3;
    p += games.firstShown << 5;
    // Loop until we've shown the desired number of games, or
    // there are no more games in the list
    while ((*p != 0xFF) && (shown < NUMBER_OF_GAMES_TO_SHOW) &&
           ((shown + games.firstShown) < games.count))
    {
        if (games.highlighted == shown + games.firstShown)
            puts(&p[8], 1, row, PALETTE1); // the palette bit is bit 2 of the attribute
        else
            puts(&p[8], 1, row, PALETTE0);
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


void main()
{
    BYTE temp;

    Frame1 = 1;

    mute_psg();

    games.count = count_games_on_gbac();
    games.firstShown = games.highlighted = 0;

    region = check_sms_region();

    load_font();

    puts("NeoSmsMenu", 11, 1, PALETTE1);

    setup_vdp();

    vdp_set_cram_addr(0x0000);
    VdpData = 0;    // color 0
    VdpData = 0x25; // color 1 (blue)
    vdp_set_cram_addr(0x0010);
    VdpData = 0;    // color 16
    VdpData = 0x3F; // color 17 (white)

    puts_game_list();

    keys = keysRepeat = 0;

    while (1)
    {
        // All keys for joy1
        temp = (JoyPort1 & 0x3F) ^ 0x3F;

        // Only those that were pressed since the last time
        // we checked
        keys = temp & ~keysRepeat;
        keysRepeat = temp;

        if (keys & KEY_UP)
        {
            move_to_previous_game();
        }
        else if (keys & KEY_DOWN)
        {
            move_to_next_game();
        }
    }
}

#include "util.h"
#include "sms.h"
#include "vdp.h"
#include "pad.h"
#include "shared.h"
#include "font.h"
#include "neo2.h"
#include "neo2_map.h"


#define MENU_VERSION_STRING "0.13"

#define KEY_REPEAT_INITIAL_DELAY 15
#define KEY_REPEAT_DELAY 7

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

    disable_ints;
    vdp_set_vram_addr(0x0000);

    for (i = 0; i < 960; i++)
    {
#ifdef PLAIN_BG
        BYTE b;
        b = font[i] ^ 0xFF;
        VdpData = b;
        VdpData = 0;
        VdpData = 0;
#else
        BYTE b,c;
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

    init_sat();

    vdp_set_reg(REG_MODE_CTRL_1, 0x04);     // Set mode 4, Line ints off
    vdp_set_reg(REG_MODE_CTRL_2, 0xE0);     // Screen on, Frame ints on
    vdp_set_reg(REG_HSCROLL, 0x00);         // Reset scrolling
    vdp_set_reg(REG_VSCROLL, 0x00);         // ...
    vdp_set_reg(REG_NAME_TABLE_ADDR, 0x07); // Nametable at 0x1800
    vdp_set_reg(REG_OVERSCAN_COLOR, 0x10);
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

    enable_ints();
}


/*
 * Prints the value val in hexadecimal form at position x,y
 */
void print_hex(BYTE val, BYTE x, BYTE y)
{
    BYTE lo,hi;

    vdp_set_vram_addr(0x1800 + (x << 1) + (y << 6));

    hi = (val >> 4);
    lo = val & 0x0F;
    lo += 16; hi += 16;
    if (lo > 25) lo += 7;
    if (hi > 25) hi += 7;
    VdpData = hi; VdpData = 0;
    VdpData = lo; VdpData = 0;
}


void puts_game_list()
{
    BYTE *p = (BYTE*)gbacGameList;
    BYTE row, col, show;
    static BYTE temp[32*2*NUMBER_OF_GAMES_TO_SHOW];

    // Print the current directory name
    // TODO: Handle this properly when SD support has been
    //       implemented
    //puts("/", 1, 5, PALETTE0);

    // clear all lines
    memset_asm(temp, 0, 32*2*NUMBER_OF_GAMES_TO_SHOW);

    show = (games.count < NUMBER_OF_GAMES_TO_SHOW) ? games.count : NUMBER_OF_GAMES_TO_SHOW;
    row = 0;
    p += games.firstShown << 5;
    // Loop until we've shown the desired number of games, or
    // there are no more games in the list
    while (show)
    {
        for (col=0; col<22; col++)
        {
            if (!p[col+8])
                break;
            temp[row*32*2 + col*2 + 2] = p[col+8] - 32;
            temp[row*32*2 + col*2 + 3] = (games.highlighted == (games.firstShown + row)) ? PALETTE1<<1 : PALETTE0<<1;
        }

        row++;
        show--;
        p += 0x20;
    }

    // wait for vblank and copy all at once
    vdp_wait_vblank();
    vdp_copy_to_vram(0x1800 + (7 << 6), temp, 32*2*NUMBER_OF_GAMES_TO_SHOW);
}


void dump_hex(WORD addr)
{
    BYTE *p = (BYTE*)addr;
    BYTE row,col,c,d;

    vdp_wait_vblank();

    row = 7;
    for (; row < 15; row++)
    {
        vdp_set_vram_addr(0x1800 + (row << 6) + 8);
        for (col = 0; col < 8; col++)
        {
            c = *p++;
            d = (c >> 4);
            c &= 0x0F;
            c += 16; d += 16;
            if (c > 25) c += 7;
            if (d > 25) d += 7;
            VdpData = d; VdpData = 0;
            VdpData = c; VdpData = 0;
        }
    }
}

/*
 * Move to the next entry in the games list
 */
void move_to_next_game()
{
    if (games.highlighted < (games.count - 1))
        games.highlighted++;

    // Check if the games list needs to be scrolled
    if ( ((games.highlighted - games.firstShown) >= (NUMBER_OF_GAMES_TO_SHOW >> 1)) &&
         (games.firstShown  < games.count - NUMBER_OF_GAMES_TO_SHOW) )
        games.firstShown++;

    puts_game_list();
}


/*
 * Move to the previous entry in the games list
 */
void move_to_previous_game()
{
    if (games.highlighted > 0)
        games.highlighted--;

    // Check if the games list needs to be scrolled
    if ( (games.firstShown) &&
         ((games.highlighted - games.firstShown) < (NUMBER_OF_GAMES_TO_SHOW >> 1)) )
        games.firstShown--;

    puts_game_list();
}



/*
 * Count the number of games on the GBA card
 */
WORD count_games_on_gbac()
{
    BYTE *p = (BYTE*)gbacGameList;
    WORD count = 0;

    while ((*p != 0xFF) && (count < 512))
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


#if 0
void test_strings()
{
    static char buf[32];
    static char filename[8];
    const char* ext;

    strcpy_asm(buf,"/games/");
    strcat_asm(buf,"game");
    strcat_asm(buf,".sms");
    puts(buf,8,1,PALETTE0);

    strncpy_asm(buf,"/games/",7);
    strncat_asm(buf,"game2",5);
    strncat_asm(buf,".sms",4);
    puts(buf,8,2,PALETTE0);

    strcpy_asm(filename,"game.sms");
    strcpy_asm(buf,"File ext is :");

    ext = get_file_extension_asm(filename);

    if(ext)
        strcat_asm(buf,ext);
    else
        strcat_asm(buf,"Not found");

    puts(buf,8,3,PALETTE0);

    while(1){}
}
#endif


void main()
{
    BYTE temp;
    WORD i;
    BYTE ftype;
    BYTE padUpReptDelay;
    BYTE padDownReptDelay;
    char type[2];

    void (*bank1_dispatcher)(WORD) = (void (*)(WORD))0x4000;

    MemCtrl = 0xA8;

    neoMode = 0;

    Frame1 = 2;
    // Copy code from ROM to RAM
    for (i = 0; i < 0x400; i++)
    {
        *(volatile BYTE*)(0xD000+i) = *(volatile BYTE*)(0x4000+i);
    }

    temp = pfn_neo2_check_card();

    Frm2Ctrl = FRAME2_AS_ROM;

    Frame1 = 3;
    ftype = *(volatile BYTE*)0x7FF0; // flash mem type at 0xFFF0: 0 = newer(C), 1 = new(B), 2 = old(A)
    type[0] = 'C' - ftype;
    type[1] = 0;

    Frame1 = 1;
    Frame2 = 2;

    mute_psg();

    games.count = count_games_on_gbac();
    games.firstShown = games.highlighted = 0;

    region = check_sms_region();

    // Make sure the display is off before we write to VRAM
    vdp_set_reg(REG_MODE_CTRL_2, 0xA0);

    // Clear the nametable
    vdp_set_vram(0x1800, 0, 32*24*2);

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

    // Print some Myth info
    print_hex(idLo, 24, 20);
    print_hex(idHi, 26, 20);
    print_hex(*(BYTE*)0xC000, 28, 20);
    // Print flash type
    puts(type, 30, 20, PALETTE0);

    setup_vdp();

    #if 0
    test_strings();
    #endif

    //dump_hex(0xB000);
    puts_game_list();

    pad = padLast = 0;
    padUpReptDelay = KEY_REPEAT_INITIAL_DELAY;
    padDownReptDelay = KEY_REPEAT_INITIAL_DELAY;

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
        else if (padLast & PAD_UP)
        {
            if (0 == --padUpReptDelay)
            {
                padUpReptDelay = KEY_REPEAT_DELAY;
                move_to_previous_game();
            }
        }
        else
        {
            padUpReptDelay = KEY_REPEAT_INITIAL_DELAY;
        }


        if (pad & PAD_DOWN)
        {
            move_to_next_game();
        }
        else if (padLast & PAD_DOWN)
        {
            if (0 == --padDownReptDelay)
            {
                padDownReptDelay = KEY_REPEAT_DELAY;
                move_to_next_game();
            }
        }
        else
        {
            padDownReptDelay = KEY_REPEAT_INITIAL_DELAY;
        }

        if (pad & PAD_SW1)
        {
            volatile GbacGameData* gameData;
            volatile BYTE* p;

            // Copy the game info data to somewhere in RAM
            gameData = (volatile GbacGameData*)0xC800;
            p = (volatile BYTE*)0xB000;
            p += games.highlighted << 5;

            gameData->mode = ftype; // we know mode is ALWAYS 0, so pass flash type here
            gameData->typ = p[1];
            gameData->size = p[2] >> 4;
            gameData->bankHi = p[2] & 0x0F;
            gameData->bankLo = p[3];
            gameData->sramBank = p[4] >> 4;
            gameData->sramSize = p[4] & 0x0F;
            gameData->cheat[0] = p[5];
            gameData->cheat[1] = p[6];
            gameData->cheat[2] = p[7];

            pfn_neo2_run_game_gbac();
        }

        vdp_wait_vblank();
    }
}


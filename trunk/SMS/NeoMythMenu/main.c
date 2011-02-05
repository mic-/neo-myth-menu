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
	BYTE *gameList;
	BYTE row, shown;

#ifdef EMULATOR
	gameList = (char*)&dummyGameList[0];
#else
	gameList = (char*)0xB000;
#endif

	vdp_wait_vblank();

	shown = 0;
	row = 3;
	gameList += games.firstShown << 5;
	// Loop until we've shown the desired number of games, or
	// there are no more games in the list
	while ((*gameList != 0xFF) && (shown < GAMES_TO_SHOW) &&
	       ((shown + games.firstShown) < games.size))
	{
		if (games.highlighted == shown + games.firstShown)
			puts(&gameList[8], 1, row, 4);	// the palette bit is bit 2 of the attribute
		else
			puts(&gameList[8], 1, row, 0);
		row++;
		shown++;
		gameList += 0x20;
	}
}


// Count the number of games on the GBA card, and initialize the games struct
void count_games_on_gbac()
{
	BYTE *gameList;
#ifdef EMULATOR
	gameList = (char*)&dummyGameList[0];
#else
	gameList = (BYTE*)0xB000;
#endif

	games.size = 0;
	while (*gameList != 0xFF)
	{
		gameList += 0x20;
		games.size++;
	}

	games.firstShown = 0;
	games.highlighted = 0;
}


void main()
{
    Frame1 = 1;

    load_font();

    puts("NeoSmsMenu", 11, 1, 4);

    setup_vdp();

    mute_psg();

    vdp_set_cram_addr(0x0000);
    VdpData = 0; 	// color 0
    VdpData = 0x2A; // color 1
	vdp_set_cram_addr(0x0010);
	VdpData = 0;	// color 16
	VdpData = 0x3F;	// color 17

	count_games_on_gbac();
	puts_game_list();

    while (1) {}
}

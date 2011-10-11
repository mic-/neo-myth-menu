#include "util.h"
#include "sms.h"
#include "vdp.h"
#include "pad.h"
#include "shared.h"
#include "font.h"
#include "neo2.h"
#include "neo2_map.h"
#include "diskio.h"
#include "pff.h"
#include "pff_map.h"

#undef TEST_CHEAT_INPUTBOX
#define MENU_VERSION_STRING "0.17"
#define KEY_REPEAT_INITIAL_DELAY 15
#define KEY_REPEAT_DELAY 7
#define SD_DEFAULT_INFO_FETCH_TIMEOUT 30

#define LEFT_MARGIN 3
#define INSTRUCTIONS_Y 20

extern FATFS sdFatFs; 

/*
 * Use the plain single-colored background instead of the pattered one.
 * If you remove this define you should also modify the Makefile to
 * link against font2.rel instead of font.rel.
 */
#define PLAIN_BG

void mute_sound()
{
    BYTE i, j;

    // mute the PSG
    PsgPort = 0x9F;     // Mute channel 0
    PsgPort = 0xBF;     // Mute channel 1
    PsgPort = 0xDF;     // Mute channel 2
    PsgPort = 0xFF;     // Mute channel 3

    Neo2FmOn = 0x00;    // make sure the FM chip is enabled
    // mute the YM2413
    for (i=0; i<9; i++)
    {
        FMAddr = 0x20 +i;
        for (j=0; j<8; j++) ;
        FMData = 0x00; // key-off
        for (j=0; j<2; j++) ;

        FMAddr = 0x30 + i;
        for (j=0; j<8; j++) ;
        FMData = 0xFF; // full attenuation
        for (j=0; j<2; j++) ;
    }
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
        BYTE b,c;
        b = font[i] ^ 0xFF;
        c = ~b;
        VdpData = c;
        VdpData = c;
        VdpData = c;
#else
        BYTE b,c;
        b = font[i+i];      // Bitplane 0
        VdpData = b;
        c = font[i+i+1];    // Bitplane 1
        VdpData = c;
        VdpData = b & c;
#endif
        VdpData = b; //0;
    }
    enable_ints;
}

/*
 * Initializes the Sprite Attribute Table
 */
void init_sat()
{
    BYTE i;

    vdp_set_vram_addr(0x3800);

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
    vdp_set_reg(REG_NAME_TABLE_ADDR, 0x0D); // Nametable at 0x3000
    vdp_set_reg(REG_OVERSCAN_COLOR, 0x00);
    vdp_set_reg(REG_BG_PATTERN_ADDR, 0xFF); // Needed for the SMS1 VDP, ignored for later versions
    vdp_set_reg(REG_COLOR_TABLE_ADDR, 0xFF); // Needed for the SMS1 VDP, ignored for later versions
    vdp_set_reg(REG_LINE_COUNT, 0xFF);      // Line ints off
    vdp_set_reg(REG_SAT_ADDR, 0x71);        // Sprite attribute table at 0x3800

#ifdef PLAIN_BG
    vdp_set_color(16, 0, 0, 0);             // Backdrop color
    vdp_set_color(23, 0, 0, 0);
    vdp_set_color(24, 3, 3, 3);
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

    vdp_set_vram_addr(MENU_NAMETABLE + (x << 1) + (y << 6));

    hi = (val >> 4);
    lo = val & 0x0F;
    lo += 16; hi += 16;
    if (lo > 25) lo += 7;
    if (hi > 25) hi += 7;
    VdpData = hi; VdpData = 0;
    VdpData = lo; VdpData = 0;
}


void dump_hex(WORD addr)
{
    BYTE *p = (BYTE*)addr;
    BYTE row,col,c,d;

    vdp_wait_vblank();

    row = 7;
    for (; row < 15; row++)
    {
        vdp_set_vram_addr(MENU_NAMETABLE + (row << 6) + 8);
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


void vdp_delay(BYTE count)
{
    /*keep vdp busy for a bit*/
    while(count--)
        vdp_wait_vblank();
}


void clear_list_surface() __naked
{
    // Fills the buffer with tile# 0, palette 1
    __asm
    di
    ld      hl,#_generic_list_buffer
    ld      bc,#LIST_BUFFER_SIZE/2
    ld      de,#0x0008
    clear_list_surface_loop:
    ld      (hl),d
    inc     hl
    ld      (hl),e
    inc     hl
    dec     bc
    ld      a,b
    or      a,c
    jp      nz,clear_list_surface_loop
    ei
    ret
    __endasm;
}


void present_list_surface()
{
    vdp_wait_vblank();

    // Copy the buffer to VRAM, skipping the left and right
    // margin (3 columns each)
    __asm
    di
    ld      de,#MENU_NAMETABLE+576+LEFT_MARGIN*2
    ld      hl,#_generic_list_buffer+2
    ld      c,#NUMBER_OF_GAMES_TO_SHOW
    present_list_surface_loop:
    ld      a,e
    out     (0xBF),a
    ld      a,d
    or      a,#0x40
    out     (0xBF),a
    ld      b,#64-LEFT_MARGIN*4
    push    bc
    ld      c,#0xBE
    otir
    pop     bc
    ld      a,#LEFT_MARGIN*4
    add     a,l
    ld      l,a
    ld      a,#0
    adc     a,h
    ld      h,a
    ld      a,#64
    add     a,e
    ld      e,a
    ld      a,#0
    adc     a,d
    ld      d,a
    dec     c
    jp      nz,present_list_surface_loop
    ei
    __endasm;
}


void puts_active_list()
{
    BYTE* temp = generic_list_buffer;
    const BYTE* p;
    BYTE row, col, show;
    WORD offs;

    vdp_wait_vblank();
    puts("                          ", 3, 7, PALETTE1);

    if(MENU_STATE_GAME_GBAC == menu_state)
    {
        puts("GBAC:/", LEFT_MARGIN, 7, PALETTE1);

        if(!games.count)
        {
            puts("No games found on GBAC",3, 11, PALETTE0);
            return;
        }
    }
    else if(MENU_STATE_GAME_SD == menu_state)
        puts("SD:/", LEFT_MARGIN, 7, PALETTE1);//Change this to SD path
    else if(MENU_STATE_OPTIONS == menu_state)
        puts("Options:", LEFT_MARGIN, 7, PALETTE1);
    else
        puts("Media Player", LEFT_MARGIN, 7, PALETTE1);

    if((MENU_STATE_GAME_GBAC == menu_state) || (MENU_STATE_MEDIA_PLAYER == menu_state))
    {
        clear_list_surface();
        row = 0;
        show = (games.count < NUMBER_OF_GAMES_TO_SHOW) ? games.count : NUMBER_OF_GAMES_TO_SHOW;
        p = (const BYTE*)gbacGameList;
        p += games.firstShown << 5;

        while (show)
        {
            offs = row*32*2;

            for (col=0; col<22; col++)
            {
                if (!p[col+8])
                    break;
                temp[offs + col*2 + 2] = p[col+8] - 32;
                temp[offs + col*2 + 3] = (games.highlighted == (games.firstShown + row)) ? PALETTE0<<1 : PALETTE1<<1;
            }

            row++;
            show--;
            p += 0x20;
        }

        present_list_surface();
    }
    else if(MENU_STATE_OPTIONS == menu_state)
    {
        clear_list_surface();
        row = 0;
        show = 0;

        while(show < options_count)
        {
            BYTE attr = (show == options_highlighted) ? PALETTE0<<1 : PALETTE1<<1;
            BYTE ix;

            offs = row*32*2;
            col = 0;
            ix = 0;
            while (options[show].name[ix])
            {
                temp[offs + col*2 + 2] = options[show].name[ix] - 32;
                temp[offs + col*2 + 3] = attr;
                col++;
                ix++;
            }

            if(OPTION_TYPE_SETTING == options_get_type(&options[show]))
            {
                if(0 == options_get_state(&options[show]))
                {
                    ix =0;
                    while (options[show].cond0_bhv[ix])
                    {
                        temp[offs + col*2 + 2] = options[show].cond0_bhv[ix] - 32;
                        temp[offs + col*2 + 3] = attr;
                        col++;
                        ix++;
                    }
                }
                else
                {
                    ix =0;
                    while (options[show].cond1_bhv[ix])
                    {
                        temp[offs + col*2 + 2] = options[show].cond1_bhv[ix] - 32;
                        temp[offs + col*2 + 3] = attr;
                        col++;
                        ix++;
                    }
                }
            }

            row++;
            show++;
        }

        present_list_surface();
    }
}


/*
 * Move to the next entry in the games list
 */
void move_to_next_list_item()
{
    if((MENU_STATE_GAME_GBAC == menu_state) || (MENU_STATE_MEDIA_PLAYER == menu_state))
    {
        if (games.highlighted < (games.count - 1))
            games.highlighted++;

        // Check if the games list needs to be scrolled
        if ( ((games.highlighted - games.firstShown) >= ((NUMBER_OF_GAMES_TO_SHOW >> 1) + 1)) &&
             (games.firstShown  < (games.count - NUMBER_OF_GAMES_TO_SHOW)) )
            games.firstShown++;

        puts_active_list();
    }
    else if(MENU_STATE_OPTIONS == menu_state)
    {
        options_highlighted++;

        if(options_highlighted > options_count-1)
            options_highlighted = options_count-1;

        puts_active_list();
    }
}


/*
 * Move to the previous entry in the games list
 */
void move_to_previous_list_item()
{
    if((MENU_STATE_GAME_GBAC == menu_state) || (MENU_STATE_MEDIA_PLAYER == menu_state))
    {
        if (games.highlighted > 0)
            games.highlighted--;

        // Check if the games list needs to be scrolled
        if ( (games.firstShown > games.highlighted) ||
             ((games.firstShown) &&
             ((games.highlighted - games.firstShown) < (NUMBER_OF_GAMES_TO_SHOW >> 1))) )
            games.firstShown--;

        puts_active_list();
    }
    else if(MENU_STATE_OPTIONS == menu_state)
    {
        if(options_highlighted > 0)
            options_highlighted--;

        puts_active_list();
    }
}


/*
 * Move to the next page in the games list
 */
void move_to_next_page()
{
    BYTE select;

    if((MENU_STATE_GAME_GBAC == menu_state) || (MENU_STATE_MEDIA_PLAYER == menu_state))
    {
        select = games.highlighted - games.firstShown;

        // sanity check
        if (games.count <= NUMBER_OF_GAMES_TO_SHOW)
            return;

        if (games.firstShown < (games.count - NUMBER_OF_GAMES_TO_SHOW*2))
            games.firstShown += NUMBER_OF_GAMES_TO_SHOW;
        else
            games.firstShown = games.count - NUMBER_OF_GAMES_TO_SHOW;

        games.highlighted = games.firstShown + select;

        puts_active_list();
    }
}


/*
 * Move to the previous page in the games list
 */
void move_to_previous_page()
{
    BYTE select;

    if((MENU_STATE_GAME_GBAC == menu_state) || (MENU_STATE_MEDIA_PLAYER == menu_state))
    {
        select = games.highlighted - games.firstShown;

        // sanity check
        if (games.count <= NUMBER_OF_GAMES_TO_SHOW)
            return;

        if (games.firstShown > NUMBER_OF_GAMES_TO_SHOW)
            games.firstShown -= NUMBER_OF_GAMES_TO_SHOW;
        else
            games.firstShown = 0;

        games.highlighted = games.firstShown + select;

        puts_active_list();
    }
}


/*
 * Count the number of games on the GBA card
 */
WORD count_games_on_gbac()
{
    const BYTE* p = (const BYTE*)gbacGameList;
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


BYTE is_time_to_fetch_sd_file_info()
{
    if(sd_fetch_info_timeout > 0)
    {
        --sd_fetch_info_timeout;
        return 0;
    }

    sd_fetch_info_timeout = SD_DEFAULT_INFO_FETCH_TIMEOUT;
    return 1;
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
    puts(buf,8,1,PALETTE1);

    strncpy_asm(buf,"/games/",7);
    strncat_asm(buf,"game2",5);
    strncat_asm(buf,".sms",4);
    puts(buf,8,2,PALETTE1);

    strcpy_asm(filename,"game.sms");
    strcpy_asm(buf,"File ext is :");

    ext = get_file_extension_asm(filename);

    if(ext)
        strcat_asm(buf,ext);
    else
        strcat_asm(buf,"Not found");

    puts(buf,8,3,PALETTE1);

    if(ext)
    {
        if(memcmp_asm(ext,"sms",3)==0)
            strcpy_asm(buf,"Extension was .SMS!");
        else
            strcpy_asm(buf,"Extension wasn't .SMS!");

        puts(buf,8,4,PALETTE1);
    }

    print_hex(strlen_asm("Hello World"),8,8);
    if(memcmp_asm("Hello World","Hello World",strlen_asm("Hello World")) == 0)
        puts("Hello World matches",8,5,PALETTE1);
    else
        puts("Hello World doesn't match",8,5,PALETTE1);

    while(1){}
}
#endif

void sync_state()
{
	switch (menu_state)
	{
		case MENU_STATE_GAME_GBAC:
			games.highlighted = 0;
			vdp_wait_vblank();
			puts("[L/R/U/D] Navigate  ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
			puts("[I] Run [II] Options", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
		break;
		case MENU_STATE_OPTIONS:
			options_highlighted = 0;
			vdp_wait_vblank();
			puts("[L/R] Change option ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
			puts("[I] Run [II] SD card", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
		break;
		case MENU_STATE_GAME_SD:
			vdp_wait_vblank();
			puts("[L/R/U/D] Navigate  ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
			puts("[I] Run [II] Media  ", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
		break;
		case MENU_STATE_MEDIA_PLAYER:
			vdp_wait_vblank();
			puts("[L/R/U/D] Navigate  ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
			puts("[I] Play [II] Flash ", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
		break;
	}
}
void handle_action_button(BYTE button)
{
    if (MENU_STATE_OPTIONS == menu_state)
    {
        // in the options state, the controls are
        // [DOWN]/[UP] = next/prev option
        // [LEFT]/[RIGHT] = change option
        // [I] = run game
        // [II] = change menu state

        if ((button == PAD_LEFT) || (button == PAD_RIGHT))
        {
            if(options_highlighted < options_count)//sanity check
            {
                if(OPTION_TYPE_SETTING == options_get_type(&options[options_highlighted]))
                {
                    if(options_get_state(&options[options_highlighted]))
                        options_set_state(&options[options_highlighted],0);
                    else
                        options_set_state(&options[options_highlighted],1);
                }
#if 0
                else if(OPTION_TYPE_ROUTINE == options_get_type(&options[options_highlighted]))
                {
                    //TODO
                }
                else if(OPTION_TYPE_CHEAT == options_get_type(&options[options_highlighted]))
                {
                    //TODO
                }
#endif
                puts_active_list();
            }
        }

    }
#if 0
    else if(MENU_STATE_MEDIA_PLAYER == menu_state)
    {
    }
#endif

    // fall through to game flash browser state
    // in the game flash browser state, the controls are
    // [DOWN]/[UP] = next/prev game
    // [LEFT]/[RIGHT] = next/prev page of games
    // [I] = run game
    // [II] = change menu state

    if(button == PAD_SW1)
    {
        volatile GbacGameData* gameData;
        volatile BYTE* p;
        BYTE fm = options_get_state(&options[fm_enabled_option_idx]);
        BYTE reset = options_get_state(&options[reset_to_menu_option_idx]);

        // Copy the game info data to somewhere in RAM
        gameData = (volatile GbacGameData*)0xC800;
        p = (volatile BYTE*)0xB000;
        p += games.highlighted << 5;

        gameData->mode = GDF_RUN_FROM_FLASH;
        gameData->type = flash_mem_type;
        gameData->size = p[2] >> 4;
        gameData->bankHi = p[2] & 0x0F;
        gameData->bankLo = p[3];
        gameData->sramBank = p[4] >> 4;
        gameData->sramSize = p[4] & 0x0F;
        gameData->cheat[0] = p[5];
        gameData->cheat[1] = p[6];
        gameData->cheat[2] = p[7];
        pfn_neo2_run_game_gbac(fm,reset); // never returns
    }
    else if(button == PAD_SW2)
    {
        ++menu_state;

        if(menu_state > MENU_STATES-1)
            menu_state = MENU_STATE_TOP;

		sync_state();
        puts_active_list();
    }
}

#ifdef TEST_CHEAT_INPUTBOX
void cheat_inputbox(char* dst_buf,BYTE* dst_size,const char* title)
{
	BYTE key;
	BYTE addr;
	BYTE abs_addr;

	//clear list surf
	clear_list_surface();
	present_list_surface();

	//set default str
	memset_asm(dst_buf,'_',10);													//TODO make this an argument
	dst_buf[10] = '\0';
	addr = 0;

	//Render title
	puts(title,LEFT_MARGIN + (((22/2) - (strlen_asm(title)/2))) + 1,8,PALETTE1);

	//Render default str
	abs_addr = LEFT_MARGIN + (((22/2) - (strlen_asm(dst_buf)/2))) + 1;
	puts(dst_buf,abs_addr,12,PALETTE1);
	putsn(dst_buf + addr,abs_addr + addr,12,PALETTE1,1);

	while(1)
	{
        key = pad1_get_2button();
        pad = key & ~padLast;
        padLast = key;

		if(pad&PAD_SW1)
			break;
		else if (pad & PAD_UP)
		{
			++dst_buf[addr];
			putsn(dst_buf + addr,abs_addr + addr,12,PALETTE1,1);
		}
		else if (pad & PAD_DOWN)
		{
			--dst_buf[addr];
			putsn(dst_buf + addr,abs_addr + addr,12,PALETTE1,1);
		}
		else if (pad & PAD_RIGHT)
		{
			if(addr < 9)
			{
				++addr;
				puts(dst_buf,abs_addr,12,PALETTE1);
				putsn(dst_buf + addr,abs_addr + addr,12,PALETTE0,1);	
			}
		}
		else if (pad & PAD_LEFT)
		{
			if(addr)
			{
				--addr;
				puts(dst_buf,abs_addr,12,PALETTE1);
				putsn(dst_buf + addr,abs_addr + addr,12,PALETTE0,1);
			}
		}
	}
	*dst_size = addr;

	addr = 0;
	while(addr < 10)
	{
		if(addr == '_')
			break;

		++addr;
	}

	dst_buf[addr] = '\0';

	puts("                      ",LEFT_MARGIN,8,PALETTE1);
	clear_list_surface();
	sync_state();
	vdp_delay(2);
}
#endif

void import_std_options()
{
    options_init();

    fm_enabled_option_idx = options_count;
    options_add("FM : ","off","on",OPTION_TYPE_SETTING,1);
    reset_to_menu_option_idx = options_count;
    options_add("Reset to menu : ","off","on",OPTION_TYPE_SETTING,1);
}



void main()
{
    BYTE temp;
    BYTE padUpReptDelay;
    BYTE padDownReptDelay;
    char type[2];

    void (*bank1_dispatcher)(WORD) = (void (*)(WORD))0x4000;

    MemCtrl = 0xA8;
    menu_state = MENU_STATE_GAME_GBAC;  //start off with gbac game state
    sd_fetch_info_timeout = SD_DEFAULT_INFO_FETCH_TIMEOUT;
    neoMode = 0;

    // Copy neo2 code from ROM to RAM
    Frame1 = 2;
    memcpy_asm(0xD000, 0x4000, 0x700);

    temp = pfn_neo2_check_card();

    Frm2Ctrl = FRAME2_AS_ROM;

    Frame1 = 3;
    flash_mem_type = *(volatile BYTE*)0x7FF0; // flash mem type at 0xFFF0: 0 = newer(C), 1 = new(B), 2 = old(A)
    type[0] = 'C' - flash_mem_type;
    type[1] = 0;

    Frame1 = 1;
    Frame2 = 2;

    games.count = count_games_on_gbac();
    games.firstShown = games.highlighted = 0;

    region = check_sms_region();

    // make sure the sound is off
    mute_sound();

    // Clear the nametable
    // Make sure the display is off before we write to VRAM
    vdp_set_reg(REG_MODE_CTRL_2, 0xA0);

    vdp_set_vram(MENU_NAMETABLE, 0, 32*24*2);

    load_font();

#ifdef PLAIN_BG
    bank1_dispatcher(TASK_LOAD_BG);
    puts("Neo SMS Menu", LEFT_MARGIN, 1, PALETTE1);
    puts("(c) NeoTeam 2011", LEFT_MARGIN, 2, PALETTE1);
#else
    bank1_dispatcher(TASK_LOAD_BG);
#endif

    // Print software (menu) and firmware versions
    puts(MENU_VERSION_STRING, 20, 1, PALETTE1);
    puts("/", 24, 1, PALETTE1);
    puts("1.04", 25, 1, PALETTE1);  // TODO: read version from CPLD

    puts("[L/R/U/D] Navigate  ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
    puts("[I] Run [II] Options", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);

    // Print some Myth info
    /*print_hex(idLo, 24, 20);
    print_hex(idHi, 26, 20);
    print_hex(*(BYTE*)0xC000, 28, 20);*/

    // Print flash type
    puts(type, 24, 2, PALETTE1);

    setup_vdp();

    vdpSpeed = vdp_check_speed();

    // Print the console model (japanese, us, european)
    vdp_wait_vblank();
    if (region == JAPANESE)
        puts("/Jap", 25, 2, PALETTE1);
    else if (vdpSpeed == NTSC)
        puts("/U.S", 25, 2, PALETTE1);
    else
        puts("/Eur", 25, 2, PALETTE1);

    #if 0
    test_strings();
    #endif
	
	#ifdef TEST_CHEAT_INPUTBOX
	cheat_inputbox(generic_list_buffer,0,"Enter cheat code:");
	#endif

    #if 0
    {
        BYTE test1[16];
        BYTE test2[16];
        strcpy_asm(test1, "This is test 1");
        pfn_neo2_ram_to_sram(0x03, 0xC800, test1, 16);
        pfn_neo2_sram_to_ram(test2, 0x03, 0xC800, 16);
        test2[15] = 0;
        vdp_wait_vblank();
        puts(test2, 9, 17, PALETTE1);
    }
    #endif

    #if 0
    {
        BYTE test1[16];
        BYTE test2[16];
        strcpy_asm(test1, "This is test 2");
        pfn_neo2_ram_to_psram(0x12, 0x8800, test1, 16);
        pfn_neo2_psram_to_ram(test2, 0x12, 0x8800, 16);
        test2[15] = 0;
        vdp_wait_vblank();
        puts(test2, 9, 18, PALETTE1);
    }
    #endif

    /**** SD card test ****/
    temp = pfn_neo2_init_sd();
    Frame2 = 5;
    temp = pfn_pf_mount(&sdFatFs);
    Frame1 = 1;
    dump_hex((WORD)&diskioPacket[0]);
    print_hex(cardType, 2, 3);
    print_hex(temp, 4, 3);
    while (1) ;
    /**********************/
    
    //dump_hex(0xB000);
    puts_active_list();

    pad = padLast = 0;
    padUpReptDelay = KEY_REPEAT_INITIAL_DELAY;
    padDownReptDelay = KEY_REPEAT_INITIAL_DELAY;

    import_std_options();

    while (1)
    {
        temp = pad1_get_2button();

        // Only those that were pressed since the last time
        pad = temp & ~padLast;
        padLast = temp;

        if (pad & PAD_UP)
        {
            move_to_previous_list_item();
        }
        else if (padLast & PAD_UP)
        {
            if (0 == --padUpReptDelay)
            {
                padUpReptDelay = KEY_REPEAT_DELAY;
                move_to_previous_list_item();
            }
        }
        else
        {
            padUpReptDelay = KEY_REPEAT_INITIAL_DELAY;
        }


        if (pad & PAD_DOWN)
        {
            move_to_next_list_item();
        }
        else if (padLast & PAD_DOWN)
        {
            if (0 == --padDownReptDelay)
            {
                padDownReptDelay = KEY_REPEAT_DELAY;
                move_to_next_list_item();
            }
        }
        else
        {
            padDownReptDelay = KEY_REPEAT_INITIAL_DELAY;
        }

        if (pad & PAD_LEFT)
        {
            if (MENU_STATE_OPTIONS == menu_state)
                handle_action_button(PAD_LEFT);
            else
                move_to_previous_page();
        }

        if (pad & PAD_RIGHT)
        {
            if (MENU_STATE_OPTIONS == menu_state)
                handle_action_button(PAD_RIGHT);
            else
                move_to_next_page();
        }

        if (pad & PAD_SW1)
            handle_action_button(PAD_SW1);
        else if (pad & PAD_SW2)
            handle_action_button(PAD_SW2);

        vdp_wait_vblank();
    }
}


/*should be moved to another bank?*/
BYTE options_get_state(Option* option)
{
    return (option->encoded_info & 0x0F);
}


BYTE options_get_type(Option* option)
{
    return (option->encoded_info >> 4);
}


volatile void options_set_state(Option* option,BYTE new_state)
{
    option->encoded_info = (option->encoded_info & 0xF0) | (new_state & 0x0F);
}


volatile void options_set_type(Option* option,BYTE new_type)
{
    option->encoded_info = (new_type << 4) | (option->encoded_info & 0x0F);
}


Option* options_add(const char* name,const char* cond0_bhv,const char* cond1_bhv,BYTE type,BYTE state)
{
    Option* option;

    if(options_count >= MAX_OPTIONS)
        return 0;

    option = &options[options_count++];
    strcpy_asm(option->name,name);
    strcpy_asm(option->cond0_bhv,cond0_bhv);
    strcpy_asm(option->cond1_bhv,cond1_bhv);
    memset_asm(option->user_data,0,4);
    option->encoded_info = (type << 4) | (state & 0x0F);

    return option;
}


Option* options_add_ex(const char* name,const char* cond0_bhv,const char* cond1_bhv,BYTE type,BYTE state,WORD user_data0,WORD user_data1)
{
    Option* option = options_add(name,cond0_bhv,cond1_bhv,type,state);

    if(option == 0)
        return 0;

    *(volatile WORD*)&option->user_data[0] = user_data0;
    *(volatile WORD*)&option->user_data[2] = user_data1;

    return option;
}


void options_init()
{
    options_highlighted = 0;
    options_count = 0;
}




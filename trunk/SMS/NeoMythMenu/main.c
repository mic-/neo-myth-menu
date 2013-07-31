/*
 * Enhanced menu for the Neoflash SMS/MKIII Myth
 */
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
#include "sd_utils.h"
#include "vgm_player_map.h"

#undef TEST_SD_BLOCK_WRITE
#undef TEST_CHEAT_INPUTBOX
#define MENU_VERSION_STRING "1.10"
#define KEY_REPEAT_INITIAL_DELAY 15
#define KEY_REPEAT_DELAY 7
#define SD_DEFAULT_INFO_FETCH_TIMEOUT 30

#define LEFT_MARGIN 3
#define INSTRUCTIONS_Y 20

extern FATFS sdFatFs;
extern unsigned char pfmountbuf[36];
extern WCHAR LfnBuf[_MAX_LFN + 1];
void option_cb_inc_sram_bank();
void option_cb_dec_sram_bank();
void option_cb_add_cheat();
void option_cb_dump_sram();
void option_cb_restore_sram();

BYTE bank1_call(WORD task,char* user_data)
{
    BYTE f0,f1,r;
    BYTE (*dispatch)(WORD,char*) = (BYTE (*)(WORD,char*))0x4000;

    f0 = Frame1;
    f1 = Frame2;
    Frame1 = BANK_BG_GFX;
    Frame2 = BANK_RAM_CODE;
    r = dispatch(task,user_data);
    Frame1 = f0;
    Frame2 = f1;
    return r;
}

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
        BYTE b,c;
        b = font[i] ^ 0xFF;
        c = ~b;
        VdpData = c;
        VdpData = c;
        VdpData = c;
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

    vdp_set_color(16, 0, 0, 0);             // Backdrop color
    vdp_set_color(23, 0, 0, 0);
    vdp_set_color(24, 3, 3, 3);

    enable_ints();
}

/*
 * Prints the value val in hexadecimal form at position x,y
 */

void print_hex(BYTE val, BYTE x, BYTE y)
{
    val=val,x=x,y=y;
    bank1_call(TASK_PRINT_HEX,&val);
}

void dump_hex(WORD addr)
{
    addr=addr;
    bank1_call(TASK_DUMP_HEX,(char*)&addr);
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

void cls()
{
    clear_list_surface();
    present_list_surface();
    vdp_wait_vblank();
}

void puts_active_list()
{
    BYTE* temp = generic_list_buffer;
    const BYTE* p;
    BYTE row, col, show;
    WORD offs;
    DWORD praddr;
    uint16_t prbank, proffs;
    FileInfoEntry *fi;

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
    else if(MENU_STATE_GAME_SD == menu_state)
    {
        clear_list_surface();
        row = 0;
        show = games.count - games.firstShown;
        show = (show < NUMBER_OF_GAMES_TO_SHOW) ? show : NUMBER_OF_GAMES_TO_SHOW;

        praddr = games.firstShown;
        praddr <<= 6;
        proffs = praddr & 0xFFFF;
        prbank = praddr >> 16;
        prbank += 0x20;
        fi = (FileInfoEntry*)0xDA20;
        highlightedIsDir = 0;

        while (show)
        {
            offs = row*32*2;
            pfn_neo2_psram_to_ram((BYTE *)fi, prbank, proffs, 64);
            p = fi->sfn;

            if (games.highlighted == (games.firstShown + row))
            {
                memcpy_asm(highlightedFileName, fi->sfn, 13);
                p = fi->lfn;
            }
            if (fi->fattrib & AM_DIR)
            {
                temp[offs + 2] = '[' - 32;
                if (games.highlighted == (games.firstShown + row))
                {
                    highlightedIsDir = 1;
                    temp[offs + 3] = PALETTE0<<1;
                }
                else
                {
                    temp[offs + 3] = PALETTE1<<1;
                }
                offs += 2;
            }
            for (col=0; col<25; col++)
            {
                if (!p[col])
                    break;
                temp[offs + col*2 + 2] = p[col] - 32;
                temp[offs + col*2 + 3] = (games.highlighted == (games.firstShown + row)) ? PALETTE0<<1 : PALETTE1<<1;
            }
            if (fi->fattrib & AM_DIR)
            {
                temp[offs + col*2 + 2] = ']' - 32;
                if (games.highlighted == (games.firstShown + row))
                    temp[offs + col*2 + 3] = PALETTE0<<1;
                else
                    temp[offs + col*2 + 3] = PALETTE1<<1;
            }

            row++;
            show--;
            proffs += 64;
            if (proffs == 0)
                prbank++;
        }

        present_list_surface();
        //print_hex(highlightedIsDir, 10, 3); // DEBUG
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

            if(OPTION_TYPE_ROUTINE == options_get_type(&options[show]))
            {
                if(OPTION_CB_SET_SRAM_BANK == options[show].user_data)
                {
                    temp[offs + col*2 + 2] = ('0'+(options_sram_bank+1))-32;
                    temp[offs + col*2 + 3] = attr;
                    col++;
                }
                else if(OPTION_CB_CHEAT_MGR == options[show].user_data)
                {
                    //col+=4;
                    temp[offs + col*2 + 2] = '('-32;
                    temp[offs + col*2 + 3] = attr;
                    col++;
                    temp[offs + col*2 + 2] = ('0'+(MAX_CHEATS-options_cheat_ptr))-32;
                    temp[offs + col*2 + 3] = attr;
                    col++;
                    temp[offs + col*2 + 2] = ')'-32;
                    temp[offs + col*2 + 3] = attr;
                    col++;
                }
            }
            else if(OPTION_TYPE_SETTING == options_get_type(&options[show]))
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
    if((MENU_STATE_GAME_GBAC == menu_state) ||
       (MENU_STATE_MEDIA_PLAYER == menu_state) ||
       (MENU_STATE_GAME_SD == menu_state))
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
    if((MENU_STATE_GAME_GBAC == menu_state) ||
       (MENU_STATE_MEDIA_PLAYER == menu_state) ||
       (MENU_STATE_GAME_SD == menu_state))
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

    if((MENU_STATE_GAME_GBAC == menu_state) ||
       (MENU_STATE_MEDIA_PLAYER == menu_state) ||
       (MENU_STATE_GAME_SD == menu_state))
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

    if((MENU_STATE_GAME_GBAC == menu_state) ||
       (MENU_STATE_MEDIA_PLAYER == menu_state) ||
       (MENU_STATE_GAME_SD == menu_state))
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
            puts("[L/R/U/D] Navigate  ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
            puts("[I] Run [II] Options", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
        break;
        case MENU_STATE_OPTIONS:
            options_highlighted = 0;
            puts("[L/R] Change option ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
            puts("[I] Run [II] Back   ", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
        break;
        case MENU_STATE_GAME_SD:
            puts("[L/R/U/D] Navigate  ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
            puts("[I] Run [II] Options", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
        break;
        case MENU_STATE_MEDIA_PLAYER:
            puts("[L/R/U/D] Navigate  ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
            puts("[I] Play [II] Flash ", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
        break;
    }
}

void handle_action_button(BYTE button)
{
    FileInfoEntry *fi;
    WORD *gd3;
    WORD i;
    BYTE col;
    DWORD praddr;
    uint16_t prbank, proffs;
    Option* option;

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
                BYTE type;
                options_sync = 1;
                option = &options[options_highlighted];
                type = options_get_type(option);

                if(OPTION_TYPE_SETTING == type)
                {
                    if(options_get_state(option))
                        options_set_state(option,0);
                    else
                        options_set_state(option,1);
                }
                else if(OPTION_TYPE_ROUTINE == type)
                {
                    switch(option->user_data)
                    {
                        case OPTION_CB_SET_SRAM_BANK:
                            if(button == PAD_LEFT)
                                {option_cb_dec_sram_bank();}
                            else
                                {option_cb_inc_sram_bank();}
                        break;
                    }
                }
                puts_active_list();
            }
        }
        else if(button == PAD_SW1)
        {
            if(options_highlighted >= options_count){return;}
            option = &options[options_highlighted];

            if(OPTION_TYPE_ROUTINE == options_get_type(option))
            {
                cls();

                switch(option->user_data)
                {
                    case OPTION_CB_SRAM_TO_SD:
                        option_cb_dump_sram();
                    break;

                    case OPTION_CB_SD_TO_SRAM:
                        option_cb_restore_sram();
                    break;

                    case OPTION_CB_CHEAT_MGR:
                        option_cb_add_cheat();
                    break;

                    case OPTION_CB_CLEAR_SRAM:
                        sdutils_sram_cls();
                    break;

                    case OPTION_CB_IMPORT_IPS:
                    break; //TODO
                }

                cls();
                sync_state();
                puts_active_list();
                return;
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

        if (MENU_STATE_GAME_GBAC == menu_state)
        {
            // Copy the game info data to somewhere in RAM
            gameData = (volatile GbacGameData*)0xDC00;
            p = (volatile BYTE*)0xB000;
            p += games.highlighted << 5;

            gameData->mode = GDF_RUN_FROM_FLASH;
            gameData->type = flash_mem_type;
            gameData->size = p[2] >> 4;
            gameData->bankHi = p[2] & 0x0F;
            gameData->bankLo = p[3];
            gameData->sramBank = options_sram_bank; // : (p[4] >> 4);
            gameData->sramSize = p[4] & 0x0F;
            gameData->cheat[0] = p[5];
            gameData->cheat[1] = p[6];
            gameData->cheat[2] = p[7];
            pfn_neo2_run_game_gbac(fm,reset,0x0000); // never returns
        }
        else if (MENU_STATE_GAME_SD == menu_state)
        {
            if (highlightedIsDir)
            {
                change_directory(highlightedFileName);
                Frame2 = BANK_RAM_CODE; // reset Frame2, since change_directory modifies it
                puts_active_list();
                return;
            }

            // The highlighted entry is not a directory

            // Retrieve the file info struct for the highlighted file
            fi = (FileInfoEntry*)0xDA20;
            praddr = games.highlighted;
            praddr <<= 6;
            proffs = praddr & 0xFFFF;
            prbank = praddr >> 16;
            pfn_neo2_psram_to_ram((BYTE *)fi, prbank+0x20, proffs, 64);

            if (GAME_MODE_NORMAL_ROM == fi->ftype)
            {
                read_file_to_psram(fi, 0x00, 0x0000);
                Frame2 = BANK_RAM_CODE;
                // Copy the game info data to somewhere in RAM
                gameData = (volatile GbacGameData*)0xDC00;

                gameData->mode = GDF_RUN_FROM_PSRAM;
                gameData->type = flash_mem_type;
                gameData->size = fi->fsize >> 17;
                if (fi->fsize & 0x1FFFF)
                    gameData->size <<= 1;
                else if (!gameData->size)
                    gameData->size = 1;
                gameData->bankHi = 0;
                gameData->bankLo = 0;
                gameData->sramBank = 0;
                gameData->sramSize = 0;
                gameData->cheat[0] = 0;
                gameData->cheat[1] = 0;
                gameData->cheat[2] = 0;
                pfn_neo2_run_game_gbac(fm,reset,0x0000); // never returns
            }
            else if (GAME_MODE_VGM == fi->ftype)
            {
                read_file_to_psram(fi, 0x00, 0x0000);
                cls();
                puts("Now playing: ", LEFT_MARGIN, 9, PALETTE1);
                pfn_neo2_psram_to_ram(0xDD00, 0, 0x0000, 0x20);

                if ((*(DWORD *)0xDD10) != 0)
                    Neo2FmOn = 0x00;        // enable FM if the YM2413 clock is non-zero

                praddr = *(DWORD *)0xDD14;  // GD3 offset field in the VGM header
                if (praddr == 0)
                {
                    // No GD3
                    puts(highlightedFileName, LEFT_MARGIN, 11, PALETTE1);
                }
                else
                {
                    praddr += 0x14 + 12;
                    proffs = praddr & 0xFFFF;
                    prbank = praddr >> 16;
                    // ToDo: handle cases where the GD3 tag crosses a 16 kB boundary and/or
                    // is larger than 256 bytes
                    gd3 = (WORD *)0xDD00;
                    pfn_neo2_psram_to_ram((BYTE *)gd3, prbank, proffs, 256);
                    col = LEFT_MARGIN;
                    for (i=0; gd3[i]!=0; i++)
                    {
                        if (col < 25)
                            puts((char*)&gd3[i], col, 11, PALETTE1);  // english title
                        col++;
                    }
                    for (i=i+1; gd3[i]!=0; i++) ; // skip the japanese title
                    col = LEFT_MARGIN;
                    for (i=i+1; gd3[i]!=0; i++)
                    {
                        if (col < 25)
                            puts((char*)&gd3[i], col, 12, PALETTE1);  // english game name
                        col++;
                    }
                    for (i=i+1; gd3[i]!=0; i++) ; // skip the japanese game name
                    for (i=i+1; gd3[i]!=0; i++) ; // skip the english system name
                    for (i=i+1; gd3[i]!=0; i++) ; // skip the japanese system name
                    col = LEFT_MARGIN;
                    for (i=i+1; gd3[i]!=0; i++)
                    {
                        if (col < 25)
                            puts((char*)&gd3[i], col, 13, PALETTE1);  // english author name
                        col++;
                    }
                }

                Frame1 = BANK_VGM_PLAYER;
                Frame2 = BANK_RAM_CODE;

                puts("[II] Back           ", LEFT_MARGIN, INSTRUCTIONS_Y, PALETTE1);
                puts("                    ", LEFT_MARGIN, INSTRUCTIONS_Y+1, PALETTE1);
                memcpy_asm(0xDD00, 0x4000, 0x240); // copy the vgm player code to ram
                pfn_vgm_play();                     // the player will return when the user presses SW2

                mute_sound();

                Frame1 = BANK_BG_GFX;
                Frame2 = BANK_RAM_CODE;
                // SW2 being pressed is what causes the vgm player
                // to return. so mark it as pressed
                padLast |= PAD_SW2;
                sync_state();       // print SD browser instructions again
                puts_active_list();
            }
        }
    }
    else if(button == PAD_SW2)
    {
        BYTE state = menu_state;

        //++menu_state;
        if (MENU_STATE_OPTIONS != menu_state)
        {
            menu_state = MENU_STATE_OPTIONS;
            if(hasZipram)
            {
                //Save
                pfn_neo2_ram_to_psram(0x00,2048, (BYTE*)0xDA00,256);
                pfn_neo2_psram_to_ram((BYTE*)0xDA00,0x00,1024,256);
            }
        }
        else if (neoMode == 0x480)
        {
            menu_state = MENU_STATE_GAME_SD;
            if(state == MENU_STATE_OPTIONS)
            {
                if(hasZipram)
                {
                    //Restore
                    pfn_neo2_ram_to_psram(0x00,1024, (BYTE*)0xDA00,256);
                    pfn_neo2_psram_to_ram((BYTE*)0xDA00,0x00,2048,256);
                }
            }
        }
        else
        {
            menu_state = MENU_STATE_GAME_GBAC;
        }

        if(menu_state > MENU_STATES-1)
            menu_state = MENU_STATE_TOP;

        sync_state();
        puts_active_list();
    }
}

void import_std_options()
{
    Option* opt;

    if(hasZipram)
    {
        pfn_neo2_ram_to_psram(0x00,2048, (BYTE*)0xDA00,256);
    }

    options_init();

    fm_enabled_option_idx = options_count;
    options_add("FM              : ","off","on",OPTION_TYPE_SETTING,1);
    reset_to_menu_option_idx = options_count;
    options_add("Reset to menu   : ","off","on",OPTION_TYPE_SETTING,1);
    import_ips_option_idx = options_count;

    if(hasZipram)
    {
#if 0
        options_add("Import .ips     : ","off","on",OPTION_TYPE_SETTING,0);
#endif
        sram_set_option_idx = options_count;
        opt = options_add("SRAM Bank       : ","\0","\0",OPTION_TYPE_ROUTINE,0);
        opt->user_data = OPTION_CB_SET_SRAM_BANK;
        sram_cls_option_idx = options_count;
        opt = options_add("Cheat manager","\0","\0",OPTION_TYPE_ROUTINE,0);
        opt->user_data = OPTION_CB_CHEAT_MGR;
        opt = options_add("Dump SRAM to SD  ","\0","\0",OPTION_TYPE_ROUTINE,0);
        opt->user_data = OPTION_CB_SRAM_TO_SD;
        opt = options_add("Restore SRAM from SD","\0","\0",OPTION_TYPE_ROUTINE,0);
        opt->user_data = OPTION_CB_SD_TO_SRAM;
    }
    else
    {
        opt = options_add("SRAM Bank       : ","\0","\0",OPTION_TYPE_ROUTINE,0);
        opt->user_data = OPTION_CB_SET_SRAM_BANK;
    }

    opt = options_add("SRAM Clear","\0","\0",OPTION_TYPE_ROUTINE,0);
    opt->user_data = OPTION_CB_CLEAR_SRAM;

    if(hasZipram)
    {
        pfn_neo2_ram_to_psram(0x00,1024, (BYTE*)0xDA00,256);
        pfn_neo2_psram_to_ram((BYTE*)0xDA00,0x00,2048,256);
    }
}

#ifdef TEST_SD_BLOCK_WRITE
void test_w_mode()
{
        #if 0
        FRESULT (*f_write)(void*) = pfn_pf_write_sector;
        FRESULT (*f_open)(const char*) = pfn_pf_open;
        unsigned char* p = (unsigned char*)0xdb00;
        FRESULT r;

        cls();

        Frame2 = BANK_PFF;

        r = f_open("/DUMMY.BIN");

        if(r != FR_OK)
        {
            puts("Failed to open /DUMMY.BIN", 2, 8, PALETTE1);
            while(1){}
        }
        else
        {
            puts("Openned /DUMMY.BIN", 2, 8, PALETTE1);
        }

        memset_asm(p,'A',512);
        strcpy_asm(p,"Neo SMS Myth Menu...");
        //memset_asm(p,'A',256);
        //memset_asm(p+256,'B',256);
        puts("WRITE BEGIN", 2, 9, PALETTE1);
        Frame2 = BANK_PFF;

        r = f_write((void*)p);

        if(r == FR_OK)
            puts("WRITE : PASSED!", 2, 10, PALETTE1);
        else
        {
            puts("WRITE : FAILED!", 2, 10, PALETTE1);
            if(0x0002 == r){puts("CRC ERROR!", 2, 11, PALETTE1);}
            if(0x0003 == r){puts("START BIT ERROR!", 2, 11, PALETTE1);}
            //print_hex((r>>8)&0xff,2,12);
            print_hex(r&0xff,4,12);
            print_hex(diskioResp[0],6,12);
        }

        puts("WRITE END", 2, 13, PALETTE1);
        while(1){}
        #else
        sdutils_sram_to_sd("/DUMMY.BIN");
        #endif
}
#endif



void main()
{
    BYTE temp;
    BYTE padUpReptDelay;
    BYTE padDownReptDelay;
    char type[2];

    MemCtrl = 0xA8;
    menu_state = MENU_STATE_GAME_GBAC;  //start off with gbac game state
    neoMode = 0;

    // Copy neo2 code from ROM to RAM
    Frame1 = BANK_RAM_CODE;
    memcpy_asm(0xC800, 0x4000, 0xF99);

    temp = pfn_neo2_check_card();
    hasZipram = pfn_neo2_test_psram();

    Frm2Ctrl = FRAME2_AS_ROM;

    Frame1 = 3;
    flash_mem_type = *(volatile BYTE*)0x7FF0; // flash mem type at 0xFFF0: 0 = newer(C), 1 = new(B), 2 = old(A)
    type[0] = 'C' - flash_mem_type;
    type[1] = 0;

    Frame1 = BANK_BG_GFX;
    Frame2 = BANK_RAM_CODE;

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

    bank1_call(TASK_LOAD_BG,0);
    puts("Neo SMS Menu", LEFT_MARGIN, 1, PALETTE1);
    puts("(c) NeoTeam 2011", LEFT_MARGIN, 2, PALETTE1);

    // Print software (menu) and firmware versions
    puts(MENU_VERSION_STRING, 20, 1, PALETTE1);
    puts("/", 24, 1, PALETTE1);
    puts("1.00", 25, 1, PALETTE1);  // TODO: read version from CPLD

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
    if (region == JAPANESE)
        puts("/Jap", 25, 2, PALETTE1);
    else if (vdpSpeed == NTSC)
        puts("/Usa", 25, 2, PALETTE1);
    else
        puts("/Eur", 25, 2, PALETTE1);

    #if 0
    test_strings();
    #endif

    #if 0
    {
        BYTE test1[16];
        BYTE test2[16];
        strcpy_asm(test1, "This is test 1");
        pfn_neo2_ram_to_sram(0x03, 0xC800, test1, 16);
        pfn_neo2_sram_to_ram(test2, 0x03, 0xC800, 16);
        test2[15] = 0;
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
        puts(test2, 9, 18, PALETTE1);
    }
    #endif

    // No point in trying to mount an SD card if the cart doesn't have
    // zipram (e.g. normal GBAC or Neo3-SD)
    if (hasZipram)
    {
        temp = init_sd();
        Frame1 = BANK_BG_GFX;
        Frame2 = BANK_RAM_CODE;
    }

    puts_active_list();

    pad = padLast = 0;
    padUpReptDelay = KEY_REPEAT_INITIAL_DELAY;
    padDownReptDelay = KEY_REPEAT_INITIAL_DELAY;

    import_std_options();

    #ifdef TEST_SD_BLOCK_WRITE
    test_w_mode();
    #endif

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

void options_set_state(Option* option,BYTE new_state)
{
    option->encoded_info = (option->encoded_info & 0xF0) | (new_state & 0x0F);
}

void options_set_type(Option* option,BYTE new_type)
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

void options_init()
{
    options = (Option*)0xDA18;
    options_highlighted = 0;
    options_count = 0;
    options_sync = 0;
    options_cheat_ptr = 0;
    options_sram_bank = 0;
}

void options_add_cheat(BYTE data,BYTE addr_hi,WORD addr_lo)
{
    BYTE* base;

    if(options_cheat_ptr >= MAX_CHEATS){return;}

    base = (BYTE*)0xDACC;
    base += options_cheat_ptr << 2;
    base[0] = data;
    base[1] = addr_hi;
    *(volatile unsigned short*)&base[2] = addr_lo;

    ++options_cheat_ptr;
}

void option_cb_inc_sram_bank()//Up to 64KB
{
    if(options_sram_bank < 7){++options_sram_bank;}
}

void option_cb_dec_sram_bank()
{
    if(options_sram_bank > 0){--options_sram_bank;}
}

void option_cb_add_cheat()
{
    BYTE tmp[8];

    if(options_cheat_ptr >= MAX_CHEATS){return;}

    tmp[0] = 8; //field count
    if (bank1_call(TASK_EXEC_CHEAT_INPUTBOX,&tmp[0]))
    {
        options_add_cheat(tmp[0],tmp[1],*(volatile unsigned short*)&tmp[2]);
    }
}

void option_cb_dump_sram()
{
    sdutils_xfer_sram_to_sd(sram_bank_binary[options_sram_bank]);
}

void option_cb_restore_sram()
{
    sdutils_xfer_sd_to_sram(sram_bank_binary[options_sram_bank]);
}


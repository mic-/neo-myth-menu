#include "neo2.h"
#include "sms.h"
#include "shared.h"

// Since we're not linking this code against shared.rel
#define neoMode_ (*(WORD *)0xC003)

void neo2_asic_begin()
{
    Neo2FlashBankSize = FLASH_SIZE_1M;
    Neo2Frame0We = 1;
}

void neo2_asic_end()
{
    Neo2FlashBankLo = 0;
    Neo2FlashBankSize = 0;
    Neo2Frame0We = 0;
}

void neo2_asic_unlock()
{
    volatile BYTE dummy;

    // FFD200
    Neo2FlashBankLo = 0xFF;
    Frame1 = 0x06;
    dummy = *(volatile BYTE *)0x6400;

    // 001500
    Neo2FlashBankLo = 0x00;
    Frame1 = 0x00;
    dummy = *(volatile BYTE *)0x6A00;

    // 01D200
    Neo2FlashBankLo = 0x01;
    Frame1 = 0x06;
    dummy = *(volatile BYTE *)0x6400;

    // 021500
    Neo2FlashBankLo = 0x02;
    Frame1 = 0x00;
    dummy = *(volatile BYTE *)0x6A00;

    // FE1500
    Neo2FlashBankLo = 0xFE;
    Frame1 = 0x00;
    dummy = *(volatile BYTE *)0x6A00;
}

void neo2_asic_cmd(BYTE cmd, WORD data)
{
    volatile BYTE dummy;

    neo2_asic_unlock();

    Neo2FlashBankLo = cmd;
    Frame1 = data >> 13;
    dummy = *(volatile BYTE *)(0x4000 | ((data & 0x1FFF) << 1));
}

BYTE neo2_check_card() /*Returns 0 if no NEO2/3 cart found*/
{
    volatile BYTE dummy;

    __asm
    di
    __endasm;

    neo2_asic_begin();

    Neo2CardCheck = 0x01;

    neo2_asic_cmd(0xE2, 0x1500); //GBA WE on
    neo2_asic_cmd(0x37, 0x2003); //select menu flash
    neo2_asic_cmd(0xEE, 0x0630); //enable extended address bus

    Neo2Frame1We = 0x01;
    Neo2ExtMemOn = 0x01;

    Frame1 = 0x00;
    Neo2FlashBankLo = 0x00;

    *(volatile BYTE*)0x4000 = 0x90;
    *(volatile BYTE*)0x4001 = 0x90;

    dummy = *(volatile BYTE*)0x4002;
    //idLo = dummy;
    *(BYTE*)0xC001 = dummy;

    dummy = *(volatile BYTE*)0x4003;
    //idHi = dummy;
    *(BYTE*)0xC002 = dummy;

    *(volatile BYTE*)0x4000 = 0xFF;
    *(volatile BYTE*)0x4001 = 0xFF;

    Neo2Frame1We = 0x00;

    // ID ON
    neo2_asic_cmd(0x90, 0x3500);

    Frm2Ctrl = 0x80 | FRAME2_AS_SRAM;

    dummy = 0;
    if ( (*(volatile BYTE *)(0x8000)) != 0x34 )     //Check signature
        dummy = 0x34;
    else if ( (*(volatile BYTE *)(0x8001)) != 0x16 )
        dummy = 0x16;
    else if ( (*(volatile BYTE *)(0x8002)) != 0x96 )
        dummy = 0x96;
    else if ( (*(volatile BYTE *)(0x8003)) != 0x24 )
        dummy = 0x24;

    *(volatile BYTE *)0xC000 = dummy;

    Frm2Ctrl = FRAME2_AS_ROM;

    // ID OFF
    neo2_asic_cmd(0x90, 0x4900);

    neo2_asic_end();

    __asm
    ei
    __endasm;

    return 1;
}

void neo2_enable_sram(WORD offset)
{
    neo2_asic_begin();
    neo2_asic_cmd(0x37, 0x2203|neoMode_); // select game flash
    neo2_asic_cmd(0xE0, offset); // set sram offset (in 8KB units)
    neo2_asic_end();
    Frm2Ctrl = 0x80 | FRAME2_AS_SRAM; // enable sram in Frame 2
}

void neo2_disable_sram()
{
    Frm2Ctrl = FRAME2_AS_ROM; // disable sram in Frame 2
    neo2_asic_begin();
    neo2_asic_cmd(0x37, 0x2003|neoMode_); // select menu flash
    neo2_asic_end();
}

void neo2_enable_psram()
{
    neo2_asic_begin();
    neo2_asic_cmd(0xE2,0x1500); // set flash and psram write enable
    neo2_asic_cmd(0x37,0x2203|neoMode_); // select game flash
    neo2_asic_cmd(0xDA,0xAF44); // select psram (works for both Neo2-SD and Pro)
    neo2_asic_end();
    Neo2Frame1We = 0x01;
}

void neo2_disable_psram()
{
    Frame1 = 1;
    Neo2Frame1We = 0x00;
    neo2_asic_begin();
    neo2_asic_cmd(0xE2,0xD200); // set flash & psram write disable
    neo2_asic_cmd(0x37,0x2003|neoMode_); // select menu flash
    neo2_asic_cmd(0xDA,0x0044); // deselect psram
    neo2_asic_end();
}

void neo2_ram_to_sram(BYTE dsthi, WORD dstlo, BYTE* src, WORD len) __naked
{
    dsthi, dstlo, src, len; // suppress warning
    __asm
    di
    push    ix          ; save frame pointer
    ld      ix,#4       ; WORD retaddr + WORD ix
    add     ix,sp       ; frame pointer in ix

    ld      b,(ix)      ; dsthi
    ld      a,2(ix)     ; dstlo MSB
    srl     b           ; shift b0 of b into CF
    rra                 ; rotate right, CF into b7 of a
    srl     b
    rra
    srl     a
    srl     a
    srl     a
    and     a,#0x1E     ; a now holds the sram offset
    ld      l,a
    ld      h,#0
    push    hl
    call    _neo2_enable_sram
    pop     af

    ld      e,1(ix)     ; dstlo LSB
    ld      a,2(ix)     ; dstlo MSB
    and     a,#0x3F
    or      a,#0x80     ; address in frame 2
    ld      d,a
    ld      l,3(ix)     ; src LSB
    ld      h,4(ix)     ; src MSB
    ld      c,5(ix)     ; len LSB
    ld      b,6(ix)     ; len MSB
    ldir                ; block move

    call    _neo2_disable_sram

    pop     ix          ; restore frame pointer
    ei
    ret
    __endasm;
}

void neo2_sram_to_ram(BYTE* dst, BYTE srchi, WORD srclo, WORD len) __naked
{
    dst, srchi, srclo, len; // suppress warning
    __asm
    di
    push    ix          ; save frame pointer
    ld      ix,#4       ; WORD retaddr + WORD ix
    add     ix,sp       ; frame pointer in ix

    ld      b,2(ix)     ; srchi
    ld      a,4(ix)     ; srclo MSB
    srl     b           ; shift b0 of b into CF
    rra                 ; rotate right, CF into b7 of a
    srl     b
    rra
    srl     a
    srl     a
    srl     a
    and     a,#0x1E     ; a now holds the sram offset
    ld      l,a
    ld      h,#0
    push    hl
    call    _neo2_enable_sram
    pop     af

    ld      e,(ix)      ; dst LSB
    ld      d,1(ix)     ; dst MSB
    ld      l,3(ix)     ; srclo LSB
    ld      a,4(ix)     ; srclo MSB
    and     a,#0x3F
    or      a,#0x80     ; address in frame 2
    ld      h,a
    ld      c,5(ix)     ; len LSB
    ld      b,6(ix)     ; len MSB
    ldir                ; block move

    call    _neo2_disable_sram

    pop     ix          ; restore frame pointer
    ei
    ret
    __endasm;
}

void neo2_ram_to_psram(BYTE dsthi, WORD dstlo, BYTE* src, WORD len) __naked
{
    dsthi, dstlo, src, len; // suppress warning
    __asm
    di
    push    ix          ; save frame pointer
    ld      ix,#4       ; WORD retaddr + WORD ix
    add     ix,sp       ; frame pointer in ix

    call    _neo2_enable_psram

    ld      a,(ix)      ; dsthi
    ld      hl,#0xBFC0  ; bank low
    srl     a           ; bank is for word bus, lsb -> CF
    ld      (hl),a
    ld      a,2(ix)     ; dstlo MSB
    rla
    rla
    rla
    and     a,#0x07
    ld      hl,#0xFFFE
    ld      (hl),a      ; Frame1 = ((dsthi & 1) << 2) | (dstlo MSB >> 14)
    ld      e,1(ix)     ; dstlo LSB
    ld      a,2(ix)     ; dstlo MSB
    and     a,#0x3F
    or      a,#0x40     ; address in frame 1
    ld      d,a
    ld      l,3(ix)     ; src LSB
    ld      h,4(ix)     ; src MSB
    ld      c,5(ix)     ; len LSB
    ld      b,6(ix)     ; len MSB
    ldir                ; block move

    call    _neo2_disable_psram

    pop     ix          ; restore frame pointer
    ei
    ret
    __endasm;
}

void neo2_psram_to_ram(BYTE* dst, BYTE srchi, WORD srclo, WORD len) __naked
{
    dst, srchi, srclo, len; // suppress warning
    __asm
    di
    push    ix          ; save frame pointer
    ld      ix,#4       ; WORD retaddr + WORD ix
    add     ix,sp       ; frame pointer in ix

    call    _neo2_enable_psram

    ld      a,2(ix)     ; srchi
    ld      hl,#0xBFC0  ; bank low
    srl     a           ; bank is for word bus, lsb -> CF
    ld      (hl),a
    ld      a,4(ix)     ; srclo MSB
    rla
    rla
    rla
    and     a,#0x07
    ld      hl,#0xFFFE
    ld      (hl),a      ; Frame1 = ((srchi & 1) << 2) | (srclo MSB >> 14)
    ld      e,(ix)      ; dst LSB
    ld      d,1(ix)     ; dst MSB
    ld      l,3(ix)     ; src LSB
    ld      a,4(ix)     ; srclo MSB
    and     a,#0x3F
    or      a,#0x40     ; address in frame 1
    ld      h,a
    ld      c,5(ix)     ; len LSB
    ld      b,6(ix)     ; len MSB
    ldir                ; block move

    call    _neo2_disable_psram

    pop     ix          ; restore frame pointer
    ei
    ret
    __endasm;
}

/*
void neo2_sd_crc16(unsigned char *p_crc, unsigned char *data, int len)
{
    int i;
    unsigned char nybble;

    unsigned long long poly = 0x0001000000100001LL;
    unsigned long long crc = 0;
    unsigned long long n_crc; // This can probably be unsigned int

    // Load crc from array
    for (i = 0; i < 8; i++)
    {
        crc <<= 8;
        crc |= p_crc[i];
    }

    for (i = 0; i < (len << 1); i++)
    {
        if (i & 1)
            nybble = (data[i >> 1] & 0x0F);
        else
            nybble = (data[i >> 1] >> 4);

        n_crc = (crc >> 60);
        crc <<= 4;
        if ((nybble ^ n_crc) & 1) crc ^= (poly << 0);
        if ((nybble ^ n_crc) & 2) crc ^= (poly << 1);
        if ((nybble ^ n_crc) & 4) crc ^= (poly << 2);
        if ((nybble ^ n_crc) & 8) crc ^= (poly << 3);
    }

    // Output crc to array
    for (i = 7; i >= 0; i--)
    {
        p_crc[i] = crc;
        crc >>= 8;
    }
}
*/

// Returns non-zero if the cart has psram
BYTE neo2_test_psram()
{
    char test[] = "Test1234";
    char *pRam = (char *)0xDC00;
    BYTE i;

    for (i=0; i<8; i++)
        pRam[i] = test[i];
    neo2_ram_to_psram(0, 0x0000, pRam, 8);
    for (i=0; i<8; i++)
        pRam[i] = 0;
    neo2_psram_to_ram(pRam, 0, 0x0000, 8);
    for (i=0; i<8; i++)
    {
        if (pRam[i] != test[i])
        {
            i = 0;
            break;
        }
    }
    return i;
}

/*void neo2_debug_dump_hex(WORD addr)
{
    BYTE *p = (BYTE*)addr;
    WORD vaddr = MENU_NAMETABLE;
    BYTE row,col,c,d;

    while (!(VdpCtrl & 0x80)) {}

    row = 7;
    vaddr += (row << 6) + 8;
    for (; row < 15; row++)
    {

        VdpCtrl = (vaddr & 0xFF);
        VdpCtrl = (vaddr >> 8) | 0x40;
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
        vaddr += 64;
    }
}*/


void neo2_run_game_gbac(BYTE fm_enabled,BYTE reset_to_menu,WORD jump_addr)
{
    volatile GbacGameData* gameData = (volatile GbacGameData*)0xDC00;
    WORD wtemp;

    __asm
    di
    __endasm;

    if (gameData->mode == GDF_RUN_FROM_FLASH)
    {
        // run from game flash
        if (gameData->type == 0)
            wtemp = 0xAE44; // 0 = type C (newer) flash
        else if (gameData->type == 1)
            wtemp = 0x8E44; // 1 = type B (new) flash
        else if (gameData->type == 2)
            wtemp = 0x0E44; // 2 = type A (old) flash
        else
            wtemp = 0xAE44; // 0 = type C (newer) flash
    }
    else
    {
        // run from psram
        wtemp = 0xAF4E; // select psram (works for both Neo2-SD and Pro)
    }
    neo2_asic_begin();
    neo2_asic_cmd(0x37, 0x2202); // enable game flash
    neo2_asic_cmd(0xDA, wtemp);
    neo2_asic_end();

    Neo2FlashBankLo = gameData->bankLo;
    Neo2FlashBankHi = gameData->bankHi;

    if (gameData->size == 1)
        Neo2FlashBankSize = FLASH_SIZE_1M;
    else if (gameData->size == 2)
        Neo2FlashBankSize = FLASH_SIZE_2M;
    else if (gameData->size == 4)
        Neo2FlashBankSize = FLASH_SIZE_4M;
    else if (gameData->size == 8)
        Neo2FlashBankSize = FLASH_SIZE_8M;
    else
        Neo2FlashBankSize = FLASH_SIZE_16M;

    Neo2SramBank = gameData->sramBank;

        /* LD        A,03H
         LD       (0BFC8H),A   ; BIT0  RESET KEY  TO MENU ( SMS 1 )
                               ; BIT1  CARD  KEY  TO MENU ( SMS 2 )*/
    if(reset_to_menu)
        Neo2Reset2Menu = 3; // b1 = 1 (reset on Myth button), b0 = 1 (reset on Reset button)
    else
        Neo2Reset2Menu = 0;

    if(fm_enabled)
        Neo2FmOn = 0x00; // FM enabled
    else
        Neo2FmOn = 0x0F; // FM disabled

    // TODO: Handle cheats (but disable them for now)
    Neo2CheatOn = 0;

    Neo2Frame1We = 0;
    Neo2Frame0We = 0;

    Neo2Run = 0xFF;

    *(volatile BYTE *)0xC000 = 0xA8; // shadow MemCtrl (used by some games)

    Frame1 = 1;
    Frame2 = 2;
    wtemp = jump_addr;

    ((void (*)())wtemp)(); // RESET => jump to address
}



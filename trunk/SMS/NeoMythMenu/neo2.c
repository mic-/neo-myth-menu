#include "neo2.h"
#include "sms.h"
#include "shared.h"


void neo2_asic_begin()
{
    Neo2FlashBankSize = FLASH_SIZE_256K;
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

    return 1;
}




void neo2_run_game_gbac()
{
    BYTE hwtype;
    GbacGameData *gameData = (GbacGameData*)0xC800
    ;
    neo2_asic_begin();
    neo2_asic_cmd(0x37, 0x2203);
    neo2_asic_cmd(0xDA, 0xAE44);
    neo2_asic_end();

    Neo2Frame1We = 1;

    Frame1 = 0;
    *(volatile BYTE*)0x4000 = 0x90;
    *(volatile BYTE*)0x4001 = 0x90;
    *(volatile BYTE*)0x4002 = 0x90;
    *(volatile BYTE*)0x4003 = 0x90;

    hwtype = 0;
    // Check A type
    if ( 0x89 == *(volatile BYTE*)0x4000 )
        if ( 0x89 == *(volatile BYTE*)0x4002 )
            hwtype = 1;

    if (!hwtype)
    {
        neo2_asic_begin();
        neo2_asic_cmd(0xDA, 0x8E44);
        neo2_asic_end();

        Frame1 = 0;
        *(volatile BYTE*)0x4000 = 0x90;
        *(volatile BYTE*)0x4001 = 0x90;
        *(volatile BYTE*)0x4002 = 0x90;
        *(volatile BYTE*)0x4003 = 0x90;

        // Check B type
        if ( 0x89 == *(volatile BYTE*)0x4000 )
            if ( 0x89 == *(volatile BYTE*)0x4002 )
                hwtype = 2;
    }

    if (!hwtype)
    {
        neo2_asic_begin();
        neo2_asic_cmd(0xDA, 0x0E44);
        neo2_asic_end();
    }

    Frame1 = 0;
    *(volatile BYTE*)0x4000 = 0xFF;
    *(volatile BYTE*)0x4001 = 0xFF;
    *(volatile BYTE*)0x4002 = 0xFF;
    *(volatile BYTE*)0x4003 = 0xFF;

    Neo2Frame1We = 0;

    Neo2FlashBankLo = gameData->bankLo;
    Neo2FlashBankHi = gameData->bankHi;

    switch (gameData->size)
    {
        case 1:
            Neo2FlashBankSize = FLASH_SIZE_256K;
            break;
        case 2:
            Neo2FlashBankSize = FLASH_SIZE_512K;
            break;
        case 4:
            Neo2FlashBankSize = FLASH_SIZE_1M;
            break;
        case 8:     // ???
            Neo2FlashBankSize = FLASH_SIZE_2M;
            break;
         default:
            Neo2FlashBankSize = FLASH_SIZE_4M;
            break;
    }

    Neo2SramBank = gameData->sramBank;

    Neo2Frame0We = 0;

    Neo2FmOn = 0;       // TODO: Handle this

    // TODO: Handle cheats


    Neo2Run = 0xFF;

    Frame1 = 1;
    Frame2 = 2;

    // Jump to address 0
    ((void (*)())0x0000)();
}



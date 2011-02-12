#include "neo2.h"
#include "sms.h"
#include "shared.h"


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



void neo2_debug_dump_hex(WORD addr)
{
    BYTE *p = (BYTE*)addr;
    WORD vaddr = 0x1800;
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
}


void neo2_run_game_gbac()
{
    volatile GbacGameData* gameData = (volatile GbacGameData*)0xC800;
    WORD wtemp;

	__asm
	di
	__endasm;

    neo2_asic_begin();
    neo2_asic_cmd(0x37, 0x2203);
    neo2_asic_end();
    if (gameData->mode == 0)
        wtemp = 0xAE44; // 0 = type A (newer) flash
    else if (gameData->mode == 1)
        wtemp = 0x8E44; // 1 = type B (new) flash
    else
        wtemp = 0x0E44; // 2 = type C (old) flash
    neo2_asic_begin();
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

    Neo2Reset2Menu = 3; // TODO: Handle this

    Neo2FmOn = 0;       // TODO: Handle this

    // TODO: Handle cheats (but disable them for now)
    Neo2CheatOn = 0;

    Neo2Frame1We = 0;
    Neo2Frame0We = 0;

    Neo2Run = 0xFF;

    *(volatile BYTE *)0xC000 = 0xA8; // shadow MemCtrl (used by some games)

    Frame1 = 1;
    Frame2 = 2;

    //neo2_debug_dump_hex(0x0000);
	//while (1) {}

    ((void (*)())0x0000)(); // RESET => jump to address 0
}



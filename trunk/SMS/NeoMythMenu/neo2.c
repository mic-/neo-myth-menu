#include "neo2.h"
#include "sms.h"
#include "shared.h"


void neo2_flash_enable()
{
	Neo2FlashBankSize = FLASH_SIZE_256K;
	Neo2Frame0We = 1;
}


void neo2_flash_disable()
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
    Frame1 = (data >> 13) & 7;
    dummy = *(volatile BYTE *)(0x4000 | ((data & 0x1FFF) << 1));
}



BYTE neo2_check_card() /*Returns 0 if no NEO2/3 cart found*/
{
	volatile BYTE dummy;

	neo2_flash_enable();

	Neo2CardCheck = 0x01;

	neo2_asic_cmd(0xE2, 0x1500);
	neo2_asic_cmd(0x37, 0x2003);
	neo2_asic_cmd(0xEE, 0x0630);

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
	if ( (*(volatile BYTE *)(0x8000)) == 0x34 )		//Check signature
	{
		if ( (*(volatile BYTE *)(0x8001)) != 0x16 )
			dummy = 0x16;

		if (dummy == 0)
		{
	    	if ( (*(volatile BYTE *)(0x8002)) != 0x96 )
				dummy = 0x96;
		}

		if (dummy == 0)
		{
			if ( (*(volatile BYTE *)(0x8003)) != 0x24 )
				dummy = 0x24;
		}
	}
	else
		dummy = 0x34;

	*(volatile BYTE *)0xC000 = dummy;

	Frm2Ctrl = FRAME2_AS_ROM;

	// ID OFF
	neo2_asic_cmd(0x90, 0x4900);

	neo2_flash_disable();

	return 1;
}


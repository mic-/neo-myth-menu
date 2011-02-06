#include "neo2.h"
#include "sms.h"


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


void neo2_asic_cmd(uint32_t cmd)
{
	volatile BYTE dummy;

	neo2_asic_unlock();

	Neo2FlashBankLo = cmd >> 16;
	Frame1 = (cmd >> 13) & 7;
	dummy = *(volatile BYTE *)(0x4000 | ((cmd & 0x1FFF) << 1));
}


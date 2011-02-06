#ifndef __NEO2_H__
#define __NEO2_H__

#include <z80/types.h>
#include <stdint.h>


#define Neo2FlashBankLo 	(*(volatile BYTE *)0xBFC0)
#define Neo2FlashBankSize 	(*(volatile BYTE *)0xBFC1)
#define Neo2SramBank 		(*(volatile BYTE *)0xBFC2)
#define Neo2FlashBankHi 	(*(volatile BYTE *)0xBFC3)


/*
 * Peform a Neo2 ASIC command
 */
extern void neo2_asic_cmd(uint32_t cmd);

#endif
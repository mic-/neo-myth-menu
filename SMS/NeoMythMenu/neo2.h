#ifndef __NEO2_H__
#define __NEO2_H__

#include <z80/types.h>
#include <stdint.h>


/*
 * Neo Myth registers
 */
#define Neo2FlashBankLo     (*(volatile BYTE *)0xBFC0)      // Bits 23..16 of the address
#define Neo2FlashBankSize   (*(volatile BYTE *)0xBFC1)
#define Neo2SramBank        (*(volatile BYTE *)0xBFC2)
#define Neo2FlashBankHi     (*(volatile BYTE *)0xBFC3)      // Bits 25..24 of the address
#define Neo2ExtMemOn        (*(volatile BYTE *)0xBFC4)      // Enable bits 25..24 of the address
#define Neo2Frame1We        (*(volatile BYTE *)0xBFC5)      // Write-enable Frame 1 (allows flash read)
#define Neo2CardCheck       (*(volatile BYTE *)0xBFC6)
#define Neo2UseJap          (*(volatile BYTE *)0xBFC7)
#define Neo2Run             (*(volatile BYTE *)0xBFCF)      // Lock hardware for game
#define Neo2Frame0We        (*(volatile BYTE *)0xBFD0)      // Write-enable Frame 0 (allows flash read)
#define Neo2FmOn            (*(volatile BYTE *)0xBFD1)      // Enable the FM chip (YM2413)
#define Neo2CheatData0      (*(volatile BYTE *)0xBFE0)      // Cheat data (1st byte)
#define Neo2CheatData1      (*(volatile BYTE *)0xBFE1)      // Cheat data (2nd byte)
#define Neo2CheatData2      (*(volatile BYTE *)0xBFE2)      // Cheat data (3rd byte)
#define Neo2CheatOn         (*(volatile BYTE *)0xBFE3)      // Enable cheat


/*
 * Size masks for Neo2FlashBankSize
 */
enum
{
    FLASH_SIZE_4M   = 0x00, // 4 Mbit
    FLASH_SIZE_2M   = 0x08,
    FLASH_SIZE_1M   = 0x0C,
    FLASH_SIZE_512K = 0x0E,
    FLASH_SIZE_256K = 0x0F,
};



/*
 * Peform a Neo2 ASIC command
 */
extern void neo2_asic_cmd(BYTE cmd, WORD data);

/*Returns 0 if no NEO2/3 cart found*/
extern BYTE neo2_check_card();

extern void neo2_flash_enable();
extern void neo2_flash_disable();

#endif

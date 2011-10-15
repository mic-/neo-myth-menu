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
#define Neo2Reset2Menu      (*(volatile BYTE *)0xBFC8)      // b0 = 1 = reset on RESET button, b1 = 1 = reset on MYTH button
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
    FLASH_SIZE_16M = 0x00,
    FLASH_SIZE_8M  = 0x08,
    FLASH_SIZE_4M  = 0x0C,
    FLASH_SIZE_2M  = 0x0E,
    FLASH_SIZE_1M  = 0x0F,
};


typedef struct
{
    BYTE mode;
    BYTE type;
    BYTE size;
    BYTE bankLo,bankHi;
    BYTE sramBank,sramSize;
    BYTE cheat[3];
} GbacGameData;

enum
{
    GDF_RUN_FROM_FLASH = 0x00,
    GDF_RUN_FROM_PSRAM,
};


/*
 * Peform a Neo2 ASIC command
 */
extern void neo2_asic_cmd(BYTE cmd, WORD data);

/*Returns 0 if no NEO2/3 cart found*/
extern BYTE neo2_check_card();

/*
 * Returns 1 if the cart has zipram
 */
//extern BYTE neo2_check_zipram();

extern void neo2_asic_begin();
extern void neo2_asic_end();

extern void neo2_enable_sram(WORD offset);
extern void neo2_disable_sram();
extern void neo2_enable_psram();
extern void neo2_disable_psram();

extern void neo2_ram_to_sram(BYTE dsthi, WORD dstlo, BYTE* src, WORD len);
extern void neo2_sram_to_ram(BYTE* dst, BYTE srchi, WORD srclo, WORD len);
extern void neo2_ram_to_psram(BYTE dsthi, WORD dstlo, BYTE* src, WORD len);
extern void neo2_psram_to_ram(BYTE* dst, BYTE srchi, WORD srclo, WORD len);

extern void neo2_run_game_gbac(BYTE fm_enabled,BYTE reset_to_menu);

extern int neo2_init_sd();

#endif

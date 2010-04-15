#ifndef _MYTH_H_
#define _MYTH_H_

#include "snes.h"

// NEO SNES Myth I/O registers

#define MYTH_OPTION_IO *(u8*)0xC000
#define MYTH_GBAC_LIO  *(u8*)0xC001
#define MYTH_GBAC_HIO  *(u8*)0xC002
#define MYTH_GBAC_ZIO  *(u8*)0xC003
#define MYTH_GBAS_BIO  *(u8*)0xC004
#define MYTH_GBAS_ZIO  *(u8*)0xC005
#define MYTH_PRAM_BIO  *(u8*)0xC006
#define MYTH_PRAM_ZIO  *(u8*)0xC007
#define MYTH_RUN_IO    *(u8*)0xC008
#define MYTH_EXTM_ON   *(u8*)0xC00D
#define MYTH_RST_SEL   *(u8*)0xC00E
#define MYTH_RST_IO    *(u8*)0xC010
#define MYTH_SRAM_WE   *(u8*)0xC011
#define MYTH_WE_IO     *(u8*)0xC014
#define MYTH_CPLD_RIO  *(u8*)0xC016
#define MYTH_SRAM_TYPE *(u8*)0xC018
#define MYTH_SRAM_MAP  *(u8*)0xC019
#define MYTH_DSP_TYPE  *(u8*)0xC01A
#define MYTH_DSP_MAP   *(u8*)0xC01B


 // For OPTION_IO
#define MAP_MENU_FLASH_TO_ROM	0x00
#define MAP_16M_PSRAM_TO_ROM	0x01
#define MAP_24M_PSRAM_TO_ROM	0x02
#define MAP_32M_PSRAM_TO_ROM	0x03


// For WE_IO
#define GBA_FLASH_WE		0x01
#define PSRAM_WE			0x02

#endif

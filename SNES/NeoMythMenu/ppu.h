#ifndef _PPU_H_
#define _PPU_H_

#include "snes.h"

#define REG_DISPCNT 		*(u8*)0x2100
#define REG_OBSEL			*(u8*)0x2101
#define REG_OAMADDL			*(u8*)0x2102
#define REG_OAMADDH			*(u8*)0x2103
#define REG_OAMDATA			*(u8*)0x2104
#define REG_DISPMODE		*(u8*)0x2105

#define REG_MOSAIC			*(u8*)0x2106

#define REG_BG0MAP 			*(u8*)0x2107
#define REG_BG1MAP 			*(u8*)0x2108
#define REG_BG2MAP 			*(u8*)0x2109
#define REG_BG3MAP 			*(u8*)0x210A

#define REG_CHRBASE_L 		*(u8*)0x210B
#define REG_CHRBASE_H 		*(u8*)0x210C

#define REG_BG0HOFS			*(u8*)0x210D
#define REG_BG0VOFS			*(u8*)0x210E
#define REG_BG1HOFS			*(u8*)0x210F
#define REG_BG1VOFS			*(u8*)0x2110

#define REG_VRAM_INC 		*(u8*)0x2115
#define REG_VRAM_ADDR_L 	*(u8*)0x2116
#define REG_VRAM_ADDR_H 	*(u8*)0x2117
#define REG_VRAM_DATAW1 	*(u8*)0x2118
#define REG_VRAM_DATAW2 	*(u8*)0x2119
#define REG_VRAM_DATAR1 	*(u8*)0x2139
#define REG_VRAM_DATAR2 	*(u8*)0x213A

#define REG_CGRAM_ADDR 		*(u8*)0x2121
#define REG_CGRAM_DATAW 	*(u8*)0x2122

#define REG_BGCNT 			*(u8*)0x212C

#define REG_COLDATA			*(u8*)0x2132

#define REG_STAT77			*(u8*)0x213E
#define REG_STAT78			*(u8*)0x213F

/**********************************************************************************************/

// For DISPCNT
#define ENABLE_SCREEN		0x00
#define BLANK_SCREEN 		0x80
#define BRIGHTNESS_0		0x00
#define BRIGHTNESS_1		0x01
#define BRIGHTNESS_2		0x02
#define BRIGHTNESS_3		0x03
#define BRIGHTNESS_4		0x04
#define BRIGHTNESS_5		0x05
#define BRIGHTNESS_6		0x06
#define BRIGHTNESS_7		0x07
#define BRIGHTNESS_8		0x08
#define BRIGHTNESS_9		0x09
#define BRIGHTNESS_10		0x0A
#define BRIGHTNESS_11		0x0B
#define BRIGHTNESS_12		0x0C
#define BRIGHTNESS_13		0x0D
#define BRIGHTNESS_14		0x0E
#define BRIGHTNESS_15		0x0F

// For VRAM_INC
#define VRAM_BYTE_ACCESS	0x00
#define VRAM_WORD_ACCESS	0x80

// For BGCNT
#define BG0_ENABLE 			0x01
#define BG1_ENABLE 			0x02
#define BG2_ENABLE 			0x04
#define BG3_ENABLE 			0x08
#define OBJ_ENABLE 			0x10

// For STAT78
#define PPU_VERSION			0x0F
#define PPU_MODE			0x10
#define PPU_FIELD			0x40


/**********************************************************************************************/

typedef struct
{
	u8 x;
	u8 y;
	u16 chr;
	u8 palette;
	u8 flip;
	u8 prio;
} oamEntry_t;

/**********************************************************************************************/


extern void load_cgram(char *src, unsigned short cgramOffs, unsigned short numBytes);

extern void load_vram(char *src, unsigned short vramOffs, unsigned short numBytes);

extern void update_oam(oamEntry_t *oamEntries, int first, int numEntries);

extern void mosaic_up();

extern void mosaic_down();

#endif



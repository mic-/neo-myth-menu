#ifndef _SNES_H_
#define _SNES_H_

typedef unsigned char u8;
typedef unsigned short u16;


#define REG_DISPCNT 		*(u8*)0x2100
#define REG_OBSEL			*(u8*)0x2101
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

#define REG_APUI00			*(u8*)0x2140
#define REG_APUI01			*(u8*)0x2141
#define REG_APUI02			*(u8*)0x2142
#define REG_APUI03			*(u8*)0x2143

#define REG_NMI_TIMEN		*(u8*)0x4200

// Multiplier / divider
#define REG_WRMPYA			*(u8*)0x4202
#define REG_WRMPYB			*(u8*)0x4203
#define REG_WRDIVL			*(u8*)0x4204
#define REG_WRDIVH			*(u8*)0x4205
#define REG_WRDIVB			*(u8*)0x4206
#define REG_RDMPYL			*(u8*)0x4216
#define REG_RDMPYH			*(u8*)0x4217

#define REG_RDNMI			*(u8*)0x4210

#define REG_HVB_JOY			*(u8*)0x4212
#define REG_JOY1L			*(u8*)0x4218
#define REG_JOY1H			*(u8*)0x4219
#define REG_JOY2L			*(u8*)0x421A
#define REG_JOY2H			*(u8*)0x421B

#define REG_MDMAEN			*(u8*)0x420B
#define REG_HDMAEN			*(u8*)0x420C

// DMA channel 0
#define REG_DMAP0			*(u8*)0x4300
#define REG_BBAD0			*(u8*)0x4301
#define REG_A1T0L			*(u8*)0x4302
#define REG_A1T0H			*(u8*)0x4303
#define REG_A1B0			*(u8*)0x4304
#define REG_DAS0L			*(u8*)0x4305
#define REG_DAS0H			*(u8*)0x4306
#define REG_DASB0			*(u8*)0x4307

// DMA channel 1
#define REG_DMAP1			*(u8*)0x4310
#define REG_BBAD1			*(u8*)0x4311
#define REG_A1T1L			*(u8*)0x4312
#define REG_A1T1H			*(u8*)0x4313
#define REG_A1B1			*(u8*)0x4314
#define REG_DAS1L			*(u8*)0x4315
#define REG_DAS1H			*(u8*)0x4316
#define REG_DASB1			*(u8*)0x4317

//********************************************************************************************


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

// For NMI_TIMEN
#define JOY_MANUAL_READ		0x00
#define JOY_AUTO_READ		0x01

// For HVB_JOY
#define JOY_DISABLE			0x00
#define JOY_ENABLE			0x01

// For STAT78
#define PPU_VERSION			0x0F
#define PPU_MODE			0x10
#define PPU_FIELD			0x40


#endif



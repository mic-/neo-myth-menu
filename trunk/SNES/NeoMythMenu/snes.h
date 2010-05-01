#ifndef _SNES_H_
#define _SNES_H_

typedef unsigned char u8;
typedef unsigned short u16;


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


// For NMI_TIMEN
#define JOY_MANUAL_READ		0x00
#define JOY_AUTO_READ		0x01

// For HVB_JOY
#define JOY_DISABLE			0x00
#define JOY_ENABLE			0x01




#endif



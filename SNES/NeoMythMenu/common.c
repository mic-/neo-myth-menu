// Common variables for the SNES Myth shell
// Mic, 2010-2011

#include "common.h"


// Default to the GBA card as the source for games
sourceMedium_t sourceMedium = SOURCE_GBAC;

// Callback function that handles keypresses for each menu screen.
// Set by the switch_menu function in navigation.c
void (*keypress_handler)(u16);

void (*recv_sd_psram_multi)(WORD, WORD, WORD);

u16 gbaCardAlphabeticalIdx[500];

itemList_t gamesList;

u16 hasGbacPsram = 0;
u16 useGbacPsram = 0;

u8 doRegionPatch = 0;		// Should we scan the game for region checks and patch them?
int sramBankOverride;

u8 snesRomInfo[0x40];		// 64-byte info block read from the ROM

u8 gSpcMirrorVal;			// Used by the VGM player
DWORD compressedVgmSize;	// Size of the currently loaded VGM after compression
DWORD vgzSize;
u8 compressVgmBuffer[512];	// Buffer used by the VGM compressor
u16 vgmPlayerLoaded = 0;
u16 isVgz;

char loadProgress[] = "Loading......(  )";


// These three strings are modified at runtime by the shell, so they are declared separately in order to get them into
// the .data section rather than .rodata.
//
char MS2[] = "\xff\x15\x02\x02 Loading......(  )";
char MS3[] = "\xff\x0b\x02\x02                        \xff\x09\x02\x07Secondary cart:";
char MS4[] = "\xff\x15\x02\x02 Game (001)";

// Each of these metastrings are on the format 0xff,row,column,palette,"actual text". A single metastring can go on
// indefinitely until a null-terminator is reached.
//
const char * const metaStrings[] =
{
    "\xff\x03\x01\x07 Shell v 0.56\xff\x02\x01\x03 NEO POWER SNES MYTH CARD (A)\xff\x1a\x04\x05\x22 2011 WWW.NEOFLASH.COM     ",
	"\xff\x01\xfe\x0f\x0a\x68\x69\x6A\x20\x71\x72\x73\x20\x7a\x7b\x7c\x83\x84\x85\xfe\x10\x0a\x6b\x6c\x6d\x20\x74\x75\x76\x20\x7d\x7e\x7f\x86\x87\x88xfe\x11\x0a\x6e\x6f\x70\x20\x77\x78\x79\x20\x80\x81\x82\x89\x8a\x8b\xfe\x17\x06\x06\xff\x04                             ",
	MS2,
	MS3,
	MS4,
	"\xff\x02\x1b\x03(B)",
	"\xff\x02\x1b\x03(C)",
	"\xff\x03\x15\x07(H/W 0.1)",
	"\xff\x03\x15\x07(H/W 0.1)",
	"\xff\x03\x15\x07(H/W 0.1)",
    "\xff\x16\x03\x02          ",  // 10
    "\xff\x16\x03\x02Save 2KB  ",
    "\xff\x16\x03\x02Save 8KB  ",
    "\xff\x16\x03\x02Save 32KB ",
    "\xff\x16\x03\x02Save 64KB ",
    "\xff\x16\x03\x02Save 128KB",
    "\xff\x15\x01\x02 DSP      ",
	" ",
	" ",
	" ",
	"\xff\x09\x14\x05 8M/64M",   // 20
	"\xff\x09\x14\x05\x31\x36M/64M",
	"\xff\x09\x14\x05\x32\x34M/64M",
	"\xff\x09\x14\x05\x33\x32M/64M",
	"\xff\x09\x14\x05\x34\x30M/64M",
	"\xff\x09\x14\x05\x34\x38M/64M",
	"\xff\x09\x14\x05\x35\x36M/64M",
	"\xff\x09\x14\x05\x36\x34M/64M",
	"\xff\x0a\x01\x05 A24 & A25 TEST OK !  ",
	" ",
	"\xff\x09\x01\x02 TESTING PSRAM... ", // 30
	"\xff\x0b\x01\x03 PSRAM TEST OK    ",
	"\xff\x0b\x01\x01 PSRAM TEST ERROR!",
	"\xff\x08\x10\x07\x86",		// Up arrow
	"\xff\x12\x10\x07\x87",		// Down arrow
	"\xff\x08\x10\x07 ",		// Up arrow clear
	"\xff\x12\x10\x07 ",		// Down arrow clear
	// Game sizes (strings nbr 37-47)
	"\xff\x15\x0f\x02\x34M ",
	"\xff\x15\x0f\x02?M ",
	"\xff\x15\x0f\x02?M ",
	"\xff\x15\x0f\x02?M ",
	"\xff\x15\x0f\x02\x38M ",
	"\xff\x15\x0f\x02\x31\x36M",
	"\xff\x15\x0f\x02\x32\x34M",
	"\xff\x15\x0f\x02\x33\x32M",
	"\xff\x15\x0f\x02\x34\x30M",
	"\xff\x15\x0f\x02\x34\x38M",
	"\xff\x15\x0f\x02\x36\x34M",
	// ROM types
	"\xff\x15\x13\x02HIROM",
	"\xff\x15\x13\x02LOROM",
	// DSP types (50-57)
	"\xff\x15\x19\x02    ",
	"\xff\x15\x19\x02\x44SP1",
	"\xff\x15\x19\x02\x44SP2",
	"\xff\x15\x19\x02\x44SP3",
	"\xff\x15\x19\x02\x44SP4",
	"\xff\x15\x19\x02\x44SP5",
	"\xff\x15\x19\x02\x44SP6",
	"\xff\x15\x19\x02SFX ",
	"\xff\x06\x02\x07Game  \xff\x17\x03\x03\x42\xff\x17\x04\x07: Run, \xff\x17\x0c\x03Y\xff\x17\x0d\x07: Go back\xff\x0d\x01\x07 System info:",
	// Regions (59-60)
    "\xff\x0f\x01\x02 NTSC (60HZ)",
    "\xff\x0f\x01\x02 PAL  (50HZ)",
	// CPU versions (61-64)
    "\xff\x10\x01\x02 S-CPU  V0",
    "\xff\x10\x01\x02 S-CPU  V1",
    "\xff\x10\x01\x02 S-CPU  V2",
    "\xff\x10\x01\x02 S-CPU  V3",
    // PPU1 versions
    "\xff\x11\x01\x02 S-PPU1 V0",
    "\xff\x11\x01\x02 S-PPU1 V1",
    "\xff\x11\x01\x02 S-PPU1 V2",
    "\xff\x11\x01\x02 S-PPU1 V3",
    // PPU2 versions
    "\xff\x12\x01\x02 S-PPU2 V0",
    "\xff\x12\x01\x02 S-PPU2 V1",
    "\xff\x12\x01\x02 S-PPU2 V2",
    "\xff\x12\x01\x02 S-PPU2 V3",
    // 73
	"\xff\x06\x02\x07Music\xff\x17\x03\x03RESET\xff\x17\x08\x07: Go back\xff\x09\x01\x07 SPC Playback",
    "\xff\x06\x02\x07Option\xff\x17\x03\x03Start\xff\x17\x08\x07: Run, \xff\x17\x0f\x03Y\xff\x17\x10\x07: Go back\xff\x16\x03\x03\x42\xff\x16\x04\x07: Edit  ",
    "\xff\x06\x02\x07Games\xff\x17\x03\x03\x42/X\xff\x17\x06\x07: Run, \xff\x17\x0e\x03Y\xff\x17\x0f\x07: Run 2nd cart",
    "\xff\x06\x02\x07\x43odes\xff\x17\x03\x03Start\xff\x17\x08\x07: Run, \xff\x17\x0f\x03Y\xff\x17\x10\x07: Go back\xff\x16\x03\x03X\xff\x16\x04\x07: Delete, \xff\x16\x0f\x03\x42\xff\x16\x10\x07: Edit  ",
    "\xff\x17\x03\x03\x42\xff\x17\x04\x07: Add, \xff\x17\x0f\x03Y\xff\x17\x10\x07: Delete\xff\x16\x03\x03\x44pad\xff\x16\x07\x07: Pick, \xff\x16\x0f\x03\x41\xff\x16\x10\x07: Cancel",
    "\xff\x06\x02\x07\x43heats\xff\x16\x03\x03Start\xff\x16\x08\x07: Run, \xff\x15\x0f\x03\x42\xff\x15\x10\x07: Add \xff\x15\x03\x03\x44pad\xff\x15\x07\x07: Pick, \xff\x16\x0f\x03Y\xff\x16\x10\x07: Go back",
    "\xff\x17\x03\x03\x42\xff\x17\x04\x07: Edit, \xff\x17\x0f\x03Y\xff\x17\x10\x07: Go back",
    // 80
    "\xff\x06\x02\x07Info\xff\x17\x03\x03Y\xff\x17\x04\x07: Go back",
    "\xff\x06\x02\x07\x45rror\xff\x17\x03\x03Y\xff\x17\x04\x07: Go back",
    "\xff\x06\x02\x07Info \xff\x17\x03\x03Y\xff\x17\x04\x07: Go back",
    "\xff\x06\x02\x07VGM  \xff\x16\x03\x03X\xff\x16\x04\x07: Toggle echo\xff\x17\x03\x03Y\xff\x17\x04\x07: Go back",
    // 84
	"\xff\x06\x02\x07Test  \xff\x17\x03\x03\x42\xff\x17\x04\x07: Run, \xff\x17\x0c\x03Y\xff\x17\x0d\x07: Go back",
};



void wait_nmi()
{
	while (REG_RDNMI & 0x80);
	while (!(REG_RDNMI & 0x80));
}


// Set the value of a pointer to a full 24-bit bank:offset pair
//
// E.g. set_full_pointer((void**)&a_pointer, 0x7f, 0x8000) will make
// a_pointer point to 0x7f8000.
//
void set_full_pointer(void **pptr, u8 bank, u16 offset)
{
	u8 *bp = (u8*)pptr;
	u16 *wp = (u16*)pptr;

	bp[2] = bank;
	*wp = offset;
}


// Add a 24-bit bank:offset pair to a pointer
//
// E.g. a_pointer = 0x500000; add_full_pointer((void**)&a_pointer, 0x12, 0x3456)
// will make a_pointer point to 0x623456
//
void add_full_pointer(void **pptr, u8 bank, u16 offset)
{
	u8 *bp = (u8*)pptr;
	u16 *wp = (u16*)pptr;
	u16 w;

	w = *wp;
	*wp += offset;

	bp[2] += bank;
	if (*wp < w)
	{
		bp[2]++;
	}
}

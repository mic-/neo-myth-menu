#include "../snes.h"

#ifndef _CHEAT_H_
#define _CHEAT_H_

enum
{
	CODE_UNUSED,
	CODE_TARGET_ROM,
	CODE_TARGET_RAM
};

enum
{
	CODE_TYPE_GG,
	CODE_TYPE_AR
};


typedef struct
{
	uint8 used;
	uint8 bank;
	uint8 val;
	uint16 offset;
	uint8 code[8];
} ggCode_t;

#define arCode_t ggCode_t


typedef struct
{
   const char *codes;
   const char *description;
   const uint8 codeType;
} cheat_t;


typedef struct
{
   uint16 romChecksum;
   uint16 romChecksumCompl;
   cheat_t const *cheats;
   uint16 numCheats;
} cheatDbEntry_t;


#endif

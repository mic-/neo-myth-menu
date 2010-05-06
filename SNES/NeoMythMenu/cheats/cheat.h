#include "../snes.h"

#ifndef _CHEAT_H_
#define _CHEAT_H_

#define CODE_UNUSED 0
#define CODE_TARGET_ROM 1
#define CODE_TARGET_RAM 2

#define CODE_TYPE_GG 1
#define CODE_TYPE_AR 2


typedef struct
{
	u8 used;
	u8 bank;
	u8 val;
	u16 offset;
	u8 code[8];
} ggCode_t;

#define arCode_t ggCode_t


typedef struct
{
   const char *codes;
   const char *description;
   const u8 codeType;
} cheat_t;


typedef struct
{
   u16 romChecksum;
   u16 romChecksumCompl;
   cheat_t const *cheats;
   u16 numCheats;
} cheatDbEntry_t;


#endif

#ifndef _util_h_
#define _util_h_
#include <z80/types.h>

extern void puts_asm(const char *str, BYTE attributes,WORD vram_addr);
#define puts(STR,X,Y,ATTR) puts_asm(STR,ATTR,(0x1800 + (X << 1) + (Y << 6)))

extern void putsn_asm(const char *str,BYTE attributes,BYTE max_chars,WORD vram_addr);
#define putsn(STR,X,Y,ATTR,MAX_CHRS) putsn_asm(STR,ATTR,MAX_CHRS,(0x1800 + (X << 1) + (Y << 6)))

extern void disable_ints(void);
extern void enable_ints(void);

extern BYTE strlen_asm(const BYTE* str);
#endif


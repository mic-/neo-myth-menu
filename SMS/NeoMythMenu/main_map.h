// Generated by map2h

#include <z80/types.h>

#define pfn_vdp_set_reg ((void (*)(BYTE, BYTE))0x063E)
#define pfn_vdp_set_vram_addr ((void (*)(WORD))0x0627)
#define pfn_vdp_set_cram_addr ((void (*)(WORD))0x0655)
#define pfn_vdp_copy_to_vram ((void (*)(WORD, BYTE *, WORD))0x0674)
#define pfn_vdp_set_color ((void (*)(BYTE, BYTE, BYTE, BYTE))0x06A3)
#define pfn_vdp_copy_to_cram ((void (*)(WORD, BYTE *, BYTE))0x06CF)
#define pfn_vdp_wait_vblank ((void (*)())0x06FA)
#define pfn_pad1_get_2button ((BYTE (*)(void))0x0789)
#define pfn_pad1_get_3button ((BYTE (*)(void))0x07A3)
#define pfn_pad1_get_6button ((WORD (*)(void))0x07F3)
#define pfn_pad1_get_mouse ((WORD (*)(void))0x086D)
#define pfn_pad2_get_2button ((BYTE (*)(void))0x0791)
#define pfn_pad2_get_3button ((BYTE (*)(void))0x07BF)
#define pfn_pad2_get_6button ((WORD (*)(void))0x082B)
#define pfn_pad2_get_mouse ((WORD (*)(void))0x0871)

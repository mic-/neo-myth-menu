#ifndef __SMS_H__
#define __SMS_H__

#include <z80/types.h>

__sfr __at 0x7F PsgPort;
__sfr __at 0xBE VdpData;
__sfr __at 0xBF VdpCtrl;


#define Frame1 (*(volatile BYTE *)(0xFFFE))
#define Frame2 (*(volatile BYTE *)(0xFFFF))


#endif


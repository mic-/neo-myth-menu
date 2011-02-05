#ifndef __SMS_H__
#define __SMS_H__

#include <z80/types.h>

/*
 * Memory Control Port
 * b2 = 1 = disable joysticks
 * b3 = 1 = disable int 8K bios rom
 * b4 = 1 = disable int 8K ram
 * b5 = 1 = disable card (slot) rom
 * b6 = 1 = disable cart rom
 * b7 = 1 = disable ext (rear) port
 */
__sfr __at 0x3E MemCtrl; /* out only */

/*
 * Joystick Control Port
 * b0 = P1 TH Direction (1 = in, 0 = out)
 * b1 = P1 TR Direction
 * b2 = P2 TH Direction
 * b3 = P2 TR Direction
 * b4 = P1 TH Output Level
 * b5 = P1 TR Output Level
 * b6 = P2 TH Output Level
 * b7 = P2 TR Output Level
 */
__sfr __at 0x3F JoyCtrl; /* out only */

/*
 * Vertical and Horizontal Count from VDP
 */
__sfr __at 0x7E VdpVCnt; /* in only */
__sfr __at 0x7E VdpHCnt; /* in only */

/*
 * PSG Control Port
 */
__sfr __at 0x7F PsgPort; /* out only */

/*
 * VDP Control/Data Ports
 */
__sfr __at 0xBE VdpData; /* in/out */
__sfr __at 0xBF VdpCtrl; /* out only */

/*
 * VDP Status Port
 * b5 = Sprite Collision
 * b6 = Sprite Overflow (9 sprites on raster line)
 * b7 = Interrupt Source - 1 = Frame, 0 = Line
 */
__sfr __at 0xBF VdpStat; /* in only */

/*
 * Joystick Port 1
 * b0 = P1 UP
 * b1 = P1 DOWN
 * b2 = P1 LEFT
 * b3 = P1 RIGHT
 * b4 = P1 TR / S1
 * b5 = P1 S2
 * b6 = P2 UP
 * b7 = P2 DOWN
 * Joystick Port 2
 * b0 = P2 LEFT
 * b1 = P2 RIGHT
 * b2 = P2 TR / S1
 * b3 = P2 S2
 * b4 = /RESET
 * b5 =
 * b6 = P1 TH - Light Gun Sync
 * b7 = P2 TH - Light Gun Sync
 */
__sfr __at 0xDC JoyPort1; /* in only */
__sfr __at 0xDD JoyPort2; /* in only */


/*
 * Frame 2 Control Byte
 * Write a byte to set how frame 2 is handled. The value is shadowed
 * in ram at 0xDFFC.
 *
 * b3 = 1 = SaveRAM in frame 2, 0 = ROM in frame 2
 * b2 = SRAM page (0 or 1) if SRAM mapped to frame 2
 */
#define Frm2Ctrl (*(volatile BYTE *)(0xFFFC))

/*
 * Frame Page Number
 * Write the page # to appear in the frame as a byte. The value is
 * shadowed in the ram at 0xDFFD to 0xDFFF. In frame 0, the first 1KB
 * is always page 0; only the last 15KB reflects the currently set
 * page.
 */
#define Frame0 (*(volatile BYTE *)(0xFFFD))
#define Frame1 (*(volatile BYTE *)(0xFFFE))
#define Frame2 (*(volatile BYTE *)(0xFFFF))

/*
 * Inline assembly macros
 */

#define disable_ints    \
        __asm           \
        di              \
        __endasm

#define enable_ints     \
        __asm           \
        ei              \
        __endasm


#endif

#ifndef __SMS_H__
#define __SMS_H__

#include "types.h"

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
 * b0 = P1 TR Direction (1 = in, 0 = out)
 * b1 = P1 TH Direction
 * b2 = P2 TR Direction
 * b3 = P2 TH Direction
 * b4 = P1 TR Output Level
 * b5 = P1 TH Output Level
 * b6 = P2 TR Output Level
 * b7 = P2 TH Output Level
 */
__sfr __at 0x3F JoyCtrl; /* out only */

/*
 * Vertical and Horizontal Count from VDP
 */
__sfr __at 0x7E VdpVCnt; /* in only */
__sfr __at 0x7F VdpHCnt; /* in only */

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
 * b4 = /RESET (SMS only, 1 on all others)
 * b5 = CONT line of cart, 1 = SMS/SMS2/GG, 0 = MD PBC
 * b6 = P1 TH - Light Gun Sync
 * b7 = P2 TH - Light Gun Sync
 */
__sfr __at 0xDC JoyPort1; /* in only */
__sfr __at 0xDD JoyPort2; /* in only */

/*
 * YM2413 Ports
 *
 * FMDetect
 * b0 = r/w - returns what was written if FM chip present
 * b1-b2 = 00 if FM chip present
 *
 * Note, you can only read FMDetect if you turn off the IO (Joystick)
 * ports by setting b2 of MemCtrl.
 */
__sfr __at 0xF0 FMAddr; /* out only */
__sfr __at 0xF1 FMData; /* out only */
__sfr __at 0xF2 FMDetect; /* in/out */

/*
 * CodeMasters Frame Page Number
 * CodeMasters' mapper uses three 16KB banks at 0x0000, 0x4000, and 0x8000.
 * Write a byte to the first byte of the bank to set the page of the bank.
 *
 * Note, most CodeMasters games only used the third bank.
 */
#define CMFrm0Ctrl (*(volatile BYTE *)(0x0000))
#define CMFrm1Ctrl (*(volatile BYTE *)(0x4000))
#define CMFrm2Ctrl (*(volatile BYTE *)(0x8000))

/*
 * Korean Frame Page Number
 * The Korean mapper uses a 16KB bank at 0x8000. Write a byte to 0xA000
 * to set the page of the bank.
 */
#define KMFrm2Ctrl (*(volatile BYTE *)(0xA000))

/*
 * BIOS shadow variable for MemCtrl
 */
#define MemCtrlShdw (*(volatile BYTE *)(0xC000))

/*
 * 3D Glasses Control Byte
 * Write a byte to switch which lens is currently closed. The control
 * reg mirrors at 0xFFF9, 0xFFFA, and 0xFFFB. It is shadowed in the
 * ram at 0xDFF8 to 0xDFFB.
 *
 * b0 = lens close select - 1 = right closed, 0 = left closed
 */
#define ThreeDCtrl (*(volatile BYTE *)(0xFFF8))

/*
 * Frame 2 Control Byte
 * Write a byte to set how frame 2 is handled. The value is shadowed
 * in ram at 0xDFFC.
 *
 * b0-b1 = page shift (00 for normal operation, unused by games)
 * b2 = cart RAM bank, 0 = first 16KB, 1 = second 16KB (only if RAM mapped to frame 2!)
 * b3 = 1 = cart RAM mapped at 0x8000 (frame 2), 0 = ROM at 0x8000
 * b4 = 1 = cart RAM mapped at 0xC000 (disable int RAM using MemCtrl first!), 0 = int RAM at 0xC000
 * b5-b6 = reserved
 * b7 = ROM WE, 0 = development ROM write protected, 1 = write enabled (unused by games)
 *
 * Note, setting b4-2 = 111 sets 0x8000-0xBFFF to cart RAM second 16KB,
 * and 0xC000-0xFFFF to cart RAM first 16KB, all at the same time for a
 * total of 32KB of RAM addressable by the CPU.
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
 * Bitmasks for MemCtrl
 */
#define ENABLE_INTERNAL_RAM     0x00
#define DISABLE_INTERNAL_RAM    0x10


/*
 * Enums for SMS regions
 * EXPORTED == Rest Of World (US/Europe/..)
 */
enum
{
    JAPANESE,
    EXPORTED
};


/*
 * Bitmasks for Frm2Ctrl
 */
#define SRAM_PAGE0              0x00
#define SRAM_PAGE1              0x04
#define FRAME2_AS_ROM           0x00
#define FRAME2_AS_SRAM          0x08



#endif

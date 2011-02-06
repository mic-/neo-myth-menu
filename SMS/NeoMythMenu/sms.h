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
 * b4 = /RESET
 * b5 = 1 = SMS/SMS2/GG, 0 = MD PBC
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
 * CodeMasters' mapper is fixed from 0x0000 to 0x7FFF, and has one bank
 * at 0x8000 to 0xBFFF. Write a byte to 0x8000 to set the page of the
 * bank.
 */
#define CMFrmCtrl (*(volatile BYTE *)(0x8000))

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
 * b0 = lens close select - if set, one lens close; if clr, the other
 */
#define ThreeDCtrl (*(volatile BYTE *)(0xFFF8))

/*
 * Frame 2 Control Byte
 * Write a byte to set how frame 2 is handled. The value is shadowed
 * in ram at 0xDFFC.
 *
 * b0-b1 = page shift (00 for normal operation)
 * b2 = SRAM page (0 or 1) if SRAM mapped to frame 2
 * b3 = 1 = SaveRAM in frame 2, 0 = ROM in frame 2
 * b4 = 1 = SRAM write protected
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
 * Enums for SMS regions
 * EXPORTED == Rest Of World (US/Europe/..)
 */
enum
{
    JAPANESE,
    EXPORTED
};


#endif

#ifndef _NEO2_H_
#define _NEO2_H_

#include "snes.h"

// Divide a 16-bit number by an 8-bit number and return the 16-bit quotient
extern u16 hw_div16_8_quot16(u16 dividend, u8 divisor);

// Divide a 16-bit number by an 8-bit number and return the 16-bit remainder
extern u16 hw_div16_8_rem16(u16 dividend, u8 divisor);

// Divide a 16-bit number by an 8-bit number and return the 8-bit quotient (in low byte) and 8-bit remainder (in high byte)
extern u16 hw_div16_8_quot8_rem8(u16 dividend, u8 divisor);

//extern void run_3800();

extern void copy_ram_code();

//extern void copy_1mbit_from_gbac_to_psram();

extern void run_game_from_gba_card();

// Used for running the secondary cart (plugged in at the back of the Myth)
extern void run_secondary_cart();

#endif

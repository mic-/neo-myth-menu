#ifndef _HW_MATH_H_
#define _HW_MATH_H_

#include "snes.h"

// Divide a 16-bit number by an 8-bit number and return the 16-bit quotient
extern u16 hw_div16_8_quot16(u16 dividend, u8 divisor);

// Divide a 16-bit number by an 8-bit number and return the 16-bit remainder
extern u16 hw_div16_8_rem16(u16 dividend, u8 divisor);

// Divide a 16-bit number by an 8-bit number and return the 8-bit quotient (in low byte) and 8-bit remainder (in high byte)
extern u16 hw_div16_8_quot8_rem8(u16 dividend, u8 divisor);

#endif


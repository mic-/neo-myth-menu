/*
	 One-bit font data. Each group of eight bytes represent one character.

	 E.g. 0x7C,0xC6,0x06,0x0C,0x30,0x60,0xFE,0x00 =>

	      %01111100			 				.*****..
	      %11000110           		     	**...**.
	      %00000110			 				.....**.
	      %00001100   or put more clearly:	....**..
	      %00110000			 				..**....
	      %01100000			 				.**.....
	      %11111110			 				*******.
	      %00000000			 				........
*/

#include "snes.h"

const u8 font[] =
{
        // 0x20,0x21
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x30,0x30,0x30,0x00,0x30,0x00,
    0x3C,0x42,0xB9,0xA1,0xB9,0x42,0x3C,0x00,0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00,
    0x10,0x7C,0xD2,0x7C,0x86,0x7C,0x10,0x00,0xF0,0x96,0xFC,0x18,0x3E,0x72,0xDE,0x00,
    0x30,0x48,0x30,0x78,0xCE,0xCC,0x78,0x00,0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00,
       // 0x28,0x29
    0x10,0x60,0xC0,0xC0,0xC0,0x60,0x10,0x00,  // '('
    0x10,0x0C,0x06,0x06,0x06,0x0C,0x10,0x00,  // ')'
    0x00,0x54,0x38,0xFE,0x38,0x54,0x00,0x00,  // '*'
    0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,  // '+'
    0x00,0x00,0x00,0x00,0x00,0x30,0x20,0x00,  // ','
    0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,  // '-'
    0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,
    0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00,

      //0x30,0x31
    0x7C,0xCE,0xDE,0xF6,0xE6,0xE6,0x7C,0x00,0x18,0x38,0x78,0x18,0x18,0x18,0x3C,0x00, // '0', '1'
    0x7C,0xC6,0x06,0x0C,0x30,0x60,0xFE,0x00,0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00,
    0x0E,0x1E,0x36,0x66,0xFE,0x06,0x06,0x00,0xFE,0xC0,0xC0,0xFC,0x06,0x06,0xFC,0x00,
    0x7C,0xC6,0xC0,0xFC,0xC6,0xC6,0x7C,0x00,0xFE,0x06,0x0C,0x18,0x30,0x60,0x60,0x00,
    0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00,0x7C,0xC6,0xC6,0x7E,0x06,0xC6,0x7C,0x00,
    0x00,0x30,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x30,0x20,0x00, // ':', ';'
    0x00,0x1C,0x30,0x60,0x30,0x1C,0x00,0x00,0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,
    0x00,0x70,0x18,0x0C,0x18,0x70,0x00,0x00,0x7C,0xC6,0x0C,0x18,0x30,0x00,0x30,0x00,
    0x7C,0x82,0x9A,0xAA,0xAA,0x9E,0x7C,0x00,0x7C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,
    0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00,0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00,
    0xFC,0x66,0x66,0x66,0x66,0x66,0xFC,0x00,0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00, // 'D', 'E'
    0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00,0x7C,0xC6,0xC6,0xC0,0xDE,0xC6,0x7C,0x00,
    0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,
    0x1E,0x0C,0x0C,0x0C,0x0C,0xCC,0x78,0x00,0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00,
    0xF0,0x60,0x60,0x60,0x60,0x62,0xFE,0x00,0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00,
    0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
    0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x0C,
    0xFC,0x66,0x66,0x7C,0x66,0x66,0xE6,0x00,0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00,
    0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
    0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00,0xC6,0xC6,0xC6,0xC6,0xD6,0xEE,0xC6,0x00,
    0xC6,0x6C,0x38,0x38,0x38,0x6C,0xC6,0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00,
    0xFE,0xC6,0x0C,0x18,0x30,0x66,0xFE,0x00,0x1C,0x18,0x18,0x18,0x18,0x18,0x1C,0x00,

    0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00,0x70,0x30,0x30,0x30,0x30,0x30,0x70,0x00,
    0x00,0x00,0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,
    0x07,0x0C,0x38,0x38,0x78,0x78,0x78,0x78,0xE4,0x1C,0x0C,0x0C,0x04,0x04,0x00,0x00,
    0x78,0x78,0x78,0x78,0x38,0x38,0x0C,0x07,0x00,0x00,0x02,0x02,0x04,0x0C,0x18,0xF0,
    0x3F,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x3F,

	0x10,0x38,0x7c,0xfe,0xfe,0,0,0, 0,0,0,0xfe,0xfe,0x7c,0x38,0x10,

            // 68   69
    0x00,0x00,0x00,0x00,0x09,0x0F,0x19,0x18,0x00,0x3F,0xFC,0x10,0xFF,0x10,0x93,0x90,
    0x00,0x00,0x00,0x30,0xF8,0x3C,0x30,0x00,0x11,0x01,0x00,0x07,0x04,0x04,0x07,0x0C,
    0x93,0x91,0x03,0xEF,0x44,0x46,0x83,0x1F,0x80,0x00,0x20,0xE0,0xC0,0x80,0xFC,0xC0,
    0x0F,0x1C,0x14,0x27,0x44,0x00,0x00,0x00,0xC3,0x4F,0x9B,0xC3,0x03,0x03,0x03,0x01,
    0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0x0F,0x19,0x18,
    0x00,0x3F,0xFC,0x10,0xFF,0x17,0xD3,0xD3,0x00,0x00,0x00,0xF0,0xF8,0x3C,0x60,0x00,
    0x19,0x00,0x07,0x06,0x06,0x06,0x06,0x0F,0xD3,0x87,0xFF,0x31,0x73,0xC5,0xFF,0xE7,
    0x00,0xC0,0xC0,0xC0,0x80,0xE0,0xF0,0xC0,0x0C,0x0D,0x1A,0x18,0x30,0x60,0xCF,0x00,
    0xFD,0xF9,0x4D,0xCF,0xC8,0x4F,0xFF,0x00,0x78,0x3E,0x00,0x80,0x00,0xF8,0xF8,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x70,0x70,0x30,0x20,0x20,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x18,0x18,0x30,0x30,0x70,0x60,
    0x20,0x21,0x20,0x20,0x20,0x20,0x20,0x30,0x00,0x80,0xE0,0xE0,0x70,0x30,0x30,0x00,
    0x60,0x00,0x03,0x01,0x00,0x00,0x00,0x00,0x30,0x30,0xF0,0xF0,0xE0,0x60,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x03,0x00,0x00,
    0x00,0x00,0x01,0x3F,0xF3,0x07,0x06,0x3C,0x00,0x00,0x80,0xC0,0xC0,0x80,0x00,0x00,
    0x00,0x00,0x00,0x7F,0x38,0x00,0x00,0x00,0x18,0x18,0xFF,0xC8,0x0C,0x0C,0x0C,0x0C,
    0x00,0x3C,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0C,0x0C,0x1C,0xD8,0x78,0x38,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    0x00,0x00,0x00,0x00,0x04,0x04,0x0C,0x0C,0x40,0x70,0x60,0x60,0x63,0x61,0x60,0x60,
    0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xC0,0x18,0x08,0x00,0x00,0x00,0x3F,0x18,0x00,
    0xE0,0x60,0x00,0x63,0x7F,0xE0,0x60,0x60,0x00,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,
    0x00,0x00,0x01,0x03,0x0E,0x78,0x00,0x00,0xD0,0x88,0x8C,0x07,0x03,0x03,0x01,0x00,
    0x00,0x00,0x00,0x00,0x80,0xF0,0xFC,0x00,0x00,0x00,0x00,0x04,0x07,0x07,0x03,0x03,
    0x06,0x0F,0x3E,0x70,0x80,0x00,0x03,0x3F,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,
    0x03,0x03,0x03,0x03,0x03,0x03,0xFF,0xF8,0xFC,0x0C,0x0C,0x08,0x0F,0xFF,0x80,0x00,
    0x00,0x00,0x00,0x00,0xF8,0xFC,0x00,0x00,0x01,0x03,0x07,0x0F,0x1C,0x30,0x00,0x00,
    0x86,0xC3,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0xE0,0x60,0x00,0x00,

           // 9E     ;9F
    0x01,0xFF,0x3F,0x21,0x3F,0x21,0x3F,0x01,0x00,0xFE,0xF8,0x08,0xF8,0x08,0xF8,0x04,
    0x7F,0x00,0xFF,0x10,0x08,0x00,0x00,0x00,0xFC,0x10,0xFE,0x10,0x10,0x30,0x00,0x00,
    0x09,0x08,0x1F,0x10,0x3F,0x50,0x9F,0x10,0x00,0x80,0xF8,0x80,0xF8,0x80,0xF8,0x80,
    0x1F,0x00,0x1F,0x10,0x10,0x1F,0x00,0x00,0xFC,0x00,0xF8,0x08,0x08,0xF8,0x00,0x00,
    0x04,0x07,0x08,0x14,0x22,0x01,0x06,0x18,0x00,0xF0,0x10,0x20,0x40,0x80,0x60,0x1E,
    0xE0,0x0F,0x08,0x08,0x0F,0x08,0x00,0x00,0x04,0xF0,0x10,0x10,0xF0,0x10,0x00,0x00,
    0x08,0x1D,0xF0,0x13,0xFC,0x11,0x31,0x39,0x08,0xF0,0x20,0xFE,0x20,0xFC,0x24,0xFC,
    0x55,0x91,0x10,0x11,0x10,0x13,0x00,0x00,0x24,0xFC,0x20,0xFC,0x20,0xFE,0x00,0x00,
    0xFF,0x01,0x7F,0x51,0x89,0x11,0x01,0x3F,0xFE,0x00,0xFE,0x12,0x24,0x10,0x00,0xF8,
    0x21,0x3F,0x21,0x3F,0x01,0x00,0x00,0x00,0x08,0xF8,0x08,0xF8,0x00,0xFC,0x00,0x00,
    0x20,0x11,0xFD,0x05,0x09,0x11,0x31,0x59,0x00,0xFC,0x04,0xFC,0x04,0xFC,0x04,0xFC,
    0x94,0x10,0x10,0x10,0x11,0x12,0x00,0x00,0x50,0x50,0x50,0x92,0x12,0x0E,0x00,0x00,
    0x82,0x42,0x0F,0xF4,0x17,0x24,0x44,0x84,0x10,0x20,0xBC,0x00,0xBC,0x88,0x90,0xBC,
    0x48,0x20,0x22,0x21,0x40,0xBF,0x00,0x00,0x88,0x88,0xA8,0x10,0x00,0xFE,0x00,0x00,
    0x22,0x4F,0xF4,0x27,0x54,0xF7,0x01,0x7F,0x08,0xD2,0x5C,0xC8,0x52,0xDE,0x00,0xFC,
    0x01,0x09,0x11,0x21,0xC1,0x01,0x00,0x00,0x00,0x20,0x10,0x0C,0x04,0x00,0x00,0x00,
    0x3E,0x22,0x22,0x3E,0x01,0x01,0xFF,0x02,0xF8,0x88,0x88,0xF8,0x20,0x10,0xFE,0x80,
    0x0C,0x30,0xFE,0x22,0x22,0x3E,0x00,0x00,0x60,0x18,0xFE,0x88,0x88,0xF8,0x00,0x00,
    0x03,0x0C,0x18,0x30,0x30,0x30,0x30,0x30,0xC4,0x34,0x1C,0x0C,0x04,0x00,0x00,0x00,
    0x30,0x30,0x30,0x18,0x0C,0x03,0x00,0x00,0x00,0x04,0x0C,0x18,0x30,0xC0,0x00,0x00,
    0x3F,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0xC0,0x70,0x1C,0x0C,0x0C,0x0C,0x0C,0x0C,
    0x18,0x18,0x18,0x18,0x18,0x3F,0x00,0x00,0x0C,0x0C,0x0C,0x18,0x70,0xC0,0x00,0x00,
    0x02,0x02,0x03,0x02,0x02,0xFF,0x02,0x02,0x00,0x20,0xF0,0x00,0x04,0xFE,0x00,0x00,
    0x02,0x02,0x02,0x02,0x02,0x02,0x00,0x00,0x80,0x60,0x30,0x10,0x00,0x00,0x00,0x00,
    0x24,0x24,0xFF,0x24,0x47,0x00,0x7F,0x41,0x48,0x48,0xFE,0x48,0xCE,0x00,0xFE,0x02,
    0x9F,0x11,0x11,0x11,0x11,0x01,0x00,0x00,0xF4,0x10,0x10,0x50,0x20,0x00,0x00,0x00,
    0x01,0x01,0x01,0x7F,0x41,0x41,0x41,0x7F,0x00,0x00,0x04,0xFE,0x04,0x04,0x04,0xFC,
    0x41,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x01,0x01,0x01,0xFF,0x01,0x01,0x01,0x1F,0x00,0x00,0x04,0xFE,0x00,0x00,0x10,0xF8,
    0x10,0x10,0x10,0x10,0x1F,0x10,0x00,0x00,0x10,0x10,0x10,0x10,0xF0,0x10,0x00,0x00,
    0x01,0x01,0xFF,0x08,0x10,0x28,0x44,0x02,0x00,0x04,0xFE,0x10,0x0C,0x24,0x40,0x80,
    0x01,0x02,0x04,0x08,0x30,0xC0,0x00,0x00,0x00,0x80,0x40,0x30,0x0E,0x04,0x00,0x00,
    0x20,0x21,0x22,0xF8,0x23,0x2A,0x32,0x63,0x80,0xF8,0x10,0x20,0xFC,0x44,0xA4,0x14,
    0xA2,0x2F,0x20,0x21,0xA2,0x4C,0x00,0x00,0x44,0xFE,0xA0,0x10,0x08,0x06,0x00,0x00,
    0x02,0x01,0xF8,0x27,0x41,0x42,0x74,0xD7,0x08,0x98,0xA0,0xFE,0x08,0x52,0xA4,0xBC,
    0x51,0x52,0x54,0x74,0x57,0x00,0x00,0x00,0x08,0x10,0xA4,0xA2,0xFE,0x42,0x00,0x00,
    0x10,0x10,0x10,0x10,0x1F,0x10,0x10,0x1F,0x10,0x10,0x10,0x10,0xF0,0x00,0x00,0xF0,
    0x10,0x10,0x10,0x10,0x20,0x40,0x00,0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,
    0x11,0x10,0x7C,0x10,0x10,0xFE,0x10,0x50,0xFC,0x44,0x44,0x44,0x54,0x88,0x00,0xFC,
    0x5E,0x50,0x50,0x70,0x48,0x87,0x00,0x00,0x84,0x84,0x84,0xFC,0x00,0xFE,0x00,0x00,
    0x08,0x08,0x0B,0x10,0x30,0x50,0x9F,0x10,0x00,0x10,0xF8,0x40,0x40,0x44,0xFE,0x40,
    0x10,0x10,0x10,0x17,0x10,0x10,0x00,0x00,0x40,0x40,0x48,0xFC,0x00,0x00,0x00,0x00
};


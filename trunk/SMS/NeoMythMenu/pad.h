#ifndef __PAD_H__
#define __PAD_H__

#include <stdint.h>
#include <z80/types.h>
#include "sms.h"


enum
{
    PAD_UP      = 1,
    PAD_DOWN    = 2,
    PAD_LEFT    = 4,
    PAD_RIGHT   = 8,
    PAD_SW1     = 16,
    PAD_SW2     = 32,
    PAD_B       = 16,
    PAD_C       = 32,
    PAD_A       = 64,
    PAD_START   = 128,
    PAD_Z       = 256,
    PAD_Y       = 512,
    PAD_X       = 1024,
    PAD_MODE    = 2048
};


BYTE pad_get_2button(BYTE port);
BYTE pad_get_3button(BYTE port);
WORD pad_get_6button(BYTE port);
WORD pad_get_mouse(BYTE port);


#endif

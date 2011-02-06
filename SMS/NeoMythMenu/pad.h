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


BYTE pad1_get_2button(void);
BYTE pad1_get_3button(void);
WORD pad1_get_6button(void);
WORD pad1_get_mouse(void);

BYTE pad2_get_2button(void);
BYTE pad2_get_3button(void);
WORD pad2_get_6button(void);
WORD pad2_get_mouse(void);


#endif

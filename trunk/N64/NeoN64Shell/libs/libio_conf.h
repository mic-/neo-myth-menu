#ifndef _libio_conf_h_
#define _libio_conf_h_

#undef IO_TARGET_PLATFORM_SNES
#undef IO_TARGET_PLATFORM_MD32X
#undef SNES_LOVELY_POINTERS
#define IO_TARGET_PLATFORM_N64

/*
#ifdef IO_TARGET_PLATFORM_SNES
    #ifndef defined
        #define defined(__WHAT__) ( (__WHAT__) != 0 )
    #endif
#endif
*/

/*MD / 32X*/
#if (IO_TARGET_PLATFORM_MD32X)
    #define MAX_FILE_HANDLES (16)
    #define MAX_DIR_HANDLES (8)
    #define DISABLE_INTERRUPTS asm("move.w 0x2700,sr")
    #define ENABLE_INTERRUPTS asm("move.w 0x2000,sr")
#endif
    
/*SNES*/
#if defined(IO_TARGET_PLATFORM_SNES)
    #define MAX_FILE_HANDLES (8)
    #define MAX_DIR_HANDLES (4)
    #define DISABLE_INTERRUPTS asm("sei")
    #define ENABLE_INTERRUPTS asm("cli")
#endif

/*N64*/
#if defined(IO_TARGET_PLATFORM_N64)
    #define DISABLE_INTERRUPTS asm("\tmfc0 $8,$12\n\tla $9,~1\n\tand $8,$9\n\tmtc0 $8,$12\n\tnop":::"$8","$9")
    #define ENABLE_INTERRUPTS asm("\tmfc0 $8,$12\n\tori $8,1\n\tmtc0 $8,$12\n\tnop":::"$8")
#endif

#ifndef DISABLE_INTERRUPTS
    #define DISABLE_INTERRUPTS
#endif

#ifndef ENABLE_INTERRUPTS
    #define ENABLE_INTERRUPTS
#endif

#endif


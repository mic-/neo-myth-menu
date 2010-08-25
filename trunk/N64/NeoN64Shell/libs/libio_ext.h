#ifndef _libio_ext_h_
#define _libio_ext_h_
#include "libio_conf.h"

/*LIBIO Extensions*/

/*
    Prototype functions should look like this:

    Write methods(Returns bytes written):
                    uint32_t nio_###_write(const void*,uint32_t size,uint32_t addr)

    Read methods(Returns bytes written):
                    uint32_t nio_###_write(void*,uint32_t size,uint32_t addr)

    To override the dummy neo io functions , just define them and map your functions.

    For interrupt handling on all platforms , use ENABLE_INTERRUPTS and DISABLE_INTERRUPTS ASM-Macros.

    If you need to know the size of each "device" look at the constants below.
    If you wish to override a constant for a specific platform , just define it in the proper section
    otherwise the initial configuration will be triggered.
*/

#if defined(IO_TARGET_PLATFORM_SNES)
/*SNES- map nio_####_#### here*/

#elif defined(IO_TARGET_PLATFORM_MD32X)
/*MD/32X- map nio_####_#### here*/

#elif defined(IO_TARGET_PLATFORM_N64)
/*N64 - map nio_####_#### here*/

#endif

/*consts*/
#ifndef EEPROM_SIZE
    #define EEPROM_SIZE (131072)
#endif

#ifndef SRAM_SIZE
    #define SRAM_SIZE (2 * 131072)
#endif

#ifndef GBA_SRAM_SIZE
    #define GBA_SRAM_SIZE (16 * 131072)
#endif

#ifndef PSRAM_SIZE
    #define PSRAM_SIZE (256 * 131072)
#endif

#ifndef MYTH_PSRAM_SIZE
    #define MYTH_PSRAM_SIZE (64 * 131072)
#endif

/*Dummy Neo IO Routines*/
#ifndef nio_psram_write
    #define nio_psram_write(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_psram_read
    #define nio_psram_read(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_myth_psram_write
    #define nio_myth_psram_write(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_myth_psram_read
    #define nio_myth_psram_read(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_eeprom_write
    #define nio_eeprom_write(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_eeprom_read
    #define nio_eeprom_read(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_sram_read
    #define nio_sram_read(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_sram_write
    #define nio_sram_write(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_sram_read
    #define nio_sram_read(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_gba_sram_write
    #define nio_gba_sram_write(DATA,SIZE,ADDRESS) (0)
#endif

#ifndef nio_gba_sram_read
    #define nio_gba_sram_read(DATA,SIZE,ADDRESS) (0)
#endif

#endif


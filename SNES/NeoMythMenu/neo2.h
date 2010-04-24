#ifndef _NEO2_H_
#define _NEO2_H_

#include "snes.h"

extern void copy_ram_code();

extern void run_game_from_gba_card();

// Used for running the secondary cart (plugged in at the back of the Myth)
extern void run_secondary_cart();

extern void play_spc_from_gba_card();

extern void mosaic_up();
extern void mosaic_down();

#endif

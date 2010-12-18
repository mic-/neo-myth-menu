#ifndef _NEO2_H_
#define _NEO2_H_

#include "snes.h"

extern void copy_ram_code();

extern void neo2_myth_current_rom_read(char *dest, u16 romBank, u16 romOffset, u16 length);

extern void neo2_myth_psram_read(char *dest, u16 psramBank, u16 psramOffset, u16 length);
extern void neo2_myth_psram_write(char *src, u16 psramBank, u16 psramOffset, u16 length);

extern void neo2_recv_sd(unsigned char *buf);
extern void neo2_pre_sd();
extern void neo2_post_sd();
extern void neo2_enable_sd();
extern void neo2_disable_sd();

extern void run_game_from_gba_card();
extern void run_game_from_sd_card();

// Used for running the secondary cart (plugged in at the back of the Myth)
extern void run_secondary_cart();

extern void play_spc_from_gba_card();
extern void play_spc_from_sd_card();
extern void play_vgm_from_sd_card();
extern void stop_vgm();

extern void show_loading_progress();

//extern void get_rom_info();

#endif

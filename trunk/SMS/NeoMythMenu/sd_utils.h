#ifndef __SD_UTILS_H__
#define __SD_UTILS_H__

#include "shared.h"

int init_sd();
void change_directory(char *);
void read_file_to_psram(FileInfoEntry *fi, BYTE prbank, WORD proffs);
void sdutils_sram_to_sd(const char* filename);
void sdutils_sram_cls();
#endif

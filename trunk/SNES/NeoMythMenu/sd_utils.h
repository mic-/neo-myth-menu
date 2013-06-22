#ifndef _SD_UTILS_H_
#define _SD_UTILS_H_

#include "pff.h"
#include "common.h"


extern FATFS sdFatFs;
extern DIR sdDir;
extern FILINFO sdFileInfo;
extern char sdRootDir[200];
extern u16 sdRootDirLength;
extern int lastSdError, lastSdOperation;
extern char *lastSdParam;

int init_sd();
void change_directory(char *path);
sourceMedium_t set_source_medium(sourceMedium_t newSource, u16 silent);
u16 count_games_on_sd_card();

#endif

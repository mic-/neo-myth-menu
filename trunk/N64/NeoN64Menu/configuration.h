#ifndef _configuration_h_
#define _configuration_h_

#include "../NeoN64Shell/libs/libconf.h"
#include <stdint.h>
int32_t config_test_sdc_speed();
void config_create_initial_settings(const char* filename);
void config_save(const char* filename);
void config_load(const char* filename);
#endif


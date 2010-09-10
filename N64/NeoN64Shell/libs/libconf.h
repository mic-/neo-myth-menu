
#ifndef _CONF_H_INCLUDED_
#define _CONF_H_INCLUDED_

#include <stdint.h>
//A very simple config file format with the ability to save the contents to a file or buffer
//Config template 
/*
        variable = value
        anothervariable = anothervalue
*/

typedef struct ConfigEntry ConfigEntry;
struct ConfigEntry
{
        char* variable;
        char* value;
        int32_t variableLength;
        int32_t valueLength;
        ConfigEntry* next;
};

void config_init();
void config_shutdown();
ConfigEntry* config_push(const char* variable,const char* value);
ConfigEntry* config_pushI(const char* variable,const int32_t value);
ConfigEntry* config_pushF(const char* variable,const float value);
ConfigEntry* config_pushD(const char* variable,const double value);
ConfigEntry* config_pushHex(const char* variable,const uint32_t value);
ConfigEntry* config_find(const char* variable);
void config_remove(const char* variable);
ConfigEntry* config_replaceS(const char* variable,const char* newValue);
ConfigEntry* config_replace(ConfigEntry* e,const char* newValue);
char* config_getS(const char* variable);
char* config_getHex(const char* variable);
int32_t config_getI(const char* variable);
float config_getF(const char* variable);
double config_getD(const char* variable);
int32_t config_loadFromBuffer(const char* buf,const int32_t size);
int32_t config_saveToBuffer(unsigned char* buf);
int32_t config_getEntryCount();
uint32_t config_predictOutputBufferSize();
#endif


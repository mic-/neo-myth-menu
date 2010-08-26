
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
        int8_t* variable;
        int8_t* value;
        int32_t variableLength;
        int32_t valueLength;
        ConfigEntry* next;
};

void config_init();
void config_shutdown();
void config_remove(const int8_t* variable);

ConfigEntry* config_push(const int8_t* variable,const int8_t* value);
ConfigEntry* config_find(const int8_t* variable);
ConfigEntry* config_replaceS(const int8_t* variable,const int8_t* newValue);
ConfigEntry* config_replace(ConfigEntry* e,const int8_t* newValue);

int8_t* config_getS(const int8_t* variable);
int32_t config_getI(const int8_t* variable);

int32_t config_loadFromBuffer(const int8_t* buf,const int32_t size);
int32_t config_saveToBuffer(int8_t* buf);
int32_t config_getEntryCount();

#endif


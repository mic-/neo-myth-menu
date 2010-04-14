//By conle (conleon1988@gmail.com) for ChillyWilly's Extended SD Menu
#ifndef _CONF_H_INCLUDED_
#define _CONF_H_INCLUDED_

//A very simple config file format with the ability to save the contents to a file or buffer
//Config template 
/*
	variable = value
	anothervariable = anothervalue
*/

enum
{
	CONF_ENTRIES_COUNT = 15,//max entries
	CONF_ENTRY_BUFFER_Va_SIZE = 16,//variable buffer
	CONF_ENTRY_BUFFER_Pa_SIZE = 96//value buffer
};

typedef struct ConfigEntry ConfigEntry;

struct ConfigEntry
{
	char variable[CONF_ENTRY_BUFFER_Va_SIZE];
	char value[CONF_ENTRY_BUFFER_Pa_SIZE];
	short variableLength;
	short valueLength;
};

void config_init();

ConfigEntry* config_push(const char* variable,const char* value);
ConfigEntry* config_find(const char* variable);
ConfigEntry* config_replaceS(const char* variable,const char* newValue);
ConfigEntry* config_replace(ConfigEntry* e,const char* newValue);
ConfigEntry* config_getfromIndex(int index);

char* config_getS(const char* variable);
int config_getI(const char* variable);

int config_loadFromBuffer(const char* buf,const int size);
int config_saveToBuffer(char* buf);
int config_getEntryCount();

#endif


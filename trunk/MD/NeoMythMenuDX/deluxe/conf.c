//By conle (conleon1988@gmail.com) for ChillyWilly's Extended SD Menu
#include "conf.h"
#include <stdio.h>
#include <string.h>

static ConfigEntry confList[CONF_ENTRIES_COUNT];
static short confListPtr = 0;

inline int isSpace(char c)
{
    switch(c)
    {
        case 0x9:
        case 0xa:
        case 0xb:
        case 0xc:
        case 0xd:
        case 0x20:
            return 1;

        default:
            return 0;
    }

    return 0;
}

inline int isNumerical(char c)
{
    if( ( c - '0' ) < 0 )
        return 0;
    else if( ( c - '0' ) > 9 )
        return 0;

    return 1;
}

//global functions
void config_init()
{
	ConfigEntry* e;
	short i;
	confListPtr = 0;


	for(i = 0; i <CONF_ENTRIES_COUNT; i++)
	{
		e = &confList[i];
		e->variable[0] = e->value[0] = '\0';
		e->variableLength = e->valueLength = 0;
	}	
}

ConfigEntry* config_pushObj()
{
	ConfigEntry* e;

	if(confListPtr > CONF_ENTRIES_COUNT-1)
		return NULL;

	e = &confList[confListPtr++];

	return e;
}

ConfigEntry* config_push(const char* variable,const char* value)
{
	ConfigEntry* e;

	e = config_find(variable);

	if(!e)
	{
		if(confListPtr > CONF_ENTRIES_COUNT-1)
			return NULL;

		e = &confList[confListPtr++];

		e->variableLength = strlen(variable);

		if(!e->variableLength)
		{
			--confListPtr;
			return NULL;
		}

		e->valueLength = strlen(value);

		strcpy(e->variable,variable);
		strcpy(e->value,value);
	}
	else
	{
		//just update
		e->valueLength = strlen(value);
		strcpy(e->value,value);
	}

	return e;
}

ConfigEntry* config_find(const char* variable)
{
	ConfigEntry* e;
	short a;

	for(a = 0; a < confListPtr; a++)
	{
		e = &confList[a];
		
		//printf("(!) [%s] [%s] [%d] [%d]\n",e->variable,e->value,e->variableLength,e->valueLength);

		if( memcmp(e->variable,variable,(int)e->variableLength) == 0 )
			return e;		
	}

	return NULL;
}

ConfigEntry* config_replaceS(const char* variable,const char* newValue)
{
	ConfigEntry* e;

	e = config_find(variable);

	if(!e)
		return NULL;

	e->valueLength = strlen(newValue);
	strcpy(e->value,newValue);

	return e;
}

ConfigEntry* config_replace(ConfigEntry* e,const char* newValue)
{
	e->valueLength = strlen(newValue);
	strcpy(e->value,newValue);

	return e;
}

char* config_getS(const char* variable)
{
	ConfigEntry* e;

	e = config_find(variable);

	if(!e)
		return NULL;

	return e->value;
}

int config_getI(const char* variable)
{
	const char* str;
    const char* base;
    int result = 0;
    unsigned char neg = 0;
	ConfigEntry* e;

	e = config_find(variable);

	if(!e)
		return 0;

	str = base = e->value;

    while(*str)
    {
        if(!isSpace(*str))
            break;

        ++str;
    }

    if(str[0] == '0')
        return 0;
    else if(str[0] == '-')
    {
        ++str;
        neg = 1;
    }

	while(*str)
	{
	    if(str - base > 9)
            break;
        else if(!isNumerical(*str))
            break;

		result = (result * 10) + (*str - '0');
		str++;
	}

    if(neg)
        result *= -1;

	return result;
}

inline ConfigEntry* internal_config_push(const char* variable,const char* value,const short varLen,const short valLen)
{
	ConfigEntry* e;

	if(confListPtr > CONF_ENTRIES_COUNT-1)
		return NULL;

	e = &confList[confListPtr++];

	e->variableLength = varLen;

	if(!e->variableLength)
	{
		--confListPtr;
		return NULL;
	}

	e->valueLength = valLen;

	strcpy(e->variable,variable);
	strcpy(e->value,value);

	return e;
}

int config_loadFromBuffer(const char* buf,const int size)
{
	const char* pa = buf;
	int ind = 0;
	ConfigEntry obj;

	config_init();

	while(*pa)
	{
		{
			//reset pointer
			ind = 0;

			obj.variable[0] = obj.value[0] = '\0';
			obj.variableLength = obj.valueLength = 0;

			//skip head WS
			while(*pa){ if(isSpace(*pa))pa++;else break;}

			//load variable
			while(*pa)
			{
				if(*pa == '=')
					break;
				else if(*pa == ' ')
					break;
				else
				{
					if(ind > CONF_ENTRY_BUFFER_Va_SIZE-1)
						break;

					{
						if(! (*pa) )
							break;

						if(*pa == '\r')
						{
							++pa;

							if(*pa == '\n')
								++pa;

							break;
						}
						else if(*pa == '\n')
						{
							++pa;
							break;
						}
					}

					obj.variable[ind++] = *pa;
				}

				++pa;
			}

			obj.variable[ind] = '\0';
			obj.variableLength = ind;
			ind = 0;

			if( (*pa == '='))
			{
				++pa;

				//skip WS after assignment --if exists
				while(*pa){ if(isSpace(*pa))pa++;else break;}

				//load value
				while(*pa)
				{
					if(*pa == '\r') 
					{
						++pa;

						if(*pa == '\n')
						{
							++pa;
							break;
						}
					}
					else if(*pa == '\n')
					{
						++pa;
						break;
					}
					else
					{
						if(ind > CONF_ENTRY_BUFFER_Pa_SIZE-1)
							break;

						{
							if(! (*pa) )
								break;

							if(*pa == '\r')
							{
								++pa;

								if(*pa == '\n')
									++pa;

								break;
							}
							else if(*pa == '\n')
							{
								++pa;
								break;
							}
						}

						obj.value[ind++] = *pa;
					}

					++pa;
				}

				obj.value[ind] = '\0';
				obj.valueLength = ind;

				if(obj.variableLength)
				{
					//printf("Adding [%s] with data [%s]\n",obj.variable,obj.value);
					internal_config_push(obj.variable,obj.value,obj.variableLength,obj.valueLength);//config_push(obj.variable,obj.value);
				}
			}
		}
	}

	return 0;
}

int config_saveToBuffer(char* buf)
{
	ConfigEntry* e;
	short a;
	char* p;

	if(!confListPtr)
		return 1;

	p = &buf[0];

	for(a = 0; a < confListPtr; a++)
	{
		e = &confList[a];

		memcpy(p,e->variable,e->variableLength); p += e->variableLength;
		*p = '=';  p += 1;
		memcpy(p,e->value,e->valueLength); p += e->valueLength;
		*p = '\r'; p += 1;//Windows style newline \r\n
		*p = '\n'; p += 1;
	}

	*p = '\0';

	return 0;
}

int config_getEntryCount()
{
	return confListPtr;
}

ConfigEntry* config_getfromIndex(int index)
{
	return &confList[index];
}


#include "libconf.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static ConfigEntry* entry = NULL;
static ConfigEntry* entries = NULL;
static char* confConvBuf = NULL;

/*********************************************** PRIVATE BEGIN *********************************************************/
static int32_t string_length(register const char* src)
{
    register const char* pa = src;

    while(*(pa++) != '\0');

    return (int32_t)(pa - src);
}

static char* clone_string(const char* src,int32_t* saveLength)
{
    int32_t len = string_length(src);

    if(saveLength)
        *saveLength = len;

    char* res = (char*)malloc( ( ((len + 1)&1) ? (len + 2) : (len + 1) ) );
        
    if(!res)
        return NULL;

    memcpy(res,src,len);
    res[len] = '\0';
    return res;
}

static ConfigEntry* push_obj()
{
    if(!entry)
    {
        entry = (ConfigEntry*)malloc(sizeof(ConfigEntry));
        entries = entry;
        entry->next = NULL;

        return entry;
    }

    entries->next = (ConfigEntry*)malloc(sizeof(ConfigEntry));
    entries = entries->next;
    entries->next = NULL;

    return entries;
}

inline int32_t is_space(int32_t c)
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

inline int32_t is_num(int32_t c)
{
    c = c - '0';
    return ((c  >= 0) && (c <= 9));
}

static int32_t skip_whitespace(const char* src)
{
    const char* sp = src;

    while(is_space(*src))
        ++src;

    return src - sp;
}

static int32_t get_token(char* dst,const char* src,const int32_t srcSize,const int32_t allow_sp,int32_t* len,int32_t* eof)
{
    const char* sp = src;

    *eof = *len = 0;

    src += skip_whitespace(src);

    while(*src)
    {
        if(*src == '=')
            break;
        else if(src >= src + srcSize)
        {
            *eof = 1;
            break;
        }

        if(!allow_sp)
        {
            if( is_space(*src) && (!is_num(*src)) )
                break;
        }
        else
        {
            if(*src == '\r' || *src == '\n')
                break;
        }

        *(dst++) = *(src++);
    }

    src += skip_whitespace(src);

    *dst = '\0';
    *len = (int32_t)(src - sp);

    return *len;
}

/*********************************************** PRIVATE END *********************************************************/

/*********************************************** PUBLIC BEGIN *********************************************************/
void config_init()
{
    entry = entries = NULL;

	if(!confConvBuf)
		confConvBuf = (char*)malloc(256);

	//pointless? :)
	if(!confConvBuf)
		confConvBuf = (char*)malloc(128);

	if(!confConvBuf)
		confConvBuf = (char*)malloc(64);
}

void config_shutdown()
{
    while(entry)
    {
        if(entry->variable)
            free(entry->variable);

        if(entry->value)
            free(entry->value);

        entry = entry->next;
    }

	if(confConvBuf)
		free(confConvBuf);

	confConvBuf = NULL;
    entry = entries = NULL;
}

ConfigEntry* config_push(const char* variable,const char* value)
{
    ConfigEntry* e;

    e = config_find(variable);

    if(!e)
    {
        e = push_obj();

        if(!e)
            return NULL;

        e->variable = clone_string(variable,&e->variableLength);
        e->value = clone_string(value,&e->valueLength);
    }
    else
    {
        //just update
        if(e->value)
            free(e->value);

        e->value = clone_string(value,&e->valueLength);
    }
    return e;
}

ConfigEntry* config_pushI(const char* variable,const int32_t value)
{
	confConvBuf[0] = '\0';
	sprintf((char*)confConvBuf,"%d",(int)value);
	return config_push(variable,confConvBuf);
}

ConfigEntry* config_pushF(const char* variable,const float value)
{
	confConvBuf[0] = '\0';
	sprintf((char*)confConvBuf,"%f",value);
	return config_push(variable,confConvBuf);
}

ConfigEntry* config_pushD(const char* variable,const double value)
{
	confConvBuf[0] = '\0';
	sprintf((char*)confConvBuf,"%f",value);
	return config_push(variable,confConvBuf);
}

ConfigEntry* config_pushHex(const char* variable,const uint32_t value)
{
	confConvBuf[0] = '\0';
	sprintf((char*)confConvBuf,"%x",(unsigned int)value);
	return config_push(variable,confConvBuf);
}

ConfigEntry* config_find(const char* variable)
{
    ConfigEntry* e = entry;

    while(e)
    {
        if( memcmp(e->variable,variable,e->variableLength) == 0 )
            return e;   

        e = e->next;
    }

    return NULL;
}

void config_remove(const char* variable)
{
    //a nice trick to avoid making a linked list
    ConfigEntry* e = entry;
    ConfigEntry* prev = NULL;

    while(e)
    {
        if( memcmp(e->variable,variable,e->variableLength) == 0 )
        {
            if(!prev)//Head!
                config_init();
            else
            {
                //make sure we didn't hit Tail
                prev->next = (e->next) ? e->next : NULL;

                if(e->variable)
                    free(e->variable);

                if(e->value)
                    free(e->value);

                free(e);
            }

            return;
        }
        else
            prev = e;

        e = e->next;
    }

    return;
}

ConfigEntry* config_replaceS(const char* variable,const char* newValue)
{
    ConfigEntry* e;

    e = config_find(variable);

    if(!e)
        return NULL;

    if(e->value)
        free(e->value);

    e->value = clone_string(newValue,&e->valueLength);

    return e;
}

ConfigEntry* config_replace(ConfigEntry* e,const char* newValue)
{
    if(e->value)
        free(e->value);

    e->value = clone_string(newValue,&e->valueLength);

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

char* config_getHex(const char* variable)
{
    ConfigEntry* e;

    e = config_find(variable);

    if(!e)
        return NULL;

    return e->value;
}

int32_t config_getI(const char* variable)
{
    register const char* str;
    const char* base;
    int32_t result = 0;
    int32_t neg = 0;
    ConfigEntry* e;

    e = config_find(variable);

    if(!e)
        return 0;

    str = base = e->value;

    while((*str) && (!is_space(*str)))
		++str;

	switch(*str)
	{
		case '0':
			return 0;

		case '-':
			++str;
			neg = 1;
		break;
	}

    while((*str) && ((str - base) < 10) && (is_num(*str)))
    {
        result = (result * 10) + (*str - '0');
        ++str;
    }

    if(neg)
        result *= -1;

    return result;
}

float config_getF(const char* variable)
{
    ConfigEntry* e = config_find(variable);

	if(!e)
		return 0.0f;
	else if(!e->value)
		return 0.0f;

	return (float)atof((char*)e->value);
}

double config_getD(const char* variable)
{
    ConfigEntry* e = config_find(variable);

	if(!e)
		return 0.0;
	else if(!e->value)
		return 0.0;

	return (double)atof((char*)e->value);
}

int32_t config_loadFromBuffer(const char* buf,const int32_t size)
{
    int32_t eof = 0;
    int32_t len = 0;
    char token[1024];
    ConfigEntry* obj = (ConfigEntry*)malloc(sizeof(ConfigEntry));

    if(!obj)
        return 1;

    //dummy object
    obj->variable = (char*)malloc(256);
    obj->value = (char*)malloc(512);

    if(!obj->variable || !obj->value)
        return 1;

   //config_init();

    while(*buf)
    {
        if(eof)
            break;

        buf += get_token(token,buf,size,0,&len,&eof);

        if(len)
        {
            memcpy(obj->variable,token,len);obj->variable[len] = '\0';

            if(*buf != '=')
            {
                //config_shutdown();
                break;
            }

            ++buf;

            buf += get_token(token,buf,size,1,&len,&eof);

            if(!len)
            {
                //config_shutdown();
                break;
            }

            memcpy(obj->value,token,len);obj->value[len] = '\0';

            config_push(obj->variable,obj->value);
        }
    }

    free(obj->variable);
    free(obj->value);
    free(obj);

    return 0;
}

int32_t config_saveToBuffer(unsigned char* buf)
{
    ConfigEntry* e = entry;
    unsigned char* p;

    if(!e)
        return 1;

    p = &buf[0];

    while(e)
    {
        memcpy(p,e->variable,e->variableLength); p += e->variableLength;
        *p = '=';  p += 1;
        memcpy(p,e->value,e->valueLength); p += e->valueLength;
        *p = '\r'; p += 1;//Windows style newline \r\n
        *p = '\n'; p += 1;

        e = e->next;
    }

    *p = '\0';

    return 0;
}

int32_t config_getEntryCount()
{
    int32_t res = 0;
    ConfigEntry* e = entry;

    while(e)
    {
        ++res;
        e = e->next;
    }

    return res;
}

uint32_t config_predictOutputBufferSize()
{
    ConfigEntry* e;
	uint32_t res;

	if(!entry)
		return 0;

	e = entry;
	res = 0;

	while(e)
	{
		if(e->variable)
			res += string_length(e->variable);

		if(e->value)
			res += string_length(e->value);

		//const '=\r\n'
		res += 3;

		e = e->next;
	}

	return res + ((res+3)&1) ? 4 : 3;
}

/*********************************************** PUBLIC END *********************************************************/



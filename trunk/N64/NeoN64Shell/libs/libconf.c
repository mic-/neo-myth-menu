#include "libconf.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

static ConfigEntry* entry = NULL;
static ConfigEntry* entries = NULL;

/*********************************************** PRIVATE BEGIN *********************************************************/
static int32_t string_length(register const int8_t* src)
{
    register const int8_t* pa = src;

    while(*pa != '\0'){++pa;}

    return (int32_t)(pa - src);
}

static int8_t* clone_string(const int8_t* src,int32_t* saveLength)
{
    int32_t len = string_length(src);

    if(saveLength)
        *saveLength = len;

    int8_t* res = (int8_t*)malloc( ( ((len + 1)&1) ? (len + 2) : (len + 1) ) );
        
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

inline int32_t isNumerical(int32_t c)
{
    c = c - '0';
    return ((c  >= 0) && (c <= 9));
}

static int32_t skip_whitespace(const int8_t* src)
{
    const int8_t* sp = src;

    while(is_space(*src))
        ++src;

    return src - sp;
}

static int32_t get_token(int8_t* dst,const int8_t* src,const int32_t srcSize,const int32_t allow_sp,int32_t* len,int32_t* eof)
{
    const int8_t* sp = src;

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
            if( is_space(*src) && (!isdigit(*src)) )
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

    entry = entries = NULL;
}

ConfigEntry* config_push(const int8_t* variable,const int8_t* value)
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

ConfigEntry* config_find(const int8_t* variable)
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

void config_remove(const int8_t* variable)
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

ConfigEntry* config_replaceS(const int8_t* variable,const int8_t* newValue)
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

ConfigEntry* config_replace(ConfigEntry* e,const int8_t* newValue)
{
    if(e->value)
        free(e->value);

    e->value = clone_string(newValue,&e->valueLength);

    return e;
}

int8_t* config_getS(const int8_t* variable)
{
    ConfigEntry* e;

    e = config_find(variable);

    if(!e)
        return NULL;

    return e->value;
}

int32_t config_getI(const int8_t* variable)
{
    const int8_t* str;
    const int8_t* base;
    int32_t result = 0;
    int32_t neg = 0;
    ConfigEntry* e;

    e = config_find(variable);

    if(!e)
        return 0;

    str = base = e->value;

    while(*str)
    {
        if(!is_space(*str))
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

int32_t config_loadFromBuffer(const int8_t* buf,const int32_t size)
{
    const int8_t* pa = buf;
    int32_t eof = 0;
    int32_t len = 0;
    int8_t token[1024];
    ConfigEntry* obj = (ConfigEntry*)malloc(sizeof(ConfigEntry));

    if(!obj)
        return 1;

    //dummy object
    obj->variable = (int8_t*)malloc(256);
    obj->value = (int8_t*)malloc(512);

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

int32_t config_saveToBuffer(int8_t* buf)
{
    ConfigEntry* e = entry;
    int8_t* p;

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

/*********************************************** PUBLIC END *********************************************************/



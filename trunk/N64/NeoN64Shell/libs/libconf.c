#include "libconf.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

static ConfigEntry* entry = (void*)0;
static ConfigEntry* entries = (void*)0;

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
    ConfigEntry* next = ((void*)0);

    if(!entry)
    {
        entry = (ConfigEntry*)malloc(sizeof(ConfigEntry));
        entry->next = ((void*)0);
        entries = entry;
        return entry;
    }

    next = entries->next;
    next = (ConfigEntry*)malloc(sizeof(ConfigEntry));
    next->next = ((void*)0);
    entries = next;

    return entries;
}

static ConfigEntry* __config_push(const int8_t* variable,const int8_t* value,const int32_t varLen,const int32_t valLen)
{
    ConfigEntry* e = push_obj();

    if(!e)
        return NULL;

    e->variable = clone_string(variable,NULL);
    e->variableLength = varLen;

    e->value = clone_string(value,NULL);
    e->valueLength = valLen;

    return e;
}

inline int32_t isSpace(int32_t c)
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
    return ((c >= 0) && (c <= 9));
}

/*********************************************** PRIVATE END *********************************************************/

/*********************************************** PUBLIC BEGIN *********************************************************/
void config_init()
{
    config_shutdown();
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

    if(variable[0] == '\0')
        return NULL;

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
    ConfigEntry* zombie;

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

    return NULL;
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

int32_t config_loadFromBuffer(const int8_t* buf,const int32_t size)
{
    const int8_t* pa = buf;
    int32_t ind = 0;
    ConfigEntry* obj = (ConfigEntry*)malloc(sizeof(ConfigEntry));

    if(!obj)
        return 1;

    //dummy object
    obj->variable = (int8_t*)malloc(256);
    obj->value = (int8_t*)malloc(512);

    if(!obj->variable || !obj->value)
        return 1;

    config_init();

    while(*pa)
    {
        {
            //reset pointer
            ind = 0;

            obj->variable[0] = obj->value[0] = '\0';
            obj->variableLength = obj->valueLength = 0;

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

                    obj->variable[ind++] = *pa;
                }

                ++pa;
            }

            obj->variable[ind] = '\0';
            obj->variableLength = ind;
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

                        obj->value[ind++] = *pa;
                    }

                    ++pa;
                }

                obj->value[ind] = '\0';
                obj->valueLength = ind;

                if(obj->variableLength)
                {
                    //printf("Adding [%s] with data [%s]\n",obj->variable,obj->value);
                    __config_push(obj->variable,obj->value,obj->variableLength,obj->valueLength);
                }
            }
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



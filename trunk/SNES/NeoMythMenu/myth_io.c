// #########################################################################

#include "basetypes.h"
#include "u_strings.h"
//#include <u_memory.h>
#include <string.h>

// #########################################################################

typedef struct
{
    int pos;
    BYTE mode;
} m_file;

// #########################################################################

#define myth_io_implementation
#include "myth_io.h"

// #########################################################################

#define mode_neo_menu    0x01
#define mode_neo_flash   0x02
#define mode_neo_sram    0x03
#define mode_neo_psram   0x04
#define mode_neo_cache   0x05
#define mode_neo_n64menu 0x06
#define mode_neo_n64sram 0x07
#define mode_neo_sd      0x08
#define mode_mask        0x0f
#define mode_read        0x10
#define mode_write       0x20
#define mode_raw         0x30

// #########################################################################

//"r" open read-only
//"w" open write-only, erase existing file
//"rw", "wr" open read-write, preserve existing file

int _fmcmp(const WCHAR*str0,const WCHAR*str1,int*sl)
{
    int res=wstrlen(str1);
    if(memcmp(str0,str1,res*sizeof(WCHAR))==0)
    {
        sl[0]=res;
        return 1;
    }
    return 0;
}

int m_fopen(const WCHAR*filename,const char*rwmode,m_file*stream)
{
    int sl=0,mode=0;
    const WCHAR*fpath=filename;
    memset(stream,0,sizeof(stream));
    if(_fmcmp(filename,L"menu:",&sl))mode=mode_neo_menu;
    else if(_fmcmp(filename,L"flash:",&sl))mode=mode_neo_flash;
    else if(_fmcmp(filename,L"sram:",&sl))mode=mode_neo_sram;
    else if(_fmcmp(filename,L"psram:",&sl))mode=mode_neo_psram;
    else if(_fmcmp(filename,L"cache:",&sl))mode=mode_neo_cache;
    else if(_fmcmp(filename,L"n64menu:",&sl))mode=mode_neo_n64menu;
    else if(_fmcmp(filename,L"n64sram:",&sl))mode=mode_neo_n64sram;
    else if(_fmcmp(filename,L"sd:",&sl))mode=mode_neo_sd;
    else if(rwmode[0]=='r'||rwmode[1]=='r')mode|=mode_read;
    else if(rwmode[0]=='w'||rwmode[1]=='w')mode|=mode_write;
    else if((mode&(~mode_mask))==0)return -1;
    if(sl)
    {
        fpath=filename+sl;
        if(fpath[0]==0)mode|=mode_raw;
    }
    stream->mode=mode;
    switch(mode&mode_mask)
    {
    case mode_neo_menu:
        if(mode&mode_raw)return 1;
        break;
    case mode_neo_flash:
        if(mode&mode_raw)return 1;
        break;
    case mode_neo_sram:
        if(mode&mode_raw)return 1;
        break;
    case mode_neo_psram:
        if(mode&mode_raw)return 1;
        break;
    case mode_neo_cache:
        return -1;
        break;
    case mode_neo_n64menu:
        return -1;
        break;
    case mode_neo_n64sram:
        return -1;
        break;
    case mode_neo_sd:
        if(mode&mode_raw)return -1;
        break;
    default:
        break;
    };
    return -1;
}

// #########################################################################

void m_fclose(m_file*stream)
{
    stream->mode=0;
}

// #########################################################################

int m_ftell(m_file*stream)
{
    return stream->pos;
}

// #########################################################################

int m_fseek(m_file*stream,int offset,int origin)
{
    if(origin==SEEK_SET)
    {
        stream->pos=offset;
        return stream->pos;
    }
    else if(origin==SEEK_CUR)
    {
        stream->pos+=offset;
        return stream->pos;
    }
    else if(origin==SEEK_END)
    {
    }
    return -1;
}

// #########################################################################

int m_feof(m_file*stream)
{
    return -1;
}

// #########################################################################

int m_fwrite(m_file*stream,const void*data,int size)
{
    return -1;
}

// #########################################################################

int m_fread(m_file*stream,const void*data,int size)
{
    return -1;
}

// #########################################################################

int m_fenum(m_file*stream,int index,WCHAR*filename)
{
    return -1;
}

// #########################################################################

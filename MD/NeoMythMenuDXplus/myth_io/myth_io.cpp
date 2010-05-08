// #########################################################################

#define mode_neo_menu    0x01
#define mode_neo_flash   0x02
#define mode_neo_sram    0x04
#define mode_neo_psram   0x08
#define mode_neo_cache   0x10
#define mode_neo_n64menu 0x20
#define mode_neo_n64sram 0x40

// #########################################################################

typedef struct
{
    int pos;
    BYTE mode;
} m_file;

// #########################################################################

#define myth_io_implementation
#include <myth_io.h>

// #########################################################################

//"r" open read-only
//"w" open write-only, erase existing file
//"rw", "wr" open read-write, preserve existing file

int m_fopen(const WCHAR*filename,const char*mode,m_file*stream)
{
}

// #########################################################################

void m_fclose(m_file*stream)
{
}

// #########################################################################

int m_ftell(m_file*stream)
{
}

// #########################################################################

int m_fseek(m_file*stream,int offset,int origin)
{
}

// #########################################################################

int m_feof(m_file*stream)
{
}

// #########################################################################

int m_fwrite(m_file*stream,const void*data,int size)
{
}

// #########################################################################

int m_fread(m_file*stream,const void*data,int size)
{
}

// #########################################################################

int m_fenum(m_file*stream,int index,WCHAR*filename)
{
}

// #########################################################################

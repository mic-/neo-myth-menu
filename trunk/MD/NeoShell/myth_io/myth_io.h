// #########################################################################

#include <basetypes.h>

// #########################################################################

#define mtype_dir  0x01
#define mtype_file 0x02

// #########################################################################

#define SEEK_SET 0x01
#define SEEK_CUR 0x02
#define SEEK_END 0x04

// #########################################################################

#ifndef myth_io_implementation
/*
typedef struct
{
    BYTE dummy[8];
} m_file;
*/
#endif

// #########################################################################

/*
return values:

<0 - error
>0 - success
0  - function dependent
*/

// #########################################################################

//"r" open read-only
//"w" open write-only, erase existing file
//"rw", "wr" open read-write, preserve existing file

/*

"menu:"
"flash:"
"sram:"
"psram:"
"cache:"
"n64menu:"
"n64sram:"
"sd:"

examples:

m_file file;

fopen(L"flash:/Aladdin (USA)"),"r",&file); // opens rom for reading

fopen(L"flash:/"),"r",&file); // opens flash root directory for enumeration

fopen(L"flash:"),"r",&file);  // the flash for reading, discouraged

*/

int m_fopen(const WCHAR*filename,const char*mode,m_file*stream);
int m_getftype(m_file*stream);
void m_fclose(m_file*stream);

// #########################################################################

int m_ftell(m_file*stream);
int m_fseek(m_file*stream,int offset,int origin);
int m_feof(m_file*stream);

// #########################################################################

int m_fread(m_file*stream,const void*data,int size);
int m_fwrite(m_file*stream,const void*data,int size);

// #########################################################################

int m_fenum(m_file*stream,int index,WCHAR*filename);

// #########################################################################

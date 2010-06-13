/* ioapi.h -- IO base function header for compress/uncompress .zip
   part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )

         Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

         Modifications for Zip64 support
         Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )

         For more info read MiniZip_info.txt

*/

#if (defined(_WIN32))
        #define _CRT_SECURE_NO_WARNINGS
#endif

#include "ioapi.h"
#include <ff.h>

extern void ints_on();

#ifndef ALLOC
#include <u_memory.h>
# define ALLOC(size) (mm_alloc(size))
#endif
#ifndef TRYFREE
# define TRYFREE(p) {mm_free(p);}
#endif

voidpf call_zopen64 (const zlib_filefunc64_32_def* pfilefunc,const void*filename,int mode)
{
    if (pfilefunc->zfile_func64.zopen64_file != NULL)
        return (*(pfilefunc->zfile_func64.zopen64_file)) (pfilefunc->zfile_func64.opaque,filename,mode);
    else
    {
        return (*(pfilefunc->zopen32_file))(pfilefunc->zfile_func64.opaque,(const char*)filename,mode);
    }
}

long call_zseek64 (const zlib_filefunc64_32_def* pfilefunc,voidpf filestream, ZPOS64_T offset, int origin)
{
    if (pfilefunc->zfile_func64.zseek64_file != NULL)
        return (*(pfilefunc->zfile_func64.zseek64_file)) (pfilefunc->zfile_func64.opaque,filestream,offset,origin);
    else
    {
        uLong offsetTruncated = (uLong)offset;
        if (offsetTruncated != offset)
            return -1;
        else
            return (*(pfilefunc->zseek32_file))(pfilefunc->zfile_func64.opaque,filestream,offsetTruncated,origin);
    }
}

ZPOS64_T call_ztell64 (const zlib_filefunc64_32_def* pfilefunc,voidpf filestream)
{
    if (pfilefunc->zfile_func64.zseek64_file != NULL)
        return (*(pfilefunc->zfile_func64.ztell64_file)) (pfilefunc->zfile_func64.opaque,filestream);
    else
    {
        uLong tell_uLong = (*(pfilefunc->ztell32_file))(pfilefunc->zfile_func64.opaque,filestream);
        if ((tell_uLong) == ((uLong)-1))
            return (ZPOS64_T)-1;
        else
            return tell_uLong;
    }
}

void fill_zlib_filefunc64_32_def_from_filefunc32(zlib_filefunc64_32_def* p_filefunc64_32,const zlib_filefunc_def* p_filefunc32)
{
    p_filefunc64_32->zfile_func64.zopen64_file = NULL;
    p_filefunc64_32->zopen32_file = p_filefunc32->zopen_file;
    p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
    p_filefunc64_32->zfile_func64.zread_file = p_filefunc32->zread_file;
    p_filefunc64_32->zfile_func64.zwrite_file = p_filefunc32->zwrite_file;
    p_filefunc64_32->zfile_func64.ztell64_file = NULL;
    p_filefunc64_32->zfile_func64.zseek64_file = NULL;
    p_filefunc64_32->zfile_func64.zclose_file = p_filefunc32->zclose_file;
    p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
    p_filefunc64_32->zfile_func64.opaque = p_filefunc32->opaque;
    p_filefunc64_32->zseek32_file = p_filefunc32->zseek_file;
    p_filefunc64_32->ztell32_file = p_filefunc32->ztell_file;
}



static voidpf  ZCALLBACK fopen_file_func OF((voidpf opaque, const char* filename, int mode));
static uLong   ZCALLBACK fread_file_func OF((voidpf opaque, voidpf stream, void* buf, uLong size));
static uLong   ZCALLBACK fwrite_file_func OF((voidpf opaque, voidpf stream, const void* buf,uLong size));
static ZPOS64_T ZCALLBACK ftell64_file_func OF((voidpf opaque, voidpf stream));
static long    ZCALLBACK fseek64_file_func OF((voidpf opaque, voidpf stream, ZPOS64_T offset, int origin));
static int     ZCALLBACK fclose_file_func OF((voidpf opaque, voidpf stream));
static int     ZCALLBACK ferror_file_func OF((voidpf opaque, voidpf stream));

static voidpf ZCALLBACK fopen_file_func (voidpf opaque, const char* filename, int mode)
{
    FIL*file=(FIL*)ALLOC(sizeof(FIL));
    unsigned char mode_fopen = 0;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
        mode_fopen = FA_OPEN_EXISTING | FA_READ;
    else
    if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
        mode_fopen = FA_OPEN_EXISTING | FA_WRITE | FA_READ;
    else
    if (mode & ZLIB_FILEFUNC_MODE_CREATE)
        mode_fopen = FA_CREATE_ALWAYS | FA_WRITE | FA_READ;;

    ints_on();
    if (filename!=NULL)
        if(f_open(file, (XCHAR*)filename, mode_fopen) != FR_OK)
        {
            TRYFREE(file);
            file=0;
        }
    return file;
}

static voidpf ZCALLBACK fopen64_file_func (voidpf opaque, const void* filename, int mode)
{
    return fopen_file_func(opaque,filename,mode);
}


static uLong ZCALLBACK fread_file_func (voidpf opaque, voidpf stream, void* buf, uLong size)
{
    UINT ret;
    ints_on();
    f_read((FIL*)stream,buf,size,&ret);
    return ret;
}

static uLong ZCALLBACK fwrite_file_func (voidpf opaque, voidpf stream, const void* buf, uLong size)
{
    UINT ret;
    ints_on();
    f_write((FIL*)stream,buf,size,&ret);
    return ret;
}

static long ZCALLBACK ftell_file_func (voidpf opaque, voidpf stream)
{
    long ret;
    ret = ((FIL*)stream)->fptr;
    return ret;
}


static ZPOS64_T ZCALLBACK ftell64_file_func (voidpf opaque, voidpf stream)
{
    return ftell_file_func(opaque,stream);
}

static long ZCALLBACK fseek_file_func (voidpf  opaque, voidpf stream, uLong offset, int origin)
{
    long ret;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        offset+=((FIL*)stream)->fptr;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        if(offset>((FIL*)stream)->fsize)
        {
            return -1;
        }
        offset=((FIL*)stream)->fsize-offset;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        break;
    default: return -1;
    }
    ret = 0;
    ints_on();
    if (f_lseek((FIL*)stream, offset) != FR_OK)
        ret = -1;
    return ret;
}

static long ZCALLBACK fseek64_file_func (voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
{
    return fseek_file_func (opaque, stream, offset, origin);
}


static int ZCALLBACK fclose_file_func (voidpf opaque, voidpf stream)
{
    ints_on();
    f_close((FIL*)stream);
    TRYFREE(stream);
    return 0;
}

static int ZCALLBACK ferror_file_func (voidpf opaque, voidpf stream)
{
    return (((FIL*)stream)->flag&FA__ERROR)!=0;
}

void fill_fopen_filefunc (pzlib_filefunc_def)
  zlib_filefunc_def* pzlib_filefunc_def;
{
    pzlib_filefunc_def->zopen_file = fopen_file_func;
    pzlib_filefunc_def->zread_file = fread_file_func;
    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
    pzlib_filefunc_def->ztell_file = ftell_file_func;
    pzlib_filefunc_def->zseek_file = fseek_file_func;
    pzlib_filefunc_def->zclose_file = fclose_file_func;
    pzlib_filefunc_def->zerror_file = ferror_file_func;
    pzlib_filefunc_def->opaque = NULL;
}

void fill_fopen64_filefunc (zlib_filefunc64_def*  pzlib_filefunc_def)
{
    pzlib_filefunc_def->zopen64_file = fopen64_file_func;
    pzlib_filefunc_def->zread_file = fread_file_func;
    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
    pzlib_filefunc_def->ztell64_file = ftell64_file_func;
    pzlib_filefunc_def->zseek64_file = fseek64_file_func;
    pzlib_filefunc_def->zclose_file = fclose_file_func;
    pzlib_filefunc_def->zerror_file = ferror_file_func;
    pzlib_filefunc_def->opaque = NULL;
}

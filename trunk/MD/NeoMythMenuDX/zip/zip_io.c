#include <ff.h>
#include <unzip.h>
#include <zip_io.h>
#include <u_memory.h>
#include <util_68k_interface.h>

static unzFile uf=0;
static FIL*sdf=0;

FRESULT f_open_zip(FIL*file,const XCHAR*filename,BYTE mode)
{
    int L;
    unz_file_info file_info;
#if 0
    L=utility_wstrlen(filename);
    if(utility_wstrcmp(filename+L-4,L".zip")==0)
    {
        f_close_zip(0);
        uf=unzOpen((char*)filename);
        if(uf)
        {
            if(unzGoToFirstFile(uf)==UNZ_OK)
            {
                if(unzGoToNextFile(uf)==UNZ_END_OF_LIST_OF_FILE)
                {
                    if(unzGetCurrentFileInfo(uf,&file_info,0,0,0,0,0,0)==UNZ_OK)
                    {
                        file->fsize=file_info.uncompressed_size;
                        file->fptr=0;
                        if(unzOpenCurrentFile(uf)==UNZ_OK)
                        {
                            sdf=file;
                            return FR_OK;
                        }
                        unzCloseCurrentFile(uf);
                    }
                }
            }
            unzClose(uf);
            uf=0;
        }
    }
#endif
    memset(file,0,sizeof(FIL));
    return f_open(file,filename,mode);
}

FRESULT f_read_zip(FIL*file,void*buffer,UINT size,UINT*read)
{
    UINT rd;
    if(uf&&sdf&&file==sdf)
    {
        rd=unzReadCurrentFile(uf,buffer,size);
        if(read)*read=rd;
        return (rd==size)?FR_OK:FR_INT_ERR;
    }
    return f_read(file,buffer,size,read);
}

FRESULT f_lseek_zip(FIL*file,DWORD offset)
{
    if(uf&&sdf&&file==sdf)
    {
        if(unzSetOffset(uf,offset)==UNZ_OK)
        {
            return FR_OK;
        }
        return FR_INT_ERR;
    }
    return f_lseek(file,offset);
}

FRESULT f_close_zip(FIL*file)
{
    if(uf&&sdf&&file==sdf)
    {
        unzCloseCurrentFile(uf);
        unzClose(uf);
        uf=0;sdf=0;
        mm_init(); /*ugly hack - free all used memory*/
    }
    if(file)
    {
        return f_close(file);
    }
    return FR_OK;
}

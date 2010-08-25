#include "libio.h"
#include "libio_ext.h"

#if ( !defined(IO_TARGET_PLATFORM_MD32X) && !defined(IO_TARGET_PLATFORM_SNES))
    #include <malloc.h>
#endif

#if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
    static uint8_t ioGenericBuf[1024];
    static uint32_t ioGenericBufSize = 1024;
    static int16_t ioDirPtr = 0;
    static int16_t ioFilePtr = 0;

    typedef struct File File;
    typedef struct Directory Directory;

    struct File
    {
        IO_Handle handle;
        #ifndef IO_TARGET_PLATFORM_SNES
            uint32_t addr;
        #else
            uint8_t bank;
            uint8_t _pad;
            uint16_t addr;
        #endif
        int16_t alive;
    };

    struct Directory
    {
        IO_DirHandle handle;
        #ifndef IO_TARGET_PLATFORM_SNES
            uint32_t addr;
        #else
            uint8_t bank;
            uint8_t _pad;
            uint16_t addr;
        #endif
        int16_t alive;
    };

    static File ioFile[MAX_FILE_HANDLES];
    static Directory ioDir[MAX_DIR_HANDLES];

    static int16_t _____io_internal_get_next_file_handle()
    {
        int16_t addr = 0;

        while(addr < MAX_FILE_HANDLES)
        {
            if(ioFile[addr].alive)
            {
                #ifndef IO_TARGET_PLATFORM_SNES
                    ioFile[addr].addr = ((uint32_t)&ioFile[addr].handle);
                #else
                    ioFile[addr].bank = (uint8_t)((uint32_t)&ioFile[addr].handle & 0xff000000);
                    ioFile[addr].addr = (uint16_t)((uint32_t)&ioFile[addr].handle & 0x00ffff00);
                #endif
                ioFile[addr].alive = 0x0;

                return addr;
            }

            ++addr;
        }

        return 0xFFFE;
    }

    static int16_t _____io_internal_get_next_dir_handle()
    {
        int16_t addr = 0;

        while(addr < MAX_DIR_HANDLES)
        {
            if(ioDir[addr].alive)
            {
                #ifndef IO_TARGET_PLATFORM_SNES
                    ioDir[addr].addr = ((uint32_t)&ioDir[addr].handle);
                #else
                    ioDir[addr].bank = (uint8_t)((uint32_t)&ioDir[addr].handle & 0xff000000);
                    ioDir[addr].addr = (uint16_t)((uint32_t)&ioDir[addr].handle & 0x00ffff00);
                #endif
                ioDir[addr].alive = 0x0;

                return addr;
            }

            ++addr;
        }

        return 0xFFFE;
    }

    static void _____io_internal_mark_free(uint16_t isFile,uint32_t addr)
    {
        int16_t ptr = 0;
        #ifdef IO_TARGET_PLATFORM_SNES
            uint8_t bbank = (uint8_t)(addr & 0xff000000);
            uint16_t boffs = (uint16_t)(addr & 0x00ffff00);
        #endif

        if(isFile != 0x00)
        {
            while(ptr < MAX_FILE_HANDLES)
            {
                #ifndef IO_TARGET_PLATFORM_SNES
                    if(ioFile[ptr].addr == addr)
                    {
                        ioFile[ptr].alive = 0x1;
                        return;
                    }
                #else
                    if(ioFile[ptr].bank == bbank && ioFile[ptr].addr == boffs)
                    {
                        ioFile[ptr].alive = 0x1;
                        return;
                    }
                #endif

                ++ptr;
            }

            return;
        }
        else
        {
            while(ptr < MAX_DIR_HANDLES)
            {
                #ifndef IO_TARGET_PLATFORM_SNES
                    if(ioDir[ptr].addr == addr)
                    {
                        ioDir[ptr].alive = 0x1;
                        return;
                    }
                #else
                    if(ioDir[ptr].bank == bbank && ioDir[ptr].addr == boffs)
                    {
                        ioDir[ptr].alive = 0x1;
                        return;
                    }
                #endif

                ++ptr;
            }
        }
    }
#else
    static uint8_t* ioGenericBuf = ((void*)0);
    static uint32_t ioGenericBufSize = 0;
#endif

static void _____io_internal_c2wstrcpy(XCHAR* dst,const int8_t* src)
{
    #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
        register int16_t ix = 0;
        register XCHAR* a;
        register const int8_t* b;
        do
        {
            a = dst + (ix << 1);
            b = (src + (ix++));
        }while( (( *a = ((*b) & 0x00FF) )!= (XCHAR)0x0000) );
    #else
        int32_t ix = 0;

        while(1)
        {
            if( (*(XCHAR *)(dst + (ix << 1) ) = *(int8_t*)(src + ix) & 0x00FF) == (XCHAR)0)
                break;

            ++ix;
        }
    #endif
}

void io_init(uint32_t genericBufSize)
{
    #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
        ioFilePtr = MAX_FILE_HANDLES - 1;
        ioDirPtr = MAX_DIR_HANDLES - 1;

        while(ioFilePtr)
            ioFile[ioFilePtr--].alive = 0x1;

        while(ioDirPtr)
            ioDir[ioDirPtr--].alive = 0x1;

        ioDirPtr = ioFilePtr = 0;
    #else
        ioGenericBuf = (uint8_t*)malloc(genericBufSize);
        ioGenericBufSize = (ioGenericBuf) ? genericBufSize : 0;
    #endif
}

void io_shutdown()
{
    #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
        io_init(0);
    #else
        if(ioGenericBuf != ((void*)0))
            free(ioGenericBuf);

        ioGenericBufSize = 0;
        ioGenericBuf = ((void*)0);
    #endif
}

uint32_t io_seek(IO_Handle* f,uint32_t addr,uint32_t mode)
{
    switch(f->type)
    {
        case IO_DEVICE_SD:
        {
            switch(mode)
            {
                case IO_SEEK_SET:
                    ENABLE_INTERRUPTS;
                    f_lseek(&f->handle,addr);
                    DISABLE_INTERRUPTS;
                return f->handle.fptr;

                case IO_SEEK_END:
                    ENABLE_INTERRUPTS;
                    f_lseek(&f->handle,f->handle.fsize);
                    DISABLE_INTERRUPTS;
                return f->handle.fptr;

                case IO_SEEK_CUR:
                    ENABLE_INTERRUPTS;
                    f_lseek(&f->handle,(((f->handle.fptr + addr) > f->handle.fsize) ? f->handle.fsize : f->handle.fptr + addr));
                    DISABLE_INTERRUPTS;
                return f->handle.fptr;
            }
            
            return 0;
        }

        case IO_DEVICE_SRAM:
        {
            switch(mode)
            {
                case IO_SEEK_SET:
                    f->address = (addr > SRAM_SIZE) ? SRAM_SIZE : addr;
                return f->address;

                case IO_SEEK_END:
                    f->address = SRAM_SIZE;
                return f->address;

                case IO_SEEK_CUR:
                    f->address = ((f->address + addr) > SRAM_SIZE) ? SRAM_SIZE : (f->address + addr);
                return f->address;
            }
            
            return 0;
        }

        case IO_DEVICE_EEPROM:
        {
            switch(mode)
            {
                case IO_SEEK_SET:
                    f->address = (addr > EEPROM_SIZE) ? EEPROM_SIZE : addr;
                return f->address;

                case IO_SEEK_END:
                    f->address = EEPROM_SIZE;
                return f->address;

                case IO_SEEK_CUR:
                    f->address = ((f->address + addr) > EEPROM_SIZE) ? EEPROM_SIZE : (f->address + addr);
                return f->address;
            }
            
            return 0;
        }

        case IO_DEVICE_GBA_SRAM:
        {
            switch(mode)
            {
                case IO_SEEK_SET:
                    f->address = (addr > GBA_SRAM_SIZE) ? GBA_SRAM_SIZE : addr;
                return f->address;

                case IO_SEEK_END:
                    f->address = GBA_SRAM_SIZE;
                return f->address;

                case IO_SEEK_CUR:
                    f->address = ((f->address + addr) > GBA_SRAM_SIZE) ? GBA_SRAM_SIZE : (f->address + addr);
                return f->address;
            }
            
            return 0;
        }

        case IO_DEVICE_MYTH_PSRAM:
        {
            switch(mode)
            {
                case IO_SEEK_SET:
                    f->address = (addr > MYTH_PSRAM_SIZE) ? MYTH_PSRAM_SIZE : addr;
                return f->address;

                case IO_SEEK_END:
                    f->address = MYTH_PSRAM_SIZE;
                return f->address;

                case IO_SEEK_CUR:
                    f->address = ((f->address + addr) > MYTH_PSRAM_SIZE) ? MYTH_PSRAM_SIZE : (f->address + addr);
                return f->address;
            }
            
            return 0;
        }

        case IO_DEVICE_PSRAM:
        {
            switch(mode)
            {
                case IO_SEEK_SET:
                    f->address = (addr > PSRAM_SIZE) ? PSRAM_SIZE : addr;
                return f->address;

                case IO_SEEK_END:
                    f->address = PSRAM_SIZE;
                return f->address;

                case IO_SEEK_CUR:
                    f->address = ((f->address + addr) > PSRAM_SIZE) ? PSRAM_SIZE : (f->address + addr);
                return f->address;
            }
            
            return 0;
        }
    }

    return 0;
}

uint32_t io_tell(IO_Handle* f)
{
    switch(f->type)
    {
        case IO_DEVICE_SD:
            return f->handle.fptr;

        case IO_DEVICE_SRAM:
        case IO_DEVICE_EEPROM:
        case IO_DEVICE_GBA_SRAM:
        case IO_DEVICE_MYTH_PSRAM:
        case IO_DEVICE_PSRAM:
            return f->address;
    }

    return 0;
}

uint32_t io_size(IO_Handle* f)
{
    switch(f->type)
    {
        case IO_DEVICE_SD:
            return f->handle.fsize;

        case IO_DEVICE_SRAM:
            return SRAM_SIZE;

        case IO_DEVICE_EEPROM:
            return EEPROM_SIZE;

        case IO_DEVICE_GBA_SRAM:
            return GBA_SRAM_SIZE;

        case IO_DEVICE_MYTH_PSRAM:
            return MYTH_PSRAM_SIZE;

        case IO_DEVICE_PSRAM:
            return PSRAM_SIZE;
    }

    return 0;
}

uint32_t io_write(const void* ptr,uint32_t size,IO_Handle* f)
{
    UINT r = 0;

    switch(f->type)
    {
        case IO_DEVICE_SD:
            ENABLE_INTERRUPTS;
            f_write(&f->handle,ptr,size,&r);
            DISABLE_INTERRUPTS;
        return r;

        case IO_DEVICE_SRAM:
            return nio_sram_write(ptr,size,f->address);

        case IO_DEVICE_EEPROM:
            return nio_eeprom_write(ptr,size,f->address);

        case IO_DEVICE_GBA_SRAM:
            return nio_gba_sram_write(ptr,size,f->address);

        case IO_DEVICE_MYTH_PSRAM:
            return nio_myth_psram_write(ptr,size,f->address);

        case IO_DEVICE_PSRAM:
            return nio_psram_write(ptr,size,f->address);
    }

    return 0;
}

uint32_t io_read(void* ptr,uint32_t size,IO_Handle* f)
{
    UINT r = 0;

    switch(f->type)
    {
        case IO_DEVICE_SD:
            ENABLE_INTERRUPTS;
            f_read(&f->handle,ptr,size,&r);
            DISABLE_INTERRUPTS;
        return r;

        case IO_DEVICE_SRAM:
            return nio_sram_read(ptr,size,f->address);

        case IO_DEVICE_EEPROM:
            return nio_eeprom_read(ptr,size,f->address);

        case IO_DEVICE_GBA_SRAM:
            return nio_gba_sram_read(ptr,size,f->address);

        case IO_DEVICE_MYTH_PSRAM:
            return nio_myth_psram_read(ptr,size,f->address);

        case IO_DEVICE_PSRAM:
            return nio_psram_read(ptr,size,f->address);
    }

    return 0;
}

uint32_t io_puts(const int8_t* str,IO_Handle* f)
{
    UINT r = 0;

    switch(f->type)
    {
        case IO_DEVICE_SD:
            ENABLE_INTERRUPTS;
            f_write(&f->handle,(const void*)str,(uint32_t)strlen((char*)str),&r);
            DISABLE_INTERRUPTS;
        return r;

        case IO_DEVICE_SRAM:
            return nio_sram_write((const void*)str,(uint32_t)strlen((char*)str),f->address);

        case IO_DEVICE_EEPROM:
            return nio_eeprom_write((const void*)str,(uint32_t)strlen((char*)str),f->address);

        case IO_DEVICE_GBA_SRAM:
            return nio_gba_sram_write((const void*)str,(uint32_t)strlen((char*)str),f->address);

        case IO_DEVICE_MYTH_PSRAM:
            return nio_myth_psram_write((const void*)str,(uint32_t)strlen((char*)str),f->address);

        case IO_DEVICE_PSRAM:
            return nio_psram_write((const void*)str,(uint32_t)strlen((char*)str),f->address);
    }

    return 0;
}

uint32_t io_printf(IO_Handle* f,const int8_t* fmt,...)
{
    int8_t* buf = (int8_t*)&ioGenericBuf[0];
    UINT r = 0;

    buf[0] = 0;
    va_list ap;
    va_start(ap,(const char*)fmt);
    vsprintf((char*)buf,(const char*)fmt,ap);
    va_end(ap);

    switch(f->type)
    {
        case IO_DEVICE_SD:
            ENABLE_INTERRUPTS;
            f_write(&f->handle,(const void*)buf,strlen((char*)buf),&r);
            DISABLE_INTERRUPTS;
        return r;

        case IO_DEVICE_SRAM:
            return nio_sram_write((const void*)buf,strlen((char*)buf),f->address);

        case IO_DEVICE_EEPROM:
            return nio_eeprom_write((const void*)buf,strlen((char*)buf),f->address);

        case IO_DEVICE_GBA_SRAM:
            return nio_gba_sram_write((const void*)buf,strlen((char*)buf),f->address);

        case IO_DEVICE_MYTH_PSRAM:
            return nio_myth_psram_write((const void*)buf,strlen((char*)buf),f->address);

        case IO_DEVICE_PSRAM:
            return nio_psram_write((const void*)buf,strlen((char*)buf),f->address);
    }

    return 0;
}

uint32_t io_putc(int32_t c,IO_Handle* f)
{
    UINT r = 0;

    c &= 0xff;

    switch(f->type)
    {
        case IO_DEVICE_SD:
            ENABLE_INTERRUPTS;
            f_write(&f->handle,(int8_t*)(&c),1,&r);
            DISABLE_INTERRUPTS;
        return r;

        case IO_DEVICE_SRAM:
            return nio_sram_write((const void*)(&c),1,f->address);

        case IO_DEVICE_EEPROM:
            return nio_eeprom_write((const void*)(&c),1,f->address);

        case IO_DEVICE_GBA_SRAM:
            return nio_gba_sram_write((const void*)(&c),1,f->address);

        case IO_DEVICE_MYTH_PSRAM:
            return nio_myth_psram_write((const void*)(&c),1,f->address);

        case IO_DEVICE_PSRAM:
            return nio_psram_write((const void*)(&c),1,f->address);
    }

    return 0;
}

int32_t io_getc(IO_Handle* f)
{
    UINT r = 0;
    int32_t c = 0;

    switch(f->type)
    {
        case IO_DEVICE_SD:
            ENABLE_INTERRUPTS;
            f_read(&f->handle,(int8_t*)(&c),1,&r);
            DISABLE_INTERRUPTS;
        return c & 0xff;

        case IO_DEVICE_SRAM:
            r = nio_sram_read((void*)(&c),1,f->address);
        return c & 0xff;

        case IO_DEVICE_EEPROM:
            r = nio_eeprom_read((void*)(&c),1,f->address);
        return c & 0xff;

        case IO_DEVICE_GBA_SRAM:
            r = nio_gba_sram_read((void*)(&c),1,f->address);
        return c & 0xff;

        case IO_DEVICE_MYTH_PSRAM:
            r = nio_myth_psram_read((void*)(&c),1,f->address);
        return c & 0xff;

        case IO_DEVICE_PSRAM:
            r = nio_psram_read((void*)(&c),1,f->address);
        return c & 0xff;
    }

    return 0;
}

int32_t io_eof(IO_Handle* f)
{
    switch(f->type)
    {
        case IO_DEVICE_SD:
            return f->handle.fptr >= f->handle.fsize;

        case IO_DEVICE_SRAM:
            return f->address >= SRAM_SIZE;

        case IO_DEVICE_EEPROM:
            return f->address >= EEPROM_SIZE;

        case IO_DEVICE_GBA_SRAM:
            return f->address >= GBA_SRAM_SIZE;

        case IO_DEVICE_MYTH_PSRAM:
            return f->address >= MYTH_PSRAM_SIZE;

        case IO_DEVICE_PSRAM:
            return f->address >= PSRAM_SIZE;
    }

    return 0;
}

int32_t io_error(IO_Handle* f)
{
    switch(f->type)
    {
        case IO_DEVICE_SD:
            return ((f->handle.flag & FA__ERROR) != 0);

        case IO_DEVICE_SRAM:
        case IO_DEVICE_EEPROM:
        case IO_DEVICE_GBA_SRAM:
        case IO_DEVICE_MYTH_PSRAM:
        case IO_DEVICE_PSRAM:
            return 0;
    }

    return 0;
}

int32_t io_truncate(IO_Handle* f)
{
    switch(f->type)
    {
        case IO_DEVICE_SD:
            return f_truncate(&f->handle);

        case IO_DEVICE_SRAM:
        case IO_DEVICE_EEPROM:
        case IO_DEVICE_GBA_SRAM:
        case IO_DEVICE_MYTH_PSRAM:
        case IO_DEVICE_PSRAM:
            return 0;
    }

    return 0;
}

void io_close(IO_Handle* f)
{
    if(!f)
        return;

    switch(f->type)
    {
        case IO_DEVICE_SD:
            f_close(&f->handle);
        break;

        case IO_DEVICE_SRAM:
        case IO_DEVICE_EEPROM:
        case IO_DEVICE_GBA_SRAM:
        case IO_DEVICE_MYTH_PSRAM:
        case IO_DEVICE_PSRAM:
            return;
    }
    
    #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
        _____io_internal_mark_free(1,(uint32_t)(f) & 0x7fffffff);
    #else
        free(f);
    #endif
}

void io_remove(const int8_t* filename)
{
    XCHAR* buf = (XCHAR*)&ioGenericBuf[0];

    _____io_internal_c2wstrcpy(buf,filename);
    f_unlink(buf);
}

#define TEST_BIT(__F__,__A__) ( ( (__F__) & (__A__)) != (0) )

IO_Handle* io_open(const int8_t* device,const int8_t* filename,const int8_t* mode)
{
    XCHAR* buf = (XCHAR*)&ioGenericBuf[0];
    IO_Handle* f = (void*)0;
    uint32_t flags = 0;
    uint32_t eof = 0 , fnew = 0;
    IOD_Type type;

    if(!device)
        return (void*)0;

    if(!mode)
        mode = (int8_t*)"w+\0";

    if(memcmp(device,"sd",2) == 0)
        type = IO_DEVICE_SD;
    else if(memcmp(device,"sram",4) == 0)
        type = IO_DEVICE_SRAM;
    else if(memcmp(device,"eeprom",6) == 0)
        type = IO_DEVICE_EEPROM;
    else if(memcmp(device,"psram",5) == 0)
        type = IO_DEVICE_PSRAM;
    else if(memcmp(device,"gbasram",7) == 0)
        type = IO_DEVICE_GBA_SRAM;
    else if(memcmp(device,"mythpsram",9) == 0)
    {
        #if defined(IO_TARGET_PLATFORM_N64)
            return ((void*)0);
        #endif
        type = IO_DEVICE_MYTH_PSRAM;
    }
    else
        return ((void*)0);

    #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
        int16_t h =  _____io_internal_get_next_file_handle();

        if(h == 0xFFFE)
            return ((void*)0);

        f = &ioFile[h].handle;
    #else
        f = (IO_Handle*)malloc(sizeof(IO_Handle));

        if(!f)
            return (void*)0;
    #endif

    {
        while(*mode)
        {
            switch(*mode)
            {
                case 'a':
                {
                    eof = 1;

                    if(*(mode+1) == '+')
                    {   
                        ++mode;

                        if(!TEST_BIT(flags,FA_WRITE))
                            flags |= FA_WRITE;

                        if(!TEST_BIT(flags,FA_READ))
                            flags |= FA_READ;
                    }

                    break;
                }

                case 'r':
                {
                    fnew -= (fnew > 0);

                    if(!TEST_BIT(flags,FA_OPEN_EXISTING))
                        flags |= FA_OPEN_EXISTING;

                    if(!(flags & FA_READ))
                        flags |= FA_READ;

                    if(*(mode+1) == '+')
                    {   
                        ++mode;

                        if(!TEST_BIT(flags,FA_WRITE))
                            flags |= FA_WRITE;
                    }

                    break;
                }

                case 'w':
                {
                    if(!TEST_BIT(flags,FA_CREATE_ALWAYS))
                        flags |= FA_CREATE_ALWAYS;

                    if(!TEST_BIT(flags,FA_WRITE))
                        flags |= FA_WRITE;

                    if(*(mode+1) == '+')
                    {   
                        ++fnew;
                        ++mode;

                        if(!TEST_BIT(flags,FA_READ))
                            flags |= FA_READ;

                        if(!TEST_BIT(flags,FA_CREATE_NEW))
                            flags |= FA_CREATE_NEW;
                    }

                    break;
                }
            }

            ++mode;
        }
    }

    if(type == IO_DEVICE_SD)
    {
        _____io_internal_c2wstrcpy(buf,filename);

        if(f_open(&f->handle,buf,flags) != FR_OK)
        {
            #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
                free(f);
            #endif

            return (void*)0;
        }

        if((eof) && (!fnew)) 
            f_lseek(&f->handle,f->handle.fsize);
    }
    else
    {
        f->address = 0;

        if((eof) && (!fnew))
            io_seek(f,0,IO_SEEK_END);
    }

    return f;
}

#undef TEST_BIT

/*Dir functions*/
IO_DirHandle* io_open_dir(const int8_t* dirpath)
{
    XCHAR* buf = (XCHAR*)&ioGenericBuf[0];
    IO_DirHandle* dp = (void*)0;

    #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
        int16_t h =  _____io_internal_get_next_dir_handle();

        if(h == 0xFFFE)
            return ((void*)0);

        dp = &ioDir[h].handle;
    #else
        dp = (IO_DirHandle*)malloc(sizeof(IO_DirHandle));

        if(!dp)
            return (void*)0;
    #endif

    _____io_internal_c2wstrcpy(buf,dirpath);

    if(f_opendir(dp,buf) != FR_OK)
    {
        #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
            free(dp);
        #endif

        return (void*)0;
    }

    return dp;
}

void io_close_dir(IO_DirHandle* dp)
{
    if(!dp)
        return;

    #if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
        _____io_internal_mark_free(0,(uint32_t)(dp) & 0x7fffffff);
    #else
        free(dp);
    #endif
}

void io_remove_dir(const int8_t* dirpath)
{
    io_remove(dirpath);
}

int32_t io_change_dir(const int8_t* dirpath)
{
    XCHAR* buf = (XCHAR*)&ioGenericBuf[0];
    _____io_internal_c2wstrcpy(buf,dirpath);

    return f_chdir(buf);
}

int32_t io_create_dir(const int8_t* dirpath)
{
    XCHAR* buf = (XCHAR*)&ioGenericBuf[0];
    _____io_internal_c2wstrcpy(buf,dirpath);

    return f_mkdir(buf);
}

int32_t io_read_dir(IO_DirHandle* dp,IO_HandleInfo* info)
{
    if(!dp || !info)
        return -1;

    return f_readdir(dp,info);
}


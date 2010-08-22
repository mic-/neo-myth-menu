#include "libio.h"
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
//A nice , portable , and optimized wrapper for libff

//XXX for MD/32X remember to also handle io_open/close

#ifndef IO_MD32X_BUILD
	static uint8_t* ioGenericBuf = ((void*)0);
	static uint32_t ioGenericBufSize = 0;
#else
	static uint8_t ioGenericBuf[1024];
	static uint32_t ioGenericBufSize = 1024;
#endif

static void _____io_internal_c2wstrcpy(XCHAR* dst,const int8_t* src)
{
    int32_t ix = 0;

    while (1)
    {
        if( (*(XCHAR *)(dst + (ix << 1) ) = *(int8_t*)(src + ix) & 0x00FF) == (XCHAR)0)
			break;

		++ix;
    }
}

void io_init(uint32_t genericBufSize)
{
	#ifndef IO_MD32X_BUILD
	ioGenericBuf = (uint8_t*)malloc(genericBufSize);
	ioGenericBufSize = (ioGenericBuf) ? genericBufSize : 0;
	#endif
}

void io_shutdown()
{
	#ifndef IO_MD32X_BUILD
	if(ioGenericBuf != ((void*)0))
		free(ioGenericBuf);

	ioGenericBufSize = 0;
	ioGenericBuf = ((void*)0);
	#endif
}

uint32_t io_seek(IO_Handle* f,uint32_t addr,uint32_t mode)
{
	#ifndef IO_FULL_OPT
	if(!f)
		return 0;
	#endif

	switch(mode)
	{
		case IO_SEEK_SET:
			ENABLE_INTERRUPTS;
			f_lseek(f,addr);
			DISABLE_INTERRUPTS;
		return f->fptr;

		case IO_SEEK_END:
			ENABLE_INTERRUPTS;
			f_lseek(f,f->fsize);
			DISABLE_INTERRUPTS;
		return f->fptr;

		case IO_SEEK_CUR:
			ENABLE_INTERRUPTS;
			addr = ((f->fptr + addr) > f->fsize) ? f->fsize : f->fptr + addr;
			f_lseek(f,addr);
			DISABLE_INTERRUPTS;
		return f->fptr;
	}

	return 0;
}

uint32_t io_tell(IO_Handle* f)
{
	#ifndef IO_FULL_OPT
		return (f) ? (uint32_t)f->fptr : 0;
	#else
		return f->fptr;
	#endif
}

uint32_t io_size(IO_Handle* f)
{
	#ifndef IO_FULL_OPT
		return (f) ? (uint32_t)f->fsize : 0;
	#else
		return f->fsize;
	#endif
}

uint32_t io_write(const void* ptr,uint32_t size,IO_Handle* f)
{
	UINT r = 0;

	ENABLE_INTERRUPTS;
	#ifndef IO_FULL_OPT
	if(f)
	{
	#endif
		f_write(f,ptr,size,&r);
	#ifndef IO_FULL_OPT
	}
	#endif

	DISABLE_INTERRUPTS;
	return r;
}

uint32_t io_read(void* ptr,uint32_t size,IO_Handle* f)
{
	UINT r = 0;

	ENABLE_INTERRUPTS;
	#ifndef IO_FULL_OPT
	if(f)
	{
	#endif
		f_read(f,ptr,size,&r);
	#ifndef IO_FULL_OPT
	}
	#endif
	
	DISABLE_INTERRUPTS;
	return r;
}

uint32_t io_puts(const int8_t* str,IO_Handle* f)
{
	UINT r = 0;

	ENABLE_INTERRUPTS;
	#ifndef IO_FULL_OPT
	if(f)
	{
	#endif
		f_write(f,(const void*)str,(uint32_t)strlen((char*)str),&r);
	#ifndef IO_FULL_OPT
	}
	#endif

	DISABLE_INTERRUPTS;
	return r;
}

uint32_t io_printf(IO_Handle* f,const int8_t* fmt,...)
{
	char* buf = (char*)&ioGenericBuf[0];
	UINT r = 0;

	ENABLE_INTERRUPTS;

	#ifndef IO_FULL_OPT
	if(f)
	{
	#endif
		*(uint32_t*)&ioGenericBuf = 0x00000000;
		va_list	ap;
		va_start(ap,(const char*)fmt);
		vsprintf(buf,(const char*)fmt,ap);
		va_end(ap);

		f_write(f,(const void*)buf,strlen(buf),&r);
	#ifndef IO_FULL_OPT
	}
	#endif

	DISABLE_INTERRUPTS;
	return r;
}

uint32_t io_putc(int32_t c,IO_Handle* f)
{
	UINT r = 0;

	c &= 0xff;

	ENABLE_INTERRUPTS;
	#ifndef IO_FULL_OPT
	if(f)
	{
	#endif
		f_write(f,(int8_t*)(&c),1,&r);
	#ifndef IO_FULL_OPT
	}
	#endif

	DISABLE_INTERRUPTS;
	return r;
}

int32_t io_getc(IO_Handle* f)
{
	UINT r = 0;
	int32_t c = 0;

	ENABLE_INTERRUPTS;
	#ifndef IO_FULL_OPT
	if(f)
	{
	#endif
		f_read(f,(int8_t*)&c,1,&r);
	#ifndef IO_FULL_OPT
	}
	#endif
	
	DISABLE_INTERRUPTS;
	return c & 0xff;
}

int32_t io_eof(IO_Handle* f)
{
	#ifndef IO_FULL_OPT
	if(!f)
	{
	#endif
		return 0;
	#ifndef IO_FULL_OPT
	}
	#endif

	return f->fptr >= f->fsize;
}

int32_t io_error(IO_Handle* f)
{
	#ifndef IO_FULL_OPT
		return (f) ? ((f->flag & FA__ERROR) != 0) : 0;
	#else
		return ((f->flag & FA__ERROR) != 0);
	#endif
}

int32_t io_truncate(IO_Handle* f)
{
	return f_truncate(f);
}

void io_close(IO_Handle* f)
{
	if(!f)
		return;

	f_close(f);
	free(f);
}

void io_remove(const int8_t* filename)
{
	XCHAR* buf = (XCHAR*)&ioGenericBuf[0];
	_____io_internal_c2wstrcpy(buf,filename);

	f_unlink(buf);
}

IO_Handle* io_open(const int8_t* filename,const int8_t* mode)
{
	XCHAR* buf = (XCHAR*)&ioGenericBuf[0];
	IO_Handle* f = (IO_Handle*)malloc(sizeof(IO_Handle));
	uint32_t flags = 0;
	uint32_t eof = 0 , fnew = 0;

	if(!f)
		return (void*)0;

	_____io_internal_c2wstrcpy(buf,filename);

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
					flags |= FA_WRITE;
					flags |= FA_READ;
				}

				break;
			}

			case 'r':
			{
				fnew -= (fnew > 0);
				flags |= FA_OPEN_EXISTING;
				flags |= FA_READ;

				if(*(mode+1) == '+')
				{   
					++mode;
					flags |= FA_WRITE;
				}

				break;
			}

			case 'w':
			{
				flags |= FA_CREATE_ALWAYS;
				flags |= FA_WRITE;

				if(*(mode+1) == '+')
				{   
					++fnew;
					++mode;
					flags |= FA_READ;
					flags |= FA_CREATE_NEW;
				}

				break;
			}
		}

		++mode;
	}

	if(f_open(f,buf,flags) != FR_OK)
	{
		free(f);
		return (void*)0;
	}

	if((eof) && (!fnew)) 
		f_lseek(f,f->fsize);

	return f;
}

/*Dir functions*/
IO_DirHandle* io_open_dir(const int8_t* dirpath)
{
	XCHAR* buf = (XCHAR*)&ioGenericBuf[0];
	IO_DirHandle* dp = (IO_DirHandle*)malloc(sizeof(IO_DirHandle));

	if(!dp)
		return (void*)0;

	_____io_internal_c2wstrcpy(buf,dirpath);

	if(f_opendir(dp,buf) != FR_OK)
	{
		free(dp);
		return (void*)0;
	}

	return dp;
}

void io_close_dir(IO_DirHandle* dp)
{
	if(dp)
		free(dp);
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


#include "libio.h"
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>

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
		#ifndef SNES_LOVELY_POINTERS
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
		#ifndef SNES_LOVELY_POINTERS
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
				#ifndef SNES_LOVELY_POINTERS
					ioFile[addr].addr = ((uint32_t)&ioFile[addr].handle);
				#else
					ioFile[addr].bank = (uint8_t)((uint32_t)&ioFile[addr].handle & 0xff000000);
					ioFile[addr].addr = (uint16_t)((uint32_t)&ioFile[addr].handle & 0x0000ffff);
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
				#ifndef SNES_LOVELY_POINTERS
					ioDir[addr].addr = ((uint32_t)&ioDir[addr].handle);
				#else
					ioDir[addr].bank = (uint8_t)((uint32_t)&ioDir[addr].handle & 0xff000000);
					ioDir[addr].addr = (uint16_t)((uint32_t)&ioDir[addr].handle & 0x0000ffff);
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
		#ifdef SNES_LOVELY_POINTERS
			uint8_t bbank = (uint8_t)(addr & 0xff000000);
			uint16_t boffs = (uint16_t)(addr & 0x0000ffff);
		#endif

		if(isFile != 0x00)
		{
			while(ptr < MAX_FILE_HANDLES)
			{
				#ifndef SNES_LOVELY_POINTERS
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
				#ifndef SNES_LOVELY_POINTERS
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
			ioFile[ioDirPtr--].alive = 0x1;

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
			f_lseek(f,(((f->fptr + addr) > f->fsize) ? f->fsize : f->fptr + addr));
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
	int8_t* buf = (int8_t*)&ioGenericBuf[0];
	UINT r = 0;

	#ifndef IO_FULL_OPT
	if(f)
	{
	#endif
		buf[0] = 0;
		va_list	ap;
		va_start(ap,(const char*)fmt);
		vsprintf((char*)buf,(const char*)fmt,ap);
		va_end(ap);

		ENABLE_INTERRUPTS;
		f_write(f,(const void*)buf,strlen((char*)buf),&r);
		DISABLE_INTERRUPTS;
	#ifndef IO_FULL_OPT
	}
	#endif

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

	#if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
		_____io_internal_mark_free(0,(uint32_t)(f) & 0x7fffffff);
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

IO_Handle* io_open(const int8_t* filename,const int8_t* mode)
{
	XCHAR* buf = (XCHAR*)&ioGenericBuf[0];
	IO_Handle* f = (void*)0;
	uint32_t flags = 0;
	uint32_t eof = 0 , fnew = 0;

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
		#if ( defined(IO_TARGET_PLATFORM_MD32X) || defined(IO_TARGET_PLATFORM_SNES))
			free(f);
		#endif

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


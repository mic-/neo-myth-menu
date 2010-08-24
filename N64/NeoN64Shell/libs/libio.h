#ifndef _libio_h_
#define _libio_h_

//LIBIO 0.2

#include <ff.h>
#include <stdint.h>

#undef IO_TARGET_PLATFORM_SNES
#undef IO_TARGET_PLATFORM_MD32X
#undef SNES_LOVELY_POINTERS
#define IO_TARGET_PLATFORM_N64

#define IO_Handle FIL
#define IO_HandleInfo FILINFO
#define IO_HandleResult FRESULT
#define IO_DirHandle DIR
#undef IO_FULL_OPT

/*MD / 32X*/
#if (IO_TARGET_PLATFORM_MD32X)
	#define MAX_FILE_HANDLES (16)
	#define MAX_DIR_HANDLES (8)
	#define DISABLE_INTERRUPTS asm("move.w 0x2700,sr")
	#define ENABLE_INTERRUPTS asm("move.w 0x2000,sr")
#endif
	
/*SNES*/
#if defined(IO_TARGET_PLATFORM_SNES)
	#define MAX_FILE_HANDLES (8)
	#define MAX_DIR_HANDLES (4)
	#define SNES_LOVELY_POINTERS
	#define DISABLE_INTERRUPTS asm("sei")
	#define ENABLE_INTERRUPTS asm("cli")
#endif

/*N64*/
#if defined(IO_TARGET_PLATFORM_N64)
	#define DISABLE_INTERRUPTS asm("\tmfc0 $8,$12\n\tla $9,~1\n\tand $8,$9\n\tmtc0 $8,$12\n\tnop":::"$8","$9")
	#define ENABLE_INTERRUPTS asm("\tmfc0 $8,$12\n\tori $8,1\n\tmtc0 $8,$12\n\tnop":::"$8")
#endif

#ifndef DISABLE_INTERRUPTS
	#define DISABLE_INTERRUPTS
#endif

#ifndef ENABLE_INTERRUPTS
	#define ENABLE_INTERRUPTS
#endif

enum
{
	IO_SEEK_SET = 0xFFFF,
	IO_SEEK_END,
	IO_SEEK_CUR
};

void io_init(uint32_t genericBufSize);
void io_shutdown();
void io_close(IO_Handle* f);
void io_remove(const int8_t* filename);

IO_Handle* io_open(const int8_t* filename,const int8_t* mode);

int32_t io_getc(IO_Handle* f);
int32_t io_eof(IO_Handle* f);
int32_t io_error(IO_Handle* f);
int32_t io_truncate(IO_Handle* f);

uint32_t io_seek(IO_Handle* f,uint32_t addr,uint32_t mode);
uint32_t io_tell(IO_Handle* f);
uint32_t io_size(IO_Handle* f);
uint32_t io_write(const void* ptr,uint32_t size,IO_Handle* f);
uint32_t io_read(void* ptr,uint32_t size,IO_Handle* f);
uint32_t io_puts(const int8_t* str,IO_Handle* f);
uint32_t io_printf(IO_Handle* f,const int8_t* fmt,...);
uint32_t io_putc(int32_t c,IO_Handle* f);

/*Dir functions*/
IO_DirHandle* io_open_dir(const int8_t* dirpath);

void io_close_dir(IO_DirHandle* dp);
void io_remove_dir(const int8_t* dirpath);

int32_t io_change_dir(const int8_t* dirpath);
int32_t io_create_dir(const int8_t* dirpath);
int32_t io_read_dir(IO_DirHandle* dp,IO_HandleInfo* info);
#endif



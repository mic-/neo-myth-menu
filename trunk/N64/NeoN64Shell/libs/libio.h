#ifndef _libio_h_
#define _libio_h_

//A nice , portable , and optimized wrapper for libff
#include <ff.h>
#include <stdint.h>
#undef IO_MD32X_BUILD
#define IO_Handle FIL
#define IO_HandleInfo FILINFO
#define IO_HandleResult FRESULT
#define IO_DirHandle DIR
#undef IO_FULL_OPT

#ifdef IO_MD32X_BUILD
	extern void ints_on();
	extern void ints_off();
	#define DISABLE_INTERRUPTS ints_off
	#define ENABLE_INTERRUPTS ints_on
#else
	#define DISABLE_INTERRUPTS
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



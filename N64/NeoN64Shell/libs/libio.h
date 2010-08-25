#ifndef _libio_h_
#define _libio_h_

//LIBIO 0.3

#include <ff.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "libio_conf.h"

typedef enum IOD_Type IOD_Type;
enum IOD_Type
{
    IO_DEVICE_SD = 0xff,/*SD*/
    IO_DEVICE_SRAM,/*On board sram*/
    IO_DEVICE_EEPROM,/*On board eeprom(Mainly for N64 emulated eeprom)*/
    IO_DEVICE_GBA_SRAM,/*GBA sram*/
    IO_DEVICE_MYTH_PSRAM,/*Myth on board PSRAM. WARNING : N64 DOES NOT HAVE ONE*/
    IO_DEVICE_PSRAM/*GBA "ZIPRAM"*/
};

//#define IO_Handle FIL
typedef struct IO_Handle IO_Handle;
struct IO_Handle
{
    IOD_Type type;
    union
    {
        uint32_t address;
        FIL handle;
    };
};

#define IO_HandleInfo FILINFO
#define IO_HandleResult FRESULT
#define IO_DirHandle DIR

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

IO_Handle* io_open(const int8_t* device,const int8_t* filename,const int8_t* mode);

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



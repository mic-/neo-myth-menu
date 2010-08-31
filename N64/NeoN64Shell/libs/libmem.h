#ifndef _libmem_h_
#define _libmem_h_
#include <stdint.h>
#include <malloc.h>

#define N64_PORT

#ifdef N64PORT //8MB
	#define MAX_AVAIL_MEMORY (0x7fffff)
	#define GRANULARITY (sizeof(uint32_t) - 1)
#else
	#define MAX_AVAIL_MEMORY (0x7fffffff)
	#define GRANULARITY (sizeof(uint64_t) - 1)
#endif

#define mem_alloc16(__SIZE__) mem_alloc(16,__SIZE__)
#define mem_realloc16(__PTR__,__SIZE__) mem_realloc(16,__PTR__,__SIZE__)

void mem_init_leak_detector();
void mem_get_leak_stats(uint32_t* allocated,uint32_t* freed);
void* mem_alloc(int32_t alignment,uint32_t size);
void* mem_realloc(int32_t alignment,void* obj,uint32_t size);
void mem_free(void* obj);
#endif


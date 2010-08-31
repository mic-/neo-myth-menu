
#include "libmem.h"

static uint32_t leakBalance[2] = {0x00000000,0x00000000};

void mem_init_leak_detector()
{
	leakBalance[0] =
	leakBalance[1] = 0x00000000;
}

void mem_get_leak_stats(uint32_t* allocated,uint32_t* freed)
{
	*allocated = leakBalance[0];
	*freed = leakBalance[1];
}

void* mem_alloc(int32_t alignment,uint32_t size)
{
	size = (size + GRANULARITY) & (~GRANULARITY);
	size += (alignment) ? (1 << alignment) : 0;

	++leakBalance[0];
	return (void*)malloc(size);
}

void* mem_realloc(int32_t alignment,void* obj,uint32_t size)
{
	size = (size + GRANULARITY) & (~GRANULARITY);
	size += (alignment) ? (1 << alignment) : 0;

	++leakBalance[0];
	return (void*)realloc(obj,size);
}

void mem_free(void* obj)
{
	if(obj)
	{
		++leakBalance[1];
		free(obj);
	}
}




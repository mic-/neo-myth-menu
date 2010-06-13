#ifndef u_memory_g
#define u_memory_g

#include <memory.h>
#include <basetypes.h>

extern void*const mm_ram;
extern void*const mm_psram;

#ifdef __cplusplus
    extern "C" {
#endif

void*mm_alloc_int(void*mm_heap,uint32_t size);
void*mm_alloc(uint32_t size);
void mm_free(void*ptr);
void mm_init(void);
void mm_compact(void);

#ifdef __cplusplus
    }
#endif

#endif

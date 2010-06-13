#include <u_memory.h>

#define USED 1

typedef struct {
    union {
        uint32_t size;
        char data[0];
    };
} UNIT;

typedef struct {
    UNIT*free;
    UNIT*heap;
} heapdef;

static heapdef mm_ram_int;
static heapdef mm_psram_int;

void*const mm_ram=&mm_ram_int;
void*const mm_psram=&mm_psram_int;

static UNIT*compact(UNIT*p,uint32_t nsize)
{
    uint32_t bsize=0,psize;
    UNIT*best=p;
    while((psize=p->size))
    {
        if(psize&USED)
        {
            if(bsize!=0)
            {
                best->size=bsize;
                if(bsize>=nsize)
                {
                    return best;
                }
                bsize=0;
            }
            best=p=(UNIT*)&(p->data[psize&~USED]);
        }
        else
        {
            bsize+=psize;
            p=(UNIT*)&(p->data[psize]);
        }
    }
    if(bsize!=0)
    {
        best->size=bsize;
        if(bsize>=nsize)
        {
            return best;
        }
    }
    return 0;
}

void mm_free(void*ptr)
{
    if(ptr)
    {
        UNIT*p=(UNIT*)(((char*)ptr)-sizeof(UNIT));
        p->size&=~USED;
    }
}

void*mm_alloc_int(void*pvheap,uint32_t size)
{
    heapdef*mm_heap=(heapdef*)pvheap;
    uint32_t fsize;
    UNIT*p;

    if(size==0)return 0;

    size+=3+sizeof(UNIT);
    size>>=2;
    size<<=2;

    if(mm_heap->free==0||size>mm_heap->free->size)
    {
        mm_heap->free=compact(mm_heap->heap,size);
        if(mm_heap->free==0)return 0;
    }

    p=mm_heap->free;
    fsize=mm_heap->free->size;

    if(fsize>(size+sizeof(UNIT)))
    {
        mm_heap->free=(UNIT*)&(p->data[size]);
        mm_heap->free->size=fsize-size;
    }
    else
    {
        mm_heap->free=0;
        size=fsize;
    }

    p->size=size|USED;

    return (void*)&(p->data[sizeof(UNIT)]);
}

void*mm_alloc(uint32_t size)
{
    void*res=0;
    if(size<=(2*1024))
    {
        res=mm_alloc_int(mm_ram,size);
    }
    if(res==0)
    {
        res=mm_alloc_int(mm_psram,size);
    }
    return res;
}

static void mm_init_int(void*pvheap,void*heap,uint32_t len)
{
    heapdef*mm_heap=(heapdef*)pvheap;
    len>>=2;
    len<<=2;
    mm_heap->free=mm_heap->heap=(UNIT*)heap;
    mm_heap->heap->size=len-sizeof(UNIT);
    *((uint32_t*)&(mm_heap->heap->data[len]))=0;
}

#define XFER_SIZE 16384
extern unsigned char __attribute__((aligned(16))) buffer[XFER_SIZE*2];

void mm_init(void)
{
    mm_init_int(mm_ram,buffer+XFER_SIZE,sizeof(buffer)-XFER_SIZE);
    mm_init_int(mm_psram,(void*)(0x300000+0x100000/4),0x100000/4);
}

static void mm_compact_int(void*pvheap)
{
    heapdef*mm_heap=(heapdef*)pvheap;
    mm_heap->free=compact(mm_heap->heap,0x7FFFFFFF);
}

void mm_compact(void)
{
    mm_compact_int(mm_ram);
    mm_compact_int(mm_psram);
}

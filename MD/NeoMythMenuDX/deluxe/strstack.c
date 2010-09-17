
#include "strstack.h"

unsigned char __attribute__((aligned(16))) strstackBlock[STRINGSTACK_BLOCK_SIZE * STRINGSTACK_BLOCK_DIVISIONS];
unsigned short strstackBlockPtr = 0;

void strstack_init()
{
	strstackBlockPtr = 0;
}


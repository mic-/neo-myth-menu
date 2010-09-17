#ifndef __strstack_h__
#define __strstack_h__

#define STRINGSTACK_BLOCK_SIZE (1024)
#define STRINGSTACK_BLOCK_DIVISIONS (4)

extern unsigned char strstackBlock[];
extern unsigned short strstackBlockPtr;

void strstack_init();

#define strstack_push(TYPE)\
((strstackBlockPtr > STRINGSTACK_BLOCK_DIVISIONS-1) ? (TYPE*)0 : (TYPE*)strstackBlock[strstackBlockPtr++])

#define strstack_pop(TYPE)\
((strstackBlockPtr <= 0) ? (TYPE*)0 : (TYPE*)strstackBlock[--strstackBlockPtr])

#define cstrstack_push() strstack_push(char)
#define ustrstack_push() strstack_push(unsigned char)
#define wstrstack_push() strstack_push(WCHAR)
#define xstrstack_push() strstack_push(XCHAR)
#define cstrstack_pop() strstack_pop(char)
#define ustrstack_pop() strstack_pop(unsigned char)
#define wstrstack_pop() strstack_pop(WCHAR)
#define xstrstack_pop() strstack_pop(XCHAR)

#endif


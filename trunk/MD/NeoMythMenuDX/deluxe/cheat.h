//By conle (conleon1988@gmail.com) for ChillyWilly's Extended SD Menu
#ifndef _CHEAT_H_INCLUDED_
#define _CHEAT_H_INCLUDED_

typedef struct CheatPair CheatPair;

struct CheatPair
{
	unsigned short data;
	unsigned int addr;
};

int cheat_decode(const char* code,CheatPair* pair);
int cheat_apply(void* data,CheatPair* pair);
#endif



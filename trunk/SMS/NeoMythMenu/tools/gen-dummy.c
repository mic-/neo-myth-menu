#include <stdio.h>

const int pattern_size = 8 * 1024;
const int pattern = 0x00;
const int pattern_acc = 0x00;

int main()
{
	int a,b;
	FILE* f;

	if(pattern_size < 1){ return 1; }
	f = fopen("dummy.bin","wb");
	if(!f){return 1;}

	a = pattern_size;
	b = pattern;

	do
	{ 
		fputc(b,f); 
		b += pattern_acc;
	}while(--a);

	fclose(f);
	return 0;
}


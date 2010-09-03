#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "../libconf.h"

int main()
{
	int i,len;
	FILE* f;
	char* p;

	config_init();

	f = fopen("config.cfg","rb");
	
	fseek(f,0,SEEK_END);
	len = ftell(f);
	fseek(f,0,SEEK_SET);

	p = (char*)malloc(len);
	fread(p,1,len,f);
	fclose(f);
	config_loadFromBuffer(p,len);
	free(p);

	printf("Entry count = %d\n",config_getEntryCount());
	printf("name = [%s] , age = [%d] \n",config_getS("name"),config_getI("age"));
	printf("comment = [%s]\n",config_getS("comment"));
	printf("blabla = [%s]\n",config_getS("blabla"));
	printf("testtest = [%s]\n",config_getS("testtest"));
	config_shutdown();

	return 0;
}


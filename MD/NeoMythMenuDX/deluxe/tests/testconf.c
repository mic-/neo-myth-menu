#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "../conf.h"


int main()
{
	int i,len;
	FILE* f;
	char* p;

	config_init();

	f = fopen("DXCONF.CFG","rb");
	
	fseek(f,0,SEEK_END);
	len = ftell(f);
	fseek(f,0,SEEK_SET);

	p = (char*)malloc(len);
	fread(p,1,len,f);
	fclose(f);
	config_loadFromBuffer(p,len);
	free(p);

	ConfigEntry* e;

	for(i = 0; i < config_getEntryCount(); i++)
	{
		e = config_getfromIndex(i);
		printf("@[%d] va = [%s] , val = [%s] , vaLen = [%d] , valLen = [%d] \n",i,e->variable,e->value,e->variableLength,e->valueLength);
	}

	//len = strlen(config_getS("romName"));
	//printf("[%s][%d]\n",config_getS("romName"), len);

	return 0;
}

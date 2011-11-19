#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

/*SMS SKIN PACK TOOL 0.1 - conleon1988 [.@.] [g]mail com 2011*/

/*
		.SKN file :
		Header 512 bytes
		MYTHSKIN			(8BYTES)
		AUTHORNAME			(24BYTES)
		LEFT_MARGIN			(1BYTE)
		INSTRUCTIONS_Y		(1BYTE)
		pattern size		(2BYTES - LE ENCODED)
		name table size		(2BYTES - LE ENCODED)
		palette size		(2BYTES - LE ENCODED)
		font size			(2BYTES - LE ENCODED)

		DATA
		[pattern]			
		[name table]
		[palette]
		[font]
*/

int stream_open(FILE** dst,int* len,const char* s,const char* m)
{
	FILE* f;
	
	f = fopen(s,m);
	if(!f) { return 0; }

	*dst = f;
	fseek(f,0,SEEK_END);
	*len = ftell(f);
	fseek(f,0,SEEK_SET);

	return 1;
}

void stream_close(FILE* f)
{
	if(!f) { return; }
	fclose(f);
}

int pack_entry(FILE* pkg,int header_entry,const char* src)
{
	FILE* in;
	unsigned short w;
	int len,ptr;
	int alen;
	int save;

	printf("Packing %s...\n",src);

	if(!stream_open(&in,&len,src,"rb"))
	{ 
		printf("Packing %s...FAILED\n",src);
		return 0; 
	}

	save = ftell(in);
	fseek(pkg,header_entry,SEEK_SET);	
	w = (unsigned short)len;
	fwrite(&w,1,2,pkg);
	fseek(pkg,save,SEEK_SET);

	alen = (len + 512) & (~511);

	printf("Size %u\nAligned size %u\n\n",len,alen);
	for(ptr = 0;ptr < len;ptr++) { fputc(fgetc(in),pkg); }
	while(len < alen){ fputc(0x00,pkg); ++len; }

	stream_close(in);
	return 1;
}

void show_usage()
{
	printf("Usage : sms_skin_pack author_name leftMargin instructionsY out.skn pattern.bin name_tbl.bin palette.bin font.bin\n");
}

int main(int argc,char** argv)
{
	char* header;
	int entry_base,i;
	FILE* pkg;

	printf("SMS SKIN PACK TOOL 0.1 - conleon1988 [.@.] [g]mail com 2011\n");

	if(argc != 8 + 1)
	{
		show_usage();
		return 1;
	}

	printf("Packing... %s\n",argv[2]);

	pkg = fopen(argv[4],"r+b");
	if(!pkg)
	{
		printf("Packing %s FAILED\n",argv[1]);
		return 1;
	}

	header = (char*)malloc(512);
	
	if(!header)
	{
		fclose(pkg);
		printf("OUT OF MEMORY!\n");
		return 1;
	}

	memset(header,0,512);
	strcpy(header,"MYTHSKIN");
	strncpy(header + strlen("MYTHSKIN"),argv[1],24);
	header[32] = atoi(argv[2]);
	header[33] = atoi(argv[3]);
	fwrite(header,1,512,pkg);
	free(header);

	for(i = 0 , entry_base = 34;i < 5;i++,entry_base += 2)
	{
		if(!pack_entry(pkg,entry_base,argv[i + 4])) { break; }
	}

	fclose(pkg);

	printf("All done.Now place the file on your SDC :)\n");
	return 0;
}



#include <stdio.h>
#include <stdlib.h>



int main(int argc, char *argv[])
{
	FILE *inFile, *outFile;
	int i, c, files;
    int outSize = 0x4000;
    
	if (argc < 3) return 1;
    files = 1;
    if (strcmp(argv[1], "--size") == 0)
    {
        outSize = atoi(argv[2]);
        files += 2;
    }
    
	inFile = fopen(argv[files], "rb");
	outFile = fopen(argv[files+1], "wb");
	if (inFile == NULL || outFile == NULL) return 1;


	i = 0;
	while (i < outSize)
	{
		c = fgetc(inFile);
		if (c == EOF) break;
		fputc(c, outFile);
		i++;
	}

	fclose(inFile);

	while (i < outSize)
	{
		fputc(0, outFile);
		i++;
	}

	fclose(outFile);

	return 0;
}



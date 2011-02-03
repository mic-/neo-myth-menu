#include <stdio.h>
#include <stdlib.h>



int main(int argc, char *argv[])
{
	FILE *inFile, *outFile;
	int i, c;

	if (argc != 3) return 1;

	inFile = fopen(argv[1], "rb");
	outFile = fopen(argv[2], "wb");
	if (inFile == NULL || outFile == NULL) return 1;


	i = 0;
	while (i < 0x4000)
	{
		c = fgetc(inFile);
		if (c == EOF) break;
		fputc(c, outFile);
		i++;
	}

	fclose(inFile);

	while (i < 0x4000)
	{
		fputc(0, outFile);
		i++;
	}

	fclose(outFile);

	return 0;
}



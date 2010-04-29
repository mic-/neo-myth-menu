// Strips comments from assembly files
// Mic, 2010

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char**argv)
{
	FILE *inFile, *outFile;
	int column = 0;
	int c = ~EOF;

	if (argc == 3)
	{
		inFile = fopen(argv[1], "rb");
		outFile = fopen(argv[2], "wb");
		if (inFile && outFile)
		{
			while (c != EOF)
			{
				c = fgetc(inFile);
				if (c != EOF)
				{
					if ((c == ';') && (column == 0))
					{
						for (; c != 10; c = fgetc(inFile))
							if (c == EOF) break;
						column = 0;
					}
					else if ((c == 10) || (c == 13))
					{
						fputc(c, outFile);
						column = 0;
					}
					else
					{
						column++;
						fputc(c, outFile);
					}
				}
			}
		}
		else
			printf("Failed to open/create file\n");
	}
	else
		printf("Usage: stripcomments input.asm output.asm\n");

	return 0;
}

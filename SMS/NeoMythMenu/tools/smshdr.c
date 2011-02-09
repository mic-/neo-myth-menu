#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
	FILE *smsFile;
	int i;
	unsigned short chksum;
	unsigned char c;

	if (argc != 2) return 1;

	smsFile = fopen(argv[1], "rb+");
	if (smsFile == NULL) return 1;

	for (i = 0; i < 0x7FF0; i++)
	{
		c = fgetc(smsFile);
		chksum += c;
	}

	fseek(smsFile, 0x7FF0, SEEK_SET);

	fputs("TMR SEGA  ", smsFile);

	fputc(chksum&0xFF, smsFile);
	fputc(chksum>>8, smsFile);

	fputc(0, smsFile);	// product code
	fputc(0, smsFile);	// ...
	fputc(0, smsFile);	// .. / version

	fputc(0x4C, smsFile);	// 32 kB ROM / US/Eu region
							// ( ROM size doesn't have to be the actual size. It's used to determine
							//   how much of the ROM to checksum )

	fclose(smsFile);

	return 0;
}


/*conleon1988@gmail.com 2011*/
#include <stdio.h>
#include <string.h>

typedef struct RomHeader RomHeader;
struct RomHeader
{
   unsigned char init_PI_BSB_DOM1_LAT_REG;  /* 0x00 */
   unsigned char init_PI_BSB_DOM1_PGS_REG;  /* 0x01 */
   unsigned char init_PI_BSB_DOM1_PWD_REG;  /* 0x02 */
   unsigned char init_PI_BSB_DOM1_PGS_REG2; /* 0x03 */
   unsigned int clockRate;                  /* 0x04 */
   unsigned int pc;                         /* 0x08 */
   unsigned int version;                    /* 0x0C */
   unsigned int crc1;                       /* 0x10 */
   unsigned int crc2;                       /* 0x14 */
   unsigned int pad[2];                 /* 0x18 */
   unsigned char name[20];                   /* 0x20 */
   unsigned int pad1;                    /* 0x34 */
   unsigned int manufacturerID;            /* 0x38 */
   unsigned short cartID;             /* 0x3C - Game serial number  */
   unsigned short countryCode;             /* 0x3E */
   unsigned int bootCode[1008];            /* 0x40 */
};

static void byteswap16(char* src,unsigned int length)
{
    unsigned int i;
    unsigned char b;

    for(i = 0; i < length ; i+=2)
    {
        b = src[i];
        src[i] = src[i + 1];
        src[i + 1] = b;
    }
}

static void byteswap32(char* src,unsigned int length)
{
    unsigned int i;
    unsigned char b;

    for(i = 0; i < length; i += 4)
    {
        b = src[i];
        src[i] = src[i + 3];
        src[i + 3] = b;

        b = src[i + 1];
        src[i + 1] = src[i + 2];
        src[i + 2] = b;
    }
}

inline unsigned int getLong(const char* src)
{
    return
    (
        (((unsigned int)(*(src + 0))&0xffL) << 24) |
        (((unsigned int)(*(src + 1))&0xffL)  << 16) |
        (((unsigned int)(*(src + 2))&0xffL) << 8) |
        (((unsigned int)*(src + 3)) & 0xffL)
    );
}

void author()
{
	printf("bc_swap ~ conleon1988@gmail.com |2010~2011|\n");
}

void help()
{
	printf("Usage : bc_swap sourcerom.ext destrom.ext\n");
}

int main(int argc, char* argv[])
{
	RomHeader romHdr;
	FILE* out;
	FILE* rom;
	const char* src;
	const char* dst;
	unsigned int fmt;
	unsigned int read;

	author();

	if(argc < 3)
	{
		help();
		return 1;
	}

	src = argv[1];
	dst = argv[2];

	printf("%s -> %s\n",src,dst);
	printf("Patching BOOTCODE...\n");

	rom = fopen(src,"rb");
	read = fread((char*)&romHdr,1,sizeof(romHdr),rom);
	fclose(rom);

	switch(getLong((char*)&romHdr))
	{
		case 0x80371240: 
			printf("Image SOURCE format = z64\n");
		break;

		case 0x37804012: 
		{
			printf("Image SOURCE format = v64 \n");
			byteswap16((char*)&romHdr,sizeof(romHdr));
			break;
		}

		case 0x40123780: 
		{
			printf("Image SOURCE format = n64 \n");
			byteswap32((char*)&romHdr,sizeof(romHdr));
			break;
		}
	}

	out = fopen(dst,"r+b");
	read = fread((char*)&fmt,1,4,out);

	printf("Convert img..\n");
	switch(getLong((char*)&fmt))
	{
		case 0x80371240: 
			printf("Image DEST format = z64\n");
		break;

		case 0x37804012: 
		{
			printf("Image DEST format = v64 \n");
			byteswap16((char*)&romHdr,sizeof(romHdr));
			break;
		}

		case 0x40123780: 
		{
			printf("Image DEST format = n64 \n");
			byteswap32((char*)&romHdr,sizeof(romHdr));
			break;
		}
	}

	fseek(out,sizeof(romHdr) - ( 1008 * 4 ),SEEK_SET);
	fwrite( ((char*)&romHdr) + (sizeof(romHdr) - ( 1008 * 4 )) ,1,( 1008 * 4 ),out);
	fclose(out);

	printf("Patching BOOTCODE...done\n");
	return 0;
}


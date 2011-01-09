/*conleon1988@gmail.com 2011*/
#include <stdio.h>
#include <string.h>
#include "mips/dis-asm.h"
#define EIO (5)

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
	printf("bc_dump ~ conleon1988@gmail.com |2011|\n");
}

void help()
{
	printf("Usage : bc_dump rom.ext bootcode.bin bootcode_dissasembly.txt\n");
}

int main(int argc, char* argv[])
{
	RomHeader romHdr;
	FILE* bc;
	FILE* rom;
	FILE* dis;
	const char* src;
	const char* dst;
	const char* disOut;
	unsigned char* buf;
	unsigned int i;
	unsigned int read;
	int instLen;
	disassemble_info dis_inf;
	
	author();

	if(argc < 4)
	{
		help();
		return 1;
	}

	src = argv[1];
	dst = argv[2];
	disOut = argv[3];

	printf("%s -> %s -> %s\n",src,dst,disOut);

	printf("Extracting BOOTCODE...\n");

	rom = fopen(src,"rb");
	read = fread((char*)&romHdr,1,sizeof(romHdr),rom);
	fclose(rom);

	switch(getLong((char*)&romHdr))
	{
		case 0x80371240:
			printf("Image format = z64\n");
		break;

		case 0x37804012: 
		{
			printf("Image format = v64 ( output --> z64 )\n");
			byteswap16((char*)&romHdr,sizeof(romHdr));
			break;
		}

		case 0x40123780:
		{
			printf("Image format = n64 ( output --> z64 )\n");
			byteswap32((char*)&romHdr,sizeof(romHdr));
			break;
		}
	}

	printf("Entry point = 0x%08x\n",romHdr.pc);
	bc = fopen(dst,"wb");
	fwrite( ((char*)&romHdr) + (sizeof(romHdr) - ( 1008 * 4 )) ,1,( 1008 * 4 ),bc);
	fclose(bc);

	printf("Extracting BOOTCODE...done\n");

	buf = (unsigned char*)malloc(1008 * 4);
	bc = fopen(dst,"rb");
	read = fread(buf,1,1008*4,bc);
	fclose(bc);

	printf("Generating %s...\n",disOut);
	dis = fopen(disOut,"wb");
	INIT_DISASSEMBLE_INFO(dis_inf,dis,fprintf);
	dis_inf.buffer = buf;
	dis_inf.buffer_length = 1008 * 4;
	dis_inf.flavour = bfd_target_unknown_flavour;
	dis_inf.arch = bfd_arch_mips;
	dis_inf.mach = bfd_mach_mips4300;
	dis_inf.endian = BFD_ENDIAN_BIG;
	
	fprintf(dis,"Dissasembly of %s\n\n\n\n",dst);

	instLen = 0;
	i = 0;

	while(i < 1008 * 4)
	{
		fprintf(dis,"0x%x:		",i);
		instLen = print_insn_big_mips(i,&dis_inf);
		fprintf(dis,"\n");
		i += instLen;
	}

	fclose(dis);
	free(buf);
	printf("Generating %s...done\n",disOut);

	return 0;
}

int buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, int length,struct disassemble_info *info)
{
	if (memaddr < info->buffer_vma|| memaddr + length > info->buffer_vma + info->buffer_length)
		return EIO;

	memcpy (myaddr, info->buffer + (memaddr - info->buffer_vma), length);
	return 0;
}

int target_read_memory (bfd_vma memaddr,bfd_byte *myaddr,int length,struct disassemble_info *info)
{
	/*cpu_memory_rw_debug(cpu_single_env, memaddr, myaddr, length, 0);*/
	return 0;
}

void perror_memory (int status, bfd_vma memaddr, struct disassemble_info *info)
{
	if (status != EIO)
	    (*info->fprintf_func) (info->stream, "Unknown error %d\n", status);
	else
	    (*info->fprintf_func) (info->stream,"Address 0x%" PRIx64 " is out of bounds.\n", memaddr);
}

void generic_print_address (bfd_vma addr, struct disassemble_info *info)
{
	(*info->fprintf_func) (info->stream, "0x%" PRIx64, addr);
}

int generic_symbol_at_address (bfd_vma addr, struct disassemble_info *info)
{
	return 1;
}


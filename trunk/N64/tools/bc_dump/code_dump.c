/*
	by conleon1988@gmail.com 2011
	Notes:
	Ugly code..
*/

#include <stdio.h>
#include <string.h>
#include "mips/dis-asm.h"
#define EIO (5)
typedef struct Reg Reg;
struct Reg
{
	char name[4];
	char* line;
	short refd;
	unsigned int addr;
};

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

static Reg* regs = NULL;
static unsigned int regc = 0;

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
	printf("code_dump ~ conleon1988@gmail.com |2011|\n");
}

void help()
{
	printf("Usage : code_dump rom.ext rom_dissasembly.txt\n");
	printf("Usage(optional) : code_dump rom.ext rom_dissasembly.txt rom_subroutines.txt\n");
}

int read_line(FILE* f,char* buf,int* size)
{
	int last = EOF;
	int addr = 0;
	*size = 0;

	while( (last = fgetc(f)) != EOF)
	{
		if(last == '\r' || last == '\n')
			break;

		buf[addr++] = (char)last;
	}

	buf[addr] = '\0';

	*size = addr;
	return last;
}

void writeback_reg(const char* rs,const char* line,unsigned int depth)
{
	Reg* r;

	if(regs == 0)
	{
		regs = malloc(sizeof(Reg));
		r = regs;
		r->line = malloc(strlen(line));
		r->refd = 0;
		r->addr = depth;
		strcpy(r->name,rs);
		strcpy(r->line,line);
		printf("Writeback reg : %s\n",r->name);
		++regc;
		return;
	}
	else
	{
		regs = realloc(regs,(regc + 1) * sizeof(Reg));
		r = &regs[regc++];
		
		if(!regs)
			printf("Out of memory!\n");

		r->line = malloc(strlen(line));
		r->refd = 0;
		r->addr = depth;
		strcpy(r->name,rs);
		strcpy(r->line,line);
		printf("Writeback reg : %s\n",r->name);
	}
}

int main(int argc, char* argv[])
{
	RomHeader romHdr;
	FILE* rom;
	FILE* dis;
	FILE* subrf;
	const char* src;
	const char* disOut;
	const char* subr = 0;
	unsigned char* buf;
	char line[256];
	char temp[32];
	char temp2[32];
	unsigned int i,read,len,endianess,depth,line_addr;
	char* a;
	int instLen,j,/*k,*/l,lns;
	disassemble_info dis_inf;
	
	author();

	if(argc < 3 || argc > 4)
	{
		help();
		return 1;
	}

	src = argv[1];
	disOut = argv[2];
	if(argc == 4)
		subr = argv[3];

	printf("%s -> %s\n",src,disOut);

	rom = fopen(src,"rb");
	read = fread((char*)&endianess,1,4,rom);
	fseek(rom,0,SEEK_SET);
	fseek(rom,0,SEEK_END);
	len = ftell(rom);
	fseek(rom,0,SEEK_SET);
	buf = (unsigned char*)malloc(len);
	read = fread(buf,1,len,rom);
	fclose(rom);

	switch(getLong((char*)&endianess))
	{
		case 0x80371240:
			printf("Image format = z64\n");
		break;

		case 0x37804012: 
		{
			printf("Image format = v64 ( output --> z64 )\n");
			byteswap16((char*)buf,len);
			break;
		}

		case 0x40123780:
		{
			printf("Image format = n64 ( output --> z64 )\n");
			byteswap32((char*)buf,len);
			break;
		}
	}

	memcpy(&romHdr,buf,sizeof(romHdr));

	instLen = 0;
	i = 0x40; 
	len -= i;

	printf("Generating %s...\n",disOut);
	dis = fopen(disOut,"wb");
	INIT_DISASSEMBLE_INFO(dis_inf,dis,fprintf);
	dis_inf.buffer = buf + i;
	dis_inf.buffer_length = len;
	dis_inf.flavour = bfd_target_unknown_flavour;
	dis_inf.arch = bfd_arch_mips;
	dis_inf.mach = bfd_mach_mips4300;
	dis_inf.endian = BFD_ENDIAN_BIG;
	
	fprintf(dis,"Dissasembly of %s\n\n\n\n",src);
	fprintf(dis,"Entry point : 0x%08x\n\n\n\n",romHdr.pc);


	fprintf(dis,"/*********************BOOT CODE START***************************/\n\n\n\n");
	j = 0;

	while(i < len)
	{
		if((i >= 0x40 + (1008*4)) && (j==0))
		{
			j = 1;
			fprintf(dis,"/*********************BOOT CODE END***************************/\n\n\n\n");
		}
		else if( (i >= romHdr.pc) && (j == 1) )
		{
			j = 2;
			fprintf(dis,"\n\n/*********************ENTRY POINT****************************/\n\n\n\n");
		}

		fprintf(dis,"0x%x:		",i);
		instLen = print_insn_big_mips(i,&dis_inf);
		fprintf(dis,"\n");
		i += instLen;
	}

	fclose(dis);
	free(buf);
	printf("Generating %s...done\n\nWill now look for jump-to-register instructions...\n\n\n",disOut);

	dis = fopen(disOut,"r");
	fseek(dis,0,SEEK_END);
	len = ftell(dis);
	fseek(dis,0,SEEK_SET);
	i = 0;
	depth = 0;

	while(i < len)
	{
		if(read_line(dis,line,&lns) == EOF)
			break;

		if(lns)
		{
			if(strstr(line,",8(") && (!strstr(line,"sd")) && (!strstr(line,"sw")) 
			&& (!strstr(line,"sh")) && (!strstr(line,"sb")) && (!strstr(line,"(sp)"))  && (!strstr(line,"ra")) )
			{
				a = strstr(line,",");
				l = 1;

				if(a)
				{
					--a;
					while((a >= line) && (l >= 0))
					{
						if(*a == '\t' || *a == ' ')break;
						temp[l--] = *(a--);
					}

					temp[2] = '\0';
					writeback_reg(temp,line,depth);
				}
			}
		}
		depth += lns;
		i += lns;
	}

	fseek(dis,0,SEEK_SET);
	i = 0;

	printf("\n\nNow looking for jumps to entry point..(%d cached regs)\n\n",(int)regc);

	if(regs != NULL)
	{
		while(i < len)
		{
			if(read_line(dis,line,&lns) == EOF)
				break;

			if(lns)
			{
				Reg* r;
				for(j = 0;j<(int)regc;j++)
				{
					r = &regs[j];
					if(strstr(line,"jr")  && strstr(line,r->name) && (!strstr(line,"ra")) && (r->addr < depth) && (r->line[2] == line[2]))
					{
						r->refd = 1;
						printf("(!)Found jump to register (%s) that references entry point:\n",r->name);
						printf("-->Was referenced first  here   { %s }\n",r->line);
						printf("--->Was referenced again here   { %s }\n\n",line);
					}
				}
			}

			i += lns;
		}

		printf("\n\nNow looking for registers that seem to be point to entry point but where not used in jump instructions..\n\n");
		Reg* r;
		for(j = 0;j<(int)regc;j++)
		{
			r = &regs[j];
			if(r->refd)continue;
			printf("Register {%s} was used here {%s} and was not referenced\n",r->name,r->line);
		}
		printf("\n\nNow looking for registers that seem to be point to entry point but where not used in jump instructions..DONE\n\n");
	}
	else
		printf("Reg cache is empty!\n");

	
	if(subr != NULL)
	{
		printf("\n\nNow looking for subroutines -> %s ->> %s....\n",src,subr);
		subrf = fopen(subr,"w");
		fseek(dis,0,SEEK_SET);
		i = l = 0;
		depth = line_addr = 0;

		while(i < len)
		{
			if(read_line(dis,line,&lns) == EOF)
				break;

			if(lns)
			{
				j = 4;
				a = strstr(line,"jalr");
				if(!a)
				{
					a = strstr(line,"jal");
					--j;
				}

				if(a)
				{
					++depth;
					for(l = 0;(line[l]) && (line[l] != ':');l++)
						temp[l] = line[l];

					temp[l] = '\0';

					a += j;
					l = 0;

					while(*a)
						temp2[l++] = *(a++);

					temp2[l] = '\0';

					if(j == 3)
						fprintf(subrf,"Found subroutine call at line : %u (%s) @@%s {%s}\n",line_addr,temp,temp2,line);
					else
						fprintf(subrf,"Found REG subroutine call at line : %u (%s) @@%s {%s}\n",line_addr,temp,temp2,line);
				}
			}

			++line_addr;
			i += lns;
		}
		printf("\n\nNow looking for subroutines -> %s ->> %s....DONE\n",src,subr);
		printf("\n\nTotal subroutines found : %u\n",depth);
		fclose(subrf);
	}

	fseek(dis,0,SEEK_SET);
	i = 0;

	fclose(dis);

	if(regs)
	{
		while(--regc)
			free(regs[regc].line);

		free(regs);
	}

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


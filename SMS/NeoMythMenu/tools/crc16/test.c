/*
	SD CRC16 implementation using bitstreams	(to easily adapt it on platforms without 32bit instruction set)
	Author:conleon1988@gmail.com - 27/10/2011 for neo-myth-menu google code project

	Bitarray class by : Michael Dipperstein
*/
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "bitarray.h"

const int sectors_to_read = 8;						/*512byte/sector*/
const char* filename = "FILENAME HERE";				/*file to open*/

void poly_set(unsigned char* poly,int acc_in)
{	
	unsigned char acc = (unsigned char)acc_in;

	poly[0] = 0x00; poly[1] = acc;					/*000a*/
	poly[2] = 0x00; poly[3] = 0x00;					/*0000*/
	poly[4] = 0x00; poly[5] = acc << 4;				/*00a0*/
	poly[6] = 0x00; poly[7] = acc;					/*000a*/
}

void bc_xori(bit_array_t* bca,bit_array_t* bcb,bit_array_t* bcd,bit_array_t* bce,unsigned char* poly,int imm0,int imm1,int imm2)
{
	if( (imm0 ^ imm1) & imm2 )						/*(00 00 00 00 00 00 00 aa ^ 00 00 00 00 00 00 00 0b) & 00 00 00 00 00 00 00 0c*/
	{
		poly_set(poly,imm2);						/*d = p[imm2]*/
		memcpy(bcd->array,poly,8);					/*...*/
		BitArrayXor(bce,bca,bcd);					/*t = (a ^ b)*/
		memcpy(bca->array,bce->array,8);			/*a = t*/
	}
}

bit_array_t* bc_grab_quad()
{
	return BitArrayCreate(64);
}

void bc_drop(bit_array_t* bc)
{
	if(bc) { BitArrayDestroy(bc); }
}

void crc16_bc(unsigned char* out,unsigned char* data,int len)
{
	unsigned char nybble;
	unsigned char poly[8];
	unsigned char lsb;
	int i,j;
	bit_array_t* bca;
	bit_array_t* bcb;
	bit_array_t* bcd;
	bit_array_t* bce;

	bca = bc_grab_quad();
	bcb = bc_grab_quad();
	bcd = bc_grab_quad();
	bce = bc_grab_quad();

	BitArrayClearAll(bca);											

	len <<= 1;
	for(i = 0;i<len;i++)
	{
        if(i & 1) 
			{ nybble = (data[i >> 1] & 0x0f); }
        else
            { nybble = (data[i >> 1] >> 4); }

		BitArrayCopy(bcb,bca);
		BitArrayShiftRight(bcb,60);
		BitArrayShiftLeft(bca,4);

		lsb = bcb->array[7];/*00 00 00 00 00 00 00 aa*/
		j = 1;
		do
		{
			bc_xori(bca,bcb,bcd,bce,poly,lsb,nybble,j);
			j <<= 1;
		}
		while(j <= 8);
	}

	memcpy(out,bca->array,8);

	bc_drop(bca);
	bc_drop(bcb);
	bc_drop(bcd);
	bc_drop(bce);
}

void crc16_orig(unsigned char *p_crc, unsigned char *data, int len)
{
    int i;
    unsigned char nybble;
    unsigned long long poly = 0x0001000000100001LL;
    unsigned long long crc = 0;
    unsigned long long n_crc;

    for (i = 0; i < 8; i++)
    {
        crc <<= 8;
        crc |= p_crc[i];
    }

    for (i = 0; i < (len << 1); i++)
    {
        if (i & 1)
            nybble = (data[i >> 1] & 0x0F);
        else
            nybble = (data[i >> 1] >> 4);

        n_crc = (crc >> 60);
        crc <<= 4;

        if ((nybble ^ n_crc) & 1) crc ^= (poly << 0);
        if ((nybble ^ n_crc) & 2) crc ^= (poly << 1);
        if ((nybble ^ n_crc) & 4) crc ^= (poly << 2);
        if ((nybble ^ n_crc) & 8) crc ^= (poly << 3);
    }

    for (i = 7; i >= 0; i--)
    {
        p_crc[i] = crc;
        crc >>= 8;
    }
}


void dump_crc(const char* msg,unsigned char* p)
{
	int i;

	printf("%s : ",msg);

	for(i = 0;i<8;i++)
		printf("0x%02x ",p[i]);

	printf("\n");
}

int main()
{
	unsigned char* sector;
	unsigned char* quada;
	unsigned char* quadb;
	int i;
	FILE* f;

	quada = malloc(8);
	quadb = malloc(8);
	sector = malloc(512);
	f = fopen(filename,"rb");

	for(i = 0;i < sectors_to_read;i++)
	{
		printf("Applying crc on sector %d.....\n",i);
		memset(quada,0,8);
		fread(sector,1,512,f);
		crc16_orig(quada,sector,512);
		dump_crc("crc16 original  :",quada);

		memset(quadb,0,8);
		crc16_bc(quadb,sector,512);
		dump_crc("crc16 bitstream :",quadb);
		
		if(memcmp(quada,quadb,8) == 0)
			printf("CRCS PERFECTLY MATCH :D!\n");
		else
			printf("CRCS DONT MATCH !\n");
	}

	fclose(f);
	free(quada);
	free(quadb);
	free(sector);
	return 0;
}


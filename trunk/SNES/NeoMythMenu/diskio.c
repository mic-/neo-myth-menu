/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2009      */
/*-----------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include "diskio.h"

//extern unsigned short do_neo_asic_op(unsigned int op);
//extern void do_neo_asic_wop(unsigned int op, unsigned short data);
extern void neo2_recv_sd(unsigned char *buf);
extern void neo2_pre_sd(void);
extern void neo2_post_sd(void);

// for debug
//extern void delay(int count);
//extern void put_str(char *str, int fcolor);
//extern short int gCursorX;				/* range is 0 to 63 (only 0 to 39 onscreen) */
//extern short int gCursorY;				/* range is 0 to 31 (only 0 to 27 onscreen) */

//#define DEBUG_PRINT
//#define DEBUG_RESP

/* global variables and arrays */

unsigned short cardType;				/* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */

unsigned char pkt[6];					/* command packet */

unsigned int sec_last;					/* last uncached sector read */
unsigned char __attribute__((aligned(16))) sec_buf[520];				/* for uncached reads */

#define CACHE_SIZE 16					/* number sectors in cache */
unsigned int sec_tags[CACHE_SIZE];
unsigned char __attribute__((aligned(16))) sec_cache[CACHE_SIZE*512 + 8];

#define R1_LEN (48/8)
#define	R2_LEN (136/8)
#define R3_LEN (48/8)
#define R6_LEN (48/8)
#define R7_LEN (48/8)

/* support code */

typedef volatile unsigned short vu16;

void wrMmcCmdBit( unsigned int bit ) __attribute__ ((section (".data")));
void wrMmcCmdBit( unsigned int bit )
{
	unsigned short data;
	unsigned int addr =
		//( 1<<24 ) |		// always 1 (set in GBAC_LIO)
		( 1<<19 ) |			// clk enable
		( 1<<18 ) |			// cs remap
		( 1<<17 ) |			// cmd remap
		( 1<<16 ) |			// d1-d3 remap
		( 1<<15 ) |			// d0 remap
		( 1<<14 ) |			// cs value (active high)
		( (bit&1)<<13 ) |	// cmd value
		( 1<<12 ) |			// d3 value
		( 1<<11 ) |			// d2 value
		( 1<<10 ) |			// d1 value
		( 1<<9 );			// d0 value
		data = *(vu16*)(addr);
}

void wrMmcCmdByte( unsigned int byte ) __attribute__ ((section (".data")));
void wrMmcCmdByte( unsigned int byte )
{
	int i;
	for( i = 0; i < 8 ; i++ )
		wrMmcCmdBit( (byte>>(7-i)) & 1 );
}

#if 0
void wrMmcDatBit( unsigned int bit ) __attribute__ ((section (".data")));
void wrMmcDatBit( unsigned int bit )
{
	unsigned short data;
	unsigned int addr =
		//( 1<<24 ) |		// always 1 (set in GBAC_LIO)
		( 1<<19 ) |			// clk enable
		( 1<<18 ) |			// cs remap
		( 1<<17 ) |			// cmd remap
		( 1<<16 ) |			// d1-d3 remap
		( 1<<15 ) |			// d0 remap
		( 1<<14 ) |			// cs value (active high)
		( 1<<13 ) |		    // cmd value
		( 1<<12 ) |			// d3 value
		( 1<<11 ) |			// d2 value
		( 1<<10 ) |			// d1 value
		( (bit&1)<<9 );		// d0 value
		data = *(vu16*)(addr);
}

void wrMmcDatByte( unsigned int byte ) __attribute__ ((section (".data")));
void wrMmcDatByte( unsigned int byte )
{
	int i;
	for( i = 0; i < 8 ; i++ )
		wrMmcDatBit( (byte>>(7-i)) & 1 );
}
#endif

unsigned int rdMmcCmdBit() __attribute__ ((section (".data")));
unsigned int rdMmcCmdBit()
{
	unsigned short data;
	unsigned int addr =
		//( 1<<24 ) |		// always 1 (set in GBAC_LIO)
		( 1<<19 ) |			// clk enable
		( 1<<18 ) |			// cs remap
		( 1<<17 ) |			// cmd remap
		( 1<<16 ) |			// d1-d3 remap
		( 1<<15 ) |			// d0 remap
		( 1<<14 ) |			// cs value (active high)
		( 1<<13 ) |			// cmd value
		( 1<<12 ) |			// d3
		( 1<<11 ) |			// d2
		( 1<<10 ) |			// d1
		( 1<<9 ) |			// d0
		( 1<<7 );			// cmd input mode
	data = *(vu16*)(addr);
	return (data>>4) & 1;
}

// 1 to 8 bits please

unsigned char rdMmcCmdBits( int num ) __attribute__ ((section (".data")));
unsigned char rdMmcCmdBits( int num )
{
	int i;
	unsigned char byte = 0;
	for( i = 0; i < num ; i++ )
		byte = ( byte << 1) | rdMmcCmdBit();
	return byte;
}

unsigned char rdMmcCmdByte() __attribute__ ((section (".data")));
unsigned char rdMmcCmdByte()
{
	return rdMmcCmdBits( 8 );
}

unsigned int rdMmcDatBit() __attribute__ ((section (".data")));
unsigned int rdMmcDatBit()
{
	unsigned short data;
	unsigned int addr =
		//( 1<<24 ) |		// always 1 (set in GBAC_LIO)
		( 1<<19 ) |			// clk enable
		( 1<<18 ) |			// cs remap
		( 1<<17 ) |			// cmd remap
		( 1<<16 ) |			// d1-d3 remap
		//( 1<<15 ) |		// d0 remap
		( 1<<14 ) |			// cs value (active high)
		( 1<<13 ) |			// cmd value
		( 1<<12 ) |			// d3
		( 1<<11 ) |			// d2
		( 1<<10 ) |			// d1
		//( 1<<9 ) |		// d0
		//( 1<<6 ) |		// d3-d1 input (only needed in 4 bit mode)
		( 1<<5);			// d0 input
	data = *(vu16*)(addr);
	return data & 1;
}

unsigned int rdMmcDatBit4() __attribute__ ((section (".data")));
unsigned int rdMmcDatBit4()
{
	unsigned short data;
	unsigned int addr =
		//( 1<<24 ) |		// always 1 (set in GBAC_LIO)
		( 1<<19 ) |			// clk enable
		( 1<<18 ) |			// cs remap
		( 1<<17 ) |			// cmd remap
		//( 1<<16 ) |		// d1-d3 remap
		//( 1<<15 ) |		// d0 remap
		( 1<<14 ) |			// cs value (active high)
		( 1<<13 ) |			// cmd value
		//( 1<<12 ) |		// d3
		//( 1<<11 ) |		// d2
		//( 1<<10 ) |		// d1
		//( 1<<9 ) |		// d0
		( 1<<6 ) |			// d3-d1 input (only needed in 4 bit mode)
		( 1<<5);			// d0 input
	data = *(vu16*)(addr);
	return data & 15;
}

#if 0
void wrMmcDatBit4( unsigned char dat ) __attribute__ ((section (".data")));
void wrMmcDatBit4( unsigned char dat )
{
	unsigned int addr =
		//( 1<<24 ) |		// always 1 (set in GBAC_LIO)
		( 1<<19 ) |			// clk enable
		( 1<<18 ) |			// cs remap
		( 1<<17 ) |			// cmd remap
		( 1<<14 ) |			// cs value (active high)
		( 1<<13 );			// cmd value
	*(vu16*)(addr) = ( dat & 15 );
}

void wrMmcDatByte4( unsigned char val ) __attribute__ ((section (".data")));
void wrMmcDatByte4( unsigned char val )
{
	wrMmcDatBit4( (val >> 4) & 15 );
	wrMmcDatBit4( val & 15 );
}
#endif

// 4 to 8 bits please

unsigned char rdMmcDatByte4() __attribute__ ((section (".data")));
unsigned char rdMmcDatByte4()
{
	unsigned char byte = rdMmcDatBit4();
	byte = ( byte << 4) | rdMmcDatBit4();
	return byte;
}

// 1 to 8 bits please

unsigned char rdMmcDatByte() __attribute__ ((section (".data")));
unsigned char rdMmcDatByte()
{
	int i;
	unsigned char byte = 0;
	for( i = 0; i < 8 ; i++ )
		byte = ( byte << 1) | rdMmcDatBit();
	return byte;
}

/*
 * Computation of CRC-7 0x48 polynom (x**7 + x**3 + 1)
 * Used in MMC commands and responses.
 */

unsigned char crc7 (unsigned char *buf) __attribute__ ((section (".data")));
unsigned char crc7 (unsigned char *buf)
{
	int i;
	unsigned int r4 = 0x80808080;
	unsigned char crc = 0;
	unsigned char c = 0;

	i = 5 * 8;
	do {
	    if (r4 & 0x80) c = *buf++;
	    crc = crc << 1;

	    if (crc & 0x80) crc ^= 9;
	    if (c & (r4>>24)) crc ^= 9;
	    r4 = (r4 >> 1) | (r4 << 31);
  	} while (--i > 0);

	//crc = (crc << 1) | 1;

  return crc;
}

#ifdef DEBUG_RESP
void debugMmcResp( unsigned char cmd, unsigned char *resp ) __attribute__ ((section (".data")));
void debugMmcResp( unsigned char cmd, unsigned char *resp )
{
	char temp[44];
	neo2_post_sd();
	snprintf(temp, 40, "CMD%u: %02X %02X %02X %02X %02X %02X", cmd, resp[0], resp[1], resp[2], resp[3], resp[4], resp[5]);
	gCursorX = 0;
	gCursorY++;
	if (gCursorY > 27)
		gCursorY = 0;
	put_str(temp, 0);
	delay(60);
	neo2_pre_sd();
}
#else
#define debugMmcResp(x, y)
#endif

#ifdef DEBUG_PRINT
void debugMmcPrint( char *str ) __attribute__ ((section (".data")));
void debugMmcPrint( char *str )
{
	char temp[44];
	neo2_post_sd();
	snprintf(temp, 40, "%s", str);
	gCursorX = 0;
	gCursorY++;
	if (gCursorY > 27)
		gCursorY = 0;
	put_str(temp, 0);
	delay(60);
	neo2_pre_sd();
}
#else
#define debugMmcPrint(x)
#endif

void sendMmcCmd( unsigned char cmd, unsigned int arg ) __attribute__ ((section (".data")));
void sendMmcCmd( unsigned char cmd, unsigned int arg )
{
	pkt[0]=cmd|0x40; 						// b7 = 0 => start bit, b6 = 1 => host command
	pkt[1]=(unsigned char)(arg >> 24);
	pkt[2]=(unsigned char)(arg >> 16);
	pkt[3]=(unsigned char)(arg >> 8);
	pkt[4]=(unsigned char)(arg);
	pkt[5]=( crc7(pkt) << 1 ) | 1; 			// b0 = 1 => stop bit
//	wrMmcCmdByte(0xFF);
//	wrMmcCmdByte(0xFF);
	wrMmcCmdByte( pkt[0] );
	wrMmcCmdByte( pkt[1] );
	wrMmcCmdByte( pkt[2] );
	wrMmcCmdByte( pkt[3] );
	wrMmcCmdByte( pkt[4] );
	wrMmcCmdByte( pkt[5] );
}

BOOL recvMmcCmdResp( unsigned char *resp, unsigned int len, int cflag ) __attribute__ ((section (".data")));
BOOL recvMmcCmdResp( unsigned char *resp, unsigned int len, int cflag )
{
    unsigned int i, j;
	unsigned char *r = resp;

    for (i=0; i<1024; i++)
	{
		// wait on start bit
        if(rdMmcCmdBit()==0)
		{
            *r++ = rdMmcCmdBits(7);
            len--;
            for(j=0; j<len; j++)
            	*r++ = rdMmcCmdByte();

			if (cflag)
				wrMmcCmdByte(0xFF);		// 8 cycles to complete the operation so clock can halt
			debugMmcResp(pkt[0]&0x3F, resp);
    		return TRUE;
        }
    }

	debugMmcResp(pkt[0]&0x3F, resp);
    return FALSE;
}

BOOL sdReadSingleBlock( unsigned char *buf, unsigned int addr ) __attribute__ ((section (".data")));
BOOL sdReadSingleBlock( unsigned char *buf, unsigned int addr )
{
	int i = 1024 * 8;
	unsigned char resp[R1_LEN];

	debugMmcPrint("Read starting");

	sendMmcCmd(17, addr);				// READ_SINGLE_BLOCK
	if (!(cardType & 0x8000))
		if (!recvMmcCmdResp(resp, R1_LEN, 0) || (resp[0] != 17))
			return FALSE;

	while ((rdMmcDatBit4()&1) != 0)
		if (i-- <= 0)
		{
			debugMmcPrint("Timeout");
			return FALSE;				// timeout on start bit
		}
#if 0
	for (i=0; i<512; i++)
		buf[i] = rdMmcDatByte4();

	// Clock out CRC
	for (i=0; i<8; i++)
		rdMmcDatByte4();

	// Clock out end bit
	rdMmcDatBit4();
#else
	neo2_recv_sd(buf);
#endif
	wrMmcCmdByte(0xFF);					// 8 cycles to complete the operation so clock can halt

	debugMmcPrint("Read done");
	return TRUE;
}

DSTATUS sdInit(void) __attribute__ ((section (".data")));
DSTATUS sdInit(void)
{
	int i;
	unsigned short rca;
	unsigned char resp[R2_LEN];			// R2 is largest response

	for( i = 0; i < 80; i++ )
		wrMmcCmdBit(1);

	sendMmcCmd(0, 0);					// GO_IDLE_STATE
	wrMmcCmdByte(0xFF);					// 8 cycles to complete the operation so clock can halt

	sendMmcCmd(8, 0x1AA);				// SEND_IF_COND
	if (recvMmcCmdResp(resp, R7_LEN, 1))
	{
		if ((resp[0] == 8) && (resp[3] == 1) && (resp[4] == 0xAA))
			cardType |= 2;				// V2 and/or HC card
		else
			return STA_NOINIT;			// unusable
	}

	for (i=0; i<64; i++)
	{
		sendMmcCmd(55, 0xFFFF);			// APP_CMD
		if (recvMmcCmdResp(resp, R1_LEN, 1) && (resp[4] & 0x20))
		{
			sendMmcCmd(41, (cardType & 2) ? 0x40300000 : 0x00300000);	// SD_SEND_OP_COND
			if (recvMmcCmdResp(resp, R3_LEN, 1) && (resp[0] == 0x3F) && (resp[1] & 0x80))
			{
				if (resp[1] & 0x40)
					cardType |= 1;		// HC card
				if (!(resp[2] & 0x30))
					return STA_NOINIT;	// unusable
				break;
			}
		}
	}
	if (i == 64)
	{
		// timed out
		return STA_NOINIT;				// unusable
	}

	sendMmcCmd(2, 0xFFFFFFFF);			// ALL_SEND_CID
	if (!recvMmcCmdResp(resp, R2_LEN, 1) || (resp[0] != 0x3F))
		return STA_NOINIT;				// unusable

	sendMmcCmd(3, 1);					// SEND_RELATIVE_ADDR
	if (!recvMmcCmdResp(resp, R6_LEN, 1) || (resp[0] != 3))
		return STA_NOINIT;				// unusable
	rca = (resp[1]<<8) | resp[2];

	sendMmcCmd(7, (((rca>>8)&0xFF)<<24) | ((rca&0xFF)<<16) | 0xFFFF); // SELECT_DESELECT_CARD
	if (!recvMmcCmdResp(resp, R1_LEN, 1) || (resp[0] != 7))
		return STA_NOINIT;				// unusable

	sendMmcCmd(55, (((rca>>8)&0xFF)<<24) | ((rca&0xFF)<<16) | 0xFFFF); // APP_CMD
	if (!recvMmcCmdResp(resp, R1_LEN, 1) || !(resp[4] & 0x20))
		return STA_NOINIT;				// unusable
	sendMmcCmd(6, 2);					// SET_BUS_WIDTH (to 4 bits)
	if (!recvMmcCmdResp(resp, R1_LEN, 1) || (resp[0] != 6))
		return STA_NOINIT;				// unusable

	return 0;
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (void) __attribute__ ((section (".data")));
DSTATUS disk_initialize (void)
{
	int i;
	DSTATUS result;

	for (i=0; i<CACHE_SIZE; i++)
		sec_tags[i] = 0xFFFFFFFF;		// invalidate cache entry
	sec_last = 0xFFFFFFFF;				// no uncached sector read

	cardType &= 0x8000;					// keep funky flag passed from get_sd_directory()

	neo2_pre_sd();

	result = sdInit();

#ifdef	DEBUG_PRINT
	if (result)
		debugMmcPrint("Card unusable");
	else
		debugMmcPrint("Card ready");
#endif

	neo2_post_sd();
	return result;
}

/*-----------------------------------------------------------------------*/
/* Read Partial Sector                                                   */
/*-----------------------------------------------------------------------*/

DRESULT disk_readp (void* dest, DWORD sector, WORD sofs, WORD count) __attribute__ ((section (".data")));
DRESULT disk_readp (
	void* dest,			/* Pointer to the destination object */
	DWORD sector,		/* Sector number (LBA) */
	WORD sofs,			/* Offset in the sector (0..511) */
	WORD count			/* Byte count (1..512), bit15:destination flag */
)
{
	int ix;
	DRESULT res;
	BYTE (*f)(BYTE);
	unsigned char *buf;

	if (!(count & 0x7FFF) || ((count & 0x7FFF) > 512))
		return RES_PARERR;

	if ((count & 0x7FFF) > 40)
	{
		// too big for anything but a file read, don't fetch to cache
		//if ((sec_last > sector) || ((sector - sec_last) > 7))
		if (sec_last != sector)
		{
			// read sector
			neo2_pre_sd();
			debugMmcPrint("Uncached read");
			if (!sdReadSingleBlock(sec_buf, sector << ((cardType & 1) ? 0 : 9)))
			{
				// read failed, retry once
                if (!sdReadSingleBlock(sec_buf, sector << ((cardType & 1) ? 0 : 9)))
                {
                    neo2_post_sd();
                    sec_last = 0xFFFFFFFF;
                    return RES_ERROR;
                }
			}
			neo2_post_sd();
			sec_last = sector;
		}
		buf = sec_buf;
		//buf = &sec_buf[(sector-sec_last)<<9];
	}
	else
	{
		ix = sector & (CACHE_SIZE - 1);	// really simple hash
		if (sec_tags[ix] != sector)
		{
			// sector not in cache - fetch it
			neo2_pre_sd();
			debugMmcPrint("Cached read");
			if (!sdReadSingleBlock(&sec_cache[ix<<9], sector << ((cardType & 1) ? 0 : 9)))
			{
				// read failed, retry once
                if (!sdReadSingleBlock(&sec_cache[ix<<9], sector << ((cardType & 1) ? 0 : 9)))
                {
                    neo2_post_sd();
                    sec_tags[ix] = 0xFFFFFFFF;
                    return RES_ERROR;
                }
			}
			neo2_post_sd();
			sec_tags[ix] = sector;
		}
		buf = &sec_cache[ix<<9];
	}

	if (count & 0x8000)
	{
		// streaming file mode
		f = dest;
		for (ix=0; ix<(count&0x7FFF); ix++)
		{
			res = f(buf[sofs + ix]);
			if (res != RES_OK)
				return res;
		}
	}
	else
		memcpy(dest, &buf[sofs], count);

#if 0
	// print first 20 bytes of data read
	{
		char *hex = "0123456789ABCDEF";
		char temp[44];
		int i;
		int c = (count < 20) ? count : 20;
		unsigned char *d = (unsigned char *)dest;

		for (i=0; i<c; i++)
		{
			temp[i*2] = hex[d[i]>>4];
			temp[i*2+1] = hex[d[i]&15];
		}
		temp[i*2] = 0;
		gCursorX = 0;
		gCursorY++;
		if (gCursorY > 27)
			gCursorY = 0;
		put_str(temp, 0);
		delay(120);
	}
#endif

#if 0
	// print entire sector and crc
	{
		char *hex = "0123456789ABCDEF";
		char temp[44];
		int i, j;
		unsigned char *d = buf;

		gCursorY = 0;
		for (j=0; j<26; j++)
		{
			for (i=0; i<20; i++, d++)
			{
				temp[i*2] = hex[*d>>4];
				temp[i*2+1] = hex[*d&15];
			}
			temp[i*2] = 0;
			gCursorX = 0;
			gCursorY++;
			if (gCursorY > 27)
				gCursorY = 0;
			put_str(temp, 0);
		}
		delay(1200);
	}
#endif

	return RES_OK;
}

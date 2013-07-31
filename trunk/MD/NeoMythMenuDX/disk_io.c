#include <string.h>
#include <stdio.h>
#include <diskio.h>

#define CACHE_SIZE 4                    /* number sectors in cache */
#define R1_LEN (48/8)
#define R2_LEN (136/8)
#define R3_LEN (48/8)
#define R6_LEN (48/8)
#define R7_LEN (48/8)
#define INIT_RETRIES (64)
#define RESP_TIME_I (64)
#define RESP_TIME_R (1024/2)
#define RESP_TIME_W (1024*4)
//#define DEBUG_PRINT
//#define DEBUG_RESP

typedef volatile unsigned short int vu16;
typedef unsigned int u32;

extern int set_sr(int sr);
extern void neo2_recv_sd(unsigned char *buf);
extern int neo2_recv_sd_multi(unsigned char *buf, int count);
extern void neo2_pre_sd(void);
extern void neo2_post_sd(void);
// for debug
extern void delay(int count);
extern void put_str(char *str, int fcolor);
extern short int gCursorX;              /* range is 0 to 63 (only 0 to 39 onscreen) */
extern short int gCursorY;              /* range is 0 to 31 (only 0 to 27 onscreen) */
unsigned short cardType;                /* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */
unsigned char pkt[6];                   /* command packet */
unsigned int num_sectors;
unsigned int sec_tags[CACHE_SIZE];
unsigned char __attribute__((aligned(16))) sec_cache[CACHE_SIZE*512 + 8];
unsigned char __attribute__((aligned(16))) sec_buf[520]; /* for uncached reads */
unsigned char sd_csd[R2_LEN];
short do_init = 0;
static int respTime = RESP_TIME_R;


/*-----------------------------------------------------------------------*/
/*                             support code                              */
/*-----------------------------------------------------------------------*/

void sd_op_delay() __attribute__ ((section (".data")));
void sd_op_delay()
{
    short i;

    for (i=0; i<16; i++)
    {
        asm("nop\n");
    }
}

void wrMmcCmdBit( unsigned int bit ) __attribute__ ((section (".data")));
void wrMmcCmdBit( unsigned int bit )
{
    unsigned short data;
    unsigned int addr =
        //( 1<<24 ) |        // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        ( 1<<15 ) |            // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( (bit&1)<<13 ) |    // cmd value
        ( 1<<12 ) |            // d3 value
        ( 1<<11 ) |            // d2 value
        ( 1<<10 ) |            // d1 value
        ( 1<<9 );            // d0 value
    data = *(vu16*)(addr);
    if (do_init) sd_op_delay();
}

void wrMmcCmdByte( unsigned int byte ) __attribute__ ((section (".data")));
void wrMmcCmdByte( unsigned int byte )
{
    int i;
    for( i = 0; i < 8 ; i++ )
        wrMmcCmdBit( (byte>>(7-i)) & 1 );
}

#if 1
void wrMmcDatBit( unsigned int bit ) __attribute__ ((section (".data")));
void wrMmcDatBit( unsigned int bit )
{
    unsigned short data;
    unsigned int addr =
        //( 1<<24 ) |        // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        ( 1<<15 ) |            // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        ( 1<<12 ) |            // d3 value
        ( 1<<11 ) |            // d2 value
        ( 1<<10 ) |            // d1 value
        ( (bit&1)<<9 );        // d0 value
    data = *(vu16*)(addr);
    if (do_init) sd_op_delay();
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
        //( 1<<24 ) |        // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        ( 1<<15 ) |            // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        ( 1<<12 ) |            // d3
        ( 1<<11 ) |            // d2
        ( 1<<10 ) |            // d1
        ( 1<<9 ) |            // d0
        ( 1<<7 );            // cmd input mode
    data = *(vu16*)(addr);
    if (do_init) sd_op_delay();
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
        //( 1<<24 ) |        // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        //( 1<<15 ) |        // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        ( 1<<12 ) |            // d3
        ( 1<<11 ) |            // d2
        ( 1<<10 ) |            // d1
        //( 1<<9 ) |        // d0
        //( 1<<6 ) |        // d3-d1 input (only needed in 4 bit mode)
        ( 1<<5);            // d0 input
    data = *(vu16*)(addr);
    if (do_init) sd_op_delay();
    return data & 1;
}

unsigned int rdMmcDatBit4() __attribute__ ((section (".data")));
unsigned int rdMmcDatBit4()
{
    unsigned short data;
    unsigned int addr =
        //( 1<<24 ) |        // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        //( 1<<16 ) |        // d1-d3 remap
        //( 1<<15 ) |        // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        //( 1<<12 ) |        // d3
        //( 1<<11 ) |        // d2
        //( 1<<10 ) |        // d1
        //( 1<<9 ) |        // d0
        ( 1<<6 ) |            // d3-d1 input (only needed in 4 bit mode)
        ( 1<<5);            // d0 input
    data = *(vu16*)(addr);
    return data & 15;
}

#if 1
void wrMmcDatBit4( unsigned char dat ) __attribute__ ((section (".data")));
void wrMmcDatBit4( unsigned char dat )
{
    unsigned short data;
    unsigned int addr =
        //( 1<<24 ) |        // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        ( 1<<15 ) |            // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        ( (dat&15)<<9 );    // d3-d0 value
    data = *(vu16*)(addr);
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

#ifdef DEBUG_PRINT
void debugPrint( char *str ) __attribute__ ((section (".data")));
void debugPrint( char *str )
{
    char temp[44];
    static int dbgX = 0;
    static int dbgY = 0;
    neo2_post_sd();
    snprintf(temp, 40, "%s", str);
    gCursorX = dbgX;
    gCursorY = dbgY;
    dbgY++;
    if (dbgY > 27)
    {
        dbgY = 0;
        dbgX = (dbgX + 8) % 40;
    }
    put_str(temp, 0);
    delay(20);
    neo2_pre_sd();
}
#else
#define debugPrint(a)
#endif

void sendMmcCmd( unsigned char cmd, unsigned int arg ) __attribute__ ((section (".data")));
void sendMmcCmd( unsigned char cmd, unsigned int arg )
{
    pkt[0]=cmd|0x40;                         // b7 = 0 => start bit, b6 = 1 => host command
    pkt[1]=(unsigned char)(arg >> 24);
    pkt[2]=(unsigned char)(arg >> 16);
    pkt[3]=(unsigned char)(arg >> 8);
    pkt[4]=(unsigned char)(arg);
    pkt[5]=( crc7(pkt) << 1 ) | 1;             // b0 = 1 => stop bit
//    wrMmcCmdByte(0xFF);
//    wrMmcCmdByte(0xFF);
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

    resp[0] = 0;
    for (i=0; i<respTime; i++)
    {
        // wait on start bit
        if(rdMmcCmdBit()==0)
        {
            *r++ = rdMmcCmdBits(7);
            len--;
            for(j=0; j<len; j++)
                *r++ = rdMmcCmdByte();

            if (cflag)
                wrMmcCmdByte(0xFF);        // 8 cycles to complete the operation so clock can halt

            respTime = RESP_TIME_R;
            debugMmcResp(pkt[0]&0x3F, resp);
            return TRUE;
        }
    }

    respTime = RESP_TIME_R;
    debugMmcResp(pkt[0]&0x3F, resp);
    return FALSE;
}

BOOL sdReadSingleBlock( unsigned char *buf, unsigned int addr ) __attribute__ ((section (".data")));
BOOL sdReadSingleBlock( unsigned char *buf, unsigned int addr )
{
    int i = 1024 * 8;
    unsigned char resp[R1_LEN];

    debugMmcPrint("Read starting");

    sendMmcCmd(17, addr);                // READ_SINGLE_BLOCK
    if (!(cardType & 0x8000))
        if (!recvMmcCmdResp(resp, R1_LEN, 0) || (resp[0] != 17))
            return FALSE;

    while ((rdMmcDatBit4()&1) != 0)
        if (i-- <= 0)
        {
            debugMmcPrint("Timeout");
            return FALSE;                // timeout on start bit
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
    wrMmcCmdByte(0xFF);                    // 8 cycles to complete the operation so clock can halt

    debugMmcPrint("Read SD block");
    return TRUE;
}

BOOL sdReadStartMulti( unsigned int addr ) __attribute__ ((section (".data")));
BOOL sdReadStartMulti( unsigned int addr )
{
    unsigned char resp[R1_LEN];

    debugMmcPrint("Read starting");

    sendMmcCmd(18, addr);               // READ_MULTIPLE_BLOCK
    if (!(cardType & 0x8000))
        if (!recvMmcCmdResp(resp, R1_LEN, 0) || (resp[0] != 18))
            return FALSE;

    return TRUE;
}

BOOL sdReadStopMulti( void ) __attribute__ ((section (".data")));
BOOL sdReadStopMulti( void )
{
    unsigned char resp[R1_LEN];

    debugMmcPrint("Read stopping");

    sendMmcCmd(12, 0);                  // STOP_TRANSMISSION
    if (!(cardType & 0x8000))
        if (!recvMmcCmdResp(resp, R1_LEN, 0) || (resp[0] != 12))
            return FALSE;

    return TRUE;
}

void sdCrc16(unsigned char *p_crc, unsigned char *data, int len) __attribute__ ((section (".data")));
void sdCrc16(unsigned char *p_crc, unsigned char *data, int len)
{
    int i;
    unsigned char nybble;

    unsigned long long poly = 0x0001000000100001LL;
    unsigned long long crc = 0;
    unsigned long long n_crc; // This can probably be unsigned int

    // Load crc from array
    for (i = 0; i < 8; i++)
    {
        crc <<= 8;
        crc |= p_crc[i];
    }

    for (i = 0; i < (len * 2); i++)
    {
        if (i & 1)
            nybble = (data[i >> 1] & 0x0F);
        else
            nybble = (data[i >> 1] >> 4);

        n_crc = (crc >> (15 * 4));
        crc <<= 4;
        if ((nybble ^ n_crc) & 1) crc ^= (poly << 0);
        if ((nybble ^ n_crc) & 2) crc ^= (poly << 1);
        if ((nybble ^ n_crc) & 4) crc ^= (poly << 2);
        if ((nybble ^ n_crc) & 8) crc ^= (poly << 3);
    }

    // Output crc to array
    for (i = 7; i >= 0; i--)
    {
        p_crc[i] = crc;
        crc >>= 8;
    }
}

BOOL sendSdWriteBlock4( unsigned char *buf, unsigned char *crcbuf ) __attribute__ ((section (".data")));
BOOL sendSdWriteBlock4( unsigned char *buf, unsigned char *crcbuf )
{
    int i;

    //for (i = 0; i < 20; i++)
    wrMmcDatByte4(0xFF);                // minimum 2 P bits
    wrMmcDatBit4(0);                    // write start bit

    // write out data and crc bytes
    for (i=0; i<512; i++)
        wrMmcDatByte4( buf[i] );

    for (i=0; i<8; i++)
        wrMmcDatByte4( crcbuf[i] );

    wrMmcDatBit4(15);                   // write end bit

    rdMmcDatByte4();                    // clock out two bits on D0

#if 1
    // spec says the CRC response from the card appears now...
    //   start bit, three CRC response bits, end bit
    // this is followed by a busy response while the block is written
    //   start bit, variable number of 0 bits, end bit

    if( rdMmcDatBit4() & 1 )
    {
        debugPrint("No start bit after write data block.");
        return FALSE;
    }
    else
    {
        unsigned char crc_stat = 0;

        for(i=0; i<3; i++)
        {
            crc_stat <<= 1;
            crc_stat |= (rdMmcDatBit4() & 1);   // read crc status on D0
        }
        rdMmcDatBit4();                 // end bit

        // If CRC Status is not positive (010b) return error
        if(crc_stat!=2)
        {
            debugPrint("CRC error on write data block.");
            return FALSE;
        }

        // at this point the card is definetly cooperating so wait for the start bit
        while( rdMmcDatBit4() & 1 );

        // Do not time out? Writes take an unpredictable amount of time to complete.
        for(i=(1024*128);i>0;i--)
        {
            if((rdMmcDatBit4()&1) != 0 )
                break;
        }
        // check for busy timeout
        if(i==0)
        {
            debugPrint("Write data block busy timeout.");
            return FALSE;
        }
    }
#else
    // ignore the spec - lets just clock out some bits and wait
    // for the busy to clear...it works well on many non working cards

    rdMmcDatByte4();                    // clock out two bits on D0
    rdMmcDatByte4();                    // clock out two bits on D0

    while(rdMmcDatBit4() & 1) ;         // wait for DAT line to go low, indicating busy writing
    while((rdMmcDatBit4()&1) == 0) ;    // wait for DAT line high, indicating finished writing block
#endif

    wrMmcCmdByte(0xFF);                    // 8 cycles to complete the operation so clock can halt
    debugMmcPrint("Write done");
    return TRUE;
}

BOOL sdWriteSingleBlock( unsigned char *buf, unsigned int addr ) __attribute__ ((section (".data")));
BOOL sdWriteSingleBlock( unsigned char *buf, unsigned int addr )
{
    unsigned char resp[R1_LEN];
    unsigned char crcbuf[8];

    //memset(crcbuf, 0, 8); // crc in = 0
    *(unsigned int*)&crcbuf[0]=0;
    *(unsigned int*)&crcbuf[4]=0;
    sdCrc16(crcbuf, buf, 512);          // Calculate CRC16

    sendMmcCmd( 24, addr );
    respTime = RESP_TIME_W;
    if (recvMmcCmdResp(resp, R1_LEN, 0) && (resp[0] == 24))
        return sendSdWriteBlock4(buf, crcbuf);

    debugMmcResp(24, resp);
    return FALSE;
}

BOOL sdInit(void) __attribute__ ((section (".data")));
BOOL sdInit(void)
{
    int i;
    unsigned short rca;
    unsigned char resp[R2_LEN];         // R2 is largest response

    do_init = 1;

    for( i = 0; i < 64; i++ )
        wrMmcCmdBit(1);

    sendMmcCmd(0, 0);                   // GO_IDLE_STATE
    wrMmcCmdByte(0xFF);                 // 8 cycles to complete the operation so clock can halt

    sendMmcCmd(8, 0x1AA);               // SEND_IF_COND
    respTime = RESP_TIME_I;             // init response time (extra short)
    if (recvMmcCmdResp(resp, R7_LEN, 1))
    {
        if ((resp[0] == 8) && (resp[3] == 1) && (resp[4] == 0xAA))
            cardType |= 2;              // V2 and/or HC card
        else
            return FALSE;               // unusable
    }

    for (i=0; i<INIT_RETRIES; i++)
    {
        sendMmcCmd(55, 0xFFFF);         // APP_CMD
        respTime = RESP_TIME_I;
        if (recvMmcCmdResp(resp, R1_LEN, 1))
        {
            if (resp[4] & 0x20)
            {
                sendMmcCmd(41, (cardType & 2) ? 0x40300000 : 0x00300000);    // SD_SEND_OP_COND
                if (recvMmcCmdResp(resp, R3_LEN, 1) && (resp[0] == 0x3F) && (resp[1] & 0x80))
                {
                    if (resp[1] & 0x40)
                        cardType |= 1;      // HC card
                    if (!(resp[2] & 0x30))
                        return FALSE;       // unusable
                    break;
                }
            }
        }
        else
            return FALSE;               // no card
    }
    if (i == INIT_RETRIES)
    {
        // timed out
        return FALSE;                   // unusable
    }

    sendMmcCmd(2, 0xFFFFFFFF);          // ALL_SEND_CID
    if (!recvMmcCmdResp(resp, R2_LEN, 1) || (resp[0] != 0x3F))
        return FALSE;                   // unusable

    sendMmcCmd(3, 1);                   // SEND_RELATIVE_ADDR
    if (!recvMmcCmdResp(resp, R6_LEN, 1) || (resp[0] != 3))
        return FALSE;                   // unusable

    rca = (resp[1]<<8) | resp[2];
#if 1
    sendMmcCmd(9, (((rca>>8)&0xFF)<<24) | ((rca&0xFF)<<16) | 0xFFFF); // SEND_CSD
    recvMmcCmdResp(sd_csd, R2_LEN, 1);
#endif

    sendMmcCmd(7, (((rca>>8)&0xFF)<<24) | ((rca&0xFF)<<16) | 0xFFFF); // SELECT_DESELECT_CARD
    if (!recvMmcCmdResp(resp, R1_LEN, 1) || (resp[0] != 7))
        return FALSE;                   // unusable

    sendMmcCmd(55, (((rca>>8)&0xFF)<<24) | ((rca&0xFF)<<16) | 0xFFFF); // APP_CMD
    if (!recvMmcCmdResp(resp, R1_LEN, 1) || !(resp[4] & 0x20))
        return FALSE;                   // unusable

    sendMmcCmd(6, 2);                   // SET_BUS_WIDTH (to 4 bits)
    if (!recvMmcCmdResp(resp, R1_LEN, 1) || (resp[0] != 6))
        return FALSE;                   // unusable

    do_init = 0;
    return TRUE;
}

/*-----------------------------------------------------------------------*/
/* Initialize SD(HC) Card                                                */
/*-----------------------------------------------------------------------*/

DSTATUS MMC_disk_initialize (void) __attribute__ ((section (".data")));
DSTATUS MMC_disk_initialize (void)
{
    int i;
    BOOL result;

    for (i=0; i<CACHE_SIZE; i++)
        sec_tags[i] = 0xFFFFFFFF;       // invalidate cache entry

    cardType &= 0x8000;                 // keep funky flag

    neo2_pre_sd();
    result = sdInit();
    neo2_post_sd();

    if (!result)
    {
        cardType = 0xFFFF;
#ifdef DEBUG_PRINT
        debugMmcPrint("Card unusable");
#endif
        return STA_NODISK;
    }

#ifdef    DEBUG_PRINT
    if (cardType & 1)
        debugMmcPrint("SDHC Card ready");
    else
        debugMmcPrint("SD Card ready");
#endif

    if ((sd_csd[1] & 0xC0) == 0)
    {
        // CSD type 1 - version 1.x cards, and version 2.x standard capacity
        //
        //    C_Size      - 12 bits - [73:62]
        //    C_Size_Mult -  3 bits - [49:47]
        //    Read_Bl_Len -  4 bits - [83:80]
        //
        //    Capacity (bytes) = (C_Size + 1) * ( 2 ^ (C_Size_Mult + 2)) * (2 ^ Read_Bl_Len)

        unsigned int C_Size      = ((sd_csd[7] & 0x03) << 10) | (sd_csd[8] << 2) | ((sd_csd[9] >>6) & 0x03);
        unsigned int C_Size_Mult = ((sd_csd[10] & 0x03) << 1) | ((sd_csd[11] >> 7) & 0x01);
        unsigned int Read_Bl_Len = sd_csd[6] & 0x0f;

        num_sectors = ((C_Size + 1) * (1 << (C_Size_Mult + 2)) * ((1 << Read_Bl_Len)) / 512);
    }
    else
    {
        // CSD type 2 - version 2.x high capacity
        //
        //    C_Size      - 22 bits - [69:48]
        //
        //    Capacity (bytes) = (C_Size + 1) * 1024 * 512

        num_sectors = (((sd_csd[8] & 0x3F) << 16) | (sd_csd[9] << 8) | sd_csd[10]) * 1024;
    }

    return 0;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS MMC_disk_status (void);
DSTATUS MMC_disk_status (void)
{
    return (cardType != 0xFFFF) ? 0 : STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT MMC_disk_read_multi (BYTE *buff, DWORD sector, UINT count) __attribute__ ((section (".data")));
DRESULT MMC_disk_read_multi (
    BYTE *buff,
    DWORD sector,
    UINT count
)
{
        sector <<= ((cardType & 1) ? 0 : 9);
        neo2_pre_sd();

        if (!sdReadStartMulti(sector))
        {
                if (!sdReadStartMulti(sector))
                {
                        neo2_post_sd();
                        return RES_ERROR;
                }

                if (!neo2_recv_sd_multi(buff, count))
                {
                        neo2_post_sd();
                        return RES_ERROR;
                }

                sdReadStopMulti();
                neo2_post_sd();
                return RES_OK;
        }

        if (!neo2_recv_sd_multi(buff, count))
        {
                if (!sdReadStartMulti(sector))
                {
                        neo2_post_sd();
                        return RES_ERROR;
                }

                if (!neo2_recv_sd_multi(buff, count))
                {
                        neo2_post_sd();
                        return RES_ERROR;
                }
        }

        sdReadStopMulti();
        neo2_post_sd();
        return RES_OK;
}



DRESULT MMC_disk_read (BYTE *buff, DWORD sector, BYTE count) __attribute__ ((section (".data")));
DRESULT MMC_disk_read (
    BYTE *buff,            /* Data buffer to store read data */
    DWORD sector,        /* Sector address (LBA) */
    BYTE count            /* Number of sectors to read (1..255) */
)
{
    unsigned int ix;

#if 0
    char temp[40];
    sprintf(temp, "%d:%d > %x", (int)sector, count, (unsigned int)buff);
    debugPrint(temp);
#endif

    if (count == 1)
    {
        ix = sector & (CACHE_SIZE - 1);    // really simple hash
        if (sec_tags[ix] != sector)
        {
            // sector not in cache - fetch it
            neo2_pre_sd();
            if (!sdReadSingleBlock(&sec_cache[ix*512], sector << ((cardType & 1) ? 0 : 9)))
            {
                // read failed, retry once
                if (!sdReadSingleBlock(&sec_cache[ix*512], sector << ((cardType & 1) ? 0 : 9)))
                {
                    // read failed
                    neo2_post_sd();
                    sec_tags[ix] = 0xFFFFFFFF;
                    //debugPrint("Read failed!");
                    return RES_ERROR;
                }
            }
            neo2_post_sd();
            sec_tags[ix] = sector;
        }
        memcpy(buff, &sec_cache[ix*512], 512);
    }
    else
    {
#if 0
        for (ix=0; ix<count; ix++)
        {
            neo2_pre_sd();
            if (!sdReadSingleBlock(sec_buf, (sector + ix) << ((cardType & 1) ? 0 : 9)))
            {
                // read failed, retry once
                if (!sdReadSingleBlock(sec_buf, (sector + ix) << ((cardType & 1) ? 0 : 9)))
                {
                    // read failed
                    neo2_post_sd();
                    //debugPrint("Read failed!");
                    return RES_ERROR;
                }
            }
            neo2_post_sd();
            memcpy((void *)((unsigned int)buff + ix*512), sec_buf, 512);
        }
#else
        neo2_pre_sd();
        if (!sdReadStartMulti(sector << ((cardType & 1) ? 0 : 9)))
        {
            // read failed, retry once
            if (!sdReadStartMulti(sector << ((cardType & 1) ? 0 : 9)))
            {
                // start multiple sector read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
            if (!neo2_recv_sd_multi(buff, count))
            {
                // read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
        }
    else if (!neo2_recv_sd_multi(buff, count))
    {
            // read failed, retry once
            if (!sdReadStartMulti(sector << ((cardType & 1) ? 0 : 9)))
            {
                // start multiple sector read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
            if (!neo2_recv_sd_multi(buff, count))
            {
                // read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
    }

        sdReadStopMulti();
        neo2_post_sd();
#endif
    }

#if 0
    // print entire sector and crc
    {
        char *hex = "0123456789ABCDEF";
        char temp[44];
        int i, j;
        unsigned char *d = (unsigned char *)buff;

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

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT MMC_disk_write (const BYTE *buff, DWORD sector, BYTE count) __attribute__ ((section (".data")));
DRESULT MMC_disk_write (
    const BYTE *buff,    /* Data to be written */
    DWORD sector,        /* Sector address (LBA) */
    BYTE count            /* Number of sectors to write (1..255) */
)
{
    unsigned int ix, iy;

    for (ix=0; ix<count; ix++)
    {
        iy = (sector + ix) & (CACHE_SIZE - 1);    // really simple hash
        if (sec_tags[iy] == (sector + ix))
            sec_tags[iy] = 0xFFFFFFFF;
        memcpy(sec_buf, (void *)((unsigned int)buff + ix*512), 512);
        neo2_pre_sd();
        if (!sdWriteSingleBlock(sec_buf, (sector + ix) << ((cardType & 1) ? 0 : 9)))
        {
            // write failed, retry once
            if (!sdWriteSingleBlock(sec_buf, (sector + ix) << ((cardType & 1) ? 0 : 9)))
            {
                neo2_post_sd();
                return RES_ERROR;
            }
        }
        neo2_post_sd();
    }

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT MMC_disk_ioctl (BYTE ctrl, void *buff);
DRESULT MMC_disk_ioctl (
    BYTE ctrl,        /* Control code */
    void *buff        /* Buffer to send/receive control data */
)
{
    switch (ctrl)
    {
        case GET_SECTOR_COUNT:
            *(unsigned int *)buff = num_sectors;
            return num_sectors ? RES_OK : RES_ERROR;
        case GET_SECTOR_SIZE:
            *(unsigned short *)buff = 512;
            return RES_OK;
        case GET_BLOCK_SIZE:
            *(unsigned int *)buff = 1;
            return RES_OK;
        case CTRL_SYNC:
            return RES_OK;
        case CTRL_POWER:
        case CTRL_LOCK:
        case CTRL_EJECT:
        case MMC_GET_TYPE:
        case MMC_GET_CSD:
        case MMC_GET_CID:
        case MMC_GET_OCR:
        case MMC_GET_SDSTAT:
        default:
            return RES_PARERR;
    }
}

/* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */
/* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */
DWORD get_fattime (void)
{
    return 0;
}

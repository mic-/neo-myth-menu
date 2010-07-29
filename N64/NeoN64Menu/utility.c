#include <stdio.h>
#include <stdlib.h>

extern void debugText(char *msg, int x, int y, int d);

// CRC code adapted from DaedalusX64 emulator

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

unsigned int CRC_Calculate(unsigned int crc, unsigned char* buf, unsigned int len)
{
    static unsigned int crc_table[256];
    static int make_crc_table = 1;

    if (make_crc_table)
    {
        unsigned int c, n;
        int k;
        unsigned int poly;
        const unsigned char p[] = { 0,1,2,4,5,7,8,10,11,12,16,22,23,26 };

        /* make exclusive-or pattern from polynomial (0xedb88320L) */
        poly = 0L;
        for (n = 0; n < sizeof(p)/sizeof(unsigned char); n++)
            poly |= 1L << (31 - p[n]);

        for (n = 0; n < 256; n++)
        {
            c = n;
            for (k = 0; k < 8; k++)
                c = c & 1 ? poly ^ (c >> 1) : c >> 1;
            crc_table[n] = c;
        }
        make_crc_table = 0;
    }

    if (buf == (void*)0) return 0L;

    crc = crc ^ 0xffffffffL;
    while (len >= 8)
    {
        DO8(buf);
        len -= 8;
    }
    if (len)
        do
        {
            DO1(buf);
        } while (--len);

    return crc ^ 0xffffffffL;
}

int get_cic(unsigned char *buffer)
{
    unsigned int crc;
    // figure out the CIC
    crc = CRC_Calculate(0, buffer, 1000);
    switch(crc)
    {
        case 0x303faac9:
        case 0xf0da3d50:
            return 1;
        case 0xf3106101:
            return 2;
        case 0xe7cd9d51:
            return 3;
        case 0x7ae65c9:
            return 5;
        case 0x86015f8f:
            return 6;
    }
    return 2;
}

int get_swap(unsigned char *buf)
{
    char temp[40];
    unsigned int val = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
    switch(val)
    {
        case 0x80371240:
            return 0;                       // no swapping needed
        case 0x37804012:
            return 1;                       // byte swapping needed
        case 0x12408037:
            return 2;                       // word swapping needed
        case 0x40123780:
            return 3;                       // long swapping needed
    }
    sprintf(temp, "unknown byte format: %08X", val);
    debugText(temp, 5, 2, 180);
    return 0;
}

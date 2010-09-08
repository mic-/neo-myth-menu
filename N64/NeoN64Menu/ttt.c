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

#ifndef _neo2_asm_
#define _neo2_asm_

extern void neo_xferto_psram(void *src, int pstart, int len);
//extern unsigned char crc7 (unsigned char *buf);
extern int neo2_recv_sd_multi(unsigned char *buf, int count);
#endif

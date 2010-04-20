#ifndef __UTIL_68K_INTERFACE_H__
#define __UTIL_68K_INTERFACE_H__

/*
	util68k lib - By conleon1988@gmail.com for ChillyWilly's DX myth menu
	http://code.google.com/p/neo-myth-menu/

	Special thanks to ChillyWilly for all the hints & support :D
*/

//WCHAR
extern int utility_wstrcmp(const unsigned short* ws1,const unsigned short* ws2);
extern int utility_wstrlen2(const unsigned short* s);
extern int utility_wstrlen(const unsigned short* s);
extern unsigned short* utility_wstrcpy(unsigned short* ws1,const unsigned short* ws2);
extern unsigned short* utility_c2wstrcpy(unsigned short* s1,const char* s2);
extern unsigned short* utility_wstrcat(unsigned short* ws1,const unsigned short* ws2);
extern unsigned short* utility_c2wstrcat(unsigned short* ws1,const char* ws2);
extern char* utility_w2cstrcpy(char* ws1,const unsigned short* ws2);
extern unsigned short* utility_getFileExtW(unsigned short* src);

//MEM - WCHAR


//CHAR
extern int utility_strlen(const char* s);
extern int utility_strlen2(const char* s);
extern int utility_isMultipleOf(int base,int n);
extern char* utility_getFileExt(char* src);

//MEM - CHAR
extern void utility_bmemcpy(const unsigned char* src,unsigned char* dst,int len);
extern void utility_wmemcpy(const unsigned char* src,unsigned char* dst,int len);
extern void utility_lmemcpy(const unsigned char* src,unsigned char* dst,int len);
extern void utility_bmemset(unsigned char* data,unsigned char c,int len);
extern void utility_wmemset(unsigned char* data,short int c,int len);
extern void utility_lmemset(unsigned char* data,int c,int len);

#endif



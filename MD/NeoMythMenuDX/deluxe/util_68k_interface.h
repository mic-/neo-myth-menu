#ifndef __UTIL_68K_INTERFACE_H__
#define __UTIL_68K_INTERFACE_H__

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
extern char* utility_getFileExt(char* src);
extern char* utility_strcpy(char* ws1,const char* ws2);
extern char* utility_strcat(char* s1,const char* s2);
extern char* utility_strncat(char* s1,const char* s2,int n);

//MEM - CHAR
extern void utility_memcpy(void* dst,const void* src,int len);
extern void utility_memset(void* dst,int c,int len);
extern void utility_memcpy16(void* dst,const void* src,int len);
extern void utility_memcpy_entry_block(void* dst,const void* src);
extern void utility_memset_psram(void* dst,int c,int len);

//MISC
extern int utility_isMultipleOf(int base,int n);
extern int utility_memcmp(const void* dst, const void* src, int cnt);
extern int utility_logtwo(int x);
#endif



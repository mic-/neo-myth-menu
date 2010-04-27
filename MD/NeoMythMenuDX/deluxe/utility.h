/*
	Copyright (c) 2010 - 2011 conleon1988 (conleon1988@gmail.com)

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _UTILITY_H_
#define _UTILITY_H_
#ifdef __cplusplus
extern "C" {
#endif 
#undef _LIB_UTIL_ADD_FLOAT_SUPPORT_
//LOW LEVEL UTILITY MODULE (C89 COMPATIBLE)
//v1.4

#define UTIL_Abs(__N__) (( __N__ < 0 )? -__N__ :__N__)
#define UTIL_Min(__A__,__B__) ( (__A__ < __B__) ? __A__ : __B__ )
#define UTIL_Max(__A__,__B__) ( (__A__ > __B__) ? __A__ : __B__ )
#define UTIL_Neg(__N__) (-__N__)

#define BASE_36 (36)
#define BASE_16 (16)
#define BASE_10 (10)
#define BASE_8 (8)

#ifndef NULL
    #define NULL ((void*)0)
#endif

//typedef unsigned long long QuadWord;

//Copy buffer
void UTIL_CopySafe(char* dst,const char* src,int len);
void UTIL_CopyUnsafe(char* dst,const char* src);

//Set memory
void UTIL_SetMemorySafe(char* dst,const char c,int len);
void UTIL_SetMemoryUnSafe(char* dst,const char c);

//String length using block jumps
int UTIL_StringLengthMemBlockConst(const char* str,const int blockLen);
int UTIL_StringLengthMemBlock(char* str,const int blockLen);

//String length
int UTIL_StringLengthConst(const char* str);
int UTIL_StringLength(char* str);

//find string
const char* UTIL_StringFindLastCharAnyCaseConst(const char* base,const char c);
const char* UTIL_StringFindLastCharConst(const char* base,const char c);
char* UTIL_StringFindLastChar(char* base,char c);
char* UTIL_StringFindLastCharAnyCase(char* base,char c);
const char* UTIL_StringFindCharAnyCaseConst(const char* base,const char c);
const char* UTIL_StringFindCharConst(const char* base,const char c);
char* UTIL_StringFindChar(char* base,char c);
char* UTIL_StringFindCharAnyCase(char* base,char c);
const char* UTIL_StringFindConst(const char* base,const char* sub);
char* UTIL_StringFind(char* base,char* sub);
const char* UTIL_StringFindAnyCaseConst(const char* base,const char* sub);
char* UTIL_StringFindAnyCase(char* base,char* sub);
const char* UTIL_StringFindLastConst(const char* base,const char* sub);
char* UTIL_StringFindLast(char* base,char* sub);
const char* UTIL_StringFindLastAnyCaseConst(const char* base,const char* sub);
char* UTIL_StringFindLastAnyCase(char* base,char* sub);
int UTIL_StringLengthFastLE32(const char* src);
int UTIL_StringLengthFastBE32(const char* src);
char* UTIL_StringAppend(char* dst,const char* src);

const char* UTIL_StringFindReverseConst(const char* base,const char* sub);
const char* UTIL_StringFindReverseAnyCaseConst(const char* base,const char* sub);
char* UTIL_StringFindReverse(char* base,char* sub);
char* UTIL_StringFindReverseAnyCase(char* base,char* sub);
//const int UTIL_StringLengthFastLE64(const char* src);
//const int UTIL_StringLengthFastBE64(const char* src);

//Copies a string until any char exists from arg3 list
void UTIL_CopyString(char* dst,const char* src,const char* stop);

//Extracts the middle substring of 2 substrings.The result is copied to dst
void UTIL_SubString(char* dst,const char* src,const char* s1,const char* s2);
void UTIL_SubStringLast(char* dst,const char* src,const char* s1,const char* s2);
void UTIL_StringReplaceList(char* dst,const char* src,const char* lst);

//Reverse string
void UTIL_StringReverse(char* src,int len);//Similar to K&R but optimized
void UTIL_StringReverseFast(char* dst,const char* src,const int len);//Custom implementation--faster but needs result buffer

//Conv
int UTIL_StringToInteger(const char* str);
void UTIL_IntegerToString(char* dst,int value,const int base);

#ifdef _LIB_UTIL_ADD_FLOAT_SUPPORT_
float UTIL_StringToFloat(const char* str);
void UTIL_FloatToString(char* dst,float value);
#endif

//Misc
int UTIL_IsAlpha(char c);
int UTIL_IsNumerical(char c);
int UTIL_IsAlNumerical(char c);
int UTIL_MemCompare(const void *first, const void *second);
int UTIL_IsSpace(char c);
int UTIL_IsPowerOfTwo(const int num);
int UTIL_IsMultipleOf(const int base,const int n);

//Skip whitespace
void UTIL_SkipWhitespace(const char* ptr,const int len,int* resultAddr);

//Trim
void UTIL_Trim(char* dst,const char* src);

//Binary utils
unsigned short UTIL_GetWord(const char* src);
unsigned int UTIL_GetLongLE(const char* src);
unsigned int UTIL_GetLong(const char* src);
void UTIL_Byteswap16(char* src,unsigned int length);
void UTIL_Byteswap32(char* src,unsigned int length);
void UTIL_PutLong(char* dst,const unsigned int l);
void UTIL_PutWord(char* dst,const unsigned short w);

//U/L
char UTIL_ToUpper(char c);
char UTIL_ToLower(char c);
void UTIL_StringToLower(char* str);
void UTIL_StringToUpper(char* str);

char UTIL_HexademicalToDigit(const char c);
#ifdef __cplusplus
}
#endif 
#endif


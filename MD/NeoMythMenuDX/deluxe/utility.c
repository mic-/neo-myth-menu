
//LOW LEVEL UTILITY MODULE (C89 COMPATIBLE)
//v1.4

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

#include "utility.h"

//Safe copy buffer
void UTIL_CopySafe(char* dst,const char* src,int len)
{
    while(len--)
        *dst++ = *src++;
}

//Unsafe copy buffer
void UTIL_CopyUnsafe(char* dst,const char* src)
{
    while(*dst)
        *dst++ = *src++;
}

//Set memory
void UTIL_SetMemorySafe(char* dst,const char c,int len)
{
    while(len--)
        *dst++ = c;
}

void UTIL_SetMemoryUnSafe(char* dst,const char c)
{
    while(*dst)
        *dst++ = c;
}

//String length using block jumps
int UTIL_StringLengthMemBlockConst(const char* str,const int blockLen)
{
    const char* base = str;

    while( *str )
        str += blockLen;

    return str - base;
}

int UTIL_StringLengthMemBlock(char* str,const int blockLen)
{
    char* base = str;

    while( *str )
        str += blockLen;

    return str - base;
}

//String length
int UTIL_StringLengthConst(const char* str)
{
    const char* base = str;

    while( *str )
        str++;

    return str - base;
}

int UTIL_StringLength(char* str)
{
    char* base = str;

    while( *str )
        str++;

    return str - base;
}

//find string
const char* UTIL_StringFindConst(const char* base,const char* sub)
{
    const char* pa;
    const char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( *pa == *pb )
        {
            pa++;
            pb++;

            if(! (*pb) )
                return base;
        }

        ++base;
    }

    return 0;
}

char* UTIL_StringFind(char* base,char* sub)
{
    char* pa;
    char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( *pa == *pb )
        {
            pa++;
            pb++;

            if(! (*pb) )
                return base;
        }

        ++base;
    }

    return 0;
}

const char* UTIL_StringFindAnyCaseConst(const char* base,const char* sub)
{
    const char* pa;
    const char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( UTIL_ToLower(*pa) == UTIL_ToLower(*pb) )
        {
            pa++;
            pb++;

            if(! (*pb) )
                return base;
        }

        ++base;
    }

    return 0;
}

char* UTIL_StringFindAnyCase(char* base,char* sub)
{
    char* pa;
    char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( UTIL_ToLower(*pa) == UTIL_ToLower(*pb) )
        {
            pa++;
            pb++;

            if(! (*pb) )
                return base;
        }

        ++base;
    }

    return 0;
}

const char* UTIL_StringFindLastConst(const char* base,const char* sub)
{
    const char* ret = NULL;
    const char* pa;
    const char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( *pa == *pb )
        {
            pa++;
            pb++;

            if(! (*pb) )
            {
                ret = base;
                break;
            }
        }

        ++base;
    }

    return ret;
}

char* UTIL_StringFindLast(char* base,char* sub)
{
    char* ret = NULL;
    char* pa;
    char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( *pa == *pb )
        {
            pa++;
            pb++;

            if(! (*pb) )
            {
                ret = base;
                break;
            }
        }

        ++base;
    }

    return ret;
}

const char* UTIL_StringFindLastAnyCaseConst(const char* base,const char* sub)
{
    const char* ret = NULL;
    const char* pa;
    const char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( UTIL_ToLower(*pa) == UTIL_ToLower(*pb) )
        {
            pa++;
            pb++;

            if(! (*pb) )
            {
                ret = base;
                break;
            }
        }

        ++base;
    }

    return ret;
}

char* UTIL_StringFindLastAnyCase(char* base,char* sub)
{
    char* ret = NULL;
    char* pa;
    char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( UTIL_ToLower(*pa) == UTIL_ToLower(*pb) )
        {
            pa++;
            pb++;

            if(! (*pb) )
            {
                ret = base;
                break;
            }
        }

        ++base;
    }

    return ret;
}

const char* UTIL_StringFindCharAnyCaseConst(const char* base,const char c)
{
    while(*base)
    {
        if(UTIL_ToLower(*base) == UTIL_ToLower(c))
            return base;

        ++base;
    }

    return 0;
}

const char* UTIL_StringFindCharConst(const char* base,const char c)
{
    while(*base)
    {
        if(*base == c)
            return base;

        ++base;
    }

    return 0;
}

char* UTIL_StringFindChar(char* base,char c)
{
    while(*base)
    {
        if(*base == c)
            return base;

        ++base;
    }

    return 0;
}

char* UTIL_StringFindCharAnyCase(char* base,char c)
{
    while(*base)
    {
        if(UTIL_ToLower(*base) == UTIL_ToLower(c))
            return base;

        ++base;
    }

    return 0;
}

const char* UTIL_StringFindLastCharAnyCaseConst(const char* base,const char c)
{
    const char* ret = NULL;

    while(*base)
    {
        if(UTIL_ToLower(*base) == UTIL_ToLower(c))
            ret = base;

        ++base;
    }

    return ret;
}

const char* UTIL_StringFindLastCharConst(const char* base,const char c)
{
    const char* ret = NULL;

    while(*base)
    {
        if(*base == c)
            ret = base;

        ++base;
    }

    return ret;
}

char* UTIL_StringFindLastChar(char* base,char c)
{
    char* ret = NULL;

    while(*base)
    {
        if(*base == c)
            ret = base;

        ++base;
    }

    return ret;
}

char* UTIL_StringFindLastCharAnyCase(char* base,char c)
{
    char* ret = NULL;

    while(*base)
    {
        if(UTIL_ToLower(*base) == UTIL_ToLower(c))
            ret = base;

        ++base;
    }

    return ret;
}

//Copies a string until any char exists from arg3 list
void UTIL_CopyString(char* dst,const char* src,const char* stop)
{
    const char* base = src;
    const char* sps;

    *(dst + 0) = '\0';

    for(;*src;src++,dst++)
    {
        for(sps = stop; *sps; sps++)
        {
            if(*sps == *src)
            {
                *(dst + (src - base)) = '\0';
                return;
            }
        }

        *dst = *src;
    }

    while(dst < src)//cull out reminder
    {
        if(!(*dst))
            break;

        *dst = '\0';
        dst++;
    }

    *dst = '\0';
}

//Extracts the middle substring of 2 substrings.The result is copied to dst
void UTIL_SubString(char* dst,const char* src,const char* s1,const char* s2)
{
    const char* pa;
    const char* pb;

    *(dst + 0) = '\0';

    pa = UTIL_StringFindConst(src,s1);

    if(!pa)
        return;

    pa = (pa + UTIL_StringLengthConst(s1));

    pb = UTIL_StringFindConst(pa ,s2);

    if(!pb)
        return;

    UTIL_CopySafe(dst,pa,pb-pa);

    *(dst + (pb - pa)) = '\0';
}

void UTIL_SubStringLast(char* dst,const char* src,const char* s1,const char* s2)
{
    const char* pa;
    const char* pb;

    *(dst + 0) = '\0';

    pa = UTIL_StringFindLastConst(src,s1);

    if(!pa)
        return;

    pa = (pa + UTIL_StringLengthConst(s1));

    pb = UTIL_StringFindLastConst(pa ,s2);

    if(!pb)
        return;

    UTIL_CopySafe(dst,pa,pb-pa);

    *(dst + (pb - pa)) = '\0';
}

void UTIL_StringReplaceList(char* dst,const char* src,const char* lst)
{
    while(*src)
    {
        if(!UTIL_StringFindLastCharConst(lst,*src))
        {
            *dst = *src;
            dst++;
        }

        src++;
    }

    while(dst < src)//cull out reminder
    {
        if(!(*dst))
            break;

        *dst = '\0';
        dst++;
    }

    *dst = '\0';
}

//Misc
int UTIL_IsAlpha(char c)
{
    if( ( c >= 'A' ) && ( c <= 'Z' ) )
        return 1;
    else if( ( c >= 'a' ) && ( c <= 'z' ) )
        return 1;

    return 0;
}

int UTIL_IsNumerical(char c)
{
    if( ( c - '0' ) < 0 )
        return 0;
    else if( ( c - '0' ) > 9 )
        return 0;

    return 1;
}

int UTIL_IsAlNumerical(char c)
{
    if(UTIL_IsNumerical(c))
        return 1;
    else if(UTIL_IsAlpha(c))
        return 1;

    return 0;
}

int UTIL_IsSpace(char c)
{
    switch(c)
    {
        case 0x9:
        case 0xa:
        case 0xb:
        case 0xc:
        case 0xd:
        case 0x20:
            return 1;

        default:
            return 0;
    }

    return 0;
}

int UTIL_IsPowerOfTwo(const int num)
{
    if(!num)
        return 0;

    return !(num & (num - 1));
}

int UTIL_IsMultipleOf(const int base,const int n)
{
    if(!base)
        return 0;

    return ( !(base & (n - 1) ));
}

//Trim
void UTIL_Trim(char* dst,const char* src)
{
    const char* p;

    while( !UTIL_IsAlNumerical(*src) )
        src++;

    p = src + UTIL_StringLengthConst(src) - 1;

    while( (p > src) && ( !UTIL_IsAlNumerical(*p) ) )
        p--;

    UTIL_CopySafe(dst,src,( ( p - src ) + 1));

    *(dst + ( ( p - src ) + 1) ) = '\0';

}

//Skip WS
void UTIL_SkipWhitespace(const char* ptr,const int len,int* resultAddr)
{
    while( (*resultAddr < len) )
    {
        if(!UTIL_IsSpace( *(ptr + (*resultAddr)) ) )
            break;
        else
            (*resultAddr)++;
    }
}

//Binary utils
unsigned int UTIL_GetLongLE(const char* src)
{
    return
    (
        (((unsigned int)*(src + 3)) & 0xFF)|
        ((unsigned int)(*(src + 2)&0xff) << 8) |
        ((unsigned int)(*(src + 1)&0xff)  << 16) |
        ((unsigned int)(*(src + 0)&0xff) << 24)
    );
}

unsigned int UTIL_GetLong(const char* src)
{
    return
    (
        (((unsigned int)(*(src + 0))&0xffL) << 24) |
        (((unsigned int)(*(src + 1))&0xffL)  << 16) |
        (((unsigned int)(*(src + 2))&0xffL) << 8) |
        (((unsigned int)*(src + 3)) & 0xffL)
    );
}

void UTIL_PutLong(char* dst,const unsigned int l)
{
    *(dst + 0) = (l >> 24)& 0xFF;
	*(dst + 1) = (l >> 16) & 0xFF;
	*(dst + 2) = (l >> 8) & 0xFF;
	*(dst + 3) = (l & 0xFF);
}

unsigned short UTIL_GetWord(const char* src)
{
	return  (((unsigned short)*(src + 0) << 8) |
            ((unsigned short)*(src + 1)));
}

void UTIL_PutWord(char* dst,const unsigned short w)
{
    *(dst + 0) = (w >> 8);
	*(dst + 1) = (w & 0xFF);
}

void UTIL_Byteswap16(char* src,unsigned int length)
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

void UTIL_Byteswap32(char* src,unsigned int length)
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

char UTIL_ToUpper(char c)
{
    if( (c >= 'a') && (c <= 'z') )
       return ('A' + ( c - 'a' ));

    return c;
}

char UTIL_ToLower(char c)
{
    if( (c >= 'A') && (c <= 'Z') )
        return ('a' + ( c - 'A' ));

    return c;
}

void UTIL_StringToLower(char* str)
{
    for(;*str;str++)
    {
        if( (*str >= 'A') && (*str <= 'Z') )
            *str = ('a' + ( *str - 'A' ));
    }
}

void UTIL_StringToUpper(char* str)
{
    for(;*str;str++)
    {
        if( (*str >= 'a') && (*str <= 'a') )
            *str = ('A' + ( *str - 'a' ));
    }
}

int UTIL_MemCompare(const void *first, const void *second)
{
    return *(char*)first - *(char*)second;
}

void UTIL_StringReverse(char* src,int len)
{
    char c;
    int i = 0;
    --len;

    while( i < len)
    {
        c = *(src + i);
        *(src + i) = *(src + len);
        *(src + len) = c;

        i++;
        len--;
    }
}

void UTIL_StringReverseFast(char* dst,const char* src,const int len)
{
    dst = (dst + (len - 1));

    while(*dst)
        *dst-- = *src++;
}

//Base can be up to 36
void UTIL_IntegerToString(char* dst,int value,const int base)
{
    static char DIGITS[] =
    "0123456789"
    "abcdefghij"
    "klmnopqrst"
    "uvwxyz";

    unsigned char neg = 0;
    int n = 0;

    if(value == 0)
    {
        *(dst + 0) = '0';
        *(dst + 1) = '\0';

        return;
    }

    if( base == BASE_10 )
    {
        if( value < 0 )
        {
            //make abs
            value = -value;
            neg = 1;
        }
    }

    while(value)
    {
        dst[n++] = DIGITS[value%base];

        value /= base;
    }

    if(neg)
        dst[n++] = '-';

    dst[n] = '\0';

    UTIL_StringReverse(dst,n);
}

int UTIL_StringToInteger(const char* str)
{
    const char* base = str;
    int result = 0;
    unsigned char neg = 0;

    while(*str)
    {
        if(!UTIL_IsSpace(*str))
            break;

        ++str;
    }

    if(str[0] == '0')
        return 0;
    else if(str[0] == '-')
    {
        ++str;
        neg = 1;
    }

	while(*str)
	{
	    if(str - base > 9)
            break;
        else if(!UTIL_IsNumerical(*str))
            break;

		result = (result * 10) + (*str - '0');
		str++;
	}

    if(neg)
        result *= -1;

	return result;
}

int UTIL_StringLengthFastLE32(const char* src)
{
    int result = 0;
    unsigned int* dword = (unsigned int*)src;

    for(;;)
    {
        if (!(*dword & 0x000000ff))//hi nibble
            return result;

        if (!(*dword & 0x0000ff00))
            return result + 1;

        if (!(*dword & 0x00ff0000))
            return result + 2;

        if (!(*dword & 0xff000000))
            return result + 3;

        result += 4;
        dword++;
    }

    return result;
}

int UTIL_StringLengthFastBE32(const char* src)
{
    int result = 0;
    unsigned int* dword = (unsigned int*)src;

    for(;;)
    {
        if (!(*dword & 0xff000000))//lo nibble
            return result;

        if (!(*dword & 0x00ff0000))
            return result + 1;

        if (!(*dword & 0x0000ff00))
            return result + 2;

        if (!(*dword & 0x000000ff))
            return result + 3;

        result += 4;
        dword++;
    }

    return result;
}

char UTIL_HexademicalToDigit(const char c)
{
    /*if( c >= '0' && c <= '9' )
        return c - '0';
    else if( c >= 'A' && c <= 'F' )
        return c - 'A' + BASE_10;
    else if( c >= 'a' && c <= 'f' )
        return c - 'a' + BASE_10;*/

    switch(c)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return c - '0';

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            return c - 'A' + BASE_10;

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            return c - 'a' + BASE_10;
    }

    return 0;
}

char* UTIL_StringAppend(char* dst,const char* src)
{
	char* p = dst;

    while(*dst++)
        ;

    for(--dst;(*dst++ = *src++);)
        ;

    if(*dst)
        *dst = '\0';

	return p;

}

#ifdef _LIB_UTIL_ADD_FLOAT_SUPPORT_
float UTIL_StringToFloat(const char* str)
{
    return 0.0f;
}

void UTIL_FloatToString(char* dst,float value)
{
    dst[0] = '\0';
}
#endif

char* UTIL_StringFindReverse(char* base,char* sub)
{
    const char* pa;
    const char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( *pa == *pb )
        {
            pa--;
            pb--;

            if(! (*pb) )
                return base;
        }

        --base;
    }

    return 0;
}

char* UTIL_StringFindReverseAnyCase(char* base,char* sub)
{
    const char* pa;
    const char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( UTIL_ToLower(*pa) == UTIL_ToLower(*pb) )
        {
            pa--;
            pb--;

            if(! (*pb) )
                return base;
        }

        --base;
    }

    return 0;
}

const char* UTIL_StringFindReverseConst(const char* base,const char* sub)
{
    const char* pa;
    const char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( *pa == *pb )
        {
            pa--;
            pb--;

            if(! (*pb) )
                return base;
        }

        --base;
    }

    return 0;
}

const char* UTIL_StringFindReverseAnyCaseConst(const char* base,const char* sub)
{
    const char* pa;
    const char* pb;

    if(!sub)
        return 0;

    while(*base)
    {
        pa = base;
        pb = sub;

        while( UTIL_ToLower(*pa) == UTIL_ToLower(*pb) )
        {
            pa--;
            pb--;

            if(! (*pb) )
                return base;
        }

        --base;
    }

    return 0;
}
/*
const int UTIL_StringLengthFastBE64(const char* src)
{
    int result = 0;
    QuadWord* quad = (QuadWord*)src;

    for(;;)
    {
        if (!(*quad & 0xff00000000000000LLU))
            return result;

        if (!(*quad & 0x00ff000000000000LLU))
            return result + 1;

        if (!(*quad & 0x0000ff0000000000LLU))
            return result + 2;

        if (!(*quad & 0x000000ff00000000LLU))
            return result + 3;

        if (!(*quad & 0x00000000ff000000LLU))
            return result + 4;

        if (!(*quad & 0x0000000000ff0000LLU))
            return result + 5;

        if (!(*quad & 0x000000000000ff00LLU))
            return result + 6;

        if (!(*quad & 0x000000000000000ffLLU))
            return result + 7;

        result += 8;
        quad++;
    }

    return result;
}

const int UTIL_StringLengthFastLE64(const char* src)
{
    int result = 0;
    QuadWord* quad = (QuadWord*)src;

    for(;;)
    {
        if (!(*quad & 0x000000000000000ffLLU))
            return result;

        if (!(*quad & 0x000000000000ff00LLU))
            return result + 1;

        if (!(*quad & 0x0000000000ff0000LLU))
            return result + 2;

        if (!(*quad & 0x00000000ff000000LLU))
            return result + 3;

        if (!(*quad & 0x000000ff00000000LLU))
            return result + 4;

        if (!(*quad & 0x0000ff0000000000LLU))
            return result + 5;

        if (!(*quad & 0x00ff000000000000LLU))
            return result + 6;

        if (!(*quad & 0xff00000000000000LLU))
            return result + 7;

        result += 8;
        quad++;
    }

    return result;
*/

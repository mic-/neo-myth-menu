
//By conle (conleon1988@gmail.com) for ChillyWilly's Extended SD Menu
#include <string.h> //for strchr
#include "cheat.h"
#include "utility.h"

//shared from gens
static char genie_chars[] =
  "AaBbCcDdEeFfGgHhJjKkLlMmNnPpRrSsTtVvWwXxYyZz0O1I2233445566778899";
static char hex_chars[] = "00112233445566778899AaBbCcDdEeFf";

void genie_decode (const char *code, CheatPair *result)
{
  int i = 0, n;
  char *x;

  for (; i < 8; ++i)
    {
      /* If strchr returns NULL, we were given a bad character */
      if (!(x = strchr (genie_chars, code[i])))
	{
	  result->addr = -1;
	  result->data = -1;
	  return;
	}
      n = (x - genie_chars) >> 1;
      /* Now, based on which character this is, fit it into the result */
      switch (i)
	{
	case 0:
	  /* ____ ____ ____ ____ ____ ____ : ____ ____ ABCD E___ */
	  result->data |= n << 3;
	  break;
	case 1:
	  /* ____ ____ DE__ ____ ____ ____ : ____ ____ ____ _ABC */
	  result->data |= n >> 2;
	  result->addr |= (n & 3) << 14;
	  break;
	case 2:
	  /* ____ ____ __AB CDE_ ____ ____ : ____ ____ ____ ____ */
	  result->addr |= n << 9;
	  break;
	case 3:
	  /* BCDE ____ ____ ___A ____ ____ : ____ ____ ____ ____ */
	  result->addr |= (n & 0xF) << 20 | (n >> 4) << 8;
	  break;
	case 4:
	  /* ____ ABCD ____ ____ ____ ____ : ___E ____ ____ ____ */
	  result->data |= (n & 1) << 12;
	  result->addr |= (n >> 1) << 16;
	  break;
	case 5:
	  /* ____ ____ ____ ____ ____ ____ : E___ ABCD ____ ____ */
	  result->data |= (n & 1) << 15 | (n >> 1) << 8;
	  break;
	case 6:
	  /* ____ ____ ____ ____ CDE_ ____ : _AB_ ____ ____ ____ */
	  result->data |= (n >> 3) << 13;
	  result->addr |= (n & 7) << 5;
	  break;
	case 7:
	  /* ____ ____ ____ ____ ___A BCDE : ____ ____ ____ ____ */
	  result->addr |= n;
	  break;
	}
      /* Go around again */
    }
  return;
}

void hex_decode (const char *code,CheatPair* result)
{
  char *x;
  int i;
  /* 6 digits for address */
  for (i = 0; i < 6; ++i)
    {
      if (!(x = strchr (hex_chars, code[i])))
	{
	  result->addr = result->data = -1;
	  return;
	}
      result->addr = (result->addr << 4) | ((x - hex_chars) >> 1);
    }
  /* 4 digits for data */
  for (i = 6; i < 10; ++i)
    {
      if (!(x = strchr (hex_chars, code[i])))
	{
	  result->addr = result->data = -1;
	  return;
	}
      result->data = (result->data << 4) | ((x - hex_chars) >> 1);
    }
}

int cheat_decode(const char* code,CheatPair* pair)
{
	const char* p;
	char buf[16 + 1];
 

	if(!pair)
		return 1;

	pair->data = 0;
	pair->addr = 0;

	UTIL_SetMemorySafe(buf,'\0',16);
 

	p = UTIL_StringFindLastCharConst(code,'-');

	if(p)
	{
		UTIL_CopyString(buf,code,"-\0\n ");
		UTIL_StringAppend(buf,p+1);
	
		genie_decode(buf,pair);
	}
	else
	{
		p = UTIL_StringFindLastCharConst(code,':');

		if(p)
		{
			UTIL_CopyString(buf,code,":\0\n ");
			UTIL_StringAppend(buf,p+1);

			hex_decode(buf,pair);
		}
		else
			return 1;
	}

	return 0;
}

int cheat_apply(void* data,CheatPair* pair)
{
	char* p;

	if(!pair)
		return 1;

	if(!pair->addr && !pair->data)
		return 1;

	p = ((char*)data) + pair->addr;

	*p = (pair->data & 0xFF00) >> 8;
	*(p+1) = pair->data & 0xFF;

	return 0;
}


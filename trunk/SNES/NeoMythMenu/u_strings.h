#include "basetypes.h"
#include "integer.h"

int wstrcmp(WCHAR *ws1, WCHAR *ws2);
int wstrlen(const WCHAR *ws);
WCHAR *wstrcat(WCHAR *ws1, WCHAR *ws2);
WCHAR *wstrcpy(WCHAR *ws1, WCHAR *ws2);
char *w2cstrcpy(char *s1, WCHAR *ws2);
WCHAR *c2wstrcpy(WCHAR *ws1, char *s2);
WCHAR *c2wstrcat(WCHAR *ws1, char *s2);

#include "u_strings.h"

int wstrcmp(WCHAR *ws1, WCHAR *ws2)
{
    int ix = 0;
    while (ws1[ix] && ws2[ix] && (ws1[ix] == ws2[ix])) ix++;
    if (!ws1[ix] && ws2[ix])
        return -1; // ws1 < ws2
    if (ws1[ix] && !ws2[ix])
        return 1; // ws1 > ws2
    return (int)ws1[ix] - (int)ws2[ix];
}

int wstrlen(const WCHAR *ws)
{
    int ix = 0;
    while (ws[ix]) ix++;
    return ix;
}

WCHAR *wstrcat(WCHAR *ws1, WCHAR *ws2)
{
    int ix = wstrlen(ws1);
    int iy = 0;
    while (ws2[iy])
    {
        ws1[ix] = ws2[iy];
        ix++;
        iy++;
    }
    ws1[ix] = 0;
    return ws1;
}

WCHAR *wstrcpy(WCHAR *ws1, WCHAR *ws2)
{
    int ix = 0;
    while (ws2[ix])
    {
        ws1[ix] = ws2[ix];
        ix++;
    }
    ws1[ix] = 0;
    return ws1;
}

char *w2cstrcpy(char *s1, WCHAR *ws2)
{
    int ix = 0;
    while (ws2[ix])
    {
        s1[ix] = (char)ws2[ix]; // lazy conversion of wide chars
        ix++;
    }
    s1[ix] = 0;
    return s1;
}

WCHAR *c2wstrcpy(WCHAR *ws1, char *s2)
{
    int ix = 0;
    while (s2[ix])
    {
        ws1[ix] = (WCHAR)s2[ix];
        ix++;
    }
    ws1[ix] = 0;
    return ws1;
}

WCHAR *c2wstrcat(WCHAR *ws1, char *s2)
{
    int ix = wstrlen(ws1);
    int iy = 0;
    while (s2[iy])
    {
        ws1[ix] = (WCHAR)s2[iy];
        ix++;
        iy++;
    }
    ws1[ix] = 0;
    return ws1;
}

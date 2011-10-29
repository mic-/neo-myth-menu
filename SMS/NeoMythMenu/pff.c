/*----------------------------------------------------------------------------/
/  Petit FatFs - FAT file system module  R0.01a                (C)ChaN, 2009
/-----------------------------------------------------------------------------/
/ Petit FatFs module is an open source software to implement FAT file system to
/ small embedded systems. This is a free software and is opened for education,
/ research and commercial developments under license policy of following trems.
/
/  Copyright (C) 2009, ChaN, all right reserved.
/
/ * The Petit FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial use UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
/ Jun 15,'09 R0.01a  Branched from FatFs R0.07b
/----------------------------------------------------------------------------*/

#include "pff.h"        /* Petit FatFs configurations and declarations */
#include "diskio.h"     /* Declarations of low level disk I/O functions */
#include "neo2_map.h"
#include <string.h>
#include "sms.h"


/*Fix for unreachable code in sdcc*/
/*
#define IsDBCS1(c)  0
#define IsDBCS2(c)  0
#define IsUpper(c)  (((c)>='A')&&((c)<='Z'))
#define IsLower(c)  (((c)>='a')&&((c)<='z'))

*/
BYTE IsDBCS1(BYTE in){in=in; return 0; }
BYTE IsDBCS2(BYTE in){in=in; return 0; }
BYTE IsUpper(BYTE in){ return (in>='A')&&(in<='Z'); }
BYTE IsLower(BYTE in){ return (in>='a')&&(in<='a'); }
BYTE _DF1S(){ return 0;}
BYTE _FS_RPATH(){ return 1; }
/*--------------------------------------------------------------------------

   Private Work Area

---------------------------------------------------------------------------*/

extern FATFS *FatFs;   /* Pointer to the file system object (logical drive) */
extern WCHAR LfnBuf[_MAX_LFN + 1];

#define NAMEBUF(sp,lp)  BYTE sp[12]; WCHAR *lp = LfnBuf
#define INITBUF(dj,sp,lp)   dj.fn = sp; dj.lfn = lp

/* Name status flags */
#define NS			11		/* Offset of name status byte */
#define NS_LOSS		0x01	/* Out of 8.3 format */
#define NS_LFN		0x02	/* Force to create LFN entry */
#define NS_LAST		0x04	/* Last segment */
#define NS_BODY		0x08	/* Lower case flag (body) */
#define NS_EXT		0x10	/* Lower case flag (ext) */
#define NS_DOT		0x20	/* Dot entry */

/*--------------------------------------------------------------------------

   Private Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* String functions                                                      */
/*-----------------------------------------------------------------------*/

/* Fill memory */
static
void mem_set (char* d, BYTE val, WORD cnt) {
    while (cnt--) { *d++ = (char)val; }
}

/* Compare memory to memory */
static
WORD mem_cmp (const char* d, const char* s,WORD cnt) {
    WORD r = 0;
    while ( (cnt--) && ((r = ((*d++) - (*s++))) == 0)) {}
    return r;
}

/* Check if chr is contained in the string */
static
WORD chk_chr (const char* str, WORD chr) {
    while ((*str) && (*str != chr)){ str++;}
    return *str;
}

/*
void waste_time()
{
   *(BYTE*)0xC000 = *(BYTE*)0xC000;
}
void pff_debug_print(BYTE val, BYTE x)
{
    BYTE lo,hi;
    WORD vaddr = 0x3000 + (x << 1) + (4 << 6);
    VdpCtrl = (vaddr & 0xFF);
    VdpCtrl = (vaddr >> 8) | 0x40;
        
    hi = (val >> 4);
    lo = val & 0x0F;
    lo += 16; hi += 16;
    if (lo > 25) lo += 7;
    if (hi > 25) hi += 7;
    VdpData = hi; waste_time(); VdpData = 0; waste_time();
    VdpData = lo; waste_time(); VdpData = 0; waste_time();
}
*/

/*-----------------------------------------------------------------------*/
/* FAT access - Read value of a FAT entry                                */
/*-----------------------------------------------------------------------*/

void pf_grab(FATFS** fs)
{
	*fs = FatFs;
}

//static
CLUST get_fat ( /* 1:IO error, Else:Cluster status */
    CLUST clst  /* Cluster# to get the link information */
)
{
    //WORD wc, bc, ofs;
    BYTE buf[4];
	DWORD db;
    FATFS *fs = FatFs;
    DSTATUS (*p_disk_readp)(void*, DWORD, WORD, WORD) = pfn_disk_readp;

    if (clst < 2 || clst >= fs->max_clust)  /* Range check */
        return 1;

	db = fs->fatbase;/*cache it to reduce code size*/
    switch (fs->fs_type) {
    case FS_FAT12 :
        /*bc = (WORD)clst;
        bc += bc >> 1;
        ofs = bc & 511;
        bc >>= 9;
        if (ofs != 511) {
            if (disk_readp(buf, db + bc, ofs, 2)) break;
        } else {
            if (disk_readp(buf, db + bc, 511, 1)) break;
            if (disk_readp(buf+1, db + bc + 1, 0, 1)) break;
        }
        wc = LD_WORD(buf);*/
        return 0; //(clst & 1) ? (wc >> 4) : (wc & 0xFFF);

    case FS_FAT16 :
        if (p_disk_readp(buf, db + (clst >> 8), (WORD)(((WORD)clst & 255) << 1), 2)) break;
        return LD_WORD(buf);
#if _FS_FAT32
    case FS_FAT32 :
        if (p_disk_readp(buf, db + (clst >> 7), (WORD)(((WORD)clst & 127) << 2), 4)) break;
        return LD_DWORD(buf) & 0x0FFFFFFF;
#endif
    }

    return 1;   /* An error occured at the disk I/O layer */
}




/*-----------------------------------------------------------------------*/
/* Get sector# from cluster#                                             */
/*-----------------------------------------------------------------------*/

// Optimized version implemented in pff_asm.asm

static
DWORD clust2sect (    /* !=0: Sector number, 0: Failed - invalid cluster# */
  CLUST clst      /* Cluster# to be converted */
)
{
  FATFS *fs = FatFs;

  clst -= 2;
  if (clst >= (fs->max_clust - 2)) return 0;      /* Invalid cluster# */
  return (DWORD)clst * fs->csize + fs->database;
}




#if _USE_LFN

#define ff_convert(wc, x) ((char)wc)

static WCHAR ff_wtoupper(WCHAR wc) {
    char c = (char)wc;
    if (c >= 'a' && c <= 'z') wc -= ' ';
    return wc;
}


static
const BYTE LfnOfs[] = {1,3,5,7,9,14,16,18,20,22,24,28,30};  /* Offset of LFN chars in the directory entry */


static
BOOL cmp_lfn (          /* TRUE:Matched, FALSE:Not matched */
    WCHAR *lfnbuf,      /* Pointer to the LFN to be compared */
    BYTE *dir           /* Pointer to the directory entry containing a part of LFN */
)
{
    int i, s;
    WCHAR wc, uc;


    i = ((dir[LDIR_Ord] & 0xBF) - 1) * 13;  /* Get offset in the LFN buffer */
    s = 0; wc = 1;
    do {
        uc = LD_WORD(dir+LfnOfs[s]);    /* Pick an LFN character from the entry */
        if (wc) {   /* Last char has not been processed */
            wc = ff_wtoupper(uc);       /* Convert it to upper case */
            if (i >= _MAX_LFN || wc != ff_wtoupper(lfnbuf[i++]))    /* Compare it */
                return FALSE;           /* Not matched */
        } else {
            if (uc != 0xFFFF) return FALSE; /* Check filler */
        }
    } while (++s < 13);             /* Repeat until all chars in the entry are checked */

    if ((dir[LDIR_Ord] & 0x40) && wc && lfnbuf[i])  /* Last segment matched but different length */
        return FALSE;

    return TRUE;                    /* The part of LFN matched */
}



static
BOOL pick_lfn (         /* TRUE:Succeeded, FALSE:Buffer overflow */
    WCHAR *lfnbuf,      /* Pointer to the Unicode-LFN buffer */
    BYTE *dir           /* Pointer to the directory entry */
)
{
    int i, s;
    WCHAR wc, uc;


    i = ((dir[LDIR_Ord] & 0x3F) - 1) * 13;  /* Offset in the LFN buffer */

    s = 0; wc = 1;
    do {
        uc = LD_WORD(dir+LfnOfs[s]);            /* Pick an LFN character from the entry */
        if (wc) {   /* Last char has not been processed */
            if (i >= _MAX_LFN) return FALSE;    /* Buffer overflow? */
            lfnbuf[i++] = wc = uc;              /* Store it */
        } else {
            if (uc != 0xFFFF) return FALSE;     /* Check filler */
        }
    } while (++s < 13);                     /* Read all character in the entry */

    if (dir[LDIR_Ord] & 0x40) {             /* Put terminator if it is the last LFN part */
        if (i >= _MAX_LFN) return FALSE;    /* Buffer overflow? */
        lfnbuf[i] = 0;
    }

    return TRUE;
}

static
BYTE sum_sfn (
    const BYTE *dir     /* Ptr to directory entry */
)
{
    BYTE sum = 0;
    int n = 11;

    do sum = (sum >> 1) + (sum << 7) + *dir++; while (--n);
    return sum;
}
#endif



/*-----------------------------------------------------------------------*/
/* Directory handling - Rewind directory index                           */
/*-----------------------------------------------------------------------*/

static
FRESULT dir_rewind (
    DIR *dj         /* Pointer to directory object */
)
{
    CLUST clst;
    FATFS *fs = FatFs;

    dj->index = 0;
    clst = dj->sclust;
    if (clst == 1 || clst >= fs->max_clust) /* Check start cluster range */
        return FR_DISK_ERR;
#if _FS_FAT32
    if (!clst && fs->fs_type == FS_FAT32)   /* Replace cluster# 0 with root cluster# if in FAT32 */
        clst = fs->dirbase;
#endif
    dj->clust = clst;                       /* Current cluster */
    dj->sect = clst ? clust2sect(clst) : fs->dirbase;   /* Current sector */

    return FR_OK;   /* Seek succeeded */
}




/*-----------------------------------------------------------------------*/
/* Directory handling - Move directory index next                        */
/*-----------------------------------------------------------------------*/

static
FRESULT dir_next (  /* FR_OK:Succeeded, FR_NO_FILE:End of table, FR_DENIED:EOT and could not streach */
    DIR *dj         /* Pointer to directory object */
)
{
    CLUST clst;
    WORD i;
    FATFS *fs = FatFs;

    i = dj->index + 1;
    if (!i || !dj->sect)    /* Report EOT when index has reached 65535 */
        return FR_NO_FILE;

    if (!(i & (16-1))) {    /* Sector changed? */
        dj->sect++;         /* Next sector */

        if (dj->clust == 0) {   /* Static table */
            if (i >= fs->n_rootdir) /* Report EOT when end of table */
                return FR_NO_FILE;
        }
        else {                  /* Dynamic table */
            if (((i >> 4) & (fs->csize-1)) == 0) {  /* Cluster changed? */
                clst = get_fat(dj->clust);      /* Get next cluster */
                if (clst <= 1) return FR_DISK_ERR;
                if (clst >= fs->max_clust)      /* When it reached end of dynamic table */
                    return FR_NO_FILE;          /* Report EOT */
                dj->clust = clst;               /* Initialize data for new cluster */
                dj->sect = clust2sect(clst);
            }
        }
    }

    dj->index = i;

    return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Directory handling - Find an object in the directory                  */
/*-----------------------------------------------------------------------*/

static
FRESULT dir_find (
	DIR *dj			/* Pointer to the directory object linked to the file name */
)
{
	FRESULT res;
	BYTE c, *dir;
#if _USE_LFN
	BYTE a, ord, sum;
#endif
    DSTATUS (*p_disk_readp)(void*, DWORD, WORD, WORD) = pfn_disk_readp;
    
	res = dir_rewind(dj);			/* Rewind directory object */
	if (res != FR_OK) return res;

	dir = FatFs->buf;
#if _USE_LFN
	ord = sum = 0xFF;
#endif
	do {
		res = p_disk_readp(dir, dj->sect, (WORD)((dj->index & 15) << 5), 32)	/* Read an entry */
			? FR_DISK_ERR : FR_OK;
		if (res != FR_OK) break;
		c = dir[DIR_Name];	/* First character */
		if (c == 0) { res = FR_NO_FILE; break; }	/* Reached to end of table */

#if _USE_LFN	/* LFN configuration */
		a = dir[DIR_Attr] & AM_MASK;
		if (c == 0xE5 || ((a & AM_VOL) && a != AM_LFN)) {	/* An entry without valid data */
			ord = 0xFF;
		} else {
			if (a == AM_LFN) {			/* An LFN entry is found */
				if (dj->lfn) {
					if (c & 0x40) {		/* Is it start of LFN sequence? */
						sum = dir[LDIR_Chksum];
						c &= 0xBF; ord = c;	/* LFN start order */
						dj->lfn_idx = dj->index;
					}
					/* Check validity of the LFN entry and compare it with given name */
					ord = (c == ord && sum == dir[LDIR_Chksum] && cmp_lfn(dj->lfn, dir)) ? ord - 1 : 0xFF;
				}
			} else {					/* An SFN entry is found */
				if (!ord && sum == sum_sfn(dir)) break;	/* LFN matched? */
				ord = 0xFF; dj->lfn_idx = 0xFFFF;	/* Reset LFN sequence */
				if (!(dj->fn[NS] & NS_LOSS) && !mem_cmp(dir, dj->fn, 11)) break;	/* SFN matched? */
			}
		}
#else
		if (!(dir[DIR_Attr] & AM_VOL) && !mem_cmp(dir, dj->fn, 11)) /* Is it a valid entry? */
			break;
#endif
		res = dir_next(dj);							/* Next entry */
	} while (res == FR_OK);

	return res;
}

/*-----------------------------------------------------------------------*/
/* Read an object from the directory                                     */
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/* Read an object from the directory                                     */
/*-----------------------------------------------------------------------*/
#if _USE_DIR
static
FRESULT dir_read (
    DIR *dj         /* Pointer to the directory object to store read object name */
)
{
    FRESULT res;
    BYTE a, c, *dir;
#if _USE_LFN
    BYTE ord = 0xFF, sum = 0xFF;
#endif
    DSTATUS (*p_disk_readp)(void*, DWORD, WORD, WORD) = pfn_disk_readp;
    
    res = FR_NO_FILE;
    while (dj->sect) {
        dir = FatFs->buf;
        res = p_disk_readp(dir, dj->sect, (WORD)((dj->index & 15) << 5), 32)  /* Read an entry */
            ? FR_DISK_ERR : FR_OK;
        if (res != FR_OK) break;
        c = dir[DIR_Name];
        if (c == 0) { res = FR_NO_FILE; break; }    /* Reached to end of table */
        a = dir[DIR_Attr] & AM_MASK;

#if _USE_LFN    /* LFN configuration */
        a = dir[DIR_Attr] & AM_MASK;
        if (c == 0xE5 || (!_FS_RPATH() && c == '.') || ((a & AM_VOL) && a != AM_LFN)) {   /* An entry without valid data */
            ord = 0xFF;
        } else {
            if (a == AM_LFN) {          /* An LFN entry is found */
                if (c & 0x40) {         /* Is it start of LFN sequence? */
                    sum = dir[LDIR_Chksum];
                    c &= 0xBF; ord = c;
                    dj->lfn_idx = dj->index;
                }
                /* Check LFN validity and capture it */
                ord = (c == ord && sum == dir[LDIR_Chksum] && pick_lfn(dj->lfn, dir)) ? ord - 1 : 0xFF;
            } else {                    /* An SFN entry is found */
                if (ord || sum != sum_sfn(dir)) /* Is there a valid LFN? */
                    dj->lfn_idx = 0xFFFF;       /* It has no LFN. */
                break;
            }
        }
#else       /* Non LFN configuration */
        if (c != 0xE5 &&  (_FS_RPATH() || (c != (BYTE)'.')) && !(a & AM_VOL)) /* Is it a valid entry? */
            break;
#endif

        res = dir_next(dj);             /* Next entry */
        if (res != FR_OK) break;
    }

    if (res != FR_OK) dj->sect = 0;

    return res;
}
#endif



/*-----------------------------------------------------------------------*/
/* Pick a segment and create the object name in directory form           */
/*-----------------------------------------------------------------------*/



static
FRESULT create_name (
	DIR *dj,			/* Pointer to the directory object */
	const XCHAR **path	/* Pointer to pointer to the segment in the path string */
)
{

#if _USE_LFN	/* LFN configuration */
	BYTE b, cf;
	WCHAR w, *lfn;
	int i, ni, si, di;
	const XCHAR *p;

	/* Create LFN in Unicode */
	si = di = 0;
	p = *path;
	lfn = dj->lfn;
	for (;;) {
		w = p[si++];					/* Get a character */
		if (w < ' ' || w == '/' || w == '\\') {break;}	/* Break on end of segment */
		if (di >= _MAX_LFN)				/* Reject too long name */
			{return FR_INVALID_NAME;}
//#if !_LFN_UNICODE
		w &= 0xFF;
		/* If it is a DBC 1st byte */
		if (IsDBCS1(w)) {				
			b = p[si++];				 
			if (!IsDBCS2(b))			 
				{return FR_INVALID_NAME;}
			w = (w << 8) + b;
		}
		w = ff_convert(w, 1);			/* Convert OEM to Unicode */
		if (!w) {return FR_INVALID_NAME;}	/* Reject invalid code */
//#endif
		if (w < 0x80 && chk_chr("\"*:<>\?|\x7F", w)) /* Reject illegal chars for LFN */
			{return FR_INVALID_NAME;}
		lfn[di++] = w;					/* Store the Unicode char */
	}
	*path = &p[si];						/* Rerurn pointer to the next segment */
	cf = (w < ' ') ? NS_LAST : 0;		/* Set last segment flag if end of path */
	if(_FS_RPATH())
	if ((di == 1 && lfn[di - 1] == '.') || /* Is this a dot entry? */
		(di == 2 && lfn[di - 1] == '.' && lfn[di - 2] == '.')) {
		lfn[di] = 0;
		for (i = 0; i < 11; i++)
			dj->fn[i] = (i < di) ? '.' : ' ';
		dj->fn[i] = cf | NS_DOT;		/* This is a dot entry */
		return FR_OK;
	}

	while (di) {						/* Strip trailing spaces and dots */
		w = lfn[di - 1];
		if (w != ' ' && w != '.') break;
		di--;
	}
	if (!di) return FR_INVALID_NAME;	/* Reject null string */

	lfn[di] = 0;						/* LFN is created */

	/* Create SFN in directory form */
	mem_set(dj->fn, ' ', 11);
	for (si = 0; lfn[si] == ' ' || lfn[si] == '.'; si++) ;	/* Strip leading spaces and dots */
	if (si) cf |= NS_LOSS | NS_LFN;
	while (di && lfn[di - 1] != '.') di--;	/* Find extension (di<=si: no extension) */

	b = i = 0; ni = 8;
	for (;;) {
		w = lfn[si++];					/* Get an LFN char */
		if (!w) break;					/* Break on enf of the LFN */
		if (w == ' ' || (w == '.' && si != di)) {	/* Remove spaces and dots */
			cf |= NS_LOSS | NS_LFN; continue;
		}

		if (i >= ni || si == di) {		/* Extension or end of SFN */
			if (ni == 11) {				/* Long extension */
				cf |= NS_LOSS | NS_LFN; break;
			}
			if (si != di) cf |= NS_LOSS | NS_LFN;	/* Out of 8.3 format */
			if (si > di) break;			/* No extension */
			si = di; i = 8; ni = 11;	/* Enter extension section */
			b <<= 2; continue;
		}

		if (w >= 0x80) {				/* Non ASCII char */
#ifdef _EXCVT
			w = ff_convert(w, 0);		/* Unicode -> OEM code */
			if (w) w = cvt[w - 0x80];	/* Convert extended char to upper (SBCS) */
#else
			w = ff_convert(ff_wtoupper(w), 0);	/* Upper converted Unicode -> OEM code */
#endif
			cf |= NS_LFN;				/* Force create LFN entry */
		}

		if (_DF1S() && w >= 0x100) {		/* Double byte char */
			if (i >= ni - 1) {
				cf |= NS_LOSS | NS_LFN; i = ni; continue;
			}
			dj->fn[i++] = (BYTE)(w >> 8);
		} else {						/* Single byte char */
			if (!w || chk_chr("+,;[=]", w)) {		/* Replace illegal chars for SFN */
				w = '_'; cf |= NS_LOSS | NS_LFN;	/* Lossy conversion */
			} else {
				if (IsUpper(w)) {		/* ASCII large capital */
					b |= 2;
				} else {
					if (IsLower(w)) {	/* ASCII small capital */
						b |= 1; w -= 0x20;
					}
				}
			}
		}
		dj->fn[i++] = (BYTE)w;
	}

	if (dj->fn[0] == 0xE5) dj->fn[0] = 0x05;	/* If the first char collides with deleted mark, replace it with 0x05 */

	if (ni == 8) b <<= 2;
	if ((b & 0x0C) == 0x0C || (b & 0x03) == 0x03)	/* Create LFN entry when there are composite capitals */
		cf |= NS_LFN;
	if (!(cf & NS_LFN)) {						/* When LFN is in 8.3 format without extended char, NT flags are created */
		if ((b & 0x03) == 0x01) cf |= NS_EXT;	/* NT flag (Extension has only small capital) */
		if ((b & 0x0C) == 0x04) cf |= NS_BODY;	/* NT flag (Filename has only small capital) */
	}

	dj->fn[NS] = cf;	/* SFN is created */

	return FR_OK;


#else	/* Non-LFN configuration */
	BYTE c, ni, si, i, *sfn;
	const char *p;

	/* Create file name in directory form */
	sfn = dj->fn;
	mem_set(sfn, ' ', 11);
	si = i = 0; ni = 8;
	p = *path;
	for (;;) {
		c = p[si++];
		if (c < ' ' || c == '/') break;	/* Break on end of segment */
		if (c == '.' || i >= ni) {
			if (ni != 8 || c != '.') return FR_INVALID_NAME;
			i = 8; ni = 11;
			continue;
		}
		if (c >= 0x7F || chk_chr(" +,;[=\\]\"*:<>\?|", c))	/* Reject unallowable chrs for SFN */
			return FR_INVALID_NAME;
		if (c >='a' && c <= 'z') c -= 0x20;
		sfn[i++] = c;
	}
	if (!i) return FR_INVALID_NAME;		/* Reject null string */
	*path = &p[si];						/* Rerurn pointer to the next segment */

	sfn[11] = (c < ' ') ? 1 : 0;		/* Set last segment flag if end of path */

	return FR_OK;
#endif
}




/*-----------------------------------------------------------------------*/
/* Get file information from directory entry                             */
/*-----------------------------------------------------------------------*/
#if _USE_DIR


static
void get_fileinfo (     /* No return code */
    DIR *dj,            /* Pointer to the directory object */
    FILINFO *fno        /* Pointer to store the file information */
)
{
    BYTE i, c, *dir;
    char *p;

    p = fno->fname;
    if (dj->sect) {
        dir = FatFs->buf;
        for (i = 0; i < 8; i++) {   /* Copy file name body */
            c = dir[i];
            if (c == (BYTE)' ') break;
            if (c == 0x05) c = 0xE5;
            *p++ = c;
        }
        if (dir[8] != (BYTE)' ') {        /* Copy file name extension */
            *p++ = '.';
            for (i = 8; i < 11; i++) {
                c = dir[i];
                if (c == (BYTE)' ') break;
                *p++ = c;
            }
        }
        fno->fattrib = dir[DIR_Attr];               /* Attribute */
        fno->fsize = LD_DWORD(dir+DIR_FileSize);    /* Size */
        fno->fdate = LD_WORD(dir+DIR_WrtDate);      /* Date */
        fno->ftime = LD_WORD(dir+DIR_WrtTime);      /* Time */
    }
    *p = 0;

#if _USE_LFN
    if (fno->lfname) {
        XCHAR *tp = fno->lfname;
        WCHAR w, *lfn;

        i = 0;
        if (dj->sect && dj->lfn_idx != 0xFFFF) {/* Get LFN if available */
            lfn = dj->lfn;
            while ((w = *lfn++) != 0) {         /* Get an LFN char */
//#if !_LFN_UNICODE
                w = ff_convert(w, 0);           /* Unicode -> OEM conversion */
                if (!w) { i = 0; break; }       /* Could not convert, no LFN */
                if (_DF1S() && w >= 0x100)        /* Put 1st byte if it is a DBC */
                    tp[i++] = (XCHAR)(w >> 8);
//#endif
                if (i >= fno->lfsize - 1) { i = 0; break; } /* Buffer overrun, no LFN */
                tp[i++] = (XCHAR)w;
            }
        }
        tp[i] = 0;  /* Terminator */
    }
#endif

}
#endif /* _USE_DIR */



/*-----------------------------------------------------------------------*/
/* Follow a file path                                                    */
/*-----------------------------------------------------------------------*/

static
FRESULT follow_path (   /* FR_OK(0): successful, !=0: error code */
    DIR *dj,            /* Directory object to return last directory and found object */
    const char *path    /* Full-path string to find a file or directory */
)
{
    FRESULT res;
    BYTE *dir;


    if (*path == '/') path++;           /* Strip heading separator */
    dj->sclust = 0;                     /* Set start directory (always root dir) */

    if ((BYTE)*path < (BYTE)' ') {            /* Null path means the root directory */
        res = dir_rewind(dj);
        FatFs->buf[0] = 0;

    } else {                            /* Follow path */
        for (;;) {    
           res = create_name(dj, &path);   /* Get a segment */
            if (res != FR_OK) break;       
            res = dir_find(dj);             /* Find it */
            if (res != FR_OK) {             /* Could not find the object */
                if (res == FR_NO_FILE && !*(dj->fn+11))
                    res = FR_NO_PATH;
                break;
            }
            if (*(dj->fn+11)) break;        /* Last segment match. Function completed. */
            dir = FatFs->buf;               /* There is next segment. Follow the sub directory */
            if (!(dir[DIR_Attr] & AM_DIR)) { /* Cannot follow because it is a file */
                res = FR_NO_PATH; break;
            }
            dj->sclust =





#if _FS_FAT32
                ((DWORD)LD_WORD(dir+DIR_FstClusHI) << 16) |
#endif
                LD_WORD(dir+DIR_FstClusLO);
        }
    }

    return res;
}




/*-----------------------------------------------------------------------*/
/* Check a sector if it is an FAT boot record                            */
/*-----------------------------------------------------------------------*/

static
BYTE check_fs ( /* 0:The FAT boot record, 1:Valid boot record but not an FAT, 2:Not a boot record, 3:Error */
    BYTE *buf,  /* Working buffer */
    DWORD sect  /* Sector# (lba) to check if it is an FAT boot record or not */
)
{
    DSTATUS (*p_disk_readp)(void*, DWORD, WORD, WORD) = pfn_disk_readp;
    if (p_disk_readp(buf, sect, 510, 2))      /* Read the boot sector */
        return 3;
    if (LD_WORD(buf) != 0xAA55)             /* Check record signature */
        return 2;

    if (!p_disk_readp(buf, sect, BS_FilSysType, 2) && LD_WORD(buf) == 0x4146) /* Check FAT12/16 */
        return 0;
#if _FS_FAT32
    if (!p_disk_readp(buf, sect, BS_FilSysType32, 2) && LD_WORD(buf) == 0x4146)   /* Check FAT32 */
        return 0;
#endif
    return 1;
}




/*--------------------------------------------------------------------------

   Public Functions

--------------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/* Mount/Unmount a Locical Drive                                         */
/*-----------------------------------------------------------------------*/

FRESULT pf_mount (
    FATFS *fs       /* Pointer to new file system object (NULL: Unmount) */
)
{
    BYTE fmt;
    BYTE buf[36];
    DWORD bsect, fsize, tsect, mclst;
    DSTATUS (*p_disk_initialize)(void) = pfn_disk_initialize;
    DSTATUS (*p_disk_readp)(void*, DWORD, WORD, WORD) = pfn_disk_readp;
    
    FatFs = 0;
    if (!fs) return FR_OK;              /* Unregister fs object */

    if (p_disk_initialize() != 0) { /* Check if the drive is ready or not */
        return FR_NOT_READY;
    }
       
    /* Search FAT partition on the drive */
    bsect = 0;
    fmt = check_fs(buf, bsect);         /* Check sector 0 as an SFD format */

    if (fmt == 1) {                     /* Not an FAT boot record, it may be FDISK format */
        /* Check a partition listed in top of the partition table */
        if (p_disk_readp(buf, bsect, MBR_Table, 16)) {    /* 1st partition entry */
            fmt = 3;
        } else {
            if (buf[4]) {                   /* Is the partition existing? */
                bsect = LD_DWORD(&buf[8]);  /* Partition offset in LBA */
                fmt = check_fs(buf, bsect); /* Check the partition */
            }
        }
    }

    if (fmt == 3) return FR_DISK_ERR;
    if (fmt) return FR_NO_FILESYSTEM;   /* No valid FAT patition is found */

    /* Initialize the file system object */
    if (p_disk_readp(buf, bsect, 13, sizeof(buf))) return FR_DISK_ERR;

    fsize = LD_WORD(buf+BPB_FATSz16-13);                /* Number of sectors per FAT */
    if (!fsize) fsize = LD_DWORD(buf+BPB_FATSz32-13);

    fsize *= buf[BPB_NumFATs-13];                       /* Number of sectors in FAT area */
    fs->fatbase = bsect + LD_WORD(buf+BPB_RsvdSecCnt-13); /* FAT start sector (lba) */
    fs->csize = buf[BPB_SecPerClus-13];                 /* Number of sectors per cluster */
    fs->n_rootdir = LD_WORD(buf+BPB_RootEntCnt-13);     /* Nmuber of root directory entries */
    tsect = LD_WORD(buf+BPB_TotSec16-13);               /* Number of sectors on the file system */
    if (!tsect) tsect = LD_DWORD(buf+BPB_TotSec32-13);
    mclst = (tsect                      /* Last cluster# + 1 */
        - LD_WORD(buf+BPB_RsvdSecCnt-13) - fsize - (fs->n_rootdir >> 4)
        ) / fs->csize + 2;
    fs->max_clust = (CLUST)mclst;

    fmt = FS_FAT12;                         /* Determine the FAT sub type */
    if (mclst >= 0xFF7) fmt = FS_FAT16;     /* Number of clusters >= 0xFF5 */

    if (mclst >= 0xFFF7)                    /* Number of clusters >= 0xFFF5 */
    {
#if _FS_FAT32
        fmt = FS_FAT32;
#else
        return FR_NO_FILESYSTEM;
#endif
    }

    fs->fs_type = fmt;      /* FAT sub-type */
#if _FS_FAT32
    if (fmt == FS_FAT32)
        fs->dirbase = LD_DWORD(buf+(BPB_RootClus-13));  /* Root directory start cluster */
    else
#endif
        fs->dirbase = fs->fatbase + fsize;              /* Root directory start sector (lba) */
    fs->database = fs->fatbase + fsize + (fs->n_rootdir >> 4); /// 16;  /* Data start sector (lba) */

    fs->flag = 0;
    FatFs = fs;

    return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Open or Create a File                                                 */
/*-----------------------------------------------------------------------*/

FRESULT pf_open (
    const char *path    /* Pointer to the file name */
)
{
    FRESULT res;
    DIR dj;
    BYTE dir[32];
    NAMEBUF(sfn, lfn);
    FATFS *fs = FatFs;


    if (!fs)                        /* Check file system */
        return FR_NOT_ENABLED;

    INITBUF(dj, sfn, lfn);
    fs->flag = 0;
    fs->buf = dir;
    dj.fn = sfn; //sp;
    res = follow_path(&dj, path);   /* Follow the file path */
    if (res != FR_OK) {return res;}/*return 64 + res;*/   /* Follow failed */
    if (!dir[0] || (dir[DIR_Attr] & AM_DIR))    /* It is a directory */
        return FR_NO_FILE;

    fs->org_clust =                     /* File start cluster */
#if _FS_FAT32
        ((DWORD)LD_WORD(dir+DIR_FstClusHI) << 16) |
#endif
        LD_WORD(dir+DIR_FstClusLO);
    fs->fsize = LD_DWORD(dir+DIR_FileSize); /* File size */
    fs->fptr = 0;                       /* File pointer */
    fs->flag = FA_READ;

    return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Read File                                                             */
/*-----------------------------------------------------------------------*/

FRESULT pf_read (
    void* dest,     /* Pointer to the destination object */
    WORD btr,       /* Number of bytes to read (bit15:destination) */
    WORD* br        /* Pointer to number of bytes read */
)
{
    DRESULT dr;
    CLUST clst;
    DWORD sect, remain;
    WORD rcnt;
    BYTE *rbuff = dest;
    FATFS *fs = FatFs;
    DSTATUS (*p_disk_readp)(void*, DWORD, WORD, WORD) = pfn_disk_readp;

    *br = 0;
    if (!fs) return FR_NOT_ENABLED;     /* Check file system */
    if (!(fs->flag & FA_READ))
            return FR_INVALID_OBJECT;

    remain = fs->fsize - fs->fptr;
    if (btr > remain) btr = (WORD)remain;           /* Truncate btr by remaining bytes */

    for ( ;  btr;                                   /* Repeat until all data transferred */
        rbuff += rcnt, fs->fptr += rcnt, *br += rcnt, btr -= rcnt) {
        if ((fs->fptr & 511) == 0) {                /* On the sector boundary? */
            if (((fs->fptr >> 9) & (fs->csize - 1)) == 0) { /* On the cluster boundary? */
                clst = (fs->fptr == 0) ?            /* On the top of the file? */
                    fs->org_clust : get_fat(fs->curr_clust);
                if (clst <= 1) {
                    fs->flag = 0; return FR_DISK_ERR;
                }
                fs->curr_clust = clst;              /* Update current cluster */
                fs->csect = 0;                      /* Reset sector offset in the cluster */
            }
            sect = clust2sect(fs->curr_clust);      /* Get current sector */
            if (!sect) {
                fs->flag = 0; return FR_DISK_ERR;
            }
            sect += fs->csect;
            fs->dsect = sect;
            fs->csect++;                            /* Next sector address in the cluster */
        }
        rcnt = 512 - ((WORD)fs->fptr & 511);        /* Get partial sector data from sector buffer */
        if (rcnt > btr) rcnt = btr;
        if (fs->flag & FA_STREAM) {
            dr = p_disk_readp(dest, fs->dsect, (WORD)(fs->fptr & 511), (WORD)(rcnt | 0x8000));
        } else {
            dr = p_disk_readp(rbuff, fs->dsect, (WORD)(fs->fptr & 511), rcnt);
        }
        if (dr) {
            fs->flag = 0;
            return (dr == RES_WRPRT/*STRERR*/) ? FR_STREAM_ERR : FR_DISK_ERR;
        }
    }

    return FR_OK;
}

FRESULT pf_read_sectors (
    WORD destLo,
    WORD destHi,
    WORD count
)
{
    DRESULT dr;
    CLUST clst;
    CLUST prevClst;
    DWORD firstSector;
    DWORD sect;
    DWORD praddr;
    WORD remSectInClust, sectorsToRead;
    WORD adjacentSectors;
    FATFS *fs = FatFs;
    DSTATUS (*p_disk_read_sectors)(WORD, DWORD, WORD, WORD) = pfn_disk_read_sectors;
    BYTE doRead;
    
    if (!fs) return FR_NOT_ENABLED;     /* Check file system */
    if (!(fs->flag & FA_READ))
        return FR_INVALID_OBJECT;

    prevClst = 0xFFFF;
    adjacentSectors = 0;
    praddr = (((DWORD)destHi) << 16) | destLo;
    firstSector = 0;

    while (count)
    {  
        if ((fs->fptr & 511) == 0) {                /* On the sector boundary? */
            if (((fs->fptr >> 9) & (fs->csize - 1)) == 0) { /* On the cluster boundary? */
                clst = (fs->fptr == 0) ?            /* On the top of the file? */
                    fs->org_clust : get_fat(fs->curr_clust);
                if (clst <= 1) {
                    fs->flag = 0; return FR_DISK_ERR;
                }
                fs->curr_clust = clst;              /* Update current cluster */
                fs->csect = 0;                      /* Reset sector offset in the cluster */
            }
            sect = clust2sect(fs->curr_clust);      /* Get current sector */
            if (!sect) {
                fs->flag = 0; return FR_DISK_ERR;
            }
            sect += fs->csect;
            fs->dsect = sect;
            fs->csect++;                            /* Next sector address in the cluster */
        }
    
        if (prevClst == 0xFFFF) firstSector = fs->dsect;
        
        remSectInClust = fs->csize + 1 - fs->csect;
        sectorsToRead = count;
        if (sectorsToRead > remSectInClust)
            sectorsToRead = remSectInClust;
 
        doRead = 0;
        if ((fs->curr_clust != prevClst + 1) &&
            (prevClst != 0xFFFF))
        {
            doRead = 1;
        }
        else if ((sectorsToRead + adjacentSectors) > 0x100)  // diskio_asm doesn't handle requests for more than 256 sectors
        {
            doRead = 1;
        }
        prevClst = fs->curr_clust;
   
        // Execute the sectors read when we've accumulated 256 sectors or
        // the cluster linearity has been broken
        if (doRead) 
        {
            dr = p_disk_read_sectors(praddr&0xFFFF, firstSector, praddr>>16, adjacentSectors);
            if (dr) {
                fs->flag = 0;
                return (dr == RES_WRPRT/*STRERR*/) ? FR_STREAM_ERR : FR_DISK_ERR;
            }
            firstSector = fs->dsect;
            praddr += (DWORD)adjacentSectors << 9;           
            adjacentSectors = 0;
        }
        
        adjacentSectors += sectorsToRead;
        
        fs->fptr += sectorsToRead << 9;
        fs->csect += sectorsToRead-1;
        count -= sectorsToRead;
    }

    if (adjacentSectors)
    {
        dr = p_disk_read_sectors(praddr&0xFFFF, firstSector, praddr>>16, adjacentSectors);
    }
    
    return FR_OK;
}

/*See tools/crc16/ for more*/
#define BIT_CHAR(in)         ((in) >> 3)
#define BITS_TO_CHARS(in)   ((((in) - 1) >> 3) + 1)
const short b7_set = 128;			/*patch over sdcc's symbol resolving bug*/
const short num_bits = 64;			/*patch over sdcc's symbol resolving bug*/

void ba_shr(unsigned char* a,short shifts)
{
    short i,j,k;
    short mask;
    short chars;

	chars = shifts >> 3;   
    shifts = shifts & 7;
	k = BIT_CHAR(num_bits - 1);

    if(chars > 0)
    {
        #if 0
		short i,j;
		i = k;
		while(1)
		{
			j = i - chars;
			if(!j) { break; }
			a[i] = a[j];
			--i;
		}
		#else
		for(i = k; (i - chars) >= 0; i--) { a[i] = a[i - chars]; }
		#endif
        for(; chars > 0; chars--) { a[chars - 1] = 0; }
    }

    for(i = 0; i < shifts; i++)
    {
        for(j = k; j > 0; j--)
        {
            a[j] >>= 1;
            if (a[j - 1] & 0x01) { a[j] |= b7_set; }
        }

        a[0] >>= 1;
    }

    i = num_bits & 7;
	if(0 == i) { return; }

	mask = 255 << (unsigned char)((short)(8 - i));
	a[k] &= mask;
}

void ba_shl(unsigned char* a,unsigned char shifts)
{
    short i,j,k;
	short chars;

	chars = shifts >> 3;  
    shifts = shifts & 7;    

    if(chars)
    {
		k = BITS_TO_CHARS(num_bits);
		#if 1
        for(i = 0; (i + chars) <  k; i++) { a[i] = a[i + chars]; }
		#else
		for(i = 0,j = chars; j <  k; i++,j++) { a[i] = a[j]; }
		#endif
        for(i = BITS_TO_CHARS(num_bits); chars > 0; chars--) { a[i - chars] = 0; }
    }

	k = BIT_CHAR(num_bits - 1);

    for(i = 0; i < shifts; i++)
    {
        for(j = 0; j < k; j++)
        {
            a[j] <<= 1;

            if (a[j + 1] & b7_set) { a[j] |= 0x01; }
        }

        a[k] <<= 1;
    }
}

void ba_xor(unsigned char* a,const unsigned char* b,const unsigned char* c)
{
    short i,j;

	j = BITS_TO_CHARS(num_bits);
    for(i = 0; i < j; i++) { a[i] = b[i] ^ c[i]; }
}

void ba_assign(unsigned char* a,unsigned char* b)
{
	unsigned char i;

	for(i = 0;i<8;i++){*(a++) = *(b++);}
}

/************* BIT STREAM CODE BEGIN *************/
void poly_set(unsigned char* poly,short a_in)
{	
	unsigned char a = (unsigned char)a_in;
	/*poly[0] = 0x00;*/ poly[1] = a;					/*000a*/
	/*poly[2] = 0x00;   poly[3] = 0x00;	*/				/*0000*/
	/*poly[4] = 0x00;*/ poly[5] = a << 4;				/*00a0*/
	/*poly[6] = 0x00;*/ poly[7] = a;					/*000a*/
}

void bc_xori(unsigned char* bca,unsigned char* bcd,short imm0,short imm1,short imm2)
{
	if( (imm0 ^ imm1) & imm2 )						/*(00 00 00 00 00 00 00 aa ^ 00 00 00 00 00 00 00 0b) & 00 00 00 00 00 00 00 0c*/
	{
		poly_set(bcd,imm2);							/*d = p[imm2]*/
		ba_xor(bca,bca,bcd);						/*a = (a ^ d)*/
	}
}

void crc16_bc(const unsigned char* sector)
{
	short i,j,nybble;
	short lsb;
	unsigned short len;
	unsigned char* bca;
	unsigned char* bcb;
	unsigned char* bcd;

	bca = (unsigned char*)0XDA08;	/*output*/
	bcb = (unsigned char*)0xDA10;
	bcd = (unsigned char*)0xDA18;

	mem_set(bca,0,8);				/*reset*/
	mem_set(bcd,0,8);				/*optimize poly_set*/

	len = 1024;

	for(i = 0;i<len;i++)
	{
        if(i & 1) 
			{ nybble = (sector[i >> 1] & 0x0f); }
        else
            { nybble = (sector[i >> 1] >> 4); }

		ba_assign(bcb,bca);
		ba_shr(bcb,60);
		ba_shl(bca,4);

		lsb = bcb[7];/*00 00 00 00 00 00 00 aa*/
		j = 1;
		do
		{
			bc_xori(bca,bcd,lsb,nybble,j);
			j <<= 1;
		}
		while(j <= 8);
	}
}
/************* BIT STREAM CODE END *************/

FRESULT pf_write_sector(void* src)
{
    DRESULT dr;
    CLUST clst;
    DWORD sect;
    FATFS* fs = FatFs;
    DRESULT (*p_disk_writep)(BYTE*,DWORD) = pfn_disk_writep;

    if(!fs) {return FR_NOT_ENABLED;}
    if(!(fs->flag & FA_READ)){return FR_INVALID_OBJECT;}

	dr = FR_OK;/*Remove warnings*/

	if((fs->fptr & 511) == 0) {                /* On the sector boundary? */
        if(((fs->fptr >> 9) & (fs->csize - 1)) == 0) { /* On the cluster boundary? */
            clst = (fs->fptr == 0) ?            /* On the top of the file? */
                fs->org_clust : get_fat(fs->curr_clust);
            if (clst <= 1) {goto pf_write_sector_abort;}
            fs->curr_clust = clst;              /* Update current cluster */
            fs->csect = 0;                      /* Reset sector offset in the cluster */
        }
        sect = clust2sect(fs->curr_clust);      /* Get current sector */
        if (!sect) {goto pf_write_sector_abort;}
        sect += fs->csect;
        fs->dsect = sect;
        fs->csect++;                            /* Next sector address in the cluster */
    }
	crc16_bc((const unsigned char*)src);
    dr = p_disk_writep((BYTE*)src,fs->dsect);
    fs->fptr += 512;

    if(FR_OK == dr) { return FR_OK; }

pf_write_sector_abort:
	fs->flag = 0;
	return dr;
}

#if 0
FRESULT pf_write_sectors(void* src,WORD count)
{
	DWORD addr;
	FATFS *fs = FatFs;
	FRESULT fr;
	BYTE* p;

	if(!fs) {return FR_NOT_ENABLED;}

	p = src;
	addr = fs->fptr;
	if(addr >= fs->fsize){return FR_DISK_ERR;}

	while(count > 0)
	{
		fr = pf_write_sector((void*)p);
		if(fr != FR_OK){return fr;}
		p += 512;
		addr += 512;
		if(addr >= fs->fptr){pf_lseek(fs->fsize);break;}
		fr = pf_lseek(addr);
		if(fr != FR_OK){return fr;}
		count--;
	}

	return FR_OK;
}
#endif

#if _USE_LSEEK
/*-----------------------------------------------------------------------*/
/* Seek File R/W Pointer                                                 */
/*-----------------------------------------------------------------------*/

FRESULT pf_lseek (
    DWORD ofs       /* File pointer from top of file */
)
{
    CLUST clst;
    DWORD bcs, nsect, ifptr;
    DWORD bo1,bo2;
    FATFS *fs = FatFs;


    if (!fs) return FR_NOT_ENABLED;     /* Check file system */
    if (!(fs->flag & FA_READ))
            return FR_INVALID_OBJECT;

// DEBUG
//pffclst = 0;

    if (ofs > fs->fsize) ofs = fs->fsize;   /* Clip offset with the file size */
    ifptr = fs->fptr;
    fs->fptr = 0;

    if (ofs) {
        bcs = (DWORD)fs->csize << 9;    /* Cluster size (byte) */
        bo1 = (ofs - 1) / bcs;
        bo2 = (ifptr - 1) / bcs;
        //if ((ifptr != 0) &&   (bo1 >= bo2))
        if (ifptr)
        {
            if (bo1 >= bo2)
            {
                /* When seek to same or following cluster, */
                fs->fptr = (ifptr - 1) & ~(bcs - 1);    /* start from the current cluster */
                ofs -= fs->fptr;
                clst = fs->curr_clust;
                    //pffclst = 1;
            } else {                            /* When seek to back cluster, */
                clst = fs->org_clust;           /* start from the first cluster */
                fs->curr_clust = clst;
                    //pffclst = 2;
            }
        }
        else
        {
                clst = fs->org_clust;           /* start from the first cluster */
                fs->curr_clust = clst;
                    //pffclst = 3;
        }

//pffbcs = ifptr;

        while (ofs > bcs) {             /* Cluster following loop */

            clst = get_fat(clst);   /* Follow cluster chain if not in write mode */
            if ((clst <= 1) || (clst >= fs->max_clust)) {
                fs->flag = 0; return FR_DISK_ERR + 1;
            }
            fs->curr_clust = clst;
            fs->fptr += bcs;
            ofs -= bcs;
        }
        fs->fptr += ofs;
        fs->csect = (BYTE)(ofs >> 9) + 1;   /* Sector offset in the cluster */
        nsect = clust2sect(clst);   /* Current sector */
        if (!nsect) {
            fs->flag = 0; return FR_DISK_ERR;
        }
        fs->dsect = nsect + fs->csect - 1;
    }

    return FR_OK;
}
#endif


#if _USE_DIR
/*-----------------------------------------------------------------------*/
/* Create a Directroy Object                                             */
/*-----------------------------------------------------------------------*/

FRESULT pf_opendir (
    DIR *dj,            /* Pointer to directory object to create */
    const char *path    /* Pointer to the directory path */
)
{
    FRESULT res;
    BYTE dir[32];
    NAMEBUF(sfn, lfn);
    FATFS *fs = FatFs;

    if (!fs) {              /* Check file system */
        res = FR_NOT_ENABLED;
    } else {
        INITBUF((*dj), sfn, lfn);
        fs->buf = dir;
        dj->fn = sfn; //sp;  
        res = follow_path(dj, path);            /* Follow the path to the directory */
        if (res == FR_OK) {                     /* Follow completed */
            if (dir[0]) {                       /* It is not the root dir */
                if (dir[DIR_Attr] & AM_DIR) {   /* The object is a directory */
                    dj->sclust =
#if _FS_FAT32
                    ((DWORD)LD_WORD(dir+DIR_FstClusHI) << 16) |
#endif
                    LD_WORD(dir+DIR_FstClusLO);
                } else {                        /* The object is not a directory */
                    res = FR_NO_PATH;
                }
            }
            if (res == FR_OK) {     
                res = dir_rewind(dj);           /* Rewind dir */
            }
        }
        if (res == FR_NO_FILE) res = FR_NO_PATH;
    }

    return res;
}




/*-----------------------------------------------------------------------*/
/* Read Directory Entry in Sequense                                      */
/*-----------------------------------------------------------------------*/

FRESULT pf_readdir (
    DIR *dj,            /* Pointer to the open directory object */
    FILINFO *fno        /* Pointer to file information to return */
)
{
    FRESULT res;
    BYTE dir[32];
    NAMEBUF(sfn,lfn);
    FATFS *fs = FatFs;

    if (!fs) {              /* Check file system */
        res = FR_NOT_ENABLED;
    } else {
        INITBUF((*dj), sfn, lfn);
        fs->buf = dir;
        dj->fn = sfn; //sp;
        if (!fno) {
            res = dir_rewind(dj);
        } else {
            res = dir_read(dj);
            if (res == FR_NO_FILE) {
                dj->sect = 0;
                res = FR_OK;
            }
            if (res == FR_OK) {             /* A valid entry is found */
                get_fileinfo(dj, fno);      /* Get the object information */
                res = dir_next(dj);         /* Increment index for next */
                if (res == FR_NO_FILE) {
                    dj->sect = 0;
                    res = FR_OK;
                }
            }
        }
    }

    return res;
}

#endif /* _FS_DIR */
/*
	Required for hacked-in file creation
	This is a modified FF version
*/
#if 0
FRESULT move_window (
	DWORD sector	
)
{
	DWORD wsect;
    FATFS *fs = FatFs;

	wsect = fs->dsect;
	if (wsect != sector) {	/* Changed current window */
		if (fs->wflag) {	/* Write back dirty window if needed */
			if (disk_write(fs->drive, fs->win, wsect, 1) != RES_OK)
				return FR_DISK_ERR;
			fs->wflag = 0;
			if (wsect < (fs->fatbase + fs->sects_fat)) {	/* In FAT area */
				BYTE nf;
				for (nf = fs->n_fats; nf > 1; nf--) {	/* Refrect the change to all FAT copies */
					wsect += fs->sects_fat;
					disk_write(fs->drive, fs->win, wsect, 1);
				}
			}
		}
		if (sector) {
			if (disk_read(fs->drive, fs->win, sector, 1) != RES_OK)
				return FR_DISK_ERR;
			fs->dsect = sector;
		}
	}

	return FR_OK;
}

FRESULT pf_dir_register_sfn (	/* FR_OK:Successful, FR_DENIED:No free entry or too many SFN collision, FR_DISK_ERR:Disk error */
	DIR *dj,				/* Target directory with object name to be created */
	BYTE force
)
{
	FRESULT res;
	BYTE c, *dir;

	res = dir_seek(dj, 0);
	if (res == FR_OK) {
		do {	/* Find a blank entry for the SFN */
			res = move_window(dj->fs, dj->sect);
			if (res != FR_OK) break;
			c = *dj->dir;
			if (c == 0xE5 || c == 0) break;	/* Is it a blank entry? */
			res = dir_next(dj, TRUE);		/* Next entry with table streach */
		} while (res == FR_OK);
	}


	if (res == FR_OK) {		/* Initialize the SFN entry */
		res = move_window(dj->fs, dj->sect);
		if (res == FR_OK) {
			dir = dj->dir;
			mem_set(dir, 0, 32);		/* Clean the entry */
			mem_cpy(dir, dj->fn, 11);	/* Put SFN */
			dir[DIR_NTres] = *(dj->fn+NS) & (NS_BODY | NS_EXT);	/* Put NT flag */
			dj->fs->wflag = 1;
		}
	}

	if(force)
	{
		dir[DIR_Attr] = 0;					/* Reset attribute */
		ps = get_fattime();
		ST_DWORD(dir+DIR_CrtTime, ps);		/* Created time */
		dj.fs->wflag = 1;
		dj->flag |= FA__WIP;/*FA__WRITTEN;*/
	}

	return res;
}

FRESULT pf_file_exists(const char* path)
{
	DIR entry;
	return follow_path(&entry,path) != FR_NO_FILE;
}

FRESULT pf_file_register_sfn (const char* path,BYTE force)/*Haaaaaack*/
{
	FRESULT res;
	DIR entry;

	BYTE* dir;

	res = follow_path(&entry,path);
	if(res == FR_NO_FILE)
	{
		res = dir_register_sfn(&entry,force);
		if(res != FR_OK) {return res;}
		dir = dj.dir;
	}
	else
	{
		dir = dj.dir;
		if (!dir || (dir[DIR_Attr] & (AM_RDO | AM_DIR)))	/* Cannot overwrite it (R/O or DIR) */
			{return FR_DENIED;}

		cl = ((DWORD)LD_WORD(dir+DIR_FstClusHI) << 16) | LD_WORD(dir+DIR_FstClusLO);	/* Get start cluster */
		ST_WORD(dir+DIR_FstClusHI, 0);	/* cluster = 0 */
		ST_WORD(dir+DIR_FstClusLO, 0);
		ST_DWORD(dir+DIR_FileSize, 0);	/* size = 0 */
		dj.fs->wflag = 1;
		ps = dj.fs->dsect;			/* Remove the cluster chain */
		if (cl) {
			res = remove_chain(dj.fs, cl);
			if (res) LEAVE_FF(dj.fs, res);
			dj.fs->last_clust = cl - 1;	/* Reuse the cluster hole */
		}
		res = move_window(dj.fs, ps);
		if (res != FR_OK) LEAVE_FF(dj.fs, res);
	}

	return res;
}

FRESULT dir_seek (
	DIR *dj,		
	WORD idx		
)
{
	DWORD clst;
	WORD ic;

	dj->index = idx;
	clst = dj->sclust;
	if ((clst == 1) || (clst >= dj->fs->max_clust))	 { return FR_INT_ERR; }
	if ((!clst) && (dj->fs->fs_type == FS_FAT32))	{ clst = dj->fs->dirbase; }


	if (clst == 0) {	/* Static table */
		dj->clust = clst;
		if (idx >= dj->fs->n_rootdir)		/* Index is out of range */
			{return FR_INT_ERR;}
		/* Sector# */
		dj->sect = (dj->fs->dirbase + idx) >> 4; /*dj->sect = dj->fs->dirbase + idx / (SS(dj->fs) / 32);	*/
	}
	else {				/* Dynamic table */
		/* Entries per cluster */
		ic = dj->fs->csize << 4; /*ic = SS(dj->fs) / 32 * dj->fs->csize;	*/
		while (idx >= ic) {	/* Follow cluster chain */
			clst = get_fat(dj->fs, clst);				/* Get next cluster */
			if (clst == 0xFFFFFFFF) {return FR_DISK_ERR;}	/* Disk error */
			if (clst < 2 || clst >= dj->fs->max_clust)	/* Reached to end of table or int error */
				{return FR_INT_ERR;}
			idx -= ic;
		}
		dj->clust = clst;
		dj->sect = (clust2sect(dj->fs, clst) + idx) >> 4;/*dj->sect = clust2sect(dj->fs, clst) + idx / (SS(dj->fs) / 32);*/	/* Sector# */
	}

	dj->dir = (dj->fs->win + (idx & 15) << 5); /*dj->dir = dj->fs->win + (idx % (SS(dj->fs) / 32)) * 32;*/	/* Ptr to the entry in the sector */

	return FR_OK;	/* Seek succeeded */
}
#endif


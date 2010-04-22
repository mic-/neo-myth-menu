
/*
    util68k lib - By conleon1988@gmail.com for ChillyWilly's DX myth menu
    http://code.google.com/p/neo-myth-menu/

    Special thanks to ChillyWilly for all the hints & support :D
*/

        .text
        .align 2

| unsigned short* utility_getFileExtW(unsigned short* src)
    .global utility_getFileExtW

utility_getFileExtW:
        movea.l 4(sp),a0
        movea.l a0,a1
1:
        tst.w   (a0)+
        bne.b   1b

        subq.l  #2,a0
        move.l  a0,d0 /*save addr - use this if not found*/
2:
        cmpi.w  #46,-(a0)
        beq.b   3f
        cmpa.l  a0,a1
        bne.b   2b
        rts
3:
        move.l  a0,d0 /*found*/
        rts

| char* utility_getFileExt(char* src)
    .global utility_getFileExt

utility_getFileExt:
        movea.l 4(sp),a0
        movea.l a0,a1
1:
        tst.b   (a0)+
        bne.b   1b

        subq.l  #1,a0
        move.l  a0,d0 /*save addr - use this if not found*/
2:
        cmpi.b  #46,-(a0)
        beq.b   3f
        cmpa.l  a0,a1
        bne.b   2b
        rts
3:
        move.l  a0,d0 /*found*/
        rts


| char* utility_w2cstrcpy(char* s1,const unsigned short* ws2)
    .global utility_w2cstrcpy

utility_w2cstrcpy:
        movea.l 4(sp),a0
        movea.l 8(sp),a1
        moveq   #0,d1
1:
        /*typecast*/
        move.w  (a1)+,d1
        beq.b   2f
        move.b  d1,(a0)+
        bra.b   1b
2:
        move.b  #0,(a0)
        move.l  a0,d0
        rts

| int utility_wstrcmp(const unsigned short* ws1,const unsigned short* ws2)
    .global utility_wstrcmp

utility_wstrcmp:
        movea.l 4(sp),a0        /*ws1*/
        movea.l 8(sp),a1        /*ws2*/
1:
        cmpi.w  #0,(a0)         /**ws1 == 0 ?*/
        beq.b   2f              /*break*/

        cmpi.w  #0,(a1)         /**ws2 == 0 ?*/
        beq.b   2f              /*break*/

        cmp.w   (a0)+,(a1)+     /* *ws1 == *ws2*/
        beq.b   1b              /* loop! */
2:
        cmpi.w  #0,(a0)         /*!*ws1*/
        beq.b   3f

        bra.b   4f              /* *ws1 */
3: /*ws1 < ws2*/
        cmpi.w  #0,(a1)
        bne.b   5f

        move.w  #0,d0
        bra.b   6f
4: /*ws1 > ws2*/
        cmpi.w  #0,(a1)
        bne.b   5f

        move.w  #1,d0
        bra.b   6f
5: /* ws1 - ws2*/
        move.w  (a0),d0
        sub.w   (a1),d0
6:
        rts

| unsigned short* utility_wstrcpy(unsigned short* ws1,const unsigned short* ws2)
    .global utility_wstrcpy

utility_wstrcpy:
        movea.l 4(sp),a0 /* dst */
        movea.l 8(sp),a1 /* src */
1:
        move.w  (a1)+,(a0)+ /* *ws1++ = *ws2++ */
        bne.b   1b          /* loop */
2:
        subq.l  #2,a0       /* *ws1 = 0 */
        move.l  a0,d0
        rts

| unsigned short* utility_c2wstrcat(unsigned short* ws1,const char* ws2)
    .global utility_c2wstrcat

utility_c2wstrcat:
        movea.l 4(sp),a0 /* dst */
        movea.l 8(sp),a1 /* src */
1:
        tst.w   (a0)+
        bne.b   1b

        subq.l  #2,a0
        moveq   #0,d1
2:
        /*append src*/
        move.b  (a1)+,d1
        move.w  d1,(a0)+
        bne.b   2b

        subq.l  #2,a0
        move.l  a0,d0
        rts

| unsigned short* utility_wstrcat(unsigned short* ws1,const unsigned short* ws2)
    .global utility_wstrcat

utility_wstrcat:
        movea.l 4(sp),a0 /* dst */
        movea.l 8(sp),a1 /* src */

1:
        tst.w   (a0)+
        bne.b   1b

        subq.l  #2,a0
2:
        move.w  (a1)+,(a0)+
        bne.b   2b

        subq.l  #2,a0
        move.l  a0,d0
        rts

| unsigned short* utility_c2wstrcpy(unsigned short* s1,const char* s2)
    .global utility_c2wstrcpy

utility_c2wstrcpy:
        movea.l 4(sp),a0
        movea.l 8(sp),a1
        moveq   #0,d1
1:
        move.b  (a1)+,d1
        move.w  d1,(a0)+
        bne.b   1b

        subq.l  #2,a0
        move.l  a0,d0
        rts

/*This version is faster if you put a big enough string(like 12++ (Suggested 24 & up)chars) ,
otherwise prefer the strlen */
| int utility_strlen2(const char* s)
    .global utility_strlen2

utility_strlen2:
        movea.l 4(sp),a0 /* s */
        movea.l a0,a1  /* s1 <- s : copy base address to avoid increasing result */
1:
        tst.b   (a0)+ /* *(s++) == '\0' ?*/
        bne.b   1b
2:
        move.l  a0,d0 /*end addr of string*/
        sub.l   a1,d0 /* len <- (end - start) */
        subq.l  #1,d0 /*normalize result because a0 gets increased before the jump*/

        rts

| int utility_wstrlen2(const unsigned short* s)
    .global utility_wstrlen2

utility_wstrlen2:
        movea.l 4(sp),a0 /* s */
        movea.l a0,a1  /* s1 <- s : copy base address to avoid increasing result */
1:
        tst.w   (a0)+ /* *(s++) == 0 ?*/
        bne.b   1b
2:
        move.l  a0,d0 /*end addr of string*/
        sub.l   a1,d0 /* len <- (end - start) */
        lsr.l   #1,d0 /* >>= 1*/
        subq.l  #1,d0 /*normalize result because a0 gets increased before the jump*/

        rts

/*This should be much faster than string.h strlen()*/
| int utility_strlen(const char* s)
    .global utility_strlen

utility_strlen:
        movea.l 4(sp),a0 /* s */
        moveq.l #0,d0 /* init result */
1:
        tst.b   (a0)+ /* *(s++) == '\0' ?*/
        beq.b   2f          /* exit ! */

        addq.l  #1,d0 /*r <- r + 1*/
        bra.b   1b          /* loop */
2:
        rts

| int utility_wstrlen(const unsigned short* s)
    .global utility_wstrlen

utility_wstrlen:
        movea.l 4(sp),a0 /* s */
        moveq.l #0,d0 /* init result */
1:
        tst.w   (a0)+ /* *(s++) == 0 ?*/
        beq.b   2f          /* exit ! */

        addq.l  #1,d0 /*r <- r + 1*/
        bra.b   1b          /* loop */
2:
        rts

| int utility_isMultipleOf(int base,int n)
    .global utility_isMultipleOf

utility_isMultipleOf:
        /*!(base & (n - 1))*/
        move.l  8(sp),d1 /* n */

        cmpi.w  #1,d1
        blt.b   1f

        move.l  4(sp),d0 /* base */
        subq    #1,d1 /* n <- n - 1*/

        and     d1,d0 /* base & n*/
        not     d0
1:
        rts

/* ###### B/W/L MEMCPY 68K CPU IMPLEMENTATION ###### */
| void utility_bmemcpy(const unsigned char* src,unsigned char* dst,int len)
        .global utility_bmemcpy
utility_bmemcpy:
        /*0 ~ 4(sp) <- RA*/

        movea.l 4(sp),a0 /* src */
        movea.l 8(sp),a1 /* dst */
        move.l  12(sp),d0 /* length */

        bra.b   2f
1:
        move.b  (a1)+,(a0)+ /* *dst <- *src */
2:
        dbra    d0,1b /* !0 ? loop */

        rts

| void utility_wmemcpy(const unsigned char* src,unsigned char* dst,int len)
        .global utility_wmemcpy
utility_wmemcpy:
        /*0 ~ 4(sp) <- RA*/

        movea.l 4(sp),a0 /* src */
        movea.l 8(sp),a1 /* dst */
        move.l  12(sp),d0 /* length */

        lsr.w   #1,d0 /* >>= 1*/

        bra.b   2f
1:
        move.w  (a1)+,(a0)+ /* *dst <- *src */
2:
        dbra    d0,1b /* !0 ? loop */

        rts

| void utility_lmemcpy(const unsigned char* src,unsigned char* dst,int len)
        .global utility_lmemcpy
utility_lmemcpy:
        /*0 ~ 4(sp) <- RA*/

        movea.l 4(sp),a0 /* src */
        movea.l 8(sp),a1 /* dst */
        move.l  12(sp),d0 /* length */

        lsr.w   #2,d0 /* >>= 2*/

        bra.b   2f
1:
        move.l  (a1)+,(a0)+ /* *dst <- *src */
2:
        dbra    d0,1b /* !0 ? loop */

        rts


/* ###### B/W/L MEMSET 68K CPU IMPLEMENTATION ###### */
| void utility_bmemset(unsigned char* data,unsigned char c,int len)
        .global utility_bmemset
utility_bmemset:
        /*0 ~ 4(sp) <- RA*/

        movea.l 4(sp),a0 /* src */
        move.b  8(sp),d0 /* replace with this */
        move.l  9(sp),d1 /* length */

        bra.b   2f
1:
        move.b  d0,(a0)+ /* *dst <- *src */
2:
        dbra    d1,1b /* !0 ? loop */

        rts

| void utility_wmemset(unsigned char* data,short int c,int len)
        .global utility_wmemset
utility_wmemset:
        /*0 ~ 4(sp) <- RA*/
        movea.l 4(sp),a0 /* src */
        move.w  8(sp),d0 /* replace with this */
        move.l  10(sp),d1 /* length */

        lsr.w   #1,d1 /* >>= 1*/

        bra.b   2f
1:
        move.w  d0,(a0)+ /* *dst <- *src */
2:
        dbra    d1,1b /* !0 ? loop */

        rts

| void utility_lmemset(unsigned char* data,int c,int len)
        .global utility_lmemset
utility_lmemset:
        /*0 ~ 4(sp) <- RA*/
        movea.l 4(sp),a0 /* src */
        move.l  8(sp),d0 /* replace with this */
        move.l  12(sp),d1 /* length */

        lsr.w   #2,d1 /* >>= 2*/

        bra.b   2f
1:
        move.l  d0,(a0)+ /* *dst <- *src */
2:
        dbra    d1,1b /* !0 ? loop */

        rts

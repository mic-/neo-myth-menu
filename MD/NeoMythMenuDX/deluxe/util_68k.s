|Todo : optimize this mess
        .text
        .align 2

|int utility_logtwo(int x)
        .global utility_logtwo
utility_logtwo:
		move.l 4(sp),d1
		moveq.l #-1,d0

0:
		tst.l d1
		ble.b 1f
		addq.l #1,d0
		lsr.l #1,d1
		bra 0b
1:
		rts

|int utility_word_cmp(const void* dst, const void* src, int cnt)
|Returns 0 on success
        .global utility_word_cmp
utility_word_cmp:
		movem.l 4(sp),a0-a1
		move.l  12(sp),d1
		
		moveq	#0,d0
		lsr.l	#1,d1
		subq.l	#1,d1
0:
		move.w  (a0)+,d0
		sub.w	(a1)+,d0
		dbne	d1,0b

|		Check both count + delta. Non zero = failure
		ext.l	d0
		addq.w	#1,d1
		add.l	d1,d0

		rts

|int utility_memcmp (const void* dst, const void* src, int cnt)
        .global utility_memcmp
utility_memcmp:
		movea.l 4(sp),a0
		movea.l 8(sp),a1
		move.l  12(sp),d1
		moveq   #0,d0
0:
		tst.l   d1
        ble.b   1f

		subq.l  #1,d1
		move.b  (a0)+,d0
		sub.b   (a1)+,d0
        beq.b   0b
1:
        ext.w   d0
        ext.l   d0
		rts

| unsigned short* utility_getFileExtW(unsigned short* src)
    .global utility_getFileExtW

utility_getFileExtW:
        movea.l 4(sp),a0
        movea.l a0,a1
1:
        tst.w   (a0)+
        bne.b   1b

        subq.l  #2,a0
        move.l  a0,d0
2:
        cmpi.w  #46,(a0)
        beq.b   3f
        subq.l  #2,a0
        cmpa.l  a0,a1
        bne.b   2b
        rts
3:
        move.l  a0,d0
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
        move.l  a0,d0
2:
        cmpi.b  #46,-(a0)
        beq.b   3f
        cmpa.l  a0,a1
        bne.b   2b
        rts
3:
        move.l  a0,d0
        rts


| char* utility_w2cstrcpy(char* s1,const unsigned short* ws2)
    .global utility_w2cstrcpy

utility_w2cstrcpy:
        |movea.l 4(sp),a0
        |movea.l 8(sp),a1
		movem.l 4(sp),a0-a1
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
        |movea.l 4(sp),a0
        |movea.l 8(sp),a1
		movem.l 4(sp),a0-a1
		moveq   #0,d0
0:
		tst.w   (a0)
        ble.b   1f

		tst.w   (a1)
        ble.b   1f

		move.w  (a0)+,d0
		sub.w   (a1)+,d0
        beq.b   0b
1:
        ext.w   d0
        ext.l   d0
		rts

| unsigned short* utility_wstrcpy(unsigned short* ws1,const unsigned short* ws2)
    .global utility_wstrcpy

utility_wstrcpy:
        |movea.l 4(sp),a0
        |movea.l 8(sp),a1
		movem.l 4(sp),a0-a1
1:
        move.w  (a1)+,(a0)+
        bne.b   1b
2:
        subq.l  #2,a0
        move.l  a0,d0
        rts

| char* utility_strcpy(char* ws1,const char* ws2)
    .global utility_strcpy

utility_strcpy:
        |movea.l 4(sp),a0
        |movea.l 8(sp),a1
		movem.l 4(sp),a0-a1
1:
        move.b  (a1)+,(a0)+
        bne.b   1b
2:
        subq.l  #1,a0
        move.l  a0,d0
        rts

| unsigned short* utility_c2wstrcat(unsigned short* ws1,const char* ws2)
    .global utility_c2wstrcat

utility_c2wstrcat:
        |movea.l 4(sp),a0
        |movea.l 8(sp),a1
		movem.l 4(sp),a0-a1
1:
        tst.w   (a0)+
        bne.b   1b

        subq.l  #2,a0
        moveq   #0,d1
2:
        /*append src*/
        move.b  (a1)+,d1
        move.w  d1,(a0)+
        dbra	d1,2b

        subq.l  #2,a0
        move.l  a0,d0
        rts

| unsigned short* utility_wstrcat(unsigned short* ws1,const unsigned short* ws2)
    .global utility_wstrcat

utility_wstrcat:
        |movea.l 4(sp),a0
        |movea.l 8(sp),a1
		movem.l 4(sp),a0-a1

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

| char* utility_strcat(char* s1,const char* s2)
    .global utility_strcat

utility_strcat:
        |movea.l 4(sp),a0
        |movea.l 8(sp),a1
		movem.l 4(sp),a0-a1

1:
        tst.b   (a0)+
        bne.b   1b

        subq.l  #1,a0
2:
        move.b  (a1)+,(a0)+
        bne.b   2b

        subq.l  #1,a0
        move.l  a0,d0
        rts

| char* utility_strncat(char* s1,const char* s2,int n)
    .global utility_strncat

utility_strncat:
		movem.l 4(sp),a0-a1
		move.l 12(sp),d1

1:
        tst.b   (a0)+
        bne.b   1b

        subq.l  #1,a0
2:
		tst.l d1
		ble.b 3f

		subq.l #1,d1

        move.b  (a1)+,(a0)+
        bne.b   2b
3:
        subq.l  #1,a0
        move.l  a0,d0
        rts

| unsigned short* utility_c2wstrcpy(unsigned short* s1,const char* s2)
    .global utility_c2wstrcpy

utility_c2wstrcpy:
        |movea.l 4(sp),a0
        |movea.l 8(sp),a1
		movem.l 4(sp),a0-a1
        moveq   #0,d1
0:
        move.b  (a1)+,d1
        move.w  d1,(a0)+
        dbra	d1,0b

        subq.l  #2,a0
        move.l  a0,d0
        rts

/*This version is faster if you put a big enough string(like 12++ (Suggested 24 & up)chars) ,
otherwise prefer the strlen */
| int utility_strlen2(const char* s)
    .global utility_strlen2

utility_strlen2:
        movea.l 4(sp),a0
        movea.l a0,a1
1:
        tst.b   (a0)+
        bne.b   1b
2:
        move.l  a0,d0
        sub.l   a1,d0
        subq.l  #1,d0

        rts

| int utility_wstrlen2(const unsigned short* s)
    .global utility_wstrlen2

utility_wstrlen2:
        movea.l 4(sp),a0
        movea.l a0,a1
1:
        tst.w   (a0)
        bne.b   1b
2:
        move.l  a0,d0
        sub.l   a1,d0
        lsr.l   #1,d0
        subq.l  #1,d0

        rts

/*This should be much faster than string.h strlen()*/
| int utility_strlen(const char* s)
    .global utility_strlen

utility_strlen:
        movea.l 4(sp),a0
        moveq.l #0,d0
1:
        tst.b   (a0)+
        beq.b   2f

        addq.l  #1,d0
        bra.b   1b
2:
        rts

| int utility_wstrlen(const unsigned short* s)
    .global utility_wstrlen

utility_wstrlen:
        movea.l 4(sp),a0
        moveq.l #0,d0
1:
        tst.w   (a0)+
        beq.b   2f

        addq.l  #1,d0
        bra.b   1b
2:
        rts

| int utility_isMultipleOf(int base,int n)
    .global utility_isMultipleOf

utility_isMultipleOf:
        /*!(base & (n - 1))*/
        move.l  4(sp),d0
        move.l  8(sp),d1

        cmpi.w  #1,d1
        blt.b   1f

        subq    #1,d1

        and     d1,d0
        not     d0
1:
        rts

| void utility_memcpy(void* dst,const void* src,int len)
        .global utility_memcpy
utility_memcpy:
		movem.l	4(sp),a0-a1
        move.l  12(sp),d0
		subq.l	#1,d0
1:
        move.b  (a1)+,(a0)+
2:
        dbra    d0,1b

        rts

| void utility_memcpy16(void* dst,const void* src,int len)
        .global utility_memcpy16
utility_memcpy16:
		movem.l	4(sp),a0-a1
        move.l  12(sp),d0

		lsr.l	#1,d0
        subq.l	#1,d0
1:
        move.w  (a1)+,(a0)+
2:
        dbra    d0,1b

        rts

| void utility_memcpy_entry_block(void* dst,const void* src)
        .global utility_memcpy_entry_block
utility_memcpy_entry_block:
		movem.l	4(sp),a0-a1

		|16 bytes (long-writes are slower , actually :) )
        move.w  (a1)+,(a0)+
        move.w  (a1)+,(a0)+
        move.w  (a1)+,(a0)+
        move.w  (a1)+,(a0)+
        move.w  (a1)+,(a0)+
        move.w  (a1)+,(a0)+
        move.w  (a1)+,(a0)+
        move.w  (a1),(a0)

        rts

| void utility_memset(void* dst,int c,int len)
        .global utility_memset
utility_memset:

        movea.l	4(sp),a0
        movem.l	8(sp),d0-d1
		subq.l	#1,d1
1:
        move.b  d0,(a0)+
2:
        dbra    d1,1b

        rts

| void utility_memset_psram(void* dst,int c,int len)
        .global utility_memset_psram
utility_memset_psram:

        movea.l	4(sp),a0
        movem.l	8(sp),d0-d1
		subq.l	#1,d1
1:
        move.w  d0,(a0)+
2:
        dbra    d1,1b

        rts



 
multi_sd_to_myth_psram: |quad version
        move.l  12(sp),d0               /* buf = pstart + offset */
        move.w  #20,d1
        lsr.l   d1,d0                   /* bank = pstart / 1MB  */
        move.w  d0,PRAM_BIO(a1)         /* set the neo myth psram bank register */

        move.l  12(sp),d0               /* buf */
        andi.l  #0x0FFFFE,d0            /* offset inside sram space (bank was set to closest 1MB) */
        ori.l   #0x200000,d0            /* sram space access */
        movea.l d0,a0
        lea     0x6060.w,a1
        move.l  16(sp),d3               /* count */
        subq.w  #1,d3
		movem.l	d4-d7,-(sp)
0:
        move.w  #1023,d0
1:
        moveq   #1,d1
        and.w   (a1),d1
        dbeq    d0,1b
        bne.w   9f                      /* timeout */

		moveq   #63,d0					|moveq	#31,d0
		move.l	d3,-(sp)
2:
|.rept 2
		move.l	#0x000f000f,d2
		move.l	d2,d3
		move.l	d3,d4
		move.l	d4,d5
		move.l	d5,d6
		move.l	d6,d7
		
        move.w  (a1),d1
        and.w   (a1),d2
        lsl.b   #4,d1
        or.b    d2,d1
		swap	d2
        and.w   (a1),d2
        lsl.w   #4,d1
        or.b    d2,d1
        and.w   (a1),d3
        lsl.w   #4,d1
        or.b    d3,d1
		swap	d3
        move.w  d1,d2				
		swap	d2
        move.w  (a1),d1
        and.w   (a1),d3
        lsl.b   #4,d1
        or.b    d3,d1
        and.w   (a1),d4
        lsl.w   #4,d1
        or.b    d4,d1
        swap	d4
        and.w   (a1),d4
        lsl.w   #4,d1
        or.b    d4,d1
        move.w  d1,d2				
		move.l	d2,(a0)+
        move.w  (a1),d1
        and.w   (a1),d5
        lsl.b   #4,d1
        or.b    d5,d1
		swap.w	d5
        and.w   (a1),d5
        lsl.w   #4,d1
        or.b    d5,d1
        and.w   (a1),d6
        lsl.w   #4,d1
        or.b    d6,d1
        move.w  d1,d2				
		swap	d2
		swap.w	d6
        move.w  (a1),d1
        and.w   (a1),d6
        lsl.b   #4,d1
        or.b    d6,d1
        and.w   (a1),d7
        lsl.w   #4,d1
        or.b    d7,d1	
		swap	d7
        and.w   (a1),d7
        lsl.w   #4,d1
        or.b    d7,d1
		move.w	d1,d2
        move.l  d2,(a0)+				

|.endr
        dbra    d0,2b

        /* throw away CRC */
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)
		tst.w	(a1)             /* end bit */

		move.l	(sp)+,d3
        dbra    d3,0b

        lea     0xA10000,a1
        move.w  #0x0080,GBAC_LIO(a1)
        move.w  #0x0000,PRAM_BIO(a1)    /* set psram to bank 0 */
		movem.l	(sp)+,d4-d7
        movem.l (sp)+,d2-d3
        moveq   #1,d0                   /* TRUE */
        rts
9:
        lea     0xA10000,a1
        move.w  #0x0080,GBAC_LIO(a1)
        move.w  #0x0000,PRAM_BIO(a1)    /* set psram to bank 0 */
		movem.l	(sp)+,d4-d7
        movem.l (sp)+,d2-d3
        moveq   #0,d0                   /* FALSE */
        rts

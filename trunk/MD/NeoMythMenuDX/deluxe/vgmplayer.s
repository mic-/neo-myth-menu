| SEGA MD VGM Player
| by _mic, adapted for Neo Myth by Chilly Willy

        .text

| void PlayVGM(void);
        .global PlayVGM
PlayVGM:
        movem.l d2-d7/a2-a6,-(sp)

        move.w  #6,0xA1300C     /* PRAM_BIO set to bank 6 */

        /* Allow the 68k to access the FM chip */
        move.w 	#0x100,0xA11100
        move.w 	#0x100,0xA11200

        lea     0x200000,a3     /* vgm song pointer */
        move.l	0x1c(a3),d0		/* Get loop offset */
        beq.b	0f              /* no loop offset */
        rol.w	#8,d0
        swap.w	d0
        rol.w 	#8,d0			/* Convert to big-endian */
        add.l 	#0x1c,d0
        bra.b	1f
0:
        move.w 	#0x40,d0		/* No loop offset - use the start of the song */
1:
        lea 	0(a3,d0.l),a5   /* song loop address */
        lea 	0x40(a3),a4     /* song start address */

        moveq.l	#0,d5

        move.l	0x2c(a3),d7     /* YM2612 clock */
        rol.w	#8,d7
        swap.w	d7
        rol.w	#8,d7
        cmp.l	#0x00720000,d7
        bmi.b	2f
        moveq.l	#1,d7
        bra.b	read_cmd
2:
        moveq.l	#0,d7

| main player loop

read_cmd:
        /* exit when C pressed */
        move.b  0xA10003,d0     /* - 1 c b r l d u */
        andi.w  #0x0020,d0      /* 0 0 0 0 0 0 0 0 0 0 c 0 0 0 0 0 */
        bne     9f

| reset YM2612
        lea     FMReset(pc),a5
        lea     0x00A00000,a0
        move.w  #0x4000,d1
        moveq   #26,d2
0:
        move.b  (a5)+,d1                /* FM reg */
        move.b  (a5)+,0(a0,d1.w)        /* FM data */
        nop
        nop
        dbra    d2,0b

| reset PSG
        lea     PSGReset(pc),a5
        lea     0x00C00000,a0
        move.b  (a5)+,0x0011(a0)
        move.b  (a5)+,0x0011(a0)
        move.b  (a5)+,0x0011(a0)
        move.b  (a5),0x0011(a0)
1:
        /* wait on C released */
        move.b  0xA10003,d0     /* - 1 c b r l d u */
        andi.w  #0x0020,d0      /* 0 0 0 0 0 0 0 0 0 0 c 0 0 0 0 0 */
        beq.b   1b

        movem.l (sp)+,d2-d7/a2-a6
        rts

| process next VGM command
9:
        move.b	(a4)+,d0		/* Read one byte from the VGM data */

        cmp.b	#0x70,d0
        bmi 	below_0x70
        cmp.b	#0x80,d0
        bmi 	wait_n_short

        cmp.b	#0x90,d0
        bmi		pcm_write
        cmp.b	#0xB0,d0
        bmi		pcm_write_pack1
        cmp.b	#0xD0,d0
        bmi		pcm_write_pack4
        cmp.b	#0xE0,d0
        beq		pcm_seek

below_0x70:
        cmp.b	#0x4C,d0
        beq		pcm_seek_0
        cmp.b	#0x4D,d0
        beq		pcm_write_pack3
        cmp.b	#0x4E,d0
        beq		pcm_write_pack2
        cmp.b	#0x4F,d0
        beq		pan
        cmp.b	#0x50,d0
        beq		psg_write
        cmp.b	#0x52,d0
        beq		ym2612_write_low
        cmp.b	#0x53,d0
        beq		ym2612_write_high
        cmp.b	#0x61,d0
        beq		wait_n
        cmp.b	#0x62,d0
        beq		wait_735
        cmp.b	#0x63,d0
        beq		wait_882
        cmp.b	#0x66,d0
        beq		data_end
        cmp.b	#0x67,d0
        beq		data_block
        /* not recognized, ignore */
        bra		read_cmd	

pan:
        move.b	(a4)+,d0
        bra		read_cmd

psg_write:
        move.b	(a4)+,d0
        move.b	d0,0xC00011
        move.b	d0,d1
        bra		read_cmd

ym2612_write_low:
        move.b	(a4)+,d0
        move.b	d0,0xA04000
        move.b	d0,d2
        nop
        move.b	(a4)+,d1
        move.b	d1,0xA04001
        bra		read_cmd

ym2612_write_high:
        move.b	(a4)+,d0
        move.b	d0,0xA04002
        move.b	d0,d2
        nop
        move.b	(a4)+,d1
        move.b	d1,0xA04003
        bra		read_cmd

wait_n_short:
        and.l	#15,d0
        addq.l	#1,d0
        jsr		delay_n_samples_short
        bra		read_cmd

wait_n:
        move.b	(a4)+,d0
        lsl.w	#8,d0
        move.b	(a4)+,d0
        rol.w	#8,d0
        sub.l   #24,d5	
        jsr 	delay_n_samples2
        bra		read_cmd

wait_735:
        move.w	#735,d0
        jsr		delay_n_samples2
        bra		read_cmd

wait_882:
        move.w	#882,d0
        jsr		delay_n_samples2
        bra		read_cmd

pcm_write:
        move.b	(a3)+,d1
        move.b	#0x2a,0xA04000
        move.b	d1,0xA04001
        sub.l	#136,d5
        and.l	#15,d0
        beq		read_cmd
        lea     read_cmd,a1
        bra     delay_n_samples_short2

pcm_write_pack1:
        move.b	(a4)+,d3
        and.l   #0xFF,d3
        addq.l	#1,d3
        move.l	d0,d4
        sub.l   #80,d5
pcm_write_pack1_loop:
        cmp.l	#0,d3
        beq		read_cmd
        move.b	(a3)+,d1
        move.b	#0x2a,0xA04000
        move.b	d1,0xA04001
        subq.l	#1,d3
        move.l	d4,d0
        sub.l   #112,d5
        and.l	#15,d0
        beq		pcm_write_pack1_loop
        lea     pcm_write_pack1_loop,a1
        bra     delay_n_samples_short2

pcm_write_pack2:
        move.b	(a4)+,d3
        and.l   #0xFF,d3	
        add.l	d3,d3
        sub.l   #80,d5
pcm_write_pack2_loop:
        cmp.l	#0,d3
        beq		read_cmd
        move.b	(a3)+,d1
        move.b	#0x2a,0xA04000
        move.b	d1,0xA04001
        subq.l	#1,d3
        btst	#0,d3
        beq		pcm_write_pack2_shift
        move.b	(a4)+,d4
pcm_write_pack2_shift:
        move.l	d4,d0
        lsr.b	#4,d4
        sub.l   #150,d5
        and.l	#15,d0
        beq		pcm_write_pack2_loop
        lea     pcm_write_pack2_loop,a1
        bra     delay_n_samples_short2

pcm_write_pack3:
        move.b  (a4)+,d4
pcm_write_pack5:
        move.b	(a4)+,d3
        and.l   #0xFF,d3
        and.l   #0xFF,d4
        addq.l	#1,d4
        sub.l   #80,d5
pcm_write_pack3_loop:
        cmp.l	#0,d4
        beq		read_cmd
        move.b	0(a2,d3.l),d0   /* get byte from dictionary */
        move.b	(a3)+,d1
        move.b	#0x2a,0xA04000
        move.b	d1,0xA04001
        subq.l	#1,d4
        add.b	#1,d3
        sub.l   #128,d5
        and.l	#15,d0
        beq		pcm_write_pack3_loop
        lea     pcm_write_pack3_loop,a1
        bra     delay_n_samples_short2

pcm_write_pack4:
        move.l	d0,d4
        and.l	#15,d4
        bra 	pcm_write_pack5

pcm_seek_0:
        move.l	a6,a3
        bra 	read_cmd

pcm_seek:
        move.b	(a4)+,d0
        lsl.l	#8,d0
        move.b	(a4)+,d1
        or.b	d1,d0
        lsl.l	#8,d0
        move.b	(a4)+,d1
        or.b	d1,d0
        lsl.l	#8,d0
        move.b	(a4)+,d1
        or.b	d1,d0
        rol.w	#8,d0
        swap.w	d0
        rol.w	#8,d0
        move.l	a6,a3
        add.l	d0,a3
        bra		read_cmd

data_block:
        move.b	(a4)+,d0
        cmp.b	#0x66,d0
        bne		read_cmd
        move.b	(a4)+,d0
        cmp.b	#0x01,d0
        beq 	read_dict
        cmp.b	#0x00,d0
        bne		read_cmd
        move.b	(a4)+,d0
        lsl.l	#8,d0
        move.b	(a4)+,d1
        or.b	d1,d0
        lsl.l	#8,d0
        move.b	(a4)+,d1
        or.b	d1,d0
        lsl.l	#8,d0
        move.b	(a4)+,d1
        or.b	d1,d0
        rol.w	#8,d0
        swap.w	d0
        rol.w	#8,d0
        move.l	a4,a6           /* set start of PCM data block */
        move.l	a6,a3           /* set current PCM sample */
        add.l	d0,a4
        bra		read_cmd

read_dict:
        lea 	4(a4),a2
        add.l	#260,a4         /* skip 256 byte dictionary */
        bra 	read_cmd

data_end:
        move.l	a5,a4           /* set current song pointer to song loop address */
        bra		read_cmd

delay_n_samples_short2:
        subq.l	#1,d0
        add.w	d0,d0
        add.w	d0,d0
        move.l	#shortwaits2,a0
        add.l	d0,a0
        add.l	(a0),d5
        sub.l 	#56,d5
delay_loop2:
        sub.l	#34,d5  
        bpl 	delay_loop2
        jmp 	(a1)

delay_n_samples_short3:
        subq.l	#1,d0
        add.w	d0,d0
        add.w	d0,d0
        move.l	#shortwaits,a0
        add.l	d0,a0
        add.l	(a0),d5
        sub.l 	#56,d5
delay_loop3:
        sub.l	#34,d5  
        bpl	    delay_loop3
        jmp 	(a1)
	
delay_n_samples_short:
        lsl.l   #2,d0
        move.l  d0,d1	/* d1 = n*4 */
        add.l   d0,d0	/* d0 = n*8 */
        add.l   d0,d1	/* d1 = n*12 */
        lsl.l   #2,d0	/* d0 = n*32 */
        add.l   d0,d1	/* d1 = n*44 */
        lsl.l   #2,d0	/* d0 = n*128 */
        add.l   d0,d1	/* d1 = n*172 */
        lsr.l   #7,d0	
        lsr.l   #2,d0	/* d0 = n/4 */
        add.l   d0,d1	/* d1 = n*172.25 */
        add.l   d1,d5
        sub.l   #112,d5
delay_loop:
		sub.l	#29,d5  /* 26,52 */
		bpl 	delay_loop
        rts

/* Delay n/44100 s using YM2612 Timer A */
delay_n_samples2:
        cmp.w	#0,d0
        beq		delay_done

        /* Multiply by 1.2 (approximated as 77/64) */
        move.l	d0,d1
        move.l	d0,d2
        lsl.l	#6,d0
        lsl.l	#4,d1
        sub.l	d2,d1
        sub.l	d2,d1
        sub.l	d2,d1
        add.l	d1,d0
        lsr.l	#6,d0
        move.l	d0,d2

delay_again:
        move.l	d2,d0
        and.l	#0xFFF00,d0
        beq		low_left
        move.l	#256,d0
        bra		okdelay
low_left:
        move.l	d2,d0
        and.w	#255,d0
okdelay:
        eor.w	#0x3ff,d0
        addq.l	#1,d0
        move.l	d0,d1
        lsr	    #2,d0
        and.w	#3,d1
        move.b	#0x24,0xA04000
        nop
        nop
        move.b	d0,0xA04001
        nop
        move.b	#0x25,0xA04000
        nop
        nop
        move.b	d1,0xA04001
        nop
        nop
        move.b	#0x27,0xA04000
        nop
        nop
        move.b	#0x15,0xA04001
        nop
check_timer:
		move.b	0xA04000,d0
		and.b	#1,d0
		beq 	check_timer
        sub.l	#256,d2
        bpl		delay_again
delay_done:
        rts

        .align  4
shortwaits:
        .long   172,345,517,689,862,1034,1206,1379,1551,1723,1896,2068,2240,2413,2585,2757
shortwaits2:
        .long   120,296,517,689,862,1034,1206,1379,1551,1723,1896,2068,2240,2413,2585,2757,0

FMReset:
        /* disable LFO */
        .byte   0,0x22
        .byte   1,0x00
        /* disable timer & set channel 6 to normal mode */
        .byte   0,0x27
        .byte   1,0x00
        /* all KEY_OFF */
        .byte   0,0x28
        .byte   1,0x00
        .byte   1,0x04
        .byte   1,0x01
        .byte   1,0x05
        .byte   1,0x02
        .byte   1,0x06
        .byte   1,0x03
        .byte   1,0x07
        /* disable DAC */
        .byte   0,0x2B
        .byte   1,0x00
        /* turn off channels */
        .byte   0,0xB4
        .byte   1,0x00
        .byte   0,0xB5
        .byte   1,0x00
        .byte   0,0xB6
        .byte   1,0x00
        .byte   2,0xB4
        .byte   3,0x00
        .byte   2,0xB5
        .byte   3,0x00
        .byte   2,0xB6
        .byte   3,0x00

PSGReset:
        .byte   0x9f
        .byte   0xbf
        .byte   0xdf
        .byte   0xff

        .align  2

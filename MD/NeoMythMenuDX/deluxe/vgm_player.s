| SEGA MD VGM Player
| by Chilly Willy, loosely based on mic's VGM Player

| NTSC: 53693175, /15 = 3579545, /7 = 7670454 => 7670454/44100 = 174 cycles/sample
|  PAL: 53203424, /15 = 3546895, /7 = 7600489 => 7600489/44100 = 172 cycles/sample
|
| we're going to use 173 so we don't have to worry about NTSC vs PAL ;)

        .macro  fetch_vgm reg
        cmpa.l  a3,a6           /* check for wrap around */
        bcs.b   9f
        suba.l  a2,a6           /* wrap bank */
        addq.w  #1,d7           /* next bank */
9:
        move.w  d7,0xA1300C     /* set PRAM_BIO */
        move.b  (a6)+,\reg
        .endm

        .macro  fetch_pcm reg
        cmpa.l  a3,a5           /* check for wrap around */
        bcs.b   9f
        suba.l  a2,a5           /* wrap bank */
        addq.w  #1,d6           /* next bank */
9:
        move.w  d6,0xA1300C     /* set PRAM_BIO */
        move.b  (a5)+,\reg
        .endm

        .macro  check_button
        move.b  0xA10003,d1     /* - 1 c b r l d u */
        andi.w  #0x0020,d1      /* 0 0 0 0 0 0 0 0 0 0 c 0 0 0 0 0 */
        beq.w   ExitVGM         /* exit when C pressed */
        .endm


        .text

ExitVGM:
| reset YM2612
        lea     FMReset(pc),a5
        lea     0xA00000,a0
        moveq   #6,d3
0:
        move.b  (a5)+,d0                /* FM reg */
        move.b  (a5)+,d1                /* FM data */
        moveq   #15,d2
1:
        move.b  d0,0x4000(a0)           /* FM reg */
        nop
        nop
        nop
        move.b  d1,0x4001(a0)           /* FM data */
        nop
        nop
        nop
        move.b  d0,0x4002(a0)           /* FM reg */
        nop
        nop
        nop
        move.b  d1,0x4003(a0)           /* FM data */
        addq.b  #1,d0
        dbra    d2,1b
        dbra    d3,0b

        move.w  #0x4000,d1
        moveq   #28,d2
2:
        move.b  (a5)+,d1                /* YM reg */
        move.b  (a5)+,0(a0,d1.w)        /* YM data */
        dbra    d2,2b

| reset PSG
        move.b  #0x9F,0xC00011
        move.b  #0xBF,0xC00011
        move.b  #0xDF,0xC00011
        move.b  #0xFF,0xC00011
9:
        /* wait on C released */
        move.b  0xA10003,d0     /* - 1 c b r l d u */
        andi.w  #0x0020,d0      /* 0 0 0 0 0 0 0 0 0 0 c 0 0 0 0 0 */
        beq.b   9b

        move.w  #0x2000,sr
        movem.l (sp)+,d2-d7/a2-a6
        rts


| void PlayVGM(void);
        .global PlayVGM
PlayVGM:
        movem.l d2-d7/a2-a6,-(sp)
        move.w  #0x2700,sr

        /* Allow the 68k to access the FM chip */
        move.w  #0x100,0xA11100
        move.w  #0x100,0xA11200

        move.w  #0,0xA1300C     /* set PRAM_BIO */
        lea     0x100000,a2     /* wrap length for bank */
        lea     0x300000,a3     /* bank limit */
        lea     0x200000,a6     /* current vgm ptr */
        moveq   #0,d5           /* accum cycles */
        moveq   #0,d6           /* pcm bank */
        moveq   #0,d7           /* vgm bank */

        move.l  0x1C(a6),d4     /* Get loop offset */
        beq.b   0f              /* no loop offset */
        ror.w   #8,d4
        swap.w  d4
        ror.w   #8,d4           /* Convert to big-endian */
        addi.l  #0x1C,d4
        bra.b   1f
0:
        move.w  #0x40,d4        /* No loop offset - use the start of the song */
1:
        move.l  8(a6),d0        /* version */
        ror.w   #8,d0
        swap    d0
        ror.w   #8,d0
        cmpi.l  #0x150,d0       /* >= v1.50? */
        bcs.b   2f              /* no, assume data always starts at offset 0x40 */
        move.l  0x34(a6),d0     /* VGM data offset */
        beq.b   2f              /* not set - use 0x40 */
        ror.w   #8,d0
        swap    d0
        ror.w   #8,d0
        addi.l  #0x34,d0
        bra.b   3f
2:
        moveq   #0x40,d0
3:
        move.l  d0,d7
        andi.l  #0x0FFFFF,d0
        ori.l   #0x200000,d0
        move.l  d0,a6           /* new current vgm ptr */
        swap    d7
        lsr.w   #4,d7
        andi.w  #7,d7           /* new vgm bank */

| main player loop - process next VGM command

read_cmd:
        moveq   #0,d0
        fetch_vgm d0            /* Read one byte from the VGM data */
        add.w   d0,d0
        move.w  cmd_table(pc,d0.w),d1
        jmp     cmd_table(pc,d1.w)      /* dispatch command - 92 cycles */

cmd_table:
        /* 00 - 0F */
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        /* 10 - 1F */
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        /* 20 - 2F */
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        .word   reserved_0 - cmd_table
        /* 30 - 3F */
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        /* 40 - 4F */
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table
        .word   reserved_1 - cmd_table /* Game Gear PSG stereo, write dd to port 0x06 */
        /* 50 - 5F */
        .word   write_psg - cmd_table  /* PSG (SN76489/SN76496) write value dd */
        .word   reserved_2 - cmd_table /* YM2413, write value dd to register aa */
        .word   write_fm0 - cmd_table  /* YM2612 port 0, write value dd to register aa */
        .word   write_fm1 - cmd_table  /* YM2612 port 1, write value dd to register aa */
        .word   reserved_2 - cmd_table /* YM2151, write value dd to register aa */
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        /* 60 - 6F */
        .word   reserved_2 - cmd_table
        .word   wait_nnnn - cmd_table  /* Wait n samples, n can range from 0 to 65535 */
        .word   wait_735 - cmd_table   /* Wait 735 samples (60th of a second) */
        .word   wait_882 - cmd_table   /* Wait 882 samples (50th of a second) */
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   end_data - cmd_table   /* end of sound data */
        .word   data_block - cmd_table /* data block: 0x67 0x66 tt ss ss ss ss (data) */
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        /* 70 - 7F wait n+1 samples */
        .word   wait_1 - cmd_table
        .word   wait_2 - cmd_table
        .word   wait_3 - cmd_table
        .word   wait_4 - cmd_table
        .word   wait_5 - cmd_table
        .word   wait_6 - cmd_table
        .word   wait_7 - cmd_table
        .word   wait_8 - cmd_table
        .word   wait_9 - cmd_table
        .word   wait_10 - cmd_table
        .word   wait_11 - cmd_table
        .word   wait_12 - cmd_table
        .word   wait_13 - cmd_table
        .word   wait_14 - cmd_table
        .word   wait_15 - cmd_table
        .word   wait_16 - cmd_table
        /* 80 - 8F YM2612 port 0 address 2A write from the data bank, then wait n samples */
        .word   write_pcm_wait_0 - cmd_table
        .word   write_pcm_wait_1 - cmd_table
        .word   write_pcm_wait_2 - cmd_table
        .word   write_pcm_wait_3 - cmd_table
        .word   write_pcm_wait_4 - cmd_table
        .word   write_pcm_wait_5 - cmd_table
        .word   write_pcm_wait_6 - cmd_table
        .word   write_pcm_wait_7 - cmd_table
        .word   write_pcm_wait_8 - cmd_table
        .word   write_pcm_wait_9 - cmd_table
        .word   write_pcm_wait_10 - cmd_table
        .word   write_pcm_wait_11 - cmd_table
        .word   write_pcm_wait_12 - cmd_table
        .word   write_pcm_wait_13 - cmd_table
        .word   write_pcm_wait_14 - cmd_table
        .word   write_pcm_wait_15 - cmd_table
        /* 90 - 9F */
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        /* A0 - AF */
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        /* B0 - BF */
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        .word   reserved_2 - cmd_table
        /* C0 - CF */
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        /* D0 - DF */
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        .word   reserved_3 - cmd_table
        /* E0 - EF */
        .word   seek_pcm - cmd_table   /* seek to offset dddddddd in PCM data bank */
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        /* F0 - FF */
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table
        .word   reserved_4 - cmd_table

reserved_0:
        bra     read_cmd        /* command has no args */

reserved_1:
        addq.l  #1,a6
        bra     read_cmd        /* command has one arg */

reserved_2:
        addq.l  #2,a6
        bra     read_cmd        /* command has two args */

reserved_3:
        addq.l  #3,a6
        bra     read_cmd        /* command has three args */

reserved_4:
        addq.l  #4,a6
        bra     read_cmd        /* command has four args */

write_psg:
        fetch_vgm d0
        move.b  d0,0xC00011
        subi.l  #92+56+42,d5
        bra     read_cmd

write_fm0:
        fetch_vgm d0
        move.b  d0,0xA04000
        fetch_vgm d0
        move.b  d0,0xA04001
        subi.l  #92+112+58,d5
        bra     read_cmd

write_fm1:
        fetch_vgm d0
        move.b  d0,0xA04002
        fetch_vgm d0
        move.b  d0,0xA04003
        subi.l  #92+112+58,d5
        bra     read_cmd

wait_1:
        addi.l  #173-118,d5
0:
        subi.l  #26,d5          /* 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_2:
        addi.l  #173*2-118,d5
0:
        subi.l  #26,d5          /* 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_3:
        addi.l  #173*3-118,d5
0:
        subi.l  #26,d5          /* 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_4:
        addi.l  #173*4-118,d5
0:
        subi.l  #26,d5          /* 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_5:
        addi.l  #173*5-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_6:
        addi.l  #173*6-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_7:
        addi.l  #173*7-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_8:
        addi.l  #173*8-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_9:
        addi.l  #173*9-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_10:
        addi.l  #173*10-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_11:
        addi.l  #173*11-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_12:
        addi.l  #173*12-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_13:
        addi.l  #173*13-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_14:
        addi.l  #173*14-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_15:
        addi.l  #173*15-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_16:
        addi.l  #173*16-118,d5
0:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   0b
        bra     read_cmd

wait_735:
        move.w  #735,d0
        subi.l  #92+8+16+10,d5
0:
        subq.w  #1,d0
        bcs     read_cmd

        addi.l  #173-42,d5
        bmi.b   0b
1:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   1b
        bra.b   0b

wait_882:
        move.w  #882,d0
        subi.l  #92+8+16+10,d5
0:
        subq.w  #1,d0
        bcs     read_cmd

        addi.l  #173-42,d5
        bmi.b   0b
1:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   1b
        bra.b   0b

wait_nnnn:
        fetch_vgm d0
        lsl.w   #8,d0
        fetch_vgm d0
        ror.w   #8,d0
        subi.l  #92+112+44+16+10,d5
0:
        subq.w  #1,d0
        bcs     read_cmd

        addi.l  #173-42,d5
        bmi.b   0b
1:
        check_button
        subi.l  #36+26,d5       /* 36 for check button, 16 for subi, 10 for bpl */
        bpl.b   1b
        bra.b   0b

write_pcm_wait_0:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     read_cmd

write_pcm_wait_1:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_1

write_pcm_wait_2:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_2

write_pcm_wait_3:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_3

write_pcm_wait_4:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_4

write_pcm_wait_5:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_5

write_pcm_wait_6:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_6

write_pcm_wait_7:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_7

write_pcm_wait_8:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_8

write_pcm_wait_9:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_9

write_pcm_wait_10:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_10

write_pcm_wait_11:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_11

write_pcm_wait_12:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_12

write_pcm_wait_13:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_13

write_pcm_wait_14:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_14

write_pcm_wait_15:
        move.b  #0x2A,0xA04000
        fetch_pcm d0
        move.b  d0,0xA04001
        subi.l  #36+26,d5
        bra     wait_15

seek_pcm:
        check_button
        fetch_vgm d0
        lsl.w   #8,d0
        fetch_vgm d0
        ror.w   #8,d0
        swap    d0
        fetch_vgm d0
        lsl.w   #8,d0
        fetch_vgm d0
        ror.w   #8,d0
        swap    d0
        add.l   a4,d0
        move.l  d0,d6
        andi.l  #0x0FFFFF,d0
        ori.l   #0x200000,d0
        move.l  d0,a5           /* new current pcm ptr */
        swap    d6
        lsr.w   #4,d6
        andi.w  #7,d6           /* new pcm bank */
        bra     read_cmd

data_block:
        fetch_vgm d0
        cmpi.b  #0x66,d0
        bne     ExitVGM         /* error in stream */
        fetch_vgm d3

        fetch_vgm d0
        lsl.w   #8,d0
        fetch_vgm d0
        ror.w   #8,d0
        swap    d0
        fetch_vgm d0
        lsl.w   #8,d0
        fetch_vgm d0
        ror.w   #8,d0
        swap    d0              /* size of data block */

        moveq   #7,d1
        and.l   d7,d1
        lsl.w   #4,d1
        swap    d1
        move.l  a6,d2
        andi.l  #0x0FFFFF,d2
        or.l    d2,d1           /* current vgm offset */

        tst.b   d3
        bne.b   0f              /* not pcm */
        movea.l d1,a4           /* pcm base offset = current vgm offset */
        move.w  d7,d6           /* pcm bank = vgm bank */
        movea.l a6,a5           /* curr pcm ptr = curr vgm ptr */
0:
        add.l   d1,d0           /* new vgm offset */
        move.l  d0,d7
        andi.l  #0x0FFFFF,d0
        ori.l   #0x200000,d0
        move.l  d0,a6           /* new current vgm ptr */
        swap    d7
        lsr.w   #4,d7
        andi.w  #7,d7           /* new vgm bank */

        move.b  #0x2B,0xA04000
        nop
        nop
        nop
        move.b  #0x80,0xA04001  /* enable DAC */
        nop
        nop
        nop
        move.b  #0x2A,0xA04000
        nop
        nop
        nop
        move.b  #0x80,0xA04001  /* silence */
        bra     read_cmd

end_data:
        check_button
        move.l  d4,d0           /* loop offset */
        move.l  d0,d7
        andi.l  #0x0FFFFF,d0
        ori.l   #0x200000,d0
        move.l  d0,a6           /* new current vgm ptr */
        swap    d7
        lsr.w   #4,d7
        andi.w  #7,d7           /* new vgm bank */
        moveq   #0,d5           /* start afresh */
        bra     read_cmd


FMReset:
        /* block settings */
        .byte   0x30,0x00
        .byte   0x40,0x7F
        .byte   0x50,0x1F
        .byte   0x60,0x1F
        .byte   0x70,0x1F
        .byte   0x80,0xFF
        .byte   0x90,0x00

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
        /* disable DAC */
        .byte   0,0x2B
        .byte   1,0x80
        .byte   0,0x2A
        .byte   1,0x80
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

        .align  2

| Neo Super 32X/MD/SMS Flash Cart Menu support code
| by Chilly Willy, based on Dr. Neo's Menu code

| Neo Myth hardware equates (relative to 0xA10000)
        .equ    OPTION_IO,  0x3000
        .equ    GBAC_LIO,   0x3002
        .equ    GBAC_HIO,   0x3004
        .equ    GBAC_ZIO,   0x3006
        .equ    GBAS_BIO,   0x3008
        .equ    GBAS_ZIO,   0x300A
        .equ    PRAM_BIO,   0x300C
        .equ    PRAM_ZIO,   0x300E
        .equ    RUN_IO,     0x3010
        .equ    USB_ON,     0x3014
        .equ    LED_ON,     0x3016
        .equ    RST_IO,     0x3018
        .equ    ID_ON,      0x301A
        .equ    CPLD_ID,    0x301E      /* on V12 Neo Myth */
        .equ    WE_IO,      0x3020
        .equ    RST_SEL,    0x3024      /* on V12 Neo Myth */
        .equ    EXTM_ON,    0x3028


| MD support code goes in the code segment

        .text

        .align 2

| old SR = set_sr(new SR)
| entry: arg = SR value
| exit:  d0 = previous SR value
        .global set_sr
set_sr:
        moveq   #0,d0
        move.w  sr,d0
        move.l  4(sp),d1
        move.w  d1,sr
        rts

| initialize the hardware - loads font tiles
        .global init_hardware
init_hardware:
        movem.l d2-d7/a2-a6,-(sp)

| init joyports
        lea     0xA10000,a5
        move.b  #0x40,0x09(a5)
        move.b  #0x40,0x0B(a5)
        move.b  #0x40,0x03(a5)
        move.b  #0x40,0x05(a5)

        lea     InitData,a5
        movem.w (a5)+,d5-d7
        movem.l (a5)+,a0-a4

| wait on VDP DMA (in case of init or we reset in the middle of DMA)
0:
        move.w  (a4),d0                 /* read VDP status */
        btst    #1,d0                   /* DMA busy? */
        bne.b   0b                      /* yes */

        moveq   #0,d0

| Set VDP registers
        moveq   #18,d1
1:
        move.b  (a5)+,d5                /* lower byte = register data */
        move.w  d5,(a4)                 /* set VDP register */
        add.w   d7,d5                   /* + 0x0100 = next register */
        dbra    d1,1b
        lea     9(a5),a5                /* skip DMA Fill operation */

| clear VRAM
        move.w  #0x8F02,(a4)            /* set INC to 2 */
        move.l  #0x40000000,(a4)        /* write VRAM address 0 */
        move.w  #0x7FFF,d1              /* 32K - 1 words */
2:
        move.w  d0,(a3)                 /* clear VRAM */
        dbra    d1,2b

| The VDP state at this point is: Display disabled, ints disabled, Name Tbl A at 0xC000,
| Name Tbl B at 0xE000, Name Tbl W at 0xB000, Sprite Attr Tbl at 0xA800, HScroll Tbl at 0xAC00,
| H40 V28 mode, and Scroll size is 64x32.

| init Z80 and FM
        move.w  d7,(a1)                 /* Z80 assert BusReq */
        move.w  d7,(a2)                 /* Z80 cancel Reset */
3:
        btst    d0,(a1)                 /* wait on BusReq */
        bne.b   3b
| reset YM2612
        lea     FMReset(pc),a6
        move.w  #0x4000,d1
        moveq   #26,d2
9:
        move.b  (a6)+,d1                /* FM reg */
        move.b  (a6)+,0(a0,d1.w)        /* FM data */
        nop
        nop
        dbra    d2,9b

| Copy Z80 default program
        moveq   #37,d2
4:
        move.b  (a5)+,(a0)+
        dbra    d2,4b
| Restart Z80
        move.w  d0,(a2)                 /* Z80 assert Reset */
        move.w  d0,(a1)                 /* Z80 cancel BusReq */
        move.w  d7,(a2)                 /* Z80 cancel Reset */

| Clear CRAM
        move.l  (a5)+,(a4)              /* set reg 1 and reg 15 */
        move.l  (a5)+,(a4)              /* write CRAM address 0 */
        moveq   #31,d3
5:
        move.l  d0,(a3)
        dbra    d3,5b

| Clear VSRAM
        move.l  (a5)+,(a4)              /* write VSRAM address 0 */
        moveq   #19,d4
6:
        move.l  d0,(a3)
        dbra    d4,6b

| Initialize PSG registers
        moveq   #3,d5
7:
        move.b  (a5)+,0x0011(a3)
        dbra    d5,7b

        move.w  d0,(a2)                 /* Z80 assert Reset */

| load font tile data
        move.w  #0x8F02,(a4)            /* INC = 2 */
        move.l  #0x40000000,(a4)        /* write VRAM address 0 */
        lea     font_data,a0
        move.w  #0x68*8-1,d2
8:
        move.l  (a0)+,d0                /* font fg mask */
        move.l  d0,d1
        not.l   d1                      /* font bg mask */
        andi.l  #0x11111111,d0          /* set font fg color */
        andi.l  #0x00000000,d1          /* set font bg color */
        or.l    d1,d0
        move.l  d0,(a3)                 /* set tile line */
        dbra    d2,8b

| set palette
        move.l  #0xC0000000,(a4)        /* write CRAM address 0 */
        move.l  #0x00000CCC,(a3)        /* entry 0 (black) and 1 (lt gray) BGR */
        move.l  #0xC0200000,(a4)        /* write CRAM address 32 */
        move.l  #0x000000A0,(a3)        /* entry 16 (black) BGR and 17 (green) */
        move.l  #0xC0400000,(a4)        /* write CRAM address 64 */
        move.l  #0x0000000A,(a3)        /* entry 32 (black) BGR and 33 (red) */

        move.w  #0x8174,(a4)            /* display on, vblank enabled */

        movem.l (sp)+,d2-d7/a2-a6
        rts

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

        .align 2

| clear screen
        .global clear_screen
clear_screen:
        moveq   #0,d0
        lea     0xC00000,a0
        move.w  #0x8F02,4(a0)           /* set INC to 2 */
        move.l  #0x60000003,d1          /* VDP write VRAM at 0xE000 (scroll plane B) */
        move.l  d1,4(a0)                /* write VRAM at plane B start */
        move.w  #64*32-1,d1
1:
        move.w  d0,(a0)                 /* clear name pattern */
        dbra    d1,1b
        rts

| put string characters to screen
| entry: first arg = string address
|        second arg = 0 for normal color font, 0x0200 for alternate color font (use CP bits for different colors)
        .global put_str
put_str:
        movea.l 4(sp),a0
        move.l  8(sp),d0
        lea     0xC00000,a1
        move.w  #0x8F02,4(a1)           /* set INC to 2 */
        moveq   #0,d1
        move.w  gCursorY,d1
        lsl.w   #6,d1
        or.w    gCursorX,d1             /* cursor Y<<6 | x */
        add.w   d1,d1                   /* pattern names are words */
        swap    d1
        ori.l   #0x60000003,d1          /* OR cursor with VDP write VRAM at 0xE000 (scroll plane B) */
        move.l  d1,4(a1)                /* write VRAM at location of cursor in plane B */
1:
        move.b  (a0)+,d0
        subi.b  #0x20,d0                /* font starts at space */
        move.w  d0,(a1)                 /* set pattern name for character */
        tst.b   (a0)
        bne.b   1b
        rts


        .align 2

|--------------------------------------------------------------------------------------------------------
| NEO2 Flash support code goes into the data segment so that it is run from Work RAM (with ints disabled)
| _neo_xxx indicates internal assembly access only function
| neo_xxx indicates external C function with args on stack
|--------------------------------------------------------------------------------------------------------

        .data

        .align 2

| do a Neo Flash ASIC command
| entry: d0 = Neo Flash ASIC command
|        a1 = hardware base (0xA10000)
_neo_asic_cmd:
        move.l  d0,-(sp)
        /* do unlocking sequence */
        move.l  #0x00FFD200,d0
        bsr.b   _neo_asic_op
        move.l  #0x00001500,d0
        bsr.b   _neo_asic_op
        move.l  #0x0001D200,d0
        bsr.b   _neo_asic_op
        move.l  #0x00021500,d0
        bsr.b   _neo_asic_op
        move.l  #0x00FE1500,d0
        bsr.b   _neo_asic_op
        /* do ASIC command */
        move.l  (sp)+,d0
|       bsr.b   _neo_asic_op
|       moveq   #0,d0
        /* fall into _neo_asic_op for last operation */

| do a Neo Flash ASIC operation
| entry: d0 = Neo Flash ASIC operation
|             b23-16 = addr, b15-b0 = value
|        a1 = hardware base (0xA10000)
| exit:  d0 = result (usually dummy read)
_neo_asic_op:
        move.l  d0,d1
        rol.l   #8,d1
        andi.w  #0x000F,d1              /* keep b27-24 */
        move.w  d1,GBAC_HIO(a1)         /* set high bank select reg (holds flash space A27-A24) */
        rol.l   #8,d1
        andi.w  #0x00F8,d1              /* keep b23-19, b18-16 = 0 */
        move.w  d1,GBAC_LIO(a1)         /* set low bank select reg (holds flash space A23-A16) */
        andi.l  #0x0007FFFF,d0          /* b23-19 = 0, keep b18-b0 */
        add.l   d0,d0                   /* 68000 A19-A1 = b18-b0 */
        movea.l d0,a0
        move.w  (a0),d0                 /* access the flash space to do the operation */
        rts

| select Neo Flash Menu Flash ROM
| allows you to access the menu flash via flash space
| entry: a1 = hardware base (0xA10000)
_neo_select_menu:
        move.w  #0x0000,OPTION_IO(a1)   /* set mode 0 */
        move.w  #0x0000,GBAC_ZIO(a1)    /* clear bank size reg */

        move.l  #0x00370003,d0
        or.w    neo_mode,d0             /* enable/disable SD card interface */
        bsr     _neo_asic_cmd           /* set cr = select menu flash */
        move.l  #0x00DA0044,d0
        bsr     _neo_asic_cmd           /* set iosr = disable game flash */

        move.w  #0x0000,GBAC_LIO(a1)    /* clear low bank select reg */
        move.w  #0x0000,GBAC_HIO(a1)    /* clear high bank select reg */
        move.w  #0x00F8,GBAC_ZIO(a1)    /* bank size = 1MB */
        move.w  #0x0000,PRAM_BIO(a1)    /* set psram to bank 0 */
        move.w  #0x00F0,PRAM_ZIO(a1)    /* psram bank size = 2MB */
        move.w  #0x0006,WE_IO(a1)       /* map bank 7, write enable myth psram */
        move.w  #0x0007,OPTION_IO(a1)   /* set mode 7 (copy mode) */
        rts

| enable Neo Flash ASIC ID
| allows you to access the goodies in the SRAM space
| entry: a1 = hardware base (0xA10000)
_neo_enable_id:
        move.w  #0xFFFF,ID_ON(a1)       /* enable ID access on SRAM space */
        move.w  #0x0000,OPTION_IO(a1)   /* set mode 0 */
        move.l  #0x00903500,d0
        bsr     _neo_asic_cmd           /* enable ID */
        move.w  #0x0001,OPTION_IO(a1)   /* mode 1 - 16Mbit PSRAM + sram */
        rts

| disable Neo Flash ASIC ID
| entry: a1 = hardware base (0xA10000)
_neo_disable_id:
        move.w  #0x0000,OPTION_IO(a1)   /* set mode 0 */
        move.l  #0x00904900,d0
        bsr     _neo_asic_cmd           /* disable ID */
        move.w  #0x0000,ID_ON(a1)       /* disable ID access on SRAM space */
        rts

| short int neo_check_card(void);
        .global neo_check_card
neo_check_card:
        lea     0xA10000,a1
        bsr     _neo_enable_id
        lea     0x200000,a0             /* sram space */
        /* check for MAGIC signature */
        cmp.b   #0x34,1(a0)
        bne.b   1f                      /* error, Neo2/3 GBA flash card not found */
        cmp.b   #0x16,3(a0)
        bne.b   1f                      /* error, Neo2/3 GBA flash card not found */
        cmp.b   #0x96,5(a0)
        bne.b   1f                      /* error, Neo2/3 GBA flash card not found */
        cmp.b   #0x24,7(a0)
        bne.b   1f                      /* error, Neo2/3 GBA flash card not found */
|       cmp.b   #0xF6,9(a0)
|       bne.b   1f                      /* error, Neo2/3 GBA flash card not found */
        bsr     _neo_disable_id
        bsr     _neo_select_menu
        moveq   #0,d0                   /* Neo2/3 GBA flash card found! */
        rts
1:
        bsr     _neo_disable_id
        bsr     _neo_select_menu
        moveq   #-1,d0                  /* Neo2/3 GBA flash card not found! */
        rts

| void neo_run_myth_psram(int psize, int bbank, int bsize, int run);
| run from neo myth psram
        .global neo_run_myth_psram
neo_run_myth_psram:
        lea     0xA10000,a1
        bsr     _neo_select_menu        /* select Neo Myth menu flash */

        move.w  #0x0007,OPTION_IO(a1)   /* set run mode for game */
        move.w  #0x0006,WE_IO(a1)       /* write-enable psram, map bank 7 to 0x300000 */
        movea.l 0x300000,a7
        movea.l 0x300004,a3
        jmp     (a3)

| void neo_copyto_myth_psram(unsigned char *src, int pstart, int len);
        .global neo_copyto_myth_psram
neo_copyto_myth_psram:
        lea     0xA10000,a1

        move.l  8(sp),d0                /* pstart */
        move.w  #20,d1
        lsr.l   d1,d0                   /* bank = pstart / 1MB  */
        move.w  d0,PRAM_BIO(a1)         /* set the neo myth psram bank register */
        move.w  #0x00F8,PRAM_ZIO(a1)    /* set the neo myth psram bank size to 1MB */

        move.w  #0x0007,OPTION_IO(a1)   /* set mode 7 (copy mode) */
        move.w  #0x0006,WE_IO(a1)       /* map bank 7, write-enable neo myth psram */

        movea.l 4(sp),a0                /* src */
        move.l  8(sp),d1                /* pstart */
        andi.l  #0x0FFFFE,d1            /* offset inside sram space (bank was set to closest 1MB) */
        ori.l   #0x200000,d1            /* sram space access */
        movea.l d1,a1
        move.l  12(sp),d0               /* len */
        lsr.l   #1,d0                   /* # words to copy */
        subq.w  #1,d0
1:
        move.w  (a0)+,(a1)+
        dbra    d0,1b

        lea     0xA10000,a1
        bra     _neo_select_menu        /* select Menu Flash */

| void neo2_enable_sd(void);
        .global neo2_enable_sd
neo2_enable_sd:
        lea     0xA10000,a1
        move.w  #0x2700,sr              /* disable interrupts */
        move.w  #0x0480,neo_mode        /* when select menu flash, SD also enabled */
        bsr     _neo_select_menu        /* select Menu Flash */
        move.w  #0x2000,sr              /* enable interrupts */
        rts

| void neo2_disable_sd(void);
        .global neo2_disable_sd
neo2_disable_sd:
        lea     0xA10000,a1
        move.w  #0x2700,sr              /* disable interrupts */
        move.w  #0,neo_mode             /* when select menu flash, SD also disabled */
        bsr     _neo_select_menu        /* select Menu Flash */
        move.w  #0x2000,sr              /* enable interrupts */
        rts

| void neo2_pre_sd(void);
        .global neo2_pre_sd
neo2_pre_sd:
        lea     0xA10000,a1
        move.w  #0x2700,sr              /* disable interrupts */
        move.w  #0x0080,GBAC_LIO(a1)    /* set low bank select reg - all SD ops use this value */
        rts

| void neo2_post_sd(void);
        .global neo2_post_sd
neo2_post_sd:
        lea     0xA10000,a1
        move.w  #0x0000,GBAC_LIO(a1)    /* clear low bank select reg */
        move.w  #0x2000,sr              /* enable interrupts */
        rts

| void neo2_recv_sd(unsigned char *buf);
        .global neo2_recv_sd
neo2_recv_sd:
        move.l  d2,-(sp)
        lea     0xA10000,a1
        movea.l 8(sp),a0                /* buf */

        move.w  #0x0087,GBAC_LIO(a1)
        moveq   #127,d0
1:
        move.w  0x6060.w,d1
        lsl.b   #4,d1
        move.w  0x6060.w,d2
        andi.w  #0x000F,d2
        or.b    d2,d1                   /* sector byte */
        move.b  d1,(a0)+

        move.w  0x6060.w,d1
        lsl.b   #4,d1
        move.w  0x6060.w,d2
        andi.w  #0x000F,d2
        or.b    d2,d1                   /* sector byte */
        move.b  d1,(a0)+

        move.w  0x6060.w,d1
        lsl.b   #4,d1
        move.w  0x6060.w,d2
        andi.w  #0x000F,d2
        or.b    d2,d1                   /* sector byte */
        move.b  d1,(a0)+

        move.w  0x6060.w,d1
        lsl.b   #4,d1
        move.w  0x6060.w,d2
        andi.w  #0x000F,d2
        or.b    d2,d1                   /* sector byte */
        move.b  d1,(a0)+

        dbra    d0,1b

        moveq   #7,d0
2:
        move.w  0x6060.w,d1
        lsl.b   #4,d1
        move.w  0x6060.w,d2
        andi.w  #0x000F,d2
        or.b    d2,d1
|       move.b  d1,(a0)+                /* CRC byte */
        dbra    d0,2b

        move.w  0x6060.w,d1             /* end bit */

        move.w  #0x0080,GBAC_LIO(a1)
        move.l  (sp)+,d2
        rts


neo_mode:
        .word   0


        .text

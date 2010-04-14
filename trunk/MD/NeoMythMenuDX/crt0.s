| SEGA MegaDrive support code
| by Chilly Willy

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

        .text

| Initial exception vectors

        .long   0x01000000,initialize,exception,exception,exception,exception,exception,exception
        .long   exception,exception,exception,exception,exception,exception,exception,exception
        .long   exception,exception,exception,exception,exception,exception,exception,exception
        .long   exception,exception,exception,exception,hblank,exception,vblank,exception
        .long   exception,exception,exception,exception,exception,exception,exception,exception
        .long   exception,exception,exception,exception,exception,exception,exception,exception
        .long   exception,exception,exception,exception,exception,exception,exception,exception
        .long   exception,exception,exception,exception,exception,exception,exception,exception

| Standard MegaDrive ROM header at 0x100

        .ascii  "SEGA GENESIS    "
        .ascii  "(C)SEGA 2010.APR"
        .ascii  "Neo Super 32X/MD"
        .ascii  "/SMS Flash Cart "
        .ascii  "                "
        .ascii  "Neo Super 32X/MD"
        .ascii  "/SMS Flash Cart "
        .ascii  "                "
        .ascii  "GM MK-0000 -00"
        .word   0x0000                  /* checksum - fixed by flashing app */
        .ascii  "J6              "
        .long   0x00000000,0x0000FFFF   /* ROM start, end */
        .long   0x00FF0000,0x00FFFFFF   /* RAM start, end */

        .ascii  "            "          /* no SRAM */

        .ascii  "    "
        .ascii  "        "
        .ifdef  RUN_IN_PSRAM
        .ascii  "MYTH3900"              /* memo indicates Myth native executable */
        .else
        .ascii  "        "              /* memo */
        .endif
        .ascii  "                "
        .ascii  "                "
        .ascii  "F               "      /* enable any hardware configuration */


        .ifdef  RUN_IN_PSRAM

| When running from PSRAM, pretty much all the initialization has already been
| done for us by the boot rom code. No need to reset the CD or 32X or sound.
| Just clear the Work RAM, copy the initial data, and call main().

initialize:
        move    #0x2700,sr              /* interrupts disabled */

        lea     0x01000000,a0
        movea.l a0,sp                   /* set stack pointer to top of Work RAM */

| Clear Work RAM
        lea     0xFF0000,a0
        moveq   #0,d0
        move.w  #0x3FFF,d1
1:
        move.l  d0,(a0)+
        dbra    d1,1b

| Copy initialized variables from ROM to Work RAM
        lea     _stext,a0
        adda.l  #0x00300000,a0
        lea     0xFF0000,a1
        move.l  #_sdata,d0
		lsr.l	#1,d0
		subq.w	#1,d0
2:
        move.w  (a0)+,(a1)+
        dbra    d0,2b

        jsr     main                    /* call main() */
        jmp     initialize.l

| initialization data is used by init_hardware()

        .global InitData
InitData:
        .word   0x8000, 0x3FFF, 0x0100      /* d5, d6, d7 initial values */
        .long   0x00A00000, 0x00A11100, 0x00A11200, 0x00C00000, 0x00C00004  /* a0 to a4 initial values */

| VDP register initialization values
        .byte   0x04    /* 8004 => write reg 0 = /IE1 (no HBL INT), /M3 (enable read H/V cnt) */
        .byte   0x14    /* 8114 => write reg 1 = /DISP (display off), /IE0 (no VBL INT), M1 (DMA enabled), /M2 (V28 mode) */
        .byte   0x30    /* 8230 => write reg 2 = Name Tbl A = 0xC000 */
        .byte   0x2c    /* 832C => write reg 3 = Name Tbl W = 0xB000 */
        .byte   0x07    /* 8407 => write reg 4 = Name Tbl B = 0xE000 */
        .byte   0x54    /* 8554 => write reg 5 = Sprite Attr Tbl = 0xA800 */
        .byte   0x00    /* 8600 => write reg 6 = always 0 */
        .byte   0x00    /* 8700 => write reg 7 = BG color */
        .byte   0x00    /* 8800 => write reg 8 = always 0 */
        .byte   0x00    /* 8900 => write reg 9 = always 0 */
        .byte   0x00    /* 8A00 => write reg 10 = HINT = 0 */
        .byte   0x00    /* 8B00 => write reg 11 = /IE2 (no EXT INT), full scroll */
        .byte   0x81    /* 8C81 => write reg 12 = H40 mode, no lace, no shadow/hilite */
        .byte   0x2b    /* 8D2B => write reg 13 = HScroll Tbl = 0xAC00 */
        .byte   0x00    /* 8E00 => write reg 14 = always 0 */
        .byte   0x01    /* 8F01 => write reg 15 = data INC = 1 */
        .byte   0x01    /* 9001 => write reg 16 = Scroll Size = 64x32 */
        .byte   0x00    /* 9100 => write reg 17 = W Pos H = left */
        .byte   0x00    /* 9200 => write reg 18 = W Pos V = top */
        .byte   0xff    /* 93FF => write reg 19 = DMA length low byte = 0xFF */
        .byte   0xff    /* 94FF => write reg 20 = DMA length high byte = 0xFF */
        .byte   0x00    /* 9500 => write reg 21 = DMA src addr low = 0 */
        .byte   0x00    /* 9600 => write reg 22 = DMA src addr mid = 0 */
        .byte   0x80    /* 9780 => write reg 23 = DMD = VRAM FILL, DMA src addr high = 0 */
| VDP Command
        .word   0x4000, 0x0080          /* DMA Fill VRAM */

| Z80 default program code
        .word   0xaf01, 0xd91f, 0x1127, 0x0021, 0x2600, 0xf977, 0xedb0, 0xdde1
        .word   0xfde1, 0xed47, 0xed4f, 0xd1e1, 0xf108, 0xd9c1, 0xd1e1, 0xf1f9
        .word   0xf3ed, 0x5636, 0xe9e9

| VDP Commands
        .word   0x8104, 0x8f01  /* set registers 1 (display off) and 15 (INC = 1) */
        .word   0xc000, 0x0000  /* write CRAM address 0 */
        .word   0x4000, 0x0010  /* write VSRAM address 0 */

| PSG register initialization values
        .byte   0x9f
        .byte   0xbf
        .byte   0xdf
        .byte   0xff

| Note - the data section below MUST match that used when running from rom

| put redirection vectors and gTicks at start of Work RAM

        .data

        .global exception_vector
exception_vector:
        .long   0
        .global exception_vector
hblank_vector:
        .long   0
        .global exception_vector
vblank_vector:
        .long   0
        .global gTicks
gTicks:
        .long   0


| Exception handlers

exception:
        move.l  exception_vector,-(sp)
        beq.b   1f
        rts
1:
        addq.l  #4,sp
        rte

hblank:
        move.l  hblank_vector,-(sp)
        beq.b   1f
        rts
1:
        addq.l  #4,sp
        rte

vblank:
        move.l  vblank_vector,-(sp)
        beq.b   1f
        rts
1:
        addq.l  #1,gTicks
        addq.l  #4,sp
        rte

        .else

| Standard MegaDrive startup at 0x200 - sort of. Try to handle 32X reset.
| If MD reset, will start at initialize since that's in the exception table.
| If 32X reset, 32X exception table will go to 0x200 which will jump to initialize in the MD rom region of 32X.

        jmp     0x880000+initialize     /* reset = hot start */
initialize:
        cmpi.l  #0x4D415253,0xA130EC    /* "MARS" */
        bne.b   0f
        btst    #0,0xA15101
        beq.b   0f
| 32X is enabled - we're trying to reset to menu
        lea     _mars_reset(pc),a0
        lea     0xFF0000,a1
        moveq   #(_mars_reset_end-_mars_reset)/2-1,d0
99:
        move.w  (a0)+,(a1)+
        dbra    d0,99b
        jmp     0xFF0000.l              /* reset 32X */
0:
        move.w  #0,0xA12000             /* reset CD */

        tst.l   0xA10008                /* check CTRL1 and CTRL2 setup */
        bne.b   1f
        tst.w   0xA1000C                /* check CTRL3 setup */
1:
        bne     skip_setup              /* if any controller control port is setup, skip init */

| Initialize the MegaDrive hardware

        lea     InitData(pc),a5
        movem.w (a5)+,d5-d7
        movem.l (a5)+,a0-a4

| Check Hardware Version Number
        move.b  -0x10FF(a1),d0          /* 0xA10001 */
        andi.b  #0x0F,d0                /* VERS */
        beq     2f                      /* 0 = original hardware, TMSS not present */
        move.l  #0x53454741,0x2F00(a1)  /* Store Sega Security Code "SEGA" to TMSS */
2:
        move.w  (a4),d0                 /* read VDP Status reg */

| Set the stack pointer
        moveq   #0,d0
        movea.l d0,a6
        move    a6,usp

| Set VDP registers
        moveq   #23,d1
3:
        move.b  (a5)+,d5                /* lower byte = register data */
        move.w  d5,(a4)
        add.w   d7,d5                   /* + 0x0100 = next register */
        dbra    d1,3b
| clear VRAM
        move.l  (a5)+,(a4)              /* VDP Command = DMA Fill VRAM */
        move.w  d0,(a3)                 /* VDP (Fill) Data = 0 */

        move.w  d7,(a1)                 /* Z80 assert BusReq */
        move.w  d7,(a2)                 /* Z80 cancel Reset */
4:
        btst    d0,(a1)                 /* wait on BusReq */
        bne     4b
| Copy Z80 default program
        moveq   #37,d2
5:
        move.b  (a5)+,(a0)+
        dbra    d2,5b
| Restart Z80
        move.w  d0,(a2)                 /* Z80 assert Reset */
        move.w  d0,(a1)                 /* Z80 cancel BusReq */
        move.w  d7,(a2)                 /* Z80 cancel Reset */

| Clear Work RAM - (0x3FFF + 1) longs of 0
6:
        move.l  d0,-(a6)
        dbra    d6,6b

| Clear CRAM
        move.l  (a5)+,(a4)              /* set reg 1 and reg 15 */
        move.l  (a5)+,(a4)              /* write CRAM address 0 */
        moveq   #31,d3
7:
        move.l  d0,(a3)
        dbra    d3,7b

| Clear VSRAM
        move.l  (a5)+,(a4)              /* write VSRAM address 0 */
        moveq   #19,d4
8:
        move.l  d0,(a3)
        dbra    d4,8b

| Initialize PSG registers
        moveq   #3,d5
9:
        move.b  (a5)+,0x0011(a3)
        dbra    d5,9b

        move.w  d0,(a2)                 /* Z80 assert Reset */

        movem.l (a6),d0-d7/a0-a6        /* clear all CPU registers */
        move    #0x2700,sr              /* disable interrupts */

skip_setup:
        bra     continue

        .global InitData
InitData:
        .word   0x8000, 0x3FFF, 0x0100      /* d5, d6, d7 initial values */
        .long   0x00A00000, 0x00A11100, 0x00A11200, 0x00C00000, 0x00C00004  /* a0 to a4 initial values */

| VDP register initialization values
        .byte   0x04    /* 8004 => write reg 0 = /IE1 (no HBL INT), /M3 (enable read H/V cnt) */
        .byte   0x14    /* 8114 => write reg 1 = /DISP (display off), /IE0 (no VBL INT), M1 (DMA enabled), /M2 (V28 mode) */
        .byte   0x30    /* 8230 => write reg 2 = Name Tbl A = 0xC000 */
        .byte   0x2c    /* 832C => write reg 3 = Name Tbl W = 0xB000 */
        .byte   0x07    /* 8407 => write reg 4 = Name Tbl B = 0xE000 */
        .byte   0x54    /* 8554 => write reg 5 = Sprite Attr Tbl = 0xA800 */
        .byte   0x00    /* 8600 => write reg 6 = always 0 */
        .byte   0x00    /* 8700 => write reg 7 = BG color */
        .byte   0x00    /* 8800 => write reg 8 = always 0 */
        .byte   0x00    /* 8900 => write reg 9 = always 0 */
        .byte   0x00    /* 8A00 => write reg 10 = HINT = 0 */
        .byte   0x00    /* 8B00 => write reg 11 = /IE2 (no EXT INT), full scroll */
        .byte   0x81    /* 8C81 => write reg 12 = H40 mode, no lace, no shadow/hilite */
        .byte   0x2b    /* 8D2B => write reg 13 = HScroll Tbl = 0xAC00 */
        .byte   0x00    /* 8E00 => write reg 14 = always 0 */
        .byte   0x01    /* 8F01 => write reg 15 = data INC = 1 */
        .byte   0x01    /* 9001 => write reg 16 = Scroll Size = 64x32 */
        .byte   0x00    /* 9100 => write reg 17 = W Pos H = left */
        .byte   0x00    /* 9200 => write reg 18 = W Pos V = top */
        .byte   0xff    /* 93FF => write reg 19 = DMA length low byte = 0xFF */
        .byte   0xff    /* 94FF => write reg 20 = DMA length high byte = 0xFF */
        .byte   0x00    /* 9500 => write reg 21 = DMA src addr low = 0 */
        .byte   0x00    /* 9600 => write reg 22 = DMA src addr mid = 0 */
        .byte   0x80    /* 9780 => write reg 23 = DMD = VRAM FILL, DMA src addr high = 0 */
| VDP Command
        .word   0x4000, 0x0080          /* DMA Fill VRAM */

| Z80 default program code
        .word   0xaf01, 0xd91f, 0x1127, 0x0021, 0x2600, 0xf977, 0xedb0, 0xdde1
        .word   0xfde1, 0xed47, 0xed4f, 0xd1e1, 0xf108, 0xd9c1, 0xd1e1, 0xf1f9
        .word   0xf3ed, 0x5636, 0xe9e9

| VDP Commands
        .word   0x8104, 0x8f01  /* set registers 1 (display off) and 15 (INC = 1) */
        .word   0xc000, 0x0000  /* write CRAM address 0 */
        .word   0x4000, 0x0010  /* write VSRAM address 0 */

| PSG register initialization values
        .byte   0x9f
        .byte   0xbf
        .byte   0xdf
        .byte   0xff

continue:
        tst.w    0x00C00004

| Initialization is skipped on reset, so don't assume anything here...

        move    #0x2700,sr              /* interrupts disabled */

|        move.l  #0xC0000000,0xC00004    /* write CRAM address 0 */
|        move.w  #0x0008,0xC00000        /* entry 0 (red) BGR */

		moveq	#0,d0
		move.l	d0,exception_vector
		move.l	d0,hblank_vector
		move.l	d0,vblank_vector
		move.l	d0,gTicks

        lea     0x01000000,a0
        movea.l a0,sp                   /* set stack pointer to top of Work RAM */

| Setup MD Myth for proper operation
        lea     _setup_md_myth(pc),a0
        lea     0xFF0000,a1
        move.w  #(_setup_md_myth_end-_setup_md_myth)/2-1,d0
0:
        move.w  (a0)+,(a1)+
        dbra    d0,0b
        jsr     0xFF0000.l              /* setup MD Myth */

|        move.l  #0xC0000000,0xC00004    /* write CRAM address 0 */
|        move.w  #0x0080,0xC00000        /* entry 0 (green) BGR */

| Clear Work RAM
        lea     0xFF0000,a0
        moveq   #0,d0
        move.w  #0x3FFF,d1
1:
        move.l  d0,(a0)+
        dbra    d1,1b

| Copy initialized variables from ROM to Work RAM
        lea     _stext,a0
        lea     0xFF0000,a1
        move.l  #_sdata,d0
		lsr.l	#1,d0
		subq.w	#1,d0
2:
        move.w  (a0)+,(a1)+
        dbra    d0,2b

|        move.l  #0xC0000000,0xC00004    /* write CRAM address 0 */
|        move.w  #0x0888,0xC00000        /* entry 0 (gray) BGR */

        jsr     main                    /* call main() */
        jmp     initialize.w


| Reset 32X - copied into Work RAM

_mars_reset:
        lea     0xA15000,a5
        moveq   #7,d0
0:
        bclr    d0,0x100(a5)            /* clear FM - give MD access to SuperVDP */
        bne.b   0b
1:
        btst    d0,0x18A(a5)            /* VBLK */
        bne.b   1b
2:
        btst    d0,0x18A(a5)            /* VBLK */
        beq.b   2b

        move.w  #0,0x180(a5)            /* disable 32X video, give MD priority */
3:
        btst    d0,0x18A(a5)            /* VBLK */
        bne.b   3b

        move.w  #0,0x100(a5)            /* disable and reset 32X */
        jmp     initialize.l            /* go to MD initialize entry point */
_mars_reset_end:


| Setup MD Myth - copied into Work RAM

_setup_md_myth:
        lea     0xA10000,a1
        move.w  #0x0000,RST_SEL(a1)
        move.w  #0x00FF,RST_IO(a1)

        move.w  #0x0000,OPTION_IO(a1)   /* set mode 0 */
        move.w  #0x0000,GBAC_ZIO(a1)    /* clear bank size */

        move.l  #0x00370003,d0
        bsr.b   1f                      /* set cr = select menu flash */
        move.l  #0x00DA0044,d0
        bsr.b   1f                      /* set iosr = disable game flash */

        move.w  #0x00F8,GBAC_ZIO(a1)    /* bank size = 1MB */
        move.w  #0x0000,GBAC_LIO(a1)    /* clear low bank select reg */
        move.w  #0x0000,GBAC_HIO(a1)    /* clear high bank select reg */
        rts
1:
        move.l  d0,-(sp)
        /* do unlocking sequence */
        move.l  #0x00FFD200,d0
        bsr.b   2f
        move.l  #0x00001500,d0
        bsr.b   2f
        move.l  #0x0001D200,d0
        bsr.b   2f
        move.l  #0x00021500,d0
        bsr.b   2f
        move.l  #0x00FE1500,d0
        bsr.b   2f
        /* do ASIC command */
        move.l  (sp)+,d0
        /* fall into _neo_asic_op for last operation */
2:
        move.w  #0x0000,GBAC_HIO(a1)    /* clear high bank select reg */
        move.l  d0,d1
        swap    d1
        andi.w  #0x00F0,d1              /* keep b23-20, b19-16 = 0 */
        move.w  d1,GBAC_LIO(a1)         /* set low bank select reg (holds flash space A23-A16) */
        andi.l  #0x000FFFFF,d0          /* b23-20 = 0, keep b19-b0 */
        add.l   d0,d0                   /* 68000 A20-A1 = b19-b0 */
        movea.l d0,a0
        move.w  (a0),d0                 /* access the flash space to do the operation */
        rts
_setup_md_myth_end:


| put redirection vectors and gTicks at start of Work RAM

        .data

        .global exception_vector
exception_vector:
        .long   0
        .global exception_vector
hblank_vector:
        .long   0
        .global exception_vector
vblank_vector:
        .long   0
        .global gTicks
gTicks:
        .long   0


| Exception handlers

exception:
        move.l  exception_vector,-(sp)
        beq.b   1f
        rts
1:
        addq.l  #4,sp
        rte

hblank:
        move.l  hblank_vector,-(sp)
        beq.b   1f
        rts
1:
        addq.l  #4,sp
        rte

vblank:
        move.l  vblank_vector,-(sp)
        beq.b   1f
        rts
1:
        addq.l  #1,gTicks
        addq.l  #4,sp
        rte

| pad space out for BC menu

        .text

        .org    0x00B000

| EOT marker for empty menu list

        .byte   0xFF

        .org    0x00FFF0

| card type of NeoFlash card

        .word   0x0000                  /* 0x0000 = type C(7), 0x0101 = type B(5), 0x0202 = type A(5) */

| pad space out to extended code space

        .org    0x030000

        .endif

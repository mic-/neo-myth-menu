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

        .ascii  "SEGA MD NeoMyth "
        .ascii  "(C)2011 NeoFlash"
        .ascii  "Neo Super 32X/MD"
        .ascii  "/SMS Flash Cart "
        .ascii  "                "
        .ascii  "Neo Super 32X/MD"
        .ascii  "/SMS Flash Cart "
        .ascii  "                "
        .ascii  "GM MK-0000 -00"
        .word   0x0000                  /* checksum - not needed */
        .ascii  "J6              "
        .long   0x00000000,0x0007FFFF   /* ROM start, end */
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
        lsr.l   #1,d0
        subq.w  #1,d0
2:
        move.w  (a0)+,(a1)+
        dbra    d0,2b

        jsr     main                    /* call main() */
        jmp     initialize.l

        .align  4

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
        move    #0x2700,sr              /* disable interrupts */

| Initialize all hardware, including disabling the 32X and CD

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
        bne.b   skip_tmss               /* if any controller control port is setup, skip TMSS handling */

| Check Hardware Version Number
        move.b  0xA10001,d0
        andi.b  #0x0F,d0                /* VERS */
        beq     2f                      /* 0 = original hardware, TMSS not present */
        move.l  #0x53454741,0xA14000    /* Store Sega Security Code "SEGA" to TMSS */
2:
        move.w  0xC00004,d0             /* read VDP Status reg */

skip_tmss:
        move.w  #0x8104,0xC00004        /* display off, vblank disabled */
        move.w  0xC00004,d0             /* read VDP Status reg */

        moveq   #0,d0
        move.l  d0,exception_vector
        move.l  d0,hblank_vector
        move.l  d0,vblank_vector
        move.l  d0,gTicks

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
        lsr.l   #1,d0
        subq.w  #1,d0
2:
        move.w  (a0)+,(a1)+
        dbra    d0,2b

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

|        move.w  #0x00FF,CPLD_ID(a1)
|        cmpi.b  #0x63,0x300002
|        bne.b   0f
|        cmpi.b  #0x63,0x30000A
|        bne.b   0f
|        cmpi.b  #0x04,0x300000
|        bne.b   0f
|        move.w  #0x0000,CPLD_ID(a1)
0:
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

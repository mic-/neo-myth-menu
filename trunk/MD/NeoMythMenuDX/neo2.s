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
	.equ	DAT_SWAP,   0x3026	/* on V5 Neo Myth */
        .equ    EXTM_ON,    0x3028

| Neo2/3 Flash Cart equates (the useful ones)
        .equ    NEO2_VOLTS,     0x0005
        .equ    NEO2_RTC,       0x4000
        .equ    NEO2_GTC,       0x4800
        .equ    NEO2_SRAM_BANK, 0x80A0
        .equ    NEO2_CACHE,     0xC000


| MD support code goes in the code segment

| Support equates
                .equ    CMD_INTS_OFF,   0x2700
                .equ    CMD_INTS_ON,    0x2000

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

| void ints_on();
        .global ints_on
ints_on:
                move.w #CMD_INTS_ON,sr
        rts

| void ints_off();
        .global ints_off
ints_off:
                move.w #CMD_INTS_OFF,sr
        rts

| buttons = get_pad(pad)
| entry: arg = pad control port
| exit:  d0 = pad value (0 0 0 1 M X Y Z S A C B R L D U) or (0 0 0 0 0 0 0 0 S A C B R L D U)
        .global get_pad
get_pad:
        move.l  d2,-(sp)
        move.l  8(sp),d0        /* first arg is pad number */
        cmpi.w  #1,d0
        bhi     no_pad
        add.w   d0,d0
        addi.l  #0xA10003,d0    /* pad control register */
        movea.l d0,a0
        bsr.b   get_input       /* - 0 s a 0 0 d u - 1 c b r l d u */
        move.w  d0,d1
        andi.w  #0x0C00,d0
        bne.b   no_pad
        bsr.b   get_input       /* - 0 s a 0 0 d u - 1 c b r l d u */
        bsr.b   get_input       /* - 0 s a 0 0 0 0 - 1 c b m x y z */
        move.w  d0,d2
        bsr.b   get_input       /* - 0 s a 1 1 1 1 - 1 c b r l d u */
        andi.w  #0x0F00,d0      /* 0 0 0 0 1 1 1 1 0 0 0 0 0 0 0 0 */
        cmpi.w  #0x0F00,d0
        beq.b   common          /* six button pad */
        move.w  #0x010F,d2      /* three button pad */
common:
        lsl.b   #4,d2           /* - 0 s a 0 0 0 0 m x y z 0 0 0 0 */
        lsl.w   #4,d2           /* 0 0 0 0 m x y z 0 0 0 0 0 0 0 0 */
        andi.w  #0x303F,d1      /* 0 0 s a 0 0 0 0 0 0 c b r l d u */
        move.b  d1,d2           /* 0 0 0 0 m x y z 0 0 c b r l d u */
        lsr.w   #6,d1           /* 0 0 0 0 0 0 0 0 s a 0 0 0 0 0 0 */
        or.w    d1,d2           /* 0 0 0 0 m x y z s a c b r l d u */
        eori.w  #0x1FFF,d2      /* 0 0 0 1 M X Y Z S A C B R L D U */
        move.w  d2,d0
        move.l  (sp)+,d2
        rts

| no pad found, so we're going to ASSUME an SMS (compatible) pad
no_pad:
        lea     0xA10003,a0
        move.b  (a0),d0         /* - 1 c b r l d u */
        andi.w  #0x003F,d0      /* 0 0 0 0 0 0 0 0 0 0 c b r l d u */
        eori.w  #0x003F,d0      /* 0 0 0 0 0 0 0 0 0 0 C B R L D U */
        move.l  (sp)+,d2
        rts

| read single phase from controller
get_input:
        move.b  #0x00,(a0)
        nop
        nop
        move.b  (a0),d0
        move.b  #0x40,(a0)
        lsl.w   #8,d0
        move.b  (a0),d0
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

| select Neo Flash Game Flash ROM
| allows you to access the game flash via flash space
| entry: a1 = hardware base (0xA10000)
_neo_select_game:
        move.w  #0x0000,OPTION_IO(a1)   /* set mode 0 */

        move.l  #0x00370203,d0
        bsr     _neo_asic_cmd           /* set cr = select game flash */
        cmpi.b  #0,gCardType
        bne.b   1f
        move.l  #0x00DAAE44,d0          /* set iosr = enable game flash for newest cards */
        bra.b   3f
1:
        cmpi.b  #1,gCardType
        bne.b   2f
        move.l  #0x00DA8E44,d0          /* set iosr = enable game flash for new cards */
        bra.b   3f
2:
        cmpi.b  #2,gCardType
        bne.b   4f
        move.l  #0x00DA0E44,d0          /* set iosr = enable game flash for old cards */
3:
        bsr     _neo_asic_cmd           /* set iosr = enable game flash */
4:
        move.l  #0x00EE0630,d0
        bsr     _neo_asic_cmd           /* set cr1 = enable extended address bus */
        move.w  #0xFFFF,EXTM_ON(a1)     /* enable extended address bus */
        move.w  #0x0000,PRAM_BIO(a1)    /* set psram to bank 0 */
        move.w  #0x00F0,PRAM_ZIO(a1)    /* psram bank size = 2MB */
        move.w  #0x0006,WE_IO(a1)       /* map bank 7, write enable myth psram */
        move.w  #0x0007,OPTION_IO(a1)   /* set mode 7 (copy mode) */
        rts

| select Neo Flash PSRAM
| allows you to access the flash card's psram via flash space
| entry: a1 = hardware base (0xA10000)
_neo_select_psram:
        move.w  #0x0000,OPTION_IO(a1)   /* set mode 0 */
        move.w  #0x0707,WE_IO(a1)       /* map bank 7, write enable myth psram & flash psram, set SYS */

        move.l  #0x00E21500,d0
        bsr     _neo_asic_cmd           /* set flash & psram write enable */
        move.l  #0x00372203,d0
        bsr     _neo_asic_cmd           /* set cr = flash & psram we, select game flash */
        move.l  #0x00DAAF44,d0
        bsr     _neo_asic_cmd           /* set iosr = enable psram */
        move.l  #0x00C40000,d0
        bsr     _neo_asic_cmd           /* set flash bank to 0 */

	move.w	#0x0000,DAT_SWAP(a1)	/* enable byte swap function */
        move.w  #0x0000,GBAC_LIO(a1)    /* clear low bank select reg */
        move.w  #0x0000,GBAC_HIO(a1)    /* clear high bank select reg */
        move.w  #0x00F8,GBAC_ZIO(a1)    /* set bank size reg to 1MB */
        move.w  #0x0000,PRAM_BIO(a1)    /* set psram to bank 0 */
        move.w  #0x00F0,PRAM_ZIO(a1)    /* psram bank size = 2MB */
        move.w  #0x0707,WE_IO(a1)       /* map bank 7, write enable myth psram & flash psram, set SYS */
        move.w  #0x0707,OPTION_IO(a1)   /* set mode 7 (copy mode) */
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

| set the flash space bank registers
| entry: d0 = flash rom bank offset
|        a1 = hardware base (0xA10000)
_neo_set_fbank:
        lsr.l   #1,d0                   /* bus is in words */
        swap    d0
        moveq   #0,d1
        move.b  d0,d1
        move.w  d1,GBAC_LIO(a1)         /* set A23-16 */
        lsr.w   #8,d0
        andi.w  #0x000F,d0
        move.w  d0,GBAC_HIO(a1)         /* set A27-24 */
        rts

| set the flash space bank size
| entry: d0 = flash rom bank size
|        a1 = hardware base (0xA10000)
_neo_set_fsize:
        swap    d0
        move.w  #0x00FF,d1
        cmpi.w  #0x0002,d0
        bls.b   1f                      /* <= 1Mbit */
        move.w  #0x00FE,d1
        cmpi.w  #0x0004,d0
        bls.b   1f                      /* <= 2Mbit */
        move.w  #0x00FC,d1
        cmpi.w  #0x0008,d0
        bls.b   1f                      /* <= 4Mbit */
        /* greatest size for flash rom bank is 8Mbit */
        move.w  #0x00F8,d1
1:
        move.w  d1,GBAC_ZIO(a1)
        rts

| set the Neo Myth PSRAM bank size
| entry: d0 = neo myth psram bank size
|        a1 = hardware base (0xA10000)
_neo_set_myth_psize:
        swap    d0
        move.w  #0x00FF,d1
        cmpi.w  #0x0002,d0
        bls.b   1f                      /* <= 1Mbit */
        move.w  #0x00FE,d1
        cmpi.w  #0x0004,d0
        bls.b   1f                      /* <= 2Mbit */
        move.w  #0x00FC,d1
        cmpi.w  #0x0008,d0
        bls.b   1f                      /* <= 4Mbit */
        move.w  #0x00F8,d1
        cmpi.w  #0x0010,d0
        bls.b   1f                      /* <= 8Mbit */
        move.w  #0x00F0,d1
        cmpi.w  #0x0020,d0
        bls.b   1f                      /* <= 16Mbit */
        move.w  #0x00E0,d1
        cmpi.w  #0x0040,d0
        bls.b   1f                      /* <= 32Mbit */
        /* > 32Mbit => 32Mbit with SSF2 bank selection */
        move.w  #0x0000,d1
1:
        move.w  #0x0000,PRAM_BIO(a1)    /* set to bank 0 */
        move.w  d1,PRAM_ZIO(a1)         /* set bank size */
        rts

| set the sram space bank and size registers
| entry: d0 = sram bank
|        d1 = sram size (MD space)
|        a1 = hardware base (0xA10000)
_neo_set_sram:
        tst.l   d1
        beq     10f                     /* no sram */

        move.l  d2,-(sp)
        move.l  d0,d2

        cmpi.l  #0x02000,d1
        bhi.b   1f
|        andi.w  #0x000F,d0              /* 16 banks per 64KB sram */
        andi.w  #0x0007,d0              /* 8 banks since banks cannot be 4KB */
        move.w  #0x001F,d1              /* 64 Kbit space */
        lsr.w   #3,d2                   /* 64 KB bank # */
        lsl.w   #3,d2                   /* * 8 (KB) = sram bank offset */
        bra.b   9f
1:
        cmpi.l  #0x04000,d1
        bhi.b   2f
        andi.w  #0x0007,d0              /* 8 banks per 64KB sram */
        move.w  #0x001E,d1              /* 128 Kbit space */
        lsr.w   #3,d2                   /* 64 KB bank # */
        lsl.w   #3,d2                   /* * 8 (KB) = sram bank offset */
        bra.b   9f
2:
        cmpi.l  #0x08000,d1
        bhi.b   3f
        andi.w  #0x0003,d0              /* 4 banks per 64KB sram */
        lsl.w   #1,d0                   /* bank #0 to 3 => 0, 2, 4, 6 */
        move.w  #0x001C,d1              /* 256 Kbit space*/
        lsr.w   #2,d2                   /* 64 KB bank # */
        lsl.w   #3,d2                   /* * 8 (KB) = sram bank offset */
        bra.b   9f
3:
        cmpi.l  #0x10000,d1
        bhi.b   4f
        andi.w  #0x0001,d0              /* 2 banks per 64KB sram */
        lsl.w   #2,d0                   /* bank #0 to 1 => 0, 4 */
        move.w  #0x0018,d1              /* 512 Kbit space */
        lsr.w   #1,d2                   /* 64 KB bank # */
        lsl.w   #3,d2                   /* * 8 (KB) = sram bank offset */
        bra.b   9f
4:
        moveq   #0,d0                   /* 1 bank per 64KB sram */
        move.w  #0x0010,d1              /* 1 Mbit space */
        lsl.w   #3,d2                   /* * 8 (KB) = sram bank offset */
9:
        move.w  d0,GBAS_BIO(a1)
        move.w  d1,GBAS_ZIO(a1)

        move.w  #0x00E0,d0
        swap    d0
        move.w  d2,d0
        bsr     _neo_asic_cmd           /* set sram bank offset */

        move.l  (sp)+,d2
10:
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

| clear IO registers - forces game to run cold reset code
| entry: a1 = hardware base (0xA10000)
_clear_hw:
        movea.l a1,a0
        moveq   #0,d0
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)+
        move.b  d0,(a0)
        move.w  #0x8104,0xC00004        /* display off, vblank disabled */
        rts

        .ifdef  DEBUG_ASM
_debug_print_d0:
        move.l  d2,-(sp)
        move.w  #0x8F02,0xC00004
        move.l  #0x60000003,0xC00004    /* VDP write VRAM at 0xE000 (scroll plane B) */
        moveq   #7,d2
1:
        rol.l   #4,d0
        move.b  d0,d1
        andi.w  #15,d1
        cmpi.w  #10,d1
        blo.b   2f
        addq.w  #7,d1
2:
        addi.w  #16,d1
        move.w  d1,0xC00000
        dbra    d2,1b
3:
|       bra     3b
        move.l  (sp)+,d2
        rts
        .endif

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

| void neo_get_rtc(unsigned char *rtc);
| entry: arg is pointer to array of unsigned bytes large enough for RTC
        .global neo_get_rtc
neo_get_rtc:
        .ifdef  SUPPORT_RTC
        lea     0xA10000,a1
        bsr     _neo_enable_id

        lea     0x200000,a0             /* sram space */
        adda.l  #NEO2_RTC*2,a0          /* RTC registers */
        movea.l 4(sp),a1                /* pointer to RTC save variable array */

        /* copy RTC bytes */
        move.b  0x001(a0),(a1)+         /* one second */
        move.b  0x201(a0),(a1)+         /* ten seconds */
        move.b  0x401(a0),(a1)+         /* one minute */
        move.b  0x601(a0),(a1)+         /* ten minutes */
        move.b  0x801(a0),(a1)+         /* hours */
        move.b  0xA01(a0),(a1)+         /* low days */
        move.b  0xC01(a0),(a1)+         /* mid days */
        move.b  0xE01(a0),(a1)+         /* high days */

        lea     0xA10000,a1
        bsr     _neo_disable_id
        bsr     _neo_select_menu
        .endif
        rts

| set USB active and wait for user press A
        .global set_usb
set_usb:
        lea     0xA10000,a0
        move.w  #0xFFFF,USB_ON(a0)      /* activate USB */
1:
        move.w  #0x3FFF,d0
2:
        dbra    d0,2b                   /* delay */

        /* read pad */
        move.b  #0x00,3(a0)
        nop
        nop
        move.b  3(a0),d0
        move.b  #0x40,3(a0)
        lsl.w   #8,d0
        move.b  3(a0),d0                /* - 0 s a 0 0 d u - 1 c b r l d u */
        move.w  d0,d1
        andi.w  #0x0C00,d1
        beq.b   3f                      /* pad in port 1 */
        move.b  #0x00,5(a0)
        nop
        nop
        move.b  5(a0),d0
        move.b  #0x40,5(a0)
        lsl.w   #8,d0
        move.b  5(a0),d0                /* - 0 s a 0 0 d u - 1 c b r l d u */
3:
        lsr.w   #8,d0
        btst    #4,d0
        bne.b   1b                      /* wait for A pressed */

        move.w  #0x0000,USB_ON(a0)      /* deactivate USB */
4:
        move.w  #0x3FFF,d0
5:
        dbra    d0,5b                   /* delay */

        /* read pad */
        move.b  #0x00,3(a0)
        nop
        nop
        move.b  3(a0),d0
        move.b  #0x40,3(a0)
        lsl.w   #8,d0
        move.b  3(a0),d0                /* - 0 s a 0 0 d u - 1 c b r l d u */
        move.w  d0,d1
        andi.w  #0x0C00,d1
        beq.b   6f                      /* pad in port 1 */
        move.b  #0x00,5(a0)
        nop
        nop
        move.b  5(a0),d0
        move.b  #0x40,5(a0)
        lsl.w   #8,d0
        move.b  5(a0),d0                /* - 0 s a 0 0 d u - 1 c b r l d u */
6:
        lsr.w   #8,d0
        btst    #4,d0
        beq.b   4b                      /* wait for A released */
        rts

| void neo_run_game(int fstart, int fsize, int bbank, int bsize, int run);
| run directly from flash rom
        .global neo_run_game
neo_run_game:
        lea     0xA10000,a1
        bsr     _neo_select_game        /* select Game Flash */
        bra.b   neo_chk_mahb

| void neo_run_psram(int pstart, int psize, int bbank, int bsize, int run);
| run from flash card psram
        .global neo_run_psram
neo_run_psram:
        lea     0xA10000,a1
        bsr     _neo_select_psram       /* select flash cart PSRAM */

|        move.w  #0x0000,WE_IO(a1)       /* write-protect psram */
|        move.l  #0x00E2D200,d0
|        bsr     _neo_asic_cmd           /* write-protect flash cart psram */

neo_chk_mahb:
        cmpi.l  #7,20(sp)               /* check for EBIOS run mode */
        bne.b   neo_run_sms

| run EBIOS/MAHB from game flash/psram
        move.l  4(sp),d0                /* fstart */
        bsr     _neo_set_fbank          /* set the flash space bank registers */
        move.w  #0x00F8,GBAC_ZIO(a1)    /* set flash space to 1MB */
        move.w  #0x0007,OPTION_IO(a1)   /* set mode 7 (copy mode) */
        move.w  #0x0006,PRAM_BIO(a1)    /* set the neo myth psram bank register */
        move.w  #0x00F8,PRAM_ZIO(a1)    /* set the neo myth psram bank size to 1MB */
        move.w  #0x0707,WE_IO(a1)       /* write-enable psram, map bank 7 to top of rom space */
        bsr     _clear_hw
        movea.l 0.w,a7
        movea.l 4.w,a3
        jmp     (a3)

neo_run_sms:
        move.l  4(sp),d0                /* start */
        bsr     _neo_set_fbank          /* set the flash space bank registers */
        move.l  8(sp),d0                /* size */
        bsr     _neo_set_fsize          /* set the flash space bank size */
        move.l  12(sp),d0               /* bbank */
        move.l  16(sp),d1               /* bsize */
        bsr     _neo_set_sram           /* set the sram space bank and size registers */

        bsr     _clear_hw
        move.l  20(sp),d0               /* run */
        move.w  gYM2413,d1              /* 0x0000 = YM2413 disabled, 0x0001 = YM2413 enabled */
        ori.w   #0x00FE,d1
        and.w   d1,d0
        move.w  d0,OPTION_IO(a1)        /* set run mode for game */

        moveq   #0,d0
        move.w  d0,0xFF8000
        move.w  d0,0xFF8002
        move.w  #0x00FF,d0
        move.w  d0,RST_IO(a1)
        move.w  d0,RUN_IO(a1)
        movea.l 0.w,a7
        movea.l 4.w,a3
        jmp     (a3)

| void neo_run_myth_psram(int psize, int bbank, int bsize, int run);
| run from neo myth psram
        .global neo_run_myth_psram
neo_run_myth_psram:
        lea     0xA10000,a1
        bsr     _neo_select_menu        /* select Neo Myth menu flash */

        move.l  16(sp),d0               /* run */
        bclr    #5,d0
        bne.b   9f                      /* EBIOS or MA-Homebrew */

        move.l  4(sp),d0                /* psize */
        bsr     _neo_set_myth_psize     /* set the Neo Myth PSRAM bank size */
        move.l  8(sp),d0                /* bbank */
        move.l  12(sp),d1               /* bsize */
        bsr     _neo_set_sram           /* set the sram space bank and size registers */

        bsr     _clear_hw

        move.l  16(sp),d0               /* run */
        cmpi.w  #0x0009,d0              /* SCD-RAM */
        bne.b   1f
        move.l  #0xFF04FF04,0x1C8.w     /* fix memo overwritten by MYTH header */
        move.l  #0xFF04FF04,0x1CC.w
1:
        move.w  d0,OPTION_IO(a1)        /* set run mode for game */

        move.w  #0x00FF,RST_SEL(a1)     /* 0x0001 = , 0x0004 = , 0x00FF = */
        move.w  gWriteMode,WE_IO(a1)    /* 0x0000 = write protected, 0x0002 = write enabled */
        move.w  gResetMode,RST_IO(a1)   /* 0x0000 = reset to menu, 0x00FF = reset to game */
        move.w  gGameMode,RUN_IO(a1)    /* 0x0000 = menu mode, 0x00FF = game mode */
        movea.l 0.w,a7
        movea.l 4.w,a3
        jmp     (a3)
9:
        move.w  #0x0006,PRAM_BIO(a1)    /* set psram to bank 6 */
        move.w  #0x00F0,PRAM_ZIO(a1)    /* psram bank size = 2MB */
        move.w  #0x0007,OPTION_IO(a1)   /* set run mode for game */
        move.w  #0x0006,WE_IO(a1)       /* write-enable psram, map bank 7 to 0x300000 */

        /* copy data from 0x200000 to 0x300000 */
        move.l  4(sp),d0                /* psize */
        lea     0x200000,a0
        lea     0x300000,a1
8:
        move.w  (a0)+,(a1)+
        subq.l  #2,d0
        bne.b   8b

        movea.l 0x300000,a7
        movea.l 0x300004,a3
        jmp     (a3)

| void neo_copy_game(unsigned char *dest, int fstart, int len);
        .global neo_copy_game
neo_copy_game:
        lea     0xA10000,a1
        bsr     _neo_select_game        /* select Game Flash */

        move.l  8(sp),d0                /* fstart */
        bsr     _neo_set_fbank          /* set the flash space bank registers */
        move.l  #0x100000,d0
        bsr     _neo_set_fsize          /* set the flash space bank size to 1MB */

        movea.l 4(sp),a1                /* dest */
        move.l  8(sp),d1                /* fstart */
        andi.l  #0x01FFFE,d1            /* offset inside flash space (bank was set to closest 128KB) */
        movea.l d1,a0
        move.l  12(sp),d0               /* len */
        lsr.l   #1,d0                   /* # words to copy */
        subq.w  #1,d0
1:
        move.w  (a0)+,(a1)+
        dbra    d0,1b

        lea     0xA10000,a1
        bra     _neo_select_menu        /* select Menu Flash */

| void neo_copyto_psram(unsigned char *src, int pstart, int len);
        .global neo_copyto_psram
neo_copyto_psram:
        lea     0xA10000,a1
        bsr     _neo_select_psram       /* select flash cart PSRAM (write-enabled) */
	move.w	#0xFFFF,DAT_SWAP(a1)	/* disable byte swap function */

        movea.l 4(sp),a0                /* src */
        movea.l 8(sp),a1                /* pstart */
        move.l  12(sp),d0               /* len */
        lsr.l   #1,d0                   /* # words to copy */
        subq.w  #1,d0
1:
        move.w  (a0)+,d1
	ror.w	#8,d1
	move.w	d1,(a1)+
        dbra    d0,1b

        lea     0xA10000,a1
        move.w  #0x0000,WE_IO(a1)       /* write-protect psram */
	move.w	#0x0000,DAT_SWAP(a1)	/* enable byte swap function */
        move.l  #0x00E2D200,d0
        bsr     _neo_asic_cmd           /* write-protect flash cart psram */
        bra     _neo_select_menu        /* select Menu Flash */

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

| void neo_copyfrom_myth_psram(unsigned char *dst, int pstart, int len);
        .global neo_copyfrom_myth_psram
neo_copyfrom_myth_psram:
        lea     0xA10000,a1

        move.l  8(sp),d0                /* pstart */
        move.w  #20,d1
        lsr.l   d1,d0                   /* bank = pstart / 1MB  */
        move.w  d0,PRAM_BIO(a1)         /* set the neo myth psram bank register */
        move.w  #0x00F8,PRAM_ZIO(a1)    /* set the neo myth psram bank size to 1MB */

        move.w  #0x0007,OPTION_IO(a1)   /* set mode 7 (copy mode) */
        move.w  #0x0006,WE_IO(a1)       /* map bank 7, write-enable neo myth psram */

        movea.l 4(sp),a0                /* dst */
        move.l  8(sp),d1                /* pstart */
        andi.l  #0x0FFFFF,d1            /* offset inside sram space (bank was set to closest 1MB) */
        ori.l   #0x200000,d1            /* sram space access */
        movea.l d1,a1
        move.l  12(sp),d0               /* len */
        lsr.l   #1,d0                   /* # words to copy */
        subq.w  #1,d0
1:
        move.w  (a1)+,(a0)+
        dbra    d0,1b

        lea     0xA10000,a1
        bra     _neo_select_menu        /* select Menu Flash */

| void neo_copyto_sram(unsigned char *src, int sstart, int len);
        .global neo_copyto_sram
neo_copyto_sram:
        lea     0xA10000,a1
        bsr     _neo_select_game        /* select Game Flash */

        move.w  #0x0000,GBAC_LIO(a1)    /* clear low bank select reg */
        move.w  #0x0000,GBAC_HIO(a1)    /* clear high bank select reg */
        move.w  #0x00F8,GBAC_ZIO(a1)    /* bank size = 1MB */

        move.l  #0x00E00000,d0
        move.l  8(sp),d1                /* sstart */
        swap    d1
        lsl.w   #3,d1                   /* # of 8KB blocks */
        move.w  d1,d0
        bsr     _neo_asic_cmd           /* set the sram bank offset */

        move.w  #0x0000,GBAS_BIO(a1)    /* set the neo myth sram bank register */
        move.w  #0x0010,GBAS_ZIO(a1)    /* set the neo myth psram bank size to 1Mb */

        move.w  #0x0001,OPTION_IO(a1)   /* set mode 1 */

        movea.l 4(sp),a0                /* src */
        move.l  8(sp),d1                /* sstart */
        andi.l  #0x00FFFF,d1
        add.l   d1,d1                   /* offset inside sram space (bank was set to closest block) */
        ori.l   #0x200000,d1            /* sram space access */
        movea.l d1,a1
        move.l  12(sp),d0               /* len */
        subq.w  #1,d0
1:
        move.b  (a0)+,1(a1)
        lea     2(a1),a1
        dbra    d0,1b

        lea     0xA10000,a1
        bra     _neo_select_menu        /* select Menu Flash */

| void neo_copyfrom_sram(unsigned char *dst, int sstart, int len);
        .global neo_copyfrom_sram
neo_copyfrom_sram:
        lea     0xA10000,a1
        bsr     _neo_select_game        /* select Game Flash */

        move.w  #0x0000,GBAC_LIO(a1)    /* clear low bank select reg */
        move.w  #0x0000,GBAC_HIO(a1)    /* clear high bank select reg */
        move.w  #0x00F8,GBAC_ZIO(a1)    /* bank size = 1MB */

        move.l  #0x00E00000,d0
        move.l  8(sp),d1                /* sstart */
        swap    d1
        lsl.w   #3,d1                   /* # of 8KB blocks */
        move.w  d1,d0
        bsr     _neo_asic_cmd           /* set the sram bank offset */

        move.w  #0x0000,GBAS_BIO(a1)    /* set the neo myth sram bank register */
        move.w  #0x0010,GBAS_ZIO(a1)    /* set the neo myth psram bank size to 1Mb */

        move.w  #0x0001,OPTION_IO(a1)   /* set mode 1 */

        movea.l 4(sp),a0                /* dst */
        move.l  8(sp),d1                /* sstart */
        andi.l  #0x00FFFF,d1
        add.l   d1,d1                   /* offset inside sram space (bank was set to closest block) */
        ori.l   #0x200000,d1            /* sram space access */
        movea.l d1,a1
        move.l  12(sp),d0               /* len */
        subq.w  #1,d0
1:
        move.b  1(a1),(a0)+
        lea     2(a1),a1
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

| int neo2_recv_sd_multi(unsigned char *buf, int count);
        .global neo2_recv_sd_multi
neo2_recv_sd_multi:
        movem.l d2-d3,-(sp)
        lea     0xA10000,a1
        movea.l 12(sp),a0               /* buf */
        move.l  16(sp),d3               /* count */
        subq.w  #1,d3

        move.w  #0x0087,GBAC_LIO(a1)
0:
        move.w  #1023,d0
1:
        moveq   #1,d1
        and.w   0x6060.w,d1
        dbeq    d0,1b
        bne.b   9f                      /* timeout */

        moveq   #127,d0
2:
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

        dbra    d0,2b

        moveq   #7,d0
3:
        move.w  0x6060.w,d1
        lsl.b   #4,d1
        move.w  0x6060.w,d2
        andi.w  #0x000F,d2
        or.b    d2,d1
|       move.b  d1,(a0)+                /* CRC byte */
        dbra    d0,3b

        move.w  0x6060.w,d1             /* end bit */

        dbra    d3,0b

        move.w  #0x0080,GBAC_LIO(a1)
        movem.l (sp)+,d2-d3
        moveq   #1,d0                   /* TRUE */
        rts
9:
        move.w  #0x0080,GBAC_LIO(a1)
        movem.l (sp)+,d2-d3
        moveq   #0,d0                   /* FALSE */
        rts

neo_mode:
        .word   0


        .text

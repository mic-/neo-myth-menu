.include "hdr.asm"

.ramsection ".registers" bank 0 slot 1
tcc__registers dsb 0
tcc__r0 dsb 2
tcc__r0h dsb 2
tcc__r1 dsb 2
tcc__r1h dsb 2
tcc__r2 dsb 2
tcc__r2h dsb 2
tcc__r3 dsb 2
tcc__r3h dsb 2
tcc__r4 dsb 2
tcc__r4h dsb 2
tcc__r5 dsb 2
tcc__r5h dsb 2
tcc__r9 dsb 2
tcc__r9h dsb 2
tcc__r10 dsb 2
tcc__r10h dsb 2
; f0/f1 defined in libm.asm
tcc__f2 dsb 2
tcc__f2h dsb 2
tcc__f3 dsb 2
tcc__f3h dsb 2
move_insn dsb 4	; 3 bytes mvn + 1 byte rts
move_backwards_insn dsb 4 ; 3 bytes mvp + 1 byte rts
__nmi_handler dsb 4

tcc__registers_irq dsb 0
tcc__regs_irq dsb 48
.ends


.BANK 1 SLOT 0                  ; Defines the ROM bank and the slot it is inserted in memory.
.ORG 0                          ; .ORG 0 is really $8000, because the slot starts at $8000
.SECTION "EmptyVectors" SEMIFREE

jmp.l	tcc__start

EmptyHandler:
       rti

EmptyNMI:
       rtl

.ENDS




; Needed to satisfy interrupt definition in "Header.inc".
VBlank:
  rti


.bank 1
.section ".start"

.accu 16
.index 16
.16bit

      
tcc__start:
    ; Initialize the SNES.
    sei             ; Disabled interrupts
    clc             ; clear carry to switch to native mode
    xce             ; Xchange carry & emulation bit. native mode
    rep     #$18    ; Binary mode (decimal mode off), X/Y 16 bit
    ldx     #$1FFF  ; set stack to $1FFF
    ;ldx	#$ff
    txs

    ;jsr tcc__snesinit

 rep #$30	; all registers 16-bit

    ; direct page points to register set
    lda.w #tcc__registers
    tad

    lda.w #EmptyNMI
    sta.b __nmi_handler
    lda.w #:EmptyNMI
    sta.b __nmi_handler + 2

    lda #$80
    sta.l $2100
    
    ; copy .data section to RAM
    ldx #0
-   lda.l __startsection.data & $03FFFF,x
    sta.l __startramsectionram.data,x
    inx
    inx
    cpx #(__endsection.data-__startsection.data)
    bcc -

    ; set data bank register to bss section
    pea $7e7e
    plb
    plb

    ; clear .bss section
    ldx #(((__endramsection.bss-__startramsection.bss) & $fffe) + 2)
    beq +
-   dex
    dex
    stz.w $2000, x
    bne -
+


    ; used by memcpy
    lda #$0054 ; mvn + 1st byte
    sta.b move_insn
    lda #$6000 ; 2nd byte + rts
    sta.b move_insn + 2

    ; used by memmove
    lda #$0044 ; mvp + 1st byte
    sta.b move_backwards_insn
    lda #$6000 ; 2nd byte + rts
    sta.b move_backwards_insn + 2

    pea $ffff - __endramsectionram.data
    pea :__endramsectionram.data
    pea __endramsectionram.data
    jsr.l __malloc_init
    pla
    pla
    pla

    stz.b tcc__r0
    stz.b tcc__r1

    jsr.l main

    ; write exit code to $fffd
    lda.b tcc__r0
    sep #$20
    sta $fffd
    rep #$20
    stp
.ends

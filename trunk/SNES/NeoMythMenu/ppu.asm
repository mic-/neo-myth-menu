
.include "hdr.asm"
.include "snes_io.inc"
.include "myth_io.inc"

.bank 3 slot 0
.section "text_ppu"

ppu_ram_code_begin:

; Enable the mosaic effect for BG0 (the text layer), and slide the mosaic size up from 0 to
; the maximum in 16 frames.
mosaic_up:
	php
	rep	#$10
	sep	#$20
	ldx	#0
-:
	jsr.w	_wait_nmi
	txa
	asl	a
	asl	a
	asl	a
	asl	a		; Mosaic size in d4-d7
	ora	#1		; Enable effect for BG0
	sta.l	REG_MOSAIC
	inx
	cpx	#$10
	bne	-
	plp
	rtl


; Enable the mosaic effect for BG0 (the text layer), and slide the mosaic size down from the maximum
; to 0 in 16 frames.
mosaic_down:
	php
	rep	#$10
	sep	#$20
	ldx	#14
-:
	jsr.w	_wait_nmi
	txa
	asl	a
	asl	a
	asl	a
	asl	a		; Mosaic size in d4-d7
	ora	#1		; Enable effect for BG0
	sta.l	REG_MOSAIC
	dex
	bpl	-
	plp
	rtl
	

; Unlike the wait_nmi routine in main.c, this routine is copied to RAM and is expected to be called
; only by other routines located in RAM (hence the rts instead of rtl).
_wait_nmi:
	php
	sep	#$20
-:
	lda.l	REG_RDNMI
	bmi	-
-:
	lda.l	REG_RDNMI
	bpl	-
	plp
	rts
	
	
ppu_ram_code_end:

.ends


; Routines for DMA-transfering data to CGRAM or VRAM
; See dma.h for function prototypes.
; /Mic, 2010

.include "hdr.asm"

;.bank 3 slot 0
.section ".text_dma" superfree


load_cgram:
	php
	rep	#$20
	lda	11,s	; numBytes
	sta.l	$4305
	lda	5,s	; src (lower 16 bits)
	sta.l	$4302
	sep	#$20
	lda	7,s	; src bank
	sta.l	$4304
	lda	9,s	; cgramOffs
	sta.l	$2121
	lda	#0
	sta.l	$4300
	lda	#$22
	sta.l	$4301
	lda	#1
	sta.l	$420b
	plp
	rtl


load_vram:
	php
	rep	#$20
	lda	9,s	; vramOffs
	sta.l	$2116
	lda	11,s	; numBytes
	sta.l	$4305
	lda	5,s	; src (lower 16 bits)
	sta.l	$4302
	sep	#$20
	lda	7,s	; src bank
	sta.l	$4304
	lda	#1
	sta.l	$4300
	lda	#$80
	sta.l	$2115
	lda	#$18
	sta.l	$4301
	lda	#1
	sta.l	$420b
	plp
	rtl


.ends

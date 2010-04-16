; Routines for SNES hardware math functions
; See dma.h for function prototypes.
; /Mic, 2010

.include "hdr.asm"

.section ".text_hwmath" superfree

; Divide a 16-bit number by an 8-bit number and return the 16-bit quotient
hw_div16_8_quot16:
	sep	#$20
	lda	4,s
	sta.l 	$4204   	; Dividend low
	lda 	5,s
	sta.l 	$4205 		; Dividend high
	lda 	6,s
	sta.l 	$4206
	nop			; Wait for the division to finish
	nop			; ...
	nop	
	nop
	nop
	nop
	nop
	nop
	rep	#$20
	lda.l	$4214		; Quotient
	sta.b 	tcc__r0		; Store return value
	rtl
	

; Divide a 16-bit number by an 8-bit number and return the 16-bit remainder	
hw_div16_8_rem16:
	sep	#$20
	lda	4,s
	sta.l 	$4204   	; Dividend low
	lda 	5,s
	sta.l 	$4205 		; Dividend high
	lda 	6,s
	sta.l 	$4206
	nop			; Wait for the division to finish
	nop			; ...
	nop	
	nop
	nop
	nop
	nop
	nop
	rep	#$20
	lda.l	$4216		; Remainder
	sta.b 	tcc__r0		; Store return value
	rtl

; Divide a 16-bit number by an 8-bit number and return the 8-bit quotient and 8-bit remainder
hw_div16_8_quot8_rem8:
	sep	#$20
	lda	4,s
	sta.l 	$4204   	; Dividend low
	lda 	5,s
	sta.l 	$4205 		; Dividend high
	lda 	6,s
	sta.l 	$4206
	nop			; Wait for the division to finish
	nop			; ...
	nop	
	nop
	nop
	nop
	nop
	nop
	lda.l	$4216		; Remainder
	xba			; put remainder in high byte
	lda.l	$4214		; Quotient in low byte
	rep	#$20
	sta.b 	tcc__r0		; Store return value
	rtl

.ends

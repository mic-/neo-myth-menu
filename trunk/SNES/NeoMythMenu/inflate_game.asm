; inflate - uncompress data stored in the DEFLATE format
; by Piotr Fusik <fox@scene.pl>
; Last modified: 2007-06-17

; inflate is 509 bytes of code and initialized data
; inflate_data is 764 bytes of uninitialized data
; inflate_zp is 10 bytes on page zero

; SNES version by Mic, 2011

.include "hdr.asm"
.include "snes_io.inc"
.include "myth_io.inc"

.bank 3 slot 0
.section "text_inflate_game"


.EQU inflate_zp $80


; Pointer to compressed data
.EQU inputPointer                    inflate_zp    ; 3 bytes

; Pointer to uncompressed data
.EQU outputPointer                   inflate_zp+3  ; 3 bytes

; Local variables

.EQU getBit_buffer                   inflate_zp+6  ; 1 byte

.EQU getBits_base                    inflate_zp+7  ; 1 byte
.EQU inflateStoredBlock_pageCounter  inflate_zp+7  ; 1 byte

.EQU inflateCodes_sourcePointer      inflate_zp+8  ; 3 bytes

.EQU inflateDynamicBlock_lengthIndex inflate_zp+11  ; 1 byte
.EQU inflateDynamicBlock_lastLength	inflate_zp+12  ; 1 byte
.EQU inflateDynamicBlock_tempCodes   inflate_zp+12  ; 1 byte

.EQU inflateCodes_lengthMinus2       inflate_zp+13  ; 1 byte
.EQU inflateDynamicBlock_allCodes    inflate_zp+13  ; 1 byte

.EQU inflateCodes_primaryCodes       inflate_zp+14  ; 1 byte

;.EQU wordOutputPointer               inflate_zp+16  ; 3 bytes
;.EQU getBit_rem						inflate_zp+16

; Argument values for getBits
.EQU GET_1_BIT                       $81
.EQU GET_2_BITS                      $82
.EQU GET_3_BITS                      $84
.EQU GET_4_BITS                      $88
.EQU GET_5_BITS                      $90
.EQU GET_6_BITS                      $a0
.EQU GET_7_BITS                      $c0

; Maximum length of a Huffman code
.EQU MAX_CODE_LENGTH                 15

; Huffman trees
.EQU TREE_SIZE                       MAX_CODE_LENGTH+1
.EQU PRIMARY_TREE                    0
.EQU DISTANCE_TREE                   TREE_SIZE

; Alphabet
.EQU LENGTH_SYMBOLS                  1+29+2
.EQU DISTANCE_SYMBOLS                30
.EQU CONTROL_SYMBOLS                 LENGTH_SYMBOLS+DISTANCE_SYMBOLS
.EQU TOTAL_SYMBOLS                   256+CONTROL_SYMBOLS

.EQU INPUT_BASE $500000

inflate_game_ram_code_begin:



; Uncompress DEFLATE stream starting from the address stored in inputPointer
; to the memory starting from the address stored in outputPointer


; DWORD inflate_game()
;
inflate_game:
	php
	rep 	#$30
	pha
	phx
	phy
	;stz		tcc__r0
	;stz		tcc__r1
	sep 	#$30	; 8-bit A/X/Y

;	LDA    #$20      		; OFF A21
;	STA.L  MYTH_GBAC_ZIO
;	JSR    SET_NEOCMA  		;
;	JSR    SET_NEOCMB  		;
;	JSR    SET_NEOCMC  		; ON_NEO CARD A24 & A25 + SA16 & SA17
;	;LDA    #$01
;	;STA.L  MYTH_EXTM_ON  	; A25,A24 ON
;	LDA    #$01       		; 
;	STA.L  MYTH_OPTION_IO
;	LDA    #$01       		; PSRAM WE ON !
;	STA.L  MYTH_WE_IO
;	;LDA    #$F8
;	;STA.L  MYTH_GBAC_ZIO  	; GBA CARD 8M SIZE
;	LDA    #$F0
;	STA.L  MYTH_PRAM_ZIO  	; PSRAM    16M SIZE
;	LDA    #$00
;	STA.L  MYTH_PRAM_BIO

	jsr.w _neo_select_psram
	sep 	#$30
    	lda    	#$F0
    	sta.l  	MYTH_PRAM_ZIO	; PSRAM 16M SIZE
	LDA    	#1 
	STA.L  	MYTH_OPTION_IO
	lda 	#1
	sta.l 	MYTH_WE_IO
	LDA    	#$00
	STA.L  	MYTH_PRAM_BIO
	
	stz 		getBit_buffer

	lda		#2
	sta 		tcc__r0h

	lda		#$D0
	sta		inputPointer+2
	lda		#$40 
	sta		outputPointer+2

	rep		#$20
	stz		inputPointer
	stz		outputPointer
	
	; Compressed bitstream starts at offset $1e + sizeof(filename) + sizeof(extra_field)
	lda.l		INPUT_BASE+$1a		; filename length
	sta		tcc__r0
	clc
	adc.l		INPUT_BASE+$1c
	clc
	adc		#$1e
	sta		inputPointer

 	
	sep		#$20
	lda.l		INPUT_BASE+8		; compression type
	cmp		#8			; deflate
	beq		+
	rep		#$20
	lda		#1
	sta		tcc__r0
	lda		#$8000
	sta		tcc__r1
	jmp.w		inflate_game_return
	sep		#$20
+:

	; Is the first file in the ZIP an SMC file?
	ldx		#$1e		; filename offset
	ldy		#0
-:
	lda.l		INPUT_BASE,x
	cmp		#'.'
	beq		+
	inx
	iny
	cpy		tcc__r0
	bne		-
+:
	cpy		tcc__r0
	bne		+
inflate_game_not_smc:
	rep		#$20
	lda		#2
	sta		tcc__r0
	lda		#$8000
	sta		tcc__r1
	jmp.w		inflate_game_return
	sep		#$20
+:	
	lda.l		INPUT_BASE+1,x
	cmp		#'s'
	beq		+
	cmp		#'S'
	bne		inflate_game_not_smc
+:	
	lda.l		INPUT_BASE+2,x
	cmp		#'m'
	beq		+
	cmp		#'M'
	bne		inflate_game_not_smc
+:	
	lda.l		INPUT_BASE+3,x
	cmp		#'c'
	beq		+
	cmp		#'C'
	bne		inflate_game_not_smc
+:	

 	lda		#0
 	sta.l		MYTH_PRAM_BIO
 
 	lda		#$01
	sta.l		REG_MEMSEL
	
	ldy 		#0
	ldx		#0

	jml 		$DE0000+inflate_game_blockLoop	; Jump to this code in PSRAM

inflate_game_blockLoop:
; Get a bit of EOF and two bits of block type
;	ldy	#0
	sty		getBits_base
	lda		#GET_3_BITS
	jsr		game_getBits
	lsr		a 
	php
	tax
	;bne		inflateGameCompressedBlock
	beq	+
	jmp.w	inflateGameCompressedBlock
	+:
	
; Copy uncompressed block
;	ldy	#0
	sty		getBit_buffer
	jsr		game_getWord
	jsr		game_getWord
	sta		inflateStoredBlock_pageCounter
;	jmp	inflateStoredBlock_firstByte
	bcs		inflateGameStoredBlock_firstByte
inflateGameStoredBlock_copyByte
	jsr		game_getByte
inflateGameStoreByte
	;jsr		storeByte

	dec 	tcc__r0h
	bne 	+
	; odd address
	xba
	lda 	tcc__r1h
	rep	#$20
	sta	[outputPointer]
	sep	#$20
	lda 	#2
	sta 	tcc__r0h
	inc 	outputPointer
	inc 	outputPointer
	bne	++
	rep	#$20
	inc	outputPointer+1
	sep	#$20
 +:
;	  ; even address
    sta	tcc__r1h
++:

  	bcs 	+
  	jmp.w 	inflateGameCodes_loop
+:
inflateGameStoredBlock_firstByte
	inx
	bne		inflateGameStoredBlock_copyByte
	inc		inflateStoredBlock_pageCounter
	bne		inflateGameStoredBlock_copyByte

inflate_game_nextBlock
	plp
	bcc		inflate_game_blockLoop

	; Calculate size of decompressed data
  	rep 	#$30
  	lda		tcc__r0h
  	and		#1
  	clc
  	adc 	outputPointer
  	sta 	tcc__r0
  	lda 	outputPointer+2
  	and 	#$FF
  	sec
  	sbc 	#$40 ;50
  	sta 	tcc__r1
 
 	;jsr.w show_copied_data
 	;-: bra -

jml $7D0000+ccccc		; Jump to this code in WRAM
ccccc:
 	
	jsr.w	inflate_remove_smc_header

	sep		#$20
 	lda		#$00
	sta.l	REG_MEMSEL

inflate_game_return:  
	sep		#$20
	LDA     #$00       ;
	STA.L   MYTH_WE_IO     ; PSRAM WRITE OFF
	LDA     #MAP_MENU_FLASH_TO_ROM	; SET GBA CARD RUN
	STA.L   MYTH_OPTION_IO
	LDA     #$20       		; OFF A21
	STA.L   MYTH_GBAC_ZIO
	JSR     SET_NEOCMD		; SET MENU
	LDA     #$00
	STA.L   MYTH_GBAC_LIO
	STA.L   MYTH_GBAC_HIO
	STA.L   MYTH_GBAC_ZIO

	rep	#$30
	ply
	plx
	pla
	plp
	rtl 


  sep #$30
inflateGameCompressedBlock:

; Decompress a block with fixed Huffman trees:
; :144 dta 8
; :112 dta 9
; :24  dta 7
; :6   dta 8
; :2   dta 8 ; codes with no meaning
; :30  dta 5+DISTANCE_TREE
;	ldy	#0
inflateGameFixedBlock_setCodeLengths
	lda		#4
	cpy		#144
	rol		a 
	sta.w 	literalSymbolCodeLength,y
	cpy		#CONTROL_SYMBOLS
	bcs		inflateGameFixedBlock_noControlSymbol
	lda		#5+DISTANCE_TREE
	cpy		#LENGTH_SYMBOLS
	bcs		inflateGameFixedBlock_setControlCodeLength
	cpy		#24
	adc		#2-DISTANCE_TREE
inflateGameFixedBlock_setControlCodeLength:
	sta.w	controlSymbolCodeLength,y
inflateGameFixedBlock_noControlSymbol:
	iny
	bne		inflateGameFixedBlock_setCodeLengths
	lda 	#LENGTH_SYMBOLS 
	sta 	inflateCodes_primaryCodes
	
	dex
	beq		inflateGameCodes

; Decompress a block reading Huffman trees first

; Build the tree for temporary codes
	jsr	game_buildTempHuffmanTree

; Use temporary codes to get lengths of literal/length and distance codes
	ldx	#0
;	sec
inflateGameDynamicBlock_decodeLength:
	php
	stx	inflateDynamicBlock_lengthIndex
; Fetch a temporary code
	jsr		game_fetchPrimaryCode
; Temporary code 0..15: put this length
	tax
	bpl	inflateGameDynamicBlock_verbatimLength
; Temporary code 16: repeat last length 3 + getBits(2) times
; Temporary code 17: put zero length 3 + getBits(3) times
; Temporary code 18: put zero length 11 + getBits(7) times
	jsr		game_getBits
;	sec
	adc		#1
	cpx		#GET_7_BITS
  	bcc 	+
  	adc 	#7
+:
	tay
	lda		#0
	cpx		#GET_3_BITS
  	bcs		+
  	lda 	inflateDynamicBlock_lastLength
+:
inflateGameDynamicBlock_verbatimLength:
	iny
	ldx		inflateDynamicBlock_lengthIndex
	plp
inflateGameDynamicBlock_storeLength:
	bcc		inflateGameDynamicBlock_controlSymbolCodeLength
	sta.w	literalSymbolCodeLength,x 
  	inx
	cpx	#1
inflateGameDynamicBlock_storeNext:
	dey
	bne		inflateGameDynamicBlock_storeLength
	sta		inflateDynamicBlock_lastLength
;	jmp		inflateDynamicBlock_decodeLength
	beq		inflateGameDynamicBlock_decodeLength
inflateGameDynamicBlock_controlSymbolCodeLength:
	cpx		inflateCodes_primaryCodes
  	bcc 	+
  	ora 	#DISTANCE_TREE
+:
	sta.w	controlSymbolCodeLength,x 
  	inx
	cpx		inflateDynamicBlock_allCodes
	bcc		inflateGameDynamicBlock_storeNext
	dey
;	ldy	#0
;	jmp	inflateCodes

; Decompress a block
inflateGameCodes:
	jsr		game_buildHuffmanTree
inflateGameCodes_loop:
	jsr		game_fetchPrimaryCode
  	bcs +
  	jmp.w 	inflateGameStoreByte
+:
	tax
  	bne +
  	jmp.w 	inflate_game_nextBlock
+:
; Copy sequence from look-behind buffer
;	ldy	#0
	sty		getBits_base
	cmp		#9
	bcc		inflateGameCodes_setSequenceLength
	tya
;	lda	#0
	cpx		#1+28
	bcs		inflateGameCodes_setSequenceLength
	dex
	txa
	lsr		a 
	ror		getBits_base
	inc		getBits_base
	lsr		a 
	rol		getBits_base
	jsr		game_getAMinus1BitsMax8
;	sec
	adc		#0
inflateGameCodes_setSequenceLength:
	sta		inflateCodes_lengthMinus2
	ldx		#DISTANCE_TREE
	jsr		game_fetchCode
;	sec
	sbc		inflateCodes_primaryCodes
	tax
	cmp		#4
	bcc		inflateGameCodes_setOffsetLowByte
	inc		getBits_base
	lsr		a 
	jsr		game_getAMinus1BitsMax8
inflateGameCodes_setOffsetLowByte:
	eor		#$ff
	sta		inflateCodes_sourcePointer
	lda		getBits_base
	cpx		#10
	bcc		inflateGameCodes_setOffsetHighByte
	lda.w 	getNPlus1Bits_mask-10,x
	jsr		game_getBits
	clc
inflateGameCodes_setOffsetHighByte:
	eor		#$ff
;	clc
	;adc	outputPointer+1
	sta		inflateCodes_sourcePointer+1
	
	rep 	#$20
 	lda 	inflateCodes_sourcePointer
 	sta		tcc__r2
 	cmp  	#$FFFF
 	bne		+
 	jsr.w	fillBytes16bit
 	bra		game_after_copy_fill ;inflateGameCodes_loop
 +:
	lda 	inflateCodes_sourcePointer
 	clc
	adc 	outputPointer
	sta 	inflateCodes_sourcePointer
	cmp 	outputPointer
	sep 	#$20
	lda 	outputPointer+2
	bcc 	+
	dea
+:
	sta 	inflateCodes_sourcePointer+2
 
	rep 	#$30
	ldy 	#0
	lda 	inflateCodes_lengthMinus2
	and 	#$ff
	bne		+
	lda 	#$100
+:
	ina
	ina
	tax
	sep 	#$20
	
	jsr.w 	copyBytes16bit
game_after_copy_fill:
	rep		#$30
	tya
	clc
	adc		outputPointer
	sta 	outputPointer	
	ldy 	#0
	sep 	#$30
	bcc 	+
	inc 	outputPointer+2
+:
	sty 	inflateCodes_lengthMinus2
	

;rep #$20
;lda outputPointer
;sta.l pfmountbuf
;jsr show_copied_data
;sep #$30
;-: bra -

;	jmp	inflateCodes_loop
	jmp.w	inflateGameCodes_loop

game_buildTempHuffmanTree:
;	ldy	#0
	tya
inflateGameDynamicBlock_clearCodeLengths:
	sta.w	literalSymbolCodeLength,y
	sta.w	literalSymbolCodeLength+TOTAL_SYMBOLS-256,y
	iny
	bne		inflateGameDynamicBlock_clearCodeLengths
; numberOfPrimaryCodes = 257 + getBits(5)
; numberOfDistanceCodes = 1 + getBits(5)
; numberOfTemporaryCodes = 4 + getBits(4)
	ldx	#3
inflateGameDynamicBlock_getHeader:
	lda.w	inflateDynamicBlock_headerBits-1,x
	jsr		game_getBits
;	sec
	adc.w	inflateDynamicBlock_headerBase-1,x
	sta		inflateDynamicBlock_tempCodes-1,x
	sta.w	inflateDynamicBlock_headerBase+1
	dex
	bne		inflateGameDynamicBlock_getHeader


; Get lengths of temporary codes in the order stored in tempCodeLengthOrder
;	ldx	#0
inflateGameDynamicBlock_getTempCodeLengths:
	lda		#GET_3_BITS
	jsr		game_getBits
	ldy.w	tempCodeLengthOrder,x
	sta.w	literalSymbolCodeLength,y
	ldy		#0
	inx
	cpx		inflateDynamicBlock_tempCodes
	bcc		inflateGameDynamicBlock_getTempCodeLengths

; Build Huffman trees basing on code lengths (in bits)
; stored in the *SymbolCodeLength arrays
game_buildHuffmanTree:
; Clear nBitCode_totalCount, nBitCode_literalCount, nBitCode_controlCount
	tya
;	lda	#0
-:
  	sta.w	nBitCode_clearFrom,y
  	iny
  	bne -
; Count number of codes of each length
;	ldy	#0
game_buildHuffmanTree_countCodeLengths:
	ldx.w	literalSymbolCodeLength,y
	inc.w	nBitCode_literalCount,x
	inc.w	nBitCode_totalCount,x
	cpy		#CONTROL_SYMBOLS
	bcs		game_buildHuffmanTree_noControlSymbol
	ldx.w	controlSymbolCodeLength,y
	inc.w	nBitCode_controlCount,x
	inc.w	nBitCode_totalCount,x
game_buildHuffmanTree_noControlSymbol:
	iny
	bne		game_buildHuffmanTree_countCodeLengths
; Calculate offsets of symbols sorted by code length
;	lda	#0
	ldx		#-3*TREE_SIZE
game_buildHuffmanTree_calculateOffsets:
	sta.w	nBitCode_literalOffset+3*TREE_SIZE-$100,x
  	clc
	adc.w 	nBitCode_literalCount+3*TREE_SIZE-$100,x 
	inx
	bne		game_buildHuffmanTree_calculateOffsets
; Put symbols in their place in the sorted array
;	ldy	#0
game_buildHuffmanTree_assignCode:
	tya
	ldx.w	literalSymbolCodeLength,y
  	ldy.w 	nBitCode_literalOffset,x
  	inc.w 	nBitCode_literalOffset,x
	sta.w	codeToLiteralSymbol,y
	tay
	cpy		#CONTROL_SYMBOLS
	bcs		game_buildHuffmanTree_noControlSymbol2
	ldx.w	controlSymbolCodeLength,y
  	ldy.w 	nBitCode_controlOffset,x
  	inc.w 	nBitCode_controlOffset,x
	sta.w	codeToControlSymbol,y
	tay
game_buildHuffmanTree_noControlSymbol2:
	iny
	bne		game_buildHuffmanTree_assignCode
	rts

; Read Huffman code using the primary tree
game_fetchPrimaryCode:
	ldx	#PRIMARY_TREE
; Read a code from input basing on the tree specified in X,
; return low byte of this code in A,
; return C flag reset for literal code, set for length code
game_fetchCode:
;	ldy	#0
	tya
game_fetchCode_nextBit:
	;jsr		game_getBit

	lsr		getBit_buffer
	bne		++
+:
	pha
 	lda 	[inputPointer]
	;rep		#$20
	inc 	inputPointer
	;sep 	#$20
	bne		+
	rep		#$20
	inc		inputPointer+1
	sep		#$20
+:
	sec
	ror		a 
	sta		getBit_buffer
	pla
++:

	rol		a 
	inx
 	sec
  	sbc.w	nBitCode_totalCount,x 
	bcs		game_fetchCode_nextBit
;	clc
	adc.w	nBitCode_controlCount,x
	bcs		game_fetchCode_control
;	clc
	adc.w	nBitCode_literalOffset,x
	tax
	lda.w	codeToLiteralSymbol,x
	clc
	rts
game_fetchCode_control:
  	clc
	adc.w	nBitCode_controlOffset-1,x 
	tax
	lda.w	codeToControlSymbol,x
	sec
	rts



; Read A minus 1 bits, but no more than 8
game_getAMinus1BitsMax8:
	rol		getBits_base
	tax
	cmp		#9
	bcs		game_getByte
	lda.w	getNPlus1Bits_mask-2,x
game_getBits:
	;jsr		game_getBits_loop
	lsr		getBit_buffer
	beq		+
	ror		a 
	bcc		game_getBits ;_loop
	;bra game_getBits_normalizeLoop ;rts
-:
	lsr		getBits_base
	ror		a 
	bcc		-
	rts
+:	
	pha
	lda 	[inputPointer]
	rep		#$20
	inc 	inputPointer
	sep 	#$20
	bne		+
	inc		inputPointer+2
+:
	sec
	ror		a 
	sta		getBit_buffer
	pla
++:
	ror		a 
	bcc		game_getBits ;_loop
	;rts	
game_getBits_normalizeLoop:
	lsr		getBits_base
	ror		a 
	bcc		game_getBits_normalizeLoop
	rts


	
	

; Read 16 bits (these reads are always byte-aligned within the stream)
game_getWord:
	jsr		game_getByte_fast
	tax
game_getByte_fast:
	lda	#1
	sta	getBit_buffer
	lda 	[inputPointer]
	rep	#$20
	inc 	inputPointer
	sep 	#$20
	bne	+
	inc	inputPointer+2
+:
	sec
	rts
	
	
; Read 8 bits
game_getByte:
	lda		#$80
	.rept 8
	lsr		getBit_buffer
	beq		+
	ror		a 
	.endr
;	bcc		game_getBits_loop
	rts
+:
-:
;	bne		++
	pha
	lda 	[inputPointer]
	rep		#$20
	inc 	inputPointer
	sep 	#$20
	bne		+
	inc		inputPointer+2
+:
	sec
	ror		a 
	sta		getBit_buffer
	pla
++:
	ror		a 
	bcc		game_getByte_loop
	rts
game_getByte_loop:
	lsr		getBit_buffer
	ror		a 
	bcc		game_getByte_loop
	rts

	
game_getBits_loop:
	lsr		getBit_buffer
	beq		+
	ror		a 
	bcc		game_getBits_loop
	rts
+:	
;	bne		++
	pha
	lda 	[inputPointer]
	rep		#$20
	inc 	inputPointer
	sep 	#$20
	bne		+
	inc		inputPointer+2
+:
	sec
	ror		a 
	sta		getBit_buffer
	pla
++:
	ror		a 
	bcc		game_getBits_loop
	rts


; Read one bit, return in the C flag
game_getBit:
	lsr		getBit_buffer	; 5
	beq		+				; 2|3
	rts						; 6
+:
	pha						; 3
 	lda 	[inputPointer]	; 6
	rep		#$20			; 3
	inc 	inputPointer	; 7
	sep 	#$20			; 3
	bne		+				; 2|3
	inc		inputPointer+2	; 5
+:
	sec						; 2
	ror		a 				; 2
	sta		getBit_buffer	; 3
	pla						; 3
game_getBit_return:
	rts						; 6


inflate_remove_smc_header:
	rep		#$30

 	lda		tcc__r0
 	and		#512
 	beq		++
 	sep		#$20
 	lda		#$40 ;50
 	sta		tcc__r0h
 	rep		#$20
 	ldx		#0
 	ldy		tcc__r1
 	lda		tcc__r0
 	and		#$F400
 	sta		tcc__r0
 	beq		+
 	iny
 +:
 -:
 	phb
 	sep		#$20
 	lda		tcc__r0h
 	inc		tcc__r0h
 	pha
 	plb
 	rep		#$20
 remove_smc:
 	lda.w	$0200,x  ;lda.l	$500000,x
 	sta.w	$0000,x  ;sta.l	$500000,x
 	inx
 	inx
 	bne		remove_smc
 	plb
 	dey
 	bne		- ;remove_smc
++:
	rts
	

;getNPlus1Bits_mask:
;	.db	GET_1_BIT,GET_2_BITS,GET_3_BITS,GET_4_BITS,GET_5_BITS,GET_6_BITS,GET_7_BITS

;tempCodeLengthOrder:
;	.db	GET_2_BITS,GET_3_BITS,GET_7_BITS,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15

;inflateDynamicBlock_headerBits:	.db	GET_4_BITS,GET_5_BITS,GET_5_BITS
;inflateDynamicBlock_headerBase:	.db	3,0,0  ; second byte is modified at runtime!


; Data for building trees

;literalSymbolCodeLength:
;	.dsb 256,0
;controlSymbolCodeLength:
;	.dsb CONTROL_SYMBOLS,0

; Huffman trees

;nBitCode_clearFrom:
;nBitCode_totalCount:
;	.dsb TREE_SIZE*2,0
;nBitCode_literalCount:
;	.dsb TREE_SIZE,0
;nBitCode_controlCount:
;	.dsb TREE_SIZE*2,0
;nBitCode_literalOffset:
;	.dsb TREE_SIZE,0
;nBitCode_controlOffset:
;	.dsb TREE_SIZE*2,0

;codeToLiteralSymbol:
;	.dsb 256,0
;codeToControlSymbol:
;	.dsb CONTROL_SYMBOLS,0

inflate_game_ram_code_end:


.ends


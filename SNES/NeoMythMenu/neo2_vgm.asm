; Functions related to VGM playback in the SNES Myth Menu
; /Mic, 2010

.include "hdr.asm"
.include "snes_io.inc"
.include "myth_io.inc"


.bank 3 slot 0
.section "text_neo2_vgm"

neo2_vgm_ram_code_begin:


; Playback driver commands
.define CMD_PLAY			$21
.define CMD_PAUSE			$22
.define	CMD_STOP			$23
.define CMD_SEND_LOOP_MSGS	$24
.define CMD_RESTART_SONG	$25
.define	CMD_SET_MVOL 		$30
.define CMD_TOGGLE_CHN		$40
.define CMD_TOGGLE_ECHO		$50


; Zero page variables
 .equ spcSongNr		$80
 .equ spcMirrorVal	$81
 .equ spcSourceAddr	$82	; Three bytes!
 .equ spcExecAddr	$85	; Two bytes!
 .equ spcScanVal	$87
 .equ spcDataLen	$88
 .equ spcFree00		$89	; Two bytes!
 .equ spcFreeFF		$8b	; Two bytes!
 .equ spcFound00	$8d
 .equ spcFoundFF	$8e
 
 
 .macro sendMusicBlockM ; srcSeg srcAddr destAddr len
 	; Store the source address \1:\2 in musicSourceAddr.
 	sep     #$20
 	lda     #\1
 	sta     spcSourceAddr + 2
 	rep     #$20
 	lda     #\2
 	sta     spcSourceAddr
 
 	; Store the destination address in x.
 	; Store the length in y.
 	rep     #$10
 	ldx     #\3
 	ldy     #\4
 	jsr     copy_block_to_spc
 .endm
 
 
 .macro sendMusicBlockM2 ; srcSeg srcAddr destAddr len
	; Store the source address \1:\2 in musicSourceAddr.
	sep     #$20
	lda     #\1
	sta     spcSourceAddr + 2
	rep     #$20
	lda     #\2
	sta     spcSourceAddr

	; Store the destination address in x.
	; Store the length in y.
	rep     #$10
	ldx     #\3
	ldy     \4
	jsr     copy_block_to_spc
 .endm
 
 
 .macro startSPCExecM ; startAddr
	rep     #$10
	ldx     #\1
	jsr     start_spc_exec
 .endm
 
 
 .macro waitForAudio0M
 	sta	spcMirrorVal
 	-: 	cmp.l REG_APUI00
 	bne 	-
 .endm
 
 
play_vgm_from_sd_card:
	php
	rep		#$30
	pha
	phx
	phy

 	; Make sure echo is off
 	sep		#$20
 	lda		#$7D		; Register $7d (EDL)
 	sta.l	$7F2100		; Just store the value somewhere in RAM temporarily..
 	lda		#$00		
 	sta.l	$7F2101
 	sendMusicBlockM $7F, $2100, $00F2, $0002 	; Send 2 bytes from $7f2100 to $f2,$f3 on the SPC (i.e. the DSP ADDR/DATA registers)
 	sep		#$20
 	lda		#$6C		; Register $6C (FLG)
 	sta.l	$7F2100
 	lda		#$20		; Bit 5 on (~ECEN)	
 	sta.l	$7F2101
 	sendMusicBlockM $7F, $2100, $00F2, $0002
 
  	rep		#$10	        ; xy in 16-bit mode.
  	sep		#$20
  	ldx.w 	#$16FF
  -:
  	lda.l 	vgmplayer,x    	; Copy player code
  	sta.l 	$7F2000,x
  	dex
 	bpl 	-
 	
 	sendMusicBlockM $7F, $2100, $0100, $1700
 	
 	sep		#$20
    LDA    #$20      	; OFF A21
    STA.L  MYTH_GBAC_ZIO
    JSR    SET_NEOCMA  	;
    JSR    SET_NEOCMB  	;
    JSR    SET_NEOCMC  	; ON_NEO CARD A24 & A25 + SA16 & SA17

    LDA     #$01
    STA.L   MYTH_EXTM_ON   ; A25,A24 ON

	LDA    #$04       	; COPY MODE !
    STA.L  MYTH_OPTION_IO

    LDA    #$01       	; PSRAM WE ON !
    STA.L  MYTH_WE_IO
                         
    LDA    #$F8
    STA.L  MYTH_GBAC_ZIO  	; GBA CARD 8M SIZE
    STA.L  MYTH_PRAM_ZIO  	; PSRAM    8M SIZE
         
    LDA    #$00
    STA.L  MYTH_PRAM_BIO

	rep		#$20
	lda.l	highlightedFileSize
	sta		spcDataLen
; DEBUG
sta.l pfmountbuf

	jsr.w	compress_vgm

	rep		#$20
	lda.l	compressedVgmSize
	sta		spcDataLen

	lda.l	compressedVgmSize+2
	bne		_play_vgm_skip_upload
	
	; Send the entire VGM to SPC RAM at $1800
	sendMusicBlockM2 $52, $0000, $1800, spcDataLen

	; The playback driver entrypoint is at $0300 in SPC RAM
	startSPCExecM $0300

	; The playback driver will write $CD to Port0 when it's entering its command-reading loop - so
	; wait for that.
	sep		#$20
	lda		#$CD
-:
	cmp.l 	REG_APUI00
 	bne 	-

;jsr.w show_copied_data

	; Tell the driver to start playback
	sep		#$20
	lda		#CMD_PLAY
	sta.l	REG_APUI01
	lda		#$CE
	sta.l	REG_APUI00
	waitForAudio0M
	lda		spcMirrorVal
	sta.l	mSpcMirrorVal
	
_play_vgm_skip_upload:	
	sep		#$20
    LDA     #$00       ;
    STA.L   MYTH_WE_IO     ; PSRAM WRITE OFF

    LDA     #MAP_MENU_FLASH_TO_ROM	; SET GBA CARD RUN
    STA.L   MYTH_OPTION_IO

    LDA     #$20       	; OFF A21
    STA.L   MYTH_GBAC_ZIO
    JSR     SET_NEOCMD	; SET MENU

    LDA     #$00
    STA.L   MYTH_GBAC_LIO
    STA.L   MYTH_GBAC_HIO
    STA.L   MYTH_GBAC_ZIO
 	
	rep		#$30

	lda.l	compressedVgmSize
	sta		tcc__r0
	lda.l	compressedVgmSize+2
	sta		tcc__r1
	
	ply
	plx
	pla
	plp
	rtl
	

stop_vgm:
	php

	sep		#$20
	lda		#CMD_STOP
	sta.l	REG_APUI01
	lda		spcMirrorVal
	ina
	sta.l	REG_APUI00
	
	plp
	rtl


vgm_echo:
	php

	sep		#$20
	lda		#CMD_TOGGLE_ECHO
	sta.l	REG_APUI01
	lda.l	mSpcMirrorVal
	ina
	sta.l	REG_APUI00
	waitForAudio0M
	lda		spcMirrorVal
	sta.l	mSpcMirrorVal

	plp
	rtl
	

compress_vgm:
	php
	rep		#$30

	; Copy the header
	ldx		#$3E
-:
	lda.l	$500000,x
	sta.l	$520000,x
	dex
	dex
	bpl		-

	lda.l	$50001C
	clc
	adc		#$1C
	sta		tcc__r10	; loop offset
	
	lda		#0
	sta.l	compressedVgmSize+2
	sta.w	__tccs__FUNC_compress_vgm_flags
	sta.w	__tccs__FUNC_compress_vgm_numFlags
	sta.w	__tccs__FUNC_compress_vgm_chunkStart
	sta		tcc__r2		; source pointer
	sta		tcc__r3		; dest pointer
	lda		#$50
	sta		tcc__r2h	
	lda		#$52
	sta		tcc__r3h

	
	lda.l	highlightedFileSize
	sec
	sbc		#$40
	sta		tcc__r0
	lda.l	highlightedFileSize+2
	sbc		#0
	sta		tcc__r0h
	ldy		#$40
	sty.w	__tccs__FUNC_compress_vgm_destOffs
	lda		#$40
	sta.l	compressedVgmSize
	ldx		#1
_compress_vgm_loop:
	cpy		tcc__r10
	bne		+
	; Recalculate loop offset
	jsr.w	_compress_vgm_flush_buffer
	txa
	dea
	clc
	adc.w	__tccs__FUNC_compress_vgm_destOffs
	sec
	sbc		#$1C
	sta.l	$52001C
+:

	; Any bytes left to compress?
	lda		tcc__r0h
	bpl		+
	jmp		_compress_vgm_end_loop
+:
	ora		tcc__r0
	bne		+
	jmp		_compress_vgm_end_loop
+:

	lda		tcc__r0
	bne		+
	dec		tcc__r0h
+:
	dec		tcc__r0
	lda		[tcc__r2],y
	iny
	bne		+
	inc		tcc__r2h
+:
	sep		#$20
	
	cmp		#$50
	bne		+
	sec
	ror.w	__tccs__FUNC_compress_vgm_flags
	;jsr.w	_compress_vgm_read_byte
	xba	
	sta.l	compressVgmBuffer,x
	inx
	rep		#$20
	lda		tcc__r0
	bne		++
	dec		tcc__r0h
++:
	dec		tcc__r0
	iny
	bne		++
	inc		tcc__r2h
++:	
	jmp.w	_compress_vgm_next
+:

	sep		#$20
	sta.l	compressVgmBuffer,x
	inx
	clc
	ror.w	__tccs__FUNC_compress_vgm_flags
	
	cmp		#$30
	beq		+
	cmp		#$4F
	bne		++
+:
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jmp.w	_compress_vgm_next
++:

	cmp		#$51
	beq		+
	cmp		#$52
	beq		+
	cmp		#$53
	beq		+
	cmp		#$54
	beq		+
	cmp		#$61
	bne		++
+:
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jmp.w	_compress_vgm_next
++:

	cmp		#$E0
	bne		+
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jmp.w	_compress_vgm_next
+:

	; Data block
	cmp		#$67
	bne		+
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx

	; Size
	jsr.w	_compress_vgm_read_byte
	sta		tcc__r1
	sta.l	compressVgmBuffer,x
	inx
	jsr.w	_compress_vgm_read_byte
	sta		tcc__r1+1
	sta.l	compressVgmBuffer,x
	inx

	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
--:
	rep		#$20
	lda		tcc__r1
	beq		++
	sep		#$20
	jsr.w	_compress_vgm_read_byte
	sta.l	compressVgmBuffer,x
	inx
	rep		#$20
	dec		tcc__r1
	bne		--
++:
	bra		_compress_vgm_next
+:
	sep		#$20

	cmp		#$66
	bne		_compress_vgm_next
	inc.w	__tccs__FUNC_compress_vgm_numFlags
	bra		_compress_vgm_end_loop

_compress_vgm_next:
	rep		#$20
	inc.w	__tccs__FUNC_compress_vgm_numFlags
	lda.w	__tccs__FUNC_compress_vgm_numFlags
	cmp		#8
	bne		+
	jsr.w	_compress_vgm_write_buffer
+:	
	jmp.w	_compress_vgm_loop

_compress_vgm_end_loop:
	rep		#$20
	cpx		#2
	bcc		+
	jsr.w	_compress_vgm_flush_buffer
+:
	cpx		#2
	bcc		+
	lda		#$6666
	ldy.w	__tccs__FUNC_compress_vgm_destOffs
	sta		[tcc__r3],y
	clc
	lda		#2
	adc.l	compressedVgmSize
	sta.l	compressedVgmSize
	lda		#0
	adc.l	compressedVgmSize+2
	sta.l	compressedVgmSize+2	
+:

	plp
	rts
	

_compress_vgm_read_byte:
	; Load an uncompressed byte
	rep		#$20
	lda		tcc__r0
	bne		+
	dec		tcc__r0h
+:
	dec		tcc__r0
	sep		#$20
	lda		[tcc__r2],y
	iny
	bne		+
	inc		tcc__r2h
+:
	rts


	
_compress_vgm_write_buffer:
	php
	sep		#$20
	phy
	phx
	; Copy the flags byte to the start of the compressed chunk
	ldx.w	__tccs__FUNC_compress_vgm_chunkStart
	lda.w	__tccs__FUNC_compress_vgm_flags
	sta.l	compressVgmBuffer,x	
	rep		#$20
	plx
	phx
;DEBUG
;stx.w pfmountbuf
	txa
	lsr		a
	sta		tcc__r1h
	ldx		#0

; DEBUG
sty.w pfmountbuf+2

	ldy.w	__tccs__FUNC_compress_vgm_destOffs
	asl		a
	clc
	adc.l	compressedVgmSize
	sta.l	compressedVgmSize
	lda		#0
	adc.l	compressedVgmSize+2
	sta.l	compressedVgmSize+2
	lda		tcc__r1h
	beq		+
-:
;	lda		tcc__r1h
;	beq		+
	lda.l	compressVgmBuffer,x
	inx
	inx
	sta		[tcc__r3],y
	iny
	iny
	bne		++
	inc		tcc__r3h
++:
	dec		tcc__r1h
	bne		-
+:
	sty.w	__tccs__FUNC_compress_vgm_destOffs
	plx
	txa
	and		#1
	; Copy odd byte at end to the start of the buffer
	beq		+
	pha
	sep		#$20
	dex
	lda.l	compressVgmBuffer,x
	sta.l	compressVgmBuffer
	rep		#$20
	pla
+:
	sta.w	__tccs__FUNC_compress_vgm_chunkStart
	ina
	tax
	lda		#0
	sta.w	__tccs__FUNC_compress_vgm_flags
	sta.w	__tccs__FUNC_compress_vgm_numFlags
	ply
	plp
	rts


_compress_vgm_flush_buffer:
	php
	sep		#$20
	; Is there anything in the buffer?	
	cpx		#2
	bcs		+
	plp
	rts
+:
	; Pad the buffer if necessary
-:
	lda.w	__tccs__FUNC_compress_vgm_numFlags
	cmp		#8
	beq		+
	clc
	ror.w	__tccs__FUNC_compress_vgm_flags
	lda		#$4E
	sta.l	compressVgmBuffer,x	
	inx
	inc.w	__tccs__FUNC_compress_vgm_numFlags
	bra		-
+:
	jsr.w	_compress_vgm_write_buffer
	plp
	rts


		
neo2_vgm_ram_code_end:

.ends


.ramsection ".bss" bank $7e slot 2
__tccs__FUNC_compress_vgm_numFlags dsb 2
__tccs__FUNC_compress_vgm_flags dsb 2
__tccs__FUNC_compress_vgm_chunkStart dsb 2
__tccs__FUNC_compress_vgm_destOffs dsb 2
.ends

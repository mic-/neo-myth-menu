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
 	
 	sendMusicBlockM $7F, $2100, $0100, $1600
 	
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

	; Send the entire VGM to SPC RAM at $1700
	sendMusicBlockM2 $50, $0000, $1700, spcDataLen

	; The playback driver entrypoint is at $0300 in SPC RAM
	startSPCExecM $0300

	; The playback driver will write $CD to Port0 when it's entering its command-reading loop - so
	; wait for that.
	sep		#$20
	lda		#$CD
-:
	cmp.l 	REG_APUI00
 	bne 	-

jsr.w show_copied_data

	; Tell the driver to start playback
	sep		#$20
	lda		#CMD_PLAY
	sta.l	REG_APUI01
	lda		#$CE
	sta.l	REG_APUI00
	waitForAudio0M
	
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
	
	
neo2_vgm_ram_code_end:

.ends

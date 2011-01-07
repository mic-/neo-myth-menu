; Functions related to SPC playback in the SNES Myth Menu
; /Mic, 2010

.include "hdr.asm"
.include "snes_io.inc"
.include "myth_io.inc"


.bank 3 slot 0
.section "text_neo2_spc"

neo2_spc_ram_code_begin:
        
        
play_spc_from_gba_card:
play_spc_from_sd_card:
	php
	phb
	rep		#$30
	phx

	jsl		clear_screen

	pea		73
	jsl		print_meta_string			; Print instructions
	pla
	pea		0
	jsl		print_meta_string			; Print instructions
	pla	
	jsl		print_hw_card_rev

	jsl		update_screen
	
	sep		#$20
	
	lda		#$7e
	pha
	plb
	
	lda.l	romAddressPins
	sta		tcc__r2h
	
    LDA    #$20      	; OFF A21
    STA.L  MYTH_GBAC_ZIO
    JSR    SET_NEOCMA  	;
    JSR    SET_NEOCMB  	;
    JSR    SET_NEOCMC  	; ON_NEO CARD A24 & A25 + SA16 & SA17

    LDA.L   romAddressPins+1
    STA.L   MYTH_GBAC_HIO	; SET AH25,AH25
         
    LDA     #$01
    STA.L   MYTH_EXTM_ON   ; A25,A24 ON

	LDA    #$04       	; COPY MODE !
    STA.L  MYTH_OPTION_IO

    LDA    #$01       	; PSRAM WE ON !
    STA.L  MYTH_WE_IO
                         
    LDA    #$F8
    STA.L  MYTH_GBAC_ZIO  	; GBA CARD 8M SIZE
    STA.L  MYTH_PRAM_ZIO  	; PSRAM    8M SIZE

    LDA.L  romAddressPins
    STA.L  MYTH_GBAC_LIO
         
    LDA    #$00
    STA.L  MYTH_PRAM_BIO

	jsr	load_spc
	jsr	neo2_spc_puts_tag
	
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

    ; copy .data section to RAM (same as in crt0).
    ; This data has been overwritten by the SPC loader, so we need to copy it again.
    ldx #0
-   	lda.l __startsection.data,x
    	sta.l __startramsectionram.data,x
    	inx
    	inx
    	cpx #(__endsection.data-__startsection.data)
    	bcc -
    
	rep	#$20
	
-:
	bra	-
	
    plx
    plb
    plp
    rtl
        
     

neo2_spc_puts_tag:
	php
 	jsr.w	_wait_nmi
	rep		#$30
	lda		#$2162 ;3
	sta.l	REG_VRAM_ADDR_L
	ldx		#SPC_TAG_TITLE_ADDR
	jsr		++
	lda		#$2182
	sta.l	REG_VRAM_ADDR_L
	ldx		#SPC_TAG_GAME_ADDR
	jsr		++
	lda		#$21A2
	sta.l	REG_VRAM_ADDR_L
	ldx		#SPC_TAG_ARTIST_ADDR
	jsr		++
	plp
	rts
++:	
	sep		#$20
	ldy		#0
-:
	lda.l	SPC_TAG_BASE,x
	beq		+
	sta.l	REG_VRAM_DATAW1
	lda		#8
	sta.l	REG_VRAM_DATAW2
	inx
	iny
	cpy		#28
	bne		-
+:
	rep		#$20
	rts
	
	
	
 ; The initialization code will be written to this address in SPC RAM if no better location is found
 .equ SPC_DEFAULT_INIT_ADDRESS	$ff70
 
 .equ SPC_REG_ADDR		$510225 ;$4100A5 ;00
 .equ SPC_DSP_REG_ADDR	$510000 ;	$410000 ;8
 
 .equ SPC_TAG_BASE $510000
 .equ SPC_TAG_TITLE_ADDR	$51022E ;$4100AE
 .equ SPC_TAG_GAME_ADDR		$51024E ;$4100CE
 .equ SPC_TAG_ARTIST_ADDR	$5102B1 ;$410131
  
 ; The length of our init routine in bytes
 .equ SPC_INIT_CODE_LENGTH 	$2b 
 
 ; The length of our fastloader routine in bytes
 .equ SPC_FASL_LENGTH	   	$32
 
 
 ; Zero page variables
 .equ spcSongNr		$80
 .equ spcMirrorVal	$81
 .equ spcSourceAddr	$82	; Three bytes!
 .equ spcExecAddr	$85	; Two bytes!
 .equ spcScanVal	$87
 .equ spcScanLen	$88
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
 
 
 
 load_spc:
 	stz		spcSongNr
 	
	; Make sure echo is off
 	sep		#$20
 	lda		#$7d		; Register $7d (EDL)
 	sta.l	$7f0100
 	lda		#$00		
 	sta.l	$7f0101
 	sendMusicBlockM $7f, $0100, $00f2, $0002 	
 	sep		#$20
 	lda		#$6c		; Register $6c (FLG)
 	sta.l	$7f0100
 	lda		#$20		; Bit 5 on (~ECEN)	
 	sta.l	$7f0101
 	sendMusicBlockM $7f, $0100, $00f2, $0002

	lda.l	sourceMedium
	bne		+
		; GBAC
		rep #$30
		ldx	#254	; Move page 0
		-:
			lda.l	$400000,x
			sta.l	$500000,x
			dex
			dex
			bpl		-
		ldx	#126	; Move DSP regs
		-:
			lda.l	$410000,x
			sta.l	$510000,x
			dex
			dex
			bpl		-
		ldx	#126	; Move header
		-:
			lda.l	$410080,x
			sta.l	$510200,x
			dex
			dex
			bpl		-
		bra ++
	+:
		; SD
		rep #$30
		ldx	#254	; Move header
		-:
			lda.l	$500000,x
			sta.l	$510200,x
			dex
			dex
			bpl		-
		ldx	#126	; Move DSP regs
		-:
			lda.l	$510100,x
			sta.l	$510000,x
			dex
			dex
			bpl		-
		ldx	#254	; Move page 0
		-:
			lda.l	$500100,x
			sta.l	$500000,x
			dex
			dex
			bpl		-			
	++:
	
  	; Initialize DSP registers, except FLG, EDL, KON and KOF
 	jsr     init_dsp

	; Copy the SPC RAM dump to SNES RAM
	lda.l	sourceMedium
	bne		+
 		jsr     copy_spc_memory_to_ram
 		bra		++
 	+:
 		jsr     copy_spc_psram_to_ram
 	++:
 	
 	; Now we try to find a good place to inject our init code on the SPC side
  	;========================================================================
  	rep		#$10
  	ldx		#SPC_DEFAULT_INIT_ADDRESS
  	stx		spcExecAddr
  	
  	; Check if we've got a non-zero EDL. In that case we use ESA*$100 + $200 as the base
  	; address for our init code.
  	sep		#$20
  	lda.l	SPC_DSP_REG_ADDR+$7d	; EDL
  	beq		+			
  	lda.l	SPC_DSP_REG_ADDR+$6d	; ESA
 	clc
 	adc		#2
  	xba
  	lda		#0
  	rep		#$20
  	tax				; X = ESA*$100 + $200
  	stx		spcExecAddr
  	jmp		addr_search_done
  	+:
  	
  	; Search for a free chunk of RAM somewhere in the range $0100..$ff9f
  	; A free chunk is defined as a string of adequate length that contains
  	; only the value $00 or $ff (i.e. either 00000000.. or FFFFFFFF..).
  	; Strings containing $ff are preferred, so if such a string is found
  	; the search will terminate and the address of that string will be used.
  	; If a string containing $00 is found first, the search will continue
  	; until a string containing $ff is found, or until we've reached then
  	; end address ($ff9f).
  	sep		#$20
  	stz		spcFound00
  	stz		spcFoundFF
  	ldx		#0
  	stx		spcFree00
  	stx		spcFreeFF
  	ldx		#$100
  	search_free_ram:
 		cpx		#$ff9f
 		bcs		pick_best_address	; Found no free RAM. Give up and use the default address
 		lda		#0
 		sta		spcScanLen	
 		search_free_ram_inner:
 			lda.l	$7f0000,x	; Read one byte from RAM
 			xba			; Save it in the high byte of A
 			lda		spcScanLen	; Is this a new string, or one we've already begun matching?
 			beq		search_free_ram_new_string
 			xba
 			cmp		spcScanVal	; Compare with the current value we're scanning for
 			bne		scan_next_row	; No match?
 			inc		spcScanLen
 			lda		spcScanLen
 			cmp		#($0C+SPC_INIT_CODE_LENGTH)
 			beq		found_free_ram
 			inx
 			bra		search_free_ram_inner
 			scan_next_row:		; Move to the next row, i.e. (16 - (current_offset % 16)) bytes ahead. 
 			rep		#$20
 			txa
 			clc
 			adc		#$0010
 			and		#$FFF0
 			tax
 			sep		#$20
 			jmp		search_free_ram	; Gotta keep searchin' searchin'..
 			search_free_ram_new_string:
 			xba			; Restore the value we read from RAM
 			cmp		#$00
 			beq		search_free_ram_new_string00
 			cmp		#$ff
 			beq		search_free_ram_new_stringFF
 			bra		scan_next_row	; Neither $00 or $ff. Try the next row
 			search_free_ram_new_string00:
 			lda		spcFound00	
 			bne		scan_next_row	; We've already found a 00-string
 			stz		spcScanVal
 			bra		+
 			search_free_ram_new_stringFF:
 			lda		spcFoundFF
 			bne		scan_next_row	; We've already found an FF-string
 			lda		#$ff
 			sta		spcScanVal
 			+:
 			inc		spcScanLen
 			inx
 			jmp		search_free_ram_inner	
 	found_free_ram:
 	rep		#$20
 	txa
 	sec
 	sbc		#($0B+SPC_INIT_CODE_LENGTH)
 	tax
 	sep		#$20
 	lda		spcScanVal
 	bne		found_stringFF	
 	stx		spcFree00			; This was a 00-string that we found
 	lda		#1
 	sta		spcFound00
 	jmp		search_free_ram			; Find a place to hide, searchin' searchin'..
 	found_stringFF:
 	stx		spcFreeFF			; This was an FF-string that we found
 	lda		#1
 	sta		spcFoundFF
 	
 	pick_best_address:
 	lda		spcFoundFF			; Prefer the FF-string if we've found one
 	beq		+
 	ldx		spcFreeFF
 	stx		spcExecAddr
 	bra		addr_search_done
 	+:
 	lda		spcFound00
 	beq		addr_search_done
 	ldx		spcFree00
 	stx		spcExecAddr
 	
   	addr_search_done:
   	;========================================================================
   	
 	; Copy fastloader to SNES RAM
 	rep	#$10
 	sep	#$20
 	ldx	#(SPC_FASL_LENGTH-2)
 	-:
 		lda.l	spc700_fasl_code+$7D0000,x
 		sta.l	$7f0000,x
 		dex
 		bpl	-
 	; Modify some values/addresses in the fastloader	
 	lda.l   SPC_DSP_REG_ADDR+$5c	; DSP KOF
 	sta.l	$7f000b
 	lda.l   SPC_DSP_REG_ADDR+$4c	; DSP KON
 	sta.l	$7f002d
 	lda	spcExecAddr
 	sta.l	$7f0030
 	lda	spcExecAddr+1
 	sta.l	$7f0031
 	
 	; Send the fastloader to SPC RAM
 	sendMusicBlockM $7f, $0000, $0002, SPC_FASL_LENGTH
 	
 	; Start executing the fastloader
 	startSPCExecM $0002
 	
   	; Build code to initialize registers.
 	jsr     make_spc_init_code
 
 	; Upload the SPC data to $00f8..$ffff
 	sep	#$20
 	ldx	#$00f8
   	send_by_fasl:
 		lda.l	$7f0000,x
 		sta.l	REG_APUI01
 		lda.l	$7f0001,x
 		sta.l	REG_APUI02
 		txa
 		sta.l	REG_APUI00
 		waitForAudio0M
 		inx
 		inx
 		bne	send_by_fasl
 	
 	; > The SPC should be executing our initialization code now <
 
  	ldx	#0
 	copy_spc_page0:
 		lda.l	$500000,x	; Read from PSRAM
 		sta.l	REG_APUI01
 		txa
 		sta.l	REG_APUI00
 		waitForAudio0M
 		inx
 		cpx	#$f0
 		bne	copy_spc_page0
 	; Send the init value for SPC registers $f0 and $f1
 	lda	#$0a
 	sta.l	REG_APUI01
 	lda	#$f0
 	sta.l	REG_APUI00
 	waitForAudio0M
 	lda.l	$5000f1
 	and	#7		; Mask out everything except the timer enable bits
 	sta.l	REG_APUI01
 	lda	#$f1
 	sta.l	REG_APUI00
 	waitForAudio0M
 	
 	; Write the init values for the SPC I/O ports
 	lda.l	$7f00f7
 	sta.l	REG_APUI03
 	lda.l	$7f00f6
 	sta.l	REG_APUI02
 	lda.l	$7f00f4
 	sta.l	REG_APUI00
 	lda.l	$7f00f5
 	sta.l	REG_APUI01
 	
 	rts 	
 
 
 copy_spc_memory_to_ram:
  	; Copy music data from ROM to RAM, from the end backwards.
  	rep	#$10        ; xy in 16-bit mode.
  	sep	#$20
  	ldx.w 	#$7fff          ; Set counter to 32k-1.
  
  -:  	lda.l 	$400000,x     	; Copy byte from first music bank.
  	sta.l 	$7f0000,x
  	lda.l	$408000,x
  	sta.l	$7f8000,x
  	dex
  	bpl 	-
 	rts


 copy_spc_psram_to_ram:
  	; Copy music data from PSRAM to RAM, from the end backwards.
  	rep	#$10        ; xy in 16-bit mode.
  	;sep	#$20
  	rep #$20
  	ldx.w 	#$0          
  
  -: lda.l 	$500100,x     	
  	 sta.l 	$7f0000,x
  	 inx
  	 inx
  	 cpx	#$ff00
  	 bne 	-

  	ldx.w 	#$0          
  -: lda.l 	$510000,x     	
  	 sta.l 	$7fff00,x
  	 inx
  	 inx
  	 cpx	#$100
  	 bne 	-
  	 
    rts
 	
 
  ; Loads the DSP init values in reverse order
 init_dsp:
 	rep     #$10	; xy in 16-bit mode
  	sep	#$20		; 8-bit accum
 	
 	ldx	#$007F		; Reset DSP address counter.
 -:
 	sep     #$20
 	txa                   	; Write DSP address register byte.
 	sta     $7f0100            
 	lda.l   SPC_DSP_REG_ADDR,x     	
 	sta     $7f0101         ; Write DSP data register byte.    
 	phx                   	; Save x on the stack.
 
 	cpx	#$006c
 	beq	skip_dsp_write
 	cpx	#$007d
 	beq	skip_dsp_write
 	cpx	#$004c
 	beq	skip_dsp_write
 	cpx	#$005c
 	bne	+
 	lda	#$ff
 	sta	$7f0101
 	+:
 	
 	; Send the address and data bytes to the DSP memory-mapped registers.
 	sendMusicBlockM $7f, $0100, $00f2, $0002
 
 	skip_dsp_write:
 	
 	nop
 	nop
 	nop
 	nop
 	nop
 	nop
 	nop
 	nop
 	nop
 	nop
 	nop
 	nop
 	
 	rep     #$10          
 	plx
 
 	; Loop if we haven't done 128 registers yet.
 	dex
 	cpx     #$FFFF
 	bne     -
 	rts
 	
 
make_spc_init_code:
      ; Constructs SPC700 code to restore the remaining SPC state and start
      ; execution.
  
   	rep     #$20
  	lda	#0
  	ldx	#0			; Make sure A and X are both clear
  	rep	#$10
 
   	ldy	spcExecAddr	
   	
 	sep     #$20
 
 	ldx	#0
 	
 	phb				; Save DBR
 	lda	#$7f
 	pha
 	plb				; Set DBR=$7f (RAM)
 	
 	phx				; Save X
 	ldx	#0
 	-:
 	lda.l	spc700_init_code+$7D0000,x
 	sta.w	$0000,y			; Store in SNES RAM
 	iny
 	inx
 	cpx	#SPC_INIT_CODE_LENGTH
 	bne	-
 	plx				; Restore X
 
 	ldy	spcExecAddr
 	
 	; Patch the init routine with the correct register values etc. for this song
 	lda.l   SPC_REG_ADDR+6,x	; SP
 	sta.w	$0001,y
 	lda.l   SPC_REG_ADDR+5,x	; PSW
 	sta.w	$0004,y
 	lda.l   SPC_DSP_REG_ADDR+$7d	; EDL
 	sta.w	$0019,y
 	lda.l   SPC_REG_ADDR+2,x 	; A
 	sta.w	$001c,y
 	lda.l   SPC_REG_ADDR+3,x 	; X
 	sta.w	$001e,y
 	lda.l   SPC_REG_ADDR+4,x 	; Y
 	sta.w	$0020,y
 	lda.l   SPC_DSP_REG_ADDR+$6c	; FLG
 	sta.w   $0025,y
 	rep     #$20
 	lda.l   SPC_REG_ADDR,x  	; PC
 	sta.w	$0029,y
 	sep	#$20
 	
 	plb				; Restore DBR
 	rts
 
 
 ; spcSourceAddr - source address
 ; x - dest address
 ; y - count
 copy_block_to_spc:
  	rep	#$10
  	
 	; Wait until audio0 is $aa
 	sep     #$20
 	lda     #$aa
 	waitForAudio0M
 
 	rep	#$20
 	txa
 	sta.l   REG_APUI02		; Write it to APU Port2 as well
 	sep	#$20
 	
 	; Transfer count to x.
 	phy
 	plx
 
  	; Send $01cc to AUDIO0 and wait for echo.
 	lda	#$01
 	sta.l     REG_APUI01
 	lda	#$cc
 	sta.l     REG_APUI00
 	waitForAudio0M
 
 	sep     #$20
 
 	; Zero counter.
 	ldy     #$0000
  CopyBlockToSPC_loop:
 	lda     [spcSourceAddr],y
 
 	sta.l	REG_APUI01
 	
 	; Counter -> A
 	tya
 
 	sta.l	REG_APUI00
 
 	; Wait for counter to echo back.
 	waitForAudio0M
 
 	; Update counter and number of bytes left to send.
 	iny
 	dex
 	bne     CopyBlockToSPC_loop
 
  	;sep	#$20
  	
 	; Send the start of IPL ROM send routine as starting address.
 	rep	#$20
 	lda     #$ffc9
 	sta.l   REG_APUI02
 	sep	#$20
 	
 	; Tell the SPC we're done transfering for now
 	lda     #0
 	sta.l	REG_APUI01
 	lda	spcMirrorVal	; This value is filled out by waitForAudio0M
 	clc
 	adc	#$02		
 	sta.l	REG_APUI00
 
 	; Wait for counter to echo back.
 	waitForAudio0M
 
 	rep     #$20
 	
 	rts
  
; Starting address is in x.
start_spc_exec:
 	; Wait until audio0 is $aa
 	sep     #$20
 	lda     #$aa
 	waitForAudio0M
 
 	; Send the destination address to AUDIO2.
 	rep	#$20
 	txa
 	sta.l   REG_APUI02
 	sep	#$20
 	
 	; Send $00cc to AUDIO0 and wait for echo.
 	lda     #$00
 	sta.l   REG_APUI01
 	lda     #$cc
 	sta.l   REG_APUI00
 	waitForAudio0M
 
 	rts
 	
 spc700_init_code:
  .db $cd, $00		; mov 	x,#$xx		(the value is filled in later)
  .db $bd		; mov	sp,x
  .db $cd, $00		; mov 	x,#$xx		(the value is filled in later)
  .db $4d		; push	x
  .db $cd, $00		; mov   x,#0
  .db $3e, $f4  		; -: cmp   x,$f4
  .db $d0, $fc 		;    bne   -
  .db $e4, $f5  		;    mov   a,$f5
  .db $d8, $f4  		;    mov   $f4,x
  .db $af 		;    mov   (x)+,a
  .db $c8, $f2		;    cmp   x,#$f2
  .db $d0, $f3		;    bne   -
  .db $8f, $7d, $f2	; mov	$f2,#$7d
  .db $8f, $00, $f3	; mov	$f3,#$xx	(the value is filled in later)
  .db $e8, $00		; mov	a,#$xx		(the value is filled in later)
  .db $cd, $00		; mov	x,#$xx		(the value is filled in later)
  .db $8d, $00		; mov	y,#$xx		(the value is filled in later)
  .db $8f, $6c, $f2	; mov	$f2,#$6c
  .db $8f, $00, $f3	; mov	$f3,#$xx	(the value is filled in later)
  .db $8e		; pop	psw
  .db $5f, $00, $00	; jmp	$xxxx		(the address is filled in later)
  ; 43 bytes
  ; minimum 22 cycles per byte transfered
 
 
 ; Code for transferring data to SPC RAM at $00f8..$ffff
 spc700_fasl_code:
  .db $cd, $00		; 0002 mov x,#0
  .db $8d, $f8		; 0004 mov y,#$f8
  .db $8f, $00, $f1	; 0006 mov $f1, #0
  .db $8f, $5c, $f2	; 0009 mov $f2, #$5c 
  .db $8f, $00, $f3	; 000c mov $f2, #$xx  	(the value is filled in later)
  .db $7e, $f4     	; 000f -: cmp   y,$f4  
  .db $d0, $fc     	; 0011    bne   -
  .db $e4, $f5     	; 0013    mov   a,$f5
  .db $d6, $00, $00	; 0015    mov   $0000+y,a
  .db $cb, $f4     	; 0018    mov   $f4,y
  .db $e4, $f6     	; 001a    mov   a,$f6
  .db $d6, $01, $00     	; 001c    mov   $0001+y,a
  .db $fc        	; 001f    inc   y
  .db $fc        	; 0020    inc   y
  .db $d0, $ec     	; 0021    bne   - 
  .db $ac, $17, $00     	; 0023    inc   $0017
  .db $ac, $1e, $00     	; 0026    inc   $001e
  .db $d0, $e4		; 0029    bne   -
  .db $8f, $4c, $f2	; 002b mov   $f2, #$4c 
  .db $8f, $00, $f3	; 002e mov   $f2, #$xx 	(the value is filled in later)
  .db $5f, $00, $00	; 0031 jmp   $xxxx   	(the address is filled in later)
  ; 50 bytes
 ; minimum 17.55 cycles per byte transfered
 
 
neo2_spc_ram_code_end:

.ends

; Various support code in assembly for the SNES Myth Menu
; Neo TeAm, 2010
; /Mic, 2010

.include "hdr.asm"
.include "snes_io.inc"
.include "myth_io.inc"


.section ".text_neo2" superfree


copy_ram_code:
	php
	rep	#$10
	sep	#$20
	; Note: it's very important that the correct labels are used here. Look in NEOSNES.SYM if you suspect that
	; the order of the sections involved has been changed by the linker.
	ldy	#(neo2_spc_ram_code_end - ppu_ram_code_begin)
	ldx	#0
-:	
	lda.l	ppu_ram_code_begin,x
	sta.l	$7e8000,x
	inx
	dey
	bne	-
	plp
	rtl
	
	
.ends


.bank 3 slot 0
.org 0
.section "text_neo2_two"

ram_code_begin:

;===============================================================================
; Copy 1 Mbit of data from the GBA Card to PSRAM
;
; DBR is set to the source bank ($4x) so that word addressing can be used
; when reading data from the GBA Card, thus saving 1 cycle per read.
;
; Then 512 kb (64 kB) are copied at 16 bytes per iteration. The destination bank
; is then incremented by overwriting parts of the code, and then another 512 kb
; are copied.
copy_1mbit_from_gbac_to_psram:
	PHP
    	SEP	#$20
    	REP	#$10
    	PHB        	   ; Save DBR
    	LDA   	#$40	   ; +$07	
	LDY	#2	   ; Copy 2 * 512 kbit	
MOV_512K:	
    	PHA
    	PLB        	   ; Set DBR

    	REP	#$30       ; A,16 & X,Y 16 BIT
    	LDX     #$1FFE
-:
    	LDA.W   $0000,X    ; READ GBA CARD
    	STA.L   $500000,X  ; WRITE PSRAM ;+$18
    	LDA.W   $2000,X    ; READ GBA CARD
    	STA.L   $502000,X  ; WRITE PSRAM 
    	LDA.W   $4000,X    ; ...
    	STA.L   $504000,X 
    	LDA.W   $6000,X   
    	STA.L   $506000,X 
    	LDA.W   $8000,X   
    	STA.L   $508000,X 
    	LDA.W   $A000,X    
    	STA.L   $50A000,X 
    	LDA.W   $C000,X   
    	STA.L   $50C000,X  
    	LDA.W   $E000,X   
    	STA.L   $50E000,X  
    	DEX
    	DEX
    	BPL     -

	DEY
	BEQ	+
;+$51	
	SEP	#$20
	LDA	#$51		;+$54
	STA.L	$7d0000+copy_1mbit_from_gbac_to_psram+$18	; Increase the MSB of all the addresses above on the form $5xxxxx
	STA.L	$7d0000+copy_1mbit_from_gbac_to_psram+$1F	; ...
	STA.L	$7d0000+copy_1mbit_from_gbac_to_psram+$26
	STA.L	$7d0000+copy_1mbit_from_gbac_to_psram+$2D
	STA.L	$7d0000+copy_1mbit_from_gbac_to_psram+$34
	STA.L	$7d0000+copy_1mbit_from_gbac_to_psram+$3B
	STA.L	$7d0000+copy_1mbit_from_gbac_to_psram+$42
	STA.L	$7d0000+copy_1mbit_from_gbac_to_psram+$49
	
	; Increase DBR
	PHB
	PLA
	INA
	BRA	MOV_512K
+:	
    	PLB        	   ; Restore DBR
    	PLP
    	RTS
    	
    	
; Used for running the secondary cart (plugged in at the back of the Myth)
run_3800:
	sep	#$30
        LDA     #$01                 ;
        STA.L   $C00E                ; HARD REST IO

        LDA     #$01                 ;
        STA.L   $C012                ; CPLD RAM ON
        STA.L   $C015                ; CPLD RAM ON
              
        LDA     #$FF                 ; RUN
        STA.L   MYTH_RST_IO          ; RUN
        
        LDA     #$0   ;#$00 0000     ;MYTH
        PHA                          ;
        PLB
         
        LDX     #$00                 ;0  MOVE BOOT CODE TO SRAM
-:      LDA.L   CPLD_RAM+$7D0000,X  ;0
        STA.L   $3800,X             ;0  CPLD SRAM
        INX
        BNE     -                   ;0

        LDX     #$00                 ;0  MOVE CHEAT CODE TO SRAM
-:      LDA.L   CHEAT+$7D0000,X     ;0
        STA.L   $3E00,X             ;0  CPLD SRAM
        INX
        BNE     -                   ;0
        
        JMP.L   $003800
;===============================================================================
; EXIT RAM  $003800~003FFF
;===============================================================================
CPLD_RAM:
        REP	#$30        ; A,8 & X,Y 16 BIT
        LDX     #$0000
-:
        LDA     #$0000      ; READ GBA CARD
        STA.L   $7E0000,X  ; CLERA SRAM
        INX
        INX
        BNE     -
        SEP	#$30

        LDA     #$00                 ;
        STA.L   REG_DISPCNT
        STA.L   REG_MDMAEN
        STA.L   REG_HDMAEN
        STA.L   REG_NMI_TIMEN
	STA.L	REG_BG0MAP
	STA.L	REG_BG1MAP
	STA.L	REG_CHRBASE_L
	STA.L	REG_CHRBASE_H
	STA.L	REG_VRAM_INC
	STA.L	REG_VRAM_ADDR_L
	STA.L	REG_VRAM_ADDR_H
	STA.L	REG_BGCNT
	LDA	#$20
	STA.L	REG_COLDATA
	LDA	#$40
	STA.L	REG_COLDATA
	LDA	#$80
	STA.L	REG_COLDATA

        SEI
        SEC
        XCE              ;SET 65C02 MODE
        JMP     ($FFFC)

CHEAT:
        LDA   #$63
        STA.L $7EFA91
        
        LDA   #$01
-:
        BIT   $4012
        BNE   -
        RTL
        

	
run_secondary_cart:
	rep	#$30
	phx

	jsl	mosaic_up + $7D0000
	
	jsl	clear_screen
	
	sep	#$20
	lda	#1
	sta.l	$00c017
	lda	#$05
	sta.l	MYTH_OPTION_IO
	
	; Fill in the cartridge name
	ldx	#0
-:
	lda.l	$00ffc0,x
	sta.l	MS3+4,x
	inx
	cpx	#20
	bne	-
	
	lda	#$00
	sta.l	MYTH_OPTION_IO

	rep	#$20
	pea	3
	jsl	print_meta_string			; Print cart name
	pla
	pea	58
	jsl	print_meta_string			; Print instructions
	pla
	jsl	print_hw_card_rev
	
	; Print region and CPU/PPU1/PPU2 versions
	lda.l	REG_STAT78
	lsr	a
	lsr	a
	lsr	a
	lsr	a
	and	#1
	clc
	adc	#59
	pha
	jsl	print_meta_string
	pla
	lda.l	REG_RDNMI
	and	#3
	clc
	adc	#61
	pha
	jsl	print_meta_string
	pla
	lda.l	REG_STAT77
	and	#3
	clc
	adc	#65
	pha
	jsl	print_meta_string
	pla
	lda.l	REG_STAT78
	and	#3
	clc
	adc	#69
	pha
	jsl	print_meta_string
	pla
	
	jsl	dma_bg0_buffer

	jsl	mosaic_down + $7D0000
	
	; Wait until B or Y is pressed	
-:
	jsl	read_joypad
	lda.b	tcc__r0
	and	#$c000
	beq	-
	
	lda.b	tcc__r0
	and	#$8000
	beq	+
	; B was pressed
	lda	#0
	sep	#$20
	sta.l	$00c017
	jsl	mosaic_up + $7D0000
	rep	#$20
	jsl	clear_screen
	pea	0
	jsl	print_meta_string
	pla
	pea	4
	jsl	print_meta_string
	pla
	jsl	print_hw_card_rev
	jsl	print_games_list
	jsl	dma_bg0_buffer
	jsl	mosaic_down + $7D0000
	plx
	rtl
	
+:
	lda.b	tcc__r0
	and	#$4000
	beq	+
	; Y was pressed
	sep	#$20
	lda	#$05
	sta.l	MYTH_OPTION_IO
	lda	#0
	sta.l	$00c017
	lda	#$05
	sta.l	MYTH_OPTION_IO
	plx
	jmp.w	run_3800 & $ffff

+:
	rep	#$30
	plx
	rtl
	

	

; Updates and displays the "LOADING......(nn)" string 
show_loading_progress:
	jsr.w	_wait_nmi
	rep	#$30
	lda	#$22a3			; VRAM base address
	sta.l	REG_VRAM_ADDR_L
	sep	#$20
	plx
	phx
	lda.w	load_progress+15
	dea
	cmp	#'0'-1			; Do we need to wrap from 0 to 9?
	bne	+
	lda.w	load_progress+14
	dea
	sta.w	load_progress+14
	lda	#'9'
+:
	sta.w	load_progress+15
	ldx	#0
-:
	lda.w	load_progress,x
	beq	+
	sta.l	REG_VRAM_DATAW1		; Write the character number
	lda	#8			
	sta.l	REG_VRAM_DATAW2		; Write attributes (palette number)
	inx
	bra	-
+:

 ;.DEFINE NEO2_DEBUG 1
 .IFDEF NEO2_DEBUG
 	; Read back some of the data that has been copied to PSRAM and display
 	; it on-screen.
	rep	#$30
	lda	#$22c3
	sta.l	REG_VRAM_ADDR_L
	sep	#$20
	ldx	#0
-:
	lda.l	$500210,x
	lsr	a
	lsr	a
	lsr	a
	lsr	a
	clc
	adc	#'0'
	cmp	#58
	bcc	+
	clc
	adc	#7
+:
	sta.l	REG_VRAM_DATAW1
	lda	#0
	sta.l	REG_VRAM_DATAW2
	lda.l	$500210,x
	and	#15
	clc
	adc	#'0'
	cmp	#58
	bcc	+
	clc
	adc	#7
+:
	sta.l	REG_VRAM_DATAW1
	lda	#0
	sta.l	REG_VRAM_DATAW2
	inx
	cpx	#8
	bne	-
 .ENDIF
 
	rts
 
         

load_progress:
	.db "LOADING......(  )",0
	

 .MACRO MOV_PSRAM_SETUP 
        LDA.L  romSize
        CMP    #\1
        BNE    +
        LDA    #\2  
        STA    tcc__r0
          
        LDA    #<\3     
        STA    tcc__r0+1
        LDA    #>\3     
        STA    tcc__r0h
         
        LDA    #\4     
        STA    tcc__r0h+1
         
        LDA    #\5     
        STA    tcc__r1
        JMP    MOV_PSRAM
	+:
 .ENDM

 
 .MACRO MAP_SRAM_CHECK
 	 CMP     #\1
         BNE     +
         STA.L   MYTH_SRAM_TYPE  ; SET SRAM SAVE TYPE
         LDA     #\2
         STA.L   MYTH_SRAM_MAP   ; SET $206000
         JMP     MAP_SRAM_DONE
	 +:
 .ENDM
 
.8bit
.accu 8
SET_NEOCM5:
         LDA    #$E0      	; SET BANK   ;1 1 1 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $FFA400  	; FFD2XX     ;01

         LDA    #$00      	; SET BANK   ; 0 0 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $C02A00  	; 0015XX     ;02
         LDA.L  $C3A400  	; 01D2XX     ;03
         LDA.L  $C42A00  	; 0215XX     ;04

         LDA    #$E0      	; SET BANK   ;1 1 1 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $FC2A00  	; FE15XX     ;05
         RTS
;===============================================================================
SET_NEOCMA:
         JSR    SET_NEOCM5
         LDA    #$20      	; SET BANK   ;0 0 1 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $EE2406  	; 372203     ;06   ;
         RTS
;===============================================================================
SET_NEOCMB:
         JSR    SET_NEOCM5

         LDA.L  cardModel
         CMP    #$00
         BEQ    A_CARD
         CMP    #$01
         BEQ    B_CARD
         JMP    C_CARD
;-------------------------------------------------------------------------------
A_CARD:
         LDA    #$C0      	; SET    ;1 1 0 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $F55C88  	; DAAE44     ;06
         RTS
;-------------------------------------------------------------------------------
B_CARD:
         LDA    #$C0      	; SET    ;1 1 0 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $F51C88  	; DA8E44     ;06
         RTS
;-------------------------------------------------------------------------------
C_CARD:
         LDA    #$C0      	; SET    ;1 1 0 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $F41C88  	; DA0E44     ;06
         RTS
;===============================================================================
SET_NEOCMC:
         JSR    SET_NEOCM5

         LDA    #$E0      	; SET        ;1 1 1 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $DC0C60  	; EE0630     ;06
         RTS
;===============================================================================
SET_NEOCMD:

         JSR    SET_NEOCM5
         LDA    #$20      	; SET BANK   ;0 0 1 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $EE2006  	; 372003     ;06   ;  TO BIOS MENU

         JSR    SET_NEOCM5
         LDA    #$C0      	; SET        ;1 1 0 0
         STA.L  MYTH_GBAC_LIO
         LDA.L  $F50088  	; DA0044     ;06
         RTS
;===============================================================================


run_game_from_gba_card:
	jsl	clear_status_window
	jsl	dma_bg0_buffer
	
	sep	#$20
	
	lda	#$7e
	pha
	plb
	
	lda.l	romAddressPins
	sta	tcc__r2h
	
         LDA    #$20      	; OFF A21
         STA.L  MYTH_GBAC_ZIO
         JSR    SET_NEOCMA  	;
         JSR    SET_NEOCMB  	;
         JSR    SET_NEOCMC  	; ON_NEO CARD A24 & A25 + SA16 & SA17

         LDA.L   romAddressPins+1
         STA.L   MYTH_GBAC_HIO	; SET AH25,AH25
         
         LDA     #$01
         STA.L   MYTH_EXTM_ON   ; A25,A24 ON


; MOVE   PSRAM
MOV_4M:

	;                    4M    4M    4M  4M~8M
	MOV_PSRAM_SETUP $04, $3C, $3035, $04, $00
	;                    8M    8M    8M  4M~8M
	MOV_PSRAM_SETUP $08, $38, $3039, $08, $00
	;                    16M   16M   8M   16M
	MOV_PSRAM_SETUP $09, $30, $3137, $08, $01
	;                    32M   24M   8M   24M
	MOV_PSRAM_SETUP $0A, $20, $3235, $08, $02
	;                    32M   32M   8M   32M
	MOV_PSRAM_SETUP $0B, $20, $3333, $08, $03
	;                    64M   40M   8M   40M
	MOV_PSRAM_SETUP $0C, $00, $3431, $08, $04
	;                    64M   48M   8M   48M
	MOV_PSRAM_SETUP $0D, $00, $3439, $08, $05
	;                    64M   64M   8M   64M
	MOV_PSRAM_SETUP $0E, $00, $3635, $08, $07

;===============================================================================
MOV_PSRAM:
	lda	tcc__r0+1
	sta.w	load_progress+15
	lda	tcc__r0h
	sta.w	load_progress+14
	
         LDA    #$00
         STA    tcc__r1+1
             
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

MOV_PSRAM_LOOP:
         LDA    #$3E
         STA    tcc__r1h
         LDA    #$4E
         STA    tcc__r2
         
         LDA    #$3F
         STA    tcc__r1h+1
         
         LDA    #$4F
         STA    tcc__r2+1
         
-:
         INC    tcc__r1h
         INC    tcc__r2
         INC    tcc__r2+1
         
         INC    tcc__r1h
         INC    tcc__r2
         INC    tcc__r2+1

	 jsr	show_loading_progress

	 LDA	tcc__r1h
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$07		
	 
         LDA    tcc__r2
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$18	; First bank to write to
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$1f	; ...
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$26
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$2d
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$34
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$3b 
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$42 
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$49 
	 ina
	 sta.l	$7d0000+copy_1mbit_from_gbac_to_psram+$54
	

         DEC    tcc__r0+1
         LDA    tcc__r0+1        ; L-BYTE
         LDA    tcc__r0h        ; H-BYTE

         
         jsr	copy_1mbit_from_gbac_to_psram

         DEC    tcc__r0h+1
         BNE    -
         
         LDA    tcc__r1
         CMP    tcc__r1+1
         BNE    MOV_PSRAM_1
         JMP    MOV_PSRAM_DONE
MOV_PSRAM_1:
;===============================================================================
         INC    tcc__r1+1
         LDA    tcc__r1+1
         CMP    #$01
         BNE    +
         
         LDA    #$01        ;16M
         STA.L  MYTH_PRAM_BIO

         CLC
         LDA    #$08
         ADC    tcc__r2h
         STA    tcc__r2h
         STA.L  MYTH_GBAC_LIO
         
         LDA    #$8
         STA    tcc__r0h+1      ;
         JMP    MOV_PSRAM_LOOP
;-------------------------------------------------------------------------------
+:       CMP    #$02        ;24M
         BNE    +
         LDA    #$02
         STA.L  MYTH_PRAM_BIO

         CLC
         LDA    #$08
         ADC    tcc__r2h
         STA    tcc__r2h
         STA.L  MYTH_GBAC_LIO
         
         LDA    #$8
         STA    tcc__r0h+1
         JMP    MOV_PSRAM_LOOP
;-------------------------------------------------------------------------------
+:       CMP    #$03        ;32M
         BNE    +
         LDA    #$03
         STA.L  MYTH_PRAM_BIO
         
         CLC
         LDA    #$08
         ADC    tcc__r2h
         STA    tcc__r2h
         STA.L  MYTH_GBAC_LIO
         
         LDA    #$8
         STA    tcc__r0h+1      ;
         JMP    MOV_PSRAM_LOOP
;-------------------------------------------------------------------------------
+:       CMP    #$04        ;40M
         BNE    +
         LDA    #$04
         STA.L  MYTH_PRAM_BIO

         CLC
         LDA    #$08
         ADC    tcc__r2h
         STA    tcc__r2h
         STA.L  MYTH_GBAC_LIO
         
         
         LDA    #$8
         STA    tcc__r0h+1      ;
         JMP    MOV_PSRAM_LOOP
;-------------------------------------------------------------------------------
+:       CMP    #$05        ;48M
         BNE    +
         LDA    #$05
         STA.L  MYTH_PRAM_BIO
         
         CLC
         LDA    #$08
         ADC    tcc__r2h
         STA    tcc__r2h
         STA.L  MYTH_GBAC_LIO
         
         LDA    #$8
         STA    tcc__r0h+1      ;
         JMP    MOV_PSRAM_LOOP
;-------------------------------------------------------------------------------
+:       CMP    #$06        ;56M
         BNE    +
         LDA    #$06
         STA.L  MYTH_PRAM_BIO
         
         CLC
         LDA    #$08
         ADC    tcc__r2h
         STA    tcc__r2h
         STA.L  MYTH_GBAC_LIO
         
         LDA    #$8
         STA    tcc__r0h+1      ;
         JMP    MOV_PSRAM_LOOP
;-------------------------------------------------------------------------------
+:       CMP    #$07        ;64M
         BNE    MOV_PSRAM_DONE
         LDA    #$07
         STA.L  MYTH_PRAM_BIO
         
         CLC
         LDA    #$08
         ADC    tcc__r2h
         STA    tcc__r2h
         STA.L  MYTH_GBAC_LIO
         
         LDA    #$8
         STA    tcc__r0h+1      ;
         JMP    MOV_PSRAM_LOOP
MOV_PSRAM_DONE:
;===============================================================================
         LDA     #$00       ;
         STA.L   MYTH_WE_IO     ; PSRAM WRITE OFF
;-------------------------------------------------------------------------------
;neo gba card menu ,swap back to power on mode

         LDA     #MAP_MENU_FLASH_TO_ROM	; SET GBA CARD RUN
         STA.L   MYTH_OPTION_IO

         LDA     #$20       	; OFF A21
         STA.L   MYTH_GBAC_ZIO
         JSR     SET_NEOCMD	; SET MENU

         LDA     #$00
         STA.L   MYTH_GBAC_LIO
         STA.L   MYTH_GBAC_HIO
         STA.L   MYTH_GBAC_ZIO
;-------------------------------------------------------------------------------
;SET  SNES PSRAM RUN
         LDA     #$01       	; SET PSRAM RUN
         STA.L   MYTH_OPTION_IO

         LDA     #$00
         STA.L   MYTH_PRAM_BIO
        
         LDA     tcc__r0
         STA.L   MYTH_PRAM_ZIO

         LDA.L   sramBank
         LDA     #$00
         STA.L   MYTH_GBAS_BIO
;===============================================================================
;SET  GAME SAVE  SIZE & BANK
         LDA.L   sramSize
         CMP     #$00
         BNE     MAP_SRAM
         CMP     #$00
         STA.L   MYTH_SRAM_TYPE ;  OFF SRAM TYPE
         JMP     SET_EXT_DSP
;-------------------------------------------------------------------------------
MAP_SRAM:
         LDA.L   sramMode

 	MAP_SRAM_CHECK $01, $02
 	MAP_SRAM_CHECK $08, $03
 	MAP_SRAM_CHECK $07, $07
 	MAP_SRAM_CHECK $06, $07
 	MAP_SRAM_CHECK $05, $0F
 	MAP_SRAM_CHECK $04, $0F
 	
MAP_SRAM_DONE:
         LDA     #$01
         STA.L   MYTH_SRAM_WE    ; GBA SRAM WE ON !
;-------------------------------------------------------------------------------
;SET SAVE SRAM BANK

         LDA.L   sramSize
         CMP     #$01
         BNE     +
         LDA     #$FF    ;2K  SIZE
         STA.L   MYTH_GBAS_ZIO
         
         LDA.L   sramBank
         ASL     A
         ASL     A
         AND     #$FC
         STA.L   MYTH_GBAS_BIO
         JMP     SET_EXT_DSP

+:
         CMP     #$02
         BNE     +
         LDA     #$FC    ;8K   SIZE
         STA.L   MYTH_GBAS_ZIO
         
         LDA.L   sramBank
         ASL     A
         ASL     A
         AND     #$FC
         STA.L   MYTH_GBAS_BIO
         STA.L   MYTH_GBAS_BIO
         JMP     SET_EXT_DSP
         
+:
         CMP     #$03
         BNE     +
         LDA     #$F0    ;32K  SIZE
         STA.L   MYTH_GBAS_ZIO

         LDA.L   sramBank
         ASL     A
         ASL     A
         ASL     A
         ASL     A
         AND     #$F0
         STA.L   MYTH_GBAS_BIO
         JMP     SET_EXT_DSP
;-------------------------------------------------------------------------------
+:
         CMP     #$04
         BNE     +
         LDA     #$E0    ;64K SIZE
         STA.L   MYTH_GBAS_ZIO

         LDA.L   sramBank
         ASL     A
         ASL     A
         ASL     A
         ASL     A
         ASL     A
         AND     #$F0
         STA.L   MYTH_GBAS_BIO
         JMP     SET_EXT_DSP
;-------------------------------------------------------------------------------
+:
         CMP     #$05
         BNE     +
         LDA     #$C0    ;128K SIZE
         STA.L   MYTH_GBAS_ZIO

         LDA.L   sramBank
         ASL     A
         ASL     A
         ASL     A
         ASL     A
         ASL     A
         ASL     A
         AND     #$C0
         STA.L   MYTH_GBAS_BIO
         JMP     SET_EXT_DSP
+:
         ;JMP    MENU
         rtl
;-------------------------------------------------------------------------------
;SET EXIT DSP
SET_EXT_DSP:

         LDA.L   extDsp
         CMP     #$00
         BNE     +
         STA.L   MYTH_DSP_TYPE
         LDA     #$00
         STA.L   MYTH_DSP_MAP
         JMP     MAP_DSP_DONE
;-------------------------------------------------------------------------------
+:
         LDA.L   extDsp
         CMP     #$01
         BNE     +
         STA.L   MYTH_DSP_TYPE
         LDA     #$00
         STA.L   MYTH_DSP_MAP
         JMP     MAP_DSP_DONE
;-------------------------------------------------------------------------------
+:
         CMP     #$03
         BNE     +
         STA.L   MYTH_DSP_TYPE   ;EXT DSP
         LDA.L   romSize
         CMP     #$04     ;4M ROM
         BNE     +
         LDA     #$03     ;$300000
         STA.L   MYTH_DSP_MAP
         JMP     MAP_DSP_DONE
;-------------------------------------------------------------------------------
+:
         LDA     #$03
         STA.L   MYTH_DSP_TYPE  ;EXT DSP
         LDA     #$06    ;$600000
         STA.L   MYTH_DSP_MAP
         JMP     MAP_DSP_DONE
         
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
MAP_DSP_DONE:
         LDA.L   extSram
         CMP     #$07    ;
         BNE     +      ;
         LDA     #$06    ;
         STA.L   MYTH_OPTION_IO  ;
         LDA     #$C0    ;
         STA.L   $C01F  ;
+:

         LDA.L   romRunMode
         STA.L   MYTH_RUN_IO
RUN_M01:
         JMP     run_3800     ; FOR EXIT RAM
         
         
ram_code_end:

.ends

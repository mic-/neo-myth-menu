; Updated menu for the Neo Power PCE Flash Card
; v1.10
;
; Mic, 2010

.memorymap
        defaultslot 0
        slotsize $2000
        slot 0 $0000
        slot 1 $2000
        slot 2 $4000
        slot 3 $6000
        slot 4 $8000
        slot 5 $A000
        slot 6 $C000
        slot 7 $E000
.endme

.rombankmap
        bankstotal 8
        banksize $2000
        banks 8
.endro


; I/O register addresses, based on the menu's choise of memory mapping which
; maps the I/O registers to bank 2 ($4000-$5FFF)
.DEFINE VDC_CTRL			$4000
.DEFINE VDC_DATA_L			$4002
.DEFINE VDC_DATA_H			$4003

.DEFINE VCE_CTRL			$4400
.DEFINE VCE_INDEX_L			$4402
.DEFINE VCE_INDEX_H			$4403
.DEFINE VCE_DATA_L			$4404
.DEFINE VCE_DATA_H			$4405

.DEFINE PSG_CHANNEL_SELECT	$4800
.DEFINE PSG_GLOBAL_BALANCE	$4801
.DEFINE PSG_FREQ_FINE		$4802
.DEFINE PSG_FREQ_COARSE		$4803
.DEFINE PSG_CHANNEL_CTRL	$4804
.DEFINE PSG_CHANNEL_BALANCE $4805
.DEFINE PSG_CHANNEL_WAVE	$4806
.DEFINE PSG_NOISE_CTRL		$4807
.DEFINE PSG_LFO_FREQ		$4808
.DEFINE PSG_LFO_CTRL		$4809

.DEFINE TIMER_COUNTER		$4C00
.DEFINE TIMER_CTRL			$4C01

.DEFINE GAMEPAD_IO			$5000

.DEFINE INTERRUPT_CTRL		$5402
.DEFINE INTERRUPT_STATUS	$5403


.DEFINE GAMES_TO_SHOW		8

.DEFINE VALID_META_STRING_HEAD $A8

.DEFINE VALID_GAME_ENTRY_HEAD  $8A



; Ex: sti16 someZeroPageVar,1234
.macro sti16
	lda 	#<\2
	sta 	\1
	lda 	#>\2
	sta 	\1+1
.endm


; Ex: addi16 someZeroPageVar,1234
.macro addi16
	lda 	\1
	clc
	adc 	#<\2
	sta 	\1
	lda 	\1+1
	adc 	#>\2
	sta 	\1+1
.endm


; Set the VDC Memory Address Write Register
.macro set_mawri
	st0 	#0		; 	select VDC register 0 (MAWR)
	st1		#<\1
	st2		#>\1
.endm



.enum $0080
firstShown	ds 1		; The index of the first game that's currently listed on the screen.
count1		ds 1
tempw		ds 2
textColor	ds 1		; Used by write_char/print_meta_string. Contains the palette index and the upper bits of the tile index.
.ende

.enum $0091
charCode	ds 1		; Least significant byte of the tile index that write_char will output to VRAM.
vramAddr	ds 2		; Used for passing around VRAM addresses between some of the routines.
highlighted	ds 1		; The index of the currently highlighted game.
indPtr		ds 2		; Used as an indirect pointer in various places throughout the entire menu.
joyData		ds 1
temp		ds 1
lastGame	ds 1		; Index of the last game on the card (i.e. the total number of games minus one).
.ende

.enum $009D
fontPtr		ds 2
.ende


.org $0000
.dw 0


.org $1800
.db $8A,$04,$87,$00,$01,"SHANGHAI (J)              ",$FF
.db $8A,$04,$87,$00,$02,"BONK (U)                  ",$FF
.db $8A,$04,$87,$00,$04,"STREET FIGHTER II (U)     ",$FF
.db $8A,$04,$87,$00,$02,"GRADIUS (J)               ",$FF
.db $8A,$04,$87,$00,$02,"SALAMANDER (J)            ",$FF
.db $8A,$04,$87,$00,$02,"PARODIUS (J)              ",$FF
;.db $8A,$04,$87,$00,$02,"KNIGHTRIDER (U)           ",$FF
;.db $8A,$04,$87,$00,$01,"BATMAN (U)                ",$FF
;.db $8A,$04,$87,$00,$02,"TOUNGEMANS LOGIC (U)      ",$FF

.db $FF


.org $1FF0
 .dw 1,0,0
; Interrupt vectors
 .dw vdc_irq                    
 .dw vdc_irq                   
 .dw timer_irq               
 .dw vdc_irq               
 .dw start     


 
.bank 0 slot 7
.org $0000
.section "text"

; The font data needs to be placed on an even page boundary
font_data:
	.incbin "assets\pcefont.bin"
	

ram_code:
	lda		<$A3
	sta		$FFF0
	lda		<$A4
	sta		$FFF0
	lda		<$A5
	sta		$FFF0
	lda		<$9A
	sta		$FFF0
	lda		<$A3
	sta		$FFF0
	lda		<$A4
	sta		$FFF0
	lda		<$A6
	sta		$FFF0
	lda		<$9C
	sta		$FFF0
	lda		<$A3
	sta		$FFF0
	lda		<$A4
	sta		$FFF0
	lda		<$A7
	sta		$FFF0
	lda		<$9B
	sta		$FFF0
	sta		$FFF0

    ; Jump to the game's reset vector
	jmp		($FFFE)
ram_code_end:	
	


setup_video:
	; Setup the VDC registers (256x240 resolution, 32x32 BAT, BG enabled)
	stz.w	VCE_CTRL
	cly
-:
	lda.w	vdc_reg_table,y
	sta.w	VDC_CTRL
	iny
	lda.w	vdc_reg_table,y
	sta.w	VDC_DATA_L
	iny
	lda.w	vdc_reg_table,y
	sta.w	VDC_DATA_H
	iny
	cpy		#33
	bcc		-

	jsr		clear_bat
	
	sti16	fontPtr,font_data
	set_mawri $1000		; set VRAM address $1000 (byte address $2000)
	st0		#2			; select VDC register 2 (VWR)
	jsr		load_font
	inc		<fontPtr+1
	jsr		load_font
	inc		<fontPtr+1
	jsr		load_font
	inc		<fontPtr+1
	jsr		load_font

	set_mawri $2000
	st0		#2
	lda.w	$FFF0
	cmp		#1			; TGX or PCE ?
	beq		+
	; PCE
	tia		bg_patterns,VDC_DATA_L,133*32
	bra		++
+:
	; TGX
	tia		bg_patterns2,VDC_DATA_L,48*32
	tia		bg_patterns+48*32,VDC_DATA_L,85*32
++:		

	stz.w	VCE_INDEX_L
	stz.w	VCE_INDEX_H
	tia		palette,VCE_DATA_L,palette_end-palette
	rts


load_font:	
	cly
--:	
	ldx		#7
	sty		<tempw
-:
	lda		(<fontPtr),y
	sta		<temp
	sta		VDC_DATA_L
	lda.w	plane1_gradient,x
	and		<temp
	sta.w	VDC_DATA_H
	iny
	dex
	bpl		-
	ldx		#7
	ldy		<tempw
-:
	lda		(<fontPtr),y
	iny
	sta		<temp
	lda.w	plane2_gradient,x
	and		<temp
	sta.w	VDC_DATA_L
	lda.w	plane3_gradient,x
	and		<temp
	sta.w	VDC_DATA_H
	dex
	bpl		-
	cpy		#0
	bne		--
	rts


; Clear the Block Attribute Table (actually fills it with the tile indices need to display all the
; background graphics except for the text labels).
clear_bat:	
	; The BAT is located at VRAM address $0000
	set_mawri $0000
	
	st0		#2				; select VDC register 2 (VWR)
	lda.w	$FFF0
	cmp		#1
	beq		+
	sti16	indPtr,bg_nametable
	bra		++
+:
	sti16	indPtr,bg_nametable2
++:

	cly
-:
	lda		(<indPtr)
	sta.w	VDC_DATA_L
	lda		#2				; the background tiles are loaded starting at tile index $200
	sta.w	VDC_DATA_H
	addi16	indPtr,2
	iny
	bne		-

	sti16		indPtr,(bg_nametable+512)
	ldx		#2
	cly
-:
	lda		(<indPtr)
	sta.w	VDC_DATA_L
	lda		#2
	sta.w	VDC_DATA_H
	addi16	indPtr,2
	iny
	bne		-
	dex
	bpl 	-
	rts


delay:	
	pha
	lda		#$08
	sta		<tempw+1 
-:
	lda		#0
	sta		<tempw 
--:
	dec		<tempw
	lda		<tempw
	cmp		#0
	bne		--
	dec		<tempw+1
	lda		<tempw+1
	cmp		#0
	bne 	-
	pla
	rts
	
	
wait_keyrelease: 
	jsr 	read_joypad
	lda		<joyData
	bne		wait_keyrelease
	rts
	

read_joypad:	
	lda		#3
	sta		GAMEPAD_IO
	lda		#1
	sta		GAMEPAD_IO
	lda		GAMEPAD_IO
	and		#$0F
	tax
	lda.w	key_bits_lo,x
	sta		<joyData
	cla
	sta		GAMEPAD_IO
	lda		GAMEPAD_IO
	and		#$0F
	tax
	lda.w	key_bits_hi,x
	ora		<joyData
	sta		<joyData
	jsr		delay
	rts
	


start:		
	sei
	cld
	csl				; run the CPU at low speed
	
	; Setup banking. RAM at $0000 and $2000, I/O registers at $4000
	lda		#$F8
	tam		#$01
	tam		#$02
	lda		#$FF
	tam		#$04
	lda		#$01
	tam		#$40
	lda		#$02
	tam		#$20
	lda		#$03
	tam		#$10
	
	ldx		#$FF
	txs
	
	tii		ram_code,$2800,ram_code_end-ram_code

	lda		#$11				; $11 = solid white text
	sta		<textColor
	
	cly
	jsr		setup_video
	jsr 	print_strings

	; Print "I"/"II" in blue
	lda		#$31				; $31 = ligh blue text
	sta		<textColor
	lda		#'I'
	sta		<charCode
	sti16	vramAddr,$0245
	jsr		write_char
	lda		#$65
	sta		<vramAddr
	jsr		write_char
	inc		<vramAddr
	jsr		write_char
	lda		#$11
	sta		<textColor

	; Count the number of games on the card	
	ldx		#$FF
	cly
	sty		<$98
	iny
-:
	iny
	ldy		<temp
	lda.w	game_table_pointers,y
	sta		<indPtr
	iny
	lda.w	game_table_pointers,y
	sta		<indPtr+1
	iny
	sty		<temp
	lda		(<indPtr)
	cmp		#VALID_GAME_ENTRY_HEAD
	beq 	+
	stx 	<lastGame
	inx
	bra		++
+:
	inx
	jmp		-
++:

	stz		<highlighted
	stz		<firstShown
	jsr		print_game_info
menu_loop:
	jsr		read_joypad
	lda		<joyData
	cmp		#$04
	beq 	key_down
	cmp		#$08
	beq		key_up
	cmp		#$40
	beq		key_ii
	cmp		#$80
	beq		key_i
	bra		menu_loop
key_ii:
	stz 	<$AB
	bra		load_game
key_i:
	lda 	#$80
	sta 	<$AB
	bra		load_game
key_down:
	jsr 	wait_keyrelease
	jsr		move_to_next_game
	bra 	print_game_info
key_up:
	jsr 	wait_keyrelease
	jsr		move_to_previous_game
	bra 	print_game_info
print_game_info:
	lda		<highlighted
	asl		a
	tay
	lda.w 	game_table_pointers,y
	sta 	<indPtr
	iny
	lda.w 	game_table_pointers,y
	sta 	<indPtr+1

	ldy 	#1
	lda 	(<indPtr),y
	sta 	<$9A
	iny
	lda 	(<indPtr),y
	sta 	<$9B
	iny
	lda 	(<indPtr),y
	sta 	<$9C
	iny
	lda 	(<indPtr),y
	sta 	<$A1

	jsr		print_games_list

	lda		#$11
	sta		<textColor
	
	jsr		print_game_num_game_size
	bra		menu_loop


load_game:
	lda.w	$FFF0
	cmp 	#1
	beq		_lg_tg16
	lda		#$57
	sta		<$A3
	lda		#$75
	sta		<$A4
	lda		#$63
	sta		<$A5
	lda		#$85
	sta		<$A6
	lda		#$36
	sta		<$A7
	lda		<$AB
	beq		+
	lda		<$9C
	ora		#$80
	sta		<$9C
+:
	jmp		$2800		; jump to our RAM code
_lg_tg16:
	lda		#$EA
	sta		<$A3
	lda		#$AE
	sta		<$A4
	lda		#$C6
	sta		<$A5
	lda		#$A1
	sta		<$A6
	lda		#$6C
	sta		<$A7
	lda		<$9A
	jsr		reverse_byte
	sta		<$9A
	lda		<$9C
	jsr		reverse_byte
	sta		<$9C
	lda		<$9B
	jsr		reverse_byte
	sta		<$9B
	lda		<$AB
	beq		+
	lda		<$9C
	ora		#1
	sta		<$9C
+:
	jmp		$2800

reverse_byte:	
	ror		a
	rol 	<temp
	ror		a
	rol 	<temp
	ror		a
	rol 	<temp
	ror		a
	rol 	<temp
	ror		a
	rol 	<temp
	ror		a
	rol 	<temp
	ror		a
	rol 	<temp
	ror		a
	rol 	<temp
	lda		<temp ;$A2
	rts
	


move_to_next_game:
	; if (highlighted != lastGame)
	;   highlighted++;
	; else {
	;   highlighted = firstShown = 0;
	; }
	lda 	<highlighted
	cmp 	<lastGame
	beq 	+
	inc		<highlighted
	bra 	++
+:
	stz 	<highlighted
	stz		<firstShown
++:
	; if ((firstShown < (GAMES_TO_SHOW / 2)-1) && (firstShown + GAMES_TO_SHOW-1 < lastGame))
	;   firstShown++;
	lda		<firstShown
	clc
	adc		#((GAMES_TO_SHOW / 2) - 1)
	cmp		<highlighted
	bcs		_mtng_done
	lda		<firstShown
	clc
	adc		#GAMES_TO_SHOW-1
	cmp		<lastGame
	bcs		_mtng_done
	inc		<firstShown
_mtng_done:
	rts

        
move_to_previous_game:
	; if (highlighted != 0)
	;   highlighted--;
	; else {
	;   highlighted = lastGame;
	;   firstShown = highlighted - (GAMES_TO_SHOW-1);
	;   firstShown = (firstShown >= 0) ? firstShown : 0;
	; }
	lda 	<highlighted
	beq 	+
	dea
	sta 	<highlighted
	bra 	++
+:
	lda 	<lastGame
	sta 	<highlighted
	sec
	sbc		#GAMES_TO_SHOW-1
	bpl		+
	cla
+:
	sta		<firstShown
++:
	; if ((firstShown >= (GAMES_TO_SHOW / 2)-1) && (firstShown + GAMES_TO_SHOW-1 < lastGame))
	;   firstShown--;
	lda		<firstShown
	beq		_mtpg_done
	clc
	adc		#((GAMES_TO_SHOW / 2) - 1)
	cmp		<highlighted
	bcc		_mtpg_done
	dec		<firstShown
_mtpg_done:
	rts



print_strings:
	stz		<tempw ;$9F
-:
	ldy		<tempw
	lda		string_table,y
	sta		<indPtr
	iny
	lda		string_table,y
	sta		<indPtr+1
	lda		(<indPtr)
	cmp		#VALID_META_STRING_HEAD
	beq		+
	rts
+:
	iny
	sty		<tempw
	ldy		#1
	lda		(<indPtr),y
	sta		<vramAddr
	iny	
	lda		(<indPtr),y
	sta		<vramAddr+1
	jsr		print_meta_string
	jmp		-
	
	
print_games_list:
	sti16	tempw,$0124			; VRAM address $0124 = screen pixel 32,72
	stz		<count1
-:
	lda		<count1
	clc
	adc		<firstShown
	cmp		<lastGame
	bcc		+
	beq		+
	bra		_pgl_done
+:
	lda		<tempw
	sta		<vramAddr
	lda		<tempw+1
	sta		<vramAddr+1

	ldx		#$21				; $21 = brown gradient text

	lda		<count1
	clc
	adc		<firstShown
	cmp		<highlighted
	bne		+
	ldx		#$11				; $11 = solid white text
+:
	stx		<textColor
	asl		a
	tay
	lda.w 	game_table_pointers,y
	sta 	<indPtr
	iny
	lda.w 	game_table_pointers,y
	sta 	<indPtr+1
	ldy		#4
	jsr		print_meta_string
	
	; Move to the next exntry in the table
	addi16	tempw,$20
	
	inc		<count1
	lda		<count1
	cmp		#GAMES_TO_SHOW
	bne		-
_pgl_done:
	rts
	
	
print_meta_string: 
	st0		#0
	lda		<vramAddr
	sta.w	VDC_DATA_L
	lda		<vramAddr+1
	sta.w	VDC_DATA_H
	st0		#2
-:
	cpy		#28
	beq		+
	iny
	lda		(<indPtr),y
	cmp		#$FF
	beq		+
	sta.w	VDC_DATA_L
	lda		<textColor
	sta.w	VDC_DATA_H
	inc		<vramAddr
	jmp		-
+:
	rts


print_game_num_game_size:   
	ldy 	<highlighted
	iny
	lda 	binary_to_decimal,y
	sta 	<$A0
	and 	#$0F
	tay
	lda 	hex_to_ascii,y
	sta 	<charCode
	sti16	vramAddr,$028A
	jsr		write_char
	lda		<$A0
	ror		a
	ror		a
	ror		a
	ror		a
	and		#$0F
	tay
	lda		hex_to_ascii,y
	sta		<charCode
	dec		<vramAddr
	jsr		write_char
	lda 	<$A1
	and 	#$0F
	tay
	lda.w 	hex_to_ascii,y
	sta 	<charCode
	sti16	vramAddr,$0293
	jsr		write_char	
	lda		<$A1
	ror		a
	ror		a
	ror		a
	ror		a
	and		#$0F
	tay
	lda.w	hex_to_ascii,y
	sta		<charCode
	dec		<vramAddr
	jsr		write_char
	rts
	

write_char:		
	st0		#0
	lda		<vramAddr
	sta.w	VDC_DATA_L
	lda		<vramAddr+1
	sta.w	VDC_DATA_H
	st0		#2
	lda		<charCode
	sta.w	VDC_DATA_L
	lda		<textColor
	sta.w	VDC_DATA_H
	rts


vdc_irq:
timer_irq:
	rti



vdc_reg_table: 
	.db $05
	.dw $0080
	.db $06
	.dw $0000
	.db $07
	.dw $0000 
	.db $08
	.dw $0000
	.db $09
	.dw $0080
	.db $0A
	.dw $0202
	.db $0B
	.dw $031F	
	.db $0C
	.dw $0F02 	
	.db $0D
	.dw $00EF
	.db $0E
	.dw $0003
	.db $0F
	.dw $0000
	
	
plane1_gradient:
	.db $FF,$00,$FF,$00,$FF,$00,$FF,$00
plane2_gradient:
	.db $FF,$FF,$00,$00,$FF,$FF,$00,$00
plane3_gradient:
	.db $FF,$FF,$FF,$FF,$00,$00,$00,$00
	
	
key_bits_lo:
	.db $0F,$07,$0E,$06,$0B,$03,$0A,$02
	.db $0D,$05,$0C,$04,$09,$01,$08,$00
key_bits_hi:
	.db $F0,$70,$B0,$30,$D0,$50,$90,$10
	.db $E0,$60,$A0,$20,$C0,$40,$80,$00
	

string_table:	
	.dw string1,string2,string3,string4
	.dw string5 


string1: .db $A8,$C4,$00,"MENU V1.10, NEOTEAM 2010",$FF
string2: .db $A8,$C0,$00,$FF
string3: .db $A8,$44,$02,"[ ] : LOAD (SAVE ON)",$FF 
string4: .db $A8,$64,$02,"[  ]: LOAD (SAVE OFF)",$FF
string5: .db $A8,$84,$02,"GAME 00, SIZE 00M",$FF
.db $A0


hex_to_ascii:  
.db "0123456789ABCDEF"

binary_to_decimal: 
.db $00,$01,$02,$03,$04,$05,$06,$07,$08,$09
.db $10,$11,$12,$13,$14,$15,$16,$17,$18,$19
.db $20,$21,$22,$23,$24,$25,$26,$27,$28,$29
.db $30,$31,$32,$33,$34,$35,$36,$37,$38,$39
.db $40,$41,$42,$43,$44,$45,$46,$47,$48,$49
.db $50,$51,$52,$53,$54,$55,$56,$57,$58,$59
.db $60,$61,$62,$63,$64,$65,$66,$67,$68,$69


game_table_pointers:	
.dw $F800,$F820,$F840,$F860,$F880,$F8A0,$F8C0,$F8E0
.dw $F900,$F920,$F940,$F960,$F980,$F9A0,$F9C0,$F9E0
.dw $FA00,$FA20,$FA40,$FA60,$FA80,$FAA0,$FAC0,$FAE0
.dw $FB00,$FB20,$FB40,$FB60,$FB80,$FBA0,$FBC0,$FBE0
.dw $FC00,$FC20,$FC40,$FC60,$FC80,$FCA0,$FCC0,$FCE0
.dw $FD00,$FD20,$FD40,$FD60,$FD80,$FDA0,$FDC0,$FDE0
.dw $FE00,$FE20,$FE40,$FE60,$FE80,$FEA0,$FEC0,$FEE0
.dw $FF00,$FF20,$FF40,$FF60,$FF80,$FFA0,$FFC0,$FFE0

	
palette:
	.incbin "assets\menu_bg3.pal"
	.dw 0
	.dw 0,0,0,0,0,0,0,0
	.dw 0,$01FF,$01FF,$01FF,$01FF,$01FF,$01FF,$01FF
	.dw $01FF,$01FF,$01FF,$01FF,$01FF,$01FF,$01FF,$01FF
	;.dw 0,$0111,0,$0199,0,$01AA,0,$01BC
	;.dw 0,$01AC,0,$019A,0,$0192,0,$0101
	;.dw 0,$99,0,$E2,0,$133,0,$1BC
	;.dw 0,$1BC,0,$133,0,$E2,0,$99
	.dw 0,$1BC,0,$1BC,0,$133,0,$133
	.dw 0,$E2,0,$E2,0,$99,0,$99
	.dw 0,$00D7,$00D7,$00D7,$00D7,$00D7,$00D7,$00D7
	.dw $00D7,$00D7,$00D7,$00D7,$00D7,$00D7,$00D7,$00D7
palette_end:

bg_nametable:
 	.incbin "assets\menu_bg3.nam"
	
	
.ends



.bank 1 slot 6
.section "data"


bg_nametable2:
	.incbin "assets\menu_bg4.nam"
	
bg_patterns:
	.incbin "assets\menu_bg3.chr"

bg_patterns2:
	.incbin "assets\menu_bg4.chr" read 48*32
	

.ends
.org $1FFD
.db "mic"


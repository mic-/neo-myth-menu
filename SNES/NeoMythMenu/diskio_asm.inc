; SNES specific implementation of diskio 
; /Mic


; Fixed bugs:
;
; * Missing PLX/PLY instructions
; * Wrong loop count in recvMmcCmdResp
; * sdInit's rca value getting overwritten by crc7
; * wrMmcxxxByte writing the bits in reversed order
;
; * Changed 16-bit Myth reads to 8-bit
; * Changed stack offsets in neo2_recv_sd
;
; * Switched order between neo2_enable_sd and neo2_pre_sd
;
; * Wrong loop condition in rdMmcCmdBits


.EQU R1_LEN 6
.EQU R2_LEN 17
.EQU R3_LEN 6
.EQU R6_LEN 6
.EQU R7_LEN 6


.EQU RES_OK 0
.EQU RES_ERROR 1
.EQU RES_WRPRT 2
.EQU RES_NOTRDY 3
.EQU RES_PARERR 4

	
.EQU INIT_RETRIES 64


; MMC commands
.EQU GO_IDLE_STATE 0
.EQU SET_BUS_WIDTH 6
.EQU SEND_IF_COND 8
.EQU STOP_TRANSMISSION 12
.EQU READ_SINGLE_BLOCK 17
.EQU READ_MULTIPLE_BLOCK 18
.EQU SD_SEND_OP_COND 41
.EQU APP_CMD 55


.EQU DISKIO_CACHE_SIZE 16

;**********************************************************************************************



; void wrMmcCmdBit( unsigned int bit );
;
.EQU _wrMmcCmdBit_save_regs 2					; the total size of the registers being saved when entering the function (i.e. the PHX in this case, which equals 2 bytes of stack space)
.EQU _wrMmcCmdBit_bit 3+_wrMmcCmdBit_save_regs	; the stack-relative position of the 'bit' argument, after registers have been saved upon function entry
;
wrMmcCmdBit:
    rep    	#$30    			; 16-bit regs
    phx
    lda    	_wrMmcCmdBit_bit,s 	; read argument from stack
    and    	#1
    beq    	+
    lda    	#$2000  			; set a.13 if (bit & 1) == 1
+:
    tax
    sep	   	#$20				; 8-bit A
    lda.l  	$CFDE00,x  			; read either $CFDE00 or $CFFE00 depending on the value of bit & 1
    rep    	#$20				; 16-bit A
    plx
    rts


;**********************************************************************************************


; void wrMmcDatBit( unsigned int bit )
;
.EQU _wrMmcDatBit_save_regs 2					
.EQU _wrMmcDatBit_bit 3+_wrMmcDatBit_save_regs
;	
wrMmcDatBit:
    rep    	#$30    				; 16-bit regs
    phx
    lda    	_wrMmcDatBit_bit,s 		; read argument from stack
    and    	#1
    beq    	+
    lda    	#$200  					; set a.9 if (bit & 1) == 1
+:
    tax
    sep  	#$20					; 8-bit A
    lda.l  	$CFFC00,x  				; read either $CFFC00 or $CFFE00 depending on the value of bit & 1
    rep		#$20					; 16-bit A
    plx
    rts
    

;**********************************************************************************************


; void wrMmcDatBit4( unsigned char dat )
;
.EQU _wrMmcDatBit4_save_regs 2					
.EQU _wrMmcDatBit4_dat 3+_wrMmcDatBit4_save_regs	
;
wrMmcDatBit4:
    rep    #$30    					; 16-bit regs
    phx
    lda    _wrMmcDatBit4_dat,s  	; read argument from stack
    and    #15
    asl	   a
    xba								; A = (dat & 15) << 9
    tax
    sep		#$20					; 8-bit A
    lda.l  	$CFE000,x
    rep		#$20					; 16-bit A
    plx
    rts



;**********************************************************************************************


; void wrMmcCmdByte( unsigned int byte )
;
.EQU _wrMmcCmdByte_save_regs 2
.EQU _wrMmcCmdByte_byte 3+_wrMmcCmdByte_save_regs
;
wrMmcCmdByte:
    rep    #$30    				; 16-bit regs
    phx
    lda    _wrMmcCmdByte_byte,s ; read argument from stack
    ldx    #8
-:
	asl 	a					; d7 -> d8
    pha							; save A
	xba							; d8 -> d0
    and    #1
    pha							; put argument on stack
    jsr    wrMmcCmdBit			; write  (byte >> (7-i)) & 1
    pla							; remove argument for wrMmcCmdBit
    pla							; restore A
    dex
    bne    -
    plx
    rts
        	

;**********************************************************************************************


; void wrMmcDatByte( unsigned int byte )
;
.EQU _wrMmcDatByte_save_regs 2
.EQU _wrMmcDatByte_byte 3+_wrMmcDatByte_save_regs
;
wrMmcDatByte:
    rep    #$30    				; 16-bit regs
    phx
    lda    _wrMmcDatByte_byte,s    
    ldx    #8
-:
	asl 	a
	pha
	xba
    and    #1
    pha
    jsr    wrMmcDatBit			; write  (byte >> (7-i)) & 1
    pla
    pla
    dex
    bne    -
    plx
    rts


;**********************************************************************************************


; unsigned int rdMmcCmdBit()
;
rdMmcCmdBit:
    sep		#$20						; 8-bit A
    lda.l  	$CFFE81
    lsr    	a
    lsr    	a
    lsr    	a
    lsr    	a
    rep		#$20						; 16-bit A
    and	   	#1
    sta.b  	tcc__r0						; return ((*(u8*)0xCFFE81) >> 4) & 1
    rts


;**********************************************************************************************


; unsigned int rdMmcDatBit()
;
rdMmcDatBit:
    sep		#$20						; 8-bit A
    lda.l  	$CF7C21
    rep		#$20						; 16-bit A
    and	   	#1
    sta.b  	tcc__r0
    rts


;**********************************************************************************************


; unsigned int rdMmcDatBit4()
;
rdMmcDatBit4:
    sep		#$20						; 8-bit A
    lda.l  	$CE6061
    rep		#$20						; 16-bit A
    and	  	#15
    sta.b  	tcc__r0
    rts


;**********************************************************************************************


; unsigned char rdMmcCmdBits( int num )
;
.EQU _rdMmcCmdBits_save_regs 2
.EQU _rdMmcCmdBits_num 3+_rdMmcCmdBits_save_regs
;
rdMmcCmdBits:
    rep    #$30    				; 16-bit regs
    phx
    lda    _rdMmcCmdBits_num,s 
    tax
    lda    #0
-:
    pha
    jsr    rdMmcCmdBit
    pla
    asl    a
    ora    tcc__r0				; A = (A << 1) | rdMmcCmdBit()
    dex
	bne		-
    sta    tcc__r0
    plx
    rts


;**********************************************************************************************


; unsigned char rdMmcCmdByte()
rdMmcCmdByte:
    rep    #$30
    pea.w  8
    jsr    rdMmcCmdBits
    pla
    rts    	


;**********************************************************************************************


; void wrMmcDatByte4( unsigned char val )
; 
.EQU _wrMmcDatByte4_save_regs 0
.EQU _wrMmcDatByte4_val 3+_wrMmcDatByte4_save_regs
;
wrMmcDatByte4:
    rep    #$30
    lda    _wrMmcDatByte4_val,s		; read argument from stack
    lsr    a
    lsr    a
    lsr    a
    lsr    a
    and    #15
    pha
    jsr    wrMmcDatBit4				; write high nybble
    pla
    lda    _wrMmcDatByte4_val,s		
    and    #15
    pha
    jsr    wrMmcDatBit4				; write low nybble
    pla
    rts    


;**********************************************************************************************


; unsigned char rdMmcDatByte4()
;
rdMmcDatByte4:
	rep	   #$30						; 16-bit regs
	jsr    rdMmcDatBit4
	lda	   tcc__r0
	asl    a
	asl    a
	asl    a
	asl    a						; A = rdMmcDatBit4() << 4
	pha
	jsr    rdMmcDatBit4
	pla
	ora    tcc__r0					; A |= rdMmcDatBit4()
	sta    tcc__r0			
	rts
	

;**********************************************************************************************


; unsigned char rdMmcDatByte()
;
rdMmcDatByte:
    rep    #$30    					; 16-bit regs
    phx
    ldx    #8
    lda    #0
-:
    pha
    jsr    rdMmcDatBit
    pla
    asl    a
    ora    tcc__r0					; A = (A << 1) | rdMmcDatBit()
    dex
    bne    -
    sta    tcc__r0
    plx
    rts
    

;**********************************************************************************************


; unsigned char crc7 (unsigned char *buf)
;
.EQU _crc7_save_regs 4
.EQU _crc7_buf 3+_crc7_save_regs
;
crc7:
    rep    	#$30
    phx
    phy
    lda    	_crc7_buf,s				; buf offset
    sta    	tcc__r1
    lda    	_crc7_buf+2,s 			; buf bank
    sta    	tcc__r1h
    lda    	#$8080
    sta    	tcc__r2					; r4 = 0x80808080
    sta    	tcc__r2h					; ....
    stz    	tcc__r3					; c = 0
    stz    	tcc__r0					; crc = 0
    ldx    	#40						; 40 bits
    ldy    	#0
-:
    sep    	#$20					; 8-bit A
    lda    	tcc__r2					; if (r4 & 0x80) c = *buf++;
    bpl    	+
    lda    	[tcc__r1],y
    iny
	sta		tcc__r3
+:
    asl    	tcc__r0					; crc <<= 1

    lda    	tcc__r0					; if (crc & 0x80) crc ^= 9;
    bpl    	+
    eor    	#9
    sta    	tcc__r0
+:
    lda    	tcc__r2h+1				; if (c & (r4>>24)) crc ^= 9;
    and    	tcc__r3
    beq    	+
    lda    	tcc__r0
    eor    	#9
    sta    	tcc__r0
+:
    rep    	#$20					; 16-bit A
    lda    	tcc__r2					; r4 = (r4 >> 1) | (r4 << 31);
    lsr    	a
    ror    	tcc__r2h
    ror    	tcc__r2
   
    dex
    bne    	-
    ply
    plx
    rts    
 

;**********************************************************************************************



; void sendMmcCmd( unsigned char cmd, unsigned int arg )
;
.EQU _sendMmcCmd_save_regs 2
.EQU _sendMmcCmd_cmd 3+_sendMmcCmd_save_regs
.EQU _sendMmcCmd_arg _sendMmcCmd_cmd+2
;
sendMmcCmd:
    rep  	#$30    				; 16-bit regs
    phx
    
    sep   	#$20					; 8-bit A
    lda    	_sendMmcCmd_cmd,s		; cmd
    ora    	#$40					; b7 = 0 => start bit, b6 = 1 => host command
    sta.w	diskioPacket
    rep		#$20					; 16-bit A
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla
    
    sep   	#$20
    lda		_sendMmcCmd_arg+3,s		; arg >> 24
    sta.w	diskioPacket+1
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		_sendMmcCmd_arg+2,s		; arg >> 16
    sta.w	diskioPacket+2
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		_sendMmcCmd_arg+1,s		; arg >> 8
    sta.w	diskioPacket+3
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		_sendMmcCmd_arg,s		; arg 
    sta.w	diskioPacket+4
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    pea.w	:diskioPacket
    pea.w	diskioPacket
    jsr		crc7
    pla
    pla
    lda		tcc__r0
    asl		a
    ora		#1						; b0 = 1 => stop bit

; DEBUG
sep #$20
sta.w diskioPacket+5
rep #$20

    and		#$FF
    pha
    jsr		wrMmcCmdByte			; wrMmcCmdByte( crc7(pkt) << 1 ) | 1 )
    pla
    
    plx
    rts


.MACRO SEND_MMC_CMD ;cmd, arg
	pea.w	\2 / 65536
	pea.w	\2 & 65535
	pea.w	\1
	jsr		sendMmcCmd			
	pla
	pla
	pla    
.ENDM


;**********************************************************************************************


; BOOL recvMmcCmdResp( unsigned char *resp, unsigned int len, int cflag )
;
.EQU _recvMmcCmdResp_save_regs 4
.EQU _recvMmcCmdResp_resp 3+_recvMmcCmdResp_save_regs
.EQU _recvMmcCmdResp_len _recvMmcCmdResp_resp+4
.EQU _recvMmcCmdResp_cflag _recvMmcCmdResp_len+2
;
recvMmcCmdResp:
    rep  	#$30    					; 16-bit regs
    phx
    phy
    
    lda		_recvMmcCmdResp_resp,s		; resp offset
    sta		tcc__r1
    lda		_recvMmcCmdResp_resp+2,s	; resp bank
    sta		tcc__r1h
    
    ldy		#0
    ldx		#1024
-:
	; Wait for start bit
	jsr		rdMmcCmdBit
	lda		tcc__r0
	bne		+
	pea.w	7
	jsr		rdMmcCmdBits
	pla
	sep		#$20						; 8-bit A
	lda		tcc__r0
	sta		[tcc__r1],y					; *r++ = rdMmcCmdBits(7);
	iny
	
	rep		#$20						; 16-bit A
	lda		_recvMmcCmdResp_len,s		; len
	phx
	dea
	tax
--:										; len--; for(j=0; j<len; j++)
	cpx		#0
	beq		++
	jsr		rdMmcCmdByte
	sep		#$20						; 8-bit A
	lda		tcc__r0
	sta		[tcc__r1],y					; *r++ = rdMmcCmdByte();
	iny
	dex
	bra		--
++:
	plx
	
	rep		#$20						; 16-bit A
	lda		_recvMmcCmdResp_cflag,s		; cflag
	beq		++
	pea.w	$FF
	jsr		wrMmcCmdByte				; wrMmcCmdByte(0xFF);
	pla
++:

	lda		#1
	sta		tcc__r0						; return TRUE
	ply
	plx
	rts
+:
	dex
	bne		-
	
	stz		tcc__r0						; return FALSE
	ply
	plx
	rts
	

.MACRO RECV_MMC_CMD_RESP
	pea.w	\3
	pea.w	\2
	pea.w	:\1
	pea.w	\1
	jsr		recvMmcCmdResp
	pla
	pla
	pla
	pla
.ENDM
    

;**********************************************************************************************



; BOOL sdReadSingleBlock( unsigned char *buf, unsigned int addr )
sdReadSingleBlock:
	rep		#$30
	phx

	lda		11,s				; addr bank
	pha
	lda		11,s				; addr offset (at 9,s. but the PHA on the previous line pushed it 2 bytes further away)
	pha
	pea.w	READ_SINGLE_BLOCK				
	jsr		sendMmcCmd
	pla
	pla
	pla
	
	lda.l	cardType
	and		#$8000				; cardType & 0x8000 ?
	bne		+
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,0
	lda		tcc__r0
	beq		++
	sep		#$20
	lda.w	diskioResp
	cmp		#READ_SINGLE_BLOCK
	beq		+
++:
	plx
	rep		#$20
	stz		tcc__r0				; return FALSE
	rts
+:
	rep		#$20

	ldx		#8191	
-:								; while ((rdMmcDatBit4()&1) != 0)
	jsr		rdMmcDatBit4		
	lda		tcc__r0
	and		#1
	beq		+
	txa
	bmi		++
	dex
	bra		-
++:
	plx
	stz		tcc__r0				; if (i-- < 0) return FALSE (timeout on start bit)
	rts
+:

	lda		7,s					; buf bank
	pha
	lda		7,s					; buf offset (at 5,s. but the PHA on the previous line pushed it 2 bytes further away)
	pha
	jsr		neo2_recv_sd
	pla
	pla

	pea.w	$FF
	jsr		wrMmcCmdByte		; wrMmcCmdByte(0xFF);
	pla

	lda		#1
	sta		tcc__r0				; return TRUE
	plx
	rts
	

;**********************************************************************************************


; BOOL sdReadStartMulti( unsigned int addr )
;
.EQU _sdReadStartMulti_save_reg 2
.EQU _sdReadStartMulti_addr 3+_sdReadStartMulti_save_reg
;
sdReadStartMulti:
	rep		#$30
	phx

	lda		_sdReadStartMulti_addr+2,s	; addr bank
	pha
	lda		_sdReadStartMulti_addr+2,s	; addr offset (the last pha pushed it 2 bytes further away)
	pha
	pea.w	READ_MULTIPLE_BLOCK				
	jsr		sendMmcCmd
	pla
	pla
	pla

	lda.l	cardType
	and		#$8000
	bne		+
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,0
	lda		tcc__r0
	beq		++
	sep		#$20
	lda.l	diskioResp
	cmp		#READ_MULTIPLE_BLOCK
	beq		+
++:
	plx
	rep		#$20
	stz		tcc__r0				; return FALSE
	rts
+:
	rep		#$20

	lda		#1
	sta		tcc__r0				; return TRUE
	plx
	rts

;**********************************************************************************************


; BOOL sdReadStopMulti( void )
sdReadStopMulti:
	rep		#$30
	phx
	SEND_MMC_CMD STOP_TRANSMISSION,0

	lda.l	cardType
	and		#$8000
	bne		+
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,0
	lda		tcc__r0
	beq		++
	sep		#$20
	lda.l	diskioResp
	cmp		#STOP_TRANSMISSION
	beq		+
++:
	plx
	rep		#$20
	stz		tcc__r0			; return FALSE
	rts
+:
	rep		#$20

	lda		#1
	sta		tcc__r0			; return TRUE
	plx
	rts

;**********************************************************************************************


; void sdCrc16(unsigned char *p_crc, unsigned char *data, int len)
;
.EQU _sdCrc16_save_regs 4
.EQU _sdCrc16_p_crc 3+_sdCrc16_save_regs
.EQU _sdCrc16_data _sdCrc16_p_crc+4
.EQU _sdCrc16_len _sdCrc16_data+4
;
.EQU _sdCrc16_n_crc tcc__r4
;
sdCrc16:
	rep		#$30
	phx
	phy

    ; crc = 0
    stz		tcc__r2
    stz		tcc__r2h
    stz		tcc__r3
    stz		tcc__r3h

	; Pre-multiply len by 2
	lda		_sdCrc16_len,s
	asl		a
	sta		_sdCrc16_len,s

	; Backup some of the TCC zeropage vars (tcc__r4/r4h, tcc__r5/r5h, tcc__r9/r9h)
    ldx		#12
-:
	lda.b	tcc__r4,x
    sta.l	diskioRegBackup,x
	dex
	bne		-

	; Load crc from array
	lda		_sdCrc16_p_crc,s
	sta		tcc__r9
	lda		_sdCrc16_p_crc+2,s
	sta		tcc__r9h
	ldy		#0
	ldx		#7
	sep		#$20
-:
	lda		[tcc__r9],y
	iny
	sta.b	tcc__r2,x
	dex
	bpl		-
	rep		#$20

	lda		_sdCrc16_data,s
	sta		tcc__r9
	lda		_sdCrc16_data+2
	sta		tcc__r9h

	; for (i = 0; i < (len * 2); i++)
	ldy		#0
-:
	tya
	cmp		_sdCrc16_len,s
	bcc		+
	jmp		_sdCrc16_loop_end
	+:

	; poly = 0x0001000000100001LL;
    lda		#1
	sta		tcc__r0
  	sta		tcc__r0+6
	lda		#2
	sta		tcc__r0+2
	stz		tcc__r0+4
	
	phy							; save Y
	lsr		a
	tay
	sep		#$20
	lda		[tcc__r9],y
	ply							; restore Y
	sta		tcc__r5				; data[i >> 1]
	tya
	and		#1					; i & 1 ?
	bne		+
	lsr		a
	lsr		a
	lsr		a
	lsr		a
	+:
	and		#$0F
	sta		tcc__r5				; nybble = (i & 1) ? (data[i >> 1] & 0x0F) : (data[i >> 1] >> 4)

	; n_crc = (crc >> 60)
    lda		tcc__r2+7			; crc d63:56
	lsr		a
	lsr		a
	lsr		a
	lsr		a
	sta		tcc__r4

    ; crc <<= 4;
	rep		#$20
	asl		tcc__r2
	rol		tcc__r2+2
	rol		tcc__r2+4
	rol		tcc__r2+6
	asl		tcc__r2
	rol		tcc__r2+2
	rol		tcc__r2+4
	rol		tcc__r2+6
	asl		tcc__r2
	rol		tcc__r2+2
	rol		tcc__r2+4
	rol		tcc__r2+6
	asl		tcc__r2
	rol		tcc__r2+2
	rol		tcc__r2+4
	rol		tcc__r2+6

	lda		tcc__r5
	eor		tcc__r4				; nybble ^ n_crc
	sta		tcc__r5h
	ldx		#4
	--:	
	lda		tcc__r5h	
	and		#1
	beq		+
	; if ((nybble ^ n_crc) & (1<<i)) crc ^= (poly << i)
	lda		tcc__r2
	eor		tcc__r0
	sta		tcc__r2
	lda		tcc__r2+2
	eor		tcc__r0+2
	sta		tcc__r2+2
	lda		tcc__r2+4
	eor		tcc__r0+4
	sta		tcc__r2+4
	lda		tcc__r2+6
	eor		tcc__r0+6
	sta		tcc__r2+6
	+:
	lsr		tcc__r5h
	; poly <<= 1
	asl		tcc__r0
	rol		tcc__r0+2
	rol		tcc__r0+4
	rol		tcc__r0+6
	dex
	bne		--
	
	iny
	jmp		-
_sdCrc16_loop_end:

	; Output crc to array
	lda		_sdCrc16_p_crc,s
	sta		tcc__r9
	lda		_sdCrc16_p_crc+2,s
	sta		tcc__r9h
	ldy		#7
	ldx		#0
	sep		#$20				; 8-bit A
-:
	lda		tcc__r2,x
	inx
	sta		[tcc__r9],y
	dey
	bpl		-	

	rep		#$20				; 16-bit A

	; restore the TCC zeropage vars
    ldx		#12
-:
    lda.l	diskioRegBackup,x
	sta.b	tcc__r4,x
	dex
	bne		-

	ply
	plx
	rts


;**********************************************************************************************


; BOOL sendSdWriteBlock4( unsigned char *buf, unsigned char *crcbuf )
;
.EQU _sendSdWriteBlock4_save_regs 2
.EQU _sendSdWriteBlock4_buf 3+_sendSdWriteBlock4_save_regs
.EQU _sendSdWriteBlock4_crcbuf _sendSdWriteBlock4_buf+4
;
sendSdWriteBlock4:
	rep		#$30
	phx

	pea.w	$FF
	jsr		wrMmcDatByte4		; minimum 2 P bits
	pla
	pea.w	0
	jsr		wrMmcDatBit4		; write start bit
	pla

	; write out data and crc bytes
	lda		_sendSdWriteBlock4_buf,s
	sta		tcc__r1
	lda		_sendSdWriteBlock4_buf+2,s
	sta		tcc__r1h
	ldy		#0
-:
	lda		[tcc__r1],y
	iny
	and		#$FF
	pha
	jsr		wrMmcDatByte4		; wrMmcDatByte4( buf[i] )
	pla
	cpy		#512
	bne		-

	lda		_sendSdWriteBlock4_crcbuf,s
	sta		tcc__r1
	lda		_sendSdWriteBlock4_crcbuf+2,s
	sta		tcc__r1h
	ldy		#0
-:
	lda		[tcc__r1],y
	iny
	and		#$FF
	pha
	jsr		wrMmcDatByte4
	pla
	cpy		#8
	bne		-

	pea.w	15
	jsr		wrMmcDatBit4		; write end bit
	pla

	jsr		rdMmcDatByte4		; clock out two bits on D0

    ; spec says the CRC response from the card appears now...
    ;   start bit, three CRC response bits, end bit
    ; this is followed by a busy response while the block is written
    ;   start bit, variable number of 0 bits, end bit

	jsr		rdMmcDatBit4
	lda		tcc__r0
	and		#1
	beq		+
	plx
	stz		tcc__r0				; return FALSE
	rts

	stz		tcc__r2				; crc_stat
	ldx		#3
-:
	jsr		rdMmcDatBit4
	lsr		tcc__r0
	rol		tcc__r2				; crc_stat = (crc_stat << 1) | (rdMmcDatBit4() & 1)
	dex
	bne		-
	jsr		rdMmcDatBit4		; end bit

	; If CRC Status is not positive (010b) return error
	lda		tcc__r2
	cmp		#2
	beq		+
	plx
	stz		tcc__r0				; return FALSE
	rts
+:

  	; at this point the card is definetly cooperating so wait for the start bit
-:
	jsr		rdMmcDatBit4
	lda		tcc__r0
	and		#1
	bne		-

  	; Do not time out? Writes take an unpredictable amount of time to complete.
	ldx		#0
-:
	jsr		rdMmcDatBit4
	lda		tcc__r0
	and		#1
	bne		+
	dex
	bne		-
+:

	; check for busy timeout
	cpx		#0
	bne		+
	plx
	stz		tcc__r0				; return FALSE
	rts
+:

	pea.w	$FF
	jsr		wrMmcCmdByte
	pla

	plx
	lda		#1
	sta		tcc__r0				; return TRUE
	rts



;**********************************************************************************************


; BOOL sdWriteSingleBlock( unsigned char *buf, unsigned int addr )
;
.EQU _sdWriteSingleBlock_save_regs 2
.EQU _sdWriteSingleBlock_buf 3+_sdWriteSingleBlock_save_regs
.EQU _sdWriteSingleBlock_addr _sdWriteSingleBlock_buf+4
;
sdWriteSingleBlock:
	rep		#$30
	phx

	lda		#0
	sta.l	diskioCrcbuf
	sta.l	diskioCrcbuf+2
	sta.l	diskioCrcbuf+4
	sta.l	diskioCrcbuf+6
	pea.w	512
	lda		_sdWriteSingleBlock_buf+2
	pha
	lda		_sdWriteSingleBlock_buf+2
	pha
	pea.w	:diskioCrcbuf
	pea.w	diskioCrcbuf
	jsr		sdCrc16							; sdCrc16(diskioCrcbuf, buf, 512)
	pla
	pla
	pla
	pla
	pla

	lda		_sdWriteSingleBlock_addr+2,s	; addr bank
	pha
	lda		_sdWriteSingleBlock_addr+2,s	; addr offset (the last pha pushed it 2 bytes further away)
	pha
	pea.w	24				
	jsr		sendMmcCmd						; sendMmcCmd(24, addr)
	pla
	pla
	pla

	RECV_MMC_CMD_RESP diskioResp,R1_LEN,0
	lda		tcc__r0
	beq		+
	sep		#$20							
	lda.l	diskioResp
	cmp		#24								; diskioResp[0] == 24 ?
	rep		#$20
	bne		+

	pea.w	:diskioCrcbuf
	pea.w	diskioCrcbuf
	lda		_sdWriteSingleBlock_buf+2
	pha
	lda		_sdWriteSingleBlock_buf+2
	pha
	jsr		sendSdWriteBlock4				
	pla
	pla
	pla
	pla
	plx
	rts										; return sendSdWriteBlock4(buf, crcbuf)	
+:

	plx
	stz		tcc__r0							; return FALSE
	rts


;**********************************************************************************************


; BOOL sdInit(void)
;
sdInit:
	rep		#$30
	phx

	; Send 80 clks on. Initializes the mmc card into spi mode
	ldx		#80
	pea.w	1
-:
	jsr		wrMmcCmdBit					; wrMmcCmdBit(1)
	dex
	bne		-
	pla

	SEND_MMC_CMD GO_IDLE_STATE,0		
	pea.w	$FF
	jsr		wrMmcCmdByte
	pla

	SEND_MMC_CMD SEND_IF_COND,$1AA		; Check if the card can operate on the given voltage (2.7-3.6 V)

	RECV_MMC_CMD_RESP diskioResp,R7_LEN,1
	lda		tcc__r0
	beq		+
	sep		#$20						; 8-bit A
	lda.l	diskioResp
	cmp		#8
	bne		++
	lda.l	diskioResp+3
	cmp		#1							; if successul, the card should've echoed back the voltage and check pattern we specified in the command
	bne		++
	lda.l	diskioResp+4
	cmp		#$AA
	bne		++
	rep		#$20						; 16-bit A
	lda.l	cardType
	ora		#2
	sta.l	cardType					; V2 and/or HC card
	bra		+	
++:
	jmp		_sdInit_failed
+:

	ldx		#INIT_RETRIES
-:
	SEND_MMC_CMD APP_CMD,$FFFF				
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,1
	lda		tcc__r0
	beq		+
	lda.l	diskioResp+4
	and		#$20
	beq		+

	; sendMmcCmd(41, (cardType & 2) ? 0x40300000 : 0x00300000)
	stx		tcc__r1						; save X
	ldx		#$0030
	lda.l	cardType
	and		#2
	beq		++
	ldx		#$4030
++:
	phx
	ldx		tcc__r1						; restore X
	pea.w	0
	pea.w	SD_SEND_OP_COND
	jsr		sendMmcCmd					
	pla
	pla
	pla

	RECV_MMC_CMD_RESP diskioResp,R3_LEN,1
	lda		tcc__r0
	beq		++
	sep		#$20						; 8-bit A
	lda.l	diskioResp
	cmp		#$3F						; diskioResp[0] == 0x3F ?
	bne		++
	lda.l	diskioResp+1				
	bpl		++							; diskioResp[1] & 0x80 ?

	and		#$40						; diskioResp[1] & 0x40 ?
	beq		+++
	lda.l	cardType
	ora		#1
	sta.l	cardType					; HC card
+++:
	lda.l	diskioResp+2
	and		#$30
	bne		+++
	rep		#$20						; 16-bit A
	plx
	stz		tcc__r0						; if (!(resp[2] & 0x30)) return FALSE
	rts
+++:
	rep		#$20						; 16-bit A
	bra		_sdInit_loop_end
++:
	rep		#$20						; 16-bit A
+:	
	dex
	beq		_sdInit_loop_end
	jmp		-							; loop INIT_RETRIES times
_sdInit_loop_end:

	cpx		#0
	bne		+
	; timed out
	jmp		_sdInit_failed
+:

	SEND_MMC_CMD 2,$FFFFFFFF			; ALL_SEND_CID
	RECV_MMC_CMD_RESP diskioResp,R2_LEN,1
	lda		tcc__r0
	beq		+
	sep		#$20						; 8-bit A
	lda.l	diskioResp
	cmp		#$3F						; diskioResp[0] == 0x3F ?
	rep		#$20						; 16-bit A
	beq		++
+:
	jmp		_sdInit_failed
++:

	SEND_MMC_CMD 3,1					; SEND_RELATIVE_ADDR
	RECV_MMC_CMD_RESP diskioResp,R6_LEN,1
	lda		tcc__r0
	beq		+
	sep		#$20						; 8-bit A
	lda.l	diskioResp
	cmp		#3							; diskioResp[0] == 2 ?
	rep		#$20						; 16-bit A	
	beq		++
+:
	jmp		_sdInit_failed
++:

	; rca = (resp[1]<<8) | resp[2]
	lda.w	diskioResp+1
	xba
	sta.w	diskioTemp

;	lda.w	diskioTemp
	pha
	pea.w	$FFFF
	pea.w	9
	jsr		sendMmcCmd					; SEND_CSD
	pla
	pla
	pla	
	RECV_MMC_CMD_RESP sd_csd,R2_LEN,1	

	lda.w	diskioTemp
 	pha
	pea.w	$FFFF
	pea.w	7
	jsr		sendMmcCmd					; SELECT_DESELECT_CARD
	pla
	pla
	pla
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,1	
	lda		tcc__r0
	beq		+
	sep		#$20						; 8-bit A
	lda.l	diskioResp
	cmp		#7
	rep		#$20						; 16-bit A
	beq		++
+:
	jmp		_sdInit_failed
++:

	lda.w	diskioTemp
 	pha
	pea.w	$FFFF
	pea.w	APP_CMD
	jsr		sendMmcCmd					; APP_CMD
	pla
	pla
	pla
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,1	
	lda		tcc__r0
	beq		_sdInit_failed
	lda.l	diskioResp
	and		#$20	
	beq		_sdInit_failed

	SEND_MMC_CMD SET_BUS_WIDTH,2		; SET_BUS_WIDTH (to 4 bits)
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,1	
	lda		tcc__r0
	beq		_sdInit_failed
	sep		#$20
	lda.l	diskioResp
	cmp		#6
	rep		#$20
	bne		_sdInit_failed

	plx
	lda		#1
	sta		tcc__r0						; return TRUE
	rts

_sdInit_failed:
	rep		#$20
	jsr		neo2_disable_sd
	plx
	stz		tcc__r0						; return FALSE
	rts

;**********************************************************************************************


; DSTATUS disk_initialize (void)
disk_initialize_asm:
	php
	rep		#$30
	phx

	jsr		neo2_enable_sd

	; Invalidate all chache entries
	ldx		#0
	lda		#$FFFF
-:
	sta.l	sec_tags,x					; sec_tags[i] = 0xFFFFFFFF
	inx	
	inx
	sta.l	sec_tags,x					; ....
	inx
	inx
	cpx		#DISKIO_CACHE_SIZE*4
	bne		-
	sta.l	sec_last
	sta.l	sec_last+2
	
	lda.l	cardType
	and		#$8000						; keep funky flag
	sta.l	cardType
	
	jsr		neo2_pre_sd
	jsr		sdInit
	jsr		neo2_post_sd
	
	lda		tcc__r0						; the result from sdInit
	bne		+
	lda		#$FFFF
	sta.l	cardType
+:

	sep		#$20
	lda.l	sd_csd+1
	and		#$C0
	rep		#$20
	bne		+
    ; CSD type 1 - version 1.x cards, and version 2.x standard capacity
    ;
    ;    C_Size      - 12 bits - [73:62]
    ;    C_Size_Mult -  3 bits - [49:47]
    ;    Read_Bl_Len -  4 bits - [83:80]
    ;
    ;    Capacity (bytes) = (C_Size + 1) * ( 2 ^ (C_Size_Mult + 2)) * (2 ^ Read_Bl_Len)	
	
	sep		#$20	
	lda.l	sd_csd+9
	asl		a
	rol		a
	rol		a
	rep		#$20
	and		#3
	sta		tcc__r1						; (sd_csd[9] >> 6) & 3
	lda.l	sd_csd+8
	and		#$FF
	asl		a
	asl		a							; sd_csd[8] << 2
	ora		tcc__r1
	sta		tcc__r1
	lda.l	sd_csd+7
	and		#3
	xba
	asl		a
	asl		a							; (sd_csd[7] & 3) << 10
	ora		tcc__r1
	ina
	sta		tcc__r1						; C_Size+1

	sep		#$20
	lda.l	sd_csd+11
	asl		a
	rol		a								
	rep		#$20
	and		#1							; (sd_csd[11] >> 7) & 1
	sta		tcc__r2
	lda.l	sd_csd+10
	and		#3
	asl		a							; (sd_csd[10] & 3) << 1
	ora		tcc__r2
	sta		tcc__r2						; C_Size_Mult

	lda.l	sd_csd+6
	and		#$0F						; Read_Bl_Len
	clc
	adc		tcc__r2
	adc		#2								
	
	tax									; X = Read_Bl_Len + C_Size_Mult+2
	stz		tcc__r1h
	; tcc__r1 = (C_Size+1) << X
-:
	cpx		#0
	beq		+++
	asl		tcc__r1
	rol		tcc__r1h
	dex
	bne		-
+++:

	; /= 512 (>>= 9)
	lda		tcc__r1+1
	sta		tcc__r1
	lda		tcc__r1h
	xba
	and		#$FF
	lsr		a
	sta.l	num_sectors+2
	ror		tcc__r1
	lda		tcc__r1
	sta.l	num_sectors
	
	bra		++
	
	+:
    ; CSD type 2 - version 2.x high capacity
    ;
    ;    C_Size      - 22 bits - [69:48]
    ;
    ;    Capacity (bytes) = (C_Size + 1) * 1024 * 512
    sep		#$20
    lda.w	sd_csd+8
    and		#$3F
    sta.w	num_sectors+3				; store the values in bytes 1..3 of num_sectors instead of 0..2, i.e. the same as if the full value had been shifted left 8 bits
    lda.w	sd_csd+9
    sta.w	num_sectors+2
    lda.w	sd_csd+10
    sta.w	num_sectors+1
    lda		#0
    sta.w	num_sectors
	; we've already "shifted" the value left 8 bits. now we need to shift it 2 more bits to get the multiplication by 1024
    asl.w	num_sectors+1
    rol.w	num_sectors+2
    rol.w	num_sectors+3
    asl.w	num_sectors+1
    rol.w	num_sectors+2
    rol.w	num_sectors+3
    rep		#$20
++:   

	ldx		#0
	lda		tcc__r0
	bne		+
	ldx		#2							; STA_NODISK
+:
	stx		tcc__r0
	plx
	plp
	rtl
	
	
;**********************************************************************************************


; DSTATUS disk_status (void)
disk_status_asm:
	php
	rep		#$30
	stz		tcc__r0
	lda.l	cardType
	cmp		#$FFFF
	bne		+
	lda		#1				; STA_NOINIT
	sta		tcc__r0
+:
	plp
	rtl
	


;**********************************************************************************************


; DRESULT disk_read (
;     BYTE *buff,            /* Data buffer to store read data */
;     DWORD sector,        /* Sector address (LBA) */
;     BYTE count            /* Number of sectors to read (1..255) */
; )
;
.EQU _disk_read_save_regs 5
.EQU _disk_read_buff 4+_disk_read_save_regs
.EQU _disk_read_sector _disk_read_buff+4
.EQU _disk_read_count _disk_read_sector+4
;
disk_read_asm:
	php
	rep		#$30
	phx
	phy

	lda		_disk_read_count,s
	cmp		#1
	beq		+
	jmp		_disk_read_multiple_sectors
+:
	; count == 1
	lda		_disk_read_sector,s
	and		#DISKIO_CACHE_SIZE-1
	asl		a
	asl		a
	tax									; X = ix*4 (scaled for int array access)
	lda.l	sec_tags,x
	cmp		_disk_read_sector,s
	bne		_disk_read_fetch_single
	lda.l	sec_tags+2,x
	cmp		_disk_read_sector+2,s
	beq		_disk_read_copy_single
_disk_read_fetch_single:
	; sector not in cache - fetch it
	jsr		neo2_pre_sd

	jsr		_disk_read_calc_sector
	lda		tcc__r1h
	pha
	lda		tcc__r1
	pha
	txa
	xba
	and		#$FC00
	lsr		a								; the index was shifted left twice earlier, so after the XBA it's shifted left 10 bits. shift right once to get ix*512
	; push &sec_cache[ix*512]
	clc
	adc		#sec_cache
	sta		tcc__r1
	lda		#0
	adc		#:sec_cache
	pha
	lda		tcc__r1
	pha
	jsr		sdReadSingleBlock				; sdReadSingleBlock(&sec_cache[ix*512], sector << ((cardType & 1) ? 0 : 9))
	lda		tcc__r0
	bne		++
	; read failed, retry once
	jsr		sdReadSingleBlock
	lda		tcc__r0
	bne		++
	; read failed
	pla
	pla
	pla
	pla
	jsr		neo2_post_sd
	lda		#$FFFF
	sta.l	sec_tags,x						; sec_tags[ix] = 0xFFFFFFFF
	sta.l	sec_tags+2,x
	lda		#RES_ERROR
	sta		tcc__r0
	ply
	plx
	plp
	rtl
++:
	pla
	pla
	pla
	pla
	jsr		neo2_post_sd
	lda		_disk_read_sector,s
	sta.l	sec_tags,x
	lda		_disk_read_sector+2,s
	sta.l	sec_tags+2,x
_disk_read_copy_single:
	phb										; save DBR
	txa
	xba
	and		#$FC00
	lsr		a								; the index was shifted left twice earlier, so after the XBA it's shifted left 10 bits. shift right once to get ix*512
	clc
	adc		#sec_cache
	tax
	lda		#0
	adc		#:sec_cache
	sep		#$20
	pha
	plb										; DBR points out the bank of &sec_cache[ix*512], X contains the offset
	rep		#$20

	lda		_disk_read_buff,s
	sta.b	tcc__r1
	lda		_disk_read_buff+2,s
	sta.b	tcc__r1h
	ldy		#0
-:
	lda.w	$0000,x							; read 16 bits from sec_cache
	inx
	inx
	sta		[tcc__r1],y						; write 16 bits to buff
	iny
	iny
	cpy		#512
	bne		-
	plb										; restore DBR
	jmp		_disk_read_return
	
_disk_read_multiple_sectors:

	jsr		neo2_pre_sd
	jsr		_disk_read_calc_sector
	lda		tcc__r1h
	pha
	lda		tcc__r1
	pha
	jsr		sdReadStartMulti			; sdReadStartMulti(sector << ((cardType & 1) ? 0 : 9))
	lda		tcc__r0
	bne		+
	; read failed, retry once
	jsr		sdReadStartMulti			; sdReadStartMulti(sector << ((cardType & 1) ? 0 : 9))
	lda		tcc__r0
	bne		++
	; read failed
	pla
	pla
	jsr		neo2_post_sd
	lda		#RES_ERROR
	sta		tcc__r0
	ply
	plx
	plp
	rtl
++:
	pla
	pla
	lda		_disk_read_count,s
	pha
	lda		_disk_read_buff+4,s
	pha
	lda		_disk_read_buff+4,s
	pha
	jsr		neo2_recv_sd_multi			; neo2_recv_sd_multi(buff, count)
	lda		tcc__r0
	bne		++
	; read failed
	pla
	pla
	pla
	jsr		neo2_post_sd
	lda		#RES_ERROR
	sta		tcc__r0
	ply
	plx
	plp
	rtl
++:
	pla
	pla
	pla
	jmp		_disk_read_return
	
+:
	pla									; pop the arguments for sdReadStartMulti
	pla

	lda		_disk_read_count,s
	pha
	lda		_disk_read_buff+4,s
	pha
	lda		_disk_read_buff+4,s
	pha
	jsr		neo2_recv_sd_multi			; neo2_recv_sd_multi(buff, count)
	lda		tcc__r0
	bne		+
	jsr		_disk_read_calc_sector
	lda		tcc__r1h
	pha
	lda		tcc__r1
	pha
	jsr		sdReadStartMulti	
	lda		tcc__r0
	bne		++
	; read failed
	pla
	pla
	jsr		neo2_post_sd
	lda		#RES_ERROR
	sta		tcc__r0
	ply
	plx
	plp
	rtl
++:
	pla
	pla
	lda		_disk_read_count,s
	pha
	lda		_disk_read_buff+4,s
	pha
	lda		_disk_read_buff+4,s
	pha
	jsr		neo2_recv_sd_multi			; neo2_recv_sd_multi(buff, count)
	lda		tcc__r0
	bne		++
	; read failed
	pla
	pla
	pla
	jsr		neo2_post_sd
	lda		#RES_ERROR
	sta		tcc__r0
	ply
	plx
	plp
	rtl
++:
	pla
	pla
	pla
	jmp		_disk_read_return
+:
	pla
	pla
	pla

_disk_read_return:
	lda		#RES_OK
	sta		tcc__r0
	ply
	plx
	plp
	rtl

; Helper subroutine
_disk_read_calc_sector:
	lda		_disk_read_sector+2,s
	sta		tcc__r1
	lda		_disk_read_sector+4,s
	sta		tcc__r1h
	lda.l	cardType
	and		#1
	bne		+
	; sector << 9
	lda		tcc__r1+1
	sta		tcc__r1h
	lda		tcc__r1
	xba
	and		#$FF00
	asl		a
	sta		tcc__r1
	rol		tcc__r1h
+:
	rts


;**********************************************************************************************


; DRESULT disk_readp (
;     void* dest,         /* Pointer to the destination object */
;     DWORD sector,       /* Sector number (LBA) */
;     WORD sofs,          /* Offset in the sector (0..511) */
;     WORD count          /* Byte count (1..512), bit15:destination flag */
; )
;
.EQU _disk_readp_save_regs 9
.EQU _disk_readp_dest 4+_disk_readp_save_regs
.EQU _disk_readp_sector _disk_readp_dest+4
.EQU _disk_readp_sofs _disk_readp_sector+4
.EQU _disk_readp_count _disk_readp_sofs+2
;
disk_readp_asm:
	php
	rep		#$30
	phx
	phy
	lda		tcc__r9					; save tcc__r9/r9h
	pha
	lda		tcc__r9h
	pha

	lda		_disk_readp_count,s
	and		#$7FFF
	beq		+
	cmp		#513
	bcs		+
	bra		++
+:
	lda		#RES_PARERR
	jmp		_disk_readp_return
++:

	cmp		#41
	bcc		_disk_readp_small_read
    ; too big for anything but a file read, don't fetch to cache
	lda		_disk_readp_sector,s
	cmp.w	sec_last
	bne		+
	lda		_disk_readp_sector+2,s
	cmp.w	sec_last+2
	beq		++
+:
	; read sector
	jsr		neo2_pre_sd
	jsr		_disk_readp_calc_sector		
	lda		tcc__r1h
	pha
	lda		tcc__r1
	pha
	pea.w	:sec_buf
	pea.w	sec_buf
	jsr		sdReadSingleBlock			; sdReadSingleBlock(sec_buf, sector << ((cardType & 1) ? 0 : 9))
	lda		tcc__r0
	bne		+++
	; read failed, retry once
	jsr		sdReadSingleBlock
	lda		tcc__r0
	bne		+++
	; read failed
	pla
	pla
	pla
	pla
	jsr		neo2_post_sd
	lda		#$FFFF
	sta.l	sec_last
	sta.l	sec_last+2
	lda		#RES_ERROR
	jmp		_disk_readp_return
+++:
	pla
	pla
	pla
	pla
	jsr		neo2_post_sd
	lda		_disk_readp_sector,s
	sta.l	sec_last
	lda		_disk_readp_sector+2,s
	sta.l	sec_last+2
++:									; end if (sec_last != sector)

	lda		#sec_buf
	sta		tcc__r9
	lda		#:sec_buf
	sta		tcc__r9h				; buf = sec_buf
	jmp		_disk_readp_copy_data

_disk_readp_small_read:

	lda		_disk_readp_sector,s
	and		#DISKIO_CACHE_SIZE-1
	asl		a
	asl		a
	tax									; X = ix*4 (scaled for int array access)
	lda.l	sec_tags,x
	cmp		_disk_readp_sector,s
	bne		_disk_readp_fetch_single
	lda.l	sec_tags+2,x
	cmp		_disk_readp_sector+2,s
	beq		_disk_readp_fetch_done
_disk_readp_fetch_single:
	; sector not in cache - fetch it
	jsr		neo2_pre_sd

	jsr		_disk_readp_calc_sector
	lda		tcc__r1h
	pha
	lda		tcc__r1
	pha
	txa
	xba
	and		#$FC00
	lsr		a								; the index was shifted left twice earlier, so after the XBA it's shifted left 10 bits. shift right once to get ix*512
	; push &sec_cache[ix*512]
	clc
	adc		#sec_cache
	sta		tcc__r1
	lda		#0
	adc		#:sec_cache
	pha
	lda		tcc__r1
	pha
	jsr		sdReadSingleBlock				; sdReadSingleBlock(&sec_cache[ix*512], sector << ((cardType & 1) ? 0 : 9))
	lda		tcc__r0
	bne		++
	; read failed, retry once
	jsr		sdReadSingleBlock
	lda		tcc__r0
	bne		++
	; read failed
	pla
	pla
	pla
	pla
	jsr		neo2_post_sd
	lda		#$FFFF
	sta.l	sec_tags,x						; sec_tags[ix] = 0xFFFFFFFF
	sta.l	sec_tags+2,x
	lda		#RES_ERROR
	jmp		_disk_readp_return
++:
	pla
	pla
	pla
	pla
	jsr		neo2_post_sd
	lda		_disk_readp_sector,s
	sta.l	sec_tags,x
	lda		_disk_readp_sector+2,s
	sta.l	sec_tags+2,x
_disk_readp_fetch_done:
	; buf = &sec_cache[ix << 9]
	txa
	xba
	and		#$FC00
	lsr		a								
	clc
	adc		#sec_cache
	sta		tcc__r9
	lda		#0
	adc		#:sec_cache
	sta		tcc__r9h

; DEBUG
lda _disk_readp_count,s
sta.w diskioTemp+6

	
_disk_readp_copy_data:

	lda		_disk_readp_count,s
	tax
	bpl		+
	; streaming file mode
	; TODO: Handle this?
	lda		#RES_ERROR
	bra		_disk_readp_return
+:
	lda		_disk_readp_sofs,s
	clc
	adc		tcc__r9
	sta		tcc__r9
	lda		tcc__r9h
	adc		#0
	sta		tcc__r9h
	lda		_disk_readp_dest,s
	sta		tcc__r1
	lda		_disk_readp_dest+2,s
	sta		tcc__r1h
	ldy		#0
	sep		#$20
-:
	lda		[tcc__r9],y
	sta		[tcc__r1],y
	iny
	dex
	bne		-
	rep		#$20

	lda		#RES_OK
_disk_readp_return:
	sta		tcc__r0
	pla
	sta		tcc__r9h
	pla
	sta		tcc__r9
	ply
	plx
	plp
	rtl
	

; Helper subroutine
_disk_readp_calc_sector:
	lda		_disk_readp_sector+2,s
	sta		tcc__r1
	lda		_disk_readp_sector+4,s
	sta		tcc__r1h
	lda.l	cardType
	and		#1
	bne		+
	; sector << 9
	lda		tcc__r1+1
	sta		tcc__r1h
	lda		tcc__r1
	xba
	and		#$FF00
	asl		a
	sta		tcc__r1
	rol		tcc__r1h
+:
	rts

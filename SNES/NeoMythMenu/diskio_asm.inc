; SNES specific implementation of diskio (work in progress)
; /Mic


.EQU R1_LEN 6
.EQU R2_LEN 17
.EQU R3_LEN 6
.EQU R6_LEN 6
.EQU R7_LEN 6


.EQU INIT_RETRIES 64


; MMC commands
.EQU STOP_TRANSMISSION 12
.EQU READ_SINGLE_BLOCK 17
.EQU READ_MULTIPLE_BLOCK 18

.EQU DISKIO_CACHE_SIZE 16

;**********************************************************************************************



; void wrMmcCmdBit( unsigned int bit );
;
.EQU _wrMmcCmdBit_save_regs 2					; the total size of the registers being saved when entering the function (i.e. the PHX in this case, which equals 2 bytes of stack space)
.EQU _wrMmcCmdBit_bit 3+_wrMmcCmdBit_save_regs	; the stack-relative position of the 'bit' argument, after registers have been saved upon function entry
;
wrMmcCmdBit:
    rep    #$30    				; 16-bit regs
    phx
    lda    _wrMmcCmdBit_bit,s 	; read argument from stack
    and    #1
    beq    +
    lda    #$2000  				; set a.13 if (bit & 1) == 1
+:
    tax
    lda.l  $CFDE00,x  			; read either $CFDE00 or $CFFE00 depending on the value of bit & 1
    plx
    rts


;**********************************************************************************************


; void wrMmcDatBit( unsigned int bit )
;
.EQU _wrMmcDatBit_save_regs 2					
.EQU _wrMmcDatBit_bit 3+_wrMmcDatBit_save_regs
;	
wrMmcDatBit:
    rep    #$30    				; 16-bit regs
    phx
    lda    _wrMmcDatBit_bit,s 	; read argument from stack
    and    #1
    beq    +
    lda    #$200  				; set a.9 if (bit & 1) == 1
+:
    tax
    lda.l  $CFFC00,x  			; read either $CFFC00 or $CFFE00 depending on the value of bit & 1
    plx
    rts
    

;**********************************************************************************************


; void wrMmcDatBit4( unsigned char dat )
;
.EQU _wrMmcDatBit4_save_regs 2					
.EQU _wrMmcDatBit4_dat 3+_wrMmcDatBit4_save_regs	
;
wrMmcDatBit4:
    rep    #$30    				; 16-bit regs
    phx
    lda    _wrMmcDatBit4_dat,s  ; read argument from stack
    and    #15
    asl	   a
    xba							; A = (dat & 15) << 9
    tax
    lda.l  $CFE000,x  
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
    pha							; save A
    and    #1
    pha							; put argument on stack
    jsr    wrMmcCmdBit
    pla							; remove argument for wrMmcCmdBit
    pla							; restore A
    lsr    a
    dex
    bne    -
    rts
        	

;**********************************************************************************************


; void wrMmcDatByte( unsigned int byte )
wrMmcDatByte:
    rep    #$30    	; 16-bit regs
    phx
    lda    5,s    	; read argument from stack
    ldx    #8
-:
    pha
    and    #1
    pha
    jsr    wrMmcDatBit
    pla
    pla
    lsr    a
    dex
    bne    -
    rts


;**********************************************************************************************


; unsigned int rdMmcCmdBit()
rdMmcCmdBit:
    rep    #$30
    lda.l  $CFFE80
    lsr    a
    lsr    a
    lsr    a
    lsr    a
    and	   #1
    sta.b  tcc__r0
    rts


;**********************************************************************************************


; unsigned int rdMmcDatBit()
rdMmcDatBit:
    rep    #$30
    lda.l  $CF7C20
    and	   #1
    sta.b  tcc__r0
    rts


;**********************************************************************************************


; unsigned int rdMmcDatBit4()
rdMmcDatBit4:
    rep    #$30
    lda.l  $CE6060
    and	   #15
    sta.b  tcc__r0
    rts


;**********************************************************************************************


; unsigned char rdMmcCmdBits( int num )
;
.EQU _rdMmcCmdBits_save_regs 2
.EQU _rdMmcCmdBits_num 3+_rdMmcCmdBits_save_regs
;
rdMmcCmdBits:
    rep    #$30    	; 16-bit regs
    phx
    lda    _rdMmcCmdBits_num,s 
    tax
    lda    #0
-:
    cpx    #0
    bne    +
    pha
    jsr    rdMmcCmdBit
    pla
    asl    a
    ora    tcc__r0
    dex
    bra    -
+:
    sta    tcc__r0
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
    jsr    wrMmcDatBit4
    pla
    lda    _wrMmcDatByte4_val,s		
    and    #15
    pha
    jsr    wrMmcDatBit4
    pla
    rts    


;**********************************************************************************************


; unsigned char rdMmcDatByte4()
rdMmcDatByte4:
	rep	   #$30
	jsr    rdMmcDatBit4
	lda	   tcc__r0
	asl    a
	asl    a
	asl    a
	asl    a
	pha
	jsr    rdMmcDatBit4
	pla
	ora    tcc__r0
	sta    tcc__r0
	rts
	

;**********************************************************************************************


; unsigned char rdMmcDatByte()
rdMmcDatByte:
    rep    #$30    	; 16-bit regs
    phx
    ldx    #8
    lda    #0
-:
    pha
    jsr    rdMmcDatBit
    pla
    asl    a
    ora    tcc__r0
    dex
    bne    -
    sta    tcc__r0
    rts
    

;**********************************************************************************************


; unsigned char crc7 (unsigned char *buf)
;
.EQU _crc7_save_regs 4
.EQU _crc7_buf 3+_crc7_save_regs
;
crc7:
    rep    #$30
    phx
    phy
    lda    _crc7_buf,s	; buf offset
    sta    tcc__r1
    lda    _crc7_buf+2,s ; buf bank
    sta    tcc__r1h
    lda    #$8080
    sta    tcc__r2		; r4
    sta    tcc__r2h
    stz    tcc__r3		; c
    stz    tcc__r0		; crc
    ldx    #40
    ldy    #0
-:
    sep    #$20
    lda    tcc__r2		; if (r4 & 0x80) c = *buf++;
    bpl    +
    lda    [tcc__r1],y
    iny
+:
    asl    tcc__r0		; crc <<= 1

    lda    tcc__r0		; if (crc & 0x80) crc ^= 9;
    bpl    +
    eor    #9
    sta    tcc__r0
+:
    lda    tcc__r2h+1	; if (c & (r4>>24)) crc ^= 9;
    and    tcc__r3
    beq    +
    lda    tcc__r0
    eor    #9
    sta    tcc__r0
+:
    rep    #$20
    lda    tcc__r2		; r4 = (r4 >> 1) | (r4 << 31);
    lsr    a
    ror    tcc__r2h
    ror    tcc__r2
   
    dex
    bne    -
    rts    
 

;**********************************************************************************************



; void sendMmcCmd( unsigned char cmd, unsigned int arg )
;
.EQU _sendMmcCmd_save_regs 2
.EQU _sendMmcCmd_cmd 3+_sendMmcCmd_save_regs
.EQU _sendMmcCmd_arg _sendMmcCmd_cmd+2
;
sendMmcCmd:
    rep  	#$30    			; 16-bit regs
    phx
    
    sep   	#$20
    lda    	_sendMmcCmd_cmd,s	; cmd
    ora    	#$40				; b7 = 0 => start bit, b6 = 1 => host command
    sta.l	diskioPacket
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla
    
    sep   	#$20
    lda		_sendMmcCmd_arg+3,s	; arg >> 24
    sta		diskioPacket+1
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		_sendMmcCmd_arg+2,s	; arg >> 16
    sta		diskioPacket+2
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		_sendMmcCmd_arg+1,s	; arg >> 8
    sta		diskioPacket+3
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		_sendMmcCmd_arg,s	; arg 
    sta		diskioPacket+4
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
    ora		#1					; b0 = 1 => stop bit
    and		#$FF
    pha
    jsr		wrMmcCmdByte		; wrMmcCmdByte( crc7(pkt) << 1 ) | 1 )
    pla
    plx
    rts


.MACRO SEND_MMC_CMD
	pea.w	>\2
	pea.w	<\2
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
    rep  	#$30    				; 16-bit regs
    phx
    phy
    
    lda		_recvMmcCmdResp_resp,s	; resp offset
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
	sep		#$20
	lda		tcc__r0
	sta		[tcc__r1],y				; *r++ = rdMmcCmdBits(7);
	iny
	
	rep		#$20
	lda		_recvMmcCmdResp_len,s	; len
	dea
	phx
--:									; for(j=0; j<len; j++)
	cpx		#0
	beq		++
	jsr		rdMmcCmdByte
	sep		#$20
	lda		tcc__r0
	sta		[tcc__r1],y				; *r++ = rdMmcCmdByte();
	iny
	dex
	bra		--
++:
	plx
	
	rep		#$20
	lda		_recvMmcCmdResp_cflag,s	; cflag
	beq		++
	pea.w	$FF
	jsr		wrMmcCmdByte			; wrMmcCmdByte(0xFF);
	pla
++:

	lda		#1
	sta		tcc__r0					; return TRUE
	ply
	plx
	rts
+:
	dex
	bne		-
	
	stz		tcc__r0					; return FALSE
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

	lda		11,s			; addr bank
	pha
	lda		11,s			; addr offset (at 9,s. but the last pha pushed it 2 bytes further away)
	pha
	pea.w	READ_SINGLE_BLOCK				
	jsr		sendMmcCmd
	pla
	pla
	pla
	
	lda		cardType
	and		#$8000
	bne		+
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,0
	lda		tcc__r0
	beq		++
	sep		#$20
	lda.l	diskioResp
	cmp		#READ_SINGLE_BLOCK
	beq		+
++:
	plx
	rep		#$20
	stz		tcc__r0			; return FALSE
	rts
+:
	rep		#$20

	ldx		#8191	
-:							; while ((rdMmcDatBit4()&1) != 0)
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
	stz		tcc__r0			; if (i-- < 0) return FALSE (timeout on start bit)
	rts
+:

	lda		7,s				; buf bank
	pha
	lda		7,s				; buf offset
	pha
	jsr		neo2_recv_sd
	pla
	pla

	pea.w	$FF
	jsr		wrMmcCmdByte	; wrMmcCmdByte(0xFF);
	pla

	lda		#1
	sta		tcc__r0			; return TRUE
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

	lda		cardType
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
	stz		tcc__r0			; return FALSE
	rts
+:
	rep		#$20

	lda		#1
	sta		tcc__r0			; return TRUE
	plx
	rts

;**********************************************************************************************


; BOOL sdReadStopMulti( void )
sdReadStopMulti:
	rep		#$30
	phx
	SEND_MMC_CMD STOP_TRANSMISSION,0

	lda		cardType
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
.EQU _sdCrc16_poly tcc__r0
.EQU _sdCrc16_crc tcc__r2
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
	sta.b	_sdCrc16_crc,x
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
	sta		_sdCrc16_poly
  	sta		_sdCrc16_poly+6
	lda		#2
	sta		_sdCrc16_poly+2
	stz		_sdCrc16_poly+4
	
	phy
	lsr		a
	tay
	sep		#$20
	lda		[tcc__r9],y
	ply
	sta		tcc__r5				; data[i >> 1]
	tya
	and		#1
	bne		+
	lsr		a
	lsr		a
	lsr		a
	lsr		a
	+:
	and		#$0F
	sta		tcc__r5			; nybble = (i & 1) ? (data[i >> 1] & 0x0F) : (data[i >> 1] >> 4)

	; n_crc = (crc >> 60)
    lda		_sdCrc16_crc+7	; crc d63:56
	lsr		a
	lsr		a
	lsr		a
	lsr		a
	sta		_sdCrc16_n_crc

    ; crc <<= 4;
	rep		#$20
	asl		_sdCrc16_crc
	rol		_sdCrc16_crc+2
	rol		_sdCrc16_crc+4
	rol		_sdCrc16_crc+6
	asl		_sdCrc16_crc
	rol		_sdCrc16_crc+2
	rol		_sdCrc16_crc+4
	rol		_sdCrc16_crc+6
	asl		_sdCrc16_crc
	rol		_sdCrc16_crc+2
	rol		_sdCrc16_crc+4
	rol		_sdCrc16_crc+6
	asl		_sdCrc16_crc
	rol		_sdCrc16_crc+2
	rol		_sdCrc16_crc+4
	rol		_sdCrc16_crc+6

	lda		tcc__r5
	eor		_sdCrc16_n_crc	; nybble ^ n_crc
	sta		tcc__r5h
	ldx		#4
	--:	
	lda		tcc__r5h	
	and		#1
	beq		+
	; if ((nybble ^ n_crc) & (1<<i)) crc ^= (poly << i)
	lda		_sdCrc16_crc
	eor		_sdCrc16_poly
	sta		_sdCrc16_crc
	lda		_sdCrc16_crc+2
	eor		_sdCrc16_poly+2
	sta		_sdCrc16_crc+2
	lda		_sdCrc16_crc+4
	eor		_sdCrc16_poly+4
	sta		_sdCrc16_crc+4
	lda		_sdCrc16_crc+6
	eor		_sdCrc16_poly+6
	sta		_sdCrc16_crc+6
	+:
	lsr		tcc__r5h
	; poly <<= 1
	asl		_sdCrc16_poly
	rol		_sdCrc16_poly+2
	rol		_sdCrc16_poly+4
	rol		_sdCrc16_poly+6
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
	sep		#$20
-:
	lda		_sdCrc16_crc,x
	inx
	sta		[tcc__r9],y
	dey
	bpl		-	

	rep		#$20

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
.EQU _sendSdWriteBlock4_buf 3+_sendSdWriteBlock4
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
	jsr		sdCrc16
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
	jsr		sendMmcCmd
	pla
	pla
	pla

	RECV_MMC_CMD_RESP diskioResp,R1_LEN,0
	lda		tcc__r0
	beq		+
	sep		#$20
	lda.l	diskioResp
	cmp		#24
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
sdInit:
	rep		#$30
	phx

	ldx		#80
	pea.w	1
-:
	jsr		wrMmcCmdBit
	dex
	bne		-
	pla

	SEND_MMC_CMD 0,0					; GO_IDLE_STATE
	pea.w	$FF
	jsr		wrMmcCmdByte
	pla

	SEND_MMC_CMD 8,$1AA					; SEND_IF_COND

	RECV_MMC_CMD_RESP diskioResp,R7_LEN,1
	lda		tcc__r0
	beq		+
	sep		#$20
	lda		diskioResp
	cmp		#8
	bne		++
	lda		diskioResp+3
	cmp		#1
	bne		++
	lda		diskioResp+4
	cmp		#$AA
	bne		++
	rep		#$20
	lda		cardType
	ora		#2
	sta		cardType					; V2 and/or HC card
	bra		+	
++:
	rep		#$20
	plx
	stz		tcc__r0						; return FALSE
	rts
+:

	ldx		#INIT_RETRIES
-:
	SEND_MMC_CMD 55,$FFFF				; APP_CMD 
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,1
	lda		tcc__r0
	beq		+
	lda		diskioResp+4
	and		#$20
	beq		+

	; sendMmcCmd(41, (cardType & 2) ? 0x40300000 : 0x00300000)
	stx		tcc__r1						; save X
	ldx		#$0030
	lda		cardType
	and		#2
	beq		++
	ldx		#$4030
++:
	phx
	ldx		tcc__r1						; restore X
	pea.w	0
	pea.w	41
	jsr		sendMmcCmd					;  SD_SEND_OP_COND		
	pla
	pla
	pla

	RECV_MMC_CMD_RESP diskioResp,R3_LEN,1
	lda		tcc__r0
	beq		++
	sep		#$20
	lda		diskioResp
	cmp		#$3F
	bne		++
	lda		diskioResp+1
	bpl		++

	and		#$40
	beq		+++
	lda		cardType
	ora		#1
	sta		cardType					; HC card
+++:
	lda		diskioResp+2
	and		#$30
	bne		+++
	rep		#$20
	plx
	stz		tcc__r0						; if (!(resp[2] & 0x30)) return FALSE
	rts
+++:
	rep		#$20
	bra		_sdInit_loop_end
++:
	rep		#$20
+:	
	dex
	bne		-
_sdInit_loop_end:

	cpx		#0
	bne		+
	; timed out
	plx
	stz		tcc__r0						; return FALSE
	rts
+:

	SEND_MMC_CMD 2,$FFFFFFFF			; ALL_SEND_CID
	RECV_MMC_CMD_RESP diskioResp,R2_LEN,1
	lda		tcc__r0
	beq		+
	sep		#$20
	lda		diskioResp
	cmp		#$3F
	rep		#$20
	beq		++
+:
	plx
	stz		tcc__r0						; return FALSE
	rts
++:

	SEND_MMC_CMD 3,1					; SEND_RELATIVE_ADDR
	RECV_MMC_CMD_RESP diskioResp,R6_LEN,1
	lda		tcc__r0
	beq		+
	sep		#$20
	lda		diskioResp
	cmp		#3
	rep		#$20
	beq		++
+:
	plx
	stz		tcc__r0						; return FALSE
	rts
++:

	; rca = (resp[1]<<8) | resp[2]
	sep		#$20
	lda		diskioResp+2
	sta		tcc__r2
	lda		diskioResp+1
	sta		tcc__r2+1		
	rep		#$20

	lda		tcc__r2
	pha
	pea.w	$FFFF
	pea.w	9
	jsr		sendMmcCmd					; SEND_CSD
	pla
	pla
	pla	
	RECV_MMC_CMD_RESP sd_csd,R2_LEN,1	

	lda		tcc__r2
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
	sep		#$20
	lda		diskioResp
	cmp		#7
	rep		#$20
	beq		++
+:
	plx
	stz		tcc__r0						; return FALSE
	rts
++:

	lda		tcc__r2
 	pha
	pea.w	$FFFF
	pea.w	55
	jsr		sendMmcCmd					; APP_CMD
	pla
	pla
	pla
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,1	
	lda		tcc__r0
	beq		+
	lda		diskioResp
	and		#$20	
	bne		++
+:
	plx
	stz		tcc__r0						; return FALSE
	rts
++:

	SEND_MMC_CMD 6,2					; SET_BUS_WIDTH (to 4 bits)
	RECV_MMC_CMD_RESP diskioResp,R1_LEN,1	
	lda		tcc__r0
	beq		+
	sep		#$20
	lda		diskioResp
	cmp		#6
	rep		#$20
	beq		++
+:
	plx
	stz		tcc__r0						; return FALSE
	rts
++:

	plx
	lda		#1
	sta		tcc__r0						; return TRUE
	rts


;**********************************************************************************************


; DSTATUS disk_initialize (void)
disk_initialize_asm:
	php
	rep		#$30
	phx

	; Invalidate all chache entries
	ldx		#0
	lda		#$FFFF
-:
	sta.l	sec_tags,x
	inx
	inx
	sta.l	sec_tags,x
	inx
	inx
	cpx		#DISKIO_CACHE_SIZE*4
	bne		-
	sta		sec_last
	sta		sec_last+2
	
	lda		cardType
	and		#$8000							; keep funky flag
	sta		cardType
	
	jsr		neo2_pre_sd
	jsr		sdInit
	jsr		neo2_post_sd
	
	lda		tcc__r0							; the result from sdInit
	bne		+
	lda		#$FFFF
	sta		cardType
+:

	sep		#$20
	lda		sd_csd+1
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
	
	;TODO: calculate capacity
    ;    unsigned int C_Size      = ((sd_csd[7] & 0x03) << 10) | (sd_csd[8] << 2) | ((sd_csd[9] >>6) & 0x03);
    ;    unsigned int C_Size_Mult = ((sd_csd[10] & 0x03) << 1) | ((sd_csd[11] >> 7) & 0x01);
    ;    unsigned int Read_Bl_Len = sd_csd[6] & 0x0f;
    ;    num_sectors = ((C_Size + 1) * (1 << (C_Size_Mult + 2)) * ((1 << Read_Bl_Len)) / 512);
	
	bra		++
	
	+:
    ; CSD type 2 - version 2.x high capacity
    ;
    ;    C_Size      - 22 bits - [69:48]
    ;
    ;    Capacity (bytes) = (C_Size + 1) * 1024 * 512
    sep		#$20
    lda		sd_csd+8
    and		#$3F
    sta		num_sectors+3
    lda		sd_csd+9
    sta		num_sectors+2
    lda		sd_csd+10
    sta		num_sectors+1
    lda		#0
    sta		num_sectors
    asl		num_sectors+1
    rol		num_sectors+2
    rol		num_sectors+3
    asl		num_sectors+1
    rol		num_sectors+2
    rol		num_sectors+3
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
	lda		cardType
	cmp		#$FFFF
	bne		+
	lda		#1				; STA_NOINIT
	sta		tcc__r0
+:
	plp
	rtl
	




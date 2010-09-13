; SNES specific implementation of diskio (work in progress)
; /Mic


; void wrMmcCmdBit( unsigned int bit );
wrMmcCmdBit:
    rep    #$30    	; 16-bit regs
    phx
    lda    5,s    	; read argument from stack
    and    #1
    beq    +
    lda    #$2000  	; set a.13 if (bit & 1) == 1
+:
    tax
    lda.l  $CFDE00,x  ; read either $CFDE00 or $CFFE00 depending on the value of bit & 1
    plx
    rts


; void wrMmcDatBit( unsigned int bit )
wrMmcDatBit:
    rep    #$30    	; 16-bit regs
    phx
    lda    5,s    	; read argument from stack
    and    #1
    beq    +
    lda    #$200  	; set a.9 if (bit & 1) == 1
+:
    tax
    lda.l  $CFFC00,x  ; read either $CFFC00 or $CFFE00 depending on the value of bit & 1
    plx
    rts
    

; void wrMmcDatBit4( unsigned char dat )
wrMmcDatBit4:
    rep    #$30    	; 16-bit regs
    phx
    lda    5,s    	; read argument from stack
    and    #15
    asl	   a
    xba			; A = (dat & 15) << 9
    tax
    lda.l  $CFE000,x  
    plx
    rts



; void wrMmcCmdByte( unsigned int byte )
wrMmcCmdByte:
    rep    #$30    	; 16-bit regs
    phx
    lda    5,s    	; read argument from stack
    ldx    #8
-:
    pha			; save A
    and    #1
    pha			; put argument on stack
    jsr    wrMmcCmdBit
    pla			; remove argument for wrMmcCmdBit
    pla			; restore A
    lsr    a
    dex
    bne    -
    rts
        	


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


; unsigned int rdMmcDatBit()
rdMmcDatBit:
    rep    #$30
    lda.l  $CF7C20
    and	   #1
    sta.b  tcc__r0
    rts


; unsigned int rdMmcDatBit4()
rdMmcDatBit4:
    rep    #$30
    lda.l  $CE6060
    and	   #15
    sta.b  tcc__r0
    rts


; unsigned char rdMmcCmdBits( int num )
rdMmcCmdBits:
    rep    #$30    	; 16-bit regs
    phx
    lda    5,s    	; read argument from stack
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



; unsigned char rdMmcCmdByte()
rdMmcCmdByte:
    rep    #$30
    pea.w  8
    jsr    rdMmcCmdBits
    pla
    rts    	


; void wrMmcDatByte4( unsigned char val )
wrMmcDatByte4:
    rep    #$30
    lda    3,s    	; read argument from stack
    lsr    a
    lsr    a
    lsr    a
    lsr    a
    and    #15
    pha
    jsr    wrMmcDatBit4
    pla
    lda    3,s    	; read argument from stack
    and    #15
    pha
    jsr    wrMmcDatBit4
    pla
    rts    


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
    


; unsigned char crc7 (unsigned char *buf)
crc7:
    rep    #$30
    phx
    phy
    lda    7,s			; buf offset
    sta    tcc__r1
    lda    9,s			; buf bank
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
 


diskioPacket: .db 0,0,0,0,0,0,0


; void sendMmcCmd( unsigned char cmd, unsigned int arg )
sendMmcCmd:
    rep  	#$30    		; 16-bit regs
    phx
    
    sep   	#$20
    lda    	5,s				; cmd
    ora    	#$40			; b7 = 0 => start bit, b6 = 1 => host command
    sta.l	$7d0000+diskioPacket
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla
    
    sep   	#$20
    lda		10,s			; arg >> 24
    sta		$7d0000+diskioPacket+1
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		9,s				; arg >> 16
    sta		$7d0000+diskioPacket+2
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		8,s				; arg >> 8
    sta		$7d0000+diskioPacket+3
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    sep   	#$20
    lda		7,s				; arg 
    sta		$7d0000+diskioPacket+4
    rep		#$20
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla

    pea.w	$7e
    pea.w	diskioPacket
    jsr		crc7
    pla
    pla
    lda		tcc__r0
    asl		a
    ora		#1				; b0 = 1 => stop bit
    and		#$FF
    pha
    jsr		wrMmcCmdByte
    pla
    plx
    rts
    

; BOOL recvMmcCmdResp( unsigned char *resp, unsigned int len, int cflag )
recvMmcCmdResp:
    rep  	#$30    		; 16-bit regs
    phx
    phy
    
    lda		7,s				; resp offset
    sta		tcc__r1
    lda		9,s				; resp bank
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
	sta		[tcc__r1],y		; *r++ = rdMmcCmdBits(7);
	iny
	
	rep		#$20
	lda		11,s			; len
	dea
	phx
--:							; for(j=0; j<len; j++)
	cpx		#0
	beq		++
	jsr		rdMmcCmdByte
	sep		#$20
	lda		tcc__r0
	sta		[tcc__r1],y		; *r++ = rdMmcCmdByte();
	iny
	dex
	bra		--
++:
	plx
	
	rep		#$20
	lda		13,s			; cflag
	beq		++
	pea.w	$FF
	jsr		wrMmcCmdByte	; wrMmcCmdByte(0xFF);
	pla
++:

	lda		#1
	sta		tcc__r0			; return TRUE
	rts
+:
	dex
	bne		-
	
	stz		tcc__r0			; return FALSE
	rts
	
    


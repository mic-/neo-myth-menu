; Sega Master System implementation of diskio
; /Mic, 2011


.area   _CODE

; MMC response lengths
R1_LEN = 6
R2_LEN = 17
R3_LEN = 6
R6_LEN = 6
R7_LEN = 6

; PFF Constants
RES_OK = 0
RES_ERROR = 1
RES_WRPRT = 2
RES_NOTRDY = 3
RES_PARERR = 4

INIT_RETRIES = 64

; MMC commands
GO_IDLE_STATE = 0
SET_BUS_WIDTH = 6
SEND_IF_COND = 8
STOP_TRANSMISSION = 12
READ_SINGLE_BLOCK = 17
READ_MULTIPLE_BLOCK = 18
WRITE_SINGLE_BLOCK = 24
SD_SEND_OP_COND = 41
APP_CMD = 55

; Number of entries in the sector cache. 
DISKIO_CACHE_SIZE = 1

MYTH_NEO2_WR_CMD1_CLR13 = 0x5E00    ; FlashBankLo=0x87,Frame1=7
MYTH_NEO2_WR_CMD1_SET13 = 0x7E00    ; 0x87,7
MYTH_NEO2_WR_DAT1_CLR9 = 0x7C00     ; 0x87,7
MYTH_NEO2_WR_DAT1_SET9 = 0x7E00     ; 0x87,7
MYTH_NEO2_WR_DAT4 = 0x6000          ; 0x87,7

MYTH_NEO2_RD_CMD1 = 0x7E81          ; 0x87,7
MYTH_NEO2_RD_DAT1 = 0x7C21          ; 0x87,5
MYTH_NEO2_RD_DAT4 = 0x6061          ; 0x87,1


;**********************************************************************************************

; External variables (hardcoded)

_vregs      = 0xC006
_cardType   = 0xC016
_diskioPacket = 0xC27D                
_diskioResp = 0xC284                   
_diskioTemp = 0xC295                  
_sd_csd     = 0xC29D                       
_sec_tags   = 0xC2AE                      
_sec_last   = 0xC2B6                      
_numSectors = 0xC2BA  

.globl _neo2_ram_to_psram

; Exported functions
.globl _disk_initialize2
.globl _disk_readp2
.globl _disk_read_sector2
.globl _disk_read_sectors2
.globl _sdWriteSingleBlock
.globl neo2_enable_sd
.globl neo2_disable_sd
.globl neo2_pre_sd
.globl neo2_post_sd

;**********************************************************************************************



; void wrMmcCmdBit( unsigned int bit );
;
;   A = bit
wrMmcCmdBit:
        and a,#1
        jr  z,1$
        ld  a,(MYTH_NEO2_WR_CMD1_SET13)
        ret
1$:
        ld  a,(MYTH_NEO2_WR_CMD1_CLR13)
        ret


;**********************************************************************************************


; void wrMmcDatBit( unsigned int bit )
;
;   A = bit
wrMmcDatBit:
        and a,#1
        jr  z,1$
        ld  a,(MYTH_NEO2_WR_DAT1_SET9)
        ret
1$:
        ld  a,(MYTH_NEO2_WR_DAT1_CLR9)
        ret


;**********************************************************************************************


; void wrMmcDatBit4( unsigned char dat )
;
;   A = dat
wrMmcDatBit4:
        and     a,#15
        ld      h,a
        sla     h
        ld      l,#0
        ld      bc,#MYTH_NEO2_WR_DAT4
        add     hl,bc               ; HL = MYTH_NEO2_WR_DAT4 + ((dat & 15) << 9)
        ld      a,(hl)
        ret



;**********************************************************************************************


; void wrMmcCmdByte( unsigned int byte )
;
;   A = byte
wrMmcCmdByte:
        push    bc
        ld      b,#8
1$:
        rlc     a               ; cf = a.7, a = [a << 1] + cf
        ld      c,a
        call    wrMmcCmdBit     ; write  (byte >> (7-i)) & 1
        ld      a,c
        djnz    1$
        pop     bc
        ret


;**********************************************************************************************


; void wrMmcDatByte( unsigned int byte )
;
;   A = byte
wrMmcDatByte:
        ld      b,#8
1$:
        rlc     a
        ld      c,a
        call    wrMmcDatBit         ; write  (byte >> (7-i)) & 1
        ld      a,c
        djnz    1$
        ret

;**********************************************************************************************


; unsigned int rdMmcCmdBit()
;
;rdMmcCmdBit:
;        ld      a,(MYTH_NEO2_RD_CMD1)  
;        srl     a
;        srl     a
;        srl     a
;        srl     a
;        and     a,#1
;        ret


;**********************************************************************************************


; unsigned int rdMmcDatBit()
;
;rdMmcDatBit:
;   ld  a,(MYTH_NEO2_RD_DAT1)
;   and a,#1
;   ret


;**********************************************************************************************


; unsigned int rdMmcDatBit4()
;
rdMmcDatBit4:
        ld      a,(MYTH_NEO2_RD_DAT4)
        and     a,#15
        ret


;**********************************************************************************************


; unsigned char rdMmcCmdBits( int num )
;
;  A = num
rdMmcCmdBits:
        push    bc						;11
		push	hl						;11
        ld      b,a						;4
		ld		c,#0x01					;7
		xor		a						;4
		ld		hl,#MYTH_NEO2_RD_CMD1	;10
0$:		
		rla								;4
		bit		4,(hl)					;12
		jp		z,1$					;10
		or		a,c						;4
1$:
		djnz    0$						;13
2$:
		pop		hl						;10
        pop     bc						;10
        ret								;10

;**********************************************************************************************


; unsigned char rdMmcCmdByte()
rdMmcCmdByte:
        ld    a,#8
        call  rdMmcCmdBits
        ret

;**********************************************************************************************


; void wrMmcDatByte4( unsigned char val )
;
;  A = val
wrMmcDatByte4:
        push  af
        srl   a
        srl   a
        srl   a
        srl   a
        call  wrMmcDatBit4  ; write high nybble
        pop   af
        call  wrMmcDatBit4  ; write low nybble
        ret


;**********************************************************************************************


; unsigned char rdMmcDatByte4()
;
rdMmcDatByte4:
        call  rdMmcDatBit4
        sla   a
        sla   a
        sla   a
        sla   a         ; a = rdMmcDatBit4() << 4
        ld    b,a
        call  rdMmcDatBit4
        or    a,b       ; a |= rdMmcDatBit4()
        ret


;**********************************************************************************************


; unsigned char rdMmcDatByte()
;
rdMmcDatByte:
        ld      a,#5
        ld      (0xFFFE),a    ; Frame1=5
        ld      b,#8
        ld      c,#0
1$:
        ld      a,(MYTH_NEO2_RD_DAT1)
        and     a,#1 ;call  rdMmcDatBit
        srl     a
        rl      c       ; c = (c << 1) | rdMmcDatBit()
        djnz    1$
        ld      a,#7
        ld      (0xFFFE),a    ; Frame1=7
        ld      a,c
        ret


;**********************************************************************************************

; unsigned char crc7 (unsigned char *buf)
;
; In:
;   DE = buf
; Out:
;   A = crc
crc7:
        ld      a,#0x80         ; r0 = 0x80808080
        ld      iy,#_vregs
        ld      h,a
        ld      1(iy),a
        ld      2(iy),a
        ld      3(iy),a

        ld      b,#40
        ld      a,#0
        ld      4(iy),a         ; x = 0
        ld      c,a             ; crc = 0
1$:
        bit     7,h             ; if (r0 & 0x80) x = *buf++;
        jr      z,2$
        ld      a,(de)
        inc     de
        ld      (_vregs+4),a
2$:
        sla     c               ; crc <<= 1
          
        jp      p,3$            ; if (crc & 0x80) crc ^= 9;
        ld      a,#9
        xor     a,c
        ld      c,a
3$:

        ld      a,(_vregs+3)    ; if (x & (r0>>24)) crc ^= 9;
        ld      l,a
        ld      a,(_vregs+4)
        and     a,l
        jr      z,4$
        ld      a,#9
        xor     a,c
        ld      c,a
4$:
        ; r0 = (r0 >> 1) | (r0 << 31);
        ld      a,h
        srl     a
        rr      3(iy)
        rr      2(iy)
        rr      1(iy)
        rr      h

        djnz    1$
        ld      a,c
        ret

;**********************************************************************************************



; void sendMmcCmd( unsigned char cmd, unsigned int arg )
;
; In:
;   A = cmd
;   DE:BC = arg
sendMmcCmd:
		push	hl	
		ld		hl,#_diskioPacket

        or      a,#0x40         ; b7 = 0 => start bit, b6 = 1 => host command
        ld      (hl),a		
        call    wrMmcCmdByte
		inc		l

        ld      a,d             ; arg >> 24
        ld      (hl),a
        call    wrMmcCmdByte
		inc		l

        ld      a,e             ; arg >> 16
        ld      (hl),a
        call    wrMmcCmdByte
		inc		l

        ld      a,b             ; arg >> 8
        ld      (hl),a
        call    wrMmcCmdByte
		inc		l

        ld      a,c             ; arg
        ld      (hl),a
        call    wrMmcCmdByte

		ld		de,#_diskioPacket
        call    crc7
        sla     a
        or      a,#1            ; b0 = 1 => stop bit
    ; -- DEBUG --
		ld		l,#0x82
        ld      (hl),a
    ; -----------
        call    wrMmcCmdByte    ; wrMmcCmdByte( (crc7(diskioPacket) << 1) | 1 )
		pop		hl
        ret

;**********************************************************************************************

; BOOL recvMmcCmdResp( unsigned char *resp, unsigned int len, int cflag )
;
; In:
;   DE = buf
;   B = len
;   C = cflag
; Out:
;   A = 0 (failure) or 1 (success)
recvMmcCmdResp:
		push	bc
		ld		hl,#MYTH_NEO2_RD_CMD1

		;timeout 256x4
		ld		b,#0
		ld		c,#4
0$:
		bit		4,(hl)
		jp		z,1$
		djnz	0$
		ld		b,c
		dec		c
		jr		nz,0$
		pop		bc
		xor		a
		ret
1$:
		pop		bc
		ld		a,#0x07
		call	rdMmcCmdBits
		ld		(de),a
		inc		de
		dec		b
		ld		a,b
		or		a
		jr		z,3$
2$:
		call    rdMmcCmdByte
	  	ld      (de),a
		inc     de
		djnz    2$

		ld		a,c
		dec		a				; 1->0 , 0->ff
		jr		nz,3$
		cpl						; a = ~a
		call    wrMmcCmdByte	; wrMmcCmdByte(0xff)
3$:
		ld		a,#0x01
		ret


;**********************************************************************************************

; BOOL sdReadSingleBlock( unsigned char *buf, unsigned int addr )
;
; In:
;   HL = buf
;   DE:BC = addr
; Out:
;   A = 0 (failure) or 1 (success)
sdReadSingleBlock:
        push    hl

        ld      a,#READ_SINGLE_BLOCK
        call    sendMmcCmd

        ; VERIFY_DISK_RESP READ_SINGLE_BLOCK,R1_LEN,0
        ld      a,(_cardType+1)
        ; cardType & 0x8000 ?
        and     a,#0x80
        jr      nz,1$
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#0
        call    recvMmcCmdResp
        and     a,a
        jr      z,2$
        ld      a,(_diskioResp+0)
        cp      a,#READ_SINGLE_BLOCK
        jr      z,1$
2$:
        pop hl
        xor		a;ld      a,#0            ; return FALSE
        ret
1$:

        ld      de,#8192
3$:
        ld      a,#1
        ld      (0xFFFE),a     ; Frame1 = 1
                               ; while ((rdMmcDatBit4()&1) != 0)
        call    rdMmcDatBit4
        and     a,#1
        jr      z,4$
        dec     de
        ld      a,d
        or      a,e
        jr      nz,3$

        pop hl
        xor		a;ld      a,#0            ; return FALSE (timeout on start bit)
        ret
4$:

        pop hl
        call    neo2_recv_sd

        ld      a,#7
        ld      (0xFFFE),a      ; Frame1 = 7
        ld      a,#0xFF
        call    wrMmcCmdByte    ; wrMmcCmdByte(0xFF);

        ld      a,#1            ; return TRUE
        ret


;**********************************************************************************************


; BOOL sdReadStartMulti( unsigned int addr )
;
; In:
;   DE:BC = addr
; Out:
;   A = 0 (failure) or 1 (success)
sdReadStartMulti:
        ld      a,#READ_MULTIPLE_BLOCK
        call    sendMmcCmd

        ; VERIFY_DISK_RESP READ_MULTIPLE_BLOCK,R1_LEN,0
        ld      a,(_cardType+1)
        ; cardType & 0x8000 ?
        and     a,#0x80
        jr		nz,1$
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#0
        call    recvMmcCmdResp
        and     a,a
        jr      z,2$
        ld      a,(_diskioResp+0)
        cp      a,#READ_MULTIPLE_BLOCK
        jr      z,1$
2$:
        xor		a;ld      a,#0            ; return FALSE
        ret
1$:

        ld      a,#1        ; return TRUE
        ret

;**********************************************************************************************


; BOOL sdReadStopMulti( void )
;
; Out:
;   A = 0 (failure) or 1 (success)
sdReadStopMulti:
        ;ld      bc,#0
        ;ld      de,#0
		ld		b,#0x00
		ld		c,b
		ld		d,b
		ld		e,b
        ld      a,#STOP_TRANSMISSION
        call    sendMmcCmd

        ; VERIFY_DISK_RESP STOP_TRANSMISSION,R1_LEN,0
        ld      a,(_cardType+1)
        ; cardType & 0x8000 ?
        and     a,#0x80
        jr      nz,1$
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#0
        call    recvMmcCmdResp
        and     a,a
        jr      z,2$
        ld      a,(_diskioResp+0)
        cp      a,#STOP_TRANSMISSION
        jr      z,1$
2$:
        xor		a;ld      a,#0            ; return FALSE
        ret
1$:

        ld      a,#1        ; return TRUE
        ret

;**********************************************************************************************


; BOOL sdInit(void)
;
; Out:
;   A = 0 (failure) or 1 (success)
sdInit:
        ; Send 80 clks on to initialize the card
        ld      b,#80
1$:
        ld      a,#1
        call    wrMmcCmdBit             ; wrMmcCmdBit(1)
        djnz    1$
          
        ;ld      bc,#0
        ;ld      de,#0
		ld		b,#0x00
		ld		c,b
		ld		d,b
		ld		e,b

        ld      a,#GO_IDLE_STATE
        call    sendMmcCmd
        
        ld      a,#0xFF
        call    wrMmcCmdByte

        ; Check if the card can operate on the given voltage (2.7-3.6 V)
        ld      bc,#0x01AA
        ld      de,#0x0000
        ld      a,#SEND_IF_COND
        call    sendMmcCmd

        ; RECV_MMC_CMD_RESP _diskioResp,R7_LEN,1
        ld      de,#_diskioResp
        ld      b,#R7_LEN
        ld      c,#1
        call    recvMmcCmdResp
        
        and     a,a
        jr      z,2$
        ld      a,(_diskioResp+0)
        cp      a,#8
        jr      nz,3$
        ; if successul, the card should have echoed back the voltage and check pattern we specified in the command
        ld      a,(_diskioResp+3)
        cp      a,#1
        jr      nz,3$
        ld      a,(_diskioResp+4)
        cp      a,#0xAA
        jr      nz,3$
        ld      a,(_cardType)
        or      a,#2
        ld      (_cardType),a           ; V2 and/or HC card
        jr      2$
3$:
        jp      sdInit_failed
2$:

        ld      b,#INIT_RETRIES
4$:
        push    bc

        ld      bc,#0xFFFF
        ld      de,#0
        ld      a,#APP_CMD
        call    sendMmcCmd

        ; RECV_MMC_CMD_RESP _diskioResp,R1_LEN,1
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#1
        call    recvMmcCmdResp

        and     a,a
        jr      z,5$
        ld      a,(_diskioResp+4)
        and     a,#0x20
        jr      z,5$

        ; sendMmcCmd(41, (cardType & 2) ? 0x40300000 : 0x00300000)
        ld      bc,#0x0000
        ld      e,#0x30
        ld      d,#0x00
        ld      a,(_cardType)
        and     a,#2
        jr      z,6$
        ld      d,#0x40
6$:
        ld      a,#SD_SEND_OP_COND
        call    sendMmcCmd

        ; RECV_MMC_CMD_RESP _diskioResp,R3_LEN,1
        ld      de,#_diskioResp
        ld      b,#R3_LEN
        ld      c,#1
        call    recvMmcCmdResp

        and     a,a
        jr      z,7$
        ld      a,(_diskioResp+0)
        cp      a,#0x3F             ; diskioResp[0] == 0x3F ?
        jr      nz,7$
        ld      a,(_diskioResp+1)
        bit     7,a                ; diskioResp[1] & 0x80 ?
        jr      z,7$
        bit     6,a                ; diskioResp[1] & 0x40 ?
        jr      z,8$
        ld      a,(_cardType)
        or      a,#1                ; HC card
        ld      (_cardType),a
8$:
        ld      a,(_diskioResp+2)
        and     a,#0x30
        jr      nz,9$
        pop     bc
        xor		a;ld      a,#0                ; if (!(diskioResp[2] & 0x30)) return FALSE
        ret
9$:
        pop     bc
        jr      sdInit_loop_end
7$:
5$:

        pop     bc
        dec     b
        jr      nz,4$
sdInit_loop_end:

        ld      a,b
        cp      a,#0
        jp      z,sdInit_failed     ; timed out

        ; ALL_SEND_CID
        ;ld      bc,#0xFFFF
        ;ld      de,#0xFFFF
		ld		b,#0xFF
		ld		c,b
		ld		d,b
		ld		e,b
        ld      a,#2
        call    sendMmcCmd

        ; RECV_MMC_CMD_RESP _diskioResp,R2_LEN,1
        ld      de,#_diskioResp
        ld      b,#R2_LEN
        ld      c,#1
        call    recvMmcCmdResp
        and     a,a
        jp      z,sdInit_failed
        ld      a,(_diskioResp+0)
        cp      a,#0x3F             ; diskioResp[0] == 0x3F ?
        jp      nz,sdInit_failed

        ; SEND_RELATIVE_ADDR
        ld      bc,#1
        ld      de,#0
        ld      a,#3
        call    sendMmcCmd

        ; RECV_MMC_CMD_RESP _diskioResp,R6_LEN,1
        ld      de,#_diskioResp
        ld      b,#R6_LEN
        ld      c,#1
        call    recvMmcCmdResp
        and     a,a
        jp      z,sdInit_failed
        ld      a,(_diskioResp+0)
        cp      a,#3                ; diskioResp[0] == 3 ?
        jp      nz,sdInit_failed

        ; rca = (resp[1]<<8) | resp[2]
        ld      a,(_diskioResp+2)
        ld      (_diskioTemp+0),a
        ld      a,(_diskioResp+1)
        ld      (_diskioTemp+1),a

        ld      a,(_diskioTemp+0)
        ld      e,a
        ld      a,(_diskioTemp+1)
        ld      d,a
        ld      bc,#0xFFFF
        ld      a,#9               ; SEND_CSD
        call    sendMmcCmd
        ; RECV_MMC_CMD_RESP _sd_csd,R2_LEN,1
        ld      de,#_sd_csd
        ld      b,#R2_LEN
        ld      c,#1
        call    recvMmcCmdResp

        ld      a,(_diskioTemp+0)
        ld      e,a
        ld      a,(_diskioTemp+1)
        ld      d,a
        ld      bc,#0xFFFF
        ld      a,#7               ; SELECT_DESELECT_CARD
        call    sendMmcCmd

        ; RECV_MMC_CMD_RESP _diskioResp,R1_LEN,1
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#1
        call    recvMmcCmdResp
        and     a,a
        jr      z,sdInit_failed
        ld      a,(_diskioResp+0)
        cp      a,#7
        jr      nz,sdInit_failed

        ld      a,(_diskioTemp+0)
        ld      e,a
        ld      a,(_diskioTemp+1)
        ld      d,a
        ld      bc,#0xFFFF
        ld      a,#APP_CMD
        call    sendMmcCmd

        ; RECV_MMC_CMD_RESP _diskioResp,R1_LEN,1
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#1
        call    recvMmcCmdResp
        and     a,a
        jr      z,sdInit_failed
        ld      a,(_diskioResp+0)
        and     #0x20
        jr      z,sdInit_failed

        ; SET_BUS_WIDTH (to 4 bits)
        ld      bc,#2
        ld      de,#0
        ld      a,#SET_BUS_WIDTH
        call    sendMmcCmd

        ; RECV_MMC_CMD_RESP _diskioResp,R1_LEN,1
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#1
        call    recvMmcCmdResp
        and     a,a
        jr      z,sdInit_failed
        ld      a,(_diskioResp+0)
        cp      a,#6
        jr      nz,sdInit_failed

        ld      a,#1               ; return TRUE
        ret

sdInit_failed:
        call    neo2_disable_sd
        xor		a               ; return FALSE
        ret


;**********************************************************************************************


; DSTATUS disk_initialize (void)
_disk_initialize2:
        push    af
        push    bc
        push    de
        push    ix

        call    neo2_enable_sd

        ; Invalidate all cache entries (sec_tags[i] = 0xFFFFFFFF)
        ld      a,#0xFF
        ld      b,#DISKIO_CACHE_SIZE*4
        ld      de,#_sec_tags
1$:
        ld      (de),a
        inc     de
        djnz    1$

        ld      (_sec_last+0),a
        ld      (_sec_last+1),a
        ld      (_sec_last+2),a
        ld      (_sec_last+3),a

        ld      a,(_cardType+1)
        and     a,#0x80
        ld      (_cardType+1),a         ; keep funky flag

        call    neo2_pre_sd
      
        call    sdInit
        and     a,a                      ; if (!sdInit()) cardType = 0xFFFF
        jr      nz,2$
        ld      a,#0xFF
        ld      (_cardType+0),a
        ld      (_cardType+1),a
2$:
        call    neo2_post_sd
        
        ld      a,(_sd_csd+1)
        and     a,#0xC0
        jr      nz,3$

        ; CSD type 1 - version 1.x cards, and version 2.x standard capacity
        ;
        ;    C_Size      - 12 bits - [73:62]
        ;    C_Size_Mult -  3 bits - [49:47]
        ;    Read_Bl_Len -  4 bits - [83:80]
        ;
        ;    Capacity (bytes) = (C_Size + 1) * ( 2 ^ (C_Size_Mult + 2)) * (2 ^ Read_Bl_Len)

        ld      a,(_sd_csd+9)
        rlc     a
        rlc     a
        and     a,#3
        ld      c,a                 ; (sd_csd[9] >> 6) & 3
        ld      a,(_sd_csd+8)
        ld      b,#0
        sla     a
        rl      b
        sla     a
        rl      b                   ; sd_csd[8] << 2
        or      a,c
        ld      c,a
        ld      a,(_sd_csd+7)
        and     a,#3
        or      a,b
        ld      b,a
        inc     bc                  ; bc = C_Size+1

        ld      a,(_sd_csd+11)
        rlc     a
        and     a,#1                ; (sd_csd[11] >> 7) & 1
        ld      e,a
        ld      a,(_sd_csd+10)
        and     a,#3
        sla     a                   ; (sd_csd[10] & 3) << 1
        or      a,e
        ld      e,a                 ; e = C_Size_Mult

        ld      a,(_sd_csd+6)
        and     a,#0x0F
        add     a,e                 ; Read_Bl_Len
        add     a,#2                ; a = Read_Bl_Len + C_Size_Mult + 2

        ; de:bc <<= a
        ld      de,#0
4$:
        cp      a,#0
        jr      z,5$
        sla     c
        rl      b
        rl      e
        rl      d
        dec     a
        jr      4$
5$:

        ; de:bc /= 512
        ld      c,b
        ld      b,e
        ld      e,d
        srl     e
        rr      b
        rr      c

        ld      a,c
        ld      (_numSectors+0),a
        ld      a,b
        ld      (_numSectors+1),a
        ld      a,e
        ld      (_numSectors+2),a
        ld      a,#0
        ld      (_numSectors+3),a

        jr      6$
3$:
        ; CSD type 2 - version 2.x high capacity
        ;
        ;    C_Size      - 22 bits - [69:48]
        ;
        ;    Capacity (bytes) = (C_Size + 1) * 1024 * 512
        ld      a,(_sd_csd+8)
        and     a,#0x3F
        ld      (_numSectors+3),a       ; store the values in bytes 1..3 of num_sectors instead of 0..2, i.e. the same as if the full value had been shifted left 8 bits
        ld      a,(_sd_csd+9)
        ld      (_numSectors+2),a
        ld      a,(_sd_csd+10)
        ld      (_numSectors+1),a
        ld      a,#0
        ld      (_numSectors+0),a

        ; we've already "shifted" the value left 8 bits. now we need to shift it 2 more bits to get the multiplication by 1024
        ld      ix,#_numSectors
        sla     1(ix)
        rl      2(ix)
        rl      3(ix)
        sla     1(ix)
        rl      2(ix)
        rl      3(ix)
6$:

        ld      hl,#0           ; NO_ERROR
        ld      a,(_cardType+1)
        cp      #0xFF
        jr      nz,7$
        ld      l,#2            ; STA_NODISK
7$:
        pop     ix
        pop     de
        pop     bc
        pop     af
        ret


;**********************************************************************************************


; DSTATUS disk_status (void)
_disk_status:
        ld      l,#0
        ld      a,(_cardType+0)
        ld      h,a
        ld      a,(_cardType+1)
        and     a,h
        cp      a,#0xFF
        jr      nz,1$
        inc		l	;ld      l,#1        ; STA_NOINIT
1$:
        ld      h,#0
        ret


 ;**********************************************************************************************


;; STUBS ;;

_disk_readp2:
;     void* dest,         /* Pointer to the destination object */
;     DWORD sector,       /* Sector number (LBA) */
;     WORD sofs,          /* Offset in the sector (0..511) */
;     WORD count          /* Byte count (1..512), bit15:destination flag */

        push    ix
        di
        ld      ix,#4                                       
        add     ix,sp
        
        ld      l,8(ix)             ; count                                      
        ld      h,9(ix)             ; ...
        ; if ((count & 0x8000) || (count >= 513)) return RES_PARERR
        ld      a,h
        and     a,#0x7F
        ld      h,a
        or      a,l
        jr      z,disk_readp_invalid_count
        push	hl
        ld	    de,#513
        and	    a,a                 ; clear carry
        sbc	    hl,de
        pop	    hl
        jr	    nc,disk_readp_invalid_count
        jr      disk_readp_count_ok
disk_readp_invalid_count:        
        pop     ix
        ld      hl,#RES_PARERR
        ei
        ret
disk_readp_count_ok:
        push	hl
        ld	    de,#41
        and	    a,a                 ; clear carry
        sbc	    hl,de
        pop	    hl
        jr	    c,disk_readp_small_read
        ; too big for anything but a file read, don't fetch to cache
        ld      l,2(ix)             ; sector (lo)
        ld      h,3(ix)             ; ...
        ld      de,(_sec_last)
        and     a,a                 ; clear carry
        sbc     hl,de
        jr      nz,2$
        ld      l,4(ix)             ; sector (hi)
        ld      h,5(ix)             ; ...
        ld      de,(_sec_last+2)
        and     a,a                 ; clear carry
        sbc     hl,de
        jr      z,disk_readp_sector_in_buffer
2$:
        ; read sector
        call    neo2_pre_sd
        call    disk_readp_calc_sector
        ld      hl,#_sec_buf
        call    sdReadSingleBlock
        cp      a,#1
        jr      z,3$
        ; read failed, retry once
        call    disk_readp_calc_sector
        ld      hl,#_sec_buf
        call    sdReadSingleBlock
        cp      a,#1
        jr      z,3$
        call    neo2_post_sd
        ld      hl,#0xFFFF
        ld      (_sec_last),hl
        ld      (_sec_last+2),hl
        pop     ix
        ld      hl,#RES_ERROR
        ei
        ret
3$:
        call    neo2_post_sd
        ld      l,2(ix)             ; sector (lo)
        ld      h,3(ix)             ; ...
        ld      (_sec_last),hl
        ld      l,4(ix)             ; sector (hi)
        ld      h,5(ix)             ; ...
        ld      (_sec_last+2),hl
disk_readp_sector_in_buffer:
        ld      hl,#_sec_buf
        jp      disk_readp_copy_data

disk_readp_small_read:
        ld      a,2(ix)             ; sector
        and     a,#DISKIO_CACHE_SIZE-1
        sla     a
        sla     a
        ld      hl,#_sec_tags
        ld      b,#0
        ld      c,a
        ld      (_vregs+8),bc
        add     hl,bc               ; hl = &sec_tags[sector & DISKIO_CACHE_SIZE-1]
        ld      a,(hl)
        cp      2(ix)               ; sector
        jr      nz,disk_readp_fetch_single
        inc     hl
        ld      a,(hl)
        cp      3(ix)               ; sector
        jr      nz,disk_readp_fetch_single
        inc     hl
        ld      a,(hl)
        cp      4(ix)               ; sector
        jr      nz,disk_readp_fetch_single
        inc     hl
        ld      a,(hl)
        cp      5(ix)               ; sector
        jr      z,disk_readp_fetch_done
disk_readp_fetch_single:
        ; sector not in cache - fetch it
        call    neo2_pre_sd
     
        ; hl = &sec_cache[(sector & DISKIO_CACHE_SIZE-1) * 512]
        ld      bc,(_vregs+7)
        ld      c,#0
        sla     b
        ld      hl,#_sec_cache
        add     hl,bc
        ld      (_vregs+10),hl
        
        call    disk_readp_calc_sector
        call    sdReadSingleBlock
        cp      a,#1
        jr      z,4$
        ; read failed, retry once    
        call    disk_readp_calc_sector
        ld      hl,(_vregs+10)
        call    sdReadSingleBlock
        cp      a,#1
        jr      z,4$
        call    neo2_post_sd
        ld      hl,#_sec_tags
        ld      bc,(_vregs+8)
        add     hl,bc
		ld		a,#0xff
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        pop     ix
        ld      hl,#RES_ERROR
        ei
        ret
4$:
        call    neo2_post_sd
        ld      hl,#_sec_tags
        ld      bc,(_vregs+8)
        add     hl,bc
        ld      a,2(ix)         ; sector
        ld      (hl),a
        inc     hl
        ld      a,3(ix)         ; sector
        ld      (hl),a
        inc     hl
        ld      a,4(ix)         ; sector
        ld      (hl),a
        inc     hl
        ld      a,5(ix)         ; sector
        ld      (hl),a
disk_readp_fetch_done:
        ; buf = &sec_cache[ix << 9]
        ld      bc,(_vregs+7)
        ld      c,#0
        sla     b
        ld      hl,#_sec_cache
        add     hl,bc

	
disk_readp_copy_data:
        ld      c,8(ix)         ; count
        ld      b,9(ix)         ; ...
        ld      a,b
        and     a,#0x80
        jr      z,5$
        ; streaming file mode
        ; TODO: Handle this?
        ld      hl,#RES_ERROR
        pop     ix
        ei
        ret
5$:
        ld      e,6(ix)         ; sofs
        ld      d,7(ix)         ; ...
        add     hl,de           ; buf += sofs
        ld      e,0(ix)         ; dest
        ld      d,1(ix)         ; ...
        ldir                    ; block copy
        
        ld      hl,#RES_OK
_disk_readp_return:
        pop     ix
        ei
        ret
	

; Helper subroutine
disk_readp_calc_sector:
        ld      c,2(ix)       ; sector
        ld      b,3(ix)       ; ...
        ld      e,4(ix)       ; ...
        ld      d,5(ix)       ; ...
        ld      a,(_cardType)
        and     a,#1
        jr      nz,1$
        ; sector <<= 9
        ld      d,e
        ld      e,b
        ld      b,c
        ld      c,#0
        sla     b
        rl      e
        rl      d
1$:
        ret


; Reads multiple sectors into PSRAM (going through RAM)
_disk_read_sectors2:
;     WORD destLo,         
;     DWORD sector,       /* Sector number (LBA) */
;     WORD destHi,
;     WORD count

        push    ix
        di
        ld      ix,#4                                       
        add     ix,sp
        
        ; read sector
        call    neo2_pre_sd
        call    disk_readp_calc_sector  
        call    sdReadStartMulti
        cp      a,#1
        jr      z,3$
        ; read failed, retry once
        call    disk_readp_calc_sector
        call    sdReadStartMulti
        cp      a,#1
        jr      z,3$
        call    neo2_post_sd
        pop     ix
        ld      hl,#RES_ERROR
        ei
        ret
3$:
        ld      c,8(ix)     ; count
        ld      b,6(ix)     ; destHi
        ld      e,0(ix)     ; destLo
        ld      d,1(ix)     ; ...
        call    neo2_recv_multi_sd
        ld      a,#RES_OK
        cp      a,l
        jr      z,4$
        ; reading sector data fail, try again
        call    disk_readp_calc_sector
        call    sdReadStartMulti
        cp      a,#1
        jr      z,5$
        call    neo2_post_sd
        pop     ix
        ld      hl,#RES_ERROR
        ei
        ret
5$:
        ld      c,8(ix)     ; count
        ld      b,6(ix)     ; destHi
        ld      e,0(ix)     ; destLo
        ld      d,1(ix)     ; ...
        call    neo2_recv_multi_sd
        ld      a,#RES_OK
        cp      a,l
        jr      z,4$
        call    neo2_post_sd
        pop     ix
        ld      hl,#RES_ERROR
        ei
        ret
4$:        
        call    sdReadStopMulti
        call    neo2_post_sd  
        ld      hl,#RES_OK
_disk_read_sectors_return:
        pop     ix
        ei
        ret

; Reads an entire sector into a destination in RAM
; No extra buffers are used, and no data is cached
_disk_read_sector2:
;     void* dest,         /* Pointer to the destination object */
;     DWORD sector,       /* Sector number (LBA) */

        push    ix
        di
        ld      ix,#4                                       
        add     ix,sp
        
        ; read sector
        call    neo2_pre_sd
        call    disk_readp_calc_sector
        ld      l,0(ix)
        ld      h,1(ix)
        call    sdReadSingleBlock
        cp      a,#1
        jr      z,3$
        ; read failed, retry once
        call    disk_readp_calc_sector
        ld      l,0(ix)
        ld      h,1(ix)
        call    sdReadSingleBlock
        cp      a,#1
        jr      z,3$
        call    neo2_post_sd
        pop     ix
        ld      hl,#RES_ERROR
        ei
        ret
3$:
        call    neo2_post_sd  
        ld      hl,#RES_OK
_disk_read_sector_return:
        pop     ix
        ei
        ret

neo2_enable_sd:
        ret

neo2_pre_sd:
		;push	hl
		ld		hl,#0xBFC0
        ld      (hl),#0x87     		 ; Neo2FlashBankLo = 0x87
		inc		l
        ld      (hl),#0x0F      	; Neo2FlashBankSize = FLASH_SIZE_1M
		ld		l,#0xD0
        ld      (hl),#0x01      	; Neo2Frame0We = 1
        ld      a,#7
        ld      (0xFFFE),a     		 ; Frame1 = 7
		;pop		hl
        ret
    
neo2_disable_sd:
        ret
    
neo2_post_sd:
		;push	hl
		ld		hl,#0xBFC0
		xor		a
        ld      (hl),a      	; Neo2FlashBankLo = 0x0
		inc		l
        ld      (hl),a     		 ; Neo2FlashBankSize = FLASH_SIZE_16M
		ld		l,#0xD0
        ld      (hl),a      	; Neo2Frame0We = 0
		;pop		hl
        ret

; Read to RAM/SRAM
;
; 46.625 cycles/byte
;
; In:
;   HL = buf
neo2_recv_sd:
        ld      b,#64
        ld      de,#MYTH_NEO2_RD_DAT4
        ; Read one sector (512 bytes)
1$:
        ld      a,(de)   ; 7
        ld      (hl),a   ; 7
        ld      a,(de)   ; 7
        rld              ; 18.  (hl) = (hl)<<4 + a&0x0F
        inc     hl       ; 6
; 2nd byte
        ld      a,(de)
        ld      (hl),a
        ld      a,(de)
        rld
        inc     hl
; 3rd byte
        ld      a,(de)
        ld      (hl),a
        ld      a,(de)
        rld
        inc     hl
; 4th byte
        ld      a,(de)
        ld      (hl),a
        ld      a,(de)
        rld
        inc     hl
; 5th byte
        ld      a,(de)
        ld      (hl),a
        ld      a,(de)
        rld
        inc     hl
; 6th byte
        ld      a,(de)
        ld      (hl),a
        ld      a,(de)
        rld
        inc     hl
; 7th byte
        ld      a,(de)
        ld      (hl),a
        ld      a,(de)
        rld
        inc     hl
; 8th byte
        ld      a,(de)
        ld      (hl),a
        ld      a,(de)
        rld
        inc     hl

        djnz    1$       ; 13

        ; Read 8 CRC bytes (16 nybbles)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)

        ld      a,(de)   ; end bit

        ret

;neo2_fill_sector_buffer
;Desc:Fills partially a sector buffer
;In:
;	DE = rdmmcdatbyte4
;	HL = dest buffer				(Destroys LSB)
;	B = 8BYTE units					(Destroys it)
neo2_fill_sector_buffer:

neo2_fill_sector_buffer_loop:
	ld      a,(de)	; 7
	ld      (hl),a	; 7
	ld      a,(de)	; 7
	rld				; 18.  (hl) = (hl)<<4 + a&0x0F
	inc     l		; 4
	ld      a,(de)	; 2nd byte
	ld      (hl),a
	ld      a,(de)
	rld
	inc     l	
	ld      a,(de)	; 3rd byte
	ld      (hl),a
	ld      a,(de)
	rld
	inc     l		
	ld      a,(de)	; 4th byte
	ld      (hl),a
	ld      a,(de)
	rld
	inc     l      
	ld      a,(de)	; 5th byte
	ld      (hl),a
	ld      a,(de)
	rld
	inc     l	
	ld      a,(de)	; 6th byte
	ld      (hl),a
	ld      a,(de)
	rld
	inc     l       
	ld      a,(de)	; 7th byte
	ld      (hl),a
	ld      a,(de)
	rld
	inc     l		
	ld      a,(de)	; 8th byte
	ld      (hl),a
	ld      a,(de)
	rld
	inc     l
	djnz    neo2_fill_sector_buffer_loop       ; 13

neo2_fill_sector_buffer_return:
	ret

; Read to RAM/SRAM (multiple sectors)
; In:
;   C = number of sectors
;   B = PSRAM bank
;   DE = PSRAM offset
neo2_recv_multi_sd:
        push    bc
        push    de
 
        ld      a,#1
        ld      (0xFFFE),a      ; Frame1 = 1
        ld      a,#0x87
        ld      (0xBFC0),a      ; Neo2FlashBankLo = 0x87
        
        ; Wait for start bit
		ld		hl,#MYTH_NEO2_RD_DAT4

		;timeout = 256x16
		ld		b,#0
		ld		c,#16
3$:
        bit		0,(hl)			;12
        jp      z,4$			;10
		djnz	3$				;13
		ld		b,c
		dec		c
		jr		nz,3$
        pop     de
        pop     bc
        ld      hl,#RES_ERROR
        jr      neo2_recv_multi_sd_return
4$:
		ex		de,hl
		ld      hl,#_sec_buf
		ld		b,#32
		call	neo2_fill_sector_buffer
		inc		h
		ld		l,#0x00			;JUST in case
		ld		b,#32
		call	neo2_fill_sector_buffer

        ; Read 8 CRC bytes (16 nybbles)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        ld      a,(de)
        
        ld      a,(de)   ; end bit

        pop     de      ; PSRAM offset
        pop     bc      ; PSRAM bank / number of sectors
        push    bc
        push    de
    
        ; neo2_ram_to_psram(b, de, _sec_buf, 512)
        ld      hl,#512
        push    hl
        ld      hl,#_sec_buf
        push    hl
        push    de
        push    bc
        inc     sp
        call    _neo2_ram_to_psram
        di
        inc     sp
        pop     af
        pop     af
        pop     af
      
        ; reset registers modified by neo2_ram_to_psram
        call    neo2_pre_sd
        
        pop     de
        pop     bc
        ; PSRAM offset += 512
        ld      hl,#512
        and     a,a     ; clear carry
        adc     hl,de
        ex      de,hl
        ; if (offset == 0) bank++
        jr      nc,5$
        inc     b
5$:
        ; numSectors--
        dec     c
        jr      nz,neo2_recv_multi_sd
        ld      hl,#RES_OK
neo2_recv_multi_sd_return:
		push	hl
        call    neo2_pre_sd
		pop		hl
        ret
        
_sec_cache = 0xC580 ;: .ds 520
_sec_buf = 0xDB00  ;: .ds 520

; DRESULT sdWriteSingleBlock(void* src,DWORD addr)
_sdWriteSingleBlock:
        ret





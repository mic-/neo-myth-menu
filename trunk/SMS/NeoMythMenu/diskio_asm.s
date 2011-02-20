; Sega Master System implementation of diskio
; /Mic, 2011


.area   _CODE


; MMC response lengths
R1_LEN = 6
R2_LEN = 17
R3_LEN = 6
R6_LEN = 6
R7_LEN = 6


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
SD_SEND_OP_COND = 41
APP_CMD = 55


; Number of entries in the sector cache. The cache itself is declared in diskio.c.
DISKIO_CACHE_SIZE = 2


MYTH_NEO2_WR_CMD1_CLR13 = 0x5E00    ; FlashBankLo=0x87,Frame1=7
MYTH_NEO2_WR_CMD1_SET13 = 0x7E00    ; 0x87,7
MYTH_NEO2_WR_DAT1_CLR9 = 0x7C00     ; 0x87,7
MYTH_NEO2_WR_DAT1_SET9 = 0x7E00     ; 0x87,7
MYTH_NEO2_WR_DAT4 = 0x6000          ; 0x87,7

MYTH_NEO2_RD_CMD1 = 0x7E81          ; 0x87,7
MYTH_NEO2_RD_DAT1 = 0x7C21          ; 0x87,5
MYTH_NEO2_RD_DAT4 = 0x6061          ; 0x87,1


;**********************************************************************************************

; External variables
.globl _vregs
.globl _diskioPacket
.globl _diskioResp
.globl _diskioTemp
.globl _cardType
.globl _sd_csd
.globl _numSectors
.globl _sec_tags
.globl _sec_last


; Exported functions
.globl _disk_initialize
.globl _disk_readp

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
        and a,#15
        ld  h,a
        sla h
        ld  l,#0
        ld  bc,#MYTH_NEO2_WR_DAT4
        add hl,bc               ; HL = MYTH_NEO2_WR_DAT4 + ((dat & 15) << 9)
        ld  a,(hl)
        ret



;**********************************************************************************************


; void wrMmcCmdByte( unsigned int byte )
;
;   A = byte
wrMmcCmdByte:
        ld  b,#8
1$:
        rlc a
        ld  c,a
        call    wrMmcCmdBit
        ld  a,c
        djnz    1$
        ret


;**********************************************************************************************


; void wrMmcDatByte( unsigned int byte )
;
;   A = byte
wrMmcDatByte:
        ld  b,#8
1$:
        rlc a
        ld  c,a
        call    wrMmcDatBit         ; write  (byte >> (7-i)) & 1
        ld  a,c
        djnz    1$
        ret

;**********************************************************************************************


; unsigned int rdMmcCmdBit()
;
rdMmcCmdBit:
        ld  a,(MYTH_NEO2_RD_CMD1)
        srl a
        srl a
        srl a
        srl a
        and a,#1
        ret


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
        ld  a,(MYTH_NEO2_RD_DAT4)
        and a,#15
        ret


;**********************************************************************************************


; unsigned char rdMmcCmdBits( int num )
;
;  A = num
rdMmcCmdBits:
        ld    b,a
        ld    c,#0
1$:
        ld    a,(MYTH_NEO2_RD_CMD1)
        srl   a
        srl   a
        srl   a
        srl   a
        srl   a
        rl    c     ; c = (c << 1) | rdMmcCmdBit()
        djnz  1$
        ld    a,c
        ret

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
        ld      ix,#_vregs
        ld      0(ix),a
        ld      1(ix),a
        ld      2(ix),a
        ld      3(ix),a

        ld      b,#0x40
        ld      a,#0
        ld      4(ix),a         ; x = 0
        ld      c,a             ; crc = 0
1$:
        ld      a,(_vregs+0)    ; if (r0 & 0x80) x = *buf++;
        and     a,#0x80
        jr      z,2$
        ld      a,(de)
        inc     de
        ld      (_vregs+4),a
2$:
        sla     a               ; crc <<= 1

        bit     7,c            ; if (crc & 0x80) crc ^= 9;
        jr      z,3$
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
        srl     0(ix)           ; r0 = (r0 >> 1) | (r0 << 31);
        rr      3(ix)
        rr      2(ix)
        rr      1(ix)
        rr      0(ix)

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
        or      a,#0x40         ; b7 = 0 => start bit, b6 = 1 => host command
        ld      (_diskioPacket),a
        call    wrMmcCmdByte

        ld      a,d             ; arg >> 24
        ld      (_diskioPacket+1),a
        call    wrMmcCmdByte

        ld      a,e             ; arg >> 16
        ld      (_diskioPacket+2),a
        call    wrMmcCmdByte

        ld      a,b             ; arg >> 8
        ld      (_diskioPacket+3),a
        call    wrMmcCmdByte

        ld      a,c             ; arg
        ld      (_diskioPacket+4),a
        call    wrMmcCmdByte

        ld      de,#_diskioPacket
        call    crc7
        sla     a
        or      a,#1            ; b0 = 1 => stop bit
    ; -- DEBUG --
        ld      (_diskioPacket+5),a
    ; -----------
        call    wrMmcCmdByte    ; wrMmcCmdByte( (crc7(diskioPacket) << 1) | 1 )

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
        ld      hl,#1024
1$:
        ld  a,(MYTH_NEO2_RD_CMD1)
        srl a
        srl a
        srl a
        srl a
        and a,#1

        jr      nz,2$
        ld      a,#7
        call    rdMmcCmdBits
        ld      (de),a          ; *resp++ = rdMmcCmdBits(7);
        inc     de

        djnz    3$              ; while (--len)
        jr      4$
3$:
        call    rdMmcCmdByte
        ld      (de),a          ; *resp++ = rdMmcCmdByte();
        inc     de
        djnz    3$
4$:

        ld      a,c             ; cflag
        and     a,c
        jr      z,5$
        ld      a,#0xFF
        call    wrMmcCmdByte
5$:
        ld      a,#1            ; return TRUE
        ret
2$:
        dec     hl
        ld      a,h
        or      a,l
        jp      nz,1$

        ld      a,#0            ; return FALSE
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
        jp      nz,1$
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#0
        call    recvMmcCmdResp
        and     a,a
        jr      z,2$
        ld      a,(_diskioResp+0)
        cp      a,#READ_SINGLE_BLOCK
        jp      z,1$
2$:
        pop hl
        ld      a,#0            ; return FALSE
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
        jp      nz,3$

        pop hl
        ld      a,#0            ; return FALSE (timeout on start bit)
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
        jp      nz,1$
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#0
        call    recvMmcCmdResp
        and     a,a
        jr      z,2$
        ld      a,(_diskioResp+0)
        cp      a,#READ_MULTIPLE_BLOCK
        jp      z,1$
2$:
        ld      a,#0            ; return FALSE
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
        ld      bc,#0
        ld      de,#0
        ld      a,#STOP_TRANSMISSION
        call    sendMmcCmd

        ; VERIFY_DISK_RESP STOP_TRANSMISSION,R1_LEN,0
        ld      a,(_cardType+1)
        ; cardType & 0x8000 ?
        and     a,#0x80
        jp      nz,1$
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#0
        call    recvMmcCmdResp
        and     a,a
        jr      z,2$
        ld      a,(_diskioResp+0)
        cp      a,#STOP_TRANSMISSION
        jp      z,1$
2$:
        ld      a,#0            ; return FALSE
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
        ld      b,#0x80
1$:
        ld      a,#1
        call    wrMmcCmdBit             ; wrMmcCmdBit(1)
        djnz    1$

        ld      bc,#0
        ld      de,#0
        ld      a,#GO_IDLE_STATE
        call    sendMmcCmd
        ld      a,#0xFF
        call    wrMmcCmdByte

        ; Check if the card can operate on the given voltage (2.7-3.6 V)
        ld      bc,#0xAA
        ld      de,#0x01
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
        jp      2$
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
        ld      a,#0                ; if (!(diskioResp[2] & 0x30)) return FALSE
        ret
9$:
        pop     bc
        jr      sdInit_loop_end
7$:
5$:
        pop     bc
        dec     b
        jp      nz,4$
sdInit_loop_end:

        ld      a,b
        cp      a,#0
        jp      z,sdInit_failed     ; timed out

        ; ALL_SEND_CID
        ld      bc,#0xFFFF
        ld      de,#0xFFFF
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
        jp      z,sdInit_failed
        ld      a,(_diskioResp+0)
        cp      a,#7
        jp      nz,sdInit_failed

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
        jp      z,sdInit_failed
        ld      a,(_diskioResp+0)
        and     #0x20
        jp      z,sdInit_failed

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
        jp      z,sdInit_failed
        ld      a,(_diskioResp+0)
        cp      a,#6
        jp      nz,sdInit_failed

        ld      a,#1               ; return TRUE
        ret

sdInit_failed:
            call    neo2_disable_sd
        ld      a,#0               ; return FALSE
        ret


;**********************************************************************************************


; DSTATUS disk_initialize (void)
_disk_initialize:
        push    af
        push    bc
        push    de
        push    ix

        call    neo2_enable_sd

            ; Invalidate all chache entries (sec_tags[i] = 0xFFFFFFFF)
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
        jr      z,2$
        ld      a,#0xFF
        ld      (_cardType+0),a
        ld      (_cardType+1),a
2$:
        call    neo2_post_sd

        ld      a,(_sd_csd+1)
        and     a,#0xC0
        jp      nz,3$

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
        jp      4$
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

        jp      6$
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
        jp      nz,7$
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
        ld      l,#1        ; STA_NOINIT
1$:
        ld      h,#0
        ret


 ;**********************************************************************************************


;; STUBS ;;

_disk_readp:
ret


.globl neo2_enable_sd
.globl neo2_disable_sd
.globl neo2_pre_sd
.globl neo2_post_sd

neo2_enable_sd:
neo2_disable_sd:
neo2_pre_sd:
neo2_post_sd:
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


; Read to PSRAM
;
; 59.625 cycles/byte
;
; In:
;   HL = buf
neo2_recv_sd_psram:
        ld      b,#64
        ld      de,#MYTH_NEO2_RD_DAT4
        ; Read one sector (512 bytes)
1$:
        ld      a,(de)   ; 7
        add     a,a      ; 4
        add     a,a      ; 4
        add     a,a      ; 4
        add     a,a      ; 4
        ld      c,a      ; 4
        ld      a,(de)   ; 7
        and     #0x0F    ; 7
        or      a,c      ; 4
        ld      (hl),a   ; 7
        inc     hl       ; 6

; 2nd byte
        ld      a,(de)
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        ld      c,a
        ld      a,(de)
        and     #0x0F
        or      a,c
        ld      (hl),a
        inc     hl
; 3rd byte
        ld      a,(de)
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        ld      c,a
        ld      a,(de)
        and     #0x0F
        or      a,c
        ld      (hl),a
        inc     hl
; 4th byte
        ld      a,(de)
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        ld      c,a
        ld      a,(de)
        and     #0x0F
        or      a,c
        ld      (hl),a
        inc     hl
; 5th byte
        ld      a,(de)
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        ld      c,a
        ld      a,(de)
        and     #0x0F
        or      a,c
        ld      (hl),a
        inc     hl
; 6th byte
        ld      a,(de)
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        ld      c,a
        ld      a,(de)
        and     #0x0F
        or      a,c
        ld      (hl),a
        inc     hl
; 7th byte
        ld      a,(de)
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        ld      c,a
        ld      a,(de)
        and     #0x0F
        or      a,c
        ld      (hl),a
        inc     hl
; 8th byte
        ld      a,(de)
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        ld      c,a
        ld      a,(de)
        and     #0x0F
        or      a,c
        ld      (hl),a
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






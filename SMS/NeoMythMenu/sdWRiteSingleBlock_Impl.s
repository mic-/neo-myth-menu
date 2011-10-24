
; DRESULT sdWriteSingleBlock(void* src,DWORD addr)
_sdWriteSingleBlock:
		push	ix
		ld		ix,#4
		add		ix,sp

		call    neo2_pre_sd
		call	disk_readp_calc_sector
        ld      a,#WRITE_SINGLE_BLOCK
        call    sendMmcCmd
        ld      de,#_diskioResp
        ld      b,#R1_LEN
        ld      c,#0
        call    recvMmcCmdResp
        and     a,a
        jr      z,2$
        ld      a,(_diskioResp+0)
        cp      a,#WRITE_SINGLE_BLOCK
        jr      z,1$
2$:
		call    neo2_post_sd
		pop		ix
        ld      hl,#0            ; return FALSE
        ret
1$:	
		ld		hl,#512
		push	hl
		ld		h,1(ix)
		ld		l,0(ix)
		push	hl
		ld		hl,#0xDA08		; HARDCODED - 8bytes -> crc
		push	hl
		;call	neo2_sd_crc16 ;(unsigned char *p_crc, unsigned char *data, int len)
		pop		hl
		pop		hl
		pop		hl

		;wrMmcDatByte4(0xff)
		ld		a,#0xff
		call	wrMmcDatByte4

		;Write sector data
		ld		bc,#512
		ld		h,1(ix)
		ld		l,0(ix)
3$:
		ld		a,(hl)
		call	wrMmcDatByte4
		inc		hl
		dec		bc
		ld		a,h
		or		a,l
		jp		nz,3$

		;Write crc
		ld		hl,#0xDA08	
		ld		b,#8
4$:
		ld		a,(hl)
		call	wrMmcDatByte4
		inc		hl
		djnz	4$

		;end bit
		ld		a,#0x0f
		call	wrMmcDatBit4

		;clock out two bits on D0
		call	rdMmcDatByte4

		;check for start bit
		call	rdMmcDatBit4
		and		a,#0x01
		jp		nz,2$

		;crc status
		ld		c,#0x00
		ld		b,#3
5$:
		sla		c
		call	rdMmcDatBit4
		and		a,#0x01
		ld		l,a
		ld		a,c
		or		a,l
		ld		c,a
		djnz	5$

		;end bit
		call	rdMmcDatBit4

		;crc status has to be 0B10
		bit		1,c
		jp		nz,2$

		;wait for start bit
6$:
		call	rdMmcDatBit4
		and		a,#0x01
		jr		nz,6$

		;wait for write to finish (b0)
		ld		bc,#60*1024
7$:
		call	rdMmcDatBit4
		and		a,#0x01
		jr		nz,8$
		dec		bc
		ld		a,b
		or		a,c
		jp		nz,7$

8$:
		;8 cycles to complete the operation so clock can halt
		ld		a,#0xff
		call	wrMmcCmdByte

		call    neo2_post_sd
		pop		ix
        ld      hl,#1        ; return TRUE
        ret


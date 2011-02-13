        .area   _CODE

        .globl _vdp_set_vram_addr
_vdp_set_vram_addr:
		ld      a,l				;;lb
		out     (0xbf),a		;;w
		ld      a,h				;;hb
		or      a,#0x40			;;CMD_VRAM_WRITE
		out     (0xbf),a		;;w
        ret


     ; check if this is an NTSC (60 Hz) or PAL (50 Hz) vdp
    .globl _vdp_check_speed
    _vdp_check_speed:
	ld	c,#0x7E		; V counter
    ; wait until it reaches zero
    1$:
    	in	a,(c)	
    	jp	nz,1$
    	
    ; the v counter sequence should be 00-DA,D5-FF for NTSC and
    ; 00-F2,BA-FF for PAL. so we loop until the linear pattern
    ; is broken (D5 / BA), at which point our own counter should
    ; have reached either DB (NTSC) or F3 (PAL).
    	ld	b,a
    	ld	hl,#0		; NTSC
    2$:
    	in	a,(c)
    	cp	a,b
    	jp	z,2$
    	inc	b
    	cp	a,b
    	jp	z,2$
    	ld	a,b
    	and	a,#0xF0
    	cp	a,#0xF0
    	ret	nz
    	ld	l,#1		; PAL
    	ret

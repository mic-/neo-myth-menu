        .area   _CODE
		;;.globl	_vdp_wait_vblank

        .globl _vdp_set_vram_addr				;;Does not overwrite HL , so the caller doesn't have to S/R HL
		_vdp_set_vram_addr:
		ld      a,l				;;lb
		out     (0xbf),a		;;w
		ld      a,h				;;hb
		or      a,#0x40			;;CMD_VRAM_WRITE
		out     (0xbf),a		;;w
        ret

		;;void vdp_blockcopy_to_vram(WORD dest, BYTE *src, WORD len)
		.globl _vdp_blockcopy_to_vram
		_vdp_blockcopy_to_vram:
		push            ix
		        ld      ix,#4										;;2 + stack depth * sizeof word
		        add     ix,sp										;;+=sp
		        ld      l,(ix)										;;dest
		        ld      h,1(ix)										;;...

				;;call	_vdp_wait_vblank
				call	_vdp_set_vram_addr
		        ld      l,2(ix)										;;src
		        ld      h,3(ix)										;;...
				ld		e,4(ix)										;;len
				ld		d,5(ix)										;;...
				ld		c,#0xbe
				jp		vdp_blockcopy_to_vram_loop

				vdp_blockcopy_to_vram_large_block:					;;Copy 128bytes at once (21cycles * b)
				ld		b,#0x80
				otir
				ld		bc,#-0x80
				ex		de,hl
				add		hl,bc
				ex		de,hl
				ld		c,#0xbe

				vdp_blockcopy_to_vram_loop:							;;Check if there's enough bytes left to do large copies
				ld		a,d
				or		a,e
				jp		z,vdp_blockcopy_to_vram_loop_pre_single
				push	hl
				ld		hl,#0
				add		hl,de
				ld		bc,#-0x80
				add		hl,bc
				ld		a,h
				or		a,l
				pop		hl
				jp		z,vdp_blockcopy_to_vram_large_block

				vdp_blockcopy_to_vram_loop_pre_single:				;;Rollback to single byte copies but first make sure that the size isn't zero
				ld		a,d
				or		e
				jp		z,vdp_blockcopy_to_vram_loop_done

				ld		c,#0xbe
				vdp_blockcopy_to_vram_loop_single:					;;Copy a byte at a time 
				outi
				dec		de
				ld		a,d
				or		e
				jp		nz,vdp_blockcopy_to_vram_loop_single

				vdp_blockcopy_to_vram_loop_done:
		pop				ix

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


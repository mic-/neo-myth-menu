; VGM player for the SMS Myth menu


.area _CODE

.globl _vgm_play


SNPORT = 0x7F
VDPSPEED = 0xC005
VREGS = 0xC006
VGMBASE = 0x86B2

CMD_GG_STEREO = 0x4F
CMD_PSG = 0x50
CMD_FM = 0x51
CMD_WAIT = 0x61
CMD_WAIT_FRAME_NTSC = 0x62
CMD_WAIT_FRAME_PAL = 0x62
CMD_END = 0x66


_vgm_play:
	push	ix

	di
	
	; calc loop offset
	ld	ix,#VGMBASE
	ld	l,0x1C(ix)
	ld	h,0x1D(ix)
	ld	bc,#VGMBASE+0x1C
	add	hl,bc
	ld	a,l
	ld	(VREGS+0),a
	ld	a,h
	ld	(VREGS+1),a
	ld	l,0x1E(ix)
	ld	h,0x1F(ix)
	ld	bc,#0
	adc	hl,bc
	ld	a,l
	ld	(VREGS+2),a
	ld	a,h
	ld	(VREGS+3),a

	ld	bc,#VGMBASE+0x40

vgm_play_loop:	
	ld	a,(bc)		; 7
	inc	bc		; 6. TODO: handle 16kB frame wraps
	.db 0xDD,0x67 ;ld	ixh,a		; 8
	
	add	a,#0xC0		; 7
	ld	d,#0		; 7
	ld	e,a		; 4
	sla	e		; 8
	rl	d		; 8
	ld	hl,#vgm_jump_table ; 10
	add	hl,de		; 11
	ld	(vdm_load_address+1),hl ;16
vdm_load_address:
	ld	hl,(0000)	; 16. address is overwritten by the previous instruction
	jp	(hl)		; 4

	; Currently never returns

vgm_psg_write:
	ld	a,(bc)
	inc	bc
	out	(SNPORT),a
	jp	vgm_play_loop

	
vgm_fm_write:
	; TODO: handle this
	inc	bc
	inc	bc
	jp	vgm_play_loop
	
	
vgm_gg_stereo:	; Ignored
	inc	bc
	jp	vgm_play_loop


vgm_end:
	; TODO: handle loop points outside of the first 16kB
	ld	a,(VREGS+0)
	ld	c,a
	ld	a,(VREGS+1)
	ld	b,a
	jp	vgm_play_loop



;3.546893  PAL,   80.428 cycles/sample
;3.579545  NTSC,  81.169 cycles/sample

vgm_short_wait:
	.db 0xDD,0x7C ;ld	a,ixh	; 8
	and	a,#0x0F	; 7
	jp	z,1$	; 10
	
	; 80 cycles/iteration
2$:
	ld	ix,#0	; 20
	ld	ix,#0	; 20
	ld	ix,#0	; 20
	dec	de	; 6
	dec	a	; 4
	jp	nz,2$	; 10
1$:
	jp	vgm_play_loop ; 10


vgm_wait:
	ld	a,(bc)
	inc	bc
	ld	e,a
	ld	a,(bc)
	inc	bc
	ld	d,a
vgm_wait2:
	dec	de	; 6
	ld	a,e	; 4
	or	a,d	; 4
	jp	z,1$	; 10
	
	; 80 cycles/iteration
2$:
	set	#0,a	; 8
	set	#0,a	; 8
	set	#0,a	; 8
	set	#0,a	; 8
	set	#0,a	; 8
	set	#0,a	; 8
	set	#0,a	; 8
	dec	de	; 6
	ld	a,e	; 4
	or	a,d	; 4
	jp	nz,2$	; 10
1$:
	jp	vgm_play_loop ; 10
	
	
vgm_wait_pal:
	ld	de,#882
	jp	vgm_wait2
	
vgm_wait_ntsc:
	ld	de,#735
	jp	vgm_wait2



vgm_jump_table:
.dw 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,vgm_gg_stereo
.dw vgm_psg_write,vgm_fm_write,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
.dw 0,vgm_wait,vgm_wait_ntsc,vgm_wait_pal, 0,0,vgm_end,0, 0,0,0,0, 0,0,0,0
.dw vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait
.dw vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait

.globl _vgm_player_end
_vgm_player_end:

; VGM player for the SMS Myth menu

.module vgm_player

.area _DATA
.area _OVERLAY
.area _HOME
.area _GSINIT
.area _GSFINAL
.area _GSINIT
.area _HOME
    

.area _CODE

.globl _vgm_play


SNPORT = 0x7F
VDPSPEED = 0xC005
VREGS = 0xC006
VGMBASE = 0x4000

CMD_GG_STEREO = 0x4F
CMD_PSG = 0x50
CMD_FM = 0x51
CMD_WAIT = 0x61
CMD_WAIT_FRAME_NTSC = 0x62
CMD_WAIT_FRAME_PAL = 0x63
CMD_END = 0x66

.include "neo2_map.inc"


; Needs to be aligned on a 256-byte boundary
vgm_jump_table:
.dw 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,vgm_gg_stereo
.dw vgm_psg_write,vgm_fm_write,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
.dw 0,vgm_wait,vgm_wait_ntsc,vgm_wait_pal, 0,0,vgm_end,0, 0,0,0,0, 0,0,0,0
.dw vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait
.dw vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait,vgm_short_wait


_vgm_play:
	push	ix
	push	iy

    di
        
    call    neo2_enable_psram
    ld      a,#0
    ld      (0xBFC0),a
    ld      (0xFFFE),a   
	ld      (VREGS+10),a    ; psram bank
    
	; calc loop offset
	ld	    ix,#VGMBASE
	ld	    l,0x1C(ix)
	ld	    h,0x1D(ix)
	ld	    bc,#VGMBASE+0x1C
	add	    hl,bc
    ld      a,l
    or      a,h
    jr      nz,1$
    ld      hl,#VGMBASE+0x40    ; The loop point is zero, use 0x40
1$:
	ld	    a,l
	ld	    (VREGS+0),a
	ld	    a,h
	ld	    (VREGS+1),a
	ld	    l,0x1E(ix)
	ld	    h,0x1F(ix)
	ld	    bc,#0
	adc	    hl,bc
	ld	    a,l
	ld	    (VREGS+2),a
	ld	    a,h
	ld	    (VREGS+3),a

	ld	    bc,#VGMBASE+0x40
	ld	    iy,#vgm_jump_table
	
vgm_play_loop:	
    bit     7,b
    jr      z,1$
    ld      b,#0x40
    ld      a,(VREGS+10)
    inc     a
    ld      (VREGS+10),a
    ld      d,a
    and     a,#7
    ld      (0xFFFE),a
    ld      a,d
    srl     a
    srl     a
    srl     a
    ld      (0xBFC0),a
1$:
	ld	    a,(bc)		; 7
	inc	    bc		    ; 6
	.db     0xDD,0x67   ; ld	ixh,a		; 8

    add     a,#0xC0     ; 7. subtract 0x40
    rlca                ; 4. multiply by 2
    .db     0xFD,0x6F   ; ld      iyl,a   ; 8
    ld      l,0(iy)     ; 19
    ld      h,1(iy)     ; 19
    jp      (hl)        ; 4
	
vgm_play_return:
	pop	    iy
	pop	    ix
	; mute the PSG
	ld	    a,#0x9F
	out	    (SNPORT),a
	ld	    a,#0xBF
	out	    (SNPORT),a
	ld	    a,#0xDF
	out	    (SNPORT),a
	ld	    a,#0xFF
	out	    (SNPORT),a
	ret

vgm_psg_write:
    bit     7,b
    jr      z,1$
    ld      b,#0x40
    ld      a,(VREGS+10)
    inc     a
    ld      (VREGS+10),a
    ld      d,a
    and     a,#7
    ld      (0xFFFE),a
    ld      a,d
    srl     a
    srl     a
    srl     a
    ld      (0xBFC0),a
1$:
	ld	    a,(bc)
	inc	    bc
	out	    (SNPORT),a
	jp	    vgm_play_loop

	
vgm_fm_write:
	; TODO: handle this
	inc	    bc
	inc	    bc
	jp	    vgm_play_loop
	
	
vgm_gg_stereo:	; Ignored
	inc	    bc
	jp	    vgm_play_loop


vgm_end:
	; TODO: handle loop points outside of the first 16kB
    ld      a,#0
    ld      (0xBFC0),a
    ld      (0xFFFE),a   
	ld      (VREGS+10),a    ; psram bank    
	ld	    a,(VREGS+0)
	ld	    c,a
	ld	    a,(VREGS+1)
	ld	    b,a
	jp	    vgm_play_loop



;3.546893  PAL,   80.428 cycles/sample
;3.579545  NTSC,  81.169 cycles/sample

vgm_short_wait:
	.db     0xDD,0x7C   ; ld	a,ixh	; 8
	and	    a,#0x0F	    ; 7
	jp	    z,vgm_play_loop	; 10
	
	; 80 cycles/iteration
1$:
	ld	    ix,#0	; 20
	ld	    ix,#0	; 20
	ld	    ix,#0	; 20
	dec	    de	; 6
	dec	    a	; 4
	jp	    nz,1$	; 10
	jp	    vgm_play_loop ; 10


vgm_wait:
    bit     7,b
    jr      z,1$
    ld      b,#0x40
    ld      a,(VREGS+10)
    inc     a
    ld      (VREGS+10),a
    ld      d,a
    and     a,#7
    ld      (0xFFFE),a
    ld      a,d
    srl     a
    srl     a
    srl     a
    ld      (0xBFC0),a
1$:
	ld	    a,(bc)
	inc	    bc
	ld	    e,a
    bit     7,b
    jr      z,2$
    ld      b,#0x40
    ld      a,(VREGS+10)
    inc     a
    ld      (VREGS+10),a
    ld      d,a
    and     a,#7
    ld      (0xFFFE),a
    ld      a,d
    srl     a
    srl     a
    srl     a
    ld      (0xBFC0),a
2$:    
	ld	    a,(bc)
	inc	    bc
	ld	    d,a
vgm_wait2:
	dec	    de	; 6
	ld	    a,e	; 4
	or	    a,d	; 4
	jp	    z,vgm_play_loop	; 10
	
	; 80 cycles/iteration
1$:
	set	    #0,a	; 8
	set	    #0,a	; 8
	set	    #0,a	; 8
	set	    #0,a	; 8
	set	    #0,a	; 8
	set	    #0,a	; 8
	set	    #0,a	; 8
	dec	    de	; 6
	ld	    a,e	; 4
	or	    a,d	; 4
	jp	    nz,1$	; 10
	jp	    vgm_play_loop ; 10
	
	
vgm_wait_pal:
	in	    a,(0xDC)
	and	    a,#0x10
	;jp	    nz,vgm_play_return
	ld	    de,#882
	jp	    vgm_wait2
	
vgm_wait_ntsc:
	in	    a,(0xDC)
	and	    a,#0x10
	;jp	    nz,vgm_play_return
	ld	    de,#735
	jp	    vgm_wait2



.globl _vgm_player_end
_vgm_player_end:

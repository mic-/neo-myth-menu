; SGC player for the SMS Myth menu
; /Mic, 2011

.module sgc_player

.area _DATA
.area _OVERLAY
.area _HOME
.area _GSINIT
.area _GSFINAL
.area _GSINIT
.area _HOME
    

.area   _HEADER (ABS)


SGC_SPEED = 0x0305
SGC_INIT_ADDR = 0x030A
SGC_PLAY_ADDR = 0x030C
SGC_STACK_PTR = 0x030E
SGC_RST_VECTORS = 0x0312
SGC_MAPPER_SETUP = 0x0320


.org    0
    di
    im      1
    jp      sgc_play

.org    0x08
    jp  0x0008  ; filled in later
.org    0x10
    jp  0x0010
.org    0x18
    jp  0x0018
.org    0x20
    jp  0x0020
.org    0x28
    jp  0x0028
.org    0x30
    jp  0x0030
.org    0x38
    jp  0x0038

.org    0x66
    retn
        

.org    0x100
        
sgc_play:
    ld      a,(SGC_MAPPER_SETUP)
    ld      (0xFFFC),a
    ld      a,(SGC_MAPPER_SETUP+1)
    ld      (0xFFFD),a
    ld      a,(SGC_MAPPER_SETUP+2)
    ld      (0xFFFE),a
    ld      a,(SGC_MAPPER_SETUP+3)
    ld      (0xFFFF),a

    ; Clear RAM
    ld      a,#0
    ld      hl,#0xC000
    ld      de,#0xC001
    ld      (hl),a
    ld      bc,#0x1FFF
    ldir
   
    ld      a,(SGC_STACK_PTR)
    ld      l,a
    ld      a,(SGC_STACK_PTR+1)
    ld      h,a
    ld      sp,hl
    
    ld      a,(SGC_INIT_ADDR)
    ld      l,a
    ld      a,(SGC_INIT_ADDR+1)
    ld      h,a
    ld      a,(0x0324)
    call    sgc_call_hl
    
sgc_play_loop:
    ld      a,(SGC_SPEED)
    and     a,a
    jp      z,sgc_wait_ntsc
    jp      sgc_wait_pal
sgc_wait_done:
    ld      a,(SGC_PLAY_ADDR)
    ld      l,a
    ld      a,(SGC_PLAY_ADDR+1)
    ld      h,a
    call    sgc_call_hl
    jp      sgc_play_loop

    
sgc_call_hl:
    jp      (hl)
    
    
sgc_wait:
	dec	    de	; 6
	ld	    a,e	; 4
	or	    a,d	; 4
	jp	    z,sgc_wait_done	; 10
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
	jp	    sgc_wait_done ; 10
	

; Wait 1/50 s	
sgc_wait_pal:
	ld	    de,#882
	jp	    sgc_wait
	
; Wait 1/60 s
sgc_wait_ntsc:
	ld	    de,#735
	jp	    sgc_wait
    
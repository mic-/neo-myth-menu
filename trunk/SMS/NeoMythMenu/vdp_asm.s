        .area   _CODE
        ;;.globl    _vdp_wait_vblank

        .globl _vdp_set_vram_addr               ;;Does not overwrite HL , so the caller doesn't have to S/R HL
        _vdp_set_vram_addr:
        ld      a,l             ;;lb
        out     (0xbf),a        ;;w
        ld      a,h             ;;hb
        or      a,#0x40         ;;CMD_VRAM_WRITE
        out     (0xbf),a        ;;w
        ret

        ;;void vdp_blockcopy_to_vram(WORD dest, BYTE *src, WORD len)
        ;;can copy up to $80FF bytes
        .globl _vdp_blockcopy_to_vram
        _vdp_blockcopy_to_vram:
        push            ix
                ld      ix,#4                                       ;;2 + stack depth * sizeof word
                add     ix,sp                                       ;;+=sp
                ld      l,(ix)                                      ;;dest
                ld      h,1(ix)                                     ;;...

                call    _vdp_set_vram_addr
                ld      l,2(ix)                                     ;;src
                ld      h,3(ix)                                     ;;...
                ld      c,#0xbe
                ld      a,5(ix)                                     ;;len MSB
                or      a,a
                jp      z,vdp_blockcopy_to_vram_loop_done           ;;not at least $100 bytes
                ld      b,#0
                ld      e,a
            vdp_blockcopy_to_vram_loop:
                otir                                                ;;output $100 bytes
                dec     e
                jp      z,vdp_blockcopy_to_vram_loop_done
                ld      b,#0
                jp      vdp_blockcopy_to_vram_loop

            vdp_blockcopy_to_vram_loop_done:
                ld      a,4(ix)                                     ;;len LSB
                or      a,a
                jp      z,vdp_blockcopy_to_vram_done                ;;no "left over" bytes
                ld      b,a
                otir                                                ;;output len & $FF bytes
            vdp_blockcopy_to_vram_done:
        pop             ix

     ; check if this is an NTSC (60 Hz) or PAL (50 Hz) vdp
    .globl _vdp_check_speed
    _vdp_check_speed:
    ld  c,#0x7E     ; V counter
    ; wait until it reaches zero
    1$:
        in  a,(c)
        jp  nz,1$

    ; the v counter sequence should be 00-DA,D5-FF for NTSC and
    ; 00-F2,BA-FF for PAL. so we loop until the linear pattern
    ; is broken (D5 / BA), at which point our own counter should
    ; have reached either DB (NTSC) or F3 (PAL).
        ld  b,a
        ld  hl,#0       ; NTSC
    2$:
        in  a,(c)
        cp  a,b
        jp  z,2$
        inc b
        cp  a,b
        jp  z,2$
        ld  a,b
        and a,#0xF0
        cp  a,#0xF0
        ret nz
        ld  l,#1        ; PAL
        ret



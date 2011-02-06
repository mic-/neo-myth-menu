
        .area   _CODE
        .globl  _pad_get_2button
_pad_get_2button:
        push    ix
        ld      ix,#0
        add     ix,sp       ; ix = frame pointer

        xor     a,a         ; clear a
        or      a,4(ix)     ; test port index
        jr      nz,_read22  ; read port 2

        in      a,(0xDC)    ; -  -  s2 s1 r  l  d  u
        and     a,#0x3F
        xor     a,#0x3F     ; 0  0  S2 S1 R  L  D  U

        ld      l,a         ; return value
        pop     ix
        ret

_read22:
        in      a,(0xDC)    ; d  u  -  -  -  -  -  -
        and     a,#0xC0     ; d  u  0  0  0  0  0  0
        rlca                ; u  0  0  0  0  0  0  d
        rlca                ; 0  0  0  0  0  0  d  u
        ld      c,a
        in      a,(0xDD)    ; -  -  -  -  s2 s1 r  l
        and     a,#0x0F     ; 0  0  0  0  s2 s1 r  l
        add     a,a         ; 0  0  0  s2 s1 r  l  0
        add     a,a         ; 0  0  s2 s1 r  l  0  0
        or      a,c         ; 0  0  s2 s1 r  l  d  u
        xor     a,#0x3F     ; 0  0  S2 S1 R  L  D  U

        ld      l,a         ; return value
        pop     ix
        ret


        .globl  _pad_get_3button
_pad_get_3button:
        push    ix
        ld      ix,#0
        add     ix,sp       ; ix = frame pointer

        xor     a,a         ; clear a
        or      a,4(ix)     ; test port index
        jr      nz,_read32  ; read port 2

        ld      a,#0xD5     ; P1 TH = 0
        out     (0x3F),a
        nop
        nop
        in      a,(0xDC)    ; - - s a 0 0 d u
        and     a,#0x30     ; 0 0 s a 0 0 0 0
        add     a,a         ; 0 s a 0 0 0 0 0
        add     a,a         ; s a 0 0 0 0 0 0
        ld      c,a

        ld      a,#0xF5     ; P1 TH = 1
        out     (0x3F),a
        nop
        nop
        in      a,(0xDC)    ; - - c b r l d u
        and     a,#0x3F     ; 0 0 c b r l d u
        or      a,c         ; s a c b r l d u
        xor     a,#0xFF     ; S A C B R L D U

        ld      l,a         ; return value
        pop     ix
        ret

_read32:
        ld      a,#0x75     ; P2 TH = 0
        out     (0x3F),a
        nop
        nop
        in      a,(0xDD)    ; - - - - s a - -
        and     a,#0x0C     ; 0 0 0 0 s a 0 0
        add     a,a         ; 0 0 0 s a 0 0 0
        add     a,a         ; 0 0 s a 0 0 0 0
        add     a,a         ; 0 s a 0 0 0 0 0
        add     a,a         ; s a 0 0 0 0 0 0
        ld      c,a

        ld      a,#0xF5     ; P2 TH = 1
        out     (0x3F),a
        nop
        nop
        in      a,(0xDC)    ; d u - - - - - -
        and     a,#0xC0     ; d u 0 0 0 0 0 0
        rlca                ; u 0 0 0 0 0 0 d
        rlca                ; 0 0 0 0 0 0 d u
        ld      b,a
        in      a,(0xDD)    ; - - - - c b r l
        and     a,#0x0F     ; 0 0 0 0 c b r l
        add     a,a         ; 0 0 0 c b r l 0
        add     a,a         ; 0 0 c b r l 0 0
        or      a,b         ; 0 0 c b r l d u
        or      a,c         ; s a c b r l d u
        xor     a,#0xFF     ; S A C B R L D U

        ld      l,a         ; return value
        pop     ix
        ret

        .globl  _pad_get_6button
_pad_get_6button:
        ld      hl,#0
        ret

        .globl  _pad_get_mouse
_pad_get_mouse:
        ld      hl,#0
        ret


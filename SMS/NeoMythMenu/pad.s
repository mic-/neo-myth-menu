
        .area   _CODE

;; BYTE pad1_get_2button(void);
;; Reads buttons from the SMS pad in port 1.
;; Out: 0  0  S2 S1 R  L  D  U

        .globl  _pad1_get_2button
_pad1_get_2button:
        in      a,(0xDC)    ; -  -  s2 s1 r  l  d  u
        and     a,#0x3F     ; 0  0  s2 s1 r  l  d  u
        xor     a,#0x3F     ; 0  0  S2 S1 R  L  D  U

        ld      l,a         ; return value
        ret

;; BYTE pad2_get_2button(void);
;; Reads buttons from the SMS pad in port 2.
;; Out: 0  0  S2 S1 R  L  D  U

        .globl  _pad2_get_2button
_pad2_get_2button:
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
        ret

;; BYTE pad1_get_3button(void);
;; Reads buttons from the MD 3-button pad in port 1.
;; Out: S A C B R L D U

        .globl  _pad1_get_3button
_pad1_get_3button:
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
        ret

;; BYTE pad2_get_3button(void);
;; Reads buttons from the MD 3-button pad in port 2.
;; Out: S A C B R L D U

        .globl  _pad2_get_3button
_pad2_get_3button:
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
        ret

; do phase for pad for 6-button protocol

_pad1_phase:
        ld      a,#0xD5     ; P1 TH = 0
        out     (0x3F),a
        nop
        nop
        ld      a,#0xF5     ; P1 TH = 1
        out     (0x3F),a
        nop
        ret

;; WORD pad1_get_6button(void);
;; Reads buttons from the MD 6-button pad in port 1.
;; Out: 0 0 0 0 M X Y Z S A C B R L D U

        .globl  _pad1_get_6button
_pad1_get_6button:
        ; phase 1
        ; - - s a 0 0 d u   - - c b r l d u
        call    _pad1_phase

        ; phase 2
        ; - - s a 0 0 d u   - - c b r l d u
        call    _pad1_phase

        ; phase 3
        ; - - s a 0 0 0 0   - - c b m x y z
        call    _pad1_phase
        in      a,(0xDC)    ; - - c b m x y z
        and     a,#0x0F     ; 0 0 0 0 m x y z
        xor     a,#0x0F     ; 0 0 0 0 M X Y Z
        ld      h,a         ; high return value

        ; phase 4
        ; - - s a 1 1 1 1
        ld      a,#0xD5     ; P1 TH = 0
        out     (0x3F),a
        nop
        nop
        in      a,(0xDC)    ; - - s a 1 1 1 1
        and     a,#0x30     ; 0 0 s a 0 0 0 0
        add     a,a         ; 0 s a 0 0 0 0 0
        add     a,a         ; s a 0 0 0 0 0 0
        ld      c,a
        ; - - c b r l d u
        ld      a,#0xF5     ; P1 TH = 1
        out     (0x3F),a
        nop
        nop
        in      a,(0xDC)    ; - - c b r l d u
        and     a,#0x3F     ; 0 0 c b r l d u
        or      a,c         ; s a c b r l d u
        xor     a,#0xFF     ; S A C B R L D U

        ld      l,a         ; low return value
        ret

_pad2_phase:
        ld      a,#0x75     ; P1 TH = 0
        out     (0x3F),a
        nop
        nop
        ld      a,#0xF5     ; P1 TH = 1
        out     (0x3F),a
        nop
        ret

;; WORD pad2_get_6button(void);
;; Reads buttons from the MD 6-button pad in port 2.
;; Out: 0 0 0 0 M X Y Z S A C B R L D U

        .globl  _pad2_get_6button
_pad2_get_6button:
        ; phase 1
        ; - - s a 0 0 d u   - - c b r l d u
        call    _pad2_phase

        ; phase 2
        ; - - s a 0 0 d u   - - c b r l d u
        call    _pad2_phase

        ; phase 3
        ; - - s a 0 0 0 0   - - c b m x y z
        call    _pad2_phase
        in      a,(0xDC)    ; y z - - - - - -
        and     a,#0xC0     ; y z 0 0 0 0 0 0
        rlca                ; z 0 0 0 0 0 0 y
        rlca                ; 0 0 0 0 0 0 y z
        ld      c,a
        in      a,(0xDD)    ; - - - - c b m x
        and     a,#0x03     ; 0 0 0 0 0 0 m x
        add     a,a         ; 0 0 0 0 0 m x 0
        add     a,a         ; 0 0 0 0 m x 0 0
        or      a,c         ; 0 0 0 0 m x y z
        xor     a,#0x0F     ; 0 0 0 0 M X Y Z
        ld      h,a         ; high return value

        ; phase 4
        ; - - s a 1 1 1 1
        ld      a,#0x75     ; P2 TH = 0
        out     (0x3F),a
        nop
        nop
        in      a,(0xDD)    ; - - - - s a 1 1
        and     a,#0x0C     ; 0 0 0 0 s a 0 0
        add     a,a         ; 0 0 0 s a 0 0 0
        add     a,a         ; 0 0 s a 0 0 0 0
        add     a,a         ; 0 s a 0 0 0 0 0
        add     a,a         ; s a 0 0 0 0 0 0
        ld      c,a
        ; - - c b r l d u
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

        ld      l,a         ; low return value
        ret

        .globl  _pad1_get_mouse
_pad1_get_mouse:
        ld      hl,#0
        ret

        .globl  _pad2_get_mouse
_pad2_get_mouse:
        ld      hl,#0
        ret


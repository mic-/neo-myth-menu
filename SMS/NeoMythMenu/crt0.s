;--------------------------------------------------------------------------
;  crt0.s - Generic crt0.s for a Z80
;
;  Copyright (C) 2000, Michael Hope
;
;  This library is free software; you can redistribute it and/or modify it
;  under the terms of the GNU General Public License as published by the
;  Free Software Foundation; either version 2.1, or (at your option) any
;  later version.
;
;  This library is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this library; see the file COPYING. If not, write to the
;  Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
;   MA 02110-1301, USA.
;
;  As a special exception, if you link this library with other files,
;  some of which are compiled with SDCC, to produce an executable,
;  this library does not by itself cause the resulting executable to
;  be covered by the GNU General Public License. This exception does
;  not however invalidate any other reasons why the executable file
;   might be covered by the GNU General Public License.
;--------------------------------------------------------------------------

        .module crt0
        .globl  _main

        .area   _HEADER (ABS)
        ;; Reset vector
        .org    0
        di
        im      1
        jp      init

        ;; Restart vectors
        .org    0x08
        ret
        .org    0x10
        ret
        .org    0x18
        ret
        .org    0x20
        ret
        .org    0x28
        ret
        .org    0x30
        ret

        ;; Interrupt vector
        .org    0x38
        push    af
        in      a,(0xBF) ;; clears int and gives vdp status
        pop     af
        ei
        ret

        ;; NMI vector (Pause pressed)
        .org    0x66
        retn

        .org    0x100
init:
        ;; Stack at the top of memory minus shadow frame control bytes
        ld      sp,#0xdff0

        ;; Initialise global variables
        call    gsinit
        ;; Call main entry point
        call    _main
        jp      _exit

        ;; Ordering of segments for the linker.
        .area   _HOME
        .area   _CODE
        .area   _GSINIT
        .area   _GSFINAL

        .area   _DATA
        .area   _BSEG
        .area   _BSS
        .area   _HEAP

        .area   _CODE
__clock::
        ld      a,#2
        rst     0x08
        ret

_exit::
        ;; Exit - special code to the emulator
        ld      a,#0
        rst     0x08
1$:
        halt
        jr      1$

        .area   _GSINIT
gsinit::

        .area   _GSFINAL
        ret

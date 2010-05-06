.include "hdr.asm"

; Put this section in bank 2 ($010000-$017fff)
.BANK 2 SLOT 3 
.section ".rodata_bg" 

; The lower halves of each 64kB bank can only be accessed through the mirrors at
; bank+$40 and bank+$c0. That's why the base address is set to $400000 here.
.base $40

bg_patterns:
.incbin "assets/menu_bg.lzs"
bg_patterns_end:

bg_map:
.incbin "assets/menu_bg.nam"
bg_map_end:

bg_palette:
.incbin "assets/menu_bg.pal"

obj_marker:
.incbin "assets/marker.chr"

font:
.incbin "assets/adore.chr"

; We don't want any code/data generated by the C compiler to be put at $010000-$017fff,
; because that tends to screw up the addressing. So we fill up the rest of the (32kB) bank
; here. I haven't figured out a way to make the assembler calculate the size automatically,
; so I hardcoded it. 
.dsb 32768-11190,0


.ends
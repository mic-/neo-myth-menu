.include "hdr.asm"

.BANK 3 SLOT 0 
.section ".rodata_bg" 

bg_patterns:
.incbin "assets/menu_bg.lzs"
bg_patterns_end:

bg_map:
.incbin "assets/menu_bg.nam"
bg_map_end:

bg_palette:
.incbin "assets/menu_bg.pal"

.ends
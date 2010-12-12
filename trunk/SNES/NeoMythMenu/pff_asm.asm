.include "hdr.asm"
.accu 16
.index 16
.16bit
.define __pf_read_1mbit_to_psram_locals 64

.section ".text_pff_read_opt" superfree
pf_read_1mbit_to_psram_asm:
.ifgr __pf_read_1mbit_to_psram_locals 0
tsa
sec
sbc #__pf_read_1mbit_to_psram_locals
tas
.endif
lda.w FatFs + 0 + 2
sta.b tcc__r0h
lda.w FatFs + 0
sta.b tcc__r0
sta -4 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0h
sta -2 + __pf_read_1mbit_to_psram_locals + 1,s
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
lda.b tcc__r0 ; DON'T OPTIMIZE
bne +
brl __local_302
+
bra __local_303
__local_302:
lda.w #8
sta.b tcc__r0
jmp.w __local_304
__local_303:
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
inc a
inc a
sta.b tcc__r0
lda.w #0
sep #$20
lda.b [tcc__r0]
rep #$20
and.w #1
sta.b tcc__r1
lda.b tcc__r1 ; DON'T OPTIMIZE
bne +
brl __local_305
+
bra __local_306
__local_305:
lda.w #7
sta.b tcc__r0
jmp.w __local_307
__local_306:
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #32
sta.b tcc__r0
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1h
clc
lda.b tcc__r1
adc.w #28
sta.b tcc__r1
sta -8 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1h
sta -6 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b [tcc__r0]
sta.b tcc__r2
inc.b tcc__r0
inc.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r1
lda.b tcc__r2
sta -12 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r2h
sta -10 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1
sta -16 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1h
sta -14 + __pf_read_1mbit_to_psram_locals + 1,s
lda -8 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10
lda -6 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10h
lda.b [tcc__r10]
sta.b tcc__r0
lda -6 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1h
lda -8 + __pf_read_1mbit_to_psram_locals + 1,s
inc a
inc a
sta.b tcc__r1
lda.b [tcc__r1]
sta.b tcc__r2
lda -12 + __pf_read_1mbit_to_psram_locals + 1,s
sec
sbc.b tcc__r0
sta.b tcc__r1
lda -16 + __pf_read_1mbit_to_psram_locals + 1,s
sbc.b tcc__r2
sta.b tcc__r0
lda.b tcc__r1
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_remain + 0
lda.b tcc__r0
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_remain + 2
stz.b tcc__r0
lda.w #2
sta.b tcc__r1
lda.b tcc__r0
sta -20 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0h
sta -18 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1
sta -24 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1h
sta -22 + __pf_read_1mbit_to_psram_locals + 1,s
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_remain + 0
sta.b tcc__r2
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_remain + 2
sta.b tcc__r0
lda -24 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1
ldx #1
sec
sbc.b tcc__r0
tay
bcs ++
+ dex
++
stx.b tcc__r5
txa
bne +
brl __local_308
+
tya
bne __local_309
lda -20 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
ldx #1
sec
sbc.b tcc__r2
tay
beq +
bcs ++
+ dex
++
stx.b tcc__r5
txa
bne +
__local_308:
brl __local_310
+
__local_309:
lda.w #7
sta.b tcc__r0
jmp.w __local_311
__local_310:
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #40
sta.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r1
inc.b tcc__r0
inc.b tcc__r0
lda.b [tcc__r0]
pha
pei (tcc__r1)
jsr.l clust2sect
tsa
clc
adc #4
tas
lda.b tcc__r0
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect + 0
lda.b tcc__r1
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect + 2
lda.w #256
sta.b tcc__r0
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_numSects + 0
__local_334:
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_numSects + 0
sta.b tcc__r0
lda.b tcc__r0 ; DON'T OPTIMIZE
bne +
brl __local_312
+
lda 7 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda.b tcc__r0 ; DON'T OPTIMIZE
bne +
brl __local_313
+
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #28
sta.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r1
inc.b tcc__r0
inc.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r2
sta.b tcc__r0
lda.b tcc__r2h
sta.b tcc__r0h
lda.b tcc__r0
;ldy.w #9
;-
;lsr a
;dey
;bne -
xba
lsr a
and #$7F

+
sta.b tcc__r0
lda.b tcc__r2
;ldy.w #7
;-
;asl a
;dey
;bne -
asl a
asl a
asl a
asl a
asl a
asl a
asl a

+
sta.b tcc__r2
lda.b tcc__r1
;ldy.w #9
;-
;lsr a
;dey
;bne -
xba
lsr a
and #$7F
+
sta.b tcc__r1
ora.b tcc__r2
sta.b tcc__r2
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1h
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
inc a
sta.b tcc__r1
lda.w #0
sep #$20
lda.b [tcc__r1]
rep #$20
dec a
sta.b tcc__r3
sta.b tcc__r1
lda.b tcc__r3h
sta.b tcc__r1h
lda.b tcc__r1
;ldy.w #15
;-
;cmp #$8000
;ror a
;dey
;bne -
and #$8000
asl a
sbc #$ffff

+
sta.b tcc__r1
lda.b tcc__r2
and.b tcc__r3
sta.b tcc__r2
lda.b tcc__r0
and.b tcc__r1
sta.b tcc__r0
stz.b tcc__r1
lda.w #0
sta.b tcc__r3
ldx #1
lda.b tcc__r0
sec
sbc.b tcc__r3
tay
beq +
dex
+
stx.b tcc__r5
txa
bne +
brl __local_314
+
ldx #1
lda.b tcc__r2
sec
sbc.b tcc__r1
tay
beq +
dex
+
stx.b tcc__r5
txa
bne +
__local_314:
brl __local_315
+
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #28
sta.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r1
inc.b tcc__r0
inc.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r2
stz.b tcc__r0
lda.w #0
sta.b tcc__r3
ldx #1
lda.b tcc__r2
sec
sbc.b tcc__r3
tay
beq +
dex
+
stx.b tcc__r5
txa
bne +
brl __local_316
+
ldx #1
lda.b tcc__r1
sec
sbc.b tcc__r0
tay
beq +
dex
+
stx.b tcc__r5
txa
bne +
__local_316:
brl __local_317
+
lda #1
bra +
__local_317:
lda #0
+
sta.b tcc__r0
lda.b tcc__r0 ; DON'T OPTIMIZE
bne +
brl __local_318
+
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #36
sta.b tcc__r0
jmp.w __local_319
__local_318:
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #40
sta.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r1
inc.b tcc__r0
inc.b tcc__r0
lda.b [tcc__r0]
pha
pei (tcc__r1)
jsr.l get_fat
tsa
clc
adc #4
tas
lda.b tcc__r0
sta -28 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1
sta -26 + __pf_read_1mbit_to_psram_locals + 1,s
lda -28 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
stz.b tcc__r1h
tsa
clc
adc #(-28 + __pf_read_1mbit_to_psram_locals + 1) + 2
sta.b tcc__r1
sta -32 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1h
sta -30 + __pf_read_1mbit_to_psram_locals + 1,s
lda -32 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10
lda -30 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10h
lda.b [tcc__r10]
sta.b tcc__r1
bra __local_320
__local_319:
lda.b tcc__r0
sta -36 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0h
sta -34 + __pf_read_1mbit_to_psram_locals + 1,s
lda -36 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10
lda -34 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10h
lda.b [tcc__r10]
sta.b tcc__r0
lda -34 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1h
lda -36 + __pf_read_1mbit_to_psram_locals + 1,s
inc a
inc a
sta.b tcc__r1
sta -40 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1h
sta -38 + __pf_read_1mbit_to_psram_locals + 1,s
lda -40 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10
lda -38 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10h
lda.b [tcc__r10]
sta.b tcc__r1
__local_320:
lda.b tcc__r0
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_clst + 0
lda.b tcc__r1
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_clst + 2
;lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_clst + 0
;sta.b tcc__r0
;lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_clst + 2
;sta.b tcc__r1
lda.w #1
sta.b tcc__r2
lda.w #0
sta.b tcc__r3
ldx #1
lda.b tcc__r1
sec
sbc.b tcc__r3
tay
beq ++
bcc ++
+ dex
++
stx.b tcc__r5
txa
bne +
brl __local_321
+
tya
bne __local_322
ldx #1
lda.b tcc__r0
sec
sbc.b tcc__r2
tay
beq ++
bcc ++
+ dex
++
stx.b tcc__r5
txa
bne +
__local_321:
brl __local_323
+
__local_322:
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
inc a
inc a
sta.b tcc__r0
lda.w #0
sta.b tcc__r1
sep #$20
sta.b [tcc__r0]
rep #$20
lda.w #1
sta.b tcc__r0
jmp.w __local_324
__local_323:
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #40
sta.b tcc__r0
sta -44 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0h
sta -42 + __pf_read_1mbit_to_psram_locals + 1,s
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_clst + 0
sta.b tcc__r1
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_clst + 2
sta.b tcc__r0
lda -44 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r2
lda -42 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r2h
lda.b tcc__r1
sta.b [tcc__r2]
inc.b tcc__r2
inc.b tcc__r2
lda.b tcc__r0
sta.b [tcc__r2]
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #3
sta.b tcc__r0
lda.w #0
sep #$20
sta.b [tcc__r0]
rep #$20
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #40
sta.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r1
inc.b tcc__r0
inc.b tcc__r0
lda.b [tcc__r0]
pha
pei (tcc__r1)
jsr.l clust2sect
tsa
clc
adc #4
tas
lda.b tcc__r0
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect + 0
lda.b tcc__r1
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect + 2
__local_315:
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect + 0
sta.b tcc__r0
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect + 2
sta.b tcc__r1
ora.b tcc__r0
bne +
brl __local_325
+
bra __local_326
__local_325:
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
inc a
inc a
sta.b tcc__r0
lda.w #0
sta.b tcc__r1
sep #$20
sta.b [tcc__r0]
rep #$20
lda.w #1
sta.b tcc__r0
jmp.w __local_327
__local_326:
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #44
sta.b tcc__r0
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1h
clc
lda.b tcc__r1
adc.w #3
sta.b tcc__r1
lda.w #0
sep #$20
lda.b [tcc__r1]
rep #$20
sta.b tcc__r2
sta.b tcc__r1
lda.b tcc__r2h
sta.b tcc__r1h
lda.b tcc__r1
;ldy.w #15
;-
;cmp #$8000
;ror a
;dey
;bne -
and #$8000
asl a
sbc #$ffff

+
sta.b tcc__r1
lda.b tcc__r0
sta -48 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0h
sta -46 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r2
sta -52 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r1
sta -50 + __pf_read_1mbit_to_psram_locals + 1,s
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect + 0
sta.b tcc__r3
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect + 2
sta.b tcc__r0
lda.b tcc__r3
sta -56 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r3h
sta -54 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0
sta -60 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0h
sta -58 + __pf_read_1mbit_to_psram_locals + 1,s
lda -52 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1
stz.b tcc__r0h
tsa
clc
adc #(-52 + __pf_read_1mbit_to_psram_locals + 1) + 2
sta.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r2
lda -56 + __pf_read_1mbit_to_psram_locals + 1,s
clc
adc.b tcc__r1
sta.b tcc__r0
lda -60 + __pf_read_1mbit_to_psram_locals + 1,s
adc.b tcc__r2
sta.b tcc__r1
lda -48 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r2
lda -46 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r2h
lda.b tcc__r0
sta.b [tcc__r2]
inc.b tcc__r2
inc.b tcc__r2
lda.b tcc__r1
sta.b [tcc__r2]
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #3
sta.b tcc__r0
lda.w #0
sep #$20
lda.b [tcc__r0]
rep #$20
sta.b tcc__r1
sta.b tcc__r2
lda.b tcc__r1h
sta.b tcc__r2h
inc.b tcc__r1
sep #$20
lda.b tcc__r1
sta.b [tcc__r0]
rep #$20
__local_313:
lda.w #1
sta 7 + __pf_read_1mbit_to_psram_locals + 1,s
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #44
sta.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r1
inc.b tcc__r0
inc.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r2
pha
pei (tcc__r1)
lda 9 + __pf_read_1mbit_to_psram_locals + 1,s
pha
lda 9 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
pha
lda.l disk_readsect_psram + 0
sta.b tcc__r10
lda.l disk_readsect_psram + 0 + 2
sta.b tcc__r10h
jsr.l tcc__jsl_r10
tsa
clc
adc #8
tas
lda.b tcc__r0
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_dr + 0
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_dr + 0
sta.b tcc__r0
lda.b tcc__r0 ; DON'T OPTIMIZE
bne +
brl __local_328
+
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
inc a
inc a
sta.b tcc__r0
lda.w #0
sta.b tcc__r1
sep #$20
sta.b [tcc__r0]
rep #$20
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_dr + 0
sta.b tcc__r0
cmp #2
beq +
brl __local_329
+
bra __local_330
__local_329:
lda.w #1
sta.b tcc__r0
bra __local_331
__local_330:
lda.w #6
sta.b tcc__r0
__local_331:
jmp.w __local_332
__local_328:
lda 5 + __pf_read_1mbit_to_psram_locals + 1,s
clc
adc.w #512
sta 5 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
cmp #0
beq +
brl __local_333
+
lda 3 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r1
ina
sta.b tcc__r0
sta 3 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0h
sta.b tcc__r1h
__local_333:
lda -4 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -2 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
clc
lda.b tcc__r0
adc.w #28
sta.b tcc__r0
sta -64 + __pf_read_1mbit_to_psram_locals + 1,s
lda.b tcc__r0h
sta -62 + __pf_read_1mbit_to_psram_locals + 1,s
lda -64 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10
lda -62 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r10h
lda.b [tcc__r10]
sta.b tcc__r1
lda -62 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
lda -64 + __pf_read_1mbit_to_psram_locals + 1,s
inc a
inc a
sta.b tcc__r0
lda.b [tcc__r0]
sta.b tcc__r2
lda.w #512
sta.b tcc__r0
lda.w #0
sta.b tcc__r3
clc
lda.b tcc__r1
adc.b tcc__r0
sta.b tcc__r1
lda.b tcc__r2
adc.b tcc__r3
sta.b tcc__r2
lda -64 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0
lda -62 + __pf_read_1mbit_to_psram_locals + 1,s
sta.b tcc__r0h
lda.b tcc__r1
sta.b [tcc__r0]
inc.b tcc__r0
inc.b tcc__r0
lda.b tcc__r2
sta.b [tcc__r0]
lda.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_numSects + 0
sta.b tcc__r0
sta.b tcc__r1
lda.b tcc__r0h
sta.b tcc__r1h
lda.b tcc__r0
dea
sta.b tcc__r0
sta.w __tccs__FUNC_pf_read_1mbit_to_psram_asm_numSects + 0
jmp.w __local_334
__local_312:
lda.w #0
sta.b tcc__r0
__local_304:
__local_307:
__local_311:
__local_324:
__local_327:
__local_332:
__local_335:
.ifgr __pf_read_1mbit_to_psram_locals 0
tsa
clc
adc #__pf_read_1mbit_to_psram_locals
tas
.endif
rtl
.ends

.ramsection ".bss" bank $7e slot 2
__tccs__FUNC_pf_read_1mbit_to_psram_asm_dr dsb 2
__tccs__FUNC_pf_read_1mbit_to_psram_asm_clst dsb 4
__tccs__FUNC_pf_read_1mbit_to_psram_asm_basesect dsb 4
__tccs__FUNC_pf_read_1mbit_to_psram_asm_sect dsb 4
__tccs__FUNC_pf_read_1mbit_to_psram_asm_remain dsb 4
__tccs__FUNC_pf_read_1mbit_to_psram_asm_numSects dsb 2
.ends

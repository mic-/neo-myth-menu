	.area   _CODE
	.globl _vdp_set_vram_addr		;;temp

	;;void enable_ints(void)
	.globl _enable_ints
	_enable_ints:
	ei
	ret

	;;void _disable_ints(void)
	.globl _disable_ints
	_disable_ints:
	di
	ret

	;;void puts_asm(const char *str,BYTE attributes,WORD vram_addr);
	.globl _puts_asm
	_puts_asm:
	di
		push ix		
		ld ix,#0	;;ix=sp
		add ix,sp	;;..

		ld l,2 + 5(ix)	;;lo(hl)
		ld h,2 + 6(ix) 	;;hi(hl)

		push hl		;;save
		call _vdp_set_vram_addr 
		pop hl		;;restore

		ld l,2 + 2(ix)	;;str
		ld h,2 + 3(ix)	;;..
		ld b,2 + 4(ix)	;;attrs
		sla b			;;<<=1

	_puts_asm_output:
		ld a,(hl)		;;*str
		or a			;;z-tst
		jr z,_puts_asm_output_done	;;zero
		sub a,#0x20		;;-=' '
		out (#0xbe),a	;;w
		ld a,b			;;attr
		out (#0xbe),a	;;w
		inc hl			;;++str
		jr _puts_asm_output	;;busy

	_puts_asm_output_done:

	pop ix
	ei
	ret

	;;void putsn_asm(const char *str,BYTE attributes,BYTE max_chars,WORD vram_addr);
	.globl _putsn_asm
	_putsn_asm:
	di
		push ix
		ld ix,#0	;;ix=sp
		add ix,sp	;;..

		ld l,2 + 6(ix)	;;lo(hl)
		ld h,2 + 7(ix) 	;;hi(hl)

		push hl		;;save
		call _vdp_set_vram_addr 
		pop hl		;;restore

		ld l,2 + 2(ix)	;;str
		ld h,2 + 3(ix)	;;..
		ld b,2 + 4(ix)	;;attrs
		ld c,2 + 5(ix)	;;max
		sla b			;;<<=1
	
	_putsn_asm_output:
		or c			;;z-tst
		jr z,_puts_asm_output_done	;;zero
		ld a,(hl)		;;*str
		or a			;;z-tst
		jr z,_puts_asm_output_done	;;zero
		sub a,#0x20		;;-=' '
		out (#0xbe),a	;;w
		ld a,b			;;attr
		out (#0xbe),a	;;w
		inc hl			;;++str
		dec c			;;--left
		jr _puts_asm_output	;;busy

	_putsn_asm_output_done:

	pop ix
	ei
	ret

	;BYTE strlen_asm(const BYTE* str)
	.globl _strlen_asm
	_strlen_asm:
	
	push ix		;;save
	ld ix,#4	;;rel addr in stack
	add ix,sp	;;+=sp
	ld l,(ix)	;;lo(hl)
	ld h,1(ix)	;;hi(hl)
	xor a		;;z(A)

	strlen_asm_loop:
	ld b,(hl)	;;*str
	or b		;;ztst
	jr z,strlen_asm_done
	inc hl		;;++str
	inc a		;;++r
	jr strlen_asm_loop

	strlen_asm_done:
	ld l,a		;;l = res
	pop ix		;;restore
	ret



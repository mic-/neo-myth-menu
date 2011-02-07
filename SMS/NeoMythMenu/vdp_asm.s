	.area   _CODE

	.globl _vdp_set_vram_addr
	_vdp_set_vram_addr:
	push	ix		;;save
	ld	ix,#4		;;addr rel offs
	add	ix,sp		;;+=sp
	ld a,(ix)		;;lb
	out (0xbf),a	;;w
	ld a,1(ix)		;;hb
	or a,#0x40		;;CMD_VRAM_WRITE
	out (0xbf),a	;;w
	pop ix			;;restore
	ret


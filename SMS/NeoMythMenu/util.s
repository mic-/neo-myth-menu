    .area   _CODE
    .globl _vdp_set_vram_addr       ;;temp

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
    push        ix
        ld      ix,#4       ;;ix=sp
        add     ix,sp       ;;..

        ld      l,3(ix) ;;lo(hl)
        ld      h,4(ix) ;;hi(hl)

        ;;push        hl          ;;save
            call    _vdp_set_vram_addr
        ;;pop         hl          ;;restore

        ld      l,(ix) ;;str
        ld      h,1(ix) ;;..
        ld      a,2(ix) ;;attrs
        add     a
        ld      b,a
        ld      c,#0xbe

    puts_asm_output:
        ld      a,(hl)      ;;*str
        or      a           ;;z-tst
        jp      z,puts_asm_output_done  ;;zero
        sub     #0x20       ;;-=' '
        out     (c),a       ;;w
		push	ix
		pop		ix
        out     (c),b       ;;attr
        inc     hl          ;;++str
        jp      puts_asm_output ;;busy

    puts_asm_output_done:
    pop     ix
    ei
    ret

    ;;void putsn_asm(const char *str,BYTE attributes,BYTE max_chars,WORD vram_addr);
    .globl _putsn_asm
    _putsn_asm:
    di
    push        ix
        ld      ix,#4   ;;ix=sp
        add     ix,sp   ;;..
        ld      l,4(ix)     ;;lo(hl)
        ld      h,5(ix)     ;;hi(hl)

        ;;push        hl      ;;save
            call    _vdp_set_vram_addr
        ;;pop         hl      ;;restore

        ld      l,(ix) ;;str
        ld      h,1(ix) ;;..
        ld      b,2(ix) ;;attrs
        ld      a,3(ix) ;;max
        add     a
        ld      b,a
        ld      c,#0xbe

    putsn_asm_output:
        xor     a
        or      c
        jp      z,putsn_asm_output_done
        ld      a,(hl)      ;;*str
        or      a
        jp      z,putsn_asm_output_done
        sub     a,#0x20     ;;-=' '
        out     (c),a       ;;w
		push	ix
		pop		ix
        out     (c),b       ;;w
        inc     hl          ;;++str
        dec     c           ;;--left
        jp      nz,puts_asm_output

    putsn_asm_output_done:
    pop         ix
    ei
    ret

    ;;void strcpy_asm(BYTE* dst,const BYTE* src);
    .globl _strcpy_asm
    _strcpy_asm:
    push            ix
            ld      ix,#4                   ;;2 + stack depth * sizeof word
            add     ix,sp                   ;;+=sp
            ld      e,(ix)                  ;;dst
            ld      d,1(ix)                 ;;...
            ld      l,2(ix)                 ;;src
            ld      h,3(ix)                 ;;...

        strcpy_asm_loop:
            ld          a,(hl)              ;;*src
            ldi
            or          a                   ;;ztst
            jp          nz,strcpy_asm_loop  ;;loop

    pop             ix
    ret

    ;;void strncpy_asm(BYTE* dst,const BYTE* src,BYTE cnt);
    .globl _strncpy_asm
    _strncpy_asm:
    push                ix
                ld      ix,#4                   ;;2 + stack depth * sizeof word
                add     ix,sp                   ;;+=sp
                ld      e,(ix)                  ;;dst
                ld      d,1(ix)                 ;;...
                ld      l,2(ix)                 ;;src
                ld      h,3(ix)                 ;;...
                ld      b,4(ix)                 ;;cnt
                xor     a

                ;;      check if zero to avoid wrapping bc in ldir
                or      b                       ;;tst len
                jp      z,strncpy_asm_done      ;;zr
                xor     c                       ;;z(c)

                ldir                            ;;21cycles * iterations(->bc)

            strncpy_asm_done:                   ;;even if len == 0 null terminate string
                ex          de,hl               ;;de = hl,hl = de
                ld          (hl),#0x00          ;;null-terminate
    pop                     ix
    ret

    ;;void strcat_asm(BYTE* dst,const BYTE* src);
    .globl _strcat_asm
    _strcat_asm:
    push            ix
            ld      ix,#4                   ;;2 + stack depth * sizeof word
            add     ix,sp                   ;;+=sp
            ld      l,(ix)                  ;;dst
            ld      h,1(ix)                 ;;...
            ld      e,2(ix)                 ;;src
            ld      d,3(ix)                 ;;...
            xor     a

        strcat_asm_z_loop:                  ;;find zero in dst
            cpi
            jp      nz,strcat_asm_z_loop
            dec     hl
            ex      de,hl

        strcat_asm_loop:
            ld          a,(hl)                  ;;*src
            ldi
            or          a
            jp          nz,strcat_asm_loop     ;;loop
    pop                 ix
    ret

    ;;void strncat_asm(BYTE* dst,const BYTE* src,BYTE cnt);
    .globl _strncat_asm
    _strncat_asm:
    push                ix
                ld      ix,#4                   ;;2 + stack depth * sizeof word
                add     ix,sp                   ;;+=sp
                ld      l,(ix)                  ;;dst
                ld      h,1(ix)                 ;;...
                ld      e,2(ix)                 ;;src
                ld      d,3(ix)                 ;;...
                ld      b,4(ix)                 ;;cnt
                xor     a

            strncat_asm_z_loop:                 ;;find zero in dst
                cpi
                jp      nz,strncat_asm_z_loop
                dec     hl
                ex      de,hl

            strncat_asm_loop:
                ldir
    pop                 ix
    ret

    ;;extern const char* get_file_extension_asm(const BYTE* src)
    .globl _get_file_extension_asm
    _get_file_extension_asm:
    push            ix                                      ;;save                                      ;;save
            ld      ix,#4                                   ;;2 + stack depth * sizeof word
            add     ix,sp                                   ;;+=sp
            ld      l,(ix)                                  ;;src
            ld      h,1(ix)                                 ;;...

            get_file_extension_asm_loop:                    ;;Will only look for a single dot (for now)
            ld      a,(hl)
            or      a
            jp      z,get_file_extension_asm_nothing_found
            sub     a,#0x2e                                 ;;-='.'
            jp      z,get_file_extension_asm_found_dot
            inc     hl
            jp      get_file_extension_asm_loop

            get_file_extension_asm_nothing_found:
            xor     a
            xor     h
            xor     l
            jp      get_file_extension_asm_done

            get_file_extension_asm_found_dot:
            inc     hl                                      ;;move over dot

            get_file_extension_asm_done:
    pop             ix                                  ;;restore
    ret

    ;;BYTE strlen_asm(const BYTE* str)
    .globl _strlen_asm
    _strlen_asm:

	xor			a
	ld			e,#0

    strlen_asm_loop:
		inc		e
		cpi
        jp     nz,strlen_asm_loop

	dec			e
	ld			l,e
    ret

	;;BYTE memcmp_asm(const BYTE* a,const BYTE* b,WORD len)
	.globl			_vregs
	.globl			_memcmp_asm
	_memcmp_asm:
    push            ix                                      
            ld      ix,#4                                  
            add     ix,sp                                   
            ld      l,(ix)        				;;a                          
            ld      h,1(ix) 					;;.
			ld		e,2(ix)						;;b
			ld		d,3(ix)						;;.
			ld		c,4(ix)						;;len
			ld		b,5(ix)						;;....

			xor		a							;;flag to vreg
			ld		(_vregs+0),a				;;............

			memcmp_asm_loop:
			ld		a,(hl)						;;save *a to vreg1
			ld		(_vregs+1),a				;;...............
			ld		a,b							;;save b to vreg2
			ld		(_vregs+2),a				;;...............
			ld		a,(de)						;;b = *b
			ld		b,a							;;.....
			ld		a,(_vregs+1)				;;r = a - vr[1]
			sub		a,b							;;.............
			ld		(_vregs+0),a				;;save to vr[0]
			or		a							;;test
			jp		nz,memcmp_asm_done_skip_load
			ld		a,(_vregs+2)				;;b = vr[2]
			ld		b,a							;;.........
			inc		de
			cpi									;;dec bc + inc hl in one op
			ld		a,b
			or		c
			jp		nz,memcmp_asm_loop

			memcmp_asm_done:
			ld		a,(_vregs+0)				;;flag in vreg

			memcmp_asm_done_skip_load:
			ld		l,a

            memcmp_asm_skip_load:
	pop				ix
	ret

    ;;void memcpy_asm(BYTE* dst,const BYTE* src,WORD size);
    .globl _memcpy_asm
    _memcpy_asm:
    push                ix
                ld      ix,#4                   ;;2 + stack depth * sizeof word
                add     ix,sp                   ;;+=sp
                ld      e,(ix)                  ;;dst
                ld      d,1(ix)                 ;;...
                ld      l,2(ix)                 ;;src
                ld      h,3(ix)                 ;;...
                ld      c,4(ix)                 ;;cnt
                ld      b,5(ix)                 ;;...
                ldir                            ;;write all (21cycles/iter)
    pop                 ix
    ret

    ;;void memset_asm(char* dst,char val,WORD size);
    .globl _memset_asm
    _memset_asm:
    push                ix
                ld      ix,#4                   ;;2 + stack depth * sizeof word
                add     ix,sp                   ;;+=sp
                ld      l,(ix)                  ;;dst
                ld      h,1(ix)                 ;;...
                ld      a,2(ix)                 ;;val
                ld      c,3(ix)                 ;;cnt
                ld      b,4(ix)                 ;;...

            memset_loop:
                ld      (hl),a                  ;;7cycles
                cpi                             ;;16cycles
                jp      pe,memset_loop          ;;10cycles
    pop                 ix
    ret








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
    push        bc
        ld      ix,#2       ;;ix=sp
        add     ix,sp       ;;..

        ld      l,2 + 5(ix) ;;lo(hl)
        ld      h,2 + 6(ix) ;;hi(hl)

        push        hl          ;;save
            call    _vdp_set_vram_addr
        pop         hl          ;;restore

        ld      l,2 + 2(ix) ;;str
        ld      h,2 + 3(ix) ;;..
        ld      b,2 + 4(ix) ;;attrs
        sla     b           ;;<<=1

    puts_asm_output:
        ld      a,(hl)      ;;*str
        or      a           ;;z-tst
        jp      z,puts_asm_output_done  ;;zero
        sub     a,#0x20     ;;-=' '
        out     (#0xbe),a   ;;w
        ld      a,b         ;;attr
        out     (#0xbe),a   ;;w
        inc     hl          ;;++str
        jp      puts_asm_output ;;busy

    puts_asm_output_done:
    pop     bc
    pop     ix
    ei
    ret

    ;;void putsn_asm(const char *str,BYTE attributes,BYTE max_chars,WORD vram_addr);
    .globl _putsn_asm
    _putsn_asm:
    di
    push        ix
    push        bc
        ld      ix,#2   ;;ix=sp
        add     ix,sp   ;;..

        ld      l,2 + 6(ix)     ;;lo(hl)
        ld      h,2 + 7(ix)     ;;hi(hl)

        push        hl      ;;save
            call    _vdp_set_vram_addr
        pop         hl      ;;restore

        ld      l,2 + 2(ix) ;;str
        ld      h,2 + 3(ix) ;;..
        ld      b,2 + 4(ix) ;;attrs
        ld      c,2 + 5(ix) ;;max
        sla     b           ;;<<=1

    putsn_asm_output:
        xor     a
        or      c
        jp      z,putsn_asm_output_done
        ld      a,(hl)      ;;*str
        or      a
        jp      z,putsn_asm_output_done
        sub     a,#0x20     ;;-=' '
        out     (#0xbe),a   ;;w
        ld      a,b         ;;attr
        out     (#0xbe),a   ;;w
        inc     hl          ;;++str
        dec     c           ;;--left
        jp      nz,puts_asm_output

    putsn_asm_output_done:
    pop         bc
    pop         ix
    ei
    ret

    ;;void strcpy_asm(BYTE* dst,const BYTE* src);
    .globl _strcpy_asm
    _strcpy_asm:
    push            ix
        push        de
            ld      ix,#6                   ;;2 + stack depth * sizeof word
            add     ix,sp                   ;;+=sp
            ld      l,(ix)                  ;;dst
            ld      h,1(ix)                 ;;...
            ld      e,2(ix)                 ;;src
            ld      d,3(ix)                 ;;...

        strcpy_asm_loop:
            ld          a,(de)              ;;*src
            or          a                   ;;ztst
            jp          z,strcpy_asm_done   ;;zr
            ld          (hl),a              ;;*dst=*src
            inc         hl                  ;;++dst
            inc         de                  ;;++src
            jp          strcpy_asm_loop     ;;loop

        strcpy_asm_done:
            ld          (hl),#0x00          ;;null-terminate
        pop         de
    pop             ix
    ret

    ;;void strncpy_asm(BYTE* dst,const BYTE* src,BYTE cnt);
    .globl _strncpy_asm
    _strncpy_asm:
    push                ix
        push            de
            push        bc
                ld      ix,#8                   ;;2 + stack depth * sizeof word
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

                ;;de = dst,hl = src,bc = count
                ;;while(bc--)*(de++) = *(hl++)
                ldir                            ;;21cycles * iterations(->bc)

            strncpy_asm_done:                   ;;even if len == 0 null terminate string
                ex          de,hl               ;;de = hl,hl = de
                ld          (hl),#0x00          ;;null-terminate
            pop         bc
        pop             de
    pop                 ix
    ret

    ;;void strcat_asm(BYTE* dst,const BYTE* src);
    .globl _strcat_asm
    _strcat_asm:
    push            ix
        push        de
            ld      ix,#6                   ;;2 + stack depth * sizeof word
            add     ix,sp                   ;;+=sp
            ld      l,(ix)                  ;;dst
            ld      h,1(ix)                 ;;...
            ld      e,2(ix)                 ;;src
            ld      d,3(ix)                 ;;...

        strcat_asm_z_loop:                  ;;find zero in dst
            ld          a,(hl)
            or          a
            jp          z,strcat_asm_loop
            inc         hl
            jp          strcat_asm_z_loop

        strcat_asm_loop:
            ld          a,(de)              ;;*src
            or          a                   ;;ztst
            jp          z,strcat_asm_done   ;;zr
            ld          (hl),a              ;;*dst=*src
            inc         hl                  ;;++dst
            inc         de                  ;;++src
            jp          strcat_asm_loop     ;;loop

        strcat_asm_done:
            ld          (hl),#0x00          ;;null-terminate
        pop         de
    pop             ix
    ret

    ;;void strncat_asm(BYTE* dst,const BYTE* src,BYTE cnt);
    .globl _strncat_asm
    _strncat_asm:
    push                ix
        push            de
            push        bc
                ld      ix,#8                   ;;2 + stack depth * sizeof word
                add     ix,sp                   ;;+=sp
                ld      l,(ix)                  ;;dst
                ld      h,1(ix)                 ;;...
                ld      e,2(ix)                 ;;src
                ld      d,3(ix)                 ;;...
                ld      b,4(ix)                 ;;cnt
                xor     a

                ;;      check if zero to avoid wrapping bc in ldir
                or      b
                jp      z,strncat_asm_done
                xor     c

            strncat_asm_z_loop:                 ;;find zero in dst
                ld          a,(hl)
                or          a
                jp          z,strncat_asm_loop
                inc         hl
                jp          strncat_asm_z_loop

            strncat_asm_loop:
                ex          de,hl
                ldir
                ex          de,hl

            strncat_asm_done:
                ld          (hl),#0x00          ;;null-terminate
            pop             bc
        pop                 de
    pop                     ix
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

    push        ix      ;;save
        ld      ix,#4   ;;rel addr in stack
        add     ix,sp   ;;+=sp
        ld      l,(ix)  ;;lo(hl)
        ld      h,1(ix) ;;hi(hl)
        xor     a       ;;z(A)

    strlen_asm_loop:
        ld      b,(hl)  ;;*str
        ld      c,a
        xor     a
        or      b       ;;ztst
        ld      a,c
        jp      z,strlen_asm_done
        inc     hl      ;;++str
        inc     a       ;;++r
        jp      strlen_asm_loop

    strlen_asm_done:
        ld      l,a     ;;l = res
    pop         ix      ;;restore
    ret

    ;;TODO : strcmp_asm

    ;;void memcpy_asm(BYTE* dst,const BYTE* src,WORD size);
    .globl _memcpy_asm
    _memcpy_asm:
    push                ix
        push            de
            push        bc
                ld      ix,#8                   ;;2 + stack depth * sizeof word
                add     ix,sp                   ;;+=sp
                ld      e,(ix)                  ;;dst
                ld      d,1(ix)                 ;;...
                ld      l,2(ix)                 ;;src
                ld      h,3(ix)                 ;;...
                ld      c,4(ix)                 ;;cnt
                ld      b,5(ix)                 ;;...
                ldir                            ;;write all (21cycles/iter)
            pop         bc
        pop             de
    pop                 ix
    ret

    ;;void memset_asm(char* dst,char val,WORD size);
    .globl _memset_asm
    _memset_asm:
    push                ix
        push            de
            push        bc
                ld      ix,#8                   ;;2 + stack depth * sizeof word
                add     ix,sp                   ;;+=sp
                ld      l,(ix)                  ;;dst
                ld      h,1(ix)                 ;;...
                ld      d,2(ix)                 ;;val
                ld      c,3(ix)                 ;;cnt
                ld      b,4(ix)                 ;;...
            memset_loop:
                ld      (hl),d
                inc     hl
                dec     bc
                ld      a,b
                or      c
                jp      nz,memset_loop

            pop         bc
        pop             de
    pop                 ix
    ret



   	
    
    	
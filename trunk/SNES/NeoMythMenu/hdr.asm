.MEMORYMAP                      ; Begin describing the system architecture.
  SLOTSIZE $8000                ; The slot is $8000 bytes in size. More details on slots later.
  DEFAULTSLOT 0                 ; There's only 1 slot in SNES, there are more in other consoles.
  SLOT 0 $8000                  ; Defines Slot 0's starting address.
  SLOT 1 $0 $2000
  SLOT 2 $2000 $E000
  SLOT 3 $0 $10000
.ENDME          ; End MemoryMap definition

.ROMBANKSIZE $8000              ; Every ROM bank is 32 KBytes in size
.ROMBANKS 8                     ; 1 Mbit - Tell WLA we want to use 4 ROM Banks

.SNESHEADER
  ID "SNES"                     ; 1-4 letter string, just leave it as "SNES"
  
  NAME "NEO TEAM (C) 2010    "  ; Program Title - can't be over 21 bytes,
  ;    "123456789012345678901"  ; use spaces for unused bytes of the name.

  SLOWROM
  HIROM

  CARTRIDGETYPE $00             ; $00 = ROM only, see WLA documentation for others
  ROMSIZE $04                   ; $08 = 2 Mbits,  see WLA doc for more..
  SRAMSIZE $00                  ; No SRAM         see WLA doc for more..
  COUNTRY $01                   ; $01 = U.S.  $00 = Japan, that's all I know
  LICENSEECODE $00              ; Just use $00
  VERSION $00                   ; $00 = 1.00, $01 = 1.01, etc.
.ENDSNES

.SNESNATIVEVECTOR               ; Define Native Mode interrupt vector table
  COP EmptyHandler
  BRK EmptyHandler
  ABORT EmptyHandler
  NMI VBlank
  IRQ EmptyHandler
.ENDNATIVEVECTOR

.SNESEMUVECTOR                  ; Define Emulation Mode interrupt vector table
  COP EmptyHandler
  ABORT EmptyHandler
  NMI EmptyHandler
  RESET tcc__start                   ; where execution starts
  IRQBRK EmptyHandler
.ENDEMUVECTOR

.BANK 0 SLOT 0                  ; Defines the ROM bank and the slot it is inserted in memory.
.ORG 0                          ; .ORG 0 is really $8000, because the slot starts at $8000
.IFDEF EMULATOR
.db $AD, $3F, $21, $29, $10			; lda $213f / and #$10
.db $AF, $3F, $21, $00, $89, $10		; lda.l $00213f / bit #$10
.db $AD, $3F, $21, $89, $10			; lda $213f / bit #$10
.db $AF, $3F, $21, $00, $29, $10		; lda.l $00213f / and #$10

.db $AD, $3F, $21, $29, $10			; lda $213f / and #$10
.db $AF, $3F, $21, $00, $89, $10		; lda.l $00213f / bit #$10
.db $AD, $3F, $21, $89, $10			; lda $213f / bit #$10
.db $AF, $3F, $21, $00, $29, $10	

.DSB $8000-44,0

.ELSE

.DSB $8000,0
.ENDIF

;.BANK 2 SLOT 0                  ; Defines the ROM bank and the slot it is inserted in memory.
;.ORG 0  
;.DSB $8000,0					; Fill up $010000-$017FFF

.BANK 4 SLOT 0                  ; Defines the ROM bank and the slot it is inserted in memory.
.ORG 0  
.DSB $8000,0					; Fill up $020000-$027FFF

.BANK 6 SLOT 0                  ; Defines the ROM bank and the slot it is inserted in memory.
.ORG 0  
.DSB $8000,0					; Fill up $030000-$037FFF

.bank 1 slot 0
.org 0

.base 0

.EMPTYFILL $00                  ; fill unused areas with $00, opcode for BRK.  
                                ; BRK will crash the snes if executed.

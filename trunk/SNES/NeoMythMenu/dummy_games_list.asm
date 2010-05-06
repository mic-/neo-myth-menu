.include "hdr.asm"

	
	.BANK 1
	.ORG	$4000
	.DSB	$800,0

;  XY
; $00 IF FF HAVE GAME
; $01 ,MODE 04 >SNES GAME
; $02 ,RBH  XY >X= ROM SIZE & Y = A25 ~A24
; $03 ,RBL     >A23~A16
; $04 ,SRAM XY >X SRAM BANK & Y = SRAM SIZE
; $05 ,EXT  P  >X = EXT DSP & Y = EXT SAVE
; $06 ,RUX      X = SAVE MODE Y = RUN MODE
;
	.BANK 1
        .ORG    $4800
;                 0   1   2   3   4   5   6   7
        .DB     $FF,$04,$40,$00,$01,$11,$01,$00
        .DB     $FF,10,01,0
        .DB   	"DSP1  HROM 4M SAVE 2K       "
        .DB     0

        .ORG    $4840
        .DB     $FF,$04,$40,$00,$00,$30,$10,$00
        .DB     $FF,10,01,1
        .DB  	"DSP1  LROM 4M               "
        .DB     0

        .ORG    $4880
        .DB     $FF,$04,$80,$00,$03,$74,$10,$00
        .DB     $FF,10,01,2
        .DB   	"DSP FX 8M                   "
        .DB     0

        .ORG    $48C0
        .DB     $FF,$00,$00,$F0,$02,$00,$00,$00
        .DB     $FF,10,01,3
        .DB   	"ROM 24M                     "
        .DB     0

        .ORG    $4900
        .DB     $FF,$01,$B0,$00,$01,$01,$01,$32
        .DB     $FF,10,01,4
        .DB   	"HROM  32M SAVE 2K            "
        .DB     0

        .ORG    $4940
        .DB     $FF,$04,$D0,$00,$01,$01,$21,$00
        .DB     $FF,10,01,5
        .DB   	"HROM 48M S8K (256M-B0)       "
        .DB     0

        .ORG    $4980
        .DB     $FF,$04,$D1,$00,$02,$01,$21,$00
        .DB     $FF,10,01,6
        .DB   	"B HROM 48M S8K (256M-B1)      "
        .DB     0

        .ORG    $49C0
        .DB     $FF,$04,$D2,$00,$02,$01,$21,$00
        .DB     $FF,10,01,7
        .DB   	"E HROM 48M S8K (256M-B2)      "
        .DB     0

        .ORG    $4A00
        .DB     $FF,$04,$D3,$00,$02,$01,$21,$00
        .DB     $FF,10,01,8
        .DB   	"A HROM 48M S8K (256M-B3)      "
        .DB     0

        .ORG    $4A40
        .DB     $FF,$04,$D0,$40,$02,$01,$21,$00
        .DB     $FF,10,01,9
        .DB   	"G HROM 48M S8K (64M-B1)......."
        .DB     0

        .ORG    $4A80
        .DB     $FF,$04,$D0,$40,$02,$01,$21,$00
        .DB     $FF,10,01,10
        .DB   	"HROM 48M S8K (64M-B1)........"
        .DB     0
      
      	; Fill everything up to 4FFF, to avoid the compiler from putting anything else here
      	.ORG	$4AC0
 	.DSB 	$34f1,0
 

	
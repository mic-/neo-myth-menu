; NEO SNES Myth I/O registers

 .EQU MYTH_OPTION_IO $C000
 .EQU MYTH_GBAC_LIO  $C001
 .EQU MYTH_GBAC_HIO  $C002
 .EQU MYTH_GBAC_ZIO  $C003
 .EQU MYTH_GBAS_BIO  $C004
 .EQU MYTH_GBAS_ZIO  $C005
 .EQU MYTH_PRAM_BIO  $C006
 .EQU MYTH_PRAM_ZIO  $C007
 .EQU MYTH_RUN_IO    $C008
 .EQU MYTH_EXTM_ON   $C00D
 .EQU MYTH_RST_SEL   $C00E
 .EQU MYTH_RST_IO    $C010
 .EQU MYTH_SRAM_WE   $C011
 .EQU MYTH_WE_IO     $C014
 .EQU MYTH_CPLD_RIO  $C016
 .EQU MYTH_SRAM_TYPE $C018
 .EQU MYTH_SRAM_MAP  $C019
 .EQU MYTH_DSP_TYPE  $C01A
 .EQU MYTH_DSP_MAP   $C01B
 .EQU MYTH_SDBUF_IO  $C043
 
 
 ; For OPTION_IO
 .EQU MAP_MENU_FLASH_TO_ROM	$00
 .EQU MAP_16M_PSRAM_TO_ROM	$01
 .EQU MAP_24M_PSRAM_TO_ROM	$02
 .EQU MAP_32M_PSRAM_TO_ROM	$03
 .EQU GBAC_TO_PSRAM_COPY_MODE $04
 
 
; For WE_IO
 .EQU GBA_FLASH_WE		$01
 .EQU PSRAM_WE			$02
 
 
 ; For SDBUF_IO
 .EQU SDBUF_OFF			$00
 .EQU SDBUF_LO_HI		$01
 .EQU SDBUF_HI_LO		$03
 
 
 .EQU MYTH_NEO2_WR_CMD1 $41DE00  ;CF
 .EQU MYTH_NEO2_WR_DAT1 $41FC00
 .EQU MYTH_NEO2_WR_DAT4 $41E000
 
 .EQU MYTH_NEO2_RD_CMD1 $41FE81
 .EQU MYTH_NEO2_RD_DAT1 $417C21
 .EQU MYTH_NEO2_RD_DAT4 $406061	; CE
 
 
      
 
 
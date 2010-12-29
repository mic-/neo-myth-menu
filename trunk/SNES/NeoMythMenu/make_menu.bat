REM Convert graphics data
tools\sixpack -image -target snes -format p4 -planes 4 -o assets\marker.chr assets\marker.bmp
tools\sixpack -image -target snes -format p1 -bg 6,0 -o assets\font.chr assets\adore.bmp
tools\sixpack -image -target snes -format p4 -opt -codec aplib -o assets\menu_bg.lzs -pack assets\menu_bg2.bmp


REM C -> ASM / S
..\bin\816-tcc.exe -Wall -I../include -o main.ps -c main.c
..\bin\816-tcc.exe -Wall -I../include -o navigation.ps -c navigation.c
..\bin\816-tcc.exe -Wall -I../include -o game_genie.ps -c game_genie.c
..\bin\816-tcc.exe -Wall -I../include -o action_replay.ps -c action_replay.c
..\bin\816-tcc.exe -Wall -I../include -o ppuc.ps -c ppuc.c
..\bin\816-tcc.exe -Wall -I../include -o bg_buffer.ps -c bg_buffer.c
..\bin\816-tcc.exe -Wall -I../include -o common.ps -c common.c
..\bin\816-tcc.exe -Wall -I../include -o sd_utils.ps -c sd_utils.c

..\bin\816-tcc.exe -Wall -I../include -o cheat_db.ps -c cheats\cheat_database.c

..\bin\816-tcc.exe -Wall -I../include -o pff.ps -c pff.c
..\bin\816-tcc.exe -Wall -I../include -o u_strings.ps -c u_strings.c
..\bin\816-tcc.exe -Wall -I../include -o diskio.ps -c diskio.c
..\bin\816-tcc.exe -Wall -I../include -o myth_io.ps -c myth_io.c


REM Optimize ASM files
@echo off
tools\stripcom main.ps main.ps2
tools\stripcom navigation.ps navigation.ps2
tools\stripcom game_genie.ps game_genie.ps2
tools\stripcom action_replay.ps action_replay.ps2
tools\stripcom ppuc.ps ppuc.ps2
tools\stripcom bg_buffer.ps bg_buffer.ps2
tools\stripcom common.ps common.ps2
tools\stripcom sd_utils.ps sd_utils.ps2
tools\stripcom diskio.ps diskio.ps2
tools\stripcom myth_io.ps myth_io.ps2
tools\stripcom pff.ps pff.ps2
tools\stripcom u_strings.ps u_strings.ps2
tools\stripcom cheat_db.ps cheat_db.s
del *.ps 

..\bin\816-opt.py main.ps2 > main.s
..\bin\816-opt.py navigation.ps2 > navigation.s
..\bin\816-opt.py game_genie.ps2 > game_genie.s
..\bin\816-opt.py action_replay.ps2 > action_replay.s
..\bin\816-opt.py ppuc.ps2 > ppuc.s
..\bin\816-opt.py bg_buffer.ps2 > bg_buffer.s
..\bin\816-opt.py sd_utils.ps2 > sd_utils.s
..\bin\816-opt.py diskio.ps2 > diskio.s
..\bin\816-opt.py myth_io.ps2 > myth_io.s
..\bin\816-opt.py pff.ps2 > pff.s
..\bin\816-opt.py u_strings.ps2 > u_strings.s

tools\optimore-816 main.s mainopt.s
tools\optimore-816 navigation.s navigopt.s
tools\optimore-816 game_genie.s ggopt.s
tools\optimore-816 action_replay.s aropt.s
tools\optimore-816 bg_buffer.s bg_bufferopt.s
tools\optimore-816 sd_utils.s sd_utilsopt.s
tools\optimore-816 ppuc.s ppucopt.s
tools\optimore-816 pff.s pffopt.s
tools\optimore-816 diskio.s diskioopt.s
tools\optimore-816 myth_io.s myth_ioopt.s
tools\optimore-816 u_strings.s u_stringsopt.s

tools\constify main.c mainopt.s mainopt2.s
tools\constify common.c common.ps2 common.s
tools\constify navigation.c navigopt.s navigopt2.s
tools\constify cheats\cheat_database.c cheat_db.s cheat_db2.s
@echo on

wla-spc700 -vo vgmplay_spc700_ext.asm vgmplay.o
wlalink -vb vgmplay.link assets\vgmplay.bin

REM ASM -> OBJ
..\bin\wla-65816.exe -io assets\data.asm data.obj
..\bin\wla-65816.exe -io dma.asm dma.obj
..\bin\wla-65816.exe -io hw_math.asm hw_math.obj
REM ..\bin\wla-65816.exe -io lzss_decode.asm lzss_decode.obj
..\bin\wla-65816.exe -io neo2.asm neo2.obj
..\bin\wla-65816.exe -io neo2_spc.asm neo2_spc.obj
..\bin\wla-65816.exe -io neo2_vgm.asm neo2_vgm.obj
..\bin\wla-65816.exe -io ppu.asm ppu.obj
..\bin\wla-65816.exe -io pff_asm.asm pff_asm.obj
..\bin\wla-65816.exe -io dummy_games_list.asm dummy_games_list.obj
..\bin\wla-65816.exe -io cheat_db2.s cheat_db.obj

..\bin\wla-65816.exe -io mainopt2.s main.obj
..\bin\wla-65816.exe -io navigopt2.s navigation.obj
..\bin\wla-65816.exe -io ggopt.s game_genie.obj
..\bin\wla-65816.exe -io aropt.s action_replay.obj
..\bin\wla-65816.exe -io ppucopt.s ppuc.obj
..\bin\wla-65816.exe -io common.s common.obj
..\bin\wla-65816.exe -io bg_bufferopt.s bg_buffer.obj
..\bin\wla-65816.exe -io sd_utilsopt.s sd_utils.obj

..\bin\wla-65816.exe -io diskioopt.s diskio.obj
..\bin\wla-65816.exe -io myth_ioopt.s myth_io.obj
..\bin\wla-65816.exe -io pffopt.s pff.obj
..\bin\wla-65816.exe -io u_stringsopt.s u_strings.obj

..\bin\wla-65816.exe -io aplib_decrunch.asm aplib_decrunch.obj

REM OBJ -> SMC
..\bin\wlalink.exe -dvso main.obj navigation.obj ppuc.obj data.obj dma.obj game_genie.obj action_replay.obj hw_math.obj aplib_decrunch.obj neo2.obj neo2_spc.obj neo2_vgm.obj ppu.obj cheat_db.obj dummy_games_list.obj diskio.obj myth_io.obj pff_asm.obj pff.obj bg_buffer.obj sd_utils.obj common.obj u_strings.obj NEOSNES.BIN

@echo off
REM Delete files
del *.ps2
REM del *.s
del *.obj
REM del *.sym
@echo on

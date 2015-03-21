REM Convert graphics data
tools\sixpack -image -target snes -format p4 -planes 4 -o assets\marker.chr assets\marker.bmp
tools\sixpack -image -target snes -format p1 -bg 6,0 -o assets\font.chr assets\adore.bmp
tools\sixpack -image -target snes -format p4 -opt -codec aplib -o assets\menu_bg.lzs -pack assets\menu_bg2.bmp
tools\sixpack -image -target snes -format p4 -opt -o assets\menu_bg.pat assets\menu_bg2.bmp

SET SNES_SDK=C:\snes\SNES_SDK_r67_Win32

REM C -> ASM / S
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o main.ps -c main.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o navigation.ps -c navigation.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o game_genie.ps -c game_genie.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o action_replay.ps -c action_replay.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o ppuc.ps -c ppuc.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o bg_buffer.ps -c bg_buffer.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o common.ps -c common.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o sd_utils.ps -c sd_utils.c

%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o cheat_db.ps -c cheats\cheat_database.c

%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o pff.ps -c pff.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o u_strings.ps -c u_strings.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o diskio.ps -c diskio.c
%SNES_SDK%\bin\816-tcc.exe -Wall -I%SNES_SDK%/include -o myth_io.ps -c myth_io.c


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

%SNES_SDK%\bin\816-opt.py main.ps2 > main.s
%SNES_SDK%\bin\816-opt.py navigation.ps2 > navigation.s
%SNES_SDK%\bin\816-opt.py game_genie.ps2 > game_genie.s
%SNES_SDK%\bin\816-opt.py action_replay.ps2 > action_replay.s
%SNES_SDK%\bin\816-opt.py ppuc.ps2 > ppuc.s
%SNES_SDK%\bin\816-opt.py bg_buffer.ps2 > bg_buffer.s
%SNES_SDK%\bin\816-opt.py sd_utils.ps2 > sd_utils.s
%SNES_SDK%\bin\816-opt.py diskio.ps2 > diskio.s
%SNES_SDK%\bin\816-opt.py myth_io.ps2 > myth_io.s
%SNES_SDK%\bin\816-opt.py pff.ps2 > pff.s
%SNES_SDK%\bin\816-opt.py u_strings.ps2 > u_strings.s

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

%SNES_SDK%\bin\wla-spc700 -vo vgmplay_spc700_ext.asm vgmplay.o
assets\wlalink -vb vgmplay.link assets\vgmplay.bin

REM ASM -> OBJ
%SNES_SDK%\bin\wla-65816.exe -io assets\data.asm data.obj
%SNES_SDK%\bin\wla-65816.exe -io dma.asm dma.obj
%SNES_SDK%\bin\wla-65816.exe -io hw_math.asm hw_math.obj
REM ..\bin\wla-65816.exe -io lzss_decode.asm lzss_decode.obj
%SNES_SDK%\bin\wla-65816.exe -io neo2.asm neo2.obj
%SNES_SDK%\bin\wla-65816.exe -io neo2_spc.asm neo2_spc.obj
%SNES_SDK%\bin\wla-65816.exe -io neo2_vgm.asm neo2_vgm.obj
%SNES_SDK%\bin\wla-65816.exe -io ppu.asm ppu.obj
%SNES_SDK%\bin\wla-65816.exe -io inflate.asm inflate.obj
%SNES_SDK%\bin\wla-65816.exe -io inflate_game.asm inflate_game.obj
%SNES_SDK%\bin\wla-65816.exe -io pff_asm.asm pff_asm.obj
%SNES_SDK%\bin\wla-65816.exe -io dummy_games_list.asm dummy_games_list.obj
%SNES_SDK%\bin\wla-65816.exe -io cheat_db2.s cheat_db.obj

%SNES_SDK%\bin\wla-65816.exe -io mainopt2.s main.obj
%SNES_SDK%\bin\wla-65816.exe -io navigopt2.s navigation.obj
%SNES_SDK%\bin\wla-65816.exe -io ggopt.s game_genie.obj
%SNES_SDK%\bin\wla-65816.exe -io aropt.s action_replay.obj
%SNES_SDK%\bin\wla-65816.exe -io ppucopt.s ppuc.obj
%SNES_SDK%\bin\wla-65816.exe -io common.s common.obj
%SNES_SDK%\bin\wla-65816.exe -io bg_bufferopt.s bg_buffer.obj
%SNES_SDK%\bin\wla-65816.exe -io sd_utilsopt.s sd_utils.obj

%SNES_SDK%\bin\wla-65816.exe -io diskioopt.s diskio.obj
%SNES_SDK%\bin\wla-65816.exe -io myth_ioopt.s myth_io.obj
%SNES_SDK%\bin\wla-65816.exe -io pffopt.s pff.obj
%SNES_SDK%\bin\wla-65816.exe -io u_stringsopt.s u_strings.obj

%SNES_SDK%\bin\wla-65816.exe -io aplib_decrunch.asm aplib_decrunch.obj

REM OBJ -> SMC
%SNES_SDK%\bin\wlalink.exe -dvso main.obj navigation.obj dummy_games_list.obj inflate.obj inflate_game.obj aplib_decrunch.obj neo2.obj ppuc.obj data.obj dma.obj game_genie.obj action_replay.obj hw_math.obj neo2_spc.obj neo2_vgm.obj ppu.obj cheat_db.obj diskio.obj myth_io.obj pff_asm.obj pff.obj bg_buffer.obj sd_utils.obj common.obj u_strings.obj NEOSNES.BIN

@echo off
REM Delete files
del *.ps2
REM del *.s
del *.obj
REM del *.sym
@echo on

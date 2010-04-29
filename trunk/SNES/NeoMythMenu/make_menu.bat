REM Convert graphics data
tools\sixpack -image -opt -target snes -format p4 -o assets\menu_bg.lzs -pack assets\menu_bg2.bmp

REM C -> ASM / S
..\bin\816-tcc.exe -Wall -I../include -o main.ps -c main.c
..\bin\816-tcc.exe -Wall -I../include -o navigation.ps -c navigation.c
..\bin\816-tcc.exe -Wall -I../include -o game_genie.ps -c game_genie.c
..\bin\816-tcc.exe -Wall -I../include -o font.s -c assets\font.c

REM Optimize ASM files
tools\stripcom main.ps main.ps2
tools\stripcom navigation.ps navigation.ps2
tools\stripcom game_genie.ps game_genie.ps2
del *.ps 
..\bin\816-opt.py main.ps2 > main.s
..\bin\816-opt.py navigation.ps2 > navigation.s
..\bin\816-opt.py game_genie.ps2 > game_genie.s
tools\optimore-816 main.s mainopt.s
tools\optimore-816 navigation.s navigopt.s

REM ASM -> OBJ
..\bin\wla-65816.exe -io assets\data.asm data.obj
..\bin\wla-65816.exe -io dma.asm dma.obj
..\bin\wla-65816.exe -io hw_math.asm hw_math.obj
..\bin\wla-65816.exe -io lzss_decode.asm lzss_decode.obj
..\bin\wla-65816.exe -io neo2.asm neo2.obj
..\bin\wla-65816.exe -io neo2_spc.asm neo2_spc.obj
..\bin\wla-65816.exe -io ppu.asm ppu.obj
..\bin\wla-65816.exe -io dummy_games_list.asm dummy_games_list.obj

..\bin\wla-65816.exe -io mainopt.s main.obj
..\bin\wla-65816.exe -io navigopt.s navigation.obj
..\bin\wla-65816.exe -io game_genie.s game_genie.obj
..\bin\wla-65816.exe -io font.s font.obj

REM OBJ -> SMC
..\bin\wlalink.exe -dvo main.obj navigation.obj data.obj dma.obj font.obj game_genie.obj hw_math.obj lzss_decode.obj neo2.obj neo2_spc.obj ppu.obj dummy_games_list.obj NEOSNES.BIN

REM Delete files
del *.ps2
del main.s
del font.s
del navigation.s
del navigopt.s
REM del mainopt.s
REM del *.s
del *.obj
REM del *.sym


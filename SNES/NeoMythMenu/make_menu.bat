REM Convert graphics data
tools\sixpack -image -opt -target snes -format p4 -o assets\menu_bg.lzs -pack assets\menu_bg2.bmp

REM C -> ASM / S
..\bin\816-tcc.exe -Wall -I../include -o main.ps -c main.c
..\bin\816-tcc.exe -Wall -I../include -o navigation.ps -c navigation.c
..\bin\816-tcc.exe -Wall -I../include -o font.s -c assets\font.c

REM Optimize ASM files
..\bin\816-opt.py main.ps > main.s
..\bin\816-opt.py navigation.ps > navigation.s
tools\optimore-816 main.s mainopt.s
tools\optimore-816 navigation.s navigopt.s

REM ASM -> OBJ
..\bin\wla-65816.exe -io assets\data.asm data.obj
..\bin\wla-65816.exe -io dma.asm dma.obj
..\bin\wla-65816.exe -io hw_math.asm hw_math.obj
..\bin\wla-65816.exe -io lzss_decode.asm lzss_decode.obj
..\bin\wla-65816.exe -io neo2.asm neo2.obj
..\bin\wla-65816.exe -io dummy_games_list.asm dummy_games_list.obj

..\bin\wla-65816.exe -io mainopt.s main.obj
..\bin\wla-65816.exe -io navigopt.s navigation.obj
..\bin\wla-65816.exe -io font.s font.obj

REM OBJ -> SMC
..\bin\wlalink.exe -dvso main.obj navigation.obj data.obj dma.obj font.obj hw_math.obj lzss_decode.obj neo2.obj dummy_games_list.obj NEOSNES.BIN

REM Delete files
del *.ps
del main.s
del font.s
del navigation.s
del navigopt.s
REM del mainopt.s
REM del *.s
del *.obj
REM del *.sym


REM Convert graphics data
tools\sixpack -image -target snes -format p4 -planes 4 -o assets\marker.chr assets\marker.bmp
tools\sixpack -image -target snes -format p1 -bg 6,0 -o assets\font.chr assets\adore.bmp
tools\sixpack -image -target snes -format p4 -opt -o assets\menu_bg.lzs -pack assets\menu_bg2.bmp


REM C -> ASM / S
..\bin\816-tcc.exe -Wall -I../include -DEMULATOR -o main.ps -c main.c
..\bin\816-tcc.exe -Wall -I../include -DEMULATOR -o navigation.ps -c navigation.c
..\bin\816-tcc.exe -Wall -I../include -DEMULATOR -o game_genie.ps -c game_genie.c
..\bin\816-tcc.exe -Wall -I../include -DEMULATOR -o action_replay.ps -c action_replay.c
..\bin\816-tcc.exe -Wall -I../include -DEMULATOR -o ppuc.ps -c ppuc.c
..\bin\816-tcc.exe -Wall -I../include -DEMULATOR -o cheat_db.ps -c cheats\cheat_database.c


REM Optimize ASM files
tools\stripcom main.ps main.ps2
tools\stripcom navigation.ps navigation.ps2
tools\stripcom game_genie.ps game_genie.ps2
tools\stripcom action_replay.ps action_replay.ps2
tools\stripcom ppuc.ps ppuc.ps2
tools\stripcom cheat_db.ps cheat_db.s
del *.ps 
..\bin\816-opt.py main.ps2 > main.s
..\bin\816-opt.py navigation.ps2 > navigation.s
..\bin\816-opt.py game_genie.ps2 > game_genie.s
..\bin\816-opt.py action_replay.ps2 > action_replay.s
..\bin\816-opt.py ppuc.ps2 > ppuc.s

tools\optimore-816 main.s mainopt.s
tools\optimore-816 navigation.s navigopt.s
tools\optimore-816 game_genie.s ggopt.s
tools\optimore-816 action_replay.s aropt.s
tools\optimore-816 ppuc.s ppucopt.s

tools\constify main.c mainopt.s mainopt2.s
tools\constify navigation.c navigopt.s navigopt2.s
tools\constify cheats\cheat_database.c cheat_db.s cheat_db2.s

REM ASM -> OBJ
..\bin\wla-65816.exe -io -DEMULATOR assets\data.asm data.obj
..\bin\wla-65816.exe -io -DEMULATOR dma.asm dma.obj
..\bin\wla-65816.exe -io -DEMULATOR hw_math.asm hw_math.obj
..\bin\wla-65816.exe -io -DEMULATOR lzss_decode.asm lzss_decode.obj
..\bin\wla-65816.exe -io -DEMULATOR neo2.asm neo2.obj
..\bin\wla-65816.exe -io -DEMULATOR neo2_spc.asm neo2_spc.obj
..\bin\wla-65816.exe -io -DEMULATOR ppu.asm ppu.obj
..\bin\wla-65816.exe -io -DEMULATOR dummy_games_list.asm dummy_games_list.obj

..\bin\wla-65816.exe -io -DEMULATOR cheat_db2.s cheat_db.obj

..\bin\wla-65816.exe -io -DEMULATOR mainopt2.s main.obj
..\bin\wla-65816.exe -io -DEMULATOR navigopt2.s navigation.obj
..\bin\wla-65816.exe -io -DEMULATOR ggopt.s game_genie.obj
..\bin\wla-65816.exe -io -DEMULATOR aropt.s action_replay.obj
..\bin\wla-65816.exe -io -DEMULATOR ppucopt.s ppuc.obj

REM OBJ -> SMC
..\bin\wlalink.exe -dvso main.obj navigation.obj ppuc.obj data.obj dma.obj game_genie.obj action_replay.obj hw_math.obj lzss_decode.obj neo2.obj neo2_spc.obj ppu.obj cheat_db.obj dummy_games_list.obj NEOSNES.TMP

REM Delete files
del *.ps2
del *.s
del *.obj
REM del *.sym


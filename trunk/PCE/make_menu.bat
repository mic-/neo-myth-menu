tools\sixpack -image -opt -target tgx -bg 128,128 -t -v -planes 4 -o assets\menu_bg3.chr assets\menu_bg3.bmp
tools\sixpack -image -opt -target tgx -bg 128,128 -t -v -planes 4 -o assets\menu_bg4.chr assets\menu_bg4.bmp
wla-huc6280 -o neopcemenu.asm menu.o
wlalink -vbS menu.link neopcemenu.pce

copy /Y neopcemenu.pce PCEBIOS.BIN

@echo off
del menu.o
del assets\*.chr
del assets\*.nam
del assets\*.pal
@echo on

c:\pcedev\mednafen-0.8.D-win32\mednafen.exe neopcemenu.pce


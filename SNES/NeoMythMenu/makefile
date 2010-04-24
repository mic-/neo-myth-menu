CC = $(SNESSDK)/bin/816-tcc
AS = $(SNESSDK)/bin/wla-65816
LD = $(SNESSDK)/bin/wlalink
RM = rm -rf
OPTIMIZE = python $(SNESSDK)/bin/816-opt.py
OPTIMORE = ./tools/optimore-816
SIXPACK = wine ./tools/sixpack.exe

OBJS = main.obj navigation.obj assets/data.obj dma.obj font.obj hw_math.obj lzss_decode.obj neo2.obj dummy_games_list.obj

CFLAGS = -Wall
IFLAGS = -I./ -I$(SNESSDK)/include

all: assets/menu_bg.lzs font.s $(OBJS) NEOSNES.BIN

# OBJ -> SMC
NEOSNES.BIN:
	$(LD) -dvso $(OBJS) NEOSNES.BIN

# Convert graphics data
assets/menu_bg.lzs: assets/menu_bg2.bmp
	$(SIXPACK) -image -opt -target snes -format p4 -o $@ -pack $<

# C -> ASM - don't optimize asm afterwards
font.s: assets/font.c
	$(CC) $(CFLAGS) $(IFLAGS) -o $@ -c $<

# C -> ASM - optimize asm afterwards
%.a0: %.c
	$(CC) $(CFLAGS) $(IFLAGS) -o $@ -c $<

%.a1: %.a0
	$(OPTIMIZE) $< >$@

%.a2: %.a1
	$(OPTIMORE) $< >$@

%.obj: %.asm
	$(AS) -io $< $@

%.obj: %.s
	$(AS) -io $< $@

#%.obj: %.a2
%.obj: %.a1
	$(AS) -io $< $@

clean:
	$(RM) NEOSNES.BIN *.obj *.s *.a2 *.a1 *.a0 *.sym assets/*.obj assets/*.lzs

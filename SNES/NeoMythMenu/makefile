CC = $(SNESSDK)/bin/816-tcc
AS = $(SNESSDK)/bin/wla-65816
LD = $(SNESSDK)/bin/wlalink
RM = rm -rf
OPTIMIZE = python $(SNESSDK)/bin/816-opt.py
OPTIMORE = ./tools/optimore-816
STRIPCOM = ./tools/stripcom
CONSTIFY = ./tools/constify
SIXPACK = tools/sixpack

OBJS = main.obj navigation.obj ppuc.obj assets/data.obj dma.obj game_genie.obj action_replay.obj hw_math.obj lzss_decode.obj neo2.obj neo2_spc.obj ppu.obj cheats/cheat_database.obj dummy_games_list.obj

EMUOBJS = main.eobj navigation.eobj ppuc.eobj assets/data.obj dma.obj game_genie.obj action_replay.obj hw_math.obj lzss_decode.obj neo2.eobj neo2_spc.eobj ppu.eobj cheats/cheat_database.obj dummy_games_list.obj

CFLAGS = -Wall
CFLAGSEMU = -Wall -DEMULATOR
IFLAGS = -I./ -I$(SNESSDK)/include

all: assets/menu_bg.lzs assets/font.chr assets/marker.chr $(OBJS) NEOSNES.BIN
all_emu: assets/menu_bg.lzs assets/font.chr assets/marker.chr $(EMUOBJS) NEOSNESEMU.BIN

# OBJ -> SMC
NEOSNES.BIN:
	$(LD) -dvso $(OBJS) NEOSNES.BIN

# OBJ -> SMC
NEOSNESEMU.BIN:
	$(LD) -dvso $(EMUOBJS) NEOSNESEMU.BIN

# Convert graphics data
assets/menu_bg.lzs: assets/menu_bg2.bmp
	$(SIXPACK) -image -target snes -opt -format p4 -o $@ -pack $<
assets/font.chr: assets/adore.bmp
	$(SIXPACK) -image -target snes -format p1 -bg 6,0 -o $@ $<
assets/marker.chr: assets/marker.bmp
	$(SIXPACK) -image -target snes -format p4 -planes 4 -o $@ $<

# C -> ASM - optimize asm afterwards
%.a0: %.c
	$(CC) $(CFLAGS) $(IFLAGS) -o $@ -c $<

%.a1: %.a0
	$(STRIPCOM) $< $@

%.a2: %.a1
	$(OPTIMIZE) $< >$@

%.a3: %.a2
	$(OPTIMORE) $< $@

%.a4: %.a3
	$(CONSTIFY) $*.c $< $@

%.obj: %.asm
	$(AS) -io $< $@

%.obj: %.s
	$(AS) -io $< $@

%.obj: %.a4
	$(AS) -io $< $@


%.a0e: %.c
	$(CC) $(CFLAGSEMU) $(IFLAGS) -o $@ -c $<

%.a1e: %.a0e
	$(STRIPCOM) $< $@

%.a2e: %.a1e
	$(OPTIMIZE) $< >$@

%.a3e: %.a2e
	$(OPTIMORE) $< $@

%.a4e: %.a3e
	$(CONSTIFY) $*.c $< $@

%.eobj: %.asm
	$(AS) -io -DEMULATOR $< $@

%.eobj: %.a4e
	$(AS) -io $< $@

clean:
	$(RM) NEOSNES.BIN NEOSNESEMU.BIN *.obj *.eobj *.s *.sym assets/*.obj assets/*.lzs assets/*.chr cheats/*.obj

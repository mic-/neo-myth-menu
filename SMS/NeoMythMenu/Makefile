ifndef $(SDCC_HOME)
SDCC_HOME = /opt/toolchains/sdcc
endif
SDCC_INCLUDE = $(SDCC_HOME)/share/sdcc/include
SDCC_LIB = $(SDCC_HOME)/share/sdcc/lib

BINPATH = $(SDCC_HOME)/bin
CC = $(BINPATH)/sdcc
AS = $(BINPATH)/sdasz80
RM = rm -rf
PADTRIM = tools/bin16k
HEX2BIN = tools/hex2bin
MAP2H = tools/map2h
SMSHDR = tools/smshdr

CFLAGS = -mz80 --data-loc 0xC001
#CFLAGS = -mz80 --data-loc 0xC001 -DEMULATOR

BANK0OBJS = vdp_asm.rel vdp.rel util.rel pad.rel font3.rel crt0.rel
SHARED = shared.rel

all: tools/map2h tools/bin16k tools/hex2bin tools/smshdr diskio_ram.rel ramcode.bin vgmcode.bin $(SHARED) pffromcode.bin $(BANK0OBJS) bank0.bin bank1.bin menu.sms NEOSEGA4.BIN


tools/map2h:
	g++ -o tools/map2h -lstdc++ tools/map2h.cpp

tools/bin16k:
	gcc tools/bin16k.c -o tools/bin16k

tools/hex2bin:
	gcc tools/hex2bin.c -o tools/hex2bin

tools/smshdr:
	gcc tools/smshdr.c -o tools/smshdr

ramcode.bin:
	$(AS) -o diskio_asm.s
	$(CC) $(CFLAGS) --no-std-crt0 --no-peep --code-loc 0xC800 neo2.c diskio_ram.rel diskio_asm.rel
	$(HEX2BIN) neo2.ihx
	$(PADTRIM) neo2.bin ramcode.bin
	$(MAP2H) neo2.map neo2.h diskio.h
	$(MAP2H) neo2.map --output-asm neo2.h
	$(RM) neo2.bin neo2.ihx neo2.asm *.noi *.lnk *.rst

vgmcode.bin:
	$(AS) -o vgm_player.s
	$(CC) $(CFLAGS) --no-std-crt0 --no-peep --code-loc 0xDD00 vgm_player.rel
	$(HEX2BIN) vgm_player.ihx
	$(PADTRIM) vgm_player.bin vgmcode.bin
	$(MAP2H) vgm_player.map vgm_player.h
	$(RM) vgm_player.bin vgm_player.ihx *.noi *.lnk *.rst

pffromcode.bin:
	$(CC) $(CFLAGS) --no-std-crt0 --oldralloc --code-loc 0x8000 pff.c $(SHARED)
	$(HEX2BIN) pff.ihx
	$(PADTRIM) pff.bin pffromcode.bin
	$(MAP2H) pff.map pff.h
	$(RM) pff.bin pff.ihx pff.asm *.noi *.lnk *.rst

bank0.bin:
	$(CC) $(CFLAGS) -c sd_utils.c
	$(CC) $(CFLAGS) -Wl-b_GSINIT=0x3400 --no-std-crt0 --oldralloc main.c $(BANK0OBJS) sd_utils.rel $(SHARED)
	$(HEX2BIN) main.ihx
	$(PADTRIM) main.bin bank0.bin
	$(MAP2H) main.map vdp.h pad.h util.h
	$(RM) main.asm

bank1.bin:
	$(CC) $(CFLAGS) --no-std-crt0 --no-peep --code-loc 0x4000 bank1.c $(SHARED)
	$(HEX2BIN) bank1.ihx
	$(PADTRIM) bank1.bin bank_1.bin
	$(MAP2H) bank1.map bank1.h
	$(RM) bank1.bin bank1.ihx bank1.asm *.noi *.lnk *.rst

menu.sms:
	cat bank0.bin bank_1.bin ramcode.bin bank0.bin bank0.bin pffromcode.bin vgmcode.bin bank0.bin > menu.sms
	$(SMSHDR) menu.sms
	$(RM) main.ihx *.noi *.lnk *.rst *.rel *.sym

NEOSEGA4.BIN:
	cp menu.sms NEOSEGA4.BIN

%.rel: %.c
	$(CC) $(CFLAGS) -c $<
	rm -f $*.asm
%.rel: %.s
	$(AS) -o $<


clean:
	$(RM) NEOSEGA4.BIN main_map.h neo2_map.h neo2_map.inc pff_map.h bank1_map.h vgm_player_map.h menu.sms *.bin *.asm *.ihx *.rel *.sym *.rst *.noi *.lnk *.lk *.lst *.map

distclean:
	$(RM) tools/map2h tools/bin16k tools/hex2bin tools/smshdr NEOSEGA4.BIN main_map.h neo2_map.h menu.sms *.bin *.asm *.ihx *.rel *.sym *.rst *.noi *.lnk *.lst *.map

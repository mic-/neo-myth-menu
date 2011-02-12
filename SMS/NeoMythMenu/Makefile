CC = $(SMSDEV)sdcc
AS = $(SMSDEV)sdasz80
RM = rm -rf
PADTRIM = tools/bin16k
HEX2BIN = tools/hex2bin
MAP2H = tools/map2h
SMSHDR = tools/smshdr

CFLAGS = -mz80 --data-loc 0xC001 -DEMULATOR
#CFLAGS = -mz80 --data-loc 0xC001 -DEMULATOR

BANK0OBJS = vdp_asm.rel vdp.rel util.rel pad.rel font.rel crt0.rel
SHARED = shared.rel

all: tools/map2h tools/bin16k tools/hex2bin tools/smshdr $(BANK0OBJS) $(SHARED) ramcode.bin sramcode.bin bank0.bin bank1.bin menu.sms MDBIOS.BIN


tools/map2h:
	g++ -o tools/map2h -lstdc++ tools/map2h.cpp

tools/bin16k:
	gcc tools/bin16k.c -o tools/bin16k

tools/hex2bin:
	gcc tools/hex2bin.c -o tools/hex2bin

tools/smshdr:
	gcc tools/smshdr.c -o tools/smshdr


ramcode.bin:
	$(CC) $(CFLAGS) --no-std-crt0 --code-loc 0xD000 neo2.c
	$(HEX2BIN) neo2.ihx
	$(PADTRIM) neo2.bin ramcode.bin
	$(MAP2H) neo2.map neo2.h
	$(RM) neo2.bin neo2.ihx *.noi *.lnk *.rst

sramcode.bin:
	$(AS) -o diskio_asm.s
	$(CC) $(CFLAGS) --no-std-crt0 --code-loc 0x8000 diskio_asm.rel $(SHARED)
	$(HEX2BIN) diskio_asm.ihx
	$(PADTRIM) diskio_asm.bin sramcode.bin
	$(RM) diskio_asm.bin diskio_asm.ihx *.noi *.lnk *.rst

bank0.bin:
	$(CC) $(CFLAGS) -Wl-b_GSINIT=0x3000 --no-std-crt0 main.c $(BANK0OBJS) $(SHARED)
	$(HEX2BIN) main.ihx
	$(PADTRIM) main.bin bank0.bin
	$(MAP2H) main.map vdp.h pad.h

bank1.bin:
	$(CC) $(CFLAGS) --no-std-crt0 --code-loc 0x4000 bank1.c $(SHARED)
	$(HEX2BIN) bank1.ihx
	$(PADTRIM) bank1.bin bank_1.bin
	$(RM) bank1.bin bank1.ihx *.noi *.lnk *.rst

menu.sms:
	cat bank0.bin bank_1.bin ramcode.bin bank0.bin > menu.sms
	$(SMSHDR) menu.sms
	$(RM) bank0.bin bank_1.bin main.bin sramcode.bin main.ihx *.noi *.lnk *.rst *.sym *.rel

MDBIOS.BIN:
	cp menu.sms MDBIOS.BIN

%.rel: %.c
	$(CC) $(CFLAGS) -c $<
	rm -f $*.asm
%.rel: %.s
	$(AS) -o $<


clean:
	$(RM) MDBIOS.BIN main_map.h neo2_map.h menu.sms *.bin *.asm *.ihx *.rel *.sym *.rst *.noi *.lnk *.lst *.map

distclean:
	$(RM) tools/map2h tools/bin16k tools/hex2bin tools/smshdr MDBIOS.BIN main_map.h menu.sms *.bin *.asm *.ihx *.rel *.sym *.rst *.noi *.lnk *.lst *.map

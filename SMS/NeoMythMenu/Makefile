CC = sdcc
AS = sdasz80
RM = rm -rf
PADTRIM = tools/bin16k
HEX2BIN = tools/hex2bin
MAP2H = tools/map2h

CFLAGS = -mz80 --data-loc 0xC001 -DEMULATOR

BANK0OBJS = vdp_asm.rel vdp.rel util.rel pad.rel font.rel neo2.rel crt0.rel
SHARED = shared.rel

all: tools/map2h tools/bin16k tools/hex2bin $(BANK0OBJS) $(SHARED) bank0.bin bank1.bin menu.sms

tools/map2h:
	g++ -o tools/map2h -lstdc++ tools/map2h.cpp

tools/bin16k:
	gcc tools/bin16k.c -o tools/bin16k

tools/hex2bin:
	gcc tools/hex2bin.c -o tools/hex2bin

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
	cat bank0.bin bank_1.bin > menu.sms
	$(RM) bank0.bin bank_1.bin main.bin main.ihx *.noi *.lnk *.rst *.sym

%.rel: %.c
	$(CC) $(CFLAGS) -c $<
	rm -f $*.asm
%.rel: %.s
	$(AS) -o $<


clean:
	$(RM) tools/map2h tools/bin16k tools/hex2bin main_map.h menu.sms *.bin *.asm *.ihx *.rel *.sym *.rst *.noi *.lnk *.lst *.map

CC = sdcc
AS = sdasz80
RM = rm -rf
PADTRIM = tools/bin16k
HEX2BIN = tools/hex2bin

CFLAGS = -mz80 --data-loc 0xC001 -DEMULATOR

BANK0OBJS = vdp.rel pad.rel font2.rel crt0.rel
SHARED = shared.rel

all: $(BANK0OBJS) $(SHARED) bank1.bin menu.sms


bank1.bin:
	$(CC) $(CFLAGS) --no-std-crt0 --code-loc 0x4000 bank1.c $(SHARED)
	$(HEX2BIN) bank1.ihx
	$(PADTRIM) bank1.bin bank_1.bin
	$(RM) bank1.bin bank1.ihx *.noi *.lnk *.rst

menu.sms:
	$(CC) $(CFLAGS) -Wl-b_GSINIT=0x3000 --no-std-crt0 main.c $(BANK0OBJS) $(SHARED)
	$(HEX2BIN) main.ihx
	$(PADTRIM) main.bin bank0.bin
	cat bank0.bin bank_1.bin > menu.sms
	$(RM) bank0.bin bank_1.bin main.bin main.ihx *.noi *.lnk *.rst *.sym

%.rel: %.c
	$(CC) $(CFLAGS) -c $<
	rm -f $*.asm
%.rel: %.s
	$(AS) -o $<


clean:
	$(RM) menu.sms *.bin *.asm *.ihx *.rel *.sym *.rst *.noi *.lnk *.lst *.map

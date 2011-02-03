CC = sdcc
AS = sdasz80
RM = rm -rf
PADTRIM = tools/bin16k

CFLAGS = -mz80 --data-loc 0xC000

BANK0OBJS = vdp.rel crt0.rel
SHARED = shared.rel

all: $(BANK0OBJS) $(SHARED) bank1.bin menu.sms


bank1.bin:
	$(CC) $(CFLAGS) --no-std-crt0 --code-loc 0x4000 bank1.c $(SHARED)
	hex2bin bank1.ihx
	$(PADTRIM) bank1.bin bank_1.bin
	
menu.sms:
	$(CC) $(CFLAGS) -Wl-b_GSINIT=0x3000 --no-std-crt0 main.c $(BANK0OBJS) $(SHARED)
	hex2bin main.ihx
	$(PADTRIM) main.bin bank0.bin
	cat bank0.bin bank_1.bin > menu.sms
	
%.rel: %.c
	$(CC) $(CFLAGS) -c $<
%.rel: %.s
	$(AS) -o $< 
	
	
clean:
	$(RM) menu.sms bank1.bin bank_1.bin bank0.bin *.ihx *.rel *.sym *.rst
	
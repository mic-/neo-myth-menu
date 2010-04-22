CC = $(GENDEV)/m68k/bin/gcc
AS = $(GENDEV)/m68k/bin/as
RM = rm -f
OBJC = $(GENDEV)/m68k/bin/objcopy
ASMZ80 = $(GENDEV)/bin/zasm
BINTOC = $(GENDEV)/bin/bin2c

OBJS = main.o neo2.o font.o pff.o diskio.o
INC_OBJS = 

INCS = -I$(GENDEV)/m68k/include -I$(GENDEV)/m68k/sdk/include -I.
FLAGS = $(OPTION) -m68000 -Wall -Os -c -fomit-frame-pointer $(INCS)
FLAGSZ80 = -vb2
LIBS = -L$(GENDEV)/m68k/lib -L$(GENDEV)/m68k/sdk/lib -lc -lgcc

all: crt0.o MDBIOS.BIN

MDBIOS.BIN: rom.out
	$(OBJC) --pad-to 65536 -O binary rom.out MDBIOS.BIN

rom.out: $(OBJS) $(INC_OBJS)
	$(CC) -T $(GENDEV)/m68k/lib/md.ld -nostdlib crt0.o $(OBJS) $(LIBS) $(INC_OBJS) -o rom.out

%.o80: %.s80
	$(ASMZ80) $(FLAGSZ80) -o $@ $<

%.c: %.o80
	$(BINTOC) $<

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

%.o: %.s
	$(AS) -m68000 --register-prefix-optional $< -o $@

clean:
	$(RM) -rf *.o *.a *.o80 *.out *.bin *.log *.BIN

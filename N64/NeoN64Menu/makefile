.PHONY: NEON64.v64 NEON64MF.v64 NEON64SD.v64 clean

all: NEON64.v64 NEON64MF.v64 NEON64SD.v64 NEON64SD.z64

clean:
	make -f makefile.u2 clean
	make -f makefile.mf clean
	make -f makefile.sd clean
	make -f makefile.sd_nat clean

NEON64.v64:
	rm -f *.elf *.o *.bin
	make -f makefile.u2

NEON64MF.v64:
	rm -f *.elf *.o *.bin
	make -f makefile.mf

NEON64SD.v64:
	rm -f *.elf *.o *.bin
	make -f makefile.sd

NEON64SD.z64:
	rm -f *.elf *.o *.bin
	make -f makefile.sd_nat

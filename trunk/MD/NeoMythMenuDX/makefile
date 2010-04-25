.PHONY: MDBIOS MDEBIOS clean

all: MDBIOS MDEBIOS

clean:
	make -f makefile.rom clean
	make -f makefile.ram clean

MDBIOS:
	make -f makefile.rom

MDEBIOS:
	make -f makefile.ram

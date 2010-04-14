all: MDBIOS MDEBIOS

clean:
	make -f makefile.rom clean
	make -f makefile.ram clean

MDBIOS:
	rm -rf crt0.o main.o
	make -f makefile.rom

MDEBIOS:
	rm -rf crt0.o main.o
	make -f makefile.ram

ROOTDIR = $(N64_INST)
CC = $(ROOTDIR)/bin/mips64-elf-gcc
AR = $(ROOTDIR)/bin/mips64-elf-ar
RANLIB = $(ROOTDIR)/bin/mips64-elf-ranlib
RM = rm -rf

libff.a_OBJS = ff.o diskio.o option/ccsbcs.o

INCS = -I. -I$(ROOTDIR)/include
FLAGS = -march=vr4300 -mtune=vr4300 -O2 -G0 -Wall -g -D_GCC_LIMITS_H_ $(INCS)

all: $(libff.a_OBJS) libff.a

%.a: $(libff.a_OBJS)
	$(RM) $@
	$(AR) cru $@ $($@_OBJS)
	$(RANLIB) $@

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

clean:
	$(RM) *.o *.a option/*.o

install: all
	cp ff.h $(ROOTDIR)/include
	cp ffconf.h $(ROOTDIR)/include
	cp integer.h $(ROOTDIR)/include
	cp diskio.h $(ROOTDIR)/include
	cp libff.a $(ROOTDIR)/lib

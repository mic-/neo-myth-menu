NAME=bc_dump
RM=rm
CXX=gcc
LD=-Wall -lbfd -pedantic -O3
SRC=bc_dump.c mips/mips-dis.c

all:
	$(CXX) $(SRC) $(LD) -o $(NAME)

clean:
	$(RM) $(NAME)


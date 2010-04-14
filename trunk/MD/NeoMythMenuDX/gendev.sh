# gendev env vars
export GENDEV=/gendev
export PATH=$GENDEV/m68k/bin:$GENDEV/bin:$PATH
cp extra\ files/libff/libff.a .
make -f makefile

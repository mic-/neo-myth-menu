#include "Z80.h"

extern volatile int gTicks;
extern void ints_on();

void Z80_requestBus(unsigned short wait)
{
   volatile unsigned short *pw_bus;
   volatile unsigned short *pw_reset;

   // request bus (need to end reset)
   pw_bus = (unsigned short *)Z80_HALT_PORT;
   pw_reset = (unsigned short *)Z80_RESET_PORT;
   *pw_bus = 0x0100;
   *pw_reset = 0x0100;

   if (wait)
   {
       // wait for bus taken
       while (*pw_bus & 0x0100);
   }
}

void Z80_releaseBus(void)
{
   volatile unsigned short *pw;

   pw = (unsigned short *)Z80_HALT_PORT;
   *pw = 0x0000;
}


void Z80_startReset(void)
{
   volatile unsigned short *pw;

   pw = (unsigned short *)Z80_RESET_PORT;
   *pw = 0x0000;
}

void Z80_endReset(void)
{
   volatile unsigned short *pw;

   pw = (unsigned short *)Z80_RESET_PORT;
   *pw = 0x0100;
}


void Z80_setBank(const unsigned short bank)
{
   volatile unsigned char *pb;
   unsigned short i, value;

   pb = (unsigned char *)Z80_BANK_REGISTER;

   i = 9;
   value = bank;
   while (i--)
   {
       *pb = value;
       value >>= 1;
   }
}


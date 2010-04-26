#ifndef _Z80_CTRL_H_
#define _Z80_CTRL_H_

#define Z80_HALT_PORT       0xA11100
#define Z80_RESET_PORT      0xA11200

#define Z80_RAM             0xA00000
#define Z80_YM2612          0xA04000
#define Z80_BANK_REGISTER   0xA06000

void Z80_requestBus(unsigned short wait);
void Z80_releaseBus(void);
void Z80_startReset(void);
void Z80_endReset(void);
void Z80_setBank(const unsigned short bank);


#endif // _Z80_CTRL_H_

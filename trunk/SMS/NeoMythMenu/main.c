#include "sms.h"
#include "vdp.h"
#include "shared.h"


void mute_psg()
{
	PsgPort = 0x9F;		// Mute channel 0
	PsgPort = 0xBF;		// Mute channel 1
	PsgPort = 0xDF;		// Mute channel 2
	PsgPort = 0xFF;		// Mute channel 3
}


void main()
{
	Frame1 = 1;

	mute_psg();

	vdp_set_reg(0, 0x04);	// Set mode 4
	vdp_set_reg(1, 0xE0);
	vdp_set_cram_addr(0x0000);
	VdpData = 0x7;
	VdpData = 0;

	while (1) {}
}

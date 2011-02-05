#include "vdp.h"


void vdp_set_reg(BYTE rn, BYTE val)
{
    disable_ints;
    VdpCtrl = val;
    VdpCtrl = rn | CMD_VDP_REG_WRITE;
    enable_ints;
}


void vdp_set_vram_addr(WORD addr)
{
    disable_ints;
    VdpCtrl = (addr & 0xFF);
    VdpCtrl = (addr >> 8) | CMD_VRAM_WRITE;
    enable_ints;
}


void vdp_set_cram_addr(WORD addr)
{
    disable_ints;
    VdpCtrl = (addr & 0xFF);
    VdpCtrl = (addr >> 8) | CMD_CRAM_WRITE;
    enable_ints;
}


void vdp_copy_to_vram(WORD dest, BYTE *src, WORD len)
{
    vdp_set_vram_addr(dest);

    while (len--)
    {
        VdpData = *src++;
    }
}


void vdp_wait_vblank()
{
	while (!(VdpCtrl & 0x80)) {}
}



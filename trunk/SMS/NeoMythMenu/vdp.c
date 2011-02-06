#include "vdp.h"


void vdp_set_reg(BYTE rn, BYTE val)
{
    VdpCtrl = val;
    VdpCtrl = rn | CMD_VDP_REG_WRITE;
}


void vdp_set_vram_addr(WORD addr)
{
    VdpCtrl = (addr & 0xFF);
    VdpCtrl = (addr >> 8) | CMD_VRAM_WRITE;
}


void vdp_set_cram_addr(WORD addr)
{
    VdpCtrl = (addr & 0xFF);
    VdpCtrl = (addr >> 8) | CMD_CRAM_WRITE;
}


void vdp_copy_to_vram(WORD dest, BYTE *src, WORD len)
{
    vdp_set_vram_addr(dest);

    while (len--)
    {
        VdpData = *src++;
    }
}


void vdp_set_color(BYTE cnum, BYTE red, BYTE green, BYTE blue)
{
	vdp_set_cram_addr(cnum);
	VdpData = (blue << 4) | (green << 2) | red;
}


void vdp_wait_vblank()
{
    while (!(VdpCtrl & 0x80)) {}
}




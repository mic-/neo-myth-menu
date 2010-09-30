#include <libdragon.h>
#include "interrupts.h"
//extern void simulate_pif_boot(u32 cic_chip);

void ints_dump_regs(TExceptionBlock* e)
{
	//dummy
}

void onException(TExceptionBlock* e)
{
	switch(e->type)
	{
		case EXCEPTION_TYPE_RESET:
			//simulate_pif_boot(sys_get_boot_cic());
		return;

		case EXCEPTION_TYPE_CRITICAL:
		default:
			ints_dump_regs(e);
		return;
	}
}

void ints_setup()
{
	register_exception_handler(onException);
}

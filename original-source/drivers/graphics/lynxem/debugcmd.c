/* :ts=8 bk=0
 *
 * debugcmd.c:	Kernel debugger commands.
 *
 * Leo L. Schwab					2000.07.06
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>

#include "driver.h"


/****************************************************************************
 * Structure definitions.
 */
struct status {
	uint32	mask;
	char	name[32];
};


/****************************************************************************
 * Globals.
 */
extern driverdata	*gdd;

uint32			dbg_ncalls;


/****************************************************************************
 * Debuggery.
 */
static int
lynxemdump (int argc, char **argv)
{
	int i;
	
	kprintf ("LynxEM kernel driver\n%d device(s) present\n",
		 gdd->dd_NDevs);
	kprintf ("Driver-wide data: 0x%08lx\n", gdd);
	kprintf ("Driver-wide benahpore: %d/%d\n",
		 gdd->dd_DriverLock.b4_Sema4,
		 gdd->dd_DriverLock.b4_FastLock);
	kprintf ("Total calls to IRQ handler: %ld\n", dbg_ncalls);
	kprintf ("Total relevant interrupts: %ld\n", gdd->dd_NInterrupts);

	for (i = 0;  i < gdd->dd_NDevs;  i++) {
		devinfo		*di = &gdd->dd_DI[i];
		gfx_card_info	*ci = di->di_GfxCardInfo;
		gfx_card_ctl	*cc = di->di_GfxCardCtl;

		kprintf ("Device %s\n", di->di_Name);
		kprintf ("\tTotal interrupts: %ld\n", di->di_NInterrupts);
		kprintf ("\tVBlanks: %ld\n", di->di_NVBlanks);
		kprintf ("\tLast IRQ mask: 0x%08lx\n", di->di_LastIrqMask);
		if (ci) {
			kprintf ("      gfx_card_info: 0x%08lx\n", ci);
			kprintf ("       gfx_card_ctl: 0x%08lx\n", cc);
			kprintf ("    Base register 0: 0x%08lx\n",
				 ci->ci_BaseAddr0);
			kprintf ("         ci_VGARegs: 0x%08lx\n",
				 ci->ci_VGARegs);
			kprintf ("          ci_DPRegs: 0x%08lx\n",
				 ci->ci_DPRegs);
			kprintf ("        cc_IRQFlags: 0x%08lx\n",
				 cc->cc_IRQFlags);
		}
	}
	kprintf ("\n");
	return (TRUE);
}

static int
idxinb (int ac, char **av)
{
	pci_module_info	*pci;
	register int	i, port;
	register uint8	first, last;

	if (ac < 3) {
		kprintf ("usage: %s <ioreg> <firstindex> [<lastindex>]\n",
			 av[0]);
		return (TRUE);
	}

	port = strtol (av[1], NULL, 0);
	if (port <= 0) {
		kprintf ("Bad port number.\n");
		return (TRUE);
	}
	first = strtol (av[2], NULL, 0);
	if (ac == 4)
		last = strtol (av[3], NULL, 0);
	else
		last = first;

	if (get_module (B_PCI_MODULE_NAME, (module_info **) &pci)) {
		kprintf ("Can't open PCI module (?!?).\n");
		return (TRUE);
	}

	while (first <= last) {
		kprintf ("0x%04x.%02x:", port, first);
		for (i = 16;  --i >= 0; ) {
			(pci->write_io_8) (port, first++);
			kprintf (" %02x", (pci->read_io_8) (port + 1));
			if (first > last)
				break;
		}
		kprintf ("\n");
	}
	put_module (B_PCI_MODULE_NAME);
	return (TRUE);
}

static int
idxoutb (int ac, char **av)
{
	pci_module_info	*pci;
	register int	i, port;
	register uint8	idx, value;

	if (ac < 4) {
		kprintf ("usage: %s <ioreg> <index> <value>\n", av[0]);
		return (TRUE);
	}

	port = strtol (av[1], NULL, 0);
	if (port <= 0) {
		kprintf ("Bad port number.\n");
		return (TRUE);
	}
	idx = strtol (av[2], NULL, 0);
	value = strtol (av[3], NULL, 0);

	if (get_module (B_PCI_MODULE_NAME, (module_info **) &pci)) {
		kprintf ("Can't open PCI module (?!?).\n");
		return (TRUE);
	}

	(pci->write_io_8) (port, idx);
	(pci->write_io_8) (port + 1, value);

	put_module (B_PCI_MODULE_NAME);
	return (TRUE);
}


/****************************************************************************
 * Install/remove debugger commands.
 */
void
init_debug (void)
{
	add_debugger_command ("lynxemdump",
			      lynxemdump,
			      "lynxemdump - dump LynxEM kernel driver data");
	add_debugger_command ("idxinb",
			      idxinb,
			      "idxinb - dump VGA-style indexed I/O registers");
	add_debugger_command ("idxoutb",
			      idxoutb,
			      "idxoutb - write VGA-style indexed I/O registers");
}

void
uninit_debug (void)
{
	remove_debugger_command ("idxoutb", idxoutb);
	remove_debugger_command ("idxinb", idxinb);
	remove_debugger_command ("lynxemdump", lynxemdump);
}

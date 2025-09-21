/* :ts=8 bk=0
 *
 * debugcmd.c:	Kernel debugger commands.
 *
 * Leo L. Schwab					1999.12.23
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <stdlib.h>

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
i810dump (int argc, char **argv)
{
	int i;

	(void) argc;
	(void) argv;

	kprintf ("I810 kernel driver\nThere are %ld device(s)\n",
		 gdd->dd_NDevs);
	kprintf ("Driver-wide data: 0x%08lx\n", (uint32) gdd);
	kprintf ("Driver-wide semahpore: %ld\n", gdd->dd_DriverLock);
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
			kprintf ("      gfx_card_info: 0x%08lx\n",
			         (uint32) ci);
			kprintf ("       gfx_card_ctl: 0x%08lx\n",
			         (uint32) cc);
			kprintf ("  Base register 0,1: 0x%08lx, 0x%08lx\n",
				 (uint32) ci->ci_BaseAddr0,
				 (uint32) ci->ci_BaseAddr1);
			kprintf ("   \"Taken\" DMA Base: 0x%08lx\n",
				 di->di_TakenBase_DMA);
			kprintf ("        Cursor Base: 0x%08lx (0x%08lx)\n",
				 (uint32) ci->ci_CursorBase,
				 di->di_CursorBase_DMA);
			kprintf ("    Overlay reg src: 0x%08lx\n",
				 (uint32) ci->ci_OverlayRegs);
			kprintf ("          HW Status: 0x%08lx (0x%08lx)\n",
				 (uint32) ci->ci_HWStatusPage,
				 di->di_HWStatusPage_DMA);
			kprintf ("           GTT Base: 0x%08lx\n",
				 (uint32) di->di_GTT);
			kprintf ("           IRQFlags: 0x%08lx\n",
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
	register int	port;
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

static int
i810step (int ac, char **av)
{
	devinfo		*di;
	gfx_card_info	*ci;

	di = &gdd->dd_DI[0];	/*  Assume only one device.  */
	ci = di->di_GfxCardInfo;
	if (ac > 1) {
		/*  Switch stepping mode.  */
		int	i;

		i = strtol (av[1], NULL, 0);
		if (i)
			ci->ci_Regs->IMR &= ~MASKFIELD (IxR, BREAKPOINT);
		else
			ci->ci_Regs->IMR |= MASKFIELD (IxR, BREAKPOINT);
		kprintf ("i180 breakpoints %s.\n", i ? "enabled" : "disabled");
	} else
		ci->ci_Regs->IIR = MASKFIELD (IxR, BREAKPOINT);

	return (B_KDEBUG_CONT);
}

static int
i810status (int ac, char **av)
{
	devinfo			*di;
	gfx_card_info		*ci;
	int			i;
	static struct status	irqs[] = {
		{ VAL2FIELD (IxR, HWERR, TRUE), "HWERR" },
		{ VAL2FIELD (IxR, SYNCFLUSH, TRUE), "SYNCFLUSH" },
		{ VAL2FIELD (IxR, DISP0FLIPPENDING, TRUE), "DISP0FLIPPENDING" },
		{ VAL2FIELD (IxR, OVL0FLIPPENDING, TRUE), "OVL0FLIPPENDING" },
		{ VAL2FIELD (IxR, VBLANK0, TRUE), "VBLANK0" },
		{ VAL2FIELD (IxR, SOMETHINGHAPPENED0, TRUE),
		  "SOMETHINGHAPPENED0" },
		{ VAL2FIELD (IxR, USER, TRUE), "USER" },
		{ VAL2FIELD (IxR, BREAKPOINT, TRUE), "BREAKPOINT" }
	};
	static struct status	errs[] = {
		{ VAL2FIELD (ExR, MMLMREFRESH, TRUE), "MMLMREFRESH" },
		{ VAL2FIELD (ExR, PGTBL, TRUE), "PGTBL" },
		{ VAL2FIELD (ExR, DISPUNDERRUN, TRUE), "DISPUNDERRUN" },
		{ VAL2FIELD (ExR, CAPTUREOVERRUN, TRUE), "CAPTUREOVERRUN" },
		{ VAL2FIELD (ExR, HOSTPORT, TRUE), "HOSTPORT" },
		{ VAL2FIELD (ExR, INSTPARSER, TRUE), "INSTPARSER" }
	};
	
	(void) ac;
	(void) av;

	di = &gdd->dd_DI[0];	/*  Assume only one device.  */
	ci = di->di_GfxCardInfo;

	kprintf ("i810 Interrupts:\n");
	for (i = 0;  i < NUMOF (irqs);  i++) {
		kprintf ("  %c%c%c%c  ",
			 ci->ci_Regs->IER & irqs[i].mask ? 'E' : '-',
			 ci->ci_Regs->IIR & irqs[i].mask ? 'I' : '-',
			 ci->ci_Regs->IMR & irqs[i].mask ? 'M' : '-',
			 ci->ci_Regs->ISR & irqs[i].mask ? 'S' : '-');
		kprintf ("%s\n", irqs[i].name);
	}
	kprintf ("i810 Errors:\n");
	for (i = 0;  i < NUMOF (errs);  i++) {
		kprintf ("  -%c%c%c  ",
			 ci->ci_Regs->EIR & errs[i].mask ? 'I' : '-',
			 ci->ci_Regs->EMR & errs[i].mask ? 'M' : '-',
			 ci->ci_Regs->ESR & errs[i].mask ? 'S' : '-');
		kprintf ("%s\n", errs[i].name);
	}
	return (B_OK);
}

static int
ovlupdate (int ac, char **av)
{
	devinfo			*di;
	gfx_card_info		*ci;

	(void) ac;
	(void) av;

	di = &gdd->dd_DI[0];	/*  Assume only one device.  */
	ci = di->di_GfxCardInfo;
	kprintf ("Updating overlay: OV0ADDR <- 0x%08lx\n",
		 ci->ci_OverlayRegs_DMA);

	ci->ci_Regs->OV0ADDR = ci->ci_OverlayRegs_DMA | (1U << 31);

	return (B_OK);
}


#if DEBUG_SYNCLOG
static int
dumpsynclog (int ac, char **av)
{
	devinfo		*di;
	gfx_card_ctl	*cc;
	int		count, idx;

	di = &gdd->dd_DI[0];	/*  Assume only one device.  */
	cc = di->di_GfxCardCtl;

	if (ac < 2) {
		kprintf ("usage: %s <num-of-log-entries-to-dump>\n", av[0]);
		return (TRUE);
	}
	count = strtol (av[1], NULL, 0);
	if (count <= 0) {
		kprintf ("Bad count argument.\n");
		return (TRUE);
	}

	for (idx = cc->cc_SyncLogIdx - 1;  --count >= 0;  idx--) {
		if (idx < 0)
			idx += NSYNCLOGENTRIES;
		kprintf ("%c%c%c%c: 0x%08lx\n",
		         (char) ((cc->cc_SyncLog[idx][0] >> 24) & 0xff),
		         (char) ((cc->cc_SyncLog[idx][0] >> 16) & 0xff),
		         (char) ((cc->cc_SyncLog[idx][0] >> 8) & 0xff),
		         (char) (cc->cc_SyncLog[idx][0] & 0xff),
		         cc->cc_SyncLog[idx][1]);
	}

	return (B_OK);
}
#endif


/****************************************************************************
 * Install/remove debugger commands.
 */
void
init_debug (void)
{
	add_debugger_command ("i810dump",
			      i810dump,
			      "i810dump - dump I810 kernel driver data");
	add_debugger_command ("idxinb",
			      idxinb,
			      "idxinb - dump VGA-style indexed I/O registers");
	add_debugger_command ("idxoutb",
			      idxoutb,
			      "idxoutb - write VGA-style indexed I/O registers");
	add_debugger_command ("i810step",
			      i810step,
			      "i810step [0 | 1] - i180 rendering breakpoints/stepping");
	add_debugger_command ("i810status",
			      i810status,
			      "i810status - Print i810 HW status.");
	add_debugger_command ("ovlupdate",
			      ovlupdate,
			      "ovlupdate - Update on-chip overlay registers.");
#if DEBUG_SYNCLOG
	add_debugger_command ("dumpsynclog",
			      dumpsynclog,
			      "dumpsynclog <count> - Dump i810 sync token log buffer.");
#endif
}

void
uninit_debug (void)
{
#if DEBUG_SYNCLOG
	remove_debugger_command ("dumpsynclog", dumpsynclog);
#endif
	remove_debugger_command ("ovlupdate", ovlupdate);
	remove_debugger_command ("i810status", i810status);
	remove_debugger_command ("i810step", i810step);
	remove_debugger_command ("idxoutb", idxoutb);
	remove_debugger_command ("idxinb", idxinb);
	remove_debugger_command ("i810dump", i810dump);
}

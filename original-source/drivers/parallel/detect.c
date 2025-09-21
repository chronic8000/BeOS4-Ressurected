/*
** Detecting Port
*/ 

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>
#include <ISA.h>
#include <kernel/OS.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

#include <parallel_driver.h>
#include "parallel.h"
#include "ieee1284.h"
#include "detect.h"


/* ===================================================
** PARA_check_data_port
**
** Check if this port has valid data lines (SPP)
** This test _must_ pass for all // types
**
** Return:
**		true : Has valid data lines
**		false: This is not a // port, or it is broken
** =================================================== */
bool PARA_check_data_port(int io_base)
{
	bool err = false;
	const int v1 = 0xAA;
	const int v2 = 0x55;
	
	/* See "Parallel Port Complete", page 83 */
	write_io_8(io_base + PDATA, v1);
	if (read_io_8(io_base + PDATA) == v1)
	{
		write_io_8(io_base + PDATA, v2);
		if (read_io_8(io_base + PDATA) == v2)
			err = true;
	}

	return err;
}


/* ===================================================
** PARA_check_ecp_port
**
** Check if this port is ECP compliant (true)
** =================================================== */
bool PARA_check_ecp_port(int io_base)
{
	int ecr;
	int con;

	/* See "Parallel Port Complete", page 82 */
	write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
	ecr = read_io_8(io_base + ECR);
	con = read_io_8(io_base + PCON);

	/* 0) FIFO must be empty */
	if ((ecr & (EC_FIFOES|EC_FIFOFS)) != EC_FIFOES)
	{
		DD(bug("parallel: no ECP because FIFOES == 0\n"));
		return false;
	}

	/* 1) Verify that PCON != ECR */	
	write_io_8(io_base + PCON, (con ^ 0x02));
	if (read_io_8(io_base + ECR) != ecr)
	{ /* we modified PCON but ECR changed also! */
		DD(bug("parallel: no ECP because PCON == ECR\n"));
		return false;
	}

	/* 2) Verify that ECR[1:0] is read-only */
	write_io_8(io_base + ECR, (ecr ^ (EC_FIFOES|EC_FIFOFS)));
	if (read_io_8(io_base + ECR) != ecr)
	{
		DD(bug("parallel: no ECP because ECR's bit 0 & 1 are writtable\n"));
		return false;
	}

	DD(bug("parallel: ECP port detected\n"));
	return true;
}

/* ===================================================
** PARA_check_ps2_port
**
** Check if this port is PS2 compliant, simple bidir (true)
** =================================================== */
bool PARA_check_ps2_port(int io_base)
{
	/* See "Parallel Port Complete", page 83 */	
	write_io_8(io_base + PCON, C_INIT | C_DIR);
	if (PARA_check_data_port(io_base) == false)
	{
		DD(bug("parallel: PS/2 port detected\n"));
		write_io_8(io_base + PCON, C_INIT);
		return true;
	}
	return false;
}

/* ===================================================
** PARA_check_isa_port
**
** Check for ECP ISA 8 bits implementation
** =================================================== */
bool PARA_check_isa_port(par_port_info *d)
{
	int cfga;
	int cfgb;
	const int io_base = d->io_base;
	const int ecr = read_io_8(io_base + ECR);
	write_io_8(io_base + ECR, EC_CONFIG | EC_ERRINTR | EC_SERVICEINTR);
	cfga = read_io_8(io_base + ECPCFGA);
	cfgb = read_io_8(io_base + ECPCFGB);
	write_io_8(io_base + ECR, ecr);

	if ((cfga & 0x70) != 0x10)
	{
		DD(bug("parallel: unsupported ECP implementation (%02x)\n", cfga));
		return false;
	}
	DD(bug("parallel: 8 bits ISA ECP implementation detected\n"));


	DD(bug("parallel: ISA Interrupts trigger : %s\n", cfgb & 0x80 ? "Level" : "Edge"));
	if ((cfgb & 0x40) == 0)
	{ /* No IRQ :( */
		d->irq_num = -1;
		E(bug("parallel: ISA Interrupt conflict (disabling IRQ)!\n"));
	}

	return true;
}




/* ===================================================
** PARA_test_fifo
**
** Finds the size of the FIFO
** =================================================== */
bool PARA_test_fifo(par_port_info *d)
{
	int i;
	const int io_base = d->io_base;
	const int ecr = read_io_8(io_base + ECR);

	/* Switch to test mode */
	write_io_8(io_base + ECR, EC_TEST | EC_ERRINTR | EC_SERVICEINTR);

	if (!(read_io_8(io_base + ECR) & EC_FIFOES))
	{ /* The FIFO should be empty */
		DD(bug("parallel: FIFO is not empty!\n"));
		return false;
	}

	/* 1024 bytes is the max FIFO size availlable */
	for (i=0 ; i<1024 ; i++)
	{
		if (read_io_8(io_base + ECR) & EC_FIFOFS)
			break;
		write_io_8(io_base + SDFIFO, 0x55);
	}
	
	if (i == 1024)
	{ /* Some chipset use ECR but does not support FIFO mode */
		DD(bug("parallel: This chip seems to have an ECR but no FIFO!\n"));
		return false;
	}
	else if (i == 0)
	{ /* the FIFO can't be zero-length! */
		DD(bug("parallel: FIFO is 0 byte length!\n"));
		return false;
	}

	d->fifosize = i;
	DD(bug("parallel: ECP FIFO size is %d bytes\n", d->fifosize));

	/* Find the FIFO write threshold */
	write_io_8(io_base + ECR, EC_TEST | EC_ERRINTR | EC_SERVICEINTR);
	write_io_8(io_base + ECR, EC_TEST | EC_ERRINTR);
	for (i=0 ; i<d->fifosize ; i++)
	{
		read_io_8(io_base + SDFIFO);
		if (read_io_8(io_base + ECR) & EC_SERVICEINTR)
			break;
	}
	d->threshold_write = ((i == d->fifosize) ? (0) : (i+1));	
	DD(bug("parallel: ECP FIFO write threshold is %d bytes\n", d->threshold_write));

	/* Reset FIFO, reverse mode, Find the FIFO write threshold */
	write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
	write_io_8(io_base + PCON, C_INIT | C_DIR);
	write_io_8(io_base + ECR, EC_TEST | EC_ERRINTR | EC_SERVICEINTR);
	write_io_8(io_base + ECR, EC_TEST | EC_ERRINTR);
	for (i=0 ; i<d->fifosize ; i++)
	{
		write_io_8(io_base + SDFIFO, 0x55);
		if (read_io_8(io_base + ECR) & EC_SERVICEINTR)
			break;
	}
	d->threshold_read = ((i == d->fifosize) ? (0) : (i+1));	
	DD(bug("parallel: ECP FIFO read threshold is %d bytes\n", d->threshold_read));

	/* Reset FIFO */
	write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
	write_io_8(io_base + PCON, C_INIT);
	write_io_8(io_base + ECR, ecr);	
	return true;
}



/* ===================================================
** PARA_dma_channel
**
******************************************
** ****** DO NOT USE : NOT RELIABLE ******
******************************************
**
** returns:
**  -1 if no DMA supported, or the DMA channel (0 - 3)
** =================================================== */

int PARA_dma_channel(par_port_info *d)
{
	int dma_channel = -1;
	if (d->type & PAR_TYPE_ECP)
	{ /* DMA availlable only if we have an ECR */
		const int io_base = d->io_base;
		const int ecr = read_io_8(io_base + ECR);
		write_io_8(io_base + ECR, EC_CONFIG | EC_ERRINTR | EC_SERVICEINTR);
		dma_channel = read_io_8(io_base + ECPCFGB) & 0x03;
		write_io_8(io_base + ECR, ecr);	
		if (!dma_channel)
			dma_channel = -1;
	}
	DD(bug("parallel: DMA Channel : %d\n", dma_channel));
	return dma_channel;
}


/* ===================================================
** PARA_irq_line
**
** returns:
**  -1 if no IRQ supported, or the IRQ line
** =================================================== */

int PARA_irq_line(par_port_info *d)
{
	int irq = 0;
	const int tabIRQ[8] = { M_IRQ_NONE, 7, 9, 10, 11, 14, 15, 5 };
	if (d->type & PAR_TYPE_ECP)
	{ /* IRQ detection availlable only if we have an ECR */
		const int io_base = d->io_base;
		const int ecr = read_io_8(io_base + ECR);
		write_io_8(io_base + ECR, EC_CONFIG | EC_ERRINTR | EC_SERVICEINTR);
		irq = (read_io_8(io_base + ECPCFGB) >> 3) & 0x07;
		write_io_8(io_base + ECR, ecr);
	}

	DD(bug("parallel: IRQ # : %d\n", tabIRQ[irq]));
	return tabIRQ[irq];
}

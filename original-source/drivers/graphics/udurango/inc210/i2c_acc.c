/*---------------------------------------------------------------------------
 * I2C_ACC.C
 *
 * Version 2.0 - February 21, 2000.
 *
 * This file contains routines to write to and read from the I2C bus using
 * the ACCESS.bus hardware in the SC1400 or SC1200. 
 *
 * History:
 *    Initial version ported from code by Eyal Cohen.
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *---------------------------------------------------------------------------
 */

/* NUMBER OF READS BEFORE TIMING OUT */

#define ACC_I2C_TIMEOUT 1000000

/* CONSTANTS FOR ACCESS.bus INTERFACE */

#define AB_BASE_ADDR   0x820

#define ACBSDA			0	/* ACB serial data */
#define ACBST			1	/* ACB status */
#define ACBCST			2	/* ACB control status */
#define ACBCTL1			3	/* ACB control 1 */
#define ACBADDR			4	/* ACB own address */
#define ACBCTL2			5	/* ACB control 2 */

/* LOCAL ACCESS.bus FUNCTION DECLARATIONS */

void acc_i2c_start(void);
void acc_i2c_stop(void);
void acc_i2c_abort_data(void);
void acc_i2c_bus_recovery(void);
void acc_i2c_stall_after_start(int state);
void acc_i2c_send_address(unsigned char cData);
int acc_i2c_ack(int fPut, int negAck);
void acc_i2c_stop_clock(void);
void acc_i2c_activate_clock(void);
void acc_i2c_write_byte(unsigned char cData);
unsigned char acc_i2c_read_byte(void);
void acc_i2c_reset(void);
int acc_i2c_request_master(void);

/*---------------------------------------------------------------------------
 * gfx_i2c_reset
 *
 * This routine resets the I2C bus.
 *---------------------------------------------------------------------------
 */

#if GFX_I2C_DYNAMIC
	/* acc_i2c_reset routine already defined */
#else
void gfx_i2c_reset(void)
{
	acc_i2c_reset();
}
#endif

/*---------------------------------------------------------------------------
 * gfx_i2c_select_bus
 *
 * This routine selects which I2C bus to use.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int acc_i2c_select_bus(int bus)
#else
int gfx_i2c_select_bus(int bus)
#endif
{
	/* ### ADD ### Ability to use second I2C bus on SC1200. */
	/* Store bus number in static variable and enhance the read/write */
	/* routines to use that variable. */

	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_select_gpio
 *
 * This routine selects which GPIO pins to use.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int acc_i2c_select_gpio(int clock, int data)
#else
int gfx_i2c_select_gpio(int clock, int data)
#endif 
{
	/* THIS ROUTINE DOES NOT APPLY TO THE ACCESS.bus IMPLEMENTATION. */

	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_write
 *
 * This routine writes data to the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int acc_i2c_write(unsigned char chipadr, unsigned char subadr, 
	unsigned char data)
#else
int gfx_i2c_write(unsigned char chipadr, unsigned char subadr, 
	unsigned char data)
#endif 
{
	/* REQUEST MASTER */

	if (!acc_i2c_request_master()) return(1);

	/* WRITE ADDRESS COMMAND */

	acc_i2c_ack(1, 0);
	acc_i2c_send_address((unsigned char)(chipadr & 0xFE));
	if (!acc_i2c_ack(0, 0)) return(1);

	/* WRITE COMMAND */
	
	acc_i2c_write_byte(subadr);
	if (!acc_i2c_ack(0, 0)) return(1);

	/* WRITE DATA */
	
	acc_i2c_write_byte(data);
	if (!acc_i2c_ack(0, 0)) return(1);
	acc_i2c_stop();
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_read
 *
 * This routine reads data from the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int acc_i2c_read(unsigned char chipadr, unsigned char subadr, 
	unsigned char *data)
#else
int gfx_i2c_read(unsigned char chipadr, unsigned char subadr, 
	unsigned char data)
#endif 
{
	/* REQUEST MASTER */

	if (!acc_i2c_request_master()) return(1);

	/* WRITE ADDRESS COMMAND */

	acc_i2c_ack(1, 0);
	acc_i2c_send_address((unsigned char)(chipadr & 0xFE));
	if (!acc_i2c_ack(0, 0)) return(1);

	/* WRITE COMMAND */
	
	acc_i2c_write_byte(subadr);
	if (!acc_i2c_ack(0, 0)) return(1);

	/* START THE READ */

	acc_i2c_start();

	/* WRITE ADDRESS COMMAND */

	acc_i2c_ack(1, 1);
	acc_i2c_send_address((unsigned char)(chipadr | 0x01));
	if (!acc_i2c_ack(0, 0)) return(1);

	/* READ COMMAND */

	acc_i2c_ack(1, 1);
	*data = acc_i2c_read_byte();
	acc_i2c_stop();
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_write_multiple
 *
 * This routine writes multiple bytes of data to the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int acc_i2c_write_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char *data)
#else
int gfx_i2c_write_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char data)
#endif 
{
	/* ### ADD ### THIS ROUTINE IS NOT YET IMPLEMENTED FOR ACCESS.bus */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_read_multiple
 *
 * This routine reads multiple bytes of data from the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int acc_i2c_read_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char *data)
#else
int gfx_i2c_read_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char data)
#endif 
{
	/* ### ADD ### THIS ROUTINE IS NOT YET IMPLEMENTED FOR ACCESS.bus */
	return(0);
}

/*--------------------------------------------------------*/
/*  LOCAL ROUTINES SPECIFIC TO ACCESS.bus IMPLEMENTATION  */
/*--------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * acc_i2c_reset
 *
 * This routine resets the I2C bus.
 *---------------------------------------------------------------------------
 */
void acc_i2c_reset(void)
{
	unsigned char reg;

	/* Disable the ACCESS.bus device and */
	/* Configure the SCL frequency: 56 clock cycles */
	OUTB(AB_BASE_ADDR + ACBCTL2, 0x70);

	/* Configure no interrupt mode (polling) and */
	/* Disable global call address */
	OUTB(AB_BASE_ADDR + ACBCTL1, 0x0);

	/* Disable slave address */
	OUTB(AB_BASE_ADDR + ACBADDR, 0x0);

	/* Enable the ACCESS.bus device */
 	reg = INB(AB_BASE_ADDR + ACBCTL2);
	reg |= 0x01;
	OUTB(AB_BASE_ADDR + ACBCTL2, reg);

  	/* Issue STOP event */

	acc_i2c_stop();

	/* Clear NEGACK, STASTR and BER bits */
	OUTB(AB_BASE_ADDR + ACBST, 0x38);

	/* Clear BB (BUS BUSY) bit */
 	reg = INB(AB_BASE_ADDR + ACBCST);
	reg |= 0x02;
	OUTB(AB_BASE_ADDR + ACBCST, reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_start
 *
 * This routine starts a transfer on the I2C bus.
 *---------------------------------------------------------------------------
 */
void acc_i2c_start(void)
{
	unsigned char reg;
	reg = INB(AB_BASE_ADDR + ACBCTL1);
	reg |= 0x01;
	OUTB(AB_BASE_ADDR + ACBCTL1, reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_stop
 *
 * This routine stops a transfer on the I2C bus.
 *---------------------------------------------------------------------------
 */
void acc_i2c_stop(void)
{
	unsigned char reg;
	reg = INB(AB_BASE_ADDR + ACBCTL1);
	reg |= 0x02;
	OUTB(AB_BASE_ADDR + ACBCTL1, reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_abort_data
 *---------------------------------------------------------------------------
 */
void acc_i2c_abort_data(void)
{
	unsigned char reg;
	acc_i2c_stop();
	reg = INB(AB_BASE_ADDR + ACBCTL1);
	reg |= 0x10;
	OUTB(AB_BASE_ADDR + ACBCTL1, reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_bus_recovery
 *---------------------------------------------------------------------------
 */
void acc_i2c_bus_recovery(void)
{
	acc_i2c_abort_data();
	acc_i2c_reset();
}

/*---------------------------------------------------------------------------
 * acc_i2c_stall_after_start
 *---------------------------------------------------------------------------
 */
void acc_i2c_stall_after_start(int state)
{
	unsigned char reg;
	reg = INB(AB_BASE_ADDR + ACBCTL1);
	if (state) reg |= 0x80;
	else reg &= 0x7F;
	OUTB(AB_BASE_ADDR + ACBCTL1, reg);

	if (!state)
	{
		reg = INB(AB_BASE_ADDR + ACBST);
		reg |= 0x08;
		OUTB(AB_BASE_ADDR + ACBST, reg);
	}
}

/*---------------------------------------------------------------------------
 * acc_i2c_send_address
 *---------------------------------------------------------------------------
 */
void acc_i2c_send_address(unsigned char cData)
{
	unsigned char reg;
	unsigned long timeout = 0;

	acc_i2c_stall_after_start(1);
	
	/* WRITE THE DATA */

	OUTB(AB_BASE_ADDR + ACBSDA, cData);
	while (1) {
		reg = INB(AB_BASE_ADDR + ACBST);
		if ((reg & 0x38) != 0)		/* check STASTR, BER and NEGACK */
			break;
        if (timeout++ == ACC_I2C_TIMEOUT)
        {
            acc_i2c_bus_recovery();
            return;
        }
	}

	/* CHECK FOR BUS ERROR */

	if (reg & 0x20) 
	{	
		acc_i2c_bus_recovery();
		return;
	}

	/* CHECK NEGATIVE ACKNOWLEDGE */

	if (reg & 0x10) 
	{	
		acc_i2c_abort_data();
		return;
	}
	acc_i2c_stall_after_start(0);
}

/*---------------------------------------------------------------------------
 * acc_i2c_ack
 *
 * This routine looks for acknowledge on the I2C bus.
 *---------------------------------------------------------------------------
 */
int acc_i2c_ack(int fPut, int negAck)
{
	unsigned char reg;
	unsigned long timeout = 0;

	if (fPut) {		/* read operation */
		if (!negAck) {
			/* Push Ack onto I2C bus */
			reg = INB(AB_BASE_ADDR + ACBCTL1);
			reg &= 0xE7;
			OUTB(AB_BASE_ADDR + ACBCTL1, reg);
		}
		else {
			/* Push negAck onto I2C bus */
			reg = INB(AB_BASE_ADDR + ACBCTL1);
			reg |= 0x10;
			OUTB(AB_BASE_ADDR + ACBCTL1, reg);
		}
	} else {		/* write operation */
		/* Receive Ack from I2C bus */
		while (1) {
			reg = INB(AB_BASE_ADDR + ACBST);
			if ((reg & 0x70) != 0)		/* check SDAST, BER and NEGACK */
				break;
	        if (timeout++ == ACC_I2C_TIMEOUT)
			{
				acc_i2c_bus_recovery();
				return(0);
			}
		}

		/* CHECK FOR BUS ERROR */

		if (reg & 0x20) 
		{	
			acc_i2c_bus_recovery();
			return(0);
		}

		/* CHECK NEGATIVE ACKNOWLEDGE */

		if (reg & 0x10) 
		{	
			acc_i2c_abort_data();
			return(0);
		}
	}
	return (1);
}

/*---------------------------------------------------------------------------
 * acc_i2c_stop_clock
 *
 * This routine stops the ACCESS.bus clock.
 *---------------------------------------------------------------------------
 */
void acc_i2c_stop_clock(void) 
{
   unsigned char reg;
   reg = INB(AB_BASE_ADDR + ACBCTL2);
   reg &= ~0x01;
   OUTB(AB_BASE_ADDR + ACBCTL2, reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_activate_clock
 *
 * This routine activates the ACCESS.bus clock.
 *---------------------------------------------------------------------------
 */
void acc_i2c_activate_clock(void)
{
   unsigned char reg;
   reg = INB(AB_BASE_ADDR + ACBCTL2);
   reg |= 0x01;
   OUTB(AB_BASE_ADDR + ACBCTL2, reg);
}

/*---------------------------------------------------------------------------
 * acc_i2c_write_byte
 *
 * This routine writes a byte to the I2C bus
 *---------------------------------------------------------------------------
 */
void acc_i2c_write_byte(unsigned char cData)
{
	unsigned char reg;
	unsigned long timeout = 0;

	while (1) {
		reg = INB(AB_BASE_ADDR + ACBST);
		if (reg & 0x70) break;
        if (timeout++ == ACC_I2C_TIMEOUT)
        {
            acc_i2c_bus_recovery();
            return;
        }
	}

	/* CHECK FOR BUS ERROR */

	if (reg & 0x20) 
	{	
		acc_i2c_bus_recovery();
		return;
	}

	/* CHECK NEGATIVE ACKNOWLEDGE */

	if (reg & 0x10) 
	{	
		acc_i2c_abort_data();
		return;
	}

	/* WRITE THE DATA */

	OUTB(AB_BASE_ADDR + ACBSDA, cData);
}

/*---------------------------------------------------------------------------
 * acc_i2c_read_byte
 *
 * This routine reads a byte from the I2C bus
 *---------------------------------------------------------------------------
 */
unsigned char acc_i2c_read_byte(void)
{
	unsigned char cData, reg;
	unsigned long timeout = 0;

	while (1) {
		reg = INB(AB_BASE_ADDR + ACBST);
		if (reg & 0x60) break;
        if (timeout++ == ACC_I2C_TIMEOUT)
        {
            acc_i2c_bus_recovery();
            return(0xEF);
        }
	}

	/* CHECK FOR BUS ERROR */

	if (reg & 0x20)
	{
		acc_i2c_bus_recovery();
		return(0xEE);
	}

	/* READ DATA */

	acc_i2c_stop_clock();
	cData = INB(AB_BASE_ADDR + ACBSDA);
	acc_i2c_activate_clock();
	return (cData);
}


/*---------------------------------------------------------------------------
 * acc_i2c_request master
 *---------------------------------------------------------------------------
 */
int acc_i2c_request_master(void)
{
	unsigned char reg;
	unsigned long timeout = 0;

	acc_i2c_start();
	while (1) {
		reg = INB(AB_BASE_ADDR + ACBST);
		if (reg & 0x60) break;
        if (timeout++ == ACC_I2C_TIMEOUT)
        {
            acc_i2c_bus_recovery();
            return(0);
        }
	}

	/* CHECK FOR BUS ERROR */

	if (reg & 0x20) 
	{	
		acc_i2c_abort_data();
		return(0);
	}

	/* CHECK NEGATIVE ACKNOWLEDGE */

	if (reg & 0x10) 
	{	
		acc_i2c_abort_data();
		return(0);
	}
	return(1);
}
/* END OF FILE */

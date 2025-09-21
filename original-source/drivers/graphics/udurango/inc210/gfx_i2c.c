/*---------------------------------------------------------------------------
 * GFX_I2C.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to write to and read from the I2C bus.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *---------------------------------------------------------------------------
 */

/* INCLUDE ROUTINES FOR ACCESS.BUS, IF SPECIFIED */
/* This is for SC1200 and SC1400 systems. */

#if GFX_I2C_ACCESS
#include "i2c_acc.c"
#endif

/* INCLUDE ROUTINES FOR CS5530 GPIOs, IF SPECIFIED */
/* This is for GXLV systems that use GPIOs on the CS5530 for I2C. */

#if GFX_I2C_GPIO
#include "i2c_gpio.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either ACCESS.bus or GPIO routines. */

#if GFX_I2C_DYNAMIC

/*---------------------------------------------------------------------------
 * gfx_i2c_reset
 *---------------------------------------------------------------------------
 */
void gfx_i2c_reset(void)
{
#if GFX_I2C_ACCESS
	if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
		acc_i2c_reset();
#endif
#if GFX_I2C_GPIO
	if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
		gpio_i2c_reset();
#endif
}

/*---------------------------------------------------------------------------
 * gfx_i2c_select_bus
 *---------------------------------------------------------------------------
 */
int gfx_i2c_select_bus(int bus)
{
#if GFX_I2C_ACCESS
	if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
		acc_i2c_select_bus(bus);
#endif
#if GFX_I2C_GPIO
	if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
		gpio_i2c_select_bus(bus);
#endif
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_select_gpio
 *---------------------------------------------------------------------------
 */
int gfx_i2c_select_gpio(int clock, int data)
{
#if GFX_I2C_ACCESS
	if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
		acc_i2c_select_gpio(clock, data);
#endif
#if GFX_I2C_GPIO
	if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
		gpio_i2c_select_gpio(clock, data);
#endif
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_write
 *---------------------------------------------------------------------------
 */
int gfx_i2c_write(unsigned char chipadr, unsigned char subadr, 
	unsigned char data)
{
	int status = -1;
#if GFX_I2C_ACCESS
	if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
		status = acc_i2c_write(chipadr, subadr, data);
#endif
#if GFX_I2C_GPIO
	if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
		status = gpio_i2c_write(chipadr, subadr, data);
#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_read
 *---------------------------------------------------------------------------
 */
int gfx_i2c_read(unsigned char chipadr, unsigned char subadr, 
	unsigned char *data)
{
	int status = -1;
#if GFX_I2C_ACCESS
	if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
		status = acc_i2c_read(chipadr, subadr, data);
#endif
#if GFX_I2C_GPIO
	if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
		status = gpio_i2c_read(chipadr, subadr, data);
#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_write_multiple
 *---------------------------------------------------------------------------
 */
int gfx_i2c_write_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char *data)
{
	int status = -1;
#if GFX_I2C_ACCESS
	if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
		status = acc_i2c_write_multiple(chipadr, subadr, count, data);
#endif
#if GFX_I2C_GPIO
	if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
		status = gpio_i2c_write_multiple(chipadr, subadr, count, data);
#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_read_multiple
 *---------------------------------------------------------------------------
 */
int gfx_i2c_read_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char *data)
{
	int status = -1;
#if GFX_I2C_ACCESS
	if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
		status = acc_i2c_read_multiple(chipadr, subadr, count, data);
#endif
#if GFX_I2C_GPIO
	if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
		status = gpio_i2c_read_multiple(chipadr, subadr, count, data);
#endif
	return(status);
}

#endif /* GFX_I2C_DYNAMIC */
	
/* END OF FILE */

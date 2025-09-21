/*---------------------------------------------------------------------------
 * I2C_GPIO.C
 *
 * Version 2.0 - February 21, 2000.
 *
 * This file contains routines to write to and read from the I2C bus using
 * the GPIO pins of the CS5530.
 *
 * History:
 *    Not started.
 *
 * Copyright (c) 2000 National Semiconductor.
 *---------------------------------------------------------------------------
 */

/* STATIC VARIABLES TO STORE WHAT GPIO PINS TO USE */

int gpio_clock = 0;
int gpio_data = 0;

/* ### ADD ### ANY LOCAL ROUTINE DEFINITIONS SPECIFIC TO GPIO */

/*---------------------------------------------------------------------------
 * gfx_i2c_reset
 *
 * This routine resets the I2C bus.
 *---------------------------------------------------------------------------
 */

#if GFX_I2C_DYNAMIC
void gpio_i2c_reset(void)
#else
void gfx_i2c_reset(void)
#endif
{
	/* ### ADD ### Any code needed to reset the state of the GPIOs. */
}

/*---------------------------------------------------------------------------
 * gfx_i2c_select_bus
 *
 * This routine selects which I2C bus to use.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int gpio_i2c_select_bus(int bus)
#else
int gfx_i2c_select_bus(int bus)
#endif
{
	/* THIS ROUTINE DOES NOT APPLY TO THE GPIO IMPLEMENTATION. */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_select_gpio
 *
 * This routine selects which GPIO pins to use.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int gpio_i2c_select_gpio(int clock, int data)
#else
int gfx_i2c_select_gpio(int clock, int data)
#endif 
{
	gpio_clock = clock;
	gpio_data = data;
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_write
 *
 * This routine writes data to the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int gpio_i2c_write(unsigned char chipadr, unsigned char subadr, 
	unsigned char data)
#else
int gfx_i2c_write(unsigned char chipadr, unsigned char subadr, 
	unsigned char data)
#endif 
{
	/* ### ADD ### CODE TO WRITE BYTE TO I2B BUS */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_read
 *
 * This routine reads data from the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int gpio_i2c_read(unsigned char chipadr, unsigned char subadr, 
	unsigned char *data)
#else
int gfx_i2c_read(unsigned char chipadr, unsigned char subadr, 
	unsigned char data)
#endif 
{
	/* ### ADD ### CODE TO WRITE BYTE TO I2B BUS */
	/* For now return clock and data pins */

	*data = gpio_clock | (gpio_data << 4);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_write_multiple
 *
 * This routine writes multiple bytes of data to the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int gpio_i2c_write_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char *data)
#else
int gfx_i2c_write_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char data)
#endif 
{
	/* ### ADD ### THIS ROUTINE IS NOT YET IMPLEMENTED FOR GPIO */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_read_multiple
 *
 * This routine reads multiple bytes of data from the specified I2C address.
 *---------------------------------------------------------------------------
 */
#if GFX_I2C_DYNAMIC
int gpio_i2c_read_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char *data)
#else
int gfx_i2c_read_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char data)
#endif 
{
	/* ### ADD ### THIS ROUTINE IS NOT YET IMPLEMENTED FOR GPIO */
	return(0);
}

/* END OF FILE */

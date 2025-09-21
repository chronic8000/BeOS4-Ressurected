/* Based on work by F.M.Birth. */

#include "iic.h"

static void iic_start(envy24_dev* card);
static void iic_stop(envy24_dev* card);
static uint8 iic_send_byte(envy24_dev* card, uint8 byte);

status_t iic_write_byte(envy24_dev* card, uint8 iic_address, uint8 byte)
{
	iic_start(card);
	
	// send IIC address and data byte
	if (!iic_send_byte(card, iic_address)) return B_ERROR;	
	if (!iic_send_byte(card, byte)) return B_ERROR;
	
	iic_stop(card);
	
	return B_OK;
}

static void iic_start(envy24_dev* card)
{
	// falling edge of SDA while SCL is HIGH
	iic_set_iic(card, 0x01, 0x01);
	iic_set_iic(card, 0x00, 0x01);
}

static void iic_stop(envy24_dev* card)
{
	// rising edge of SDA while SCL is HIGH
	iic_set_iic(card, 0x00, 0x01);
	iic_set_iic(card, 0x01, 0x01);
}

static uint8 iic_send_byte(envy24_dev* card, uint8 byte)
{
	uint8 data_bit;
	uint8 ack;
	int i;
	
	// send byte (MSB first)
	for (i = 7; i >= 0; i--) {
		data_bit = (byte >> i) & 0x01;
		
		iic_set_iic(card, data_bit, 0x00);
		iic_set_iic(card, data_bit, 0x01);
	}
	
	iic_set_iic(card, 0x01, 0x00);
	
	// Get acknowledge
	iic_set_iic(card, 0x01, 0x01);
	ack = iic_get_sda(card);
	iic_set_iic(card, 0x00, 0x00);
	
	return (!ack);	// ack == 0 --> success
}

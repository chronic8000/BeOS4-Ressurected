/* Based on work by F.M.Birth. */

#ifndef _IIC_H_
#define _IIC_H_

#include "envy24.h"

status_t iic_write_byte(envy24_dev* card, uint8 iic_address, uint8 byte);
void iic_set_iic(envy24_dev* card, uint8 sda, uint8 scl);
uint8 iic_get_sda(envy24_dev* card);

#endif // #ifndef _IIC_H_

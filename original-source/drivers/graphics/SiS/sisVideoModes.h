#ifndef SISVIDEOMODES_H
#define SISVIDEOMODES_H

#include <SupportDefs.h>
#include "driver.h"

void setup_DAC(void);
void set_color_mode(uint32 depth, devinfo *di);
void sis630_SetColorMode(uint32 depth, devinfo *di) ;

status_t Move_Display_Area (uint16 x, uint16 y, devinfo *di);
void Set_Indexed_Colors (struct data_ioctl_set_indexed_colors *p);

void Screen_Off(void);
void Restart_Display(void);

#endif

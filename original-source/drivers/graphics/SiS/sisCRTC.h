#ifndef SISCRTC_H
#define SISCRTC_H

#include <SupportDefs.h>
#include "driver.h"
#include "driver_ioctl.h"

/* CRT Controller Registers */
#define CRTC_INDEX 0x03d4
#define CRTC_DATA  0x03d5

void Set_CRT(data_ioctl_sis_CRT_settings *sis_CRT_settings, devinfo *di);
void Init_CRT_Threshold(uint32 dot_clock, devinfo *di);

struct data_ioctl_sis_CRT_settings *prepare_CRT(devinfo *di, display_mode *dm, uint32 screen_start_address, uint32 pixels_per_row) ;
extern data_ioctl_sis_CRT_settings sis_CRT_settings ;

#endif

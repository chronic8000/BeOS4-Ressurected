#ifndef SISCURSOR_H
#define SISCURSOR_H

#include <SupportDefs.h>
#include "driver.h"

status_t _set_cursor_shape(struct data_ioctl_set_cursor_shape *sis_data, devinfo *di);
void _show_cursor(uint32 on);
void _move_cursor(int16 screenX,int16 screenY, devinfo *di);

#endif

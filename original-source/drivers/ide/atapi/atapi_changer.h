#ifndef ATAPI_CHANGER_H
#define ATAPI_CHANGER_H

#include "atapi_devicelist.h"

extern status_t load_cd(device_handle *h);
extern status_t reconfigure_changer(device_entry *d);
extern status_t configure_changer(device_entry *d);
extern status_t check_current_slot(device_handle *h);

#endif

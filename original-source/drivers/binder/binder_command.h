#ifndef BINDER_COMMAND_H
#define BINDER_COMMAND_H

#include "binder_node.h"
#include <kernel/OS.h>
#include <drivers/Drivers.h>

status_t init_binder_command();
void uninit_binder_command();

#endif


#ifndef DETECT_H
#define DETECT_H

#include "parallel.h"

bool PARA_check_data_port(int io_base);
bool PARA_check_ecp_port(int io_base);
bool PARA_check_ps2_port(int io_base);
bool PARA_check_isa_port(par_port_info *d);
bool PARA_test_fifo(par_port_info *d);
int PARA_dma_channel(par_port_info *d);
int PARA_irq_line(par_port_info *d);

#endif

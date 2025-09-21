#ifndef TPC_H
#define TPC_H

#include <SupportDefs.h>

#define TPC_SET_CMD             0xe1
#define TPC_READ_REG            0x4b
#define TPC_READ_PAGE_DATA      0x2d
#define TPC_GET_INT             0x78
#define TPC_WRITE_PAGE_DATA     0xd2
#define TPC_WRITE_REG           0xb4
#define TPC_SET_RW_REG_ADDR     0x87

status_t tpc_read(uint8 tpc, uint8 *data, size_t data_len);
status_t tpc_write(uint8 tpc, const uint8 *data, size_t data_len);

status_t tpc_get_media_status(bigtime_t last_checked);
status_t tpc_reset();
status_t tpc_wait_int(bigtime_t timeout);
status_t tpc_init();
void     tpc_uninit();

#endif


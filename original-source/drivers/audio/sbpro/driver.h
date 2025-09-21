/*	driver.h*/
/*	Sound Blaster Pro*/
/*	Joseph Wang*/
/*	4.4.99*/

#ifndef DRIVER_H
#define DRIVER_H




extern spinlock lock;
extern cpu_status status;
extern int port, irq, dma16, mpu;

void acquire_sl();
void release_sl();
status_t check_hw();
status_t reset_hw();

uint8 read_io(int offset);
uint8 read_reg(uint8 reg);
status_t read_data(uint8 *value);
void write_io(int offset, uint8 value);
void write_reg(uint8 reg, uint8 value);
status_t write_data(uint8 value);
int32 select_inth(void *data);

extern status_t pcm_open(const char *name, uint32 flags, void **cookie);
extern status_t pcm_close(void *cookie);
extern status_t pcm_free(void *cookie);
extern status_t pcm_control(void *cookie, uint32 op, void *data, size_t len);
extern status_t pcm_read(void *cookie, off_t pos, void *data, size_t *len);
extern status_t pcm_write(void *cookie, off_t pos, const void *data, size_t *len);

#endif

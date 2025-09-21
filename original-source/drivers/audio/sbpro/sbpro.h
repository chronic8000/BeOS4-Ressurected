/*	sbpro.h*/
/*	Sound Blaster Pro*/
/*	Joseph Wang*/
/*	4.4.99*/

#ifndef SBPRO_H
#define SBPRO_H

#define DEFAULT_PORT				0x220
#define DEFAULT_IRQ					5
#define DEFAULT_DMA8				1
#define DEFAULT_DMA16				5
#define DEFAULT_MPU					0x330
#define DMA_BUFFER_SIZE				2*B_PAGE_SIZE
#define SBPRO_DELAY					3

/*port offsets*/
#define SBPRO_RESET					0x6
#define SBPRO_READ_BUFFER_STATUS	0xe
#define SBPRO_READ_DATA				0xa
#define SBPRO_WRITE_BUFFER_STATUS	0xc
#define SBPRO_WRITE_DATA			0xc
#define SBPRO_REG					0x4
#define SBPRO_DATA					0x5

/*commands*/
#define SBPRO_VERSION				0xe1
#define SBPRO_SPEAKER_ENABLE		0xd1
#define SBPRO_SPEAKER_DISABLE		0xd3
#define SBPRO_TIME_CONSTANT			0x40
#define SBPRO_TRANSFER_SIZE			0x48
#define SBPRO_PLAYBACK_8_BIT		0x90
#define SBPRO_EXIT_DMA				0xda

/*mixer commands*/
#define SBPRO_VOICE					0x04
#define SBPRO_MIC					0x0a
#define SBPRO_INPUT_MUX				0x0c
#define SBPRO_OUTPUT_MUX			0x0e
#define SBPRO_MASTER				0x22
#define SBPRO_MIDI					0x26
#define SBPRO_CD					0x28
#define SBPRO_LINE					0x2e
typedef struct dma_buffer {
	char *data;
	int32 size;
	area_id area;
	int32 halfcount;
	sem_id int_sem;
	int32 pending;
	int32 written;
	} dma_buffer;

extern dma_buffer write_buffer, read_buffer;
extern struct isa_module_info *isa_info;
extern int dma8;

void acquire_sl(void);
void release_sl(void);
void write_reg(uint8 reg, uint8 value);
uint8 read_reg(uint8 reg);
status_t write_data(uint8 value);
status_t read_data(uint8 *value);
status_t reset_hw(void);

#endif

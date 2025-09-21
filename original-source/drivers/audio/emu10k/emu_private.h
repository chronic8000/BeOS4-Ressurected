#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include "emu10k_driver.h"

#define CHIP_NAME "emu10k"

#define ddprintf
//#define ddprintf dprintf
#define X(N) dprintf("*%d*\n",(N));

#define NUM_CARDS 1
#define DEV_NAME_LEN 32

#define EMU_VENDOR_ID 0x1102
#define EMU_DEVICE_ID 0x0002

#if defined(__INTEL__)
#define EIEIO()
#elif defined(__POWERPC__)
#define EIEIO() __eieio()
#endif

#define READ_IO_8(A) ((*pci->read_io_8)(A))
#define WRITE_IO_8(A,D) ((*pci->write_io_8)(A,D))
#define READ_IO_16(A) ((*pci->read_io_16)(A))
#define WRITE_IO_16(A,D) ((*pci->write_io_16)(A,D))
#define READ_IO_32(A) ((*pci->read_io_32)(A))
#define WRITE_IO_32(A,D) ((*pci->write_io_32)(A,D))
#define GET_NTH_PCI_INFO(N,DEV) ((*pci->get_nth_pci_info)(N,DEV))
#define READ_PCI_CONFIG(B,D,F,O,S) ((*pci->read_pci_config)(B,D,F,O,S))
#define WRITE_PCI_CONFIG(B,D,F,O,S,V) ((*pci->write_pci_config)(B,D,F,O,S,V))

typedef struct {
	char		name[DEV_NAME_LEN];
	int32		open_count;
	sem_id		int_sem;
	pci_info	info;
	spinlock	reg_lock;	// lock hardware regs and int data
	emu_int_data int_data;
} emu_card;

#define AUDIO_BASE(card) (card->info.u.h0.base_registers[0])
#define JOY_BASE(card) (card->info.u.h0.base_registers[1])


/* From e10k1reg.h */

/* Sound Engine pointer and data registers */
#define PTRREG  0x0000U 
#define DATAREG 0x0004U 

/* Interrrupt pending and Interrupt enable reigster */
#define INTP   0x0008U
#define INTE   0x000CU
#define CLIPL   0x005a0000   /* Channel Loop interrupt pending (low)  */
#define CLIPH   0x005b0000   /* Channel Loop interrupt pending (high)  */

/* Wall clock */
#define WC     0x0010U

/* "Hardware control"  register */
#define HC     0x0014U

/* Midi UART Data and Command/Status registers - byte-wide */
#define MUDTA  0x0018U

/* Timer interrupt register - holds the interrupt period in samples */
#define TIMR   0x001AU

/* AC97 I/F registers - Data and Address/Status */
#define AC97D  0x001CU
#define AC97A  0x001EU

/* Interrupt pending bit fields */
#define EINTP_TX  0x00000100
#define EINTP_RX  0x00000080
#define EINTP_CL  0x00000040

/* Channel registers */
#define CPF    0x00000000   /* Current Pitch and Fraction */
#define PTAB   0x00010000   /* Pitch Target and Sends A and B */
#define VTFT   0x00030000   /* Volume Target, Filter cutoff Target */
#define CCR    0x00090000   /* cache control */

/* Envelope Registers */
#define VEDS   0x00120000   /* Volume Envelope Decay and Sustain */

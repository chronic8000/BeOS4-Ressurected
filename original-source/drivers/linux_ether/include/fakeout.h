#ifndef __B_LX_FAKEOUT_H_
#define __B_LX_FAKEOUT_H_

#include <byteorder.h>
#include <KernelExport.h>

#define jiffies fjiffies()
#define save_flags(flags) _save_flags(&(flags))
#define panic __panic
#define printk __printk
#define inline
#define __KERNEL__ 1
#define __signed__ signed
#define __inline__
#define EISA_bus 0
#define asmlinkage
#define SA_SHIRQ 1
#define LINUX_VERSION_CODE 0x21700
#define MOD_INC_USE_COUNT do { } while (0)
#define MOD_DEC_USE_COUNT do { } while (0)

#define MODULE 1

#define outb _linux_outb
#define outw _linux_outw
#define outl _linux_outl

#define inb _linux_inb
#define inw _linux_inw
#define inl _linux_inl


void _linux_outb(unsigned char data, unsigned short port);
void _linux_outw(unsigned short data, unsigned short port);
void _linux_outl(unsigned long data, unsigned short port);

unsigned char _linux_inb(unsigned short port);
unsigned short _linux_inw(unsigned short port);
unsigned long _linux_inl(unsigned short port);

// mem mappings
#define ETHER_BUF_SIZE  64			/* size of minimum ethernet buffer */
#define N_ETHER_BUFS    3072 // 0521 was 512			/* total number of these buffers */
#define ETHER_LOW_MEM_THRESH  40	/* when to start worrying about low mem */
#define PHYS_ENTRIES	64
#define BADADDRESS		0xBADADDE5

extern uchar * 		get_area(size_t size, area_id *areap);
extern int			init_area(uchar *base);
extern void* 		bus_to_virt(ulong ph_addr);
extern ulong 		virt_to_bus(void * v_addr);

extern int 			ring_init_addr_table(void);
extern int 			ring_release_addr(uchar * v_addr);
extern int 			ring_map_n_store_addr(uchar * v_addr, int v_size);
extern void* 		ring_bus_to_virt(ulong ph_addr);
extern ulong 		ring_virt_to_bus(void * v_addr);

#endif //__B_LX_FAKEOUT_H_

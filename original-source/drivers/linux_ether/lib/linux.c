/*
 * linux.c
 * Copyright (c) 1997 Be, Inc.	All Rights Reserved 
 */
#include "fakeout.h"
#include <stdarg.h>
#include <PCI.h>
#include <OS.h>
#include <Errors.h>
#include <stdlib.h>
#include <linux/timer.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <KernelExport.h>


#define ABS(x) (((x) < 0) ? -(x) : (x))
#define TX_RETRIES 4

/*
 * For dealing with bitmaps (locating free buffers)
 */
#define CHUNKNO(x) ((x) / sizeof(ether_bitmap[0]))
#define CHUNKBIT(x) (1 << ((x) % sizeof(ether_bitmap[0])))

/*
 * lock/unlock a device (or anything with a ps & lock field)
 */
#define ALOCK_ACQUIRE(x) alock_acquire(&(x)->ps, &(x)->lock)
#define ALOCK_RELEASE(x) alock_release(&(x)->ps, &(x)->lock)


#undef outb
#undef outw
#undef outl
#undef inb
#undef inw
#undef inl


typedef struct timer_stuff {
	struct timer_list	*timer;
	sem_id 				sem;
	sem_id				done;
	thread_id			thread;
	struct timer_stuff	*next;
} timer_stuff_t;		

typedef struct intstuff {
	unsigned int irq;
	int32 (*handler)(int, void *, struct pt_regs *);
	void *dev_id;
	struct intstuff *next;
} intstuff_t;

typedef struct alock {
	cpu_status ps;
	spinlock lock;
} alock_t;

static unsigned char *dev_alloc(size_t size);
static void dev_free(unsigned char *buf, size_t size);

//0519++
#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))
#define AREA_SIZE	N_ETHER_BUFS * ETHER_BUF_SIZE
static unsigned char * ether_bufs;
//static unsigned char ether_bufs[N_ETHER_BUFS * ETHER_BUF_SIZE];
static area_id 	lxw_area;
//0519--

static unsigned long ether_bitmap[N_ETHER_BUFS / sizeof(unsigned long)];
static int ether_nbufs = N_ETHER_BUFS;

static timer_stuff_t *timers;
static sem_id locksem = B_ERROR;
static sem_id timerlocksem = B_ERROR;
unsigned long bh_active;

static intstuff_t *ints;
static struct device *devices;

static alock_t ilock;
static long global_lock;
static alock_t devlock;

static void Lock(void)
{
	acquire_sem(locksem);
}

static void Unlock(void)
{
	release_sem(locksem);
}

static status_t TimerLock(void)
{
	return (acquire_sem(timerlocksem));
}

static void TimerUnlock(void)
{
	release_sem(timerlocksem);
}

static void TimerBreakLock(void)
{
	if (TimerLock() == B_NO_ERROR) {
		delete_sem(timerlocksem);
		timerlocksem = B_ERROR;
	}
}


extern pci_module_info	*pci;


status_t
timer_thread(void *arg)
{
	timer_stuff_t *stuff = (timer_stuff_t *)arg;
	status_t res;
	timer_stuff_t **p;
	bigtime_t expires;
	bigtime_t now;
	bigtime_t wait;
	sem_id	  doneSem;
	bool	  cleanup = false;

	expires = (stuff->timer->expires * 1000000LL) / 100LL;
	now = system_time();
	if (expires > now) {
		wait = expires - now;
	} else {
		wait = 0;
	}
	//kprintf("Timer thread waiting for %d microseconds\n", wait);
	if (wait) {
		res = acquire_sem_etc(stuff->sem, 1, B_TIMEOUT, wait);
	} else {
		res = B_TIMED_OUT;
	}
	if (res == B_TIMED_OUT)
		stuff->timer->function(stuff->timer->data);

	//kprintf("Timer thread done\n");

	cleanup = TimerLock() == B_NO_ERROR;

	doneSem = stuff->done;
	delete_sem(stuff->sem);

	if (cleanup) {
		for (p = &timers; p && *p; p = &(*p)->next) {
			if (*p == stuff) {
				dev_free((unsigned char *)stuff, sizeof(*stuff));
				*p = (*p)->next;
				break;
			}
		}
	}

	TimerUnlock();
	//kprintf("Timer thread exiting\n");

	release_sem_etc(doneSem, 1, B_DO_NOT_RESCHEDULE);
	delete_sem(doneSem);

	return (0);
}


/*
 * This can be called with interrupts disabled
 * XXX: it is wrong to be using a semaphore for locking
 */
void linux_add_timer(
			   struct timer_list *timer
			   )
{
	timer_stuff_t *ntimer;

	if (TimerLock() != B_NO_ERROR)
		return;	/* XXX what to do here? */

	ntimer = (void *)dev_alloc(sizeof(*ntimer));
	ntimer->timer = timer;
	ntimer->sem = create_sem(0, "timer sem");
	ntimer->done = create_sem(0, "timer done sem");
	ntimer->thread = spawn_kernel_thread(timer_thread, "timer", B_NORMAL_PRIORITY, 
					 		  			 (void *)ntimer);
	ntimer->next = timers;
	timers = ntimer;

	TimerUnlock();

	//kprintf("Adding timer\n");
	resume_thread(ntimer->thread);
}

int linux_del_timer(struct timer_list *timer)
{
	timer_stuff_t *stuff;

	//kprintf("Deleting timer\n");
	TimerLock();	/* don't care about error case */
	for (stuff = timers; stuff; stuff = stuff->next) {
		if (stuff->timer == timer) {
			release_sem(stuff->sem);
			TimerUnlock();
			return (0); /* ??? */
		}
	}
	TimerUnlock();
	return (-1); /* ??? */
}


int barrier() {
}

void outsl(int port, void *arg, int count)
{
	long *larg = (long *)arg;

	while (count-- > 0) {
		(*pci->write_io_32)(port, *larg++);
	}
}

void insl(int port, void *arg, int count)
{
	long *larg = (long *)arg;

	while (count-- > 0) {
		*larg++ = (*pci->read_io_32)(port);
	}
}


long long fjiffies(void)
{
	return (100LL * system_time() / 1000000LL);
}


void *kmalloc(unsigned int size, int type)
{
	return (malloc(size));
}

unsigned short  eth_type_trans(struct sk_buff *skb, struct device *dev)
{
	/* figure out the protocol field and return it */
	return (0);
}


static void
alock_acquire(cpu_status *psp, volatile long *lockp)
{
	cpu_status ps;
	
	ps = disable_interrupts();
	acquire_spinlock(lockp);
	*psp = ps; /* only after we have the lock */
}

static void
alock_release(cpu_status *psp, volatile long *lockp)
{
	cpu_status ps = *psp;
	
	release_spinlock(lockp);
	restore_interrupts(ps); /* use copy since psp could be changed */
}

static void
alock_init(cpu_status *psp, volatile long *lockp)
{
	*psp = 0;
	*lockp = 0;
}

#if DEBUG
static void
print_bitmap(const char *a)
{
	int i;
	int count = 0;

	for (i = 0; i < sizeof(ether_bitmap); i++) {
		if (ether_bitmap[CHUNKNO(i)] & CHUNKBIT(i)) {
			count++;
		}
	}
	//kprintf("%s: %d\n", a, count);
}
#endif

static unsigned char *dev_alloc(size_t size)
{
	int i;
	int j;
	int nbufs;
	int bitno;
	int good;
	cpu_status ps;

	ALOCK_ACQUIRE(&devlock);
	size = (size + ETHER_BUF_SIZE - 1) & ~(ETHER_BUF_SIZE - 1);
	nbufs = size / ETHER_BUF_SIZE;
	for (i = 0; i < (N_ETHER_BUFS - (nbufs - 1)); i++) {
		good = 1;
		for (j = 0; j < nbufs; j++) {
			bitno = i + j;
			if (ether_bitmap[CHUNKNO(bitno)] & CHUNKBIT(bitno)) {
				good = 0;
				break;
			}
		}
		if (good) {
			for (j = 0; j < nbufs; j++) {
				bitno = i + j;
				ether_bitmap[CHUNKNO(bitno)] |= CHUNKBIT(bitno);
				ether_nbufs--;
			}
			ALOCK_RELEASE(&devlock);
			return (&ether_bufs[i * ETHER_BUF_SIZE]);
		}
	}
	ALOCK_RELEASE(&devlock);
	return (NULL);
}

static void
dev_free(unsigned char *buf, size_t size)
{
	int nbufs;
	int i;
	int bitno;
	int bufno;
	cpu_status ps;

	ALOCK_ACQUIRE(&devlock);
	nbufs = (size + ETHER_BUF_SIZE - 1) / ETHER_BUF_SIZE;
	bufno = (buf - ether_bufs) / ETHER_BUF_SIZE;
	for (bitno = bufno; bitno < (bufno + nbufs); bitno++) {
		ether_bitmap[CHUNKNO(bitno)] &= ~CHUNKBIT(bitno);
		ether_nbufs++;
	}
	ALOCK_RELEASE(&devlock);
}


void skb_reserve(struct sk_buff *skb, int len)
{
	skb->data += len;
	skb->tail += len;
///	map_n_store_addr(skb->tail, len);			//XXXigor
}

static struct sk_buff *alloc_skb(unsigned int size, int ignored)
{
	struct sk_buff *sk;
	unsigned char *ptr;
	int len = size;

	size = (size + 15) & ~15;
	ptr = dev_alloc(size + sizeof(struct sk_buff));
	if (ptr == NULL) {
		return (NULL);
	}
	sk = (struct sk_buff *)(ptr + size);
	sk->next = NULL;
	sk->truesize = size + sizeof(struct sk_buff);
	sk->head = ptr;
	sk->data = ptr;
	sk->tail = ptr;
	sk->end = ptr + len;
	sk->len = 0;
///	map_n_store_addr(ptr, size);			// XXXigor

	return (sk);
}


struct sk_buff *dev_alloc_skb(unsigned int length)
{
	struct sk_buff *skb;

	skb = alloc_skb(length + 16, GFP_ATOMIC);
	if (skb) {
		skb_reserve(skb, 16);
	}

	return (skb);
}

/* free skbuff */
void dev_kfree_skb(struct sk_buff *sk, int mode)
{
	if (sk) {
		dev_free(sk->head, sk->truesize);
	}
///	release_addr((uchar *) sk->head);				//XXXigor
}


int
register_netdev(struct device *dev)
{
	struct device **devpp;

	/*
	 * Find tail of list
	 */
	for (devpp = &devices; devpp && *devpp; devpp = &(*devpp)->next) {
	}

	/*
	 * Install new device at tail
	 */
	dev->next = NULL;
	if(devpp)
		*devpp = dev;
	return (0);
}

struct device *
init_etherdev(struct device *dev, int size)
{
	if (dev == NULL) {
		dev = malloc(sizeof(*dev));
		memset(dev, 0, sizeof(*dev));
	}
	dev->mc_count = 0;
	dev->mc_list = NULL;
	if (size > 0) {
		dev->priv = malloc(size);
	}
	return (dev);
}

void
ether_setup(struct device *dev)
{
	init_etherdev(dev, 0);

	//XXXigor
	ether_bufs = get_area(AREA_SIZE, &lxw_area);
	if (!ether_bufs) {
		;//kprintf("Area alloc failed, terminate\n");
	}
	init_area(ether_bufs);
}
	

/* receive packet */
void netif_rx(struct sk_buff *sk) 
{
	struct sk_buff **skpp;
	struct sk_buff *tmp;
	struct device *dev = sk->dev;

	if (dev->sksem >= B_NO_ERROR) {
		/*
		 * If low on memory, start freeing bufs
		 */
		if (ether_nbufs < ETHER_LOW_MEM_THRESH) {
			tmp = dev->sklist;
			if (tmp) {
				dev->sklist = tmp->next;
				dev_kfree_skb(tmp, 0);
			}
		}

		/*
		 * Preserve the order when adding to this list
		 */
		for (skpp = &dev->sklist; skpp && *skpp; skpp = &(*skpp)->next) {
			/* find tail of list */
		}
		sk->next = NULL;
		if(skpp)
			*skpp = sk;
		release_sem_etc(dev->sksem, 1, B_DO_NOT_RESCHEDULE);
	} else {
		//kprintf("packet dropped\n");
		dev_kfree_skb(sk, 0);
	}
}

static int32
interrupt_wrapper(void *vdata)
{

	int32 handled;
	
	intstuff_t *stuff = (intstuff_t *)vdata;

	acquire_spinlock(&ilock.lock);
	atomic_or(&global_lock, 1);

	handled = stuff->handler(stuff->irq, stuff->dev_id, NULL);

	atomic_and(&global_lock, 0);
	release_spinlock(&ilock.lock);
	
	return (handled);
}

/* set interrupt routine */
int request_irq(unsigned int irq,
				void (*handler)(int, void *, struct pt_regs *),
				unsigned long flags,
				const char *device,
				void *dev_id
				)
{
	intstuff_t *stuff;

	stuff = malloc(sizeof(*stuff));
	stuff->irq = irq;
	stuff->handler = handler;
	stuff->dev_id = dev_id;
	Lock();
	stuff->next = ints;
	ints = stuff;
	Unlock();
	install_io_interrupt_handler(irq, interrupt_wrapper, (void *)stuff, 0);
	return (0);
}

void free_irq(unsigned int irq, void *dev_id)
{
	intstuff_t **p;
	intstuff_t *tmp;

	Lock();
	for (p = &ints; p && *p; p = &(*p)->next) {
		if ((*p)->dev_id == dev_id) {
			tmp = *p;
			remove_io_interrupt_handler(tmp->irq, interrupt_wrapper, tmp);
			*p = (*p)->next;
			free(tmp);
			break;
		}
	}
	Unlock();
}

  

void request_region(void)
{
	/* do nothing */
}

int
pcibios_find_class (
						unsigned int class_code,
						unsigned short pci_index,
  						unsigned char *pci_bus,
						unsigned char *pci_device_fn
						)
{
	pci_info p;
	int i;
//0520	int n_device = 0;
	
	for (i = 0; ; i++) {
		if ((*pci->get_nth_pci_info)(i, &p) != B_NO_ERROR) {
			break;
		}
		if ( (p.vendor_id == 0x10b7) &&
						((p.device_id == 0x5900) ||
						(p.device_id == 0x5950) ||
						(p.device_id == 0x5951) ||
						(p.device_id == 0x5952) ||
						(p.device_id == 0x9000) ||
						(p.device_id == 0x9001) ||
						(p.device_id == 0x9004) ||
						(p.device_id == 0x9005) ||
						(p.device_id == 0x9050) ||
						(p.device_id == 0x9055) ||
						(p.device_id == 0x9058) ||
						(p.device_id == 0x905A) ||
						(p.device_id == 0x9200)) ) {
//0520++
//			if (pci_index != (n_device++)) {
//				kprintf("Pci_find: %x at %x, looking for more NICs.\n",
//						n_device, i);
//				continue;
//			}
//0520--	

			*pci_bus = p.bus;
			*pci_device_fn = (p.device << 3) |  p.function;
			//kprintf("Found dev bus: %02x, dev=%x (%x), fn=%x (%x)\n",
			//		*pci_bus, p.device,	(*pci_device_fn) >> 3,
			//		p.function, (*pci_device_fn) & 0x7
			//		);

			return (0); // SXSS
		}
	}
	return (-1); // FAIL
}


int pcibios_find_device(
						unsigned short vendor_id,
						unsigned short device_id,
						unsigned short pci_index,
						unsigned char *pci_bus,
						unsigned char *pci_device_fn
						)
{
	pci_info p;
	int i;
	int n_device = 0;
	

	for (i=0; ; i++) {
		if ((*pci->get_nth_pci_info)(i, &p) != B_NO_ERROR) {
			break;
		}
		if ((p.vendor_id == vendor_id) && (p.device_id == device_id)) {
			if (pci_index != (n_device++)) {
				//kprintf("Pci_find_dev: [%x], looking for more NICs.\n",
				//		n_device);
				continue;
			}

			*pci_bus = p.bus;
			*pci_device_fn = (p.device << 3) |  p.function;
			//kprintf("Found dev bus: %02x, dev=%x (%x), fn=%x (%x)\n",
			//		*pci_bus, p.device,	(*pci_device_fn) >> 3,
			//		p.function, (*pci_device_fn) & 0x7
			//		);

			return (0); // SXSS
		}
	}
	return (-1); // FAIL
}


int pcibios_present(void) {
	return (1);
}


int pcibios_read_config_byte(
							  unsigned char bus,
							  unsigned char device_fn,
							  unsigned char where,
							  unsigned char *value
							  )
{
	*value = (*pci->read_pci_config)(bus, device_fn >> 3, device_fn & 0x7,
						(long) where, sizeof(*value));
#ifdef DBG_PCI
	//kprintf("PciRead: bus=%x, dev=%x, fn=%x, offs=%08x, Result=[%0x]\n",
	//		bus, device_fn >> 3, device_fn & 0x7, (long) where, *value);
#endif
	return (0);
}

int pcibios_read_config_word(
							  unsigned char bus,
							  unsigned char device_fn,
							  unsigned char where,
							  unsigned short *value
							  )
{
	*value = (*pci->read_pci_config)(bus, device_fn >> 3, device_fn & 0x7,
					(long) where, sizeof(*value));
	return (0);
}

int pcibios_read_config_dword(
							  unsigned char bus,
							  unsigned char device_fn,
							  unsigned char where,
							  unsigned int *value
							  )
{
	*value = (*pci->read_pci_config)(bus, device_fn >> 3, device_fn & 0x7,
					(long) where, sizeof(*value));
	return (0);
}

int pcibios_write_config_dword(
							   unsigned char bus,
							   unsigned char device_fn,
							   unsigned char where,
							   unsigned long value
							   )
{
	(*pci->write_pci_config)(bus, device_fn >> 3, device_fn & 0x7,
					(long) where, sizeof(value), value);
}

int pcibios_write_config_word(
							   unsigned char bus,
							   unsigned char device_fn,
							   unsigned char where,
							   unsigned short value
							   )
{
	(*pci->write_pci_config)(bus, device_fn >> 3, device_fn & 0x7,
					(long) where, sizeof(value), value);
}

int pcibios_write_config_byte(
							   unsigned char bus,
							   unsigned char device_fn,
							   unsigned char where,
							   unsigned char value
							   )
{
	(*pci->write_pci_config)(bus, device_fn >> 3, device_fn & 0x7,
					(long) where, sizeof(value), value);
}


void __printk(const char *format, ...)
{
	va_list args;
	char buf[256];

	va_start(args, format);
	vsprintf(buf, format, args);
	dprintf(buf);
	va_end(args);
}

void __panic(const char *format, ...)
{
	va_list args;
	char buf[256];

	va_start(args, format);
	vsprintf(buf, format, args);
	dprintf(buf);
	va_end(args);
	dprintf("Spinning forever\n");
	for (;;);
	
}


int set_bit(int bit, void *addr)
{
	long *a = (long *)addr;
	long res;

	res = atomic_or(a, (1 << bit));
	return (((res >> bit) & 1) ? -1 : 0);
}


void mark_bh(int nr)
{
	set_bit(nr, &bh_active);
}

unsigned char *skb_put(struct sk_buff *skb, int len)
{
	unsigned char *tmp=skb->tail;
	skb->tail+=len;
	skb->len+=len;
	if(skb->tail>skb->end)
	{
		panic("skput:over: %d", len);
	}
	return tmp;
}


void
cli(void)
{
}

int
xxxxsti(void)
{
}

void
_save_flags(unsigned long *flags)
{
	*flags = disable_interrupts();

	ALOCK_ACQUIRE(&ilock);
}

void
restore_flags(unsigned long flags)
{
	ALOCK_RELEASE(&ilock);

	restore_interrupts(flags);
}

void
_linux_outw(unsigned short data, unsigned short port)
{
	(*pci->write_io_16)(port, data);
}

void
_linux_outl(unsigned long data, unsigned short port)
{
	(*pci->write_io_32)(port, data);
}

void
_linux_outb(unsigned char data, unsigned short port)
{
	(*pci->write_io_8)(port, data);
}


unsigned char _linux_inb(unsigned short port)
{
	unsigned char res;

	res = (*pci->read_io_8)(port);
	return (res);
}

unsigned short _linux_inw(unsigned short port)
{
	unsigned short res;

	res = (*pci->read_io_16)(port);
	return (res);
}

unsigned long _linux_inl(unsigned short port) 
{
	unsigned long res;

	res = (*pci->read_io_32)(port);
	return (res);
}

void
linux_ether_init(void)
{
	locksem = create_sem(1, "le_lock");
	timerlocksem = create_sem(1, "le_timer_lock");	
}

void
linux_ether_uninit(void)
{
	delete_sem(locksem);
	delete_sem(timerlocksem);
}

void *
linux_ether_probe(int *ncards, int (*init_module)(void))
{
	struct device *dev = NULL;
	int n;

	Lock();
	devices = NULL;
	if (init_module() == 0) {
		dev = devices;
		n = 0;
		while (devices) {
			n++;
			devices = devices->next;
		}
		*ncards = n;
	}
	Unlock();
	if (dev) {
		dev->sklist = NULL;
		dev->sksem = B_ERROR;
		alock_init(&dev->ps, &dev->lock);
		return ((void *)dev);
	}
	return (NULL);
}

int 
linux_ether_open(void *cookie, int cardno, void **session_cookie)
{
	struct device *dev = (struct device *)cookie;
	int n;
	int res;

	for (n = 0; n < cardno; n++) {
		if (dev == NULL) {
			return (EPERM);
		}
		dev = dev->next;
	}
	res = dev->init(dev);
	if (res != 0) {
		return (-ABS(res));
	}
	if (dev->open == NULL) {
		return (ENODEV);
	}
	res = dev->open(dev);
	if (res != 0) {
		return (-ABS(res));
	}
	dev->sksem = create_sem(0, "sk");
	*session_cookie = dev;
	return (0);
}

int
linux_ether_getaddr(void *cookie, char *buf)
{
	struct device *dev = (struct device *)cookie;

	memcpy(buf, dev->dev_addr, 6);
	return (B_NO_ERROR);
}

int
linux_ether_getframesize(void *cookie, char *buf)
{
	struct device *dev = (struct device *)cookie;
	const int hardcoded = 1514;

	memcpy(buf, &hardcoded, sizeof(hardcoded));
	return (B_NO_ERROR);
}

#define addreq(a, b) (memcmp(a, b, 6) == 0)

int
linux_ether_addmulti(void *cookie, char *buf)
{
	struct device *dev = (struct device *)cookie;
	struct dev_mc_list **lpp;
	struct dev_mc_list *tmp;

	for (lpp = &dev->mc_list; lpp && *lpp; lpp = &(*lpp)->next) {
		if (addreq((*lpp)->dmi_addr, buf)) {
			return (B_ERROR);
		}
	}
	tmp = malloc(sizeof(*tmp));
	memcpy(tmp->dmi_addr, buf, 6);
	if(lpp)
		*lpp = tmp;
	tmp->next = NULL;
	dev->mc_count++;
	dev->set_multicast_list(dev);
	return (B_NO_ERROR);
}

int
linux_ether_remmulti(void *cookie, char *buf)
{
	struct device *dev = (struct device *)cookie;
	struct dev_mc_list **lpp;
	struct dev_mc_list *tmp;

	for (lpp = &dev->mc_list; lpp && *lpp; lpp = &(*lpp)->next) {
		if (addreq((*lpp)->dmi_addr, buf)) {
			tmp = *lpp;
			*lpp = (*lpp)->next;
			free(tmp);
			dev->mc_count--;
			dev->set_multicast_list(dev);
			return (B_NO_ERROR);
		}
	}
	return (B_ERROR);
}

int
linux_ether_setpromisc(void *cookie, int promisc)
{
	struct device *dev = (struct device *)cookie;
	
	if (promisc) {
		dev->flags |= IFF_PROMISC;
	} else {
		dev->flags &= ~IFF_PROMISC;
	}
	dev->set_multicast_list(dev);
	return (B_NO_ERROR);
}

int
linux_ether_close(void *cookie)
{
	struct device *dev = (struct device *)cookie;
	timer_stuff_t	*stuff;
	int res;

	res = dev->stop(dev);
	if (res != 0) {
		return (-ABS(res));
	}
	delete_sem(dev->sksem);
	dev->sksem = B_ERROR;

	TimerBreakLock();

	for (stuff = timers; stuff; stuff = stuff->next) {
		status_t err = B_ERROR;

		//kprintf("le acquire stuff->done sem\n");

		while ((err = acquire_sem(stuff->done)) == B_INTERRUPTED)
			/* nothing */;
		
		if (err == B_NO_ERROR) {
			thread_info tInfo;
			while (get_thread_info(stuff->thread, &tInfo) == B_NO_ERROR)
				snooze(100000);	
		}
	}

	delete_area(lxw_area);	//0520
	
	return (B_NO_ERROR);
}

int
linux_ether_write(void *cookie, char *buf, size_t size)
{
	struct sk_buff *sk;
	struct device *dev = (struct device *)cookie;
	size_t actsize;
	int i;
	
	actsize = max(size, 60);
	sk = dev_alloc_skb(actsize);
	if (sk == NULL) {
		//kprintf("Couldn't allocate skb: %d\n", actsize);
		return (ENOBUFS);
	}
	memcpy(skb_put(sk, size), buf, size);
	if (actsize > size) {
		memset(sk->data + size, 0, actsize - size);
	}
	for (i = 0; i < TX_RETRIES; i++) {
		if (dev->hard_start_xmit(sk, dev) == 0) {
			return (size);
		}
		snooze(1000);
	}
	//kprintf("ether xmit failed\n");
	dev_kfree_skb(sk, 0);
	return (B_ERROR);
}

int
linux_ether_read(void *cookie, char *buf, size_t size, int nonblocking)
{
	status_t res;
	sem_id sem;
	struct sk_buff *sk;
	struct device *dev = (struct device *)cookie;

	ALOCK_ACQUIRE(dev);
	if (nonblocking && dev->sklist == NULL) {
		ALOCK_RELEASE(dev);
		return (0);
	}
	ALOCK_RELEASE(dev);

	do {
		res = acquire_sem(dev->sksem);
		if (res < B_NO_ERROR) {
			//kprintf("acquire sem failed\n");
			return (res);
		}

		ALOCK_ACQUIRE(dev);
		sk = dev->sklist;
		if (sk) {
			dev->sklist = dev->sklist->next;
		}
		ALOCK_RELEASE(dev);

		if (sk == NULL && nonblocking) {
			//kprintf("This shouldn't happen\n");
			return (B_ERROR);
		}
	} while (sk == NULL);
	size = min(size, sk->len);
	memcpy(buf, sk->data, size);
	dev_kfree_skb(sk, 0);
	return (size);
}


/* XXXdbg */
//extern
__inline__ int clear_bit(int nr, int * addr)
{
    int mask, retval;

    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    retval = atomic_and((long *)addr, ~mask);
    return (retval & mask) != 0;

#if 0
    cli();
    retval = (mask & *addr) != 0;
    *addr &= ~mask;
    sti();
    return retval;
#endif
}

/* XXXdbg */


void
linux_ether_dumpstats(void *cookie)
{
	struct device *dev = (struct device *)cookie;
	struct enet_statistics *stats;

	stats = dev->get_stats(dev);
#define DOIT(x) kprintf("%s: %d\n", #x, stats->x);
	DOIT(rx_packets);
	DOIT(tx_packets);
	DOIT(rx_errors);
	DOIT(tx_errors);
	DOIT(rx_dropped);
	DOIT(tx_dropped);
	DOIT(multicast);
	DOIT(collisions);
	DOIT(rx_length_errors);
	DOIT(rx_over_errors);
	DOIT(rx_crc_errors);
	DOIT(rx_frame_errors);
	DOIT(rx_fifo_errors);
	DOIT(rx_missed_errors);
	DOIT(tx_aborted_errors);
	DOIT(tx_carrier_errors);
	DOIT(tx_fifo_errors);
	DOIT(tx_heartbeat_errors);
	DOIT(tx_window_errors);
#undef DOIT
}




/* * * * * * * * * * Phys / Virt * * * * * * * * * */
#define PVMEM_ERR 1
//#define PVMEM_ALL 1

/*
 * globals
 */
// Our favorite phys mem blocks: ph_address, size
static physical_entry 	pbtable[PHYS_ENTRIES];
// The corresponding array of vmem blocks addresses
static uchar * 			vbtable[PHYS_ENTRIES];

static uchar * phys_start_area;
static uchar * virt_start_area;


static uchar *
get_area(
			size_t size,
			area_id *areap
			)
{
	uchar * base;
	area_id id;

	size = RNDUP(size, B_PAGE_SIZE);
	id = create_area("lxw_ether",
					&base,
					B_ANY_KERNEL_ADDRESS,
					size,
					B_FULL_LOCK | B_CONTIGUOUS,
					B_READ_AREA | B_WRITE_AREA);
	if (id < B_NO_ERROR) {
		//kprintf("get_area: Can't create area, terminate\n");
		return (NULL);
	}
	if (lock_memory(base, size, B_DMA_IO | B_READ_DEVICE) != B_NO_ERROR) {
		delete_area(id);
		//kprintf("get_area: Can't lock area\n");
		return (NULL);
	}
	memset(base, NULL, size);
	*areap = id;
	return (base);
}


int
init_area(
		uchar * base
		)
{
	physical_entry 	area_phys_addr[2];		// need only the start

	// Get location & size of phys blocks into pbtable[]
	get_memory_map(base,					// vbuf to translate
					B_PAGE_SIZE -1,			// size of vbuf
					&area_phys_addr[0],		// tbl to fill out
					1);						// #of entries

	// We're relying on the fact that both virt area
	// and it's phys map are contiguous as advertised.
	// If this is untrue, use the scheme similar to rings.
	virt_start_area = base;
	phys_start_area = (uchar *) (area_phys_addr[0].address);

	return TRUE;
}
	

void *
bus_to_virt(
				ulong ph_addr
				)
{
	ulong	offset;

	offset = ph_addr - (ulong) phys_start_area;

	if ((ph_addr < (ulong) phys_start_area) || (offset > AREA_SIZE)) {
#if PVMEM_ERR
		//kprintf("b2v: Out of bounds: PA=0x%08x\n", ph_addr);
#endif
		return (virt_start_area);	// wrong addr but legal
	}

	return (virt_start_area + offset);
}


ulong
virt_to_bus(
				void * v_addr
				)
{
	ulong	offset;

	offset = (ulong) v_addr - (ulong) virt_start_area;

	if ((v_addr < virt_start_area) || (offset > AREA_SIZE)) {
#if PVMEM_ERR
		//kprintf("v2b: Out of bounds: VA=0x%08x\n", v_addr);
#endif
		return ((ulong) phys_start_area);	// wrong addr but legal
	}

	return ((ulong) phys_start_area + offset);
}



/* * * * * * * * * * Tx / Rx Rings: Phys / Virt * * * * * * * * * */

int
ring_init_addr_table(
				)
{
	int i;

	for (i = 0; i < PHYS_ENTRIES; i++) {
		pbtable[i].address = NULL;
		pbtable[i].size = NULL;
		vbtable[i] = NULL;
	}

	return TRUE;
}


/*
 * Remember PA of given VA in the table
 */
int
ring_map_n_store_addr(
				uchar * v_addr,
				int v_size
				)
{
	int i;

	// Check if it's already there?
	//for (i = 0; i < PHYS_ENTRIES; i++) {
	//	if (vbtable[i] == v_addr) {
	//		goto exit_msa;
	//	}
	//}

	for (i = 0; i < PHYS_ENTRIES; i++) {
		if (!vbtable[i]) {					// get 1st free
			vbtable[i] = v_addr;

			// Get location & size of phys blocks into pbtable[]
			get_memory_map(v_addr,			// vbuf to translate
						v_size,				// size of vbuf
						&pbtable[i],		// tbl to fill out
						1);					// #of entries

#if PVMEM_ALL
			//kprintf("Storing [%d]: VA=0x%08x, PA=0x%08x, size %d\n",
			//		i, vbtable[i], pbtable[i].address, pbtable[i].size);
#endif
			return TRUE;
		}
	}
#if PVMEM_ERR
	//kprintf("Exhausted vbtable, terminate!\n");
#endif

	return FALSE;
}


int
ring_release_addr(
				uchar * addr
				)
{
	int i;

	for (i = 0; i < PHYS_ENTRIES; i++) {
		if (vbtable[i] == addr) {		//||(pbtable[i].address == addr)
			pbtable[i].address = NULL;
			vbtable[i] = NULL;
#if PVMEM_ALL
			//kprintf("Releasing 0x%08x #[%d]\n", addr, i);
#endif
			return TRUE;
		}
	}

#if PVMEM_ERR
	//kprintf("Trying to release 0x%08x: NO SUCH\n", addr);
#endif
	return FALSE;
}


void*
ring_bus_to_virt(
				ulong ph_addr
				)
{
	int i;
	uchar * bptr = (uchar *) ph_addr;

	for (i = 0; i < PHYS_ENTRIES; i++) {
		if ((ulong) pbtable[i].address == ph_addr) {
#if PVMEM_ALL
			//kprintf("rb2v(): found PA=0x%08x at [%d]\n", ph_addr, i);
#endif
			return((void*) vbtable[i]);
		}
	}

#if PVMEM_ERR
	//kprintf("rb2v(): no such PA: 0x%08x, ret BADADDR\n", ph_addr);
#endif
	return ((void *) (BADADDRESS));	// exception
}


ulong
ring_virt_to_bus(
				void* v_addr
				)
{
	int i;

	for (i = 0; i < PHYS_ENTRIES; i++) {
		if (vbtable[i] == v_addr) {
#if PVMEM_ALL
			//kprintf("rv2b(): found VA=0x%08x at [%d]\n", v_addr, i);
#endif
			return ((ulong) pbtable[i].address);
		}
	}

#if PVMEM_ERR
	//kprintf("rv2b(): no such VA: 0x%08x, ret BADADDR\n", v_addr);
#endif
	return (BADADDRESS);
}

// eof

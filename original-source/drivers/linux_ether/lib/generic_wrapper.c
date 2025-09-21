#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <ether_driver.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <SupportDefs.h>
#include "linux.h"

typedef struct generic_private {
	void *cookie;
	int nonblocking;
	int devno;
} generic_private_t;

extern void *linux_ether_probe(int *ncards, int (*init_module)(void));
extern int init_module(void);

static void *card_cookie;
static int ncards;
static unsigned long card_mask;

char				pci_name[] = B_PCI_MODULE_NAME;
pci_module_info		*pci;


status_t
init_hardware(void)
{
	if (get_module(pci_name, (module_info **) &pci))
		return B_ERROR;
	card_cookie = linux_ether_probe(&ncards, init_module);
	put_module(pci_name);
	
	if (card_cookie) {
		dprintf("generic: hw found (%d)\n", ncards);
	} else {
		dprintf("generic: no hardware found\n");
		return (B_ERROR);
	}
	return (B_NO_ERROR);
}

status_t
init_driver()
{
	status_t err = get_module(pci_name, (module_info **) &pci);

	if (err == B_NO_ERROR)
		linux_ether_init();

	return (err);
}

void
uninit_driver()
{
	linux_ether_uninit();

	put_module(pci_name);
}

static bool
init(void)
{
	if (card_cookie == NULL) {
		init_hardware();
	}
	return (card_cookie != NULL);
}



/*
 * Entry point: read a packet
 */
status_t 
generic_read(
		   void *vdata,
		   off_t pos,
		   void *buf,
		   size_t *lenp
		   )
{
	generic_private_t *data = (generic_private_t *)vdata;
	size_t len;

	if (data->cookie == NULL) {
		return (B_ERROR);
	}
	len = linux_ether_read(data->cookie, buf, *lenp, data->nonblocking);
	if (len < B_NO_ERROR) {
		return (len);
	}
	*lenp = len;
	return (B_NO_ERROR);
}


status_t
generic_write(
		   void *vdata,
		   off_t pos,
		   const void *buf,
		   size_t *lenp
		   )
{
	generic_private_t *data = (generic_private_t *)vdata;
	size_t len;

	if (data->cookie == NULL) {
		return (B_ERROR);
	}
	len = linux_ether_write(data->cookie, buf, *lenp);
	if (len < B_NO_ERROR) {
		dprintf("error returned by write\n");
		return (len);
	}
	*lenp = len;
	return (B_NO_ERROR);
}


/*
 * open the driver
 */
status_t
generic_open(
		   const char *name,
		   uint32 flags,
		   void **vdata
           )
{
	generic_private_t *data;

	if (!init()) {
		return (ENODEV);
	}
	data = malloc(sizeof(*data));
	if (data == NULL) {
		return (B_NO_MEMORY);
	}
	data->nonblocking = 0;
	data->cookie = NULL;
	data->devno = -1;
	*vdata = data;
	return (B_NO_ERROR);
}


/*
 * close the driver
 */
status_t
generic_close(
		   void *vdata
		   )
{
	generic_private_t *data = (generic_private_t *)vdata;
	
	if (data->devno != -1) {
		card_mask &= ~(1 << data->devno);
	}
	if (data->cookie != NULL) {	
		linux_ether_close(data->cookie);
	}
	return (B_NO_ERROR);
}

status_t
generic_free(
		   void *vdata
		   )
{
	generic_private_t *data = (generic_private_t *)vdata;

	if (data) {
		free(data);
	}

	return (B_NO_ERROR);

}


/*
 * Standard driver control function
 */
status_t
generic_control(
			void *vdata,
			uint32 msg,
			void *buf,
			size_t len
			)
{
	generic_private_t *data = (generic_private_t *)vdata;
	status_t status;
	void *cookie;
	int devno;
	int promisc;

	data = (generic_private_t *) vdata;
	switch (msg) {
	case ETHER_INIT:
		if (data->cookie) {
			return (B_ERROR);
		}
		for (devno = 0; devno < ncards; devno++) {
			if ((card_mask & (1 << devno)) == 0) {
				break;
			}
		}
		if (devno == ncards) {
			dprintf("All cards busy\n");
			return (B_ERROR);
		}

		status = linux_ether_open(card_cookie, devno, &cookie);
		if (status == 0) {
			card_mask |= (1 << devno);
			data->cookie = cookie;
			data->devno = devno;
			return (B_NO_ERROR);
		}
		return (status);
	case ETHER_GETADDR:
		if (data->cookie == NULL) {
			return (B_ERROR);
		}
		return (linux_ether_getaddr(data->cookie, buf));
	case ETHER_NONBLOCK:
		if (data->cookie == NULL) {
			return (B_ERROR);
		}
		memcpy(&data->nonblocking, buf, sizeof(data->nonblocking));
		return (B_NO_ERROR);
	case ETHER_GETFRAMESIZE:
		if (data->cookie == NULL) {
			return (B_ERROR);
		}
		return (linux_ether_getframesize(data->cookie, buf));
	case ETHER_ADDMULTI:
		if (data->cookie == NULL) {
			return (B_ERROR);
		}
		return (linux_ether_addmulti(data->cookie, buf));
	case ETHER_REMMULTI:
		if (data->cookie == NULL) {
			return (B_ERROR);
		}
		return (linux_ether_remmulti(data->cookie, buf));
	case ETHER_SETPROMISC:
		if (data->cookie == NULL) {
			return (B_ERROR);
		}
		memcpy(&promisc, buf, sizeof(promisc));
		return (linux_ether_setpromisc(data->cookie, promisc));
	}
	return (B_ERROR);
}

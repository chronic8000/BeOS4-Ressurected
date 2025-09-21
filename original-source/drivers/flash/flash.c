/* ++++++++++
	FILE:  joe/flash.c
	REVS:  $Revision: 1.26 $
	NAME:  herold
	DATE:  Tue Apr 18 18:02:17 PDT 1995

	Copyright (c) 1995-97 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <OS.h>
#include <KernelExport.h>
#include <PCI.h>
#include <Drivers.h>
#include <drivers_priv.h>
#include <flash_driver.h>
#include <cpu.h>
#include <fl_amd.h>
#include <vm.h>
#include <eagle.h>
#undef ANALYZE
#include <analyser.h>

#define ddprintf 


/* -----
	private definitions
----- */

typedef struct {
	ulong	block_size;
	ulong	num_blocks;
	uchar	manufacturer_code;
	uchar	device_code;
} flash_type;

typedef struct {
	area_id	aid;		/* id of flash area */
	int		config;		/* eagle config reg contents */
} enable_state;

#define FL_ADDR(offset) ((vuchar *)(flash + (offset))))


/* ----------
	private globals
----- */

static sem_id		mapping_sem = -1;	/* mutex for mapping */
static vchar		*flash;				/* -> mapped flash */
static area_id		flash_area = -1;	/* area that maps the flash */
static pci_info		eagle;				/* pci info for MPC105 */
static flash_type types[2] = {
	{ FL_BLOCK_SIZE_amd29f010, 8, FL_MANUFACTURER_amd, FL_DEVICE_amd29f010 },
	{ FL_BLOCK_SIZE_amd29f040, 8, FL_MANUFACTURER_amd, FL_DEVICE_amd29f040 }
};


/* ----------
	inp - read a byte from flash
----- */
static vuchar
inp (int offset)
{
	uchar	val;

	val = * (vuchar *) (flash + offset);
	__eieio();
	
	return val;
}



/* ----------
	outp - write a byte to flash
----- */
static void
outp (int offset, uchar val)
{
	* (vuchar *) (flash + offset) = val;
	__eieio();
}



/* ----------
	find_eagle - looks for the Eagle chip
----- */
static bool
find_eagle (void)
{
	int	i;

	for (i = 0; ; i++) {
		if (get_nth_pci_info (i, &eagle) != B_NO_ERROR)
			break;

		if (eagle.class_base == PCI_bridge &&
		  eagle.class_sub == PCI_host &&
		  eagle.vendor_id == 0x1057 &&	/* motorola */
		  eagle.device_id == 0x0001)	/* MPC105 */
			return TRUE;
	}
	return FALSE;
}


/* ----------
	init_hardware - check for Eagle chip, fail if not there
----- */
status_t
init_hardware (void)
{
	return find_eagle() ? B_NO_ERROR : B_ERROR;
}


/* ----------
	init_driver - check for Eagle chip, fail if not there
----- */
status_t
init_driver (void)
{
	if (!find_eagle())
		return B_ERROR;
	
	mapping_sem = create_sem (1, "flash driver open");
	return (mapping_sem < 0) ? B_ERROR : B_NO_ERROR;
}


/* ----------
	uninit_driver - clean up resources used by driver
----- */
void
uninit_driver (void)
{
	#if !BOOT
	if (flash_area >= 0)
		delete_area (flash_area);

	mapping_sem = -1;
	#endif
}
	

/* ----------
	map_flash - make flash rom accessible by mapping it into virtual space
----- */
static bool
map_flash (bool writing)
{
	#if BOOT
	flash = (vchar *) HOST_FLASH;
	return TRUE;

	#else
	acquire_sem (mapping_sem);

	if (flash_area < 0)
		flash_area = map_physical_memory (
			"flash",
			(void *) HOST_FLASH,
			512*1024,
			B_ANY_KERNEL_BLOCK_ADDRESS,
			writing ? B_WRITE_AREA + B_READ_AREA : B_READ_AREA,
			(void**)&flash
		);

	release_sem (mapping_sem);

	return (flash_area < 0) ? FALSE : TRUE;
	#endif
}


/* ----------
	enable_flash_writes
----- */
static int
enable_flash_writes (enable_state *s)
{
	/* make the flash writeable */

	#ifdef FLASH_UPDATER
	disable_dcache();
	#endif

	/* set up Eagle to enable flash updates */

	s->config = read_pci_config (eagle.bus, eagle.device, eagle.function, EGL_config_1, 4);
	write_pci_config (eagle.bus, eagle.device, eagle.function, EGL_config_1, 4, s->config | 0x1000);

	return B_NO_ERROR;
	
}


/* ----------
	disable_flash_writes
----- */
static void
disable_flash_writes (enable_state *s)
{
	/* restore the flash to read only */

	write_pci_config (eagle.bus, eagle.device, eagle.function, EGL_config_1, 4, s->config);
	#ifdef FLASH_UPDATER
	inval_lock_enable_dcache();
	#endif
}


/* ----------
	write_cmd writes the passed command, along with the common
	command prefix.
----- */
static void
write_cmd (char cmd)
{
	outp (FLCMD_PREFIX1_ADDR_amd, FLCMD_PREFIX1_DATA_amd);
	outp (FLCMD_PREFIX2_ADDR_amd, FLCMD_PREFIX2_DATA_amd);
	outp (FLCMD_PREFIX1_ADDR_amd, cmd);
}


/* ----------
	reset_amd returns the roms to read mode.
----- */

static void
reset_amd (void)
{
	write_cmd (FLCMD_READ_amd);
}


/* ----------
	is_sector_protected returns true if sector is write-protected.
----- */
static status_t
is_sector_protected (flash_type *f, long sector)
{
	enable_state	s;
	int				i;
	uchar 			prot;
	int				err;

	if (sector < 0 || sector > 8)
		return B_ERROR;

	err = enable_flash_writes (&s);
	if (err != B_NO_ERROR)
		return B_ERROR;

	write_cmd (FLCMD_AUTOSELECT_amd);

	prot = inp ((sector * f->block_size) + 2);	/* addr 2: protection */

	reset_amd();
	disable_flash_writes (&s);

	return prot & 0x01;
}


/* ----------
	get_flash_type figures out what kind of flash is installed.
----- */
static flash_type *
get_flash_type (void)
{
	enable_state	s;
	int				i;
	uchar 			mfg, dev;
	int				err;

	/* ---
		rwh 4/1/97
		There is some sort of cache coherency problem here.  If the 
		dprintf lines w/ "fix#1" are removed, this routines fails on a
		133 603e.  This should be investigated ASAP.  In the meantime,
		the lines stay.
	--- */

	err = enable_flash_writes (&s);
	dprintf ("get_flash_type: fix #1\n");
	if (err != B_NO_ERROR)
		return NULL;

	write_cmd (FLCMD_AUTOSELECT_amd);

	mfg = inp (0);	/* addr 0: manufacturer code */
	dev = inp (1);	/* addr 1: device code */

	reset_amd();
	dprintf ("get_flash_type: fix #1\n");
	disable_flash_writes (&s);

	/* search supported devices to get device type */

	for (i = 0; i < sizeof (types) / sizeof (types[0]); i++)
		if ((types[i].manufacturer_code) == mfg && (types[i].device_code == dev))
			return types + i;

	return NULL;
}


/* -----
   	poll_status polls the device until the operation is complete.

	The 'data polling' and 'operation timeout' bits are checked
	until the data bits equals the passed value (operation complete)
	or the operation times out.
----- */

static int
poll_status (ulong addr, uchar value)
{
	uchar	status;

	for (;;) {
		status = inp (addr);
		if ((status & FLST_DATA_amd) == (value & FLST_DATA_amd))
			break;
		if (status & FLST_TIMEOUT_amd)
			return B_ERROR;
	}
	return B_NO_ERROR;
}


/* ----------
	erase_amd erases the flash blocks intersecting with the passed
	offset/size.
	
	It is assumed that the offset is flash-block aligned, and that the
	size is a multiple of the flash block size
----- */

static int
erase_amd (ulong addr, ulong size, ulong block_size)
{
	int			err;
	cpu_status	ps;
	ulong		save_addr;
	ulong		save_size;
	int			retry;
	const int	max_retry = 50;
	
	ddprintf ("erase_amd: addr=%.8x size=%.8x block_size=%.8x\n", addr, size, block_size);

	save_addr = addr;
	save_size = size;

	for (retry = 0; retry < max_retry; retry++) {

		addr = save_addr;
		size = save_size;

		ps = disable_interrupts ();

		write_cmd (FLCMD_ERASESETUP_amd);
		outp (FLCMD_PREFIX1_ADDR_amd, FLCMD_PREFIX1_DATA_amd);
		outp (FLCMD_PREFIX2_ADDR_amd, FLCMD_PREFIX2_DATA_amd);

		/* write erase cmd for each sector.  These must all happen within
		   100 uSec of each other, so we disable interrupts while doing it */

		while (size > 0) {
			outp (addr, FLCMD_ERASECONFIRM_amd);
			addr += block_size;
			size -= block_size;
		}

		restore_interrupts (ps);

		/* wait till erase complete */

		if ((err = poll_status (save_addr, 0x80)) == B_NO_ERROR)
			break;
	}

	if (err != B_NO_ERROR)
		dprintf("erase_amd: erase failed\n");

	reset_amd ();			/* back to read mode */
	return err;
}


/* ----------
	write_amd writes stuff to the flash rom.
----- */

static int
write_amd (uchar *data, ulong addr, ulong size)
{
	const int	max_retry = 50;
	int			retry;
	int			err;

	ddprintf ("write_amd: data=%.8x addr=%.8x size=%.8x\n", data, addr, size);

	while (size) {

		for (retry = 0; retry < max_retry; retry++) {
			write_cmd (FLCMD_PROGRAMSETUP_amd);			/* setup... */
			outp (addr, *data);							/* program! */
			err = poll_status (addr, *data);			/* check */
			if (err == B_NO_ERROR)
				break;
		}

		if (err != B_NO_ERROR) {
			dprintf ("fl_amd, write_amd: error writing flash at offset %.8x\n", addr);
			break;
		}
		data++;
		addr++;
		size--;
	}
	reset_amd ();			/* back to read mode */
	return err;
}


/* ----------
	flash_open
----- */

static status_t
flash_open (const char *name, uint32 flags, void **cookie)
{
	bool writing;

	writing = (flags & (O_RDWR | O_WRONLY)) ? TRUE : FALSE;

#if 0
	/* O_EXCLOPEN is a no-op.  */
	/* if writing, better be exclusive */
	if (writing && !(flags & O_EXCLOPEN))
		return B_PERMISSION_DENIED;
#endif

	if (!find_eagle())
		return B_ERROR;

	if (!map_flash(writing))
		return B_ERROR;

	if ((*cookie = get_flash_type()) == NULL) {
		dprintf("flash_open: could not determine device type!\n");
		return B_ERROR;
	}

	return B_NO_ERROR;
}


/* ----------
	flash_close
----- */

static status_t
flash_close (void *cookie)
{
	return B_NO_ERROR;
}


/* ----------
	flash_free
----- */

static status_t
flash_free (void *cookie)
{
	#if !BOOT
	if (flash_area)
		delete_area (flash_area);
	#endif

	return B_NO_ERROR;
}

/* ----------
	flash_control
----- */

static status_t
flash_control(void *_f, uint32 msg, void *buf, size_t len)
{
	flash_type *f = (flash_type *) _f;

	switch (msg) {
	case B_GET_DEVICE_SIZE:
		*(size_t *)buf = f->block_size * f->num_blocks;
		return B_NO_ERROR;
	case B_IS_FLASH_SECTOR_PROTECTED:
		*(long *)buf = is_sector_protected (f, * (long *) buf);
		return B_NO_ERROR;
	case B_GET_FLASH_SECTOR_SIZE:
		*(long *)buf = f->block_size;
		return B_NO_ERROR;
	}

	return B_ERROR;
}


/* ----------
	flash_read
----- */

static status_t
flash_read (void *_f, off_t pos, void *buf, size_t *len)
{
	flash_type *f = (flash_type *) _f;
	size_t	count;

	count = *len;
	if ((count + pos) > (f->block_size * f->num_blocks)) {
		dprintf ("flash_write: attempt to write beyond end\n");
		return B_ERROR;
	}

	memcpy (buf, flash + pos, count);
	return 0;
}


/* ----------
	flash_write
----- */

static status_t
flash_write (void *_f, off_t pos, const void *buf, size_t *len)
{
	flash_type *f = (flash_type *) _f;

	size_t			count;
	enable_state	s;
	int				err;
	ulong			blksize;
	uchar			*mbuf = NULL;
	uchar 			*pr_buf;		/* -> data to be programmed this time */
	ulong			src_count;		/* # source bytes being programmed */
	ulong			pr_dev_count;	/* # device bytes being programmed */
	ulong			pr_pos;			/* device positon to start erase/pgm */

	count = *len;
	ddprintf ("flash_write: buf=%.8x count=%.8x pos=%.8x\n", buf, count, pos);

	if ((count + pos) > (f->block_size * f->num_blocks)) {
		dprintf ("flash_write: attempt to write beyond end\n");
		return B_ERROR;
	}

	/* get a buffer for non-block-aligned writes */
	
	blksize = f->block_size;
	if ((pos % blksize) || (count % blksize)) 
		if (!(mbuf = (uchar *) malloc (blksize)))
			return B_ERROR;
		
	err = enable_flash_writes (&s);
	if (err != B_NO_ERROR)
		goto exit;

	while (count > 0) {
		/* if not starting and ending on block boundary, we must merge
		   with what is already there */

		if ((pos % blksize) || (count < blksize)) {
			pr_pos = (pos/blksize) * blksize;
			memcpy (mbuf, flash + pr_pos, blksize);
			pr_dev_count = blksize;
			src_count = blksize - (pos % blksize);
			if (count < src_count)
				src_count = count;
			memcpy (mbuf + (pos % blksize), buf, src_count);
			pr_buf = mbuf;
		} else {
			pr_pos = pos;
			pr_dev_count = src_count = (count / blksize) * blksize;
			pr_buf = (uchar *) buf;
		}

		if ((err = erase_amd(pr_pos, pr_dev_count, blksize)) == B_NO_ERROR)
			err = write_amd (pr_buf, pr_pos, pr_dev_count);
		
		if (err != B_NO_ERROR)
			break;
		pos += src_count;
		buf = (char *) buf + src_count;
		count -= src_count;
	}
	
	disable_flash_writes (&s);

exit:
	if (mbuf)
		free (mbuf);
	return err;
}


/* ----------
	driver-related structures
----- */

static device_hooks flash_device = {
	flash_open,		/* -> open entry point */
	flash_close,	/* -> close entry point */
	flash_free,		/* -> free cookie entry point */
	flash_control,	/* -> control entry point */
	flash_read,		/* -> read entry point */
	flash_write		/* -> write entry point */
};

static const char	*flash_name[] = { "flash", NULL };

const char **
publish_devices()
{
	return flash_name;
}

device_hooks *
find_device(const char *name)
{
	return &flash_device;
}

fixed_driver_info	flash_driver_info = {
	"flash",
	B_CUR_DRIVER_API_VERSION,
	NULL,
	&publish_devices,
	&find_device,
	NULL,
	NULL
};

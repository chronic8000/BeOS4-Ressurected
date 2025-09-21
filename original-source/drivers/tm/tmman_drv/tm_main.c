/*
*/


#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#define __NO_VERSION__ /* don't define kernel_version in module.h */
#include <linux/module.h>
#include <linux/version.h>

char kernel_version [] = UTS_RELEASE;

#include <linux/kernel.h> /* printk() */
#include <linux/malloc.h> /* kmalloc() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>  /* O_ACCMODE */

#include <linux/config.h> /* pci support */

#include <linux/pci.h>    /* pci support */


#include <asm/system.h>   /* cli(), *_flags */
#include <asm/segment.h>  /* memcpy and such */
#include <asm/io.h>       /* virt_to_xxx() read[bwl]/write[bwl] */
#include <asm/pgtable.h>  /*PAGE_USERIO*/


#include <linux/stat.h>
#include <linux/unistd.h>

#include <linux/mman.h>

#include "tmif.h"           //DL


// #include "trimedia.h"     /* local definitions and KENREL VERSION CHECKING */

/******
local prototypes
****************/


/*
* I don't use static symbols here, because register_symtab is called
*/

#ifndef TRIMEDIA_MAJOR
#define TRIMEDIA_MAJOR 0
#endif

#ifndef TRIMEDIA_NR_DEVS
#define TRIMEDIA_NR_DEVS    1   // DL: this was 4 
#endif

#define TM_DEVICE_NAME "tm"

int trimedia_major   =  TRIMEDIA_MAJOR;
int trimedia_nr_devs =  TRIMEDIA_NR_DEVS;  /* max number of trimedia devices */


extern GlobalObject *TMManGlobal;

/*
* Open and close
*/


int trimedia_open (struct inode *inode, struct file *filp)
{
	int     minor_nr = MINOR(inode->i_rdev);
	int     ret;
	ioparms dIoParms; 
	
	DPF(0,("tm_main.c: trimedia_open: minor_nr : %d; from flags 0x%04x\n"
        , minor_nr, filp->f_flags));
	DPF(0,("tm_main.c: trimedia_open: address of inode is 0x%x\n"
        , inode));
	DPF(0,("tm_main.c: trimedia_open: filp->f_dentry->d_inode is 0x%x\n"
        ,filp->f_dentry->d_inode ));
	
	if(minor_nr == 0)
	{
		DPF(0,("tm_main.c: trimedia_open: The process is \"%s\" with pid is %d\n"
			,current->comm, current->pid));
		dIoParms.tid = (UInt32)current->pid;
		tmmOpen(&dIoParms);
	}
	
	return 0; 
}


int trimedia_release (struct inode *inode, struct file *filp)
{
	printk("Trimedia_release has been called.\n");
	return 0 ;
}


ssize_t trimedia_read (struct file *filp,
					   char *buf, size_t count, loff_t *ppos)
{
	printk("Trimedia_read has been called.\n");
	return -EBUSY;
}


ssize_t trimedia_write (struct file *filp,
						const char *buf, size_t count, loff_t *ppos)
{
    printk("Trimedia_write has been called.\n");
    return -ENOSPC;
}

/*
* The ioctl() implementation
*/

int trimedia_ioctl (struct inode *inode, struct file *filp,
					unsigned int cmd, unsigned long arg)
{
	tmifSharedMemoryCreate  *pTMIF;
	tTMMANIo                *pIoParmBlock;
	unsigned long           prot   = PROT_READ | PROT_WRITE;
	unsigned long           flags  = MAP_SHARED;
	unsigned long           off;
	UInt32                  kernel_space_SharedMemAddress = 0;
	UInt32                  user_space_SharedMemAddress   = 0;
	UInt32                  kernel_space_BaseSharedMemAddress;
	UInt32                  user_space_BaseSharedMemAddress;
	UInt32                  physical_BaseSharedMemAddress;
	UInt32                  addr;
	UInt32                  difference;
	static int              shared_mem_reserved = 0;
	TMManDeviceObject      *pTMManDeviceObject;
	ClientDeviceObject     *pClientDeviceObject;
	UInt32                  i;
	
	DPF(0,("Trimedia_ioctl has been called.\n"));
	tmmCntrl((ioparms *)arg);
	
	/* In case of a "shared memory creation" we still have to do some extra work:
	the obtained address of the shared memory has to be mapped to user space.
	The mapping is performed by mapping all of the "shared Data" (see tmmInit) 
	between host and trimedia to the user address space of the process. 
	The address of the shared memory memory just created 
	is calculated by the difference with the basis of this shared data.
	We do the mapping here, (and not in the file tmif.c) because the 
	function tmmCntrl does not know the file struct pointer
	We use the MemoryAddrUser field of the struct ClientDeviceObject to store
	the basis fo the "shared data" for each process.  The function tmmOpen sets
	this field to the kernel address of the basis of the shared data. Here this field
	will get the user address of this base.  Note that this field is different 
	for each process 
	*/ 
	pIoParmBlock = (tTMMANIo *) ( ((ioparms *)arg)->in_iopb );
	pTMIF=(tmifSharedMemoryCreate *)pIoParmBlock->pData;
	
	if (
		pIoParmBlock->controlCode == constIOCTLtmmanSharedMemoryCreate
		)
	{ 
		DPF(0,("tm_main.c: tm_ioctl: case tmifSharedMemoryCreate.\n"));
		
		// Finding the ClientDeviceObject struct of the process
		for(i=0; i < constTMMANMaximumClientCount;i++)
		{
			DPF(0,("tm_main.c: tm_ioctl: i= 0x%x\n",i));
			if(TMManGlobal->ClientList[i] == Null)
			{
				continue;
			}
			if(  ((ClientObject*)(TMManGlobal->ClientList[i]))->Process
				!= 
				(current->pid)
				)
			{
				continue;
			}
			else
			{
				// This case should not happen
				pClientDeviceObject =
					(ClientDeviceObject *)( ((ClientObject*)(TMManGlobal->ClientList[i]))->Device);
				break;
			}
		}
		if(i == constTMMANMaximumClientCount )
		{
			DPF(0,("tm_main.c: tm_ioctl: no ClientObject found for this process.\n"));
			return -1;
		}
		// Finding the physical and kernel address of the basis of the "Shared Data".
		pTMManDeviceObject = (TMManDeviceObject *)(TMManGlobal->DeviceList[0]);
		kernel_space_BaseSharedMemAddress = (UInt32)(pTMManDeviceObject->SharedData);
		/* DL: TMManSharedAddress is the bus address of the shared data and ioremap needs the physical addresses  
		physical_BaseSharedMemAddress     = (UInt32)(pTMManDeviceObject->TMManSharedAddress);
		*/
		
		physical_BaseSharedMemAddress = virt_to_phys(pTMManDeviceObject->TMManSharedAddress);     
		
		DPF(0,("tm_main.c: tm_ioctl: kernel_space_BaseSharedMemAddress is 0x%x\n"
			, kernel_space_BaseSharedMemAddress));
		DPF(0,("tm_main.c: tm_ioctl: physical_BaseSharedMemAddress is 0x%x\n"
			, physical_BaseSharedMemAddress));
		
		if (shared_mem_reserved == 0)
		{ 
			// Reserving the pages:
			for(  addr = KSEG0ADDR(kernel_space_BaseSharedMemAddress)
				;  addr < (KSEG0ADDR(kernel_space_BaseSharedMemAddress) + TMManGlobal->PageLockedMemorySize )
				;  addr += PAGE_SIZE )
			{
				mem_map[MAP_NR(addr)].flags |= 0x1 << PG_reserved;
			}
			shared_mem_reserved = 1;
		}
		
		// if another process creates shared memory, it must also map
		// this memory in its address space
		// Checking whether MemoryAddrUser is still a kernel address meaning that the 
		// process has not yet mapped the shared memory in its address space.
		// (on the PC platform PAGE_OFFSET is generally 0xc000000)
		if ((pClientDeviceObject->MemoryAddrUser) >= PAGE_OFFSET) 
		{
        /*The offset argument of the do_mmap function is misused to pass the physical
			memory address from where memory mapping should start.*/
			off = physical_BaseSharedMemAddress; 
			user_space_BaseSharedMemAddress = do_mmap(filp
				, 0 
				, TMManGlobal->PageLockedMemorySize 
				, prot
				, flags
				, off);  
			if (user_space_BaseSharedMemAddress == 0)
			{
				DPF(0,("tm_main.c: tm_ioctl: do_mmap failed.\n"));
			}
			DPF(0,("tm_main.c: tm_ioctl: do_mmap returned 0x%x.\n"
				, user_space_BaseSharedMemAddress));
			
			pClientDeviceObject->MemoryAddrUser = user_space_BaseSharedMemAddress;
			
		}
		
		DPF(0,("tm_main.c: tm_ioctl: pTMIF->Address is 0x%x\n", pTMIF->Address));
		kernel_space_SharedMemAddress = pTMIF->Address;
		difference = kernel_space_SharedMemAddress -
			kernel_space_BaseSharedMemAddress;
		user_space_SharedMemAddress = pClientDeviceObject->MemoryAddrUser 
			+ difference; 
		
		DPF(0,("tm_main.c: tm_ioctl: user_spaceSharedMemAddress is 0x%x.\n"
			,user_space_SharedMemAddress));
		
		pTMIF->Address = user_space_SharedMemAddress;
	}
	DPF(0,("tm_main.c: tm_ioctl: tm_ioctl returned with ((ioparms *)arg)->err is 0x%x\n"
		, ((ioparms *)arg)->err));
	//return ((ioparms *)arg)->err;
	return 0;
}

/*
* The mmap() implementation
*/
int trimedia_mmap(struct file *filp,  struct vm_area_struct *vma)
{
	unsigned long off      = vma->vm_offset;  // will probably be 0 in this case
	unsigned long physical;
	unsigned long simple_region_start; 
	unsigned long simple_region_size;
	unsigned long vsize    = vma->vm_end - vma->vm_start;
	unsigned long psize;
	UInt32        HalHandle;
	void *        SDRAMPhysical; 
	void *        SDRAMKernelMapped;
	UInt32        SDRAMSize;         
	void *        MMIOPhysical; 
	void *        MMIOKernelMapped;
	UInt32        MMIOSize;         
	
	DPF(0,("tm_main.c: tm_mmap: d_inode is 0x%x\n"
        , filp->f_dentry->d_inode));
	DPF(0,("tm_main.c: tm_mmap: the corresponding rdev number is 0x%x\n"
		, filp->f_dentry->d_inode->i_rdev));
	DPF(0,("tm_main.c: tm_mmap: the corresponding MINOR(rdev) number is 0x%x\n"
		, MINOR(filp->f_dentry->d_inode->i_rdev) 
		));
	
	DPF(0,("tm_main.c: trimedia_mmap: vma->vm_start is 0x%x, vma->vm_end is 0x%x\n, vsize is 0x%x and  vma->vm_offset is 0x%x\n"
		, vma->vm_start
		, vma->vm_end
		, vsize
		, vma->vm_offset));
	
	DPF(0,("tm_main.c: trimedia_mmap: vma->vm_file is 0x%x and filp is 0x%x\n"
		, vma->vm_file
		, filp));
	
		/* 
		The idea is that this function is going to be called to map the SDRAM
		and the MMIO of trimedia and some pieces of SDRAM of the host to be used as shared
	memory. To differentiate between these cases, the size of the areas is used. */ 
	if (vsize >= 0x800000)  
	{
        // now we suppose we have to map SDRAM
        // note that the following code only works for one
        // trimedia in the system 
        // We will first determine the physical(bus) address and the size of
		// the SDRAM of the trimedia.
		
        HalHandle = ((TMManDeviceObject *)(TMManGlobal->DeviceList[0]))->HalHandle;
        halGetSDRAMInfo(HalHandle, &SDRAMPhysical, &SDRAMKernelMapped,&SDRAMSize);
        DPF(0,("tm_main.c: trimedia_mmap: halGetSDRAMInfo has returned with SDRAMPhysical is 0x%x and SDRAMKernelMapped is 0x%x and SDRAMSize is 0x%x\n"
			, SDRAMPhysical
			, SDRAMKernelMapped
			, SDRAMSize));
        simple_region_start = virt_to_phys(SDRAMKernelMapped);
        simple_region_size  = SDRAMSize;
		
        physical = simple_region_start + off; // + PAGE_OFFSET;
        psize    = simple_region_size  - off;
		
        DPF(0,("tm_main.c: trimedia_mmap: physical is 0x%x\n",physical));
		
        if (off & (PAGE_SIZE-1))
        {
			DPF(0,("tm_main.c: trimedia_mmap: no aligned offsets?!\n"));
			return -EINVAL;     /* need aligned offsets */
        }
        if (vsize > psize)
        {
			DPF(0,("tm_main.c: trimedia_mmap: span is too high!\n"));
			return -EINVAL;  /* spans too high */
        }
        if (remap_page_range( vma->vm_start
			, physical
			, vsize
			, /* vma->vm_page_prot */ PAGE_USERIO))
        {
			DPF(0,("tm_main.c: trimedia_mmap: remap_page_range failed.\n"));
			return -EAGAIN;
        }
	}
	else if (vsize == 200000) // This the same code as above
	{
		// mapping MMIO space to user space
        HalHandle = ((TMManDeviceObject *)(TMManGlobal->DeviceList[0]))->HalHandle;
        halGetMMIOInfo(HalHandle, &MMIOPhysical, &MMIOKernelMapped,&MMIOSize);
        DPF(0,("tm_main.c: trimedia_mmap: halGetMMIOInfo has returned with MMIOPhysical is 0x%x and MMIOKernelMapped is 0x%x and MMIOSize is 0x%x\n"
			, MMIOPhysical
			, MMIOKernelMapped
			, MMIOSize));
        simple_region_start = virt_to_phys(MMIOKernelMapped);
        simple_region_size  = MMIOSize;
		
        physical = simple_region_start + off; // + PAGE_OFFSET;
        psize    = simple_region_size  - off;
		
        DPF(0,("tm_main.c: trimedia_mmap: physical is 0x%x\n",physical));
		
        if (off & (PAGE_SIZE-1))
        {
			DPF(0,("tm_main.c: trimedia_mmap: no aligned offsets?!\n"));
			return -EINVAL;     /* need aligned offsets */
        }
        if (vsize > psize)
        {
			DPF(0,("tm_main.c: trimedia_mmap: span is too high!\n"));
			return -EINVAL;  /* spans too high */
        }
        if (remap_page_range( vma->vm_start
			, physical
			, vsize
			, /* (vma->vm_page_prot)*/  PAGE_USERIO))
        {
			DPF(0,("tm_main.c: trimedia_mmap: remap_page_range failed.\n"));
			return -EAGAIN;
        }     
	}
	else if (vsize < 200000)
	{
		physical = off; // The offset is (mis)used to pass the physical address
		DPF(0,("tm_main.c: tm_mmap: now the shared mem should be remapped.\n"));
		if (remap_page_range( vma->vm_start
			, physical
			, vsize
			, /* vma->vm_page_prot*/ /*PAGE_USERIO*/ PAGE_KERNEL_UNCACHED ))
        {
			DPF(0,("tm_main.c: trimedia_mmap: remap_page_range failed.\n"));
			return -EAGAIN;
        }            
	}
	
	return 0;
}



/*
* The "extended" operations -- only seek
*/

long long trimedia_lseek (struct file *filp,
						  long long off, int whence)
{
	printk("Trimedi_lseek has been called.\n");
	return -EBUSY;
}


/*
* The different default file operations
*/

struct file_operations trimedia_fops = {
    trimedia_lseek,
		trimedia_read,
		trimedia_write,
		NULL,                   /* trimedia_readdir */
		NULL,                   /* trimedia_select */
		trimedia_ioctl,
		trimedia_mmap,          /* trimedia_mmap */
		trimedia_open,
		NULL,		    /* ZC // Mon Apr 19 19:25:05 1999
		ADDED trimedia_flush for kernel 2.2.x */
		trimedia_release
		/* nothing more, fill with NULLs */
};



/*
* Finally, the module stuff
*/

int init_module(void)
{
    char            devFileName[20];
    int             result, i;
    struct inode  * inodep;
    struct dentry * dentryp;
    ioparms         IoParms;
	
    printk("Hello world.\n");
	
    /*
	* Register your major, and accept a dynamic number
	*/
    
    result = register_chrdev(trimedia_major, "trimedia", &trimedia_fops);
    if (result < 0) {
		printk("can't get major %d\n",trimedia_major);
		return result;
    }
    if (trimedia_major == 0) trimedia_major = result; /* dynamic */
	
    printk("registered character device 'trimedia'; major = %d\n", 
		trimedia_major);
	
		/* PROBLEM: the dynamic device number can change between
		invocations!!!  Therefore, the device numbers must be changed
		in the inodes to allow user space access to devices at the
	newly allocated major... */
	
	
    for (i = 0 ; i < TRIMEDIA_NR_DEVS; i++) {
		
	if (strlen(TM_DEVICE_NAME) <= 10) { /* only when name short
										enough to fit name,
										'/dev/' and the actual
		minor in 19 characters...  */
		
		sprintf(devFileName, "/dev/%s%d", TM_DEVICE_NAME, i) ;
		
		printk("checking major # of device #%d"
			" ('%s')...\n", i, devFileName) ;
		
		
		dentryp = open_namei(devFileName, 0, 0) ;
		if (IS_ERR(dentryp)) {
			printk("couldn't get dir entry for"
				" device special file '%s'!\n", devFileName) ;
			continue ;
		}
		inodep = dentryp -> d_inode ;
		if (!inodep) {
			printk("invalid inode in dir entry for device"
				" special file '%s'!\n", devFileName) ;
			continue ;
		}
		
		if (MAJOR(inodep -> i_rdev) != trimedia_major) {
			printk("need to change the device major"
				" # of '%s' (was %d, now assigned value %d).\n",
				devFileName, MAJOR(inodep -> i_rdev),
				trimedia_major) ;
				/* CAUTION: will need fixing for struct-based kdev_t
			(coming soon to a shop near you :-) */
			inodep -> i_rdev =
				MKDEV(trimedia_major, MINOR(inodep -> i_rdev)) ;
			
			/* mark inode dirty and write back to holding device */
			
			mark_inode_dirty(inodep) ;
			sync_dev(inodep -> i_dev) ;
		}
		else {
			printk("no need to update device"
				" major # of '%s'.\n", devFileName) ;
		}
	}
    
	else {
		printk("device file name '%s' too long, cannot"
			" safely update dynamic major numbers in '/dev' !\n",
			TM_DEVICE_NAME) ;
	}
    }
	
    //    EXPORT_NO_SYMBOLS ;		/* otherwise, leave global symbols visible */
	
    IoParms.tid = (UInt32)current->pid;
    printk("tm_main.c: init_module: getpid returned %d", IoParms.tid);
    tmmInit(&IoParms);
	
    return 0; /* succeed */
	
}

void cleanup_module(void)
{
	tHalObject* pHal;
	UInt8       IrqLine;  
	UInt32      Dummy;
	UInt32      SharedDataAddress;
	if (TMManGlobal->DeviceList[0] != Null)
	{
		pHal    = ( (TMManDeviceObject *)(TMManGlobal->DeviceList[0]) )->HalHandle; 
		/*DL : in principle we are not to know the internal structure 
		of HalHandle in this file*/
		Dummy   = ((tHalObject*)pHal)->PCIRegisters[constTMMANPCIRegisters-1];
		IrqLine = (UInt8)(Dummy & (0xff));
		free_irq(IrqLine, (void *)pHal );
		if (((TMManDeviceObject *)TMManGlobal->DeviceList[0])->SharedData != Null)
		{
			SharedDataAddress =  ( (TMManDeviceObject *)(TMManGlobal->DeviceList[0]) )->SharedData;
			SharedDataAddress = KSEG0ADDR(SharedDataAddress);
			MmFreeContiguousMemory(SharedDataAddress);
		}
	}
	else
	{
		printk("tm_main.c: cleanup_module: TMManGlobal->DeviceList[0] = Null \n");
	}
	unregister_chrdev(trimedia_major, "trimedia");
	printk("tm_main.c: cleanup_module: So long, and thanks for all the fish.\n");
}


















#include <PCI.h>
#include "xpress.h"

#define POWERUP_TIMEOUT 8000000LL;

#define AC97_BAD_VALUE		(0xffff)
#define INSANE				(100L)
#define CRAZY				(200L)

#define DEBUG_BREAK(x) 		dprintf("xpress: ac97 timeout (debug_break) %d\n",x);

#define AC97_SPEW_ERR(x)		ddprintf(x)
//#define AC97_SPEW_ERR(x)		(void) 0
//#define AC97_SPEW(x)		ddprintf(x)
#define AC97_SPEW(x)		(void) 0

extern pci_module_info *pci;

status_t 	xpress_init_codec(xpress_dev *, uchar, bool );
void 		xpress_uninit_codec(xpress_dev *, uchar );
uint16 		xpress_codec_read(void *, uchar );
status_t	xpress_codec_write(void *, uchar , uint16 , uint16 );
void 		lock_ac97(xpress_dev *, uchar);
void 		unlock_ac97(xpress_dev *, uchar);
 

void lock_ac97(xpress_dev * card, uchar index)
{
	if (atomic_add(&card->ac97ben[index], 1) > 0)
		acquire_sem(card->ac97sem[index]);
}

void unlock_ac97(xpress_dev * card, uchar index)
{
	if (atomic_add(&card->ac97ben[index], -1) > 1)
		release_sem_etc(card->ac97sem[index], 1, B_DO_NOT_RESCHEDULE);
}

#if 0 
static uint16 xpress_codec_read_unsecure(xpress_dev *card, uchar offset, uchar index)
{
	uchar * F3 = card->f3; 
    uint32 ccr;
    long int lSanity = INSANE;
    uint16 wReturn = AC97_BAD_VALUE;

	/* only support 1 codec for now */
    if (index==0)
	{
        
        /* is this how we know it's ready?? */
        ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
        while ((ccr & 0x00010000) ) {
            ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
			/* save the bus... */
			spin(1); 
            /* Loop until bit 16 is 0 */
            lSanity--;
            if (lSanity<=0) {
                DEBUG_BREAK(0);
                return AC97_BAD_VALUE;
            }
        }

        /* assemble the register */
		ccr = offset << 24;
		/* this is read....      */
		ccr |= 0x80000000;         
		ccr |= (index << 22);
		ccr &= 0xffc00000;
		MEM_WR32( F3 + X_CODEC_COMMAND_REG, ccr );

		/* wait for ready (bit 16) to go 0?? */
		/* make sure the command gets sent?? */
        /* is this how we know it's ready??? */
        ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
		lSanity = INSANE;
        while ((ccr & 0x00010000) ) {
            ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
			/* save the bus... */
			spin(1); 
            /* Loop until bit 16 is 0 */
            lSanity--;
            if (lSanity<=0) {
                DEBUG_BREAK(1);
                return AC97_BAD_VALUE;
            }
        }


		/* get the data from the status register */
        ccr = MEM_RD32( F3 + X_CODEC_STATUS_REG );
		lSanity = INSANE;
        while ((ccr >> 24 ) != offset ) {
            ccr = MEM_RD32( F3 + X_CODEC_STATUS_REG );
			/* save the bus... */
			spin(1); 
            /* Loop until address matches! */
            lSanity--;
            if (lSanity<=0) {
                DEBUG_BREAK(2);
                return AC97_BAD_VALUE;
            }
        }
		/* get the data from the status register */
        ccr = MEM_RD32( F3 + X_CODEC_STATUS_REG );
		lSanity = INSANE;
        while (!(ccr & 0x00020000) ) {
            ccr = MEM_RD32( F3 + X_CODEC_STATUS_REG );
			/* save the bus... */
			spin(1); 
            /* Loop until bit 17 is 1 */
            lSanity--;
            if (lSanity<=0) {
                DEBUG_BREAK(3);
                return AC97_BAD_VALUE;
            }
        }


        /* if the data is new, I will assume that */
        /* is valid, but let's check anyway       */
        if (ccr & 0x00010000) {
			/* does offset match? */
			if ( (ccr >> 24) == offset){
	        	AC97_SPEW(("xpress: codec data is new, valid, and matching\n"));
				AC97_SPEW(("xpress: codec data is %lx\n",ccr));
	        	wReturn = (uint16)ccr;
			}
			else{
	        	AC97_SPEW_ERR(("xpress: codec data is new, and valid, but NOT matching\n"));
				AC97_SPEW(("xpress: codec data is %lx\n",ccr));
	        	wReturn = AC97_BAD_VALUE;
			}
        }
        else {
	        ccr = MEM_RD32( F3 + X_CODEC_STATUS_REG );
			lSanity = INSANE;
	        while ((ccr & 0x00010000) ) {
	            ccr = MEM_RD32( F3 + X_CODEC_STATUS_REG );
				/* save the bus... */
				spin(1); 
	            /* Loop until bit 16 is 0 */
	            lSanity--;
	            if (lSanity<=0) {
	                DEBUG_BREAK(4);
	                return AC97_BAD_VALUE;
	            }
	        }
			/* does offset match? */
			if ( (ccr >> 24) == offset){
	        	AC97_SPEW(("xpress: codec data is NOW new, valid, and matching \n"));
				AC97_SPEW(("xpress: codec data is %lx\n",ccr));
	        	wReturn = (uint16)ccr;
			}
			else{
	        	AC97_SPEW_ERR(("xpress: codec data is NOW new, and valid, but NOT matching \n"));
				AC97_SPEW(("xpress: codec data is %lx\n",ccr));
	        	wReturn = AC97_BAD_VALUE;
			}
        }
    }

    return wReturn;
}
#else

static uint16 xpress_codec_read_unsecure(xpress_dev *card, uchar offset, uchar index)
{
	uchar * F3 = card->f3; 
    uint32 ccr;
    long int lSanity = CRAZY;
    uint16 wReturn = AC97_BAD_VALUE;

	/* only support 1 codec for now */
    if (index==0)
	{
     
     	do {
	     	/* wait for valid to clear.........*/   
	        ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
	        while ((ccr & 0x00010000) ) {
	            ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
				/* save the bus... */
				spin(1); 
	            /* Loop until bit 16 is 0 */
	            lSanity--;
	            if (lSanity<=0) {
	                DEBUG_BREAK(0);
	                return AC97_BAD_VALUE;
	            }
	        }

	        /* assemble the register */
			ccr = offset << 24;
			/* this is read....      */
			ccr |= 0x80000000;         
			ccr |= (index << 22);
			ccr &= 0xffc00000;
			MEM_WR32( F3 + X_CODEC_COMMAND_REG, ccr );

			/* wait for valid in the status reg */
	        ccr = MEM_RD32( F3 + X_CODEC_STATUS_REG );
			lSanity = CRAZY;
	        while (!(ccr & 0x00010000) ) {
	            ccr = MEM_RD32( F3 + X_CODEC_STATUS_REG );
				/* save the bus... */
				spin(1); 
	            /* Loop until bit 16 is 1 */
	            lSanity--;
	            if (lSanity<=0) {
	                DEBUG_BREAK(3);
	                return AC97_BAD_VALUE;
	            }
	        }
		}
		while ((ccr >> 24 ) != offset );
		wReturn = (uint16) ccr;
    }
    return wReturn;
}
#endif

uint16 xpress_codec_read(void* host, uchar offset)
{
	xpress_host * xhost = (xpress_host *) host;
	xpress_dev * card   =  xhost->device ;
	uchar index			=  xhost->index;

	uint16 ret = AC97_BAD_VALUE;

	AC97_SPEW(("xpress: codec_read offset 0x%x\n",(uint16)offset));
	
	lock_ac97(card, index);
		ret = xpress_codec_read_unsecure(card, offset, index);
	unlock_ac97(card, index);	
	return ret;
}

static status_t xpress_codec_write_unsecure(void* host, uchar offset, uint16 value, uint16 mask )
{
	xpress_host * xhost = (xpress_host *) host;
	xpress_dev * card   =  xhost->device ;
	uchar index			=  xhost->index;
	
	uchar * F3 = card->f3; 
	status_t ret = B_OK;
    long int lSanity = 100L;
    uint32 ccr;

		if (mask != 0xffff) {
			uint16 old  = xpress_codec_read_unsecure(card, offset, index);
			value = (value&mask)|(old&~mask);
		}

		/* only support 1 codec for now */
	    if (index==0)
		{
	        /* is this how we know it's ready?? */
	        ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
	        while ((ccr & 0x00010000) ) {
	            ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
				/* save the bus... */
				spin(1); 
	            /* Loop until bit 16 is 0 */
	            lSanity--;
	            if (lSanity<=0) {
	                DEBUG_BREAK(5);
	                ret = AC97_BAD_VALUE;
	            }
	        }
	
			if (ret == B_OK) {
		        /* assemble the register */
				ccr = offset << 24;
				/* this is write....      */
				ccr &= 0x7fffffff;         
				ccr |= (index << 22);
				ccr |= value;
				MEM_WR32( F3 + X_CODEC_COMMAND_REG, ccr );
#if 0		
				/* check for ready (bit 16) again??? */
				/* make sure the command gets sent?? */
		        /* is this how we know it's ready?? */
		        ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
		        while ((ccr & 0x00010000) ) {
		            ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
					/* save the bus... */
					spin(1); 
		            /* Loop until bit 16 is 0 */
		            lSanity--;
		            if (lSanity<=0) {
		                DEBUG_BREAK(6);
		                ret = AC97_BAD_VALUE;
		            }
		        }
#endif		        
			}
	    }
	return ret;
}

status_t xpress_codec_write(void* host, uchar offset, uint16 value, uint16 mask )
{
	xpress_host * xhost = (xpress_host *) host;
	xpress_dev * card   =  xhost->device ;
	uchar index			=  xhost->index;
	status_t ret = B_OK;

	AC97_SPEW(("xpress: codec_write offset: 0x%x val: 0x%x mask: 0x%x\n",offset, value, mask));

	if (mask == 0)
		return ret;

	lock_ac97(card, index);
		ret = xpress_codec_write_unsecure( host, offset, value, mask);
	unlock_ac97(card, index);
	return ret;
}

status_t xpress_cached_codec_write(void* host, uchar offset, uint16 value, uint16 mask )
{
	xpress_host * xhost = (xpress_host *) host;
	xpress_dev * card   =  xhost->device ;
	uchar index			=  xhost->index;
	status_t ret = B_OK;

	AC97_SPEW(("xpress: cached_codec_write offset: 0x%x val: 0x%x mask: 0x%x\n",offset, value, mask));

	if (mask == 0 )
		return ret;
	if (offset >= X_MAX_CCC)
		return B_BAD_VALUE;	

	lock_ac97(card, index);
		/* get tricky...(save a codec read) */
		value = (value & mask) | (card->ccc[offset] & ~mask);	
		ret = xpress_codec_write_unsecure( host, offset, value, 0xFFFF);
		card->ccc[offset] = value;
	unlock_ac97(card, index);
	
	return ret;
}

status_t xpress_cache_codec_values(void* host, uchar offset )
{
	xpress_host * xhost = (xpress_host *) host;
	xpress_dev * card   =  xhost->device ;
	uchar index			=  xhost->index;
	status_t ret = B_OK;

	AC97_SPEW(("xpress: cache_codec_values offset: 0x%x \n",offset));

	if (offset >= X_MAX_CCC)
		return B_BAD_VALUE;	

	lock_ac97(card, index);
		card->ccc[offset] = xpress_codec_read_unsecure( card, offset, index);
		ret = card->ccc[offset];
	unlock_ac97(card, index);	

	return ret;
}


status_t xpress_init_codec(xpress_dev * card, uchar index, bool init_hw)
{
	status_t err = B_OK;
	bigtime_t timeout_at;
	uchar * F3 = card->f3; 

	AC97_SPEW(("xpress: init_codec\n"));

	card->xhost[index].device = card;
	card->xhost[index].index  = index;
	
	/* Does the BIOS init the ac97 interface? */
	if (init_hw)
	{
		uint32 ccr;
		timeout_at = system_time()+POWERUP_TIMEOUT;

		MEM_WR32( F3 + X_CODEC_COMMAND_REG, 0 );

        ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
        while ((ccr & 0x00010000) ) {
			if (system_time() > timeout_at)
				return B_TIMED_OUT;
            ccr = MEM_RD32( F3 + X_CODEC_COMMAND_REG );
			/* save the bus... */
			spin(1);
		}
	}
	
	/* init the ac97 chip */

		err = ac97init(&card->codec[index], 
						(void*)&card->xhost[index],
						xpress_codec_write,
						xpress_codec_read,
						init_hw);
	
		if(err!=B_OK) {
			dprintf("xpress: xpress_init_codec() err = %ld!\n",err);
		}

	return err;				
}

void xpress_uninit_codec(xpress_dev * card, uchar index)
{
	ddprintf(("xpress: xpress_uninit_codec()\n"));
	ac97uninit(&card->codec[index]); 
	card->xhost[index].device = NULL;
	card->xhost[index].index  = 0;
}





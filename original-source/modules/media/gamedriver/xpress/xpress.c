// Copyright 2001, Be Incorporated. All Rights Reserved 
// This file may be used under the terms of the Be Sample Code License 
#include <stdlib.h>
#include <math.h>
#include <driver_settings.h>
#include <string.h>

#include "xpress.h"
#include "debug.h"

static pci_module_info * g_pci;
ac97_module_info_v3 * g_ac97 = NULL;
const char ac97_name[] = B_AC97_MODULE_NAME_V3;
static const ga_utility_funcs * g_utility;
int g_debug = 0;

status_t 	xpress_init_ac97_module(xpress_dev *, uchar);
status_t	xpress_uninit_ac97_module(xpress_dev *, uchar);

#define DEBUG_REGISTERS_SPINLOCK 0

#define POWERUP_TIMEOUT 8000000LL

#define AC97_BAD_VALUE		(0xffff)
#define INSANE				(100L)
#define CRAZY				(200L)


#define SPINLOCK_REGISTERS(dev) for (spinlock_registers((dev) DEBUG_ARGS2), \
		(dev)->spinlocked=LOCKVAL; \
	(dev)->spinlocked || unspinlock_registers((dev) DEBUG_ARGS2); \
	(dev)->spinlocked=0)


static void
spinlock_registers(
	xpress_dev * dev
	DEBUG_ARGS_DECL2)
{
	cpu_status cp;
#if DEBUG_REGISTERS_SPINLOCK
	lddprintf(("\033[31m>>> REGISTERSPIN\033[0m %p %s %d\n", &dev->registers, file, line));
#endif
	cp = disable_interrupts();
	acquire_spinlock(&dev->registers);
	dev->cpu = cp;
}

static bool
unspinlock_registers(
	xpress_dev * dev
	DEBUG_ARGS_DECL2)
{
	cpu_status cp = dev->cpu;
	release_spinlock(&dev->registers);
	restore_interrupts(cp);
	/*	sequencing may be off, because we print outside the lock to avoid	*/
	/*	delayed dprintf()-ing	*/
#if DEBUG_REGISTERS_SPINLOCK
	lddprintf(("\033[31m<<< REGISTERSPIN\033[0m %p %s %d\n", &dev->registers, file, line));
#endif
	return false;
}

#define MEM_RD32(x) (*((volatile uint32 *)(x)))
#define MEM_RD8(x) (*((volatile uint8 *)(x)))
#define MEM_WR32(x,y) ((*((volatile uint32 *)(x))) = (y))
#define MEM_WR8(x,y) ((*((volatile uint8 *)(x))) = (y))
#if 0
# define DEBUG_BREAK(x) kernel_debugger("xpress ac97 break " #x)
#else
# define DEBUG_BREAK(x)
#endif
#define AC97_SPEW(x) (void)0

#define X_CODEC_COMMAND_REG 0x0C
#define X_CODEC_STATUS_REG 0x08

static uint16 xpress_codec_read_unsecure(xpress_dev *card, uchar offset)
{
	uchar * F3 = (uchar *)card->f3; 
    uint32 ccr;
    long int lSanity = CRAZY;
    uint16 wReturn = AC97_BAD_VALUE;

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
//		ccr |= (index << 22); index is 0
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

    return wReturn;
}

uint16 xpress_codec_read(xpress_dev * card, uchar offset)
{
	uint16 ret = AC97_BAD_VALUE;

	ddprintf(("xpress: codec_read(0x%02x) card = 0x%08lx\n",
		offset, (uint32)card));

	AC97_SPEW(("xpress: codec_read offset 0x%x\n",(uint16)offset));
	
	SPINLOCK_REGISTERS(card)
	{
		ret = xpress_codec_read_unsecure(card, offset);
	}
	return ret;
}

static status_t xpress_codec_write_unsecure(xpress_dev * card, uchar offset, uint16 value, uint16 mask )
{
	uchar * F3 = (uchar *)card->f3; 
	status_t ret = B_OK;
    long int lSanity = 100L;
    uint32 ccr;

	if (mask != 0xffff) {
		uint16 old  = xpress_codec_read_unsecure(card, offset);
		value = (value&mask)|(old&~mask);
	}

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
//		ccr |= (index << 22);	index is 0
		ccr |= value;
		MEM_WR32( F3 + X_CODEC_COMMAND_REG, ccr );
	}
	return ret;
}

status_t xpress_codec_write(xpress_dev * card, uchar offset, uint16 value, uint16 mask )
{
	status_t ret = B_OK;

	ddprintf(("xpress: codec_write(0x%02x, 0x%04x, 0x%04x) card = 0x%08lx\n",
		offset, value, mask, (uint32)card));
	AC97_SPEW(("xpress: codec_write offset: 0x%x val: 0x%x mask: 0x%x\n",offset, value, mask));

	if (mask == 0)
		return ret;

	SPINLOCK_REGISTERS(card)
	{
		ret = xpress_codec_write_unsecure( card, offset, value, mask);
	}
	return ret;
}



static inline void
xpress_write_reg(
	xpress_dev * dev,
	uint8 reg,
	uint8 size,
	uint32 value)
{
#if DEBUG
	d2printf(("\033[36mXPRESS(0x%X) = 0x%04lx\033[0m\n", reg, value));
#endif
	if (size == 4)
	{
		*((volatile uint32*)(reg+dev->f3)) = value;
	}
	else
	{
		ASSERT(size == 1);
		*((volatile uint8*)(reg+dev->f3)) = value;
	}
	__eieio();
}

static inline uint32
xpress_read_reg(
	xpress_dev * dev,
	uint8 reg,
	uint8 size)
{
	if (size == 4)
	{
		return *((volatile uint32*)(reg+dev->f3));
	}
	else
	{
		ASSERT(size == 1);
		return *((volatile uint8*)(reg+dev->f3));
	}
}


static void
xpress_reset_dac_dma(
	xpress_dev * dev)
{
	int ix;
	cancel_timer(&dev->dac_stream.pulse);
	//	set all entries to EOT
	if (dev->dac_stream.prd_base != NULL)
	{
		for (ix=0; ix<dev->dac_stream.prd_slots; ix++)
		{
			dev->dac_stream.prd_base[ix].flags = XPRESS_PRD_EOT;
			dev->dac_stream.prd_base[ix].size = 0;
			__eieio();
		}
	}
	dev->dac_stream.is_running = false;
}

static void
xpress_reset_adc_dma(
	xpress_dev * dev)
{
	int ix;
	cancel_timer(&dev->adc_stream.pulse);
	//	set all entries to EOT
	if (dev->adc_stream.prd_base != NULL)
	{
		for (ix=0; ix<dev->adc_stream.prd_slots; ix++)
		{
			dev->adc_stream.prd_base[ix].flags = XPRESS_PRD_EOT;
			dev->adc_stream.prd_base[ix].size = 0;
			__eieio();
		}
	}
	dev->adc_stream.is_running = false;
}

static int32
xpress_dac_timer(
	timer * t)
{
	xpress_stream * strm = (xpress_stream *)t;
	uint32 slot = xpress_read_reg((xpress_dev *)strm->cookie, XPRESS_DAC_PRD) - sizeof(xpress_prd);
	int ix;
	int block_per_page;
	bigtime_t now = system_time();
	int size = 0;

	slot = (slot-strm->prd_base_phys)/sizeof(xpress_prd);
	if (slot < strm->cur_slot)
	{
		//	we looped
		for (ix=strm->cur_slot; ix<strm->prd_slots; ix++)
		{
			size += strm->prd_base[ix].size;
		}
		for (ix=0; ix<slot; ix++)
		{
			size += strm->prd_base[ix].size;
		}
	}
	else
	{
		for (ix=strm->cur_slot; ix<slot; ix++)
		{
			size += strm->prd_base[ix].size;
		}
	}
//dprintf("played %l bytes (%d frames) slot %ld to %ld\n", size, size/strm->frame_size, strm->cur_slot, slot);
	strm->cur_slot = slot;
	strm->data.update_timing(strm->data.cookie, size/strm->frame_size, now);
	block_per_page = strm->page_frames/FRAME_BLOCK;
	ix = block_per_page-(slot % block_per_page);
//	kprintf("D");
//	if (ix != block_per_page)
//	{
//		kprintf("%c", '0'+ix);
//	}
	add_timer(t, xpress_dac_timer, (bigtime_t)(1000000/strm->format.cvsr*ix*FRAME_BLOCK),
		B_ONE_SHOT_RELATIVE_TIMER);
	return strm->data.int_done(strm->data.cookie);
}

static int32
xpress_adc_timer(
	timer * t)
{
	xpress_stream * strm = (xpress_stream *)t;
	uint32 slot = xpress_read_reg((xpress_dev *)strm->cookie, XPRESS_ADC_PRD) - sizeof(xpress_prd);
	int ix;
	int block_per_page;
	bigtime_t now = system_time();
	int size = 0;

	slot = (slot-strm->prd_base_phys)/sizeof(xpress_prd);
	if (slot < strm->cur_slot)
	{
		//	we looped
		for (ix=strm->cur_slot; ix<strm->prd_slots; ix++)
		{
			size += strm->prd_base[ix].size;
		}
		for (ix=0; ix<slot; ix++)
		{
			size += strm->prd_base[ix].size;
		}
	}
	else
	{
		for (ix=strm->cur_slot; ix<slot; ix++)
		{
			size += strm->prd_base[ix].size;
		}
	}
	strm->cur_slot = slot;
	strm->data.update_timing(strm->data.cookie, size/strm->frame_size, now);
	block_per_page = strm->page_frames/FRAME_BLOCK;
	ix = block_per_page-(slot % block_per_page);
	add_timer(t, xpress_adc_timer, (bigtime_t)(1000000/strm->format.cvsr*ix*FRAME_BLOCK),
		B_ONE_SHOT_RELATIVE_TIMER);
	return strm->data.int_done(strm->data.cookie);
}

static void
xpress_program_dac_dma(
	xpress_dev * dev)
{
	//	set DMA address
	xpress_write_reg(dev, XPRESS_DAC_PRD, dev->dac_stream.prd_base_phys);
	//	start DMA
	xpress_write_reg(dev, XPRESS_DAC_CMD, XPRESS_DAC_RUN);
	//	program timer interrupt
	dev->dac_stream.cur_slot = 0;
	dev->dac_stream.is_running = true;
	add_timer(&dev->dac_stream.pulse, xpress_dac_timer,
		(bigtime_t)(1000000/dev->dac_stream.format.cvsr*dev->dac_stream.page_frames),
		B_ONE_SHOT_RELATIVE_TIMER);
}

static void
xpress_program_adc_dma(
	xpress_dev * dev)
{
	//	set DMA address
	xpress_write_reg(dev, XPRESS_ADC_PRD, dev->adc_stream.prd_base_phys);
	//	start DMA
	xpress_write_reg(dev, XPRESS_ADC_CMD, XPRESS_ADC_RUN);
	//	program timer interrupt
	dev->adc_stream.cur_slot = 0;
	dev->adc_stream.is_running = true;
	add_timer(&dev->adc_stream.pulse, xpress_adc_timer,
		(bigtime_t)(1000000/dev->adc_stream.format.cvsr*dev->adc_stream.page_frames),
		B_ONE_SHOT_RELATIVE_TIMER);
}

static void
xpress_init_streams(
	xpress_dev * dev)
{
	game_codec_format cFormat;

	memset(&dev->adc_stream, 0, sizeof(&dev->adc_stream));
	memset(&dev->dac_stream, 0, sizeof(&dev->dac_stream));

	g_ac97->AC97_get_codec_format(dev->ac97codec, AC97_CURRENT_ADC_INFO, &cFormat);
	dev->adc_stream.format.frame_rate = cFormat.frame_rate;
	dev->adc_stream.format.cvsr = cFormat.cvsr;
	dev->adc_stream.format.format = cFormat.format;
	dev->adc_stream.format.channels = cFormat.channels;
	dev->adc_stream.format.chunk_frame_count = 512;
	dev->adc_stream.frame_size = cFormat.channels*g_utility->find_sample_size(cFormat.format);

	g_ac97->AC97_get_codec_format(dev->ac97codec, AC97_CURRENT_DAC_INFO, &cFormat);
	dev->dac_stream.format.frame_rate = cFormat.frame_rate;
	dev->dac_stream.format.cvsr = cFormat.cvsr;
	dev->dac_stream.format.format = cFormat.format;
	dev->dac_stream.format.channels = cFormat.channels;
	dev->dac_stream.format.chunk_frame_count = 512;
	dev->dac_stream.frame_size = cFormat.channels*g_utility->find_sample_size(cFormat.format);
}


static bool
xpress_setup(
	const pci_info * info,
	xpress_dev * dev)
{
	ASSERT(sizeof(dev->f3) == sizeof(void*));
	dev->f3area = map_physical_memory("xpress registers", (void*)dev->pci.u.h0.base_registers[0],
			B_PAGE_SIZE, B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, (void **)&dev->f3);
#if 0
	//	turn up the mixer
	xpress_codec_write(dev, 0x00, 0xffff, 0xffff);
	then = system_time() + POWERUP_TIMEOUT;
	while (system_time() < then)
	{
		uint16 r;
		snooze(100);
		r = xpress_codec_read(dev, 0x26);
		if ((r != AC97_BAD_VALUE) && ((r & 0xf) == 0xf))
			break;
	}
	ddprintf(("xpress: ac97 took %Ld us to init\n", system_time()-then-POWERUP_TIMEOUT));
	xpress_codec_write(dev, 0x02, 0x0404, 0xffff);
	xpress_codec_write(dev, 0x18, 0x0C0C, 0xffff);
#endif

if((xpress_codec_read(dev, 0x7c)== AC97_BAD_VALUE) || (xpress_codec_read(dev, 0x7e)== AC97_BAD_VALUE)) {
	dprintf("********************WARNING this codec looks like toasted\n");
	return false;
}


//	what we really should do is load the ac97 module
//	and defer all workings to it...

//	xpress_init_streams(dev);

	return true;
}



#if 0
static uint32
xpress_read_dma_addr(
	uint16 addr)
{
	return g_pci->read_io_32(addr);
}
#endif


static void
plug_uninit(
	plug_api * i_info)
{
	xpress_dev * dev = (xpress_dev *)i_info->cookie;

	ddprintf(("xpress: plug_uninit(%p), free %p\n", i_info, i_info->cookie));

	//	stop and clear DMA
	xpress_reset_dac_dma(dev);
	xpress_reset_adc_dma(dev);
	snooze(13000);	//	wait
	SPINLOCK_REGISTERS(dev)
	{
		xpress_write_reg(dev, XPRESS_DAC_CMD, XPRESS_DAC_STOP);
		xpress_write_reg(dev, XPRESS_ADC_CMD, XPRESS_ADC_STOP);
	}
	delete_area(dev->dac_stream.prd_area);
	dev->dac_stream.prd_area = -1;
	delete_area(dev->adc_stream.prd_area);
	dev->adc_stream.prd_area = -1;

//	remove_io_interrupt_handler(((xpress_dev *)(i_info->cookie))->pci.u.h0.interrupt_line,
//			xpress_interrupt, dev);
	if (g_ac97 != NULL)
	{
		// assumes we only have 1 codec...
		xpress_uninit_ac97_module(dev, 0);
		put_module(ac97_name);
		g_ac97 = NULL;
	}

	free(i_info->cookie);
}

static void
plug_init_codecs(
	plug_api * i_info,
	int * o_adcs,
	int * o_dacs,
	int * o_mixers)
{
	xpress_dev * dev = (xpress_dev *)(i_info->cookie);

	ddprintf(("xpress: plug_init_codecs(%p, %p, %p, %p)\n", i_info, o_adcs, o_dacs, o_mixers));

	//	Cold reset of the AC97 codec
	//  I assume the BIOS inits the ac97 interface 
	//  I also uassme that there is only 1 codec
	if (xpress_init_ac97_module(dev, 0) != B_OK)
	{
		//we need someway to store the return code as
		//this function doesn't have a return value...
		eprintf(("xpress: ac97 module failed to init--do SOMETHING!\n"));
	}

	*o_adcs = *o_dacs = 1;
	*o_mixers = 2;
}

void
plug_get_mixer_description(
	plug_api * i_info,
	int index,
	game_mixer_info * o_mixers)
{
	xpress_dev * dev = (xpress_dev *)(i_info->cookie);

	if (g_ac97)
	{
		ddprintf(("xpress: calling ac97_module: get_mixer_description\n"));
		g_ac97->AC97_plug_get_mixer_description(&dev->ac97codec[0], i_info, index, o_mixers);
	}
}

void
plug_get_mixer_controls(
	plug_api * i_info,
	const gaplug_get_mixer_controls * io_controls)
{	
	xpress_dev * dev = (xpress_dev *)(i_info->cookie);

	if (g_ac97)
		g_ac97->AC97_plug_get_mixer_controls(&dev->ac97codec[0], i_info, io_controls);
}	

void
plug_get_mixer_control_values(
	plug_api * i_info,
	const gaplug_mixer_control_values * io_request)
{	
	xpress_dev * dev = (xpress_dev *)(i_info->cookie);

	if (g_ac97)
		g_ac97->AC97_plug_get_mixer_control_values(&dev->ac97codec[0], i_info, io_request);
}

void
plug_set_mixer_control_values(
	plug_api * i_info,
	const gaplug_mixer_control_values * io_request)
{	
	xpress_dev * dev = (xpress_dev *)(i_info->cookie);

	if (g_ac97)
		g_ac97->AC97_plug_set_mixer_control_values(&dev->ac97codec[0], i_info, io_request);
}

#if 0
static void
xpress_set_dac_rate(
	xpress_dev * dev,
	float rate)
{
	uint32 r2 = (uint32)floor(rate*65535./48000.);
	xpress_write_indirect(dev, XPRESS_REG_PLAY_RATE_LO, r2 & 0xff);
	xpress_write_indirect(dev, XPRESS_REG_PLAY_RATE_HI, (r2 >> 8) & 0xff);
}

static status_t
xpress_set_format(
	xpress_dev * dev,
	const game_codec_format * in,
	game_codec_format * out,
	bool isDAC)
{
//	bool failDestructive = in->flags & GAME_CODEC_FAIL_IF_DESTRUCTIVE;
//	bool closestOK = in->flags & GAME_CODEC_CLOSEST_OK;
	uint8 oldDataFormat = xpress_read_reg(dev, XPRESS_REG_DMA_DATA_FORMAT_DISABLED);
	uint8 newDataFormat = oldDataFormat;

	if (in->flags & GAME_CODEC_SET_CHANNELS)
	{
		out->channels = in->channels;
		if (isDAC)
		{
			newDataFormat = (out->channels == 2) ? (oldDataFormat | XPRESS_FMT_PLAY_STEREO) :
				(oldDataFormat & ~XPRESS_FMT_PLAY_STEREO);
		}
		else
		{
			newDataFormat = (out->channels == 2) ? (oldDataFormat | XPRESS_FMT_REC_STEREO) :
				(oldDataFormat & ~XPRESS_FMT_REC_STEREO);
		}
	}
	if (in->flags & GAME_CODEC_SET_CHUNK_FRAME_COUNT)
	{
		out->chunk_frame_count = in->chunk_frame_count;
	}
	if (in->flags & GAME_CODEC_SET_FORMAT)
	{
		out->format = in->format;
		if (isDAC)
		{
			newDataFormat = (in->format & B_FMT_16BIT) ? (oldDataFormat | XPRESS_FMT_PLAY_16BIT) :
				(oldDataFormat & ~XPRESS_FMT_PLAY_16BIT);
		}
		else
		{
			newDataFormat = (in->format & B_FMT_16BIT) ? (oldDataFormat | XPRESS_FMT_REC_16BIT) :
				(oldDataFormat & ~XPRESS_FMT_REC_16BIT);
		}
	}
	if (in->flags & GAME_CODEC_SET_FRAME_RATE)
	{
		out->frame_rate = in->frame_rate;
		out->cvsr = in->cvsr;
		if (in->frame_rate & B_SR_CVSR)
		{
			if (isDAC)
			{
				xpress_set_dac_rate(dev, in->cvsr);
			}
			else
			{
				//	figure out a close-enough PLL value
				int m, n, r;
				int sr, sn = -1, sm;
				int diff;
				int freq = 0;
				int delta = 1000000;
				for (r=0; r<8; r++) {
					if ((1<<r)*in->cvsr*512 < XPRESS_PLL_MIN_FREQ) {
						continue;
					}
					if ((1<<r)*in->cvsr*512 > XPRESS_PLL_MAX_FREQ) {
						continue;
					}
					n = 0;
					do {
						n++;
						m = (int)floor(in->cvsr*512.0*(n+2)*(1<<r)/(XPRESS_F_REF)-2);
						if (m < 1) {
							continue;
						}
						if (m > 127) {
							ddprintf(("sonic_vibes: r = %d, m > 127; outahere\n", r));
							break;
						}
						freq = (int)floor((m+2)*(XPRESS_F_REF)/(512.0*(n+2)*(1<<r)));
						diff = freq - in->cvsr;
						if (diff < 0) {
							diff = -diff;
						}
						if (diff < delta) {
							sr = r;
							sn = n;
							sm = m;
							delta = diff;
						}
					} while (n < 31);
				}
				r = sr;
				n = sn;
				m = sm;
				ddprintf(("xpress: m = %d   r = %d   n = %d\n", m, r, n));
				ddprintf(("xpress: freq = %d\n", (int)((float)(m+2)/(float)((n+2)<<r)*XPRESS_F_REF/512)));
				if (n == -1)
				{
					eprintf(("\033[31mxpress: can't find PLL values to match rate %g\033[0m\n", in->cvsr));
					return B_DONT_DO_THAT;
				}
				out->cvsr = freq;
				SPINLOCK_REGISTERS(dev)
				{
					xpress_write_indirect(dev, XPRESS_REG_REC_PLL_M, m&0xff);
					xpress_write_indirect(dev, XPRESS_REG_REC_PLL_N_R, ((r<<5)&0xe0)|(n&0x1f));
				}
			}
		}
		else
		{
			if (isDAC)
			{
				xpress_set_dac_rate(dev, in->cvsr);
			}
			else
			{
				static const struct
				{
					uint32	flag;
					float	rate;
					uint8	asr;
				}
				asr_rates[] =
				{
					{ B_SR_48000, 48000.0, 0 },
					{ B_SR_24000, 24000.0, 1 },
					{ B_SR_16000, 16000.0, 2 },
					{ B_SR_12000, 12000.0, 3 },
					{ B_SR_8000, 8000.0, 5 }
				};
				int ix;
				for (ix=0; ix<sizeof(asr_rates)/sizeof(asr_rates[0]); ix++)
				{
					if (asr_rates[ix].flag & in->frame_rate)
					{
						goto done;
					}
				}
				eprintf(("\033[31mxpress: fell out the end of the ASR loop! Assuming 48 kHz\033[0m\n"));
				ix = 0;
done:
				SPINLOCK_REGISTERS(dev)
				{
					xpress_write_reg(dev, XPRESS_REG_REC_ALT_RATE,
						(xpress_read_reg(dev, XPRESS_REG_REC_ALT_RATE) & 0xf) | (asr_rates[ix].asr << 4));
					xpress_write_reg(dev, XPRESS_REG_REC_CLOCK_SOURCE,
						xpress_read_reg(dev, XPRESS_REG_REC_CLOCK_SOURCE) | XPRESS_REC_CLOCK_ALTERNATE);
				}
			}
		}
	}
d2printf(("\033[33mxpress: pre-change oldDataFormat 0x%02x newDataFormat 0x%02x\033[0m\n", oldDataFormat, newDataFormat));
	if (oldDataFormat != newDataFormat)
	{
		SPINLOCK_REGISTERS(dev)
		{
			if (isDAC)
			{
				dev->dac_stream.frame_size = out->channels *
					((out->format & B_FMT_16BIT) ? 2 : 1);
			}
			else
			{
				dev->adc_stream.frame_size = out->channels *
					((out->format & B_FMT_16BIT) ? 2 : 1);
			}
			xpress_write_indirect(dev, XPRESS_REG_DMA_DATA_FORMAT_ENABLED, newDataFormat);
//	read it back just so we re-set the "ENABLED" bit -- don't change this from DISABLED!
			oldDataFormat = xpress_read_indirect(dev, XPRESS_REG_DMA_DATA_FORMAT_DISABLED);
			/*UNUSED(oldDataFormat)*/
		}
d2printf(("\033[33mxpress: post-change oldDataFormat 0x%02x newDataFormat 0x%02x\033[0m\n", oldDataFormat, newDataFormat));
	}
	return B_OK;
}
#endif

static status_t
plug_set_codec_format(
	plug_api * i_info,
	game_codec_format * io_format)
{
	xpress_dev * dev = (xpress_dev *)(i_info->cookie);
	status_t err = B_OK;

	ddprintf(("xpress: plug_set_codec_format(%p, %p) flags 0x%08lx\n",
		i_info, io_format, io_format->flags));

	ASSERT((io_format->codec == GAME_MAKE_DAC_ID(0)) || (io_format->codec == GAME_MAKE_ADC_ID(0)));

	if (io_format->codec == GAME_MAKE_DAC_ID(0))
	{
		err = g_ac97->AC97_set_codec_format(dev->ac97codec, AC97_CURRENT_DAC_INFO, io_format);
		if (!err)
		{
			dev->dac_stream.format = *io_format;
			dev->dac_stream.format.chunk_frame_count = io_format->chunk_frame_count;
			dev->dac_stream.frame_size = io_format->channels *
				g_utility->find_sample_size(io_format->format);
		}
	}
	else
	{
		err = g_ac97->AC97_set_codec_format(dev->ac97codec, AC97_CURRENT_ADC_INFO, io_format);
		if (!err)
		{
			dev->adc_stream.format = *io_format;
			dev->adc_stream.format.chunk_frame_count = io_format->chunk_frame_count;
			dev->adc_stream.frame_size = io_format->channels *
				g_utility->find_sample_size(io_format->format);
		}
	}

	return err;
}

static status_t
plug_open_stream_buffer(
	plug_api * i_info,
	gaplug_open_stream_buffer * io_buffer)
{
//	xpress_dev * dev = (xpress_dev *)(i_info->cookie);

	if (io_buffer->io_size > MAX_FRAME_COUNT*4)
	{
		ddprintf(("xpress: requested size %ld is %ld frames; we can handle %d\n",
			io_buffer->io_size, io_buffer->io_size/4, MAX_FRAME_COUNT*4));
		return B_NO_MEMORY;
	}

	return B_OK;
}


static void
plug_get_codec_infos(
	plug_api * i_info,
	game_codec_info * o_adcs,
	game_codec_info * o_dacs)
{
	xpress_dev * dev = (xpress_dev *)(i_info->cookie);
	ac97_codec_info cInfo;

	ddprintf(("xpress: plug_get_codec_infos(%p, %p, %p)\n", i_info, o_adcs, o_dacs));

	//	Modify the default values to describe how this chip is set up

	g_ac97->AC97_get_codec_info(dev->ac97codec, AC97_ADC_INFO, &cInfo);
	o_adcs->min_chunk_frame_count = FRAME_BLOCK*2;
	o_adcs->max_chunk_frame_count = MAX_FRAME_COUNT;
	o_adcs->chunk_frame_count_increment = FRAME_BLOCK;
	o_adcs->frame_rates = cInfo.frame_rates;
	o_adcs->cvsr_min = cInfo.cvsr_min;
	o_adcs->cvsr_max = cInfo.cvsr_max;
	o_adcs->channel_counts = cInfo.channel_counts;	//	stereo
	//	streams have no capabilities, so don't give 'em any

	g_ac97->AC97_get_codec_info(dev->ac97codec, AC97_DAC_INFO, &cInfo);
	o_dacs->min_chunk_frame_count = FRAME_BLOCK*2;
	o_dacs->max_chunk_frame_count = MAX_FRAME_COUNT;
	o_dacs->chunk_frame_count_increment = FRAME_BLOCK;
	o_dacs->frame_rates = cInfo.frame_rates;
	o_dacs->cvsr_min = cInfo.cvsr_min;
	o_dacs->cvsr_max = cInfo.cvsr_max;
	o_dacs->channel_counts = cInfo.channel_counts;	//	stereo
	//	streams have no capabilities, so don't give 'em any

	//	power-on defaults

	xpress_init_streams(dev);

	o_adcs->cur_frame_rate = dev->adc_stream.format.frame_rate;
	o_adcs->cur_cvsr = dev->adc_stream.format.cvsr;
	o_adcs->cur_format = dev->adc_stream.format.format;
	o_adcs->cur_channel_count = dev->adc_stream.format.channels;
	o_adcs->cur_chunk_frame_count = dev->adc_stream.format.chunk_frame_count;

	o_dacs->cur_frame_rate = dev->dac_stream.format.frame_rate;
	o_dacs->cur_cvsr = dev->dac_stream.format.cvsr;
	o_dacs->cur_format = dev->dac_stream.format.format;
	o_dacs->cur_channel_count = dev->dac_stream.format.channels;
	o_dacs->cur_chunk_frame_count = dev->dac_stream.format.chunk_frame_count;
}


static status_t
plug_init_stream(
	plug_api * i_info,
	game_open_stream * io_request,
	const ga_stream_data * i_data,
	void ** o_stream_cookie)
{
	xpress_stream * strm;
	xpress_dev * dev = (xpress_dev *)i_info->cookie;

	ddprintf(("xpress: plug_init_stream(%p, %p)\n", i_info, io_request));
	i_info = i_info;

	if (GAME_IS_DAC(io_request->codec_id))
	{
		strm = &dev->dac_stream;
	}
	else
	{
		strm = &dev->adc_stream;
	}
	strm->cookie = dev;
	strm->data = *i_data;
	*o_stream_cookie = strm;

	return B_OK;
}

#if 0
static void
plug_remove_stream(
	plug_api * i_info,
	game_open_stream * i_stream,
	void * i_stream_cookie)
{
	xpress_dev * dev = (xpress_dev *)i_info->cookie;
	xpress_stream * strm = (xpress_stream *)i_stream_cookie;

	ddprintf(("xpress: plug_remove_stream(%p)\n", i_stream_cookie));
	i_stream = i_stream;

	ASSERT(((xpress_stream *)i_stream_cookie)->cookie == dev);
	ASSERT((i_stream_cookie == &dev->adc_stream) || (i_stream_cookie == &dev->dac_stream));

	//	turn off interrupts
	if (strm == &dev->dac_stream)
	{
		xpress_write_reg(dev, XPRESS_EM_IMASK, xpress_read_reg(dev, XPRESS_EM_IMASK) | XPRESS_STAT_AINT);
	}
	else
	{
		xpress_write_reg(dev, XPRESS_EM_IMASK, xpress_read_reg(dev, XPRESS_EM_IMASK) | XPRESS_STAT_CINT);
	}
	//	clear stream state
	strm->cookie = 0;
	memset(&strm->data, 0, sizeof(strm->data));
}
#endif

#if 0
static void
square_wave(
	void * base,
	size_t size)
{
	ulong * ul = (ulong *)base;
	static ulong uv[2] = { 0x80018001, 0x7fff7fff };
	size /= 4;
	while (size-- > 0)
	{
		*ul = uv[(size>>7)&1];
		ul++;
	}
}
#endif

static status_t
generate_prd(
	xpress_stream * stream,
	const gaplug_looping_buffer_info * info)
{
	int ix, pix, togo;
	physical_entry pents[16];
	size_t page_frames = info->page_frame_count;
	if (page_frames == 0) page_frames = stream->format.chunk_frame_count;

	if (page_frames < FRAME_BLOCK)
	{
		eprintf(("\033[31mxpress: page_frame_count must be at least %d\033[0m\n", FRAME_BLOCK));
		return B_BAD_VALUE;
	}
	stream->page_frames = page_frames;
	if (stream->prd_area > 0)
		delete_area(stream->prd_area);
	stream->prd_area = create_area("xpress PRD", (void**)&stream->prd_base,
			B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK, B_WRITE_AREA);
	if (stream->prd_area < 0)
	{
		eprintf(("\033[31mxpress: irrecoverable error in generate_prd(): %s\033[0m\n",
			strerror(stream->prd_area)));
		stream->prd_base = NULL;
		return stream->prd_area;
	}
	stream->prd_slots = info->frame_count/FRAME_BLOCK;
	ASSERT(stream->prd_slots <= MAX_FRAME_COUNT/FRAME_BLOCK);
	get_memory_map(info->address, info->frame_count*stream->frame_size, pents, 16);
	pix = 0; togo = info->frame_count*stream->frame_size;
	for (ix=0; ix<stream->prd_slots; ix++)
	{
		int s = FRAME_BLOCK*stream->frame_size;
		if (s > togo) s = togo;
		stream->prd_base[ix].address = (uint32)pents[pix].address;
		stream->prd_base[ix].size = s;
		stream->prd_base[ix].flags = 0;
		togo -= s;
		if (pents[pix].size <= s)
		{
			pix++;
		}
		else
		{
			pents[pix].address = (void *)(((uint32)pents[pix].address)+s);
			pents[pix].size -= s;
		}
	}
	get_memory_map(stream->prd_base, 8, pents, 1);
	stream->prd_base[ix].address = (uint32)pents[0].address;
	stream->prd_base[ix].size = 0;
	stream->prd_base[ix].flags = XPRESS_PRD_JMP;
	stream->prd_base_phys = (uint32)pents[0].address;

#if DEBUG
	d2printf(("xpress: prd_base 0x%08lx (phys 0x%08lx), %ld slots, each %d frames\n",
		(uint32)stream->prd_base, stream->prd_base_phys, stream->prd_slots, FRAME_BLOCK));
	for (ix=0; ix<=stream->prd_slots; ix++)
	{
		d2printf(("xpress: %d: 0x%08lx : %d   0x%04x\n", ix, stream->prd_base[ix].address,
			stream->prd_base[ix].size, stream->prd_base[ix].flags));
	}
#endif

	return B_OK;
}

static status_t
plug_run_streams(
	plug_api * i_info,
	const gaplug_run_streams * i_request)
{
	int ix;
	xpress_dev * dev = (xpress_dev *)(i_info->cookie);
	status_t ret = B_OK;

	ddprintf(("xpress: plug_run_streams(%p, %p)\n", i_info, i_request));

	//	stop streams (beacuse we start over if we're running a running stream)
	for (ix=0; ix<i_request->total_slots; ix++)
	{
		//	find out which stream
		bool isDAC = (i_request->info[ix].stream_cookie == &dev->dac_stream);

		d2printf(("xpress: request 0x%08lx, stream 0x%08lx, stream_cookie 0x%08lx\n",
				(uint32)i_request->info[ix].request, (uint32)i_request->info[ix].stream, (uint32)i_request->info[ix].stream_cookie));
		d2printf(("xpress: %s nu_state %ld\n", isDAC ? "DAC" : "ADC",
			i_request->info[ix].request->state));

		if (isDAC)
		{
			if (dev->dac_stream.is_running)
			{
				xpress_reset_dac_dma(dev);
				ASSERT(13000 > 1000000/dev->dac_stream.format.cvsr*FRAME_BLOCK);
				snooze(13000);
				SPINLOCK_REGISTERS(dev)
				{
					xpress_write_reg(dev, XPRESS_DAC_CMD, XPRESS_DAC_STOP);
				}
			}
		}
		else
		{
			if (dev->adc_stream.is_running)
			{
				xpress_reset_adc_dma(dev);
				ASSERT(13000 > 1000000/dev->adc_stream.format.cvsr*FRAME_BLOCK);
				snooze(13000);
				SPINLOCK_REGISTERS(dev)
				{
					xpress_write_reg(dev, XPRESS_ADC_CMD, XPRESS_ADC_STOP);
				}
			}
		}
	}
	//	generate PRD tables
	for (ix=0; ix<i_request->total_slots; ix++)
	{
		//	find out which stream
		bool isDAC = (i_request->info[ix].stream_cookie == &dev->dac_stream);
d2printf(("xpress: %s state %s\n", isDAC ? "DAC" : "ADC", i_request->info[ix].request->state == GAME_STREAM_RUNNING ? "RUNNING" : "STOPPED"));
		if (i_request->info[ix].request->state == GAME_STREAM_RUNNING)
		{
			status_t err = generate_prd(isDAC ? &dev->dac_stream : &dev->adc_stream,
				&i_request->info[ix].looping);
			if (err < 0)
			{
				i_request->info[ix].request->state = GAME_STREAM_STOPPED;
				ret = err;
			}
		}
	}
	//	start streams
	SPINLOCK_REGISTERS(dev)
	{
		for (ix=0; ix<i_request->total_slots; ix++)
		{
			if (i_request->info[ix].request->state == GAME_STREAM_RUNNING)
			{
				//	find out which stream
				bool isDAC = (i_request->info[ix].stream_cookie == &dev->dac_stream);
	
				(isDAC ? xpress_program_dac_dma : xpress_program_adc_dma)(dev);
			}
		}
	}
	return ret;
}

static plug_api gaplug = {

	sizeof(plug_api),				//	info_size

	NULL,							//	cookie
	"xpress",						//	short_name
	NULL,							//	vendor

	//	required
	plug_uninit,
	plug_init_codecs,
	plug_get_codec_infos,
	plug_init_stream,
	plug_run_streams,

	//	mixer API
	plug_get_mixer_description,
	plug_get_mixer_controls,
	plug_get_mixer_control_values,
	plug_set_mixer_control_values,

	//	optional driver
	NULL,							//	open
	NULL,							//	close
	NULL,							//	free
	NULL,							//	control

	//	optional API
	NULL,							//	get_info
	plug_set_codec_format,			//	set_codec_format
	plug_open_stream_buffer,		//	open_stream_buffer
	NULL,							//	bind_stream_buffer
	NULL,							//	close_stream_buffer
	NULL,							//	remove_stream
	NULL,							//	set_stream_buffer
	NULL,							//	set_stream_controls

};


static void
init_module()
{
#if DEBUG
	void * handle = load_driver_settings(gaplug.short_name);
	g_debug = atoi(get_driver_parameter(handle, "debug", "0", "1"));
	unload_driver_settings(handle);
	dprintf("xpress: debug level is %d\n", g_debug);
#endif
}

static void
uninit_module()
{
	//	yeah, whatever
}

static status_t
mod_std_ops(
	int32 code,
	...)
{
	switch (code)
	{
	case B_MODULE_INIT:
		init_module();
		break;
	case B_MODULE_UNINIT:
		uninit_module();
		break;
	default:
		eprintf(("xpress: unknown module op %ld\n", code));
		return B_DEV_INVALID_IOCTL;
	}
	return B_OK;
}


static plug_api *
plug_accept_pci(
	const pci_info * info,
	pci_module_info * module,
	const ga_utility_funcs * funcs)
{
	xpress_dev * ret;
	uint32 val;

	if (get_module(ac97_name, (module_info **) &g_ac97)) {
		eprintf(("xpress: module '%s' not found\n", ac97_name));
		g_ac97 = NULL;
		return NULL;
	}

	g_pci = module;
	g_utility = funcs;

dprintf("A\n");
	//	turn on bus mastering (if necessary)
	val = g_pci->read_pci_config(info->bus, info->device, info->function, 0x04, 2);
	if ((val & 0x6) != 0x6) {
		eprintf(("xpress: turning on bus mastering (PCI cmd was 0x%lx)\n", val));
		g_pci->write_pci_config(info->bus, info->device, info->function,
				0x04, 2, val | 0x6);
	}

	ret = (xpress_dev *)calloc(1, sizeof(xpress_dev));
	ret->pci = *info;
	ret->registers = 0;
	ret->cpu = 0;
	ret->spinlocked = 0;
	ret->gaplug = gaplug;
	//	make sure we can find our way back home!
	ret->gaplug.cookie = ret;

dprintf("B\n");
//	install_io_interrupt_handler(info->u.h0.interrupt_line, xpress_interrupt, ret, 0);

	//	set up the card's DMA base (this is skanky!)
	if (!xpress_setup(info, ret)) {
//		remove_io_interrupt_handler(info->u.h0.interrupt_line, xpress_interrupt, ret);
		free(ret);
		eprintf(("\033[31mxpress: can't setup Xpress @ 0x%lx\033[0m\n", info->u.h0.base_registers[0]));
		return NULL;
	}

	ddprintf(("xpress: returning gaplug %p (block %p)\n", &ret->gaplug, ret));

dprintf("C\n");

	return &ret->gaplug;
}

plug_module_info s_module_info = {
	{
		"media/gamedriver/pci/1078_0103/v1",
		0,
		mod_std_ops
	},
	plug_accept_pci,
	NULL
};

_EXPORT plug_module_info * modules[] = {
	&s_module_info,
	NULL
};

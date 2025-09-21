/* Copyright 2001, Be Incorporated. All Rights Reserved */

#include <PCI.h>
#include "envy24.h"

#define AC97_BAD_VALUE		(0xffff)

extern pci_module_info *pci;
extern ac97_module_info_v3 *ac97;

/* envy24_bd.c */
inline uint8  PCI_IO_RD (int);
inline void   PCI_IO_WR (int, uint8);
/* internal */
status_t envy24_init_ac97_module(envy24_dev *, uchar);
status_t envy24_uninit_ac97_module(envy24_dev *, uchar);

static status_t 	envy24_init_codec(envy24_dev *, uchar );
static void 		envy24_uninit_codec(envy24_dev *, uchar );
static uint16 		envy24_codec_read(void *, uchar );
static status_t 	envy24_codec_write(void *, uchar , uint16 , uint16 );

static void 		lock_ac97(envy24_dev *, uchar);
static void 		unlock_ac97(envy24_dev *, uchar);
 

static void lock_ac97(envy24_dev * card, uchar index)
{
	if (atomic_add(&card->ac97ben[index], 1) > 0)
		acquire_sem(card->ac97sem[index]);
}

static void unlock_ac97(envy24_dev * card, uchar index)
{
	if (atomic_add(&card->ac97ben[index], -1) > 1)
		release_sem_etc(card->ac97sem[index], 1, B_DO_NOT_RESCHEDULE);
}


status_t envy24_init_ac97_module(envy24_dev * card, uchar index)
{
	status_t err = B_OK;

	dprintf("envy24: init_ac97_module start....\n");

	card->ac97sem[index] = create_sem(1, "envy24 ac97sem");
	if ( card->ac97sem[index] < 0 ) {
		dprintf("envy24: cannot create ac97sem\n");
		return (card->ac97sem[index]);
	}
	set_sem_owner(card->ac97sem[index], B_SYSTEM_TEAM);

	/* Init codec */
	if ((err = envy24_init_codec(card, index))) 
	{
		dprintf("envy24: envy24_init_codec error!\n");
		//do this in the calling routine......
        //envy24_uninit_ac97_module(card, index);
		return err;
	}
			
	return B_OK;
}


status_t envy24_uninit_ac97_module(envy24_dev * card, uchar index)
{

	/* Uninit codec */
	envy24_uninit_codec(card, index);

	if ( card->ac97sem[index] > 0) {
		delete_sem(card->ac97sem[index]);
		card->ac97sem[index] = 0;
	}	
	/* Clean up */
	/* ???????? */
	
	return B_OK;
}


#define DEBUG_BREAK dprintf("envy24: ac97 timeout\n");

static uint16 envy24_codec_read_unsecure(envy24_dev *card, uchar offset, uchar index)
{

	uint32 BA0 = card->ctlr_iobase; 

    uint8 bStatus;
    long int lSanity = 100000L;
    uint16 wReturn = AC97_BAD_VALUE;
    uint16 wTmp;

    if (index==0)
	{
        // Read the Consumer AC97 codec.

        bStatus = 0x00;
        while ((bStatus&0x38)!=0x08) {
            bStatus = PCI_IO_RD( BA0 + ICE_AC97_CON_CMD_STATUS );
            // Loop until bit 3 is 1.
            lSanity--;
            if (lSanity<=0) {
                DEBUG_BREAK;
                return AC97_BAD_VALUE;
            }
        }
        // Write the Index.
        PCI_IO_WR(BA0 + ICE_AC97_CON_INDEX, offset);

        // Write a '1' to register 09 bit 4.
        PCI_IO_WR(BA0 + ICE_AC97_CON_CMD_STATUS, 0x10);

        // Wait for bit 4 to be zero
        bStatus = 0xFF;
        while ((bStatus&0x10)!=0x00) {
            // Loop until bit 4 is 0.
            lSanity--;
            if (lSanity<=0) {
                DEBUG_BREAK;
                return AC97_BAD_VALUE;
            }
            bStatus = PCI_IO_RD(BA0 + ICE_AC97_CON_CMD_STATUS);
        }

        // Read the data byte 1
        wReturn = PCI_IO_RD(BA0 + 0x0B) & 0xFF;

        wReturn = wReturn << 8;

        // Read the data byte 0
        wTmp = PCI_IO_RD(BA0 + 0x0A) & 0xFF;

        wReturn = wReturn | wTmp;
    }

#if 0
    else if ((index>=1) && (index<=4)) {
        // One of the professional features codecs.

        // Write the codec selection.
        OutPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG, (index-1));

        // RevA hw bug: the AC97 codec might not be ready.
        // Wait a little more.
        {
            int i;
            for (i=0; i<1000; i++) {
                InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
            }
        }

        // Waiting for the Codec to be ready.
        bStatus = 0x00;
        while ((bStatus&0x38)!=0x08) {
            bStatus = InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
            // Loop until bit 3 is 1.
            lSanity--;
            if (lSanity<=0) {
                DEBUG_BREAK;
                return(0);
            }
        }

        // RevA hw bug: the AC97 codec might not be ready.
        // Wait a little more.
        {
            int i;
            for (i=0; i<1000; i++) {
                InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
            }
        }

        // Write the Index.
        OutPort(pHwInst->wMTBase+MT_AC97_INDEX_REG, uIndex);

        // Write a '1' to register 09 bit 4 to indicate a READ.
        OutPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG, 0x10);

        // Wait for bit 4 to be zero to indicate the data is ready.
        bStatus = 0xFF;
        while ((bStatus&0x10)!=0x00) {
            // Loop until bit 4 is 0.
            lSanity--;
            if (lSanity<=0) {
                DEBUG_BREAK;
                return(0);
            }
            bStatus = InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
        }

        // RevA hw bug: the AC97 codec might not be ready.
        // Wait a little more.
        {
            int i;
            for (i=0; i<1000; i++) {
                InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
            }
        }

        // Read the data word
        wReturn = InPortw(pHwInst->wMTBase+MT_AC97_DATA_PORT_REG);
    }
#endif
    return wReturn;
}


static uint16 envy24_codec_read(void* host, uchar offset)
{
	envy24_host * ehost = (envy24_host *) host;
	envy24_dev * card   =  ehost->device ;
	uchar index			=  ehost->index;

	uint16 ret = AC97_BAD_VALUE;
	
	lock_ac97(card, index);
		ret = envy24_codec_read_unsecure(card, offset, index);
	unlock_ac97(card, index);	
	return ret;
}

static status_t envy24_codec_write(void* host, uchar offset, uint16 value, uint16 mask )
{

	envy24_host * ehost = (envy24_host *) host;
	envy24_dev * card   =  ehost->device ;
	uchar index			=  ehost->index;
	
	uint32 BA0 = card->ctlr_iobase; 
	status_t ret = B_ERROR;
    uint8 bStatus;
    long int lSanity = 100000L;

	if (mask == 0)
		return B_OK;

//dprintf("ac97: offset 0x%x value 0x%x\n",(uint32)offset, (uint32)value);
	
	lock_ac97(card, index);
		if (mask != 0xffff) {
			uint16 old  = envy24_codec_read_unsecure(card, offset, index);
			value = (value&mask)|(old&~mask);
		}

    if (index==0)
		{
	        // *** Set the Consumer AC97 codec ***
	        int i;

	        // RevA hw bug: the AC97 codec might not be ready.
	        // Wait a little more.
	        for (i=0; i<1000; i++) {
	            PCI_IO_RD(BA0 + ICE_AC97_CON_CMD_STATUS);
	        }
	
	        bStatus = 0x0;
	        while ((bStatus&0x38)!=0x08) {
	            bStatus = PCI_IO_RD(BA0 + ICE_AC97_CON_CMD_STATUS);
	            // Loop until bit 3 is 1.
	            lSanity--;
	            if (lSanity<=0) {
					dprintf("envy24: ac97 codec write timed out\n");
					unlock_ac97(card, index);
	                return B_ERROR;
	            }
	        }

	        // RevA hw bug: the AC97 codec might not be ready.
	        // Wait a little more.
	        for (i=0; i<1000; i++) {
	            PCI_IO_RD(BA0 + ICE_AC97_CON_CMD_STATUS);
	        }
	
	        // Write the Index.
	        PCI_IO_WR(BA0 + ICE_AC97_CON_INDEX, offset);
	
	        // Write the data byte 0
	        PCI_IO_WR(BA0 + 0x0A, (value&0x00FF));
	        value = value >> 8;
	        // Write the data byte 1
	        PCI_IO_WR(BA0 + 0x0B, (value&0x00FF));
	
	        // Write a '1' to register 09 bit 5.
	        PCI_IO_WR(BA0 + ICE_AC97_CON_CMD_STATUS, 0x20);
	    }
#if 0
    else if ((index>=1) && (index<=4)) {
        int i;
        
        // *** One of the professional features codecs ***

        // RevA hw bug: the AC97 codec might not be ready.
        // Wait a little more.
        for (i=0; i<1000; i++) {
            InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
        }

        bStatus = 0x0;
        while ((bStatus&0x38)!=0x08) {
            bStatus = InPort((pHwInst->wMTBase)+MT_AC97_COMM_STATUS_REG);
            // Loop until bit 3 is 1.
            lSanity--;
            if (lSanity<=0) {
                return(TRUE);
            }
        }

        // RevA hw bug: the AC97 codec might not be ready.
        // Wait a little more.
        for (i=0; i<1000; i++) {
            InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
        }

        // Write the Index.
        OutPort(pHwInst->wMTBase+MT_AC97_INDEX_REG, uIndex);

        // RevA hw bug: the AC97 codec might not be ready.
        // Wait a little more.
        for (i=0; i<1000; i++) {
            InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
        }

        // Write the data word
        OutPortw(pHwInst->wMTBase+MT_AC97_DATA_PORT_REG, wData);

        // RevA hw bug: the AC97 codec might not be ready.
        // Wait a little more.
        for (i=0; i<1000; i++) {
            InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
        }

        // Write a '1' to command register bit 5 to indicate write in progress.
        OutPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG, 0x20 | (index-1));

        // RevA hw bug: the AC97 codec might not be ready.
        // Wait a little more.
        for (i=0; i<1000; i++) {
            InPort(pHwInst->wMTBase+MT_AC97_COMM_STATUS_REG);
        }
    }
#endif
	    ret = B_OK;

	unlock_ac97(card, index);
	return ret;
}


static status_t envy24_init_codec(envy24_dev * card, uchar index)
{
	status_t err = B_OK;

	card->ehost[index].device = card;
	card->ehost[index].index  = index;
	err = (*ac97->AC97_init)(&card->codec[index], 
							(void*)&card->ehost[index],
							envy24_codec_write,
							envy24_codec_read,
							true);
	if(err!=B_OK) {
		dprintf("envy24: envy24_init_codec() err = %ld!\n",err);
	}	
	return err;				
}

static void envy24_uninit_codec(envy24_dev * card, uchar index)
{
	dprintf("envy24: envy24_uninit_codec()\n");
	(*ac97->AC97_uninit)(&card->codec[index]); 
	card->ehost[index].device = NULL;
	card->ehost[index].index  = 0;
}

#if 0
static status_t cs4280_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t err;
	err = AC97_SoundSetup(&((cs4280_hw*)card->hw)->codec, ss);
	if (err == B_OK) {
		cs4280_SetPlaybackSR(card, sr_from_constant(ss->sample_rate));
		switch (ss->playback_format) {
		case linear_8bit_unsigned_mono:
			cs4280_SetPlaybackFormat(card, 8, 1);
			break;
		case linear_8bit_unsigned_stereo:
			cs4280_SetPlaybackFormat(card, 8, 2);
			break;
		case linear_16bit_big_endian_mono:	/* REALLY MEANS HOST ENDIAN! */
		case linear_16bit_little_endian_mono:	/* REALLY MEANS HOST ENDIAN! */
			cs4280_SetPlaybackFormat(card, 16, 1);
			break;
		case linear_16bit_big_endian_stereo:	/* REALLY MEANS HOST ENDIAN! */
		case linear_16bit_little_endian_stereo:	/* REALLY MEANS HOST ENDIAN! */
			cs4280_SetPlaybackFormat(card, 16, 2);
			break;
		default:
			derror("cs4280: unknown sample format %d\n", ss->playback_format);
			cs4280_SetPlaybackFormat(card, 16, 2);
		}
	}
	return err;
}

static status_t  cs4280_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	ret = AC97_GetSoundSetup(&((cs4280_hw*)card->hw)->codec,ss);
	ss->sample_rate = constant_from_sr(((cs4280_hw*)card->hw)->SRHz);;
	switch (((cs4280_hw *)card->hw)->format) {
	default:
		derror("cs4280: unknown existing format 0x%x\n", ((cs4280_hw *)card->hw)->format);
	case 0:
		ss->playback_format = linear_16bit_big_endian_stereo;
		break;
	case PFIE_8BIT:
		ss->playback_format = linear_8bit_unsigned_stereo;
		break;
	case PFIE_MONO:
		ss->playback_format = linear_16bit_big_endian_mono;
		break;
	case PFIE_8BIT|PFIE_MONO:
		ss->playback_format = linear_8bit_unsigned_mono;
		break;
	}
	ss->capture_format = ss->playback_format;
	ddprintf(("GetSoundSetup: ss->sample_rate = %d\n",ss->sample_rate));
	return ret;
}

#endif

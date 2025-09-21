/************************************************************************/
/*                                                                      */
/*                              pcm.c 	                             	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#include <KernelExport.h>
#include "sound.h"
#include "R3MediaDefs.h"

#include "audio_card.h"
#include "device.h"
#include "debug.h"

#define PLAYBACK_HALF_BUF_LENGTH 2048
#define CAPTURE_HALF_BUF_LENGTH 2048



static void StopPcm(pcm_dev *pcm)
{
	DB_PRINTF(("StopPcm():\n"));
	
	pcm->is_runing = false; //all read and write calls will be declined after this line
							//it is safe to acquire either playback or capture semaphor
	if(B_OK != acquire_sem_etc(pcm->Capture.LockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000))
	{
		DB_PRINTF(("Error: can't get Capture.LockSem!\n"));
	}

	if (B_OK != acquire_sem_etc(pcm->Playback.LockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000))
	{
		DB_PRINTF(("Error: can't get Playback.LockSem!\n"));
	}
	atomic_add(&pcm->StopFlag,1);

	/* Stop controller */
	(*pcm->card->func->StopPcm)(pcm->card);
	DB_PRINTF(("StopPcm: done\n"));
}

static void StartPcm(pcm_dev *pcm)
{
	DB_PRINTF(("StartPcm():\n"));

	/* Start controller */
	pcm->Capture.Offset = 0;
	(*pcm->card->func->StartPcm)(pcm->card);

	atomic_add(&pcm->StopFlag,-1);
	
	pcm->is_runing = true; //allow read and write calls to be processed
	release_sem(pcm->Capture.LockSem);
	release_sem(pcm->Playback.LockSem);
	DB_PRINTF(("StartPcm(): done\n"));
}


static status_t pcm_open (const char *name, uint32 flags, void** cookie)
{	
	pcm_dev* pcm;

	DB_PRINTF(("pcm_open():\n"));

	pcm = (pcm_dev*)FindDeviceCookie(name);
	*cookie = pcm;
	if(!pcm) return ENODEV; 

	if(atomic_add(&pcm->OpenCount,1) != 0)
	{
		atomic_add(&pcm->OpenCount,-1);	
		return B_BUSY;
	}

	/* Playback section */
	{
		status_t status;
		void *addr;
		int32 size = PLAYBACK_HALF_BUF_LENGTH*2;

		if(pcm->card->func->SetPlaybackDmaBuf != NULL)
			status = (*pcm->card->func->SetPlaybackDmaBuf)(pcm->card, &size, &addr);
		else 
			status = (*pcm->card->func->SetPlaybackDmaBufOld)(pcm->card, size, &addr);
		if(status != B_OK) return status; 
		pcm->Playback.DmaBufHalfSize = size/2;

		pcm->Playback.DmaBufAddr = addr;
		(*pcm->card->func->SetPlaybackFormat)(pcm->card, 16, 2); //16 bit stereo
		(*pcm->card->func->SetPlaybackSR)(pcm->card, 44100);
	}

	/* Initialize Playback variables */
	pcm->StopFlag = 1;
	pcm->Playback.HalfBufsTwoClear = 0;
	pcm->Playback.HalfNo = 0;
	pcm->Playback.SyncFlag = 0;
	pcm->Playback.Lock = 0;
	pcm->Playback.SyncSem = create_sem(0, "Playback.SyncSem");
	if (pcm->Playback.SyncSem < 0)
	{
		DB_PRINTF(("Error: Playback.SyncSem creation error!\n"));
		DB_PRINTF(("pcm_open(): return %ld;\n",pcm->Playback.SyncSem));
		return pcm->Playback.SyncSem;
	}
	pcm->Playback.LockSem = create_sem(0, "Playback.LockSem");
	if (pcm->Playback.LockSem < 0)
	{
		DB_PRINTF(("Error: Playback.LockSem creation error!\n"));
		DB_PRINTF(("pcm_open(): return %ld;\n",pcm->Playback.LockSem));
		return pcm->Playback.LockSem;
	}

	/* Capture section */
	{
		status_t status;
		void *addr;
		int32 size = CAPTURE_HALF_BUF_LENGTH*2;

		if(pcm->card->func->SetCaptureDmaBuf != NULL)
			status = (*pcm->card->func->SetCaptureDmaBuf)(pcm->card, &size, &addr);
		else 
			status = (*pcm->card->func->SetCaptureDmaBufOld)(pcm->card, size, &addr);
		if(status != B_OK) return status;
		pcm->Capture.DmaBufHalfSize = size/2;

		pcm->Capture.DmaBufAddr = addr;
		(*pcm->card->func->SetCaptureFormat)(pcm->card, 16, 2); //16 bit stereo
		(*pcm->card->func->SetCaptureSR)(pcm->card, 44100);
	}
	pcm->Capture.HalfNo = 0;
	pcm->Capture.SyncFlag = 0;
	pcm->Capture.Lock = 0;
	pcm->Capture.SyncSem = create_sem(0, "Capture.SyncSem");
	if (pcm->Capture.SyncSem < 0)
	{
		DB_PRINTF(("Error: Capture.SyncSem creation error!\n"));
		DB_PRINTF(("pcm_open(): return %ld;\n",pcm->Capture.SyncSem));
		return pcm->Capture.SyncSem;
	}
	pcm->Capture.LockSem = create_sem(0, "Capture.LockSem");
	if (pcm->Capture.LockSem < 0)
	{
		DB_PRINTF(("Error: Capture.LockSem creation error!\n"));
		DB_PRINTF(("pcm_open(): return %ld;\n",pcm->Capture.LockSem));
		return pcm->Capture.LockSem;
	}

	StartPcm(pcm);
	DB_PRINTF(("pcm_open(): return B_OK\n"));
	return B_OK;
}


static status_t pcm_close (void *cookie)
{
	pcm_dev *pcm = (pcm_dev *) cookie;
	if((atomic_add(&pcm->OpenCount,-1) - 1) != 0) return B_ERROR;
	return B_OK;
}


/* -----
	pcm_free - called after the  close returned and 
	all i/o is complete.
----- */
static status_t pcm_free (void* cookie)
{
	pcm_dev *pcm = (pcm_dev *) cookie;
	DB_PRINTF(("pcm_free()\n"));

	(*pcm->card->func->StopPcm)(pcm->card);
	
	delete_sem(pcm->Playback.SyncSem);
	delete_sem(pcm->Playback.LockSem);
	delete_sem(pcm->Capture.SyncSem);
	delete_sem(pcm->Capture.LockSem);
	DB_PRINTF(("pcm_free(): return B_OK;\n"));
	return B_OK;
}


/* ----------
	pcm_read - handle read() calls
----- */

static status_t pcm_read (void* cookie, off_t position, void* buf, size_t* num_bytes)
{
	
	*num_bytes = 0;
	return B_IO_ERROR;
}

/* ----------
	pcm_write - handle write() calls
----- */

static status_t pcm_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_OK; 
}


static status_t playback_sound (pcm_dev *pcm, off_t position, const void* buffer, size_t* num_bytes)
{
	int halfNo;
	const void* src = buffer;
	int err = B_OK;
	size_t remain = *num_bytes;
	
	DB_PRINTF2(("playback_sound(): num_bytes = %ld\n",*num_bytes));
	err = acquire_sem_etc(pcm->Playback.LockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000);
	if (err != B_OK)
	{
		DB_PRINTF(("Error: can't get Playback.LockSem!\n"));
		DB_PRINTF2(("playback_sound(): return %ld\n", err));
		return err;
	}

	if(pcm->is_runing) while (remain > 0)
		{
			int32 tst;
			size_t block_size;
			if(remain > pcm->Playback.DmaBufHalfSize)
				block_size = pcm->Playback.DmaBufHalfSize;
			else block_size = remain;
			tst = atomic_add(&(pcm->Playback.SyncFlag),1);
			//ddprintf("Playback.SyncFlag %d\n",tst+1);
			err = acquire_sem_etc(pcm->Playback.SyncSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,500000);
			if (err != B_OK)
			{
				DB_PRINTF(("Error: can't get Playback.SyncSem!\n"));
				break;
			}
			{//critical section
//				cpu_status cp = disable_interrupts();
//				acquire_spinlock(&(pcm->Playback.Lock)); 
				halfNo = pcm->Playback.HalfNo;
//				release_spinlock(&(pcm->Playback.Lock));
//				restore_interrupts(cp); 
			}//end of critical section
			memcpy(pcm->Playback.DmaBufAddr + halfNo*pcm->Playback.DmaBufHalfSize, src,
					block_size);
			src += block_size;
			remain -= block_size;
		}	
	(*num_bytes) -= remain;

	release_sem(pcm->Playback.LockSem);
	DB_PRINTF2(("playback_sound(): num_bytes = %ld, return %ld\n",*num_bytes,err));
	return err;
}


static status_t capture_sound (pcm_dev *pcm, off_t position, void* buffer, size_t* num_bytes)
{
	int err = B_OK;
	int halfNo = 0;
	uchar *dst = (uchar *) buffer;
	size_t remain = *num_bytes;
	int32 tst;
	size_t offset = 0;
	size_t block_size;
	
	DB_PRINTF2(("capture_sound(): num_bytes = %ld\n",*num_bytes));

	err = acquire_sem_etc(pcm->Capture.LockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000);
	if (err != B_OK)
	{
		DB_PRINTF(("Error: can't get Capture.LockSem!\n"));
		DB_PRINTF2(("capture_sound(): return %ld\n", err));
		return err;
	}
	if(!pcm->is_runing)
	{
		DB_PRINTF2(("capture_sound(): pcm->is_runing == false, return B_OK\n"));
		return B_OK;
	}
	 
	{//critical section
		cpu_status cp = disable_interrupts();
		acquire_spinlock(&(pcm->Capture.Lock));
		offset = 0;
		if(pcm->Capture.Offset != 0)
		{
			offset = pcm->Capture.Offset;
			halfNo = pcm->Capture.HalfNo;
			if(remain < pcm->Capture.DmaBufHalfSize - offset) block_size = remain;
			else block_size = pcm->Capture.DmaBufHalfSize - offset;
			pcm->Capture.Offset += block_size;
			pcm->Capture.Offset %= pcm->Capture.DmaBufHalfSize;
		} 
		release_spinlock(&(pcm->Capture.Lock));
		restore_interrupts(cp); 
	}//end of critical section

	if(offset)
	{
		size_t block_size;
		if(remain < pcm->Capture.DmaBufHalfSize - offset) block_size = remain;
		else block_size = pcm->Capture.DmaBufHalfSize - offset;
		memcpy(dst, 
				pcm->Capture.DmaBufAddr + halfNo * pcm->Capture.DmaBufHalfSize + offset,
				block_size);
		dst += block_size;
		remain -= block_size;
	}


	while (remain > 0)
		{
			if(remain < pcm->Capture.DmaBufHalfSize) block_size = remain;
			else block_size = pcm->Capture.DmaBufHalfSize;
			tst = atomic_add(&(pcm->Capture.SyncFlag),1);
			err = acquire_sem_etc(pcm->Capture.SyncSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,500000);
			if (err != B_OK)
			{
				DB_PRINTF(("Error: can't get Capture.SyncSem!\n"));
				break;
			}
			{//critical section
				cpu_status cp = disable_interrupts();
				acquire_spinlock(&(pcm->Capture.Lock)); 
				halfNo = pcm->Capture.HalfNo;
				if(block_size < pcm->Capture.DmaBufHalfSize) //last block
					pcm->Capture.Offset = block_size; 
				release_spinlock(&(pcm->Capture.Lock));
				restore_interrupts(cp); 
			}//end of critical section
			memcpy(dst, 
					pcm->Capture.DmaBufAddr + halfNo * pcm->Capture.DmaBufHalfSize,
					block_size);
			dst += block_size;
			remain -= block_size;
		}	

	(*num_bytes) -= remain;
	release_sem(pcm->Capture.LockSem);
	DB_PRINTF2(("capture_sound(): captured = %ld, return %ld\n",*num_bytes,err));
	return err;
}


/* ----------
	pcm_control - handle ioctl calls
----- */
static status_t pcm_control(void *cookie, uint32 op, void *data, size_t len)
{
	pcm_dev *pcm = (pcm_dev *)cookie;
	status_t err = B_BAD_VALUE;
	bool configure = false;

	DB_PRINTF2(("pcm_control(): op = %ld\n",op));

	if(cookie == NULL)
	{
		DB_PRINTF(("pcm_control(): cookie == NULL, return B_BAD_VALUE;\n"));
		return B_BAD_VALUE;
	}
	switch (op) 
	{
	case SOUND_GET_PARAMS: 
		DB_PRINTF1(("pcm_control(): SOUND_GET_PARAMS\n"));
		err = B_OK;
		{
			err = pcm->card->func->GetSoundSetup(pcm->card, (sound_setup*)data);
		}
		break;
	case SOUND_SET_PARAMS: 
		DB_PRINTF1(("pcm_control(): SOUND_SET_PARAMS\n"));
		err = B_OK;
		{
			err = pcm->card->func->SoundSetup(pcm->card, (sound_setup*)data);
		}
		break;
	case SOUND_SET_PLAYBACK_COMPLETION_SEM:
		DB_PRINTF(("pcm_control(): SOUND_SET_PLAYBACK_COMPLETION_SEM\n"));
		pcm->Playback.CompletionSem = *(sem_id *)data;
		err = B_OK;
		break;
	case SOUND_SET_CAPTURE_COMPLETION_SEM:
		DB_PRINTF(("pcm_control(): SOUND_SET_CAPTURE_COMPLETION_SEM\n"));
		pcm->Capture.CompletionSem = *(sem_id *)data;
		err = B_OK;
		break;
	case SOUND_UNSAFE_WRITE:
		DB_PRINTF2(("pcm_control(): SOUND_UNSAFE_WRITE\n"));
		{
			static bigtime_t last = 0;
			audio_buffer_header * buf = (audio_buffer_header *)data;
			size_t n = buf->reserved_1-sizeof(*buf);
									
			playback_sound(pcm, 0, buf+1, &n);

			pcm->card->func->GetClocks(pcm->card, &buf->sample_clock, &buf->time);
			last = buf->time;
			err = release_sem_etc(pcm->Playback.CompletionSem, 1, B_DO_NOT_RESCHEDULE);
		} 
		break;
	case SOUND_UNSAFE_READ: 
		DB_PRINTF2(("pcm_control(): SOUND_UNSAFE_READ\n"));
		err = B_ERROR;
		{
			audio_buffer_header * buf = (audio_buffer_header *)data;
			size_t n = buf->reserved_1-sizeof(*buf);
									
			capture_sound(pcm, 0, buf+1, &n);

			pcm->card->func->GetClocks(pcm->card, &buf->sample_clock, &buf->time);

			err = release_sem(pcm->Capture.CompletionSem);
			
		}
		break;
	case SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE:
		{	
			void *addr;
			int32 size = (int32)data;

			if (size < 1024)
			  size = 1024;

			DB_PRINTF(("pcm_control(): SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE:\n"));
			DB_PRINTF(("resize from %ld to %ld\n", pcm->Capture.DmaBufHalfSize, size));
			if(size == pcm->Capture.DmaBufHalfSize) 
			{
				DB_PRINTF(("Resizing of the Capture buffer not requred.\n"));
				err = B_OK;
				break;
			}

			StopPcm(pcm);

			if(pcm->card->func->SetCaptureDmaBuf != NULL)
			{
				int32 tmp = size*2;
				err = (*pcm->card->func->SetCaptureDmaBuf)(pcm->card, &tmp, &addr);
				size = tmp/2;
			}

			err = (*pcm->card->func->SetCaptureDmaBufOld)(pcm->card, size * 2, &addr);
			if(err != B_OK) break;
			pcm->Capture.DmaBufAddr = addr;
			pcm->Capture.DmaBufHalfSize = size;

			memset(pcm->Capture.DmaBufAddr, 0, pcm->Capture.DmaBufHalfSize*2);
	
			StartPcm(pcm);
		}
		break;
	case SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE: 
		{	
			void *addr;
			int32 size = (int32)data;

			if (size < 1024)
			  size = 1024;

			DB_PRINTF(("pcm_control(): SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE:\n"));
			DB_PRINTF(("resize from %ld to %ld\n", pcm->Playback.DmaBufHalfSize, size));

			if(size == pcm->Playback.DmaBufHalfSize) 
			{
				DB_PRINTF(("Resizing of the PLAYBACK BUF not requred.\n"));
				err = B_OK;
				break;
			}

			StopPcm(pcm);
			if(pcm->card->func->SetPlaybackDmaBuf != NULL)
			{
				int32 tmp = size*2;
				err = (*pcm->card->func->SetPlaybackDmaBuf)(pcm->card, &tmp, &addr);
				size = tmp/2;
			}
			else err = (*pcm->card->func->SetPlaybackDmaBufOld)(pcm->card, size * 2, &addr);
			if(err != B_OK) break;
			pcm->Playback.DmaBufAddr = addr;
			pcm->Playback.DmaBufHalfSize = size;
			memset(pcm->Playback.DmaBufAddr, 0, pcm->Playback.DmaBufHalfSize*2);
	
			StartPcm(pcm);
		}
		break;
	case SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE:
		DB_PRINTF(("pcm_control(): SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE:\n"));
		{
			int32* pSize = (int32*)data;
			*pSize = pcm->Capture.DmaBufHalfSize;
			err = B_OK;
		}
		break;
	case SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE: 
		DB_PRINTF(("pcm_control(): SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE: \n"));
		{
			int32* pSize = (int32*)data; 
			*pSize = pcm->Playback.DmaBufHalfSize; 
			err = B_OK;
		}
		break;
	
	case SOUND_LOCK_FOR_DMA:
		DB_PRINTF(("pcm_control(): SOUND_LOCK_FOR_DMA:\n"));
		err = B_OK;
		break;
	default:
		DB_PRINTF(("pcm_control(): Unknown operation!!!\n"));
		break;
	}
	if ((err == B_OK) && configure) 
	{
/*
		cpu_status cp;
		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->card->hardware);
		err = configure_pcm(port, &config, false);
		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);
*/
	}
	if (err != B_OK)
	{
		DB_PRINTF(("pcm_control(): return %ld\n",err));	
	}
	else DB_PRINTF2(("pcm_control(): return B_OK\n")); 
	return err;
}

/* -----
	function pointers for the device hooks entry points
----- */
device_hooks pcm_hooks = {
	pcm_open, 		/* -> open entry point */
	pcm_close, 		/* -> close entry point */
	pcm_free,			/* -> free entry point */
	pcm_control, 		/* -> control entry point */
	pcm_read,			/* -> read entry point */
	pcm_write,		/* -> write entry point */
	NULL, /*device_select_hook		select;*/		/* start select */
	NULL, /*device_deselect_hook	deselect;*/		/* stop select */
	NULL, /*device_readv_hook		readv;*/		/* scatter-gather read from the device */
	NULL  /*device_writev_hook		writev;*/		/* scatter-gather write to the device */
};

/* 
	PCM playback interrupt handler 
	*/
bool 
pcm_playback_interrupt(
	sound_card_t *card, int half_no)
{
	bool ret = false; //true invokes the scheduler
	bool client_wait;
	pcm_dev* pcm = &((audio_card_t*)card->host)->pcm;
	if(pcm->StopFlag > 0)
	{
		return false;
	}

	acquire_spinlock(&(pcm->Playback.Lock)); 
	pcm->Playback.HalfNo = half_no;
	client_wait = (atomic_add(&(pcm->Playback.SyncFlag),-1) > 0); 
	if (client_wait)
		pcm->Playback.HalfBufsTwoClear = 2; //initialize 
	else 
		atomic_add(&(pcm->Playback.SyncFlag),1);//restore original value

	if(!(client_wait || pcm->Playback.HalfBufsTwoClear)) 
		goto return_FALSE;

	
	if (client_wait) {
		release_sem_etc(pcm->Playback.SyncSem, 1, B_DO_NOT_RESCHEDULE);
		goto return_TRUE;
	}
	
	if (pcm->Playback.HalfBufsTwoClear) { //nothing to playback - clean the buffer
		pcm->Playback.HalfBufsTwoClear--; 
		if (!pcm->Playback.HalfBufsTwoClear) {
                       //      clear both once we missed two...
                       memset(pcm->Playback.DmaBufAddr /* + half_no*pcm->Playback.DmaBufHalfSize */, 0,
                               pcm->Playback.DmaBufHalfSize*2);
        }
	}

return_FALSE:
	ret = false;
	goto return_ret;
	
return_TRUE:
	ret = true;
	goto return_ret;

return_ret:
	release_spinlock(&(pcm->Playback.Lock)); 
	return ret;
}


/* 
	PCM capture interrupt handler 
	*/
bool 
pcm_capture_interrupt(
	sound_card_t *card, int half_no)
{
	bool ret = false; //true invokes the scheduler
	bool client_wait;
	pcm_dev* pcm = &((audio_card_t*)card->host)->pcm;

	if(pcm->StopFlag > 0)
	{
		return false;
	}
	
	acquire_spinlock(&(pcm->Capture.Lock)); 

	pcm->Capture.HalfNo = half_no;

	client_wait = (atomic_add(&(pcm->Capture.SyncFlag),-1) > 0);

	pcm->Capture.Offset = 0;

	if(client_wait) {
		release_sem_etc(pcm->Capture.SyncSem, 1, B_DO_NOT_RESCHEDULE); 
		ret = true;
		goto return_ret;
	}
	atomic_add(&(pcm->Capture.SyncFlag),1);//restore value
	ret = false;
return_ret:
	release_spinlock(&(pcm->Capture.Lock)); 
	return ret;
}


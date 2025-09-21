/************************************************************************/
/*                                                                      */
/*                              ua100.h                              	*/
/*                                                                      */
/* copyright 2000 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include <OS.h>
#include <USB.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <R3MediaDefs.h>
#include <stdlib.h>
#include <stdio.h>

#include <sound.h>
#include "audio.h"

#define NB 2
#define CHIP_NAME "usb_speaker"
#define ID CHIP_NAME ": "

#define SAMPLE_SIZE 4
#define TS 4			// Number of 1ms chunks per buffer?
#define QS_OUT (44*SAMPLE_SIZE*TS)
#define DEV_MAX 4

#define LOCK(L,PS) {(PS) = disable_interrupts(); acquire_spinlock(&L);}
#define UNLOCK(L,PS) {release_spinlock(&L); restore_interrupts(PS);}

extern struct spkr_dev* pdev[DEV_MAX];
extern usb_module_info *usb;

typedef struct benaphore
{
  int32 atom;
  sem_id sem;
} benaphore;

typedef struct {
  sem_id completion_sem;
  int64 count;
  bigtime_t time;
} old_channel;

typedef struct {
  struct spkr_dev* pdev;
  old_channel p;
  sound_setup setup;
} old_dev;

typedef union volume_union
{
  uint32 u32;
  int16 i16[2];
} volume_union;

typedef struct volume_control
{
  int32 index;
  volume_union volume;
  volume_union min;
  volume_union max;
  uint8 mute;
} volume_control;

typedef struct iso_packet
{
  struct iso_packet*	next;
  struct iso_channel*	channel;	
  void*					buffer;
  usb_iso_packet_descriptor packet_descriptors[TS];
  uint32				status;
  size_t				buffer_size;
  bigtime_t				time;		/* time at start of buffer */
  int64					rw_count;	/* byte count at end of buffer */
} iso_packet;

typedef struct iso_channel
{
  usb_pipe		pipe;
  iso_packet*	next_to_queue;
  iso_packet*	current_rw;
  size_t		remain;
  char*			buf_ptr;
  sem_id		available_packets;
  area_id		buffer_area;
  char*			buffers;
  size_t		packet_size;
  bool			quit;
  iso_packet	iso_packets[NB];
  benaphore		lock;
  int32			time_lock;
  int64			rw_count;

  bigtime_t		current_time;
  int64			current_count;
  bigtime_t		next_time;
  bigtime_t		next_clock;
} iso_channel;

typedef struct spkr_dev {
  usb_device usbdev;
  const usb_configuration_info* conf;
  int32 n;
  int32 open_count;
  old_dev* lego_api;
  bool connected;
  bool audio_open;

  volume_control vc;
  iso_channel audio_out;
} spkr_dev;

status_t init_ben(benaphore* b, char* name);
status_t acquire_ben(benaphore* b);
void release_ben(benaphore* b);
void delete_ben(benaphore* b);
status_t lock_dev_table();
void unlock_dev_table();

void start_audio(spkr_dev* p);
void stop_audio(spkr_dev* p);

void init_params (old_dev* port);
status_t old_open(const char *name, uint32 flags, void **cookie);
status_t old_free (void *cookie);
status_t old_close(void *cookie);
status_t old_control(void *cookie, uint32 msg, void *buf, size_t size);
status_t old_read(void *cookie, off_t pos, void *buf, size_t *count);
status_t old_write(void *cookie, off_t pos, const void *buf, size_t *count);

status_t pcm_open(const char *name, uint32 flags, void **cookie);
status_t pcm_free (void *cookie);
status_t pcm_close(void *cookie);
status_t pcm_write(void *cookie, off_t pos, const void *buf, size_t *count);

status_t spkr_open(const char *name, int i);
status_t spkr_free (void *cookie);
status_t spkr_close(void *cookie);

volume_union vc_get(spkr_dev* p, uint8 request);
status_t vc_set(spkr_dev* p, float left, float right);
status_t vc_muet(spkr_dev* p, bool mute);

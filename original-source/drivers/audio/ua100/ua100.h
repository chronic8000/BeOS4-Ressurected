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

#define ENABLE_MIDI 1
#define MIDI_INTERRUPT 0

#define NB 2
#define CHIP_NAME "ua100"
#define ID CHIP_NAME ": "

#define TS 4	/* FIXME  TS has to be <=4 because of the primitive packetizing algorithm in pmc.c queue_packet_out() */
#define QS_IN (184*TS)  
#define QS_OUT (360*TS)
#define DEV_MAX 4

/* SEND_DEPTH and RECV_DEPTH *must* be powers of two */

#define CHAN_COUNT 3
#define SEND_DEPTH 128
#define RECV_DEPTH 128
#define SEND_MASK (SEND_DEPTH-1)
#define RECV_MASK (RECV_DEPTH-1)
#define XFER_BUFS  2
#define CHUNKS_PER_BUF 8

#define LOCK(L,PS) {(PS) = disable_interrupts(); acquire_spinlock(&L);}
#define UNLOCK(L,PS) {release_spinlock(&L); restore_interrupts(PS);}

extern struct ua100dev* pdev[DEV_MAX];
extern usb_module_info *usb;

typedef struct benaphore
{
  int32 atom;
  sem_id sem;
} benaphore;

typedef struct parser_state
{
  struct midi_chunk* ch;
  int32 status;
  int32 data;
  int32 sx_data;
  bool sx_sent;
} parser_state;

typedef struct midi_channel {
  struct ua100dev* dev;
  int32 open_count;

  parser_state state;
  sem_id send_count;  /* number of *empty* entries in sendq */
  sem_id recv_count;  /* number of *filled* entries in recvq */

  uint32 sendq_count;
  uint32 sendq_head;
  uint32 sendq_tail;
  uint32 recvq_count;
  uint32 recvq_head;
  uint32 recvq_tail;

  uchar sendq_bufr[SEND_DEPTH];
  uchar recvq_bufr[RECV_DEPTH];

  uint32 stat_recv;  /* packets received */
  uint32 stat_drop;  /* packets discarded */
  uint32 stat_sent;  /* packets sent */
} midi_channel;

typedef struct midi_chunk
{
  uchar head;
  uchar data[3];
} midi_chunk;

typedef struct xfer_buf
{
  struct xfer_buf* next;	/* list link                             */
  struct ua100dev* dev;		/* device that owns this buffer          */
  midi_chunk* ch;			/* actual buffer (CHUNKS_PER_BUF chunks) */
} xfer_buf;

typedef struct iso_packet
{
  struct iso_packet*	next;
  struct iso_channel*	channel;	
  void*					buffer;
  uint32				status;
  size_t				buffer_size;
  usb_iso_packet_descriptor	packet_descriptors[TS];
  bigtime_t				time;		/* time at start of buffer */
  int64					rw_count;	/* byte count at end of buffer */
} iso_packet;

typedef struct iso_channel
{
  usb_pipe		pipe;
  bool			is_in;
  bool			quit;
  iso_packet*	next_to_queue;
  iso_packet*	current_rw;
  size_t		remain;
  char*			buf_ptr;
  sem_id		available_packets;
  area_id		buffer_area;
  char*			buffers;
  size_t		packet_size;
  iso_packet	iso_packets[NB];
  benaphore		lock;
  int32			time_lock;
  int64			rw_count;

  bigtime_t		current_time;
  int64			current_count;
  bigtime_t		next_time;
  bigtime_t		next_clock;
} iso_channel;

typedef struct ua100dev {
  usb_device usbdev;
  const usb_configuration_info* conf;
  int32 n;
  int32 open_count;
  bool connected;
  bool audio_open;
  bool lego_api;
  iso_channel audio_in;
  iso_channel audio_out;

  usb_pipe midi_out;		/* usb pipe handles for IO                  */ 
  usb_pipe midi_in;
  sem_id send_lock;			/* guards send xfer_bufs & enqueuing thereof */
  sem_id recv_lock;
  int32 sendq_lock;			/* protects all sendq_{buffer,count} fields  */
  int32 recvq_lock;			/* protects all recvq_{buffer,count} fields  */
  uint32 send_count;		/* total bytes ready to send across all chan */
  xfer_buf* send_bufs;		/* lists of buffers not queue'd              */
  xfer_buf* recv_bufs;
  xfer_buf xfer_bufs[XFER_BUFS]; 
  int32 send_channel;
  uchar raw[(XFER_BUFS + 1) * (CHUNKS_PER_BUF * 4)];
  midi_channel chan[CHAN_COUNT];
} ua100dev;

status_t lock_dev_table();
void unlock_dev_table();

void start_audio(ua100dev* p);
void stop_audio(ua100dev* p);
void init_midi(ua100dev* p);
void uninit_midi(ua100dev* p);
void start_midi(ua100dev* p);
void stop_midi(ua100dev* p);
int32 parse_midi(midi_channel* chan, midi_chunk* ch);
void mixer_change(ua100dev* p, int32 channel, int32 control, int32 value);
void sysex_DT1(ua100dev* p, uint32 address, uint32 size, uint32 data);

status_t old_open(const char *name, uint32 flags, void **cookie);
status_t old_free (void *cookie);
status_t old_close(void *cookie);
status_t old_control(void *cookie, uint32 msg, void *buf, size_t size);
status_t old_read(void *cookie, off_t pos, void *buf, size_t *count);
status_t old_write(void *cookie, off_t pos, const void *buf, size_t *count);

status_t midi_open(const char *name, uint32 flags, void **cookie);
status_t midi_free (void *cookie);
status_t midi_close(void *cookie);
status_t midi_control(void *cookie, uint32 msg, void *buf, size_t size);
status_t midi_read(void *cookie, off_t pos, void *buf, size_t *count);
status_t midi_write(void *cookie, off_t pos, const void *buf, size_t *count);

status_t pcm_open(const char *name, uint32 flags, void **cookie);
status_t pcm_free (void *cookie);
status_t pcm_close(void *cookie);
status_t pcm_read(void *cookie, off_t pos, void *buf, size_t *count);
status_t pcm_write(void *cookie, off_t pos, const void *buf, size_t *count);

status_t ua100_open(const char *name, int i);
status_t ua100_free (void *cookie);
status_t ua100_close(void *cookie);

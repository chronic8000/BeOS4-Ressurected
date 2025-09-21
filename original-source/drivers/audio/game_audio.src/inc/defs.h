#ifndef __DEFS_H__
#define __DEFS_H__

#include <OS.h>
#include <KernelExport.h>
#include <stdio.h>

#include "game_audio.h"
#include "audio_drv.h"
#include "mvp4_debug.h"

typedef enum {dac, adc} flow_dir;

typedef struct {
	unsigned SGD_paused : 1;
	unsigned SGD_stopped : 1;
	unsigned SGD_active : 1;
} SGD_state;

struct _audiobuffer;
struct _audiobuffer_node;
struct _audiobuffer_queue;
struct _stream;

/* each stream keeps a list of bufferID's tied to it */
typedef struct _bufferID_node {
	int16 bufferID;
	struct _bufferID_node *next;
} bufferID_node;

/* information associated with audiobuffers */
typedef struct _audiobuffer {	/* contains fundamental info of a buffer */
	void *phys_addr;			/* where the buffer starts */
	area_id areaID;				/* the physical buffer area id */
	uint32 size;				/* number of bytes in the buffer */
	int16 bufferID;				/* a buffer id value */
	int16 streamID;				/* associated with this stream */
	int enqueue_count;			/* # of times buffer enqueued */
	struct _audiobuffer *next;	/* specifically to allow saving of buffers
								   through stacking for removal without reliquishing 
								   spinlock.  Buffer table can then be altered,
								   the spinlock released, and the buffers freed */
} audiobuffer;

/* enqueuing instances of buffers */
typedef struct _audiobuffer_node {
	audiobuffer entry;					/* associated buffer info */
	uint16 flags;						/* enqueuing options */
	struct _stream *in_stream;			/* stream node belongs to */
	struct _audiobuffer_node *next;
} audiobuffer_node;

/* each stream has it's own queue of buffer requests */
typedef struct _audiobuffer_queue {		
	audiobuffer_node *head;
	audiobuffer_node *tail;
} audiobuffer_queue;

/* information associated with streams */
typedef struct _stream {
	int16 streamID;						/* a stream id value */
	audiobuffer_queue queue;			/* the queue for the stream */
	enum game_stream_state state;		/* stopped, paused, running */
	sem_id stream_sem;					/* semaphore associated with stream */
	sem_id stream_close_sem;			/* semaphore used for blocking on close stream */
	bool pending_close;					/* needs to be closed but can't yet */
	bufferID_node *buffer_list;			/* list of buffers belonging to stream */
} stream;

/* information associated with DMA tables */
typedef struct {
	SGD_Table_t *start;						/* virtual address of start of table 
											   physical address written to hardware */
	area_id SGDTableAreaId; 				/* physical memory of the table */
	int size;       						/* how many entries are in the table? */
	unsigned char readpoint;				/* what entry is DMA currently on? */
	audiobuffer_node *nodes_in_SGD[NSLOTS];	/* keep track of nodes in SGD */
	SGD_state state;						/* stopped, paused, active? */
} SGD;

audiobuffer *audiobuffers[GAME_MAX_BUFFER_COUNT];	/* allocation table for buffers */

stream playback_stream,		/* one stream for playback */
       capture_stream;		/* another for capture */

SGD playback_SGD,				/* DMA table for playback */
    capture_SGD;				/* and for capture */

int16 buffers_of_DAC[NSLOTS],	/* keep track of bufferIDs in SGD */
      buffers_of_ADC[NSLOTS];

#endif
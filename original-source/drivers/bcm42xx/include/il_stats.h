/*
    EXTERNAL SOURCE RELEASE on 05/16/2000 - Subject to change without notice.

*/
/*
    Copyright 2000, Broadcom Corporation
    All Rights Reserved.
    
    This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
    the contents of this file may not be disclosed to third parties, copied or
    duplicated in any form, in whole or in part, without the prior written
    permission of Broadcom Corporation.
    

*/
/*
 * Copyright 1999 Epigram, Inc.
 *
 * $Id: il_stats.h,v 13.29 2000/02/17 08:13:26 nn Exp $
 *
 */
#ifndef _IL_STATS_H_
#define _IL_STATS_H_

#include "typedefs.h"
#include "proto/ethernet.h"
#include "proto/iline.h"
#if IL_STATS_QUEUE_IMPLEMENTED
#include "rts/queue.h"
#endif /* IL_STATS_QUEUE_IMPLEMENTED */
#include "bcm42xx.h"

#define IL_STATS_MAJOR_VERSION 3 /* increment for backward incompatable changes*/
#define IL_STATS_MINOR_VERSION 1 /* increment for backward compatable changes */

#define IL_STATS_XMT_FRAME_COUNT     0
#define IL_STATS_XMT_BYTE_COUNT	     1
#define IL_STATS_XMT_ERR_COUNT	     2
#define IL_STATS_RCV_FRAME_COUNT     3
#define IL_STATS_RCV_BYTE_COUNT	     4
#define IL_STATS_RCV_ERR_COUNT	     5
#define IL_STATS_XMT_CTRL_COUNT	     6
#define IL_STATS_RCV_CTRL_COUNT	     7
#define IL_STATS_XMT_HDR_ERR_COUNT   8
#define IL_STATS_RCV_HDR_ERR_COUNT   9
#define IL_STATS_XMT_BPB_ERR_COUNT  10	/* no longer used */
#define IL_STATS_RCV_BPB_ERR_COUNT  11
#define IL_STATS_HDR_CRC_ERR_COUNT  12
#define IL_STATS_PAY_CRC_ERR_COUNT  13
#define IL_STATS_CS_GAP_COUNT	    14
#define IL_STATS_CS_OVER_COUNT	    15
#define IL_STATS_SAT_COUNT	    16
#define IL_STATS_TRK_COUNT	    17
#define IL_STATS_XMT_EXCOL_COUNT    18
#define IL_STATS_RCV_RUNT_COUNT	    19
#define IL_STATS_RCV_GIANT_COUNT    20
#define IL_STATS_PCI_DE_COUNT	    21
#define IL_STATS_PCI_DA_COUNT	    22
#define IL_STATS_DPE_COUNT	    23
#define IL_STATS_RD_UFLO_COUNT	    24
#define IL_STATS_XF_UFLO_COUNT	    25
#define IL_STATS_RF_OFLO_COUNT	    26
#define IL_STATS_COLL_COUNT	    27
#define IL_STATS_COLLFRAMES_COUNT   28
#define IL_STATS_FILTERED_COUNT	    29
#define IL_STATS_RCV_MISSED_COUNT   30
#define IL_STATS_DEMODERR_COUNT	    31
#define IL_STATS_XMT_MISSED_ERR_COUNT 32
#define IL_STATS_XMT_TOOSHORT_COUNT 33
#define IL_STATS_RCV_TOOSHORT_COUNT 34
#define IL_STATS_LINKINT_COUNT	    35
#define	IL_STATS_FT_ERR_COUNT       36
#define	IL_STATS_RESET_COUNT        37
#define	IL_STATS_RCV_FRAG_COUNT     38
#define	IL_STATS_IFGV_COUNT         39
#define	IL_STATS_SIGV_COUNT         40
#define	IL_STATS_STACKBUB_COUNT     41
#define IL_STATS_CSA_COUNT	    42
#define	IL_STATS_CLS_COUNT          43
#define	IL_STATS_HDR_CV_ERR_COUNT   44
#define	IL_STATS_HDR_MAX_ERR_COUNT  45

#define	IL_STATS_XMT_LL_PRI         46
#define	IL_STATS_XMT_DFPQ_PRI       54
#define	IL_STATS_RCV_LL_PRI         62
#define	IL_STATS_RCV_DFPQ_PRI       70
#define IL_STATS_QOS_PHY_USAGE      78
#define IL_STATS_QOS_PHY_COLLS      86
#define IL_STATS_MAC_IDLE_USAGE     94

/* NCOUNTERS is last counter index plus one */
#define IL_STATS_NCOUNTERS          95

#ifdef STATS_NAMES
static char il_counter_names[] =
	"xmt total frames\0"
	"xmt total bytes\0"
	"xmt total errors\0"
	"rcv total frames\0"
	"rcv total bytes\0"
	"rcv total errors\0"
	"xmt control frames\0"
	"rcv control frames\0"
	"xmt hdr errors\0"
	"rcv hdr errors\0"
	"xmt hdr bpb errors\0"
	"rcv hdr bpb errors\0"
	"rcv hdr crc errors\0"
	"rcv payload crc errors\0"
	"cs gapped compatibility frames\0"
	"carrier sense overrun errors\0"
	"rcv saturation errors\0"
	"tracker failure errors\0"
	"xmt excessive collisions\0"
	"rcv runt\0"
	"rcv giant\0"
	"chip pci descriptor errors\0"
	"chip pci data errors\0"
	"chip descriptor protocol errors\0"
	"chip rcv descriptor underflow\0"
	"chip xmt fifo underflow\0"
	"chip rcv fifo overflow\0"
	"xmt collisions\0"
	"xmt colliding frames\0"
	"rcv frames discarded due to nomatch\0"
	"rcv frames discarded due to no resources\0"
	"rcv frames discarded due to demod error\0"
	"xmt frames discarded due to no resources\0"
	"xmt too short\0"
	"rcv too short\0"
	"xmt linkintegrity frames\0"
	"rcv frame type errors\0"
	"resets\0"
	"rcv collision fragments\0"
	"ifg violations\0"
	"signal violations\0"
	"stack bubbles\0"
        "xmt capability/status frames\0"
	"rcv carrier late shutoff errors\0"
	"rcv hdr cluster variance too high\0"
	"rcv hdr max error too high\0"
	"xmt LL pri 0\0"
	"xmt LL pri 1\0"
	"xmt LL pri 2\0"
	"xmt LL pri 3\0"
	"xmt LL pri 4\0"
	"xmt LL pri 5\0"
	"xmt LL pri 6\0"
	"xmt LL pri 7\0"
	"xmt DFPQ pri 0\0"
	"xmt DFPQ pri 1\0"
	"xmt DFPQ pri 2\0"
	"xmt DFPQ pri 3\0"
	"xmt DFPQ pri 4\0"
	"xmt DFPQ pri 5\0"
	"xmt DFPQ pri 6\0"
	"xmt DFPQ pri 7\0"
	"rcv LL pri 0\0"
	"rcv LL pri 1\0"
	"rcv LL pri 2\0"
	"rcv LL pri 3\0"
	"rcv LL pri 4\0"
	"rcv LL pri 5\0"
	"rcv LL pri 6\0"
	"rcv LL pri 7\0"
	"rcv DFPQ pri 0\0"
	"rcv DFPQ pri 1\0"
	"rcv DFPQ pri 2\0"
	"rcv DFPQ pri 3\0"
	"rcv DFPQ pri 4\0"
	"rcv DFPQ pri 5\0"
	"rcv DFPQ pri 6\0"
	"rcv DFPQ pri 7"
	"rcv usage pri 0\0"
	"rcv usage pri 1\0"
	"rcv usage pri 2\0"
	"rcv usage pri 3\0"
	"rcv usage pri 4\0"
	"rcv usage pri 5\0"
	"rcv usage pri 6\0"
	"rcv usage pri 7\0"
	"collisions pri 0\0"
	"collisions pri 1\0"
	"collisions pri 2\0"
	"collisions pri 3\0"
	"collisions pri 4\0"
	"collisions pri 5\0"
	"collisions pri 6\0"
	"collisions pri 7\0"
	"idle usage\0";

static char il_path_counter_names[] = 
	"path counter";
#endif

#define IL_STATS_INVALID_CH_EST_INDEX 0xFF
#define IL_STATS_NUM_CH_EST 5

#define STATS_MEM_SIZE(s)                               \
    (sizeof(phy_if_t) +                                 \
     COUNTER_BLOCK_SIZE((s)->num_counters) +            \
     XMT_HIST_SIZE((s)->num_xmt_hist) +                 \
     RCV_HIST_SIZE((s)->num_rcv_hist) +                 \
     CHANNEL_EST_BLOCK_SIZE((s)->num_ch_est, (s)->num_ch_est_val) + \
     PATH_BLOCK_SIZE((s)->num_paths) +                  \
     (((s)->len_err_queue == 0) ?                       \
       0 : ERR_QUEUE_SIZE((s)->len_err_queue)) +        \
     (s)->len_counter_names +                           \
     (s)->len_path_counter_names)

/* Shared memory header information.
 * Defines the first items in the phy_if shared memory. */
typedef struct {
    uint32 major_version;
    uint32 minor_version;
    uint32 seed;
    /* offsets in bytes to following parts of stats info */
    uint32 counters_offset;
    uint32 xmt_hist_offset;
    uint32 rcv_hist_offset;
    uint32 ch_est_offset;
    uint32 err_queue_offset; /* if 0, no error queue */
    uint32 path_block_offset;
    uint32 counter_names_offset;
    uint32 path_counter_names_offset;

    uchar driver_vers[32];
    uchar board_vers[32];
    uchar subsystem_vendor[16];
    uchar subsystem_id[16];
} phy_if_t;

/* sizes of variable size stats items */
typedef struct {
    uint32 num_counters;
    uint32 num_xmt_hist;
    uint32 num_rcv_hist;
    uint32 num_ch_est;
    uint32 num_ch_est_val;
    uint32 num_paths;
    uint32 num_path_counters;
    uint32 len_err_queue; /* if 0, no error queue */
    uint32 len_counter_names;
    uint32 len_path_counter_names;
} stats_sizes_t;

/* Calculate the size of a structure BASE_TYPE that
 * has a variable sized array of N ELT_TYPE elements.
 * The structure must be defined as having an array of
 * one element as a placeholder. */
#define VARIABLE_SIZER(base_type, elt_type, n) \
    (sizeof(base_type) + (n-1)*sizeof(elt_type))

typedef struct {
    uint32 num;                 /* number of uint32 counters */
    uint32 counters[1];
} counter_block_t;
#define COUNTER_BLOCK_SIZE(n) VARIABLE_SIZER(counter_block_t, uint32, n)

typedef struct {
    uint32 timestamp;           /* millisecs since boot */
    struct ether_addr dst_addr;
    struct ether_addr src_addr;
    uint16 ether_type;
    uint8  xmt_bpb;
    uint8  rcv_bpb;
    uint16 size;
    uint8  priority;
    uint8  flags;
} xmt_packet_info_t;

typedef struct {
    uint32 num;                 /* number of elts in packet array */
    uint32 size;                /* size in bytes of each elt */
    uint32 packet_index;        /* current write pointer, the info for the
                                 * next packet will be written to this elt */
    uint32 packet_lap;          /* incremented every time the
                                 * packet_index wraps */
    xmt_packet_info_t packet[1];
} xmt_history_t;
#define XMT_HIST_SIZE(n) \
        VARIABLE_SIZER(xmt_history_t, xmt_packet_info_t, n)

typedef struct {
    uint32 timestamp;           /* millisecs since boot */
    struct ether_addr dst_addr;
    struct ether_addr src_addr;
    uint16 ether_type;
    uint8  xmt_bpb;
    uint8  rcv_bpb;
    uint16 size;
    uint8  priority;
    uint8  flags;
    uint8  ch_est_index;
    bcm42xxrxhdr_t hw_rcv_hdr;
} rcv_packet_info_t;

typedef struct {
    uint32 num;                 /* number of elts in packet array */
    uint32 size;                /* size in bytes of each elt */
    uint32 packet_index;        /* current write pointer, the info for the
                                 * next packet will be written to this elt */
    uint32 packet_lap;          /* incremented every time the
                                 * packet_index wraps */
    rcv_packet_info_t packet[1];
} rcv_history_t;
#define RCV_HIST_SIZE(n) \
        VARIABLE_SIZER(rcv_history_t, rcv_packet_info_t, n)

typedef struct {
    int16 i; int16 q;
} channel_est_val_t;

typedef struct {
    uint32 timestamp;           /* millisecs since boot */
    channel_est_val_t vals[1];
} channel_est_item_t;
#define CHANNEL_EST_ITEM_SIZE(n) \
        VARIABLE_SIZER(channel_est_item_t, channel_est_val_t, n)

typedef struct {
    uint32 num;                 /* number of ch_est in the block */
    uint32 size;                /* sizeof each ch_est in bytes */
} channel_est_block_t;
#define CHANNEL_EST_BLOCK_SIZE(n, len) \
    (sizeof(channel_est_block_t) + (n)*CHANNEL_EST_ITEM_SIZE(len))

#define CHANNEL_EST_PTR(ch_block, n) \
    (channel_est_item_t*)((uint)(ch_block) + sizeof(channel_est_block_t) + \
                          (n)*((ch_block)->size))

typedef struct {
    struct ether_addr id;
    uint8  _pad0[2];
    uint32 last_active;         /* millisecs since boot of last rcv/xmt */
    uint32 packets_xmt;
    uint32 packets_rcv;
    uint32 bcst_rcv;
    /* The following counts are sums of the various values for each packet.
     * For example, the average xmt packet size is bytes_xmt/packets_xmt. */
    uint32 bytes_xmt;
    uint32 bytes_rcv;
    uint32 bpb_xmt_acc;         /* total of xmt bpb vals for each xmt packet */
    uint32 bpb_rcv_acc;         /* total of xmt bpb vals for each rcv packet */
    uint32 pay_syms;
    uint32 hdr_sum_errsq_hi;
    uint32 hdr_sum_errsq_lo;
    uint32 hdr_max_errsq;
    uint32 pay_sum_errsq_hi;
    uint32 pay_sum_errsq_lo;
    uint32 pay_max_errsq;
} phy_stats_path_info_t;

#define	ADD64(hi, lo, v) {(lo) += (v); if ((lo) < (v)) (hi)++; }

typedef struct {
    uint32 num;                 /* number of elts in the path_info array */
    uint32 size;                /* size in bytes of each elt */
    phy_stats_path_info_t path_info[1];
} phy_stats_path_block_t;
#define PATH_BLOCK_SIZE(n) \
        VARIABLE_SIZER(phy_stats_path_block_t, phy_stats_path_info_t, n)

typedef struct {
    uint32 timestamp;           /* millisecs since boot */
    uint16 sequence_number;     /* count of error packets that the provider
                                 * has attempted to write to the err queue */
    uint8  error_code;
    int8   gain_delta;
} err_packet_info_t;

#if IL_STATS_QUEUE_IMPLEMENTED
#define ERR_QUEUE_SIZE(num_bytes) (sizeof(queue_t) + num_bytes)
#else
#define ERR_QUEUE_SIZE(num_bytes) (0)
#endif /* IL_STATS_QUEUE_IMPLEMENTED */

/* pointers to the parts of shared mem stats */
typedef struct {
    stats_sizes_t sizes;
    phy_if_t* phy_if;
    counter_block_t* counter_block;
    xmt_history_t* xmt;
    rcv_history_t* rcv;
    channel_est_block_t* ch_est;
    phy_stats_path_block_t* path_block;
#if IL_STATS_QUEUE_IMPLEMENTED
    queue_t* err_queue;
#else
    void* err_queue;
#endif /* IL_STATS_QUEUE_IMPLEMENTED */
    char* counter_names;
    char* path_counter_names;
    uint32 err_sequence_number;
} stats_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void
il_stats_init(
    stats_state_t* state,
    void* mem,
    uint size,
    char* counter_names,
    char* path_counter_names
);

void
il_stats_update_xmt(
    stats_state_t* state,
    phy_stats_path_info_t* path_info,
    uint32 packet_timestamp,
    uchar *src,
    uchar *dst,
    uint bpb,
    uint pri,
    uint size,
    uint16 ethertype,
    uchar subtype
);

void
il_stats_update_rcv(
    stats_state_t* state,
    phy_stats_path_info_t* path_info,
    uint32 packet_timestamp,
    uchar *src,
    uchar *dst,
    uint bpb,
    uint pri,
    uint size,
    uint16 ethertype,
    uchar subtype,
    bcm42xxrxhdr_t* hw_hdr,
    uint ch_est
);

void
il_stats_queue_error_packet(
    stats_state_t* state,
    uint32 packet_timestamp,
    uint8  rcv_status,
    int8   gain_delta,
    uint8* pbuf,
    uint32 buf_len
);

#ifdef __cplusplus
}
#endif
#endif /* _IL_STATS_H_ */

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
 * Common [OS-independent] stats portion of
 * Broadcom BCM42XX PCI iLine10(TM) Device Driver.
 *
 * Copyright(c) 1999 Broadcom, Inc.
 * $Id: il_stats.c,v 13.37 2000/05/03 03:53:32 csw Exp $
 */

#include <typedefs.h>
#include <il_stats.h>
#include <il_dbg.h>
#include <proto/ethernet.h>
#include <proto/ilcp.h>

#define MAX_STR_LEN 256

#ifdef DBG
static uint string_block_length(char* s, uint num_strings);
#endif
static void xmt_history_init(xmt_history_t* phist, uint num);
static void rcv_history_init(rcv_history_t* phist, uint num);

void
il_stats_init(
    stats_state_t* state,
    void* mem_block,
    uint block_size,
    char* counter_names,
    char* path_counter_names
)
{
    phy_if_t* phy_if = (phy_if_t*)mem_block;
    stats_sizes_t* part_sizes = &state->sizes;
    uint item_size;
    int8* cur_ptr;
    uint32 cur_offset;
    
    state->phy_if = phy_if;
    
    /* bail if the stats mem_block is not big enough to set
     * the header info and offsets. */
    ASSERT(block_size >= sizeof(phy_if_t));

    bzero(mem_block, block_size);

    /* Initialize the header info */

    phy_if->major_version = IL_STATS_MAJOR_VERSION;
    phy_if->minor_version = IL_STATS_MINOR_VERSION;
    phy_if->seed = 0;

    /* Initialize offsets in the phy_if_t header struct
     * and the pointers in the stats_state_t struct.
     */

    cur_offset = 0;
    cur_ptr = (int8*)phy_if;

    /* skip over the phy_if_t header structure */
    item_size = sizeof(phy_if_t);
    cur_offset += item_size; cur_ptr += item_size;

    phy_if->counters_offset = cur_offset;
    state->counter_block = (counter_block_t*)cur_ptr;
    item_size = COUNTER_BLOCK_SIZE(part_sizes->num_counters);
    cur_offset += item_size; cur_ptr += item_size;

    phy_if->xmt_hist_offset = cur_offset;
    state->xmt = (xmt_history_t*)cur_ptr;
    item_size = XMT_HIST_SIZE(part_sizes->num_xmt_hist);
    cur_offset += item_size; cur_ptr += item_size;

    phy_if->rcv_hist_offset = cur_offset;
    state->rcv = (rcv_history_t*)cur_ptr;
    item_size = RCV_HIST_SIZE(part_sizes->num_rcv_hist);
    cur_offset += item_size; cur_ptr += item_size;

    phy_if->ch_est_offset = cur_offset;
    state->ch_est = (channel_est_block_t*)cur_ptr;
    item_size = CHANNEL_EST_BLOCK_SIZE(part_sizes->num_ch_est,
                                       part_sizes->num_ch_est_val);
    cur_offset += item_size; cur_ptr += item_size;

    phy_if->path_block_offset = cur_offset;
    state->path_block = (phy_stats_path_block_t*)cur_ptr;
    item_size = PATH_BLOCK_SIZE(part_sizes->num_paths);
    cur_offset += item_size; cur_ptr += item_size;
#if IL_STATS_QUEUE_IMPLEMENTED
    if (part_sizes->len_err_queue > 0) {
        phy_if->err_queue_offset = cur_offset;
        state->err_queue = (queue_t*)cur_ptr;
        item_size = ERR_QUEUE_SIZE(part_sizes->len_err_queue);
        cur_offset += item_size; cur_ptr += item_size;
    }
#endif /* IL_STATS_QUEUE_IMPLEMENTED */
    phy_if->counter_names_offset = cur_offset;
    state->counter_names = (char*)cur_ptr;
    item_size = part_sizes->len_counter_names;
    cur_offset += item_size; cur_ptr += item_size;
    
    phy_if->path_counter_names_offset = cur_offset;
    state->path_counter_names = (char*)cur_ptr;
    item_size = part_sizes->len_path_counter_names;
    cur_offset += item_size; cur_ptr += item_size;

    ASSERT(part_sizes->len_counter_names ==
           string_block_length(counter_names, part_sizes->num_counters));
    
    ASSERT(part_sizes->len_path_counter_names ==
           string_block_length(path_counter_names, part_sizes->num_path_counters));

    /* initialize the various phy_if parts */
    state->counter_block->num = part_sizes->num_counters;
    xmt_history_init(state->xmt, part_sizes->num_xmt_hist);
    rcv_history_init(state->rcv, part_sizes->num_rcv_hist);
    state->ch_est->num = part_sizes->num_ch_est;
    state->ch_est->size = CHANNEL_EST_ITEM_SIZE(part_sizes->num_ch_est_val);
    state->path_block->num = 0; /* Start by advertizing 0 paths, increment as
                                 * new path entries become valid. */
    state->path_block->size = sizeof(phy_stats_path_info_t);
#if IL_STATS_QUEUE_IMPLEMENTED
    if (state->err_queue != NULL) {
        /* the error queue buffer starts immediately after
         * the queue_t structure */
        void* buf = (state->err_queue + 1);
        queue_set(state->err_queue, part_sizes->len_err_queue, buf);
    }
#endif /* IL_STATS_QUEUE_IMPLEMENTED */
	bcopy(counter_names, state->counter_names, part_sizes->len_counter_names);
	bcopy(path_counter_names, state->path_counter_names, part_sizes->len_path_counter_names);

	state->err_sequence_number = 0;
}

static void
xmt_history_init(xmt_history_t* phist, uint num)
{
    phist->num          = num;
    phist->size         = sizeof(xmt_packet_info_t);
    phist->packet_index = 0;
    phist->packet_lap   = 0;
}

static void
rcv_history_init(rcv_history_t* phist, uint num)
{
    phist->num          = num;
    phist->size         = sizeof(rcv_packet_info_t);
    phist->packet_index = 0;
    phist->packet_lap   = 0;
}

#ifdef DBG
static uint
string_block_length(char* s, uint num_strings)
{
    char* end_ptr = s;
    uint str_len = 0;
    
    while (num_strings-- > 0) {
        str_len = 0;
        while (*end_ptr++ != '\0' && str_len++ < MAX_STR_LEN)
            ;
        ASSERT(str_len < MAX_STR_LEN);
    }

    return ((uint)end_ptr - (uint)s);
}
#endif

void
il_stats_update_xmt(
    stats_state_t* stats,
    phy_stats_path_info_t* path_info,
    uint32 packet_timestamp,
    uchar *src,
    uchar *dst,
    uint bpb,
    uint pri,
    uint size,
    uint16 ethertype,
    uchar subtype
)
{
    stats_sizes_t* sizes = &stats->sizes;
    xmt_packet_info_t* xmt_info;
    xmt_history_t* xmt_hist;
    uint32 index;
    uint32* counters;
    
    if (stats == NULL)
        return;

    /* update global counters */
    counters = stats->counter_block->counters;
    counters[IL_STATS_XMT_FRAME_COUNT]++;
    counters[IL_STATS_XMT_BYTE_COUNT] += size;
    if ((ethertype == ETHERTYPE_ILCP) && (subtype != ILCP_SUBTYPE_LARQ))
        counters[IL_STATS_XMT_CTRL_COUNT]++;

    /* update per-path counters */
    if (path_info != NULL) {
        path_info->packets_xmt++;
        path_info->bytes_xmt += size;
        path_info->bpb_xmt_acc += bpb;
        path_info->last_active = packet_timestamp;
    }

    /* update the xmt packet trace */
    xmt_hist = stats->xmt;
    index = xmt_hist->packet_index;
    xmt_info = &xmt_hist->packet[index];

    xmt_info->timestamp     = packet_timestamp;
    bcopy((uchar*) src, (uchar*) &xmt_info->src_addr, ETHER_ADDR_LEN);
    bcopy((uchar*) dst, (uchar*) &xmt_info->dst_addr, ETHER_ADDR_LEN);
    xmt_info->ether_type    = (uint16) ethertype;
    xmt_info->xmt_bpb       = (uint8) bpb;
    xmt_info->rcv_bpb       = (uint8) 0;
    xmt_info->size          = (uint16)size;
    xmt_info->priority      = (uint8)pri;
    xmt_info->flags         = 0;

    index++;
    if (index < sizes->num_xmt_hist) {
        xmt_hist->packet_index = index;
    } else {
        xmt_hist->packet_index = 0;
        xmt_hist->packet_lap++;
    }
    
    return;
}

void
il_stats_update_rcv(
    stats_state_t* stats,
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
)
{
    stats_sizes_t* sizes = &stats->sizes;
    rcv_packet_info_t* rcv_info;
    rcv_history_t* rcv_hist;
    uint32 index;
    uint32* counters;

    if (stats == NULL)
        return;

    /* update global counters */

    counters = stats->counter_block->counters;
    counters[IL_STATS_RCV_FRAME_COUNT]++;
    counters[IL_STATS_RCV_BYTE_COUNT] += size;
    if ((ethertype == ETHERTYPE_ILCP) && (subtype != ILCP_SUBTYPE_LARQ))
        counters[IL_STATS_RCV_CTRL_COUNT]++;

    if (FRAMELEN(hw_hdr) > ILINE_MAX_LEN) {
        counters[IL_STATS_RCV_GIANT_COUNT]++;
        counters[IL_STATS_RCV_ERR_COUNT]++;
    }

    if (PCRC(hw_hdr)) {
        if (FRAMELEN(hw_hdr) < ETHER_MIN_LEN)
            counters[IL_STATS_RCV_FRAG_COUNT]++;
        else {
            counters[IL_STATS_PAY_CRC_ERR_COUNT]++;
	    counters[IL_STATS_RCV_ERR_COUNT]++;
	}
    }
    
    if (RTERR(hw_hdr)) {
        counters[IL_STATS_RCV_BPB_ERR_COUNT]++;
        counters[IL_STATS_RCV_ERR_COUNT]++;
    }

    if (FTERR(hw_hdr)) {
        counters[IL_STATS_FT_ERR_COUNT]++;
        counters[IL_STATS_RCV_ERR_COUNT]++;
    }

    if (HCRC(hw_hdr)) {
        counters[IL_STATS_HDR_CRC_ERR_COUNT]++;
        counters[IL_STATS_RCV_ERR_COUNT]++;
    }

    if (TRKERR(hw_hdr))
        counters[IL_STATS_TRK_COUNT]++;

    if (SATDET(hw_hdr))
        counters[IL_STATS_SAT_COUNT]++;

    if (CSOVER(hw_hdr))
        counters[IL_STATS_CS_OVER_COUNT]++;

    if ((DATATYPE(hw_hdr) == DT_ILINE) && COMPAT(hw_hdr))
        counters[IL_STATS_CS_GAP_COUNT]++;

    /* update per-path counters */
    /* only record per-path stats if the received packet was OK.
     * If the packet had a bad CRC, then we can't
     * be certain that the source address is correct */
    if (!PCRC(hw_hdr) && path_info != NULL) {
        path_info->packets_rcv++;
        path_info->bytes_rcv += size;
        /* bpb is the wire rate this packet was sent to us */
        path_info->bpb_rcv_acc += bpb;
        path_info->last_active = packet_timestamp;

        if (ETHER_ISBCAST(dst))
            path_info->bcst_rcv++;

        path_info->pay_syms        += hw_hdr->symcount;
        path_info->hdr_sum_errsq_hi = 0;
        path_info->hdr_sum_errsq_lo = 0;
        path_info->hdr_max_errsq    = 0;
        path_info->pay_sum_errsq_hi = 0;
        path_info->pay_sum_errsq_lo = 0;
        path_info->pay_max_errsq    = 0;
    }

    /* update the rcv packet trace */
    rcv_hist = stats->rcv;
    index = rcv_hist->packet_index;
    rcv_info = &rcv_hist->packet[index];

    rcv_info->timestamp     = packet_timestamp;
    bcopy(src, (uchar*) &rcv_info->src_addr, ETHER_ADDR_LEN);
    bcopy(dst, (uchar*) &rcv_info->dst_addr, ETHER_ADDR_LEN);
    rcv_info->ether_type    = (uint16)ethertype;
    rcv_info->xmt_bpb       = (uint8)bpb;
    rcv_info->rcv_bpb       = (uint8)0;
    rcv_info->size          = (uint16)size;
    rcv_info->priority      = (uint8)pri;
    rcv_info->flags         = 0;
    rcv_info->ch_est_index  = (uint8)ch_est;
    rcv_info->hw_rcv_hdr    = *hw_hdr;

    index++;
    if (index < sizes->num_rcv_hist) {
        rcv_hist->packet_index = index;
    } else {
        rcv_hist->packet_index = 0;
        rcv_hist->packet_lap++;
    }
    
    return;
}

#if IL_STATS_QUEUE_IMPLEMENTED
void
il_stats_queue_error_packet(
    stats_state_t* stats,
    uint32 packet_timestamp,
    uint8  rcv_status,
    int8   gain_delta,
    uint8* pbuf,
    uint32 buf_len
)
{
    uint8 err_info_cdu[sizeof(cdu_header_t) + sizeof(err_packet_info_t)];
    err_packet_info_t* perror_info;
    cdu_header_t cdu_header;
    uint nbytes_free;
    queue_t queue_copy;
    phy_if_t *phy_if_stats = phy_if_control->phy_if_stats;
    
    /* is there room in the error queue? */
    nbytes_free = queue_empty_bytes(&phy_if_stats->err_queue);

    if (nbytes_free < sizeof(err_info_cdu) + buf_len) {
        return;
    }

    /* save a cdu containing the error info struct */
    memset(err_info_cdu, 0, sizeof(err_info_cdu));

    cdu_header = CDU_SYNC | CDU_SOF | CDU_EOF;
    SET_CDU_LENGTH(cdu_header, (uint16)sizeof(err_packet_info_t));
    STORE_CDU_HEADER(err_info_cdu, cdu_header);

    perror_info = (err_packet_info_t*)(err_info_cdu + sizeof(cdu_header_t));
    perror_info->timestamp = packet_timestamp;
    perror_info->sequence_number = phy_if_control->err_sequence_number++;
    perror_info->error_code = rcv_status;
    perror_info->gain_delta = gain_delta;
    
    /* make a copy of the shared mem queue so that we can do the following
     * two puts atomically. */
    queue_init_copy(&queue_copy, &phy_if_stats->err_queue,
                    phy_if_stats->err_queue.pbuffer);
    
    /* save the error info */
    queue_put(&queue_copy, sizeof(err_info_cdu), err_info_cdu);
    
    /* save the sample CDUs */
    queue_put(&queue_copy, buf_len, pbuf);

    /* update the shared mem queue */
    queue_flush_put(&phy_if_stats->err_queue, &queue_copy);
    
    /* notify the listener that some info is available */
    epi_event_set(phy_if_control->sync_objects.err_notify);
}
#endif /* IL_STATS_QUEUE_IMPLEMENTED */


/*
 * ether_basics.h
 * Copyright (c) 2000 Be, Inc.	All Rights Reserved 
 * written by Peter Folk <pfolk@be.com>
 *
 *
 * Ethernet driver fundamentals (common to most/all PCI devices)
 *
 * To use this, put the following in your driver's main file, foo.c:
 *
 *   #define kDevName "foo"   // the device name
 *   // devices will appear as /dev/net/foo/...
 *
 *   // PCI vendor and device ID's
 *   #define VENDOR_ID  0xDEAD
 *   #define DEVICE_ID  0xBEEF
 *
 *   #define DEBUG             // if you want debug output
 *   #define MAX_DEVICES 4     // instances of this driver, max
 *
 *   #define TINY_RING_BUFFERS_FOR_TESTING
 *   // ring buffer sizes (all must be powers of two)
 *   // - tune for optimal performance
 *   #if TINY_RING_BUFFERS_FOR_TESTING
 *   #define TX_BUFFERS             4L
 *   #define RX_BUFFERS             4L
 *   #else
 *   #define TX_BUFFERS             128L
 *   #define RX_BUFFERS             128L
 *   #endif
 *
 *   struct {...} dev_info_t;  // your per-device globals
 *   #include <ether_basics.h>
 *
 *   // Then your versions of...
 *   status_t open_hook(const char *name, uint32 flags, void **cookie);
 *   status_t close_hook(void *cookie);
 *   status_t free_hook(void *cookie);
 *   status_t control_hook(void * cookie, uint32 msg, void *buf, size_t len);
 *   status_t read_hook(void *cookie, off_t pos, void *buf, size_t *len);
 *   status_t write_hook(void *cookie, off_t pos, const void *buf, size_t *len);
 *   // where _device will be the appropriate dev_info_t*.
 *
 * Easy as could be, eh?
 */
#ifndef _ETHER_BASICS_H
#define _ETHER_BASICS_H

#define kDevDir "net"
#include "common/driver_basics.h"
#include "common/ether_driver.h"

#define BUFFER_SIZE            2048L	/* B_PAGE_SIZE divided into even amounts that will hold a 1518 frame */
#define MAX_FRAME_SIZE         1518		/* 1514 + 4 bytes checksum added by card hardware */
#define ETH_CRC_LEN            4
#define MIN_FRAME_SIZE         60

#define TX_MASK               (TX_BUFFERS - 1)
#define RX_MASK               (RX_BUFFERS - 1)
#endif /* _ETHER_BASICS_H */

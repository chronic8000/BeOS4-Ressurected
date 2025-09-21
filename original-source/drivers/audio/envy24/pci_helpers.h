/* Copyright 2001, Be Incorporated. All Rights Reserved */
#ifndef _PCI_HELPERS_H_
#define _PCI_HELPERS_H_

#include <SupportDefs.h>

inline uint32 PCI_CFG_RD (uchar, uchar, uchar, uchar, uchar);
inline void   PCI_CFG_WR (uchar, uchar, uchar, uchar, uchar, uint32);
inline uint8  PCI_IO_RD (int);
inline uint16 PCI_IO_RD_16 (int);
inline uint32 PCI_IO_RD_32 (int);
inline void   PCI_IO_WR (int, uint8);
inline void   PCI_IO_WR_16 (int, uint16);
inline void   PCI_IO_WR_32 (int, uint32);

#endif _PCI_HELPERS_H_

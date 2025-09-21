
#ifndef RENDERDEFS_H
#define RENDERDEFS_H

#include <support2/ByteOrder.h>
#include "enums.h"

#define LINEFLAG_DRAWLINE 	0x01
#define LINEFLAG_DRAWPOINT	0x02
#define LINEFLAG_XISNEG		0x04
#define LINEFLAG_YISNEG		0x08
#define LINEFLAG_XMAJOR		0x10

enum {
	LENDIAN=0,
	BENDIAN,
	NUM_ENDIANESSES /* duh! */
};

#include <endian.h>

#if B_HOST_IS_LENDIAN
#define HostEndianess LITTLE_ENDIAN
#define NonHostEndianess BIG_ENDIAN
#define HOST_ENDIANESS LENDIAN
#else
#define HostEndianess BIG_ENDIAN
#define NonHostEndianess LITTLE_ENDIAN
#define HOST_ENDIANESS BENDIAN
#endif

#if (HostEndianess == BIG_ENDIAN)
#define ShiftToByte0(a) 	((a)<<24)
#define ShiftToByte1(a) 	((a)<<16)
#define ShiftToByte2(a) 	((a)<<8)
#define ShiftToByte3(a) 	(a)
#define ShiftToWord0(a) 	((a)<<16)
#define ShiftToWord1(a) 	(a)
#define ShiftMaskByte0(a)	((a)>>24)
#define ShiftMaskByte1(a)	(((a)>>16)&0xFF)
#define ShiftMaskByte2(a)	(((a)>>8)&0xFF)
#define ShiftMaskByte3(a)	((a)&0xFF)
#define ShiftMaskWord0(a)	((a)>>16)
#define ShiftMaskWord1(a)	((a)&0xFFFF)
#else
#define ShiftToByte0(a)		(a)
#define ShiftToByte1(a)		((a)<<8)
#define ShiftToByte2(a)		((a)<<16)
#define ShiftToByte3(a)		((a)<<24)
#define ShiftToWord0(a)		(a)
#define ShiftToWord1(a)		((a)<<16)
#define ShiftMaskByte3(a)	((a)>>24)
#define ShiftMaskByte2(a)	(((a)>>16)&0xFF)
#define ShiftMaskByte1(a)	(((a)>>8)&0xFF)
#define ShiftMaskByte0(a)	((a)&0xFF)
#define ShiftMaskWord1(a)	((a)>>16)
#define ShiftMaskWord0(a)	((a)&0xFFFF)
#endif

#define TRANSPARENT_MAGIC_8BIT				0xFF
#define TRANSPARENT_MAGIC_15BIT				0x39CE
#define TRANSPARENT_MAGIC_15BIT_SWAPPED		0xCE39
#define TRANSPARENT_MAGIC_32BIT				0x00777477
#define TRANSPARENT_MAGIC_32BIT_SWAPPED		0x77747700
#define TRANSPARENT_ARGB					0x00777477
#if (HostEndianess == BIG_ENDIAN)
  #define TRANSPARENT_RGBA					0x77747700
#else
  #define TRANSPARENT_RGBA					0x00777477
#endif

#define DEFINE_OP_COPY						0
#define DEFINE_OP_OVER						1
#define DEFINE_OP_ERASE						2
#define DEFINE_OP_INVERT					3
#define DEFINE_OP_BLEND						4
#define DEFINE_OP_COMPOSITE_ALPHA			5
#define DEFINE_OP_OTHER						6
#define DEFINE_OP_FUNCTION					7

#define	SOURCE_ALPHA_PIXEL					0
#define	SOURCE_ALPHA_CONSTANT				1

#define	ALPHA_FUNCTION_OVERLAY				0
#define	ALPHA_FUNCTION_COMPOSITE			1

#define TOP_TO_BOTTOM 0x01
#define LEFT_TO_RIGHT 0x02

#endif

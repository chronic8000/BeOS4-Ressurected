#ifndef DOCH_IOCTL_H
#define DOCH_IOCTL_H

#include <Drivers.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DOCH_DEVICE_ROOT_NAME "disk/doch/"

enum
{
	DOCH_copyNFDC21Vars = B_DEVICE_OP_CODES_END+1,
	DOCH_copyAnandRec,
	DOCH_copyANANDPhysUnits,
	DOCH_bdCall
};

typedef struct
{
	FLFunctionNo	functionNo;
	IOreq			ioreq;
} DOCH_bdCall_buffer;

#ifdef __cplusplus
}
#endif

#endif


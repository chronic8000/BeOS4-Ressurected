
#ifndef CDMMCDRIVER_H
#define CDMMCDRIVER_H

#include "DataPump.h"
#include "CDDriver.h"

class CDMMCDriver;

class DataPumpMMC : public DataPump
{
public:
	DataPumpMMC(CDMMCDriver *drv,SCSIDevice *device);
	virtual ~DataPumpMMC();
	void Burn(CDTrack *first);
	virtual status_t Fill(DataBuffer *db);
	virtual status_t Empty(DataBuffer *db);	
	void NewTrack();
	
private:
	SCSIDevice *dev;
	CDMMCDriver *driver;
	CDTrack *track;
	uint32 track_no;
	uint32 frame, blocks, blocksize;
	uint32 step;
	int32 LBA;
};

class CDMMCDriver : public CDDriver
{
friend class DataPumpMMC;
	
public:
	CDMMCDriver(void);
	virtual ~CDMMCDriver();

	virtual const char *Name(void);
	virtual const char *DeviceName(void);
	virtual const char *DevicePath(void);
	
	virtual bool IsSupportedDevice(SCSIDevice *dev);
	virtual bool IsBlankDisc();
	
	virtual bool Burning(void);
	virtual float PercentFifo(void);
	virtual float PercentDone(void);

	virtual CDDriver *GetDriverInstance(SCSIDevice *dev, BLooper *looper);
	
	/* these can only be used by the driver instance */
	virtual status_t Check(CDTrack *tracks);
	virtual status_t Start(CDTrack *tracks);
	virtual status_t Abort(void);
	virtual char *GetError(void);
	
	// XXX: this is not ever defined anywhere, and has no type
	//	Status(status_t status, char *msg, ...);
	
protected:
	void Error(const char *error, bool scsi_info = false);
	
	SCSIDevice *dev;
	BLooper *looper;
	char errmsg[256];
	char devname[64];
	DataPumpMMC *datapump;
	
	thread_id reader_thread;
	thread_id writer_thread;
};

#endif


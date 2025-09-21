#include <stdio.h>
#include "CDMMCDriver.h"
#include "SCSIDevice.h"
#include "CDTrack.h"
#include "DataPump.h"
#include "CDCueSheet.h"
#include "Pages.h"

#define REALBURN 1

status_t CloseSession(SCSIDevice *dev)
{
	uchar cmd[10] = { 0x5b, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	return dev->CmdOut(NULL, 0, cmd, 10);		
}

status_t SyncCache(SCSIDevice *dev)
{
	uchar cmd[10] = { 0x35, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	return dev->CmdOut(NULL, 0, cmd, 10);	
}

status_t SendCueSheet(SCSIDevice *dev, void *data, size_t size)
{
	uchar cmd[10];
	cmd[0] = 0x5D;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;	
	cmd[4] = 0x00;	
	cmd[5] = 0x00;	
	cmd[6] = ((size >> 16) & 0xff);	
	cmd[7] = ((size >> 8 ) & 0xff);	
	cmd[8] = (size & 0xff);	
	cmd[9] = 0x00;	
	
	return dev->CmdOut(data, size, cmd, 10);
}

status_t DoOPC(SCSIDevice *dev)
{
	uchar cmd[10];
	cmd[0] = 0x54;
	cmd[1] = 0x01;
	cmd[2] = 0x00;
	cmd[3] = 0x00;	
	cmd[4] = 0x00;	
	cmd[5] = 0x00;	
	cmd[6] = 0x00;
	cmd[7] = 0x00;
	cmd[8] = 0x00;
	cmd[9] = 0x00;	
	
	return dev->CmdOut(NULL, 0, cmd, 10);
}


status_t WriteBlocks(SCSIDevice *dev, int lba, size_t count, 
					 void *data, size_t length)
{
	uint32 LBA = ((uint32) lba);
	uchar cmd[10];
	
	cmd[0] = 0x2a;
	cmd[1] = 0x00;
	cmd[2] = ((LBA >> 24) & 0xff);	
	cmd[3] = ((LBA >> 16) & 0xff);	
	cmd[4] = ((LBA >> 8 ) & 0xff);	
	cmd[5] = (LBA & 0xff);
	cmd[6] = 0x00;	
	cmd[7] = (count >> 8) & 0xff;
	cmd[8] = count & 0xff;
	cmd[9] = 0x00;		
	
	return dev->CmdOut(data, length, cmd, 10);
}


void DataPumpMMC::NewTrack()
{
	frame = 0;
	track_no++;
	blocks = track->Length();
	blocksize = track->FrameSize();
	step = (64*1024) / blocksize;
//	fprintf(stderr,"Track %02x - blocks=%d, size=%d, step=%d\n",
//		track_no,blocks,blocksize,step);
}

float 
CDMMCDriver::PercentFifo(void)
{
	Lock();
	if(datapump) {
		float r = datapump->PercentFifo(); 
		Unlock();
		return r;
	} else {
		Unlock();
		return 0.0;
	}
}

float 
CDMMCDriver::PercentDone(void)
{
	Lock();
	if(datapump) {
		float r = datapump->PercentWritten();
		Unlock();
		return r;
	} else {
		Unlock();
		return 1.0;
	}
}


void DataPumpMMC::Burn(CDTrack *first)
{
	uint32 total;
	CDTrack *t;
	
	track_no = 0;
	track = first;
	for(t=first,total=0;t;t=t->Next()){
		total += t->Length();
	}
	NewTrack();
	LBA = -150;
	Start(total);
}


DataPumpMMC::DataPumpMMC(CDMMCDriver *drv, SCSIDevice *device) 
: DataPump(4*1024*1024,64*1024)
{
	driver = drv;
	dev = device;
}

DataPumpMMC::~DataPumpMMC()
{

}

status_t 
DataPumpMMC::Fill(DataBuffer *db)
{
	if(!track) {
		return B_ERROR;
	}
	
	db->track = track_no;
	db->count = blocks > step ? step : blocks;
	db->frame = frame;
	db->blocksize = blocksize;
	
	track->FrameData(frame, db->count, const_cast<void *>(db->data));
	
	frame += db->count;
	blocks -= db->count;
	if(blocks <= 0) {
		track = track->Next();
		if(track) {
			NewTrack();
		} else {
			db->done = 1;
		}
	}
	
	return B_OK;
}

status_t 
DataPumpMMC::Empty(DataBuffer *db)
{
#if REALBURN
	while(WriteBlocks(dev, LBA, db->count, const_cast<void *>(db->data), db->blocksize * db->count)){
		if(dev->IsError(2,4,8)){
			// device busy... wait a bit
			// fprintf(stderr,"@");
			snooze(100000);
		} else {
			driver->Error("Write Failure",true);
			driver->Lock();
			delete driver->datapump;
			driver->datapump = NULL;
			driver->Unlock();
			return B_ERROR;
		}
	}
#endif
	LBA += db->count;
	
	if(db->done){
#if REALBURN
		if(SyncCache(dev)) {
			driver->Error("sync cache failed",true);
		}
#endif
		driver->Lock();
		delete driver->datapump;
		driver->datapump = NULL; 
		driver->Unlock();
	}
	
	return B_OK;
}


CDMMCDriver::CDMMCDriver(void)
{
	errmsg[0] = 0;
	looper = NULL;
	dev = NULL;
	datapump = NULL;
}


CDMMCDriver::~CDMMCDriver()
{
}

const char *
CDMMCDriver::Name(void)
{
	return "SCSI-3 MMC";
}

const char *
CDMMCDriver::DeviceName(void)
{
	if(dev) {
		InquiryInfo ii;
		if(dev->Inquiry(ii) == B_OK){
			sprintf(devname,"%s %s %s",ii.Vendor(),ii.Product(),ii.Revision());
			return devname;
		} else {
			return "MMC-3 Compliant CDR";
		}
	} else {
		return "<none>";
	}
}

const char *
CDMMCDriver::DevicePath(void)
{
	if(dev) {
		return dev->Path();
	} else {
		return "<none>";
	}
}


bool 
CDMMCDriver::IsSupportedDevice(SCSIDevice *dev)
{
	WriteParamsPage wpp;
	if(wpp.ReadPage(dev)){
		return false;
	} else {
		return true;
	}
}

bool 
CDMMCDriver::Burning(void)
{
	Lock();
	if(datapump){
		Unlock();
		return true;
	} else {
		Unlock();
		return false;
	}
}

bool 
CDMMCDriver::IsBlankDisc(void)
{
	if(!dev) return false;
	
	DiscInfo di;
	if(di.ReadPage(dev)){
//		fprintf(stderr,"cannot read discinfo\n");
		return false;
	}
//	fprintf(stderr,"** %08x %08x %s\n",di.LastSessionStart(),
//			di.LastSessionStop(),di.Blank() ? "blank" : "not blank");
	return di.Blank();
}

CDDriver *
CDMMCDriver::GetDriverInstance(SCSIDevice *dev, BLooper *looper)
{
	CDMMCDriver *d = new CDMMCDriver();
	d->dev = dev;
	d->looper = looper;
	return d;
}

status_t 
CDMMCDriver::Check(CDTrack */*tracks*/)
{
	if(!dev) {
		Error("no device?");
		return B_ERROR;
	}
	return B_OK;
}

status_t 
CDMMCDriver::Start(CDTrack *first)
{
	if(!dev) return B_ERROR;
	
	if(IsBlankDisc() == false){
		Error("Disc is not blank");
		return B_ERROR;
	}
	
	Lock();
	Error("Okay");
	
	if(datapump) {
		Unlock();
		return B_ERROR; // already got one.
	}
	
	WriteCachePage wcp;
	if(wcp.ReadPage(dev)){
		Error("cannot read write cache page",true);
		Unlock();
		return -1;
	} else {
		wcp.SetWriteCache(true);
		if(wcp.WritePage(dev)){
			Error("cannot modify write cache page",true);
			Unlock();
			return -1;
		}
	}
	
	WriteParamsPage wpp;
	if(wpp.ReadPage(dev)){
		Error("cannot read write params",true);
		Unlock();
		return -1;
	} else {
		wpp.PrepForDAO();
		if(wpp.WritePage(dev)){
			Error("cannot modify write params page",true);
			Unlock();
			return -1;
		}
	}

	CDCueSheet cuesheet(first);
	
	if(SendCueSheet(dev, cuesheet.Lines(), 8*cuesheet.LineCount())) {
		Error("cannot send cuesheet",true);
//		fprintf(stderr,"cannot send cuesheet\n");
//		dev->DumpErrorInfo();	
		Unlock();
		return -1;
	}
	
	int i;
	CDTrack *t;
	
	fprintf(stderr,"---\n");
	i = 1;
	for(t = first; t; t = t->Next()){
		uint32 m,s,f;
		t->MSF(m,s,f);
		printf("%02d: %s %02ld:%02ld:%02ld (%8ld) %8ldf / %8ldb / %4ld\n", i, 
			   (t->IsData() ? "data " : "audio"),
			   m,s,f, t->LBA(), t->Length(), 
			   t->Length() * t->FrameSize(), t->FrameSize());
		
	}
	
	TrackInfo ti(0);
//	printf("--  Sn Start    NextAddr FreeBlks PktSize  TrkSize  DM CP BL PK FP RS TM DM\n");    
	ti.ReadPage(dev,1);
//	printf("%02x: %02x %08x %08x %08x %08x %08x %s  %s  %s  %s  %s  %s  %02x %02x\n",
//	1,ti.SessionNumber(),
//	ti.TrackStartAddr(),ti.NextWritableAddr(),ti.FreeBlocks(),
//	ti.FixedPacketSize(),ti.TrackSize(), 
//	ti.Damaged()?"x":"-", ti.Copy()?"x":"-", ti.Blank()?"x":"-",
//	ti.Packet()?"x":"-", ti.FixedPacket()?"x":"-", 
//	ti.Reserved()?"x":"-", ti.TrackMode(), ti.DataMode())
	
#if REALBURN	
	fprintf(stderr,"Optimum Power Calibration: ");
	if(DoOPC(dev)){
		fprintf(stderr,"skipped\n");
	} else {
		fprintf(stderr,"done\n");
	}
#endif
	
	datapump = new DataPumpMMC(this,dev);	
	datapump->Burn(first);
	
	Unlock();
	return B_OK;
	
}
void 
CDMMCDriver::Error(const char *error, bool scsi_info)
{
	if(scsi_info){
		sprintf(errmsg,"%s (%02x:%02x:%02x)", error,
				dev->GetSenseKey(), dev->GetASC(), dev->GetASCQ());
	} else {
		strncpy(errmsg,error,255);
	}
}


status_t 
CDMMCDriver::Abort(void)
{
	return B_ERROR;
}

char *
CDMMCDriver::GetError(void)
{
	return errmsg;
}


/*

	cdda-fs
	
	functions that
	- do the actual cdda-reading
	- read the TOC
	- calculate the CDDB and CDPlayer key

*/


#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <scsi.h> 
#include <CAM.h> 
#include <ByteOrder.h>
#include <KernelExport.h>

#include "iso.h"
#include "cdda_io.h"


status_t cdda_readsectors(void *_ns, filecookie *cookie, char *buf, long sectornumber, long numsectors);

status_t cdda_readdata(void *_ns, filecookie *cookie, char *buf,off_t cdda_offset, size_t _length)
{
	nspace* ns = (nspace*)_ns;
	long length=_length;

	long buffernumber=cdda_offset/ns->CDDA_BUFFER_SIZE;
	long endbuffernumber=(cdda_offset+length)/ns->CDDA_BUFFER_SIZE;
	long bytesintobuffer=(cdda_offset%ns->CDDA_BUFFER_SIZE);
	
//	dprintf("cdda_readdata %d @ %08x (%d:%d)\n",length,buf,buffernumber,bytesintobuffer);

	if(bytesintobuffer!=0)
	{
		//dprintf("reading first block\n");
		// first read until next block boundary
		if(cookie->buffernumber!=buffernumber)
		{
			// before reading a new block into the buffer, check if the current block overlaps 
			// with any part of the requested data. If so, copy that part from the buffer first.
			// This saves us from having to read that block again later.
			// For simplicity, only the case where the current cached block overlaps with the
			// end of the requested data is handled. If data beyond the current cached requested
			// block is asked for, the block is read again later. This case should be sufficiently
			// rare not to matter.
			if(	(cdda_offset<cookie->buffernumber*ns->CDDA_BUFFER_SIZE)&&
				(cdda_offset+length>cookie->buffernumber*ns->CDDA_BUFFER_SIZE))
			{
				// overlap at end, memcpy part and adjust length
				long overlap=cdda_offset+length-cookie->buffernumber*ns->CDDA_BUFFER_SIZE;
				memcpy(buf+length-overlap,cookie->buffer,overlap);
				length-=overlap;
			}

			// read a new block and copy a part of it.
			if(cdda_readsectors(ns, cookie,cookie->buffer,buffernumber*ns->CDDA_BUFFER_SIZE/CDDA_SECTOR_SIZE,ns->CDDA_BUFFER_SIZE/CDDA_SECTOR_SIZE)!=B_OK)
			{
				dprintf("cdda_readdata ERROR, head\n");
				cookie->buffernumber=-1;
				return B_ERROR;
			}
			cookie->buffernumber=buffernumber;
		}

		long firstblockbytes=min_c(length,ns->CDDA_BUFFER_SIZE-bytesintobuffer);
		memcpy(buf,cookie->buffer+bytesintobuffer,firstblockbytes);
		length-=firstblockbytes;
		buf+=firstblockbytes;
		buffernumber++;
	}

	// read whole blocks, CDDA_BUFFER_SIZE at a time
	while(length>=ns->CDDA_BUFFER_SIZE)
	{
		long numblocks=length/ns->CDDA_BUFFER_SIZE;
		if(numblocks>1)
			numblocks=1;

		if(cdda_readsectors(ns,cookie,buf,buffernumber*ns->CDDA_BUFFER_SIZE/CDDA_SECTOR_SIZE,ns->CDDA_BUFFER_SIZE/CDDA_SECTOR_SIZE)!=B_OK)
		{
			dprintf("cdda_readdata ERROR, body\n");
			return B_ERROR;
		}

		length-=(numblocks*ns->CDDA_BUFFER_SIZE);
		buf+=(numblocks*ns->CDDA_BUFFER_SIZE);
		buffernumber+=numblocks;
	}

	if(length>0)
	{
		// read partially used last block, and save it in the buffer so the next read can use
		// the rest of the block.
		if(cdda_readsectors(ns,cookie,cookie->buffer,buffernumber*ns->CDDA_BUFFER_SIZE/CDDA_SECTOR_SIZE,ns->CDDA_BUFFER_SIZE/CDDA_SECTOR_SIZE)!=B_OK)
		{
			dprintf("cdda_readdata ERROR, tail\n");
			cookie->buffernumber= -1;
			return B_ERROR;
		}
		cookie->buffernumber=buffernumber;
		memcpy(buf,cookie->buffer,length);
	}

	//dprintf("cdda_readdata done OK\n");
	return B_OK;
};



// read multiple cdda-sectors
status_t cdda_readsectors(void *_ns, filecookie *cookie, char *buf, long sectornumber, long numsectors)
{
	nspace* ns = (nspace*)_ns;
	scsi_read_cd cmd;
	status_t result=B_NO_ERROR;

//	dprintf("cdda_read_sectors(fs,fc,%08x,%d,%d)\n",buf,sectornumber,numsectors);

	// cdda_readdata will round "sectornumber" to a multiple of the buffersize,
	// which in most cases means that "sectornumber" can be before the start of
	// the first track. Some drives can't handle reading before the start of the
	// first track, so adjust bufferstart and number of sectors to read for this
	// case.
	ulong firsttrackstart=ns->toc.tocentry[0].startaddress;
	if(sectornumber<firsttrackstart)
	{
	//	dprintf("adjusting start of read\n");
		numsectors-=(firsttrackstart-sectornumber);
		buf+=(firsttrackstart-sectornumber)*CDDA_SECTOR_SIZE;
		sectornumber=firsttrackstart;
	}
	
	while((numsectors>0)&&(result==B_NO_ERROR))
	{
		cmd.start_m = sectornumber/75/60;
		cmd.start_s = (sectornumber/75)%60;
		cmd.start_f = (sectornumber)%75;

		long realnumsectors=min_c(numsectors,ns->CDDA_BLOCKREAD_SIZE);
		cmd.length_m = (realnumsectors/75)/60;
		cmd.length_s = (realnumsectors/75)%60;
		cmd.length_f = realnumsectors%75;
		
		cmd.buffer_length = realnumsectors*CDDA_SECTOR_SIZE;
		cmd.buffer = (char *)buf;
		cmd.play = false;
	
		result = ioctl(ns->fd, B_SCSI_READ_CD, &cmd);
		if(result!=0)
		{
			// error occurred while reading data, might be a crappy cdrom drive
			// (some drives don't handle "large" reads, for some value of "large") 

			if(1==ns->CDDA_BLOCKREAD_SIZE)
				break; // give up

			// retry with small readsize (small being 1, the smallest possible value)
			dprintf("error reading with blocksize %d, using 1 (one) instead\n",ns->CDDA_BLOCKREAD_SIZE);
			ns->CDDA_BLOCKREAD_SIZE=1;
			result=B_NO_ERROR;
			continue;
		}
		
		buf+=realnumsectors*CDDA_SECTOR_SIZE;
		numsectors-=realnumsectors;
		sectornumber+=realnumsectors;
		
	}
	if(result != 0)
		return B_ERROR;
	else
		return B_OK;
}


// read toc using BeOS iocntl driver interface
int read_toc(int cdfile, t_CDFS_TOC *FStoc)
{
	int result=-1;
	t_CDtoc toc;
	
	if(cdfile>0)
	{
		result = ioctl(cdfile, B_SCSI_GET_TOC, &toc);
		if(result==B_OK)
		{
			// convert TOC to internal convenience format, filtering out data tracks
			int FStocindex=0;
			for(int i=0;i<((toc.maxtrack-toc.mintrack)+1);i++)
			{
				t_CDaddress ta=toc.tocentry[i].address;
				FStoc->tocentry[FStocindex].startaddress=(((ta.minute*60)+ta.second)*75)+ta.frame;
				ta=toc.tocentry[i+1].address;
				FStoc->tocentry[FStocindex].length=(((ta.minute*60)+ta.second)*75)+ta.frame-FStoc->tocentry[FStocindex].startaddress;
				if(!(toc.tocentry[i].flags&4))
				{
					char name[10];
					sprintf(name,"Track %02d",toc.tocentry[i].tracknumber);
					FStoc->tocentry[FStocindex].name=strdup(name);
					FStoc->tocentry[FStocindex].realtracknumber=toc.tocentry[i].tracknumber;
					FStocindex++; // increase if this is not a data track
				}
			//	dprintf("track %d, flags %02x\n",i,toc.toc[i].flags);
			}
			FStoc->numtracks=FStocindex;
			FStoc->CDDBid=cddb_discid(&toc);
			FStoc->CDPlayerkey=cdplayerkey(&toc);
			//dprintf("CDDB: %08x/%d\n",FStoc->CDDBid,FStoc->CDDBid);
		}
	}
	return result;
}


unsigned long cdplayerkey(t_CDtoc *cdtoc)
{
	char		byte;
	int32	tracks;
	uint32	key = 0;
	scsi_toc *TOC=(scsi_toc*)cdtoc;

	tracks = TOC->toc_data[3] - TOC->toc_data[2] +1;
	byte = TOC->toc_data[4 + (tracks * 8) + 5];
	key = (byte / 10) << 20;
	key |= (byte % 10) << 16;
	byte = TOC->toc_data[4 + (tracks * 8) + 6];
	key |= (byte / 10) << 12;
	key |= (byte % 10) << 8;
	byte = TOC->toc_data[4 + (tracks * 8) + 7];
	key |= (byte / 10) << 4;
	key |= byte % 10;
	
	return key;
}


// code taken from the CDDB sample code archive
static int cddb_sum(int n);
static int cddb_sum(int n) 
{
    char buf[12];
    char *p;
    unsigned long ret = 0;

    /* For backward compatibility this algoritm must not change */
    sprintf(buf, "%lu", n);
    for (p=buf; *p != '\0'; p++)
        ret += (*p-'0');
    return ret;
}

unsigned long cddb_discid(t_CDtoc *cdtoc) 
{
    int i;
    int n=0;
    int t=0;

    /* For backward compatibility this algoritm must not change */
    for (i=0;i<((cdtoc->maxtrack-cdtoc->mintrack))+1;i++) 
    {
        n+=cddb_sum((cdtoc->tocentry[i].address.minute*60)+cdtoc->tocentry[i].address.second);
        t+= ((cdtoc->tocentry[i+1].address.minute*60)+cdtoc->tocentry[i+1].address.second) -
            ((cdtoc->tocentry[i].address.minute*60)+cdtoc->tocentry[i].address.second);
    }
    return (((n%0xff) << 24) + (t << 8) + cdtoc->maxtrack);
}



#if 0
// read toc using direct scsi access
// didn't work on my R4 intel machine, so
// I changed to using CDDA ioctls instead
int read_toc(char *dev, tocbuffer *toc)
{
  static int header = 1; 
  int type,fd,e;
  raw_device_command rdc; 
  uchar data[36], sense[16]; 

  /* fill out our raw device command data */ 
  rdc.data = data; 
  rdc.data_length = 36; 
  rdc.sense_data = sense; 
  rdc.sense_data_length = 0; 
  rdc.timeout = 1000000; 
  rdc.flags = B_RAW_DEVICE_DATA_IN; 
  rdc.command_length = 6; 
  rdc.command[0] = 0x12; 
  rdc.command[1] = 0x00; 
  rdc.command[2] = 0x00; 
  rdc.command[3] = 0x00; 
  rdc.command[4] = 36; 
  rdc.command[5] = 0x00;

  if((fd = open(dev,0)) < 0) 
  	return B_ERROR;
  e = ioctl(fd, B_RAW_DEVICE_COMMAND, &rdc, sizeof(rdc)); 
  close(fd); 
  if((e != 0) || (rdc.cam_status != CAM_REQ_CMP)) 
  	return B_ERROR;

  type = data[0] & 0x1F; 
    
  if(type==5) // CDROM
  {
	  rdc.data = toc; 
	  rdc.data_length = sizeof(tocbuffer); 
	  rdc.sense_data = sense; 
	  rdc.sense_data_length = 16; 
	  rdc.timeout = 1000000; 
	  rdc.flags = B_RAW_DEVICE_DATA_IN; 
	  rdc.command_length = 10;
	  rdc.command[0] = 0x43;
	  rdc.command[1] = 0x00;
	  rdc.command[2] = 0x00;
	  rdc.command[3] = 0x00;
	  rdc.command[4] = 0x00;
	  rdc.command[5] = 0x00;
	  rdc.command[6] = 0x01;
	  rdc.command[7] = sizeof(tocbuffer)>>8;
	  rdc.command[8] = sizeof(tocbuffer)&0xff; 
	  rdc.command[9] = 0x00;

	  if((fd = open(dev,0)) < 0) 
	  	return B_ERROR; 
	  e = ioctl(fd, B_RAW_DEVICE_COMMAND, &rdc, sizeof(rdc)); 
	  close(fd); 
	  if((e != 0) || (rdc.scsi_status != 0)) 
	  	return B_ERROR;
	  	
	  return B_OK;
  }
  else
  	return B_ERROR;
} 
#endif

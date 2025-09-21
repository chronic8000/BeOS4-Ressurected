
#include <malloc.h>
#include <SupportDefs.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Drivers.h>
#include <OS.h>
#include <KernelExport.h> 
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <malloc.h>
#include <driver_settings.h>
#include <stdlib.h>
#include <errno.h>
//#include "dmalloc.h"

#include "iso.h"
#include "cdda_io.h"
#include "cdda_supportfuncs.h"

const char defaultsettings[]=
"# CDDA-fs settings file\n\n"
"# You can adjust some parameters that affect CDDA-fs' operation.\n"
"# You must unmount/remount the CD for the changes to take effect.\n\n"
"# The default blocksize for a read is 64 CDDA sectors (of 2352 bytes each).\n"
"# Some CDROM drives cannot handle this, in which case you should try\n"
"# lowering this value.\n"
"\n"
"blocksize 64\n"
"\n"
"# The default buffer size is 64 CDDA sectors (of 2352 bytes each).\n"
"# You may increase this to enhance performance on slow CDROM drives,\n"
"# or decrease it to reduce memory usage.\n"
"# The size of the buffer should be larger than or equal to the\n"
"# blocksize defined above.\n"
"\n"
"buffersize 64\n"
"\n"
;


const char readmefile[]=
"<HTML>"
"<HEAD><TITLE>CDDA-fs</TITLE></HEAD>"
"<BODY BGCOLOR=\"#ffffff\">"
"<P>"
"The files in the Audio CD window represent the songs on "
"the audio CD you inserted into the computer.  You can "
"copy the digital data from the CD to your computer's "
"hard disk by just dragging the files in the \"wav\" directory "
"to your desktop, or play them by opening them with MediaPlayer. "
"Doubleclicking a file in the \"cdda\" folder "
"will play the song using CDPlayer."
"</P>"
"<P>"
"If the files in the \"wav\" directory do not play properly "
"with MediaPlayer or cannot be copied, then BeOS may not "
"support your CD-ROM drive. This Audio CD feature of BeOS "
"only works with CD-ROM drives that support CD-DA extraction.  "
"If your drive does not work, please "
"<a href=http://bebugs.be.com/devbugs/bugform.php3>"
"let us know</a>. "
"</P>"
"<HR>"
""
"Frequently Asked Question:"
"<UL>"
"Are those real WAV files, or are you fooling the Tracker when playing the tracks?"
"</UL>"
"Frequently Given Answer:"
"<UL>"
"As far as BeOS is concerned they are real WAV files, and every application will treat them in "
"exactly the same way as a WAV file on your harddisk. While the WAV files as such aren't "
"actually on the CD (only the data contained within them is), applications will never know. It's "
"totally transparent."
"</UL>"
""
"<P>"
"<HR>"
"CDDA-fs version 1.8<BR>"
"Copyright &copy; Be Inc."
"</BODY></HTML>"
"";

void ReadConfig(nspace *ns);
void ReadConfig(nspace *ns)
{
	struct stat statbuf;
	if(stat(SETTINGSFILE,&statbuf)<0)
	{
		if(errno==ENOENT)
		{
			//dprintf("creating settings file\n");
			int fd=open(SETTINGSFILE,O_WRONLY|O_CREAT|O_TRUNC);
			if(fd>=0)
			{
				write(fd,defaultsettings,strlen(defaultsettings));
				close(fd);
				chmod(SETTINGSFILE,00666);
			}
			else
				dprintf("error writing default settings file\n");
		}
		else
			dprintf("error opening settings file\n");
	}

	void *handle=load_driver_settings("cdda");
	ns->CDDA_BLOCKREAD_SIZE = strtoul(get_driver_parameter(handle, "blocksize", "64", "64"), NULL, 0);
	unload_driver_settings(handle);
	ns->CDDA_BUFFER_SIZE=strtoul(get_driver_parameter(handle, "buffersize", "64", "64"), NULL, 0);

	// sanity checks
	if(ns->CDDA_BUFFER_SIZE<=0)
		ns->CDDA_BUFFER_SIZE=1;
	if(ns->CDDA_BLOCKREAD_SIZE<=0)
		ns->CDDA_BLOCKREAD_SIZE=1;
	if(ns->CDDA_BUFFER_SIZE<ns->CDDA_BLOCKREAD_SIZE)
		ns->CDDA_BUFFER_SIZE=ns->CDDA_BLOCKREAD_SIZE;

	ns->CDDA_BUFFER_SIZE*=CDDA_SECTOR_SIZE;
	//dprintf("USING BLOCKSIZE %d\n",ns->CDDA_BLOCKREAD_SIZE);
	//dprintf("USING BUFFERSIZE %d\n",ns->CDDA_BUFFER_SIZE);
}

// Functions specific to cdda driver.
int CDDAMount(const char *path, const int flags, nspace** newVol)
{
	//dprintf("CDDAMount - ENTER\n");

	int 		result = B_NO_ERROR;
	nspace*		vol = (nspace*)calloc(sizeof(nspace), 1);
	if (vol == NULL)
	{
		/// error.
		result = ENOMEM;
		//dprintf("ISOMount - mem error \n");
	}
	else
	{		
		strncpy(vol->devicePath,path,127);

		/* open and lock the device */
		vol->fd = open(path, O_RDONLY);
		if((vol->fd>=0)&&(read_toc(vol->fd,&vol->toc)==B_OK))
		{
			vol->mounttime=time(NULL);
			vol->locksem=create_sem(1,"cdda FS lock");
			// init root vnode
		    InitNode(vol,&(vol->rootvnode), CDDA_ROOTNODE_ID);
			ReadConfig(vol);
		}
		else
		{
			//dprintf("ISO9660 ERROR - Unable to open <%s>\n", path);
			free(vol);
			vol = NULL;
			result = EINVAL;
		}
	}
	//dprintf("CDDAMount - EXIT, result %s, returning 0x%x\n", strerror(result), vol);
	*newVol = vol;
	return result;
}

/* Reads in a single directory entry and fills in the values in the
	dirent struct. Uses the cookie to keep track of the current block
	and position within the block. Also uses the cookie to determine when
	it has reached the end of the directory file.
	
	NOTE: If your file sytem seems to work ok from the command line, but
	the tracker doesn't seem to like it, check what you do here closely;
	in particular, if the d_ino in the stat struct isn't correct, the tracker
	will not display the entry.
*/
int CDDAReadDirEnt(nspace* ns, dircookie* cookie, struct dirent* buf, size_t bufsize)
{
//	dprintf("CDDAReadDirEnt - ENTER. vnid=%Ld,counter=%d\n",cookie->vnid,cookie->counter);

	int	result = B_NO_ERROR;

	LockFS(ns);
	if(cookie->counter==1)
	{
		if(cookie->vnid>2000)
			buf->d_pino=CDDA_WAVNODE_ID;
		else if(cookie->vnid>=1000)
			buf->d_pino=CDDA_CDDANODE_ID;
		else
			buf->d_pino=CDDA_ROOTNODE_ID;
		buf->d_ino=cookie->vnid;
		sprintf(buf->d_name,".");
	}
	else if(cookie->counter==2)
	{
		buf->d_ino=CDDA_ROOTNODE_ID;
		buf->d_pino=CDDA_ROOTNODE_ID;
		sprintf(buf->d_name,"..");
	}
	else
	{
		buf->d_pino=CDDA_ROOTNODE_ID;
		if(cookie->vnid==CDDA_ROOTNODE_ID)
		{
			if(cookie->counter==3)
			{
				buf->d_ino=CDDA_CDDANODE_ID;
				sprintf(buf->d_name,"cdda");
			}
			else if(cookie->counter==4)
			{
				buf->d_ino=CDDA_WAVNODE_ID;
				sprintf(buf->d_name,"wav");
			}
			else if(cookie->counter==5)
			{
				buf->d_ino=CDDA_SETTINGS_ID;
				sprintf(buf->d_name,"settings");
			}
			else if(cookie->counter==6)
			{
				buf->d_ino=CDDA_README_ID;
				sprintf(buf->d_name,"README");
			}
			else
				result=ENOENT;
		}
		else if(cookie->vnid==CDDA_CDDANODE_ID && cookie->counter<=(ns->toc.numtracks+2))
		{
			buf->d_ino=cookie->counter-2+1000;
		    sprintf(buf->d_name,"%s",GetTrackName(ns,cookie->counter-2));
		}
		else if(cookie->vnid==CDDA_WAVNODE_ID && cookie->counter<=(ns->toc.numtracks+2))
		{
			buf->d_ino=cookie->counter-2+2000;
		    sprintf(buf->d_name,"%s",GetTrackName(ns,cookie->counter-2));
		}
		else
			result=ENOENT;
	}
    buf->d_reclen=strlen(buf->d_name)+1;
//	dprintf("CDDAReadDirEnt - EXIT, result is %s, vnid is %Lu, name is %s\n", strerror(result),buf->d_ino, buf->d_name);
	UnlockFS(ns);
	return result;
}


int
InitNode(void *_ns, vnode* rec, vnode_id vnid)
{
	//dprintf("InitNode - ENTER, vnid=%Ld\n",vnid);

	int 	result = B_NO_ERROR;
	nspace*		ns = (nspace*)_ns;

	rec->flags=0;
	rec->id=vnid;

	if(vnid==CDDA_ROOTNODE_ID)
	{
		rec->mode=(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH );
	}
	else if(vnid==CDDA_CDDANODE_ID)
	{
		rec->mode=(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH );
	}
	else if(vnid==CDDA_WAVNODE_ID)
	{
		rec->mode=(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH );
	}
	else if(vnid==CDDA_SETTINGS_ID)
	{
		rec->mode=(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH);
	}
	else if(vnid==CDDA_README_ID)
	{
		rec->mode=(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH);
	}
	else if(vnid>2000)
	{
		rec->mode=(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH);
	}
	else if(vnid>1000)
	{
		rec->mode=(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH);
	}
	else
	{
		result=ENOENT;
	}
//	dprintf("InitNode - EXIT. filename=%s\n",rec->filename);
	return result;
}

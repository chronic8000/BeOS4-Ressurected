/*

	various support functions for cdda-fs

*/

#include "cdda_supportfuncs.h"
#include <string.h>
#include <malloc.h>
#include <KernelExport.h> 


void LockFS(nspace *ns)
{
	acquire_sem(ns->locksem);
};
void UnlockFS(nspace *ns)
{
	release_sem(ns->locksem);
};


int IndexForFilename(nspace *ns, const char *filename)
{
	for(int i=0;i<ns->toc.numtracks;i++)
	{
		if(strcmp(filename,ns->toc.tocentry[i].name)==0)
			return i+1;
	}
	return -1;
}

const char *GetTrackName(nspace *ns, long tracknumber)
{
	if((tracknumber>=1)&&(tracknumber<=ns->toc.numtracks))
		return ns->toc.tocentry[tracknumber-1].name;
	else
		return NULL;
}


static int ReadLine(int fd, char *buffer, int maxfill);
static int ReadLine(int fd, char *buffer, int maxfill)
{
	char c;
	int numread;
	int total=0;
	
	do {
		numread=read(fd,&c,1);
		if((numread==1)&&(c!='\n'))
		{
			*buffer++=c;
			maxfill--;
			total++;
		}
	} while((numread==1)&&(c!='\n')&&(maxfill>0));
	*buffer='\0';
	return total;
}


void GetCDDANames(nspace *ns,bool dorequest)
{
	char filename[128];
	char trackname[B_FILE_NAME_LENGTH+2];
	
	sprintf(filename,"%s/%08x",cddbfiles,ns->toc.CDDBid);
	int file=open(filename,O_RDONLY);
	
	ns->nameschanged=false;
	if(file>=0)
	{
		if(ReadLine(file,trackname,B_FILE_NAME_LENGTH)!=0)
			strcpy(ns->volumename,trackname);
		SanitizeString(ns->volumename);

		for(int i=0;i<ns->toc.numtracks;i++)
		{
			if(ReadLine(file,trackname,B_FILE_NAME_LENGTH)!=0)
			{
				char *oldname=ns->toc.tocentry[i].name;
				ns->toc.tocentry[i].name=SanitizeString(strdup(trackname));
				free(oldname);
			}
		}
		close(file);
	}
	else if(dorequest)
	{
		char filename[128];
		mkdir(cddbfilestodo,S_IRWXU|S_IRWXG|S_IRWXO); // no error checking here ...
		sprintf(filename,"%s/%08x",cddbfilestodo,ns->toc.CDDBid);
		int file=open(filename,O_WRONLY|O_CREAT|O_EXCL);
		if(file>=0)				// ... because it is done here
		{
			//dprintf("creating request-file %08x for cddb-daemon\n",ns->toc);
			write(file,&ns->toc,sizeof(t_CDFS_TOC));
			close(file);
		}
		else
			dprintf("couldn't open %s (%d)\n",filename,file);
	}
}

void DumpCDDANames(nspace *ns)
{
	char filename[128];
	char trackname[B_FILE_NAME_LENGTH+2];
	
	mkdir(cddbfiles,S_IRWXU|S_IRWXG|S_IRWXO); // no error checking here ...
	sprintf(filename,"%s/%08x",cddbfiles,ns->toc.CDDBid);
	int file=open(filename,O_WRONLY|O_CREAT|O_TRUNC);
	if(file>=0)				// ... because it is done here
	{
		strncpy(trackname,ns->volumename,B_FILE_NAME_LENGTH);
		strcat(trackname,"\n");
		write(file,trackname,strlen(trackname));
	
		for(int i=0;i<ns->toc.numtracks;i++)
		{
			strncpy(trackname,ns->toc.tocentry[i].name,B_FILE_NAME_LENGTH);
			strcat(trackname,"\n");
			write(file,trackname,strlen(trackname));
		}
		close(file);
	}
	else
		dprintf("couldn't open %s (%d)\n",filename,file);
}


// remove invalid characters from the string
char *SanitizeString(char *ss)
{
	char *s=ss;
	while(s&&(*s!='\0'))
	{
		if(*s=='/')
			*s='-';
		s++;
	}
	return ss;
}

WAVHeader::WAVHeader(long dl)
{
	riffheader=B_HOST_TO_BENDIAN_INT32('RIFF');
	length=B_HOST_TO_LENDIAN_INT32(dl+sizeof(WAVHeader)-8);

	subtype=B_HOST_TO_BENDIAN_INT32('WAVE');
	fmtchunk=B_HOST_TO_BENDIAN_INT32('fmt ');
	fmtlength=B_HOST_TO_LENDIAN_INT32(16);
	// fill in format chunk
	formattag=B_HOST_TO_LENDIAN_INT16(1);
	numchannels=B_HOST_TO_LENDIAN_INT16(2);
	samplespersec=B_HOST_TO_LENDIAN_INT32(44100);
	avgbytespersec=B_HOST_TO_LENDIAN_INT32(44100*4);
	blockalign=B_HOST_TO_LENDIAN_INT16(4);
	bitspersample=B_HOST_TO_LENDIAN_INT16(16);

	datachunk=B_HOST_TO_BENDIAN_INT32('data');
	datalength=B_HOST_TO_LENDIAN_INT32(dl);
}



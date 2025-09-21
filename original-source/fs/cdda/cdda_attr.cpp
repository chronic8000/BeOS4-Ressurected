/*
	CDDA filesystem - copyright 1998 Marco Nelissen

	Handles attributes for the CDDA filesystem.
	Files have a BEOS:TYPE of "audio/x-cdda" for the 
	CDDA files, and "audio/x-wav" for the WAV files.
	Files also have several extra attributes, indicating
	the length and indexnumber of the track.
	The root directory, as well as all files, also have
	a CD:key and CDDB:cddbid attribute, indicating the
	cd-key (according to CDPlayer) and CDDB-ID respectively

	Writable attributes (for Tracker's sake) are implemented,
	but these attributes are not stat'able (e.g. they don't
	show up when you do a listattr on the file in question).
	The custom-attributes are written to disk as files.
*/

#include "cdda_attr.h"
#include "iso.h"
#include <malloc.h>
#include <KernelExport.h> 
#include <string.h>
#include <TypeConstants.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Mime.h>

const char *TEXTMIME="text/plain";
const char *HTMLMIME="text/html";
const char *CDDAMIME="audio/x-cdda";
const char *WAVMIME="audio/x-wav";
const char *TRACKLENGTH="Audio:Length";
const char *TRACKNUMBER="Audio:Track";
const char *CDPLAYERKEY="CD:key";
const char *CDDBATTRIB="CD:cddbid";

int	fs_open_attrdir(void *_ns, void *_node, void **_cookie)
{
	vnode*					node = (vnode*)_node;
	int						result = B_NO_ERROR;
	attrcookie* 			attrCookie = (attrcookie*)malloc(sizeof(attrcookie));

	//dprintf("fs_open_attrdir - ENTER, node is 0x%x\n", _node);

	if (attrCookie != NULL)
	{
		attrCookie->counter = 1;
		*_cookie = (void*)attrCookie;
	}
	else
	{
		// Mem error
		result = ENOMEM;
	}

	//dprintf("fs_open_attrdir - EXIT\n");
	return result;
}

int	fs_close_attrdir(void *ns, void *node, void *cookie)
{
	return 0;
}

int	fs_rewind_attrdir(void *ns, void *_node, void *_cookie)
{
	int			result = EINVAL;
	vnode* 		node = (vnode*)_node;
	attrcookie*	cookie = (attrcookie*)_cookie;
	//dprintf("fs_rewind_attrdir - ENTER\n");
	if (cookie != NULL)
	{
		cookie->counter = 1;
		result = B_NO_ERROR;
	}
	//dprintf("fs_rewind_attrdir - EXIT, result is %s\n", strerror(result));
	return result;
}

// this is kinda ugly...
int	fs_read_attrdir(void *_ns, void *_node, void *_cookie, long *num,
					struct dirent *buf, size_t bufsize)
{
	nspace	*ns =	(nspace*)_ns;
	attrcookie*	cookie = (attrcookie*)_cookie;
	vnode* 		node = (vnode*)_node;

	if((cookie->counter==1) &&(node->mode&S_IFREG))
	{
		buf->d_ino=900;
		strcpy(buf->d_name,"BEOS:TYPE");
		buf->d_reclen=strlen(buf->d_name)+1;
		*num=1;
		cookie->counter++;
	}
	else if((cookie->counter==2) && (node->mode&S_IFREG) && (node->id!=CDDA_SETTINGS_ID) && (node->id!=CDDA_README_ID))
	{
		buf->d_ino=900;
		strcpy(buf->d_name,TRACKLENGTH);
		buf->d_reclen=strlen(buf->d_name)+1;
		*num=1;
		cookie->counter++;
	}
	else if((cookie->counter==3) && (node->mode&S_IFREG) && (node->id!=CDDA_SETTINGS_ID) && (node->id!=CDDA_README_ID))
	{
		buf->d_ino=900;
		strcpy(buf->d_name,TRACKNUMBER);
		buf->d_reclen=strlen(buf->d_name)+1;
		*num=1;
		cookie->counter++;
	}
	else if(((cookie->counter==1)&&(node->id==ns->rootvnode.id)||
			(cookie->counter==4) && (node->mode&S_IFREG)) && (node->id!=CDDA_SETTINGS_ID) && (node->id!=CDDA_README_ID))
	{
		buf->d_ino=900;
		strcpy(buf->d_name,CDPLAYERKEY);
		buf->d_reclen=strlen(buf->d_name)+1;
		*num=1;
		cookie->counter++;
	}
	else if(((cookie->counter==2)&&(node->id==ns->rootvnode.id)
			||(cookie->counter==5) && (node->mode&S_IFREG)) && (node->id!=CDDA_SETTINGS_ID) && (node->id!=CDDA_README_ID))
	{
		buf->d_ino=900;
		strcpy(buf->d_name,CDDBATTRIB);
		buf->d_reclen=strlen(buf->d_name)+1;
		*num=1;
		cookie->counter++;
	}
	else
	{
		// custom attributes aren't browsable yet
		*num=0;
	}

	return B_NO_ERROR;
}


int fs_free_attrcookie(void *ns, void *node, void *cookie)
{
	//dprintf("fs_free_attrcookie - ENTER\n");
	if (cookie != NULL)
	{
		free(cookie);
	}
	//dprintf("fs_free_attrcookie - EXIT\n");
	return 0;
}



int	fs_stat_attr(void *_ns, void *_node, const char *name,
					struct attr_info *buf)
{
	nspace	*ns =	(nspace*)_ns;
	vnode* 		node = (vnode*)_node;
	
	//dprintf("stat_attr %s\n",name);

	if((strcmp("BEOS:TYPE",name)==0)&& (node->id==CDDA_SETTINGS_ID))
	{
		buf->size=strlen(TEXTMIME)+1;
		buf->type=B_MIME_STRING_TYPE;
	}
	else if((strcmp("BEOS:TYPE",name)==0)&& (node->id==CDDA_README_ID))
	{
		buf->size=strlen(HTMLMIME)+1;
		buf->type=B_MIME_STRING_TYPE;
	}
	else if((strcmp("BEOS:TYPE",name)==0)&&(node->id>1000))
	{

		if(node->id > 2000)
			buf->size=strlen(WAVMIME)+1;
		else if(node->id > 1000)
			buf->size=strlen(CDDAMIME)+1;

		buf->type=B_MIME_STRING_TYPE;
		return B_NO_ERROR;
	}
	else if((strcmp(TRACKLENGTH,name)==0)&&(node->id>1000))
	{

		if(node->id>2000)
			buf->size=6;
		else
			buf->size=9;
		buf->type=B_STRING_TYPE;

		return B_NO_ERROR;
	}
	else if((strcmp(TRACKNUMBER,name)==0)&&(node->id>1000))
	{
		buf->size=4;
		buf->type=B_INT32_TYPE;

		return B_NO_ERROR;
	}
	else if((strcmp(CDPLAYERKEY,name)==0)&&((node->id==ns->rootvnode.id)
			||(node->id>1000)))
	{
		buf->size=4;
		buf->type=B_INT32_TYPE;

		return B_NO_ERROR;
	}
	else if((strcmp(CDDBATTRIB,name)==0)&&((node->id==ns->rootvnode.id)
			||(node->id>1000)))
	{
		buf->size=4;
		buf->type=B_INT32_TYPE;

		return B_NO_ERROR;
	}
	else
	{
		char filename[B_PATH_NAME_LENGTH];

		// TODO: check for overflow
		sprintf(filename,"%s/%s",cddbfiles,mangledname(ns,node,name));

//		dprintf("attempt to stat attribute %s: %s\n",name,filename);
	
		int fd=open(filename,O_RDONLY);
		if(fd>=0)
		{
			struct stat statbuf;

			fstat(fd,&statbuf);
			buf->size=statbuf.st_size-sizeof(int);				// size of attribute

			lseek(fd,0,SEEK_SET);
			read(fd,&buf->type,sizeof(int));			// type of attribute

			close(fd);
			return B_NO_ERROR;
		}
		else
			dprintf("no attribute-file\n");
	}
	return ENOENT;
}

int	fs_read_attr(void *_ns, void *_node, const char *name, int type,
					void *_buf, size_t *len, off_t pos)
{
	int result=ENOENT;

	nspace	*ns =	(nspace*)_ns;
	char	*buf=	(char*)_buf;
	vnode	*node=	(vnode*)_node;
	
	int tracknum;
	if(node->id > 2000)
		tracknum=node->id-2000;
	else if(node->id > 1000)
		tracknum=node->id-1000;
	else
		tracknum= -1;
	
	if((strcmp("BEOS:TYPE",name)==0)&&(node->id==CDDA_SETTINGS_ID))
	{
		strncpy(buf+pos,TEXTMIME+pos,*len);
		result=B_NO_ERROR;
	}
	else if((strcmp("BEOS:TYPE",name)==0)&&(node->id==CDDA_README_ID))
	{
		strncpy(buf+pos,HTMLMIME+pos,*len);
		result=B_NO_ERROR;
	}
	else if((strcmp("BEOS:TYPE",name)==0)&&(node->id>1000))
	{
		if(node->id > 2000)
			strncpy(buf+pos,WAVMIME+pos,*len);
		else if(node->id > 1000)
			strncpy(buf+pos,CDDAMIME+pos,*len);

		result=B_NO_ERROR;
	}
	else if((strcmp(TRACKLENGTH,name)==0)&&(node->id>1000))
	{
		char timebuf[9];
		int tracklength;
		if(node->id>2000)
		{
			tracklength=(ns->toc.tocentry[tracknum-1].length+38)/75; // 38 is for rounding
			sprintf(timebuf,"%02d:%02d", tracklength/60,tracklength%60);
		}
		else
		{
			tracklength=ns->toc.tocentry[tracknum-1].length; // 38 is for rounding
			sprintf(timebuf,"%02d:%02d.%02d", tracklength/75/60,(tracklength/75)%60,tracklength%75);
		}
		*len=min_c(strlen(timebuf)+1-pos,*len);
		strncpy(buf,timebuf+pos,*len);
		result=B_NO_ERROR;
	}
	else if((strcmp(TRACKNUMBER,name)==0)&&(node->id>1000))
	{
		result=B_NO_ERROR;

		int *intbuf=(int*)buf;
		if(tracknum>0)
			*intbuf=ns->toc.tocentry[tracknum-1].realtracknumber;
		else // error condition!
		{
			*intbuf= -1;
			result=EINVAL;
		}
	}
	else if((strcmp(CDDBATTRIB,name)==0)&&((node->id==ns->rootvnode.id)||(node->id>1000)))
	{
		result=B_NO_ERROR;

		int *intbuf=(int*)buf;
		*intbuf=ns->toc.CDDBid;
	}
	else if((strcmp(CDPLAYERKEY,name)==0)&&((node->id==ns->rootvnode.id)||(node->id>1000)))
	{
		result=B_NO_ERROR;

		int *intbuf=(int*)buf;
		*intbuf=ns->toc.CDPlayerkey;
	}
	else
	{
		char filename[B_PATH_NAME_LENGTH];

		// TODO: check for overflow
		sprintf(filename,"%s/%s",cddbfiles,mangledname(ns,node,name));

//		dprintf("attempt to read attribute %s: %s\n",name,filename);
	
		int fd=open(filename,O_RDONLY);
		if(fd>=0)
		{
			lseek(fd,pos+sizeof(type),SEEK_SET);
			*len=read(fd,buf,*len);
			close(fd);
			result=B_OK;
		}
		else
			dprintf("no attribute-file\n");
	}
	return result;
}


int	fs_write_attr(void *_ns, void *_node, const char *name, int type,	
					const void *buf, size_t *len, off_t pos)
{
	nspace	*ns =	(nspace*)_ns;
	vnode	*node=	(vnode*)_node;
	char filename[B_PATH_NAME_LENGTH];
	int status=B_ERROR;
	
//	dprintf("write_attr(%s), nodeID: %d\n",name,node->id);

	// TODO: check for overflow
	sprintf(filename,"%s/%s",cddbfiles,mangledname(ns,node,name));

	// note: creat is broken, so we use this
	int fd=open(filename,O_CREAT|O_WRONLY|O_TRUNC,0666);
	if(fd>=0)
	{
		lseek(fd,0,SEEK_SET);
		write(fd,&type,sizeof(type));
		lseek(fd,pos+sizeof(type),SEEK_SET);
		*len=write(fd,buf,*len);
		close(fd);
		status=B_OK;
	}

	return status;
}


// Mangle the attribute name, so we can translate an attribute name such as _trk/windframe into
// something that can be written on disk.
// This function also includes the devicename in the name, so we have separate attributes
// for each CDROM device.
// This uses a static buffer, so calling this function invalidates any previously returned
// string;
char *mangledname(void *_ns,void *_node,const char *name)
{
	nspace	*ns =	(nspace*)_ns;
	vnode	*node=	(vnode*)_node;
	char sourcename[B_FILE_NAME_LENGTH];
	static char mangledname[B_FILE_NAME_LENGTH*2];

	mangledname[0]='\0';

	if((strlen(sourcename)+strlen(name)+2)<B_FILE_NAME_LENGTH)
	{
		sprintf(sourcename,"%s/%04Ld/%s",ns->devicePath,node->id,name);

		char *s=sourcename;
		char *d=mangledname;
	
		if(s)
		{
			while(*s!='\0')
			{
				if(*s=='/')
					*d++='@';
				else if(*s=='@')
				{
					*d++='@';
					*d++='@';
				}
				else
					*d++=*s;
				s++;
			}
			*d='\0';
		}
	}
	// truncate string if needed
	mangledname[B_FILE_NAME_LENGTH-1]='\0';
	return mangledname;
}

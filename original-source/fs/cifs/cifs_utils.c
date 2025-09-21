#include "cifs_globals.h"
#include "dir_entry.h"
#include "skiplist.h"

/*

	These functions are simple utils to get variables
	to and from a smb buffer (a char *).
	-Alfred
	
	*/


int  
char_from_ptr(uchar **p, char *c, bool advance) {

	*c = **p;
	if (advance)
		(*p)++;
	return 1;
	
}

int  
uchar_from_ptr(uchar **p, uchar *uc, bool advance) {

	*uc = (unsigned char) **p;
	if (advance)
		(*p)++;
	return 1;
	
}


int  
ushort_from_ptr(uchar **p, unsigned short *s, bool advance) {

	ushort tmp;
	uchar* ptr;
	
	ptr = (uchar *) & tmp;
	ptr[0] = **p;
	ptr[1] = *(*p + 1);
	
	(*s) = B_LENDIAN_TO_HOST_INT16(tmp);
	
	
	if (advance)
		(*p) += 2;
	return 2;
	
}

int  
ulong_from_ptr(uchar **p, unsigned long *l, bool advance) {

	ulong tmp;
	uchar* ptr;
	
	ptr = (uchar *) & tmp;
	ptr[0] = **p;
	ptr[1] = *(*p + 1);
	ptr[2] = *(*p + 2);
	ptr[3] = *(*p + 3);
	
	(*l) = B_LENDIAN_TO_HOST_INT32(tmp);
	
	if (advance)
		(*p) += 4;
	return 4;
	
}

int  
long_from_ptr(uchar **p, long *l, bool advance) {

	long tmp;
	uchar* ptr;
	
	ptr = (uchar *) & tmp;
	ptr[0] = **p;
	ptr[1] = *(*p + 1);
	ptr[2] = *(*p + 2);
	ptr[3] = *(*p + 3);
	
	(*l) = B_LENDIAN_TO_HOST_INT32(tmp);
	
	if (advance)
		(*p) += 4;
	return 4;
	
}


int
ulonglong_from_ptr(uchar** p, uint64* big, bool advance) {

	ulong low;
	ulong high;
	uchar*	hldr;
	int _tmp;
	
	hldr =  *p;
	
	ulong_from_ptr(p, & low, true);
	ulong_from_ptr(p, & high, true);
	
	(*big) = low;
	(*big) += (((uint64) high) << 32);	

	if (! advance) p -= 8;
	return 8;
}

int
longlong_from_ptr(uchar** p, int64* big, bool advance) {

	long low;
	long high;
	uchar*	hldr;
	int _tmp;
	
	hldr =  *p;
	
	long_from_ptr(p, & low, true);
	long_from_ptr(p, & high, true);
	
	(*big) = low;
	(*big) += (((int64) high) << 32);	

	if (! advance) p -= 8;
	return 8;
}

int  
char_to_ptr(uchar **p, char x) {

	**p = x;
	(*p)++;
	return 1;
}


int  
uchar_to_ptr(uchar **p, uchar x) {

	**p = (char) x;
	(*p)++;
	return 1;
}



int  
ushort_to_ptr(uchar **p, unsigned short s) {

	uchar* ptr;
	uint16 adjusted = B_HOST_TO_LENDIAN_INT16(s);
	
	ptr = (uchar *) & adjusted;
	
	**p = ptr[0];
	*(*p + 1) = ptr[1];
		
	(*p) += 2;
	return 2;
}


int
ulong_to_ptr(uchar **p, unsigned long l) {

	uchar* ptr;
	uint32 adjusted = B_HOST_TO_LENDIAN_INT32(l);
	
	ptr = (uchar *) & adjusted;
	
	**p = ptr[0];
	*(*p+1) = ptr[1];
	*(*p+2) = ptr[2];
	*(*p+3) = ptr[3];
	
	(*p) += 4;
	return 4;
}


int  
str_to_ptr(uchar **p, char *string) {

	int len = strlen(string) + 1;
	
	strcpy((char *) *p, string);
	
	(*p) += len;
	return len;
}

int
ulonglong_to_ptr(uchar **p, uint64 big) {

	uchar* ptr;
	uint64 adjusted = B_HOST_TO_LENDIAN_INT64(big);
	
	ptr = (uchar *) & adjusted;
	
	**p = ptr[0];
	*(*p+1) = ptr[1];
	*(*p+2) = ptr[2];
	*(*p+3) = ptr[3];
	*(*p+4) = ptr[4];
	*(*p+5) = ptr[5];
	*(*p+6) = ptr[6];
	*(*p+7) = ptr[7];
	
	(*p) += 8;
	return 8;
	
}



int  
unicode_to_ptr(uchar **p, char *ustring) {

	int i=0;
	while ( (*ustring != '\0') & ((*ustring+1) != '\0')) {
		**p = *ustring;
		(*p)++; ustring++;
		
		**p = *ustring;
		(*p)++; ustring++;
		i+=2;
	}
		
	**p = '\0';
	*(*p+1) = '\0';
	(*p) += 2;
	i+=2;
	
	return i;

}
	
int  
unicode_strlen(char *unicode_string) {

	int _tmp;
	int len = 0;
		
	//BUF_DUMP(unicode_string, 8);

	while ((*unicode_string != 0) | (*(unicode_string + 1) != 0)) {
		len++;
		unicode_string += 2;
	}
	return len;
}


char*
get_unicode_from_cstring(char *cstring) {

	int len = strlen(cstring) + 1;
	char *p;
	char *unicode_string;
	// = (char *) malloc(2*len);
	
	MALLOC(p, char*, (2*len));
	 //space for the unicode string
	
	unicode_string = p;
		
	while (*cstring) {
		*p++ = *cstring++;
		*p++ = '\0';
	}
	*p++ = '\0';
	*p++ = '\0';
	
	return unicode_string;
	
}



void  
get_cstring_from_smb_unicode(uchar *buf, char* cstring, const int maxlen) {

	char *p;
	int i = 0;

	// BUF_DUMP(uni_str, 50);
	p = cstring;

	while ( (*buf != 0) && (i < maxlen) ) {
		*p++ = *buf;
		buf += 2;
		i++;
	}

	*p = 0;

}


int
put_cifs_header(nspace*	vol, uchar*	buf) {

	uchar*		hldr;
	
	if (buf == NULL) {
		return B_ERROR;
	}
	
	hldr = buf;
	
	char_to_ptr(& hldr, 0xFF);
	char_to_ptr(& hldr, 'S');
	char_to_ptr(& hldr, 'M');
	char_to_ptr(& hldr, 'B');
	hldr += 5; // skip Command and Status Field
	
	uchar_to_ptr(& hldr, vol->Flags);
	ushort_to_ptr(& hldr, vol->Flags2);
	hldr += 12;  // skip pad field
	
	ushort_to_ptr(& hldr, vol->Tid);
	ushort_to_ptr(& hldr, vol->Pid);
	ushort_to_ptr(& hldr, vol->Uid);
	ushort_to_ptr(& hldr, vol->Mid);
	
	return B_OK;
	
}
	


status_t
translate_error(uchar errclass, ushort errcode) {

	status_t result;
	
	
	if ((errclass == 0) && (errcode == 0)) {
		return B_OK;
	}
	
	if (errclass == 0x1) {
		DPRINTF(-1, ("ErrDos:\t"));

		switch (errcode) {
	
		case ERR_INVALID_FUNCTION:
			DPRINTF(-1, ("Invalid Function\n"));
			result = B_ERROR;
			break;
					
		case ERR_FILE_NOT_FOUND:
			DPRINTF(-1, ("File not found\n"));
			result = B_ENTRY_NOT_FOUND;
			break;
		
	
		case ERR_ACCESS_DENIED:
			DPRINTF(-1, ("Access denied\n"));
			result = B_PERMISSION_DENIED;
			break;
		
		case ERR_BAD_FID:
			DPRINTF(-1, ("Invalid file handle\n"));
			result = ESTALE;
			break;
	
		case ERR_REM_CD:
			DPRINTF(-1, ("Cant remove the server's current directory\n"));
			result = EPERM;
			break;
	
		case ERR_FILE_EXISTS:
			DPRINTF(-1, ("File exists\n"));
			result = B_FILE_EXISTS;
			break;
		
		case ERR_FILE_ALREADY_EXISTS:
			DPRINTF(-1, ("File exists\n"));
			result = B_FILE_EXISTS;
			break;
			
		case ERR_BAD_PATH:
			DPRINTF(-1, ("Bad Path\n"));
			result = ENOTDIR;
			break;

		case ERR_BAD_SHARE:
			DPRINTF(-1, ("Bad Share\n"));
			result = EBUSY;
			break;
		


		default:
			DPRINTF(-1, ("show_error: got error %d(dec)\n", errcode));
			result = B_ERROR;
			break;
		
		}
		return result; // ErrDos

	} else if (errclass == 0x2) {
		DPRINTF(-1, ("ErrSrv:\t"));

		switch (errcode) {
	
		case ERR_BAD_PW:
			DPRINTF(-1, ("Bad name/password pair\n"));
			result = EACCES;
			break;

		case ERR_BAD_NET_NAME:
			DPRINTF(-1, ("Bad network name\n"));
			result = ENOENT;
			break;

		case ERR_NET_ACCESS_DENIED:
			debug(-1,("Network Access Denied\n"));
			result = EACCES;
			break;
		
		default:
			DPRINTF(-1, ("Unknown error %d(dec)\n", errcode));
			result = B_ERROR;
			break;
		
		}
		return result; // ErrSrv


	} else {
		DPRINTF(-1, ("Error : unknown : %x\n", errclass));
		return B_ERROR;
	}
	
	
}
	
#if 0
char *
getuser(char *prompt)
{
        static char buf[80];
        char *user;

        printf(prompt);
        buf[0] = 0;
        user = gets(buf);
        return (buf);
}


char *
getpass(const char *prompt)
{
        static char buf[80];
        struct termio save;
        struct termio t;
        char *pass;

        printf(prompt);
        fflush(stdout);
        ioctl(0, TCGETA, &t, 0);
        save = t;
        t.c_lflag &= ~ECHO;
        ioctl(0, TCSETA, &t, 0);
        buf[0] = 0;
        pass = gets(buf);
        ioctl(0, TCSETA, &save, 0);
        return (buf);
}

#endif



int
check_permission(vnode *vn, int omode)
{
	int     access_type, mask;
	mode_t  mode = vn->mode;
	uid_t   uid;
	gid_t   gid;

	uid = geteuid();
	gid = getegid();

	if (uid == 0)
		return 1;

	mask = (omode & O_ACCMODE);

	if (mask == O_RDONLY)
		access_type = S_IROTH;
	else if (mask == O_WRONLY)
		access_type = S_IWOTH;
	else if (mask == O_RDWR)
		access_type = (S_IROTH | S_IWOTH);
	else {
		DPRINTF(0, ("strange looking omode 0x%x\n", omode));
		access_type = S_IROTH;
	}

    if (uid == vn->uid)
        mode >>= 6;
    else if (gid == vn->gid)
        mode >>= 3;

    if (((mode & access_type & S_IRWXO) == access_type))
        return 1;

	return 0;
}

int
check_access(vnode *vn, int access_type)
{
	mode_t  mode = vn->mode;
	uid_t   uid;
	gid_t   gid;

	uid = geteuid();
	gid = getegid();

	if (uid == 0)
		return 1;

    if (uid == vn->uid)
        mode >>= 6;
    else if (gid == vn->gid)
        mode >>= 3;

    if (((mode & access_type & S_IRWXO) == access_type))
        return 1;

	return 0;
}


size_t
get_io_size(nspace *vol, size_t req) {
	
	size_t result;
		
	if (req <= vol->small_io_size) {
		result = req;
		return result;
	}			

	if (req <= vol->io_size) {
		result = min(req,vol->io_size);
		return result;
	}

	if (req <= (2*vol->io_size)) {
		result =  ( (req/2) + (req % 2));
		return result;
	}
		
	result =  vol->io_size;
	return result;	
	
}


// Useful functions for dealing with cifs

int_dir_entry*
find_in_dir(vnode *dir, vnode_id vnid) {
	int_dir_entry fake;
	
	if (dir->contents == NULL)
		return NULL;
	 
	fake.vnid = vnid;
	return (int_dir_entry*)SearchSL(dir->contents, (void*)&fake);
}

int_dir_entry*
next_in_dir(vnode *dir, dircookie *cookie) {
	int_dir_entry fake;
	
	if (dir->contents == NULL) {
		debug(-1, ("next_in_dir given null dir->contents"));
		return NULL;
	}
	fake.vnid = cookie->previous;
	return (int_dir_entry*)NextSL(dir->contents, (void*)&fake);
}

status_t
del_in_dir(vnode *dir, vnode_id vnid) {
	int_dir_entry fake;
	
	if (dir->contents == NULL)
		return B_ERROR;
	
	fake.vnid = vnid;
	return DeleteSL(dir->contents, (void*)&fake) ? B_OK : B_ERROR;
}

status_t
add_to_dir(vnode *dir, int_dir_entry *entry) {

	status_t result;
	if (dir->contents == NULL) {
		dir->contents = NewSL(_entry_compare, _entry_free, NO_DUPLICATES);
		if (dir->contents == NULL) return B_ERROR;
	}
	
	result = InsertSL(dir->contents, (void*)entry);
	if (result == TRUE) {
		return B_OK;
	} else {
		debug(-1, ("add_to_dir: couldnt add %s to skiplist\n", entry->filename));
		return B_ERROR;
	}
}





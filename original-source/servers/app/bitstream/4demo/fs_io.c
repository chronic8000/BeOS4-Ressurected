#include "be_glue.h"

/***********************************************************************************/

#ifndef SEEK_SET
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0
#endif

#define CACHE_ENABLED	1

/* Returns a pointer to data extracted from the sfnt. tt_get_font_fragment
   must be initialized in the fs_GlyphInputType record before fs_NewSfnt is called.  */

static long	dbref = 0;

void	fdebug1(char *format, ...)
{
	va_list args;
	char buf[255];

	if (dbref == 0) {
		dbref = open("/boot/log", O_RDWR | O_CREAT | O_BINARY);
	}
	
	va_start(args,format);
	vsprintf (buf, format, args);
	va_end(args);
	write(dbref, buf, strlen(buf)+1);
}

double	read_total = 0;

//----------------------------------------------------------------------

typedef	struct {
	int32	clientID;		//4
	int32	offset;			//8
	int32	length;			//12
	void	*data;			//16
	int16	use_count;		//18
} cache_entry;

//----------------------------------------------------------------------
#define	CACHE_SIZE	32		// need a power of 2
#define	MAX_LEN		65536
//----------------------------------------------------------------------

long		cache_inited = -32;
cache_entry	cache[CACHE_SIZE];

sem_id		fragc_sem;
long 		fragc_lock;

/*----------------------------------------------------------------*/

void	get_fragment_cache()
{
	if (atomic_add(&fragc_lock, 1) >= 1)
		acquire_sem(fragc_sem);	
}

/*----------------------------------------------------------------*/

void	release_fragment_cache()
{
	if (atomic_add(&fragc_lock, -1) > 1)
		release_sem(fragc_sem);
}	


//----------------------------------------------------------------------

void	init_fragment_cache()
{
	long	i;

	fragc_sem = create_sem(0, "font_fragment_sem");
	fragc_lock = 0;

	get_fragment_cache();
	for (i = 0; i < CACHE_SIZE; i++) {
		cache[i].clientID = -1;
		cache[i].offset = -1;
		cache[i].length = -1;	
		cache[i].data = 0;
		cache[i].use_count = 0;
	}
	cache_inited = 0;
	release_fragment_cache();
}

//----------------------------------------------------------------------

void	add_to_cache(int32 clientID, int32 offset, int32 length, void *data)
{
	int32			i0, i;
	cache_entry		*c;

//	if (length > MAX_LEN)
//		return;

	get_fragment_cache();
	
	i0 = rand() & (CACHE_SIZE-1);
	
	c = cache + i0;
	for (i = i0; i < CACHE_SIZE; i++) {
		if (c->use_count <= 0)
			goto out;
		c++;
	}
	
	c = cache;
	for (i = 0; i < i0; i++) {
		if (c->use_count <= 0)
			goto out;
		c++;
	}

	release_fragment_cache();
	fdebug1("failure\n");
	return;

out:
	if (c->length >= 0)
		free((char *)c->data);

	c->clientID = clientID;
	c->offset = offset;
	c->length = length;
	c->data = data;
	c->use_count = 1;
	release_fragment_cache();
}

//----------------------------------------------------------------------

void	fc_flush_cache(int32 clientID)
{
	int32			i;
	cache_entry		*c;

	get_fragment_cache();

	c = cache;
	for (i = 0; i < CACHE_SIZE; i++) {
		if (c->clientID == clientID) {
			free((char *)c->data);
			c->clientID = -1;
			c->offset = -1;
			c->length = -1;
			c->data = 0;
			c->use_count = 0;
		}
		c++;
	}
	
	release_fragment_cache();
}

//----------------------------------------------------------------------

void	debug_cache()
{
	long	total_size;
	long	i;

	total_size = 0;

	for (i = 0; i < CACHE_SIZE; i++)
		total_size += cache[i].length;

	fdebug1("total size=%ld\n", total_size);
}

//----------------------------------------------------------------------

void	release_cache_entry(void *ptr)
{
	long			i;
	cache_entry		*c;

	get_fragment_cache();

	c = cache;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (ptr == c->data) {
			c->use_count--;
			goto out;
		}
		c++;
	}

out:;
	release_fragment_cache();
} 

//----------------------------------------------------------------------

void	*try_cache(int32 clientID, int32 offset, int32 length)
{
	long			i;
	void			*ptr;
	cache_entry		*c;

	get_fragment_cache();

	c = cache;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (offset == c->offset) {
			if (length == c->length) {
				if (clientID == c->clientID) {
					c->use_count++;
					release_fragment_cache();
					return c->data;
				}
			}
		}
		c++;
	}
	release_fragment_cache();
	return 0;
}

//----------------------------------------------------------------------
float	xt = 0;

FUNCTION void *tt_get_font_fragment(int32 clientID, int32 offset, int32 length) {
	FILE  *fdes;
	ufix8 *ptr, *ptr2;
	int32 length2;

#if CACHE_ENABLED
	if (cache_inited < 0) {
		if (atomic_or(&cache_inited, 1) == -32)
			init_fragment_cache();
		else while (cache_inited < 0)
			snooze(5000);
	}

	ptr = try_cache(clientID, offset, length);
	if (ptr != 0)
		return ptr;
#endif

	fdes = (FILE *)clientID;
	ptr = (ufix8 *)malloc((size_t)length);
	
	if (ptr == NULL)
		return ((void *)NULL);
	if (fseek(fdes, offset, 0) != 0) {
		xprintf("Failure when readind font file (0)\n");
		memset(ptr, 0L, length);
	}
	else {
		ptr2 = ptr;
		length2 = length;
		while (length2 > 32768) {
			if (fread(ptr2,1,(size_t)32768,fdes) != 32768) {
			xprintf("Failure when readind font file (1)\n");
				memset(ptr, 0L, length);
				goto bad_read;
			}
			length2 -= 32768;
			ptr2 += 32768;
		}
		if (fread(ptr2,1,(size_t)length2,fdes) != length2) {
			xprintf("Failure when readind font file (2)\n");
			memset(ptr, 0L, length);
		}
	}
#if CACHE_ENABLED
	add_to_cache(clientID, offset, length, (void*)ptr);
#endif

bad_read:
	return((void *)ptr);
}

/* Returns the memory at pointer to the heap */
FUNCTION void tt_release_font_fragment(void *ptr) {
#if CACHE_ENABLED
	release_cache_entry(ptr);
#else
	free(ptr);
#endif
}

/* Returns the size of the font file */
FUNCTION int32 tt_get_font_size(int32 clientID) {
	int32 fileSize;

	if (fseek((FILE*)clientID, 0L, SEEK_END))
        return 0L;

	fileSize = ftell((FILE*)clientID);
	
	if (fileSize == -1L)
		fileSize = 0L;
	return fileSize;	
}

/* Read a fragment of a type1 font file */
FUNCTION fix31 t1_get_bytes(font_data *font, void *buffer, fix31 offset, fix31 length) {
	FILE		*fdes;
	ufix8		*ptr, *ptr2;
	int32		length2, rlength;
	int32		clientID;

#if 0		/* this code is leaking memory like hell !! */
	clientID = (int32)(((fc_renderer_context*)(font->ClientData))->font_fp);	
	if (cache_inited < 0) {
		if (atomic_or(&cache_inited, 1) == -32)
			init_fragment_cache();
		else while (cache_inited < 0)
			snooze(5000);
	}
	ptr = try_cache(clientID, offset, length);
	if (ptr != 0) {
    	memcpy(buffer, ptr, length);
		release_cache_entry(ptr);
    	return (length);
    }

	fdes = ((fc_renderer_context*)(font->ClientData))->font_fp;
	ptr = (ufix8 *)malloc((size_t)length);
	if (ptr == NULL)
		return 0;
	if (fseek(fdes, offset, 0) != 0)
		return 0;
	ptr2 = ptr;
	length2 = length;
	while (length2 > 32768) {
		rlength = fread(ptr2, 1, (size_t)32768, fdes);
		if (rlength != 32768) {
			length -= (length2-rlength);
			goto bad_read;
		}
		length2 -= 32768;
		ptr2 += 32768;
	}
	rlength = fread(ptr2,1,(size_t)length2,fdes);
	if (rlength != length2) {
		length -= (length2-rlength);
		goto bad_read;
	}
	add_to_cache(clientID, offset, length, (void*)ptr);
	memcpy(buffer, ptr, length);
	release_cache_entry(ptr);
	return length;

bad_read:
	memcpy(buffer, ptr, length);
	return length;
	
#else

	fdes = ((fc_renderer_context*)(font->ClientData))->font_fp;
	if (fseek(fdes, offset, 0) != 0)
		return 0;
	ptr2 = (ufix8 *)buffer;
	length2 = length;
	while (length2 > 32768) {
		rlength = fread(ptr2, 1, (size_t)32768, fdes);
		if (rlength != 32768)
			return length-(length2-rlength);
		length2 -= 32768;
		ptr2 += 32768;
	}
	length2 -= fread(ptr2,1,(size_t)length2,fdes);
	return length-length2;
#endif	
}

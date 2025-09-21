#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

#include <boot.h>
#include <fmap.h>

#if USE_DMALLOC
	#include <dmalloc.h>
#endif

#define DPRINTF(a) dprintf("fmap: "); dprintf a

#ifndef DEBUG
	#define ASSERT(c) ((void)0)
#else
	#define ASSERT(c) (!(c) ? _assert_(__FILE__,__LINE__,#c) : 0)

	int _assert_(char *a, int b, char *c)
	{
		DPRINTF(("tripped assertion in %s/%d (%s)\n", a, b, c));
		kernel_debugger("tripped assertion");
		return 0;
	}
#endif

#define DEVICE_PREFIX "disk/virtual/fmap/"

struct disk {
	char name[sizeof(DEVICE_PREFIX) + 8];
	char host_device[256];
	struct fmap_info *fmap;

	struct disk *next;
};

static struct disk *disks = NULL;

struct cookie {
	int		fd;
	struct disk *d;
};

static uint32
crc32(uchar *buff, int len)
{
	uint32 crc = 0xffffffff;
	static uint32 crc32tab[0x100];
	static bool initialized = FALSE;
	
	if (!initialized) {
		uint32 i, j, c;
		for (i=0;i<0x100;i++) {
			c = i;
			for (j=0;j<8;j++)
				c = (c & 1) ? ((c >> 1) ^ 0xedb88320) : (c >> 1);
			crc32tab[i] = c;
		}
		initialized = TRUE;
	}

	while (len--)
		crc = (crc >> 8) ^ crc32tab[(crc ^ *(buff++)) & 0xff];

	return crc ^ 0xffffffff;
}


static uint32
calculate_fmap_checksum(struct fmap_info *f)
{
	return crc32((uchar *)f + sizeof(uint32), f->size - sizeof(uint32));
}

static status_t
validate_disk(struct disk *d)
{
	if (!d) {
		panic("fmap: validate_disk failure\n");
		return B_ERROR;
	}

	if (calculate_fmap_checksum(d->fmap) != d->fmap->checksum) {
		DPRINTF(("faulty checksum detected for %s\n", d->name));
		return B_ERROR;
	}

	return B_OK;
}

static status_t
recurse(const char *path, status_t (*f)(void *, const char *),
		void *cookie, char *filename)
{
	status_t err = B_OK;
	DIR *dirp;
	struct dirent *de;

	dirp = opendir(path);
	if (!dirp) return ENOENT;
	while ((de = readdir(dirp)) != NULL) {
		char *fname;
		struct stat st;
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") ||
			!strcmp(de->d_name, "fmap"))
			continue;
		fname = malloc(strlen(path) + strlen(de->d_name) + 2);
		sprintf(fname, "%s/%s", path, de->d_name);
		strcpy(filename, fname);
		if (stat(fname, &st) < 0) continue;
		if (S_ISDIR(st.st_mode))
			err = recurse(fname, f, cookie, filename);
		else
			err = (*f)(cookie, fname);
		free(fname);
		if (err != B_OK) break;
	}
	closedir(dirp);

	return err;
}

static status_t
match_bios_id(void *bios_id, const char *path)
{
	int fd;
	uchar id = ~(*(uchar *)bios_id);

	if (strcmp(path + strlen(path) - 4, "/raw"))
		return 0;

	fd = open(path, 0);
	if (fd < 0)
		return 0;
	ioctl(fd, B_GET_BIOS_DRIVE_ID, &id, 1);
	close(fd);

	if (id == *(uchar *)bios_id)
		return 1;

	return 0;
}

/*********************************/

static status_t
_open(const char *name, uint32 mode, void **_cookie)
{
	struct disk *d;
	struct cookie *cookie;
	int fd;
	uchar id;

	for (d=disks;d;d=d->next)
		if (!strcmp(d->name, name))
			break;

	if (!d)
		return ENODEV;

	/* find device associated with this drive.
	 * do this now rather than at device initialization
	 * time due to load order ambiguities */

	if (!(d->host_device[0])) {
		id = d->fmap->bios_id;
		if (recurse("/dev/disk", match_bios_id, &id, d->host_device) != 1) {
			DPRINTF(("Could not find disk with BIOS id %2.2x\n", id));
			return ENODEV;
		}
	}

	if (validate_disk(d) != B_OK)
		return B_ERROR;

	fd = open(d->host_device, mode);
	if (fd < 0)
		return fd;

	cookie = malloc(sizeof(*cookie));
	if (!cookie) {
		close(fd);
		return ENOMEM;
	}

	cookie->fd = fd;
	cookie->d = d;

	*_cookie = (void *)cookie;

#if 0
	DPRINTF(("bios id %lx (%s), partition offset %Lx\n", \
			d->fmap->bios_id, d->host_device, d->fmap->offset));
	if (d->fmap->type == FMAP_TYPE_BLOCK) {
		uint32 i, j = 0;
		for (i=0;i<d->fmap->u.block.num_block_runs;i++)
			j += d->fmap->u.block.block_runs[i].num_blocks;
		DPRINTF(("%lx block runs, %lx bytes/block, %lx blocks\n", \
				d->fmap->u.block.num_block_runs, \
				d->fmap->u.block.block_size,j));
	} else if (d->fmap->type == FMAP_TYPE_BYTE) {
		uint32 i;
		off_t j = 0;
		for (i=0;i<d->fmap->u.byte.num_byte_runs;i++)
			j += d->fmap->u.byte.byte_runs[i].num_bytes;
		DPRINTF(("%lx byte runs, %Lx bytes\n", \
				d->fmap->u.byte.num_byte_runs,j));
	}
#endif

	return B_OK;
}

static status_t
_close(void *_cookie)
{
	return B_OK;
}

static status_t
_free(void *_cookie)
{
	struct cookie *cookie = (struct cookie *)_cookie;

	if (validate_disk(cookie->d) != B_OK)
		return B_ERROR;

	close(cookie->fd);
	free(cookie);
	
	return B_OK;
}

static off_t
total_bytes(struct fmap_info *f)
{
	uint32 i;
	if (f->type == FMAP_TYPE_BLOCK) {
		int32 blocks = 0;
		for (i=0;i<f->u.block.num_block_runs;i++)
			blocks += f->u.block.block_runs[i].num_blocks;
		return (off_t)blocks * (off_t)f->u.block.block_size;
	} else if (f->type == FMAP_TYPE_BYTE) {
		off_t bytes = 0;
		for (i=0;i<f->u.byte.num_byte_runs;i++)
			bytes += f->u.byte.byte_runs[i].num_bytes;
		return bytes;
	} else
		panic("fmap/total_bytes: unknown type %x\n", f->type);

	return 0;
}

static status_t
_ioctl(void *_cookie, uint32 op, void *data, size_t len)
{
	struct cookie *cookie = (struct cookie *)_cookie;
	struct fmap_info *f = cookie->d->fmap;

	if (validate_disk(cookie->d) != B_OK)
		return B_ERROR;

	switch (op) {
		case B_GET_DEVICE_SIZE :
			*(size_t *)data = total_bytes(f);
			return B_OK;

		case B_GET_GEOMETRY :
		{
			device_geometry *g = (device_geometry *)data;
			g->bytes_per_sector = 0x200;
			g->sectors_per_track = 1;
			g->cylinder_count = total_bytes(f) / 0x200;
			g->head_count = 1;
			g->device_type = B_DISK;
			g->removable = FALSE;
#if __RO__
			g->read_only = TRUE;
#else
			g->read_only = FALSE;
#endif
			g->write_once = FALSE;
			return B_OK;
		}

		case B_GET_ICON :
		case B_GET_MEDIA_STATUS :
		case B_FLUSH_DRIVE_CACHE :
			return ioctl(cookie->fd, op, data, len);
	}
	return ENOSYS;
}

static status_t
_read(void *cookie, off_t pos, void *buffer, size_t *len)
{
	static status_t _readv(
			void *cookie, off_t pos, const struct iovec *vec, size_t count, size_t *bytes);

	struct iovec v;
	v.iov_base = buffer;
	v.iov_len = *len;

	return _readv(cookie, pos, &v, 1, len);
}

static status_t
_write(void *cookie, off_t pos, const void *buffer, size_t *len)
{
#if __RO__
	*len = 0;
	return B_READ_ONLY_DEVICE;
#else
	static status_t _writev(
			void *cookie, off_t pos, const struct iovec *vec, size_t count, size_t *bytes);

	struct iovec v;
	v.iov_base = (void *)buffer;
	v.iov_len = *len;

	return _writev(cookie, pos, &v, 1, len);
#endif
}

static status_t
_readv_writev_block(struct cookie *cookie, off_t pos, const struct iovec *vec, 
		size_t count, size_t *bytes,
		ssize_t (*g)(int, off_t, void *, size_t),
		ssize_t (*gv)(int, off_t, const struct iovec *, size_t))
{
	struct fmap_info *f = cookie->d->fmap;

	/* status information */
	status_t error = B_OK;
	size_t bytes_processed = 0;

	/* position in block map */
	int32 brun_index;
	off_t brun_offset;	/* byte offset in current block run */

	/* position in iovec */
	int32 iov_index;
	size_t iov_offset;	/* byte offset in current iovec */

	if (pos < 0) {
		DPRINTF(("readv/writev called with negative pos\n"));
		return EINVAL;
	}

	/* first find position in virtual disk */
	{
		off_t off = 0;

		for (brun_index=0;brun_index<f->u.block.num_block_runs;brun_index++) {
			if (off + (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size > pos)
				break;
			off += (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size;
		}

		if (brun_index == f->u.block.num_block_runs) {
			*bytes = 0;
			return B_OK;
		}

		brun_offset = pos - off;
	}

	/* begin the reading and writing */
	iov_index = 0; iov_offset = 0;
	while (iov_index < count) {
		off_t apos;
		ssize_t alen, s;

		if (iov_offset >= vec[iov_index].iov_len) {
			iov_offset -= vec[iov_index].iov_len;
			iov_index++;
			continue;
		}

//		ASSERT(brun_offset <= (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size);

		if (brun_offset >= (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size) {
			brun_offset -= (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size;
			brun_index++;
			if (brun_index >= f->u.block.num_block_runs)
				break;
			continue;
		}

		apos = f->offset +
				(off_t)f->u.block.block_runs[brun_index].block * f->u.block.block_size + brun_offset;
		alen = vec[iov_index].iov_len - iov_offset;

		if ((iov_offset == 0) &&
			(alen <= (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size -
					brun_offset)) {
			int32 i;
			for (i=1;i+iov_index<count;i++) {
				s = alen + vec[iov_index+i].iov_len;
				if (s > (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size - brun_offset)
					break;
				alen = s;
			}
			if (i == 1)
				s = (*g)(cookie->fd, apos, vec[iov_index].iov_base,
						vec[iov_index].iov_len);
			else
				s = (*gv)(cookie->fd, apos, vec + iov_index, i);
		} else {
			if (alen > (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size -
					brun_offset)
				alen = (off_t)f->u.block.block_runs[brun_index].num_blocks * f->u.block.block_size -
						brun_offset;
	
			s = (*g)(cookie->fd, apos,
					(uchar *)(vec[iov_index].iov_base) + iov_offset, alen);
		}

		if (s < alen) {
			if (s < 0) {
				error = s;
			} else {
				bytes_processed += s;
				error = B_OK;
			}
			break;
		}

		bytes_processed += alen;
		iov_offset += alen;
		brun_offset += alen;
	}

	*bytes = bytes_processed;
	return error;
}

static status_t
_readv_writev_byte(struct cookie *cookie, off_t pos, const struct iovec *vec, 
		size_t count, size_t *bytes,
		ssize_t (*g)(int, off_t, void *, size_t),
		ssize_t (*gv)(int, off_t, const struct iovec *, size_t))
{
	struct fmap_info *f = cookie->d->fmap;

	/* status information */
	status_t error = B_OK;
	size_t bytes_processed = 0;

	/* position in byte map */
	int32 brun_index;
	off_t brun_offset;	/* byte offset in current byte run */

	/* position in iovec */
	int32 iov_index;
	size_t iov_offset;	/* byte offset in current iovec */

	/* first find position in virtual disk */
	{
		off_t off = 0;

		for (brun_index=0;brun_index<f->u.byte.num_byte_runs;brun_index++) {
			if (off + f->u.byte.byte_runs[brun_index].num_bytes > pos)
				break;
			off += f->u.byte.byte_runs[brun_index].num_bytes;
		}

		if (brun_index == f->u.byte.num_byte_runs) {
			*bytes = 0;
			return B_OK;
		}

		brun_offset = pos - off;
	}

	/* begin the reading and writing */
	iov_index = 0; iov_offset = 0;
	while (iov_index < count) {
		off_t apos;
		ssize_t alen, s;

		if (iov_offset >= vec[iov_index].iov_len) {
			iov_offset -= vec[iov_index].iov_len;
			iov_index++;
			continue;
		}

//		ASSERT(brun_offset <= f->u.byte.byte_runs[brun_index].num_bytes);

		if (brun_offset >= f->u.byte.byte_runs[brun_index].num_bytes) {
			brun_offset -= f->u.byte.byte_runs[brun_index].num_bytes;
			brun_index++;
			if (brun_index >= f->u.byte.num_byte_runs)
				break;
			continue;
		}

		apos = f->offset +
				f->u.byte.byte_runs[brun_index].byte + brun_offset;
		alen = vec[iov_index].iov_len - iov_offset;

		if ((iov_offset == 0) &&
			(alen <= f->u.byte.byte_runs[brun_index].num_bytes - brun_offset)) {
			int32 i;
			for (i=1;i+iov_index<count;i++) {
				s = alen + vec[iov_index+i].iov_len;
				if (s > f->u.byte.byte_runs[brun_index].num_bytes - brun_offset)
					break;
				alen = s;
			}
			if (i == 1)
				s = (*g)(cookie->fd, apos, vec[iov_index].iov_base,
						vec[iov_index].iov_len);
			else
				s = (*gv)(cookie->fd, apos, vec + iov_index, i);
		} else {
			if (alen > f->u.byte.byte_runs[brun_index].num_bytes - brun_offset)
				alen = f->u.byte.byte_runs[brun_index].num_bytes - brun_offset;

			s = (*g)(cookie->fd, apos,
					(uchar *)(vec[iov_index].iov_base) + iov_offset, alen);
		}

		if (s < alen) {
			if (s < 0) {
				error = s;
			} else {
				bytes_processed += s;
				error = B_OK;
			}
			break;
		}

		bytes_processed += alen;
		iov_offset += alen;
		brun_offset += alen;
	}

	*bytes = bytes_processed;
	return error;
}

static status_t
_readv_writev(struct cookie *cookie, off_t pos, const struct iovec *vec, 
		size_t count, size_t *bytes,
		ssize_t (*g)(int, off_t, void *, size_t),
		ssize_t (*gv)(int, off_t, const struct iovec *, size_t))
{
	if (validate_disk(cookie->d) != B_OK)
		return B_ERROR;

	if (cookie->d->fmap->type == FMAP_TYPE_BLOCK)
		return _readv_writev_block(cookie, pos, vec, count, bytes, g, gv);
	else if (cookie->d->fmap->type == FMAP_TYPE_BYTE)
		return _readv_writev_byte(cookie, pos, vec, count, bytes, g, gv);

	return B_ERROR;
}

static status_t
_readv(void *_cookie, off_t pos, const struct iovec *vec, size_t count, size_t *bytes)
{
	return _readv_writev(_cookie, pos, vec, count, bytes, read_pos, readv_pos);
}

static status_t
_writev(void *_cookie, off_t pos, const struct iovec *vec, size_t count, size_t *bytes)
{
#if __RO__
	*bytes = 0;
	return B_READ_ONLY_DEVICE;
#else
	return _readv_writev(_cookie, pos, vec, count, bytes, write_pos, writev_pos);
#endif
}

device_hooks hooks = {
	&_open,
	&_close,
	&_free,
	&_ioctl,
	&_read,
	&_write,
	NULL,
	NULL,
	&_readv,
	&_writev
};

/***************************/

status_t init_hardware()
{
	return B_OK;
}

static char **devices = NULL;

const char **publish_devices()
{
	return (const char **)devices;
}

device_hooks *find_device(const char *name)
{
	return &hooks;
}

static void free_resources()
{
	struct disk *d, *e;
	int32 i;

	for (d=disks;d;d=e) {
		e = d->next;
		free(d);
	}
	for (i=0;devices[i];i++)
		free(devices[i]);
	free(devices);
}

#include <driver_settings.h>

status_t init_driver()
{
	int i, j;
	struct disk *d, *p;

	{
		void *handle;
		bool result;
		handle = load_driver_settings("fmap");
		result = get_driver_boolean_parameter(handle, "disabled", false, true);
		unload_driver_settings(handle);
		if (result) {
			DPRINTF(("disabled due to driver setting\n"));
			return B_ERROR;
		}
	}

	disks = p = NULL;

	for (i=0;;i++) {
		struct fmap_info *fmap;
		char name[32];
		uint32 checksum;

		sprintf(name, BOOT_FMAP_DISK_PREFIX "%d", i);
		(void *)fmap = get_boot_item(name);
		if (!fmap)
			break;

		if (	(get_boot_item_size(name) <
						sizeof(*fmap) - sizeof(struct fmap_byte_run)) ||
				(fmap->size != get_boot_item_size(name))) {
			DPRINTF(("invalid size detected for disk %d\n", i));
			return B_ERROR;
		}

		if (fmap->type == FMAP_TYPE_BLOCK) {
			if (fmap->u.block.num_block_runs < 0) {
				DPRINTF(("negative block runs detected for disk %d\n", i));
				return B_ERROR;
			}

			if (fmap->size < sizeof(*fmap) + (fmap->u.block.num_block_runs - 1) * sizeof(struct fmap_block_run)) {
				DPRINTF(("incomplete block fmap detected for disk %d\n", i));
				return B_ERROR;
			}

			if (fmap->size > sizeof(*fmap) + (fmap->u.block.num_block_runs - 1) * sizeof(struct fmap_block_run)) {
				DPRINTF(("overzealous block fmap detected for disk %d\n", i));
				return B_ERROR;
			}
		} else if (fmap->type == FMAP_TYPE_BYTE) {
			if (fmap->u.byte.num_byte_runs < 0) {
				DPRINTF(("negative byte runs detected for disk %d\n", i));
				return B_ERROR;
			}

			if (fmap->size < sizeof(*fmap) + (fmap->u.byte.num_byte_runs - 1) * sizeof(struct fmap_byte_run)) {
				DPRINTF(("incomplete byte fmap detected for disk %d\n", i));
				return B_ERROR;
			}

			if (fmap->size > sizeof(*fmap) + (fmap->u.byte.num_byte_runs - 1) * sizeof(struct fmap_byte_run)) {
				DPRINTF(("overzealous byte fmap detected for disk %d\n", i));
				return B_ERROR;
			}
		} else {
			DPRINTF(("invalid type field detected for disk %d\n", i));
			return B_ERROR;
		}

		checksum = calculate_fmap_checksum(fmap);
		if (checksum != fmap->checksum) {
			DPRINTF(("faulty checksum detected for disk %d\n", i));
			return B_ERROR;
		}

		if (fmap->type == FMAP_TYPE_BLOCK) {
			int32 blocks = 0;
			for (j=0;j<fmap->u.block.num_block_runs;j++) {
				if (blocks + fmap->u.block.block_runs[j].num_blocks < blocks) {
					DPRINTF(("block run overflow detected for disk %d\n", i));
					return B_ERROR;
				}
				blocks += fmap->u.block.block_runs[j].num_blocks;
			}
			if (blocks != fmap->u.block.num_blocks) {
				DPRINTF(("block count mismatch (%lx != %lx)\n", \
						blocks, fmap->u.block.num_blocks));
				return B_ERROR;
			}
		} else if (fmap->type == FMAP_TYPE_BYTE) {
			off_t bytes = 0;
			for (j=0;j<fmap->u.byte.num_byte_runs;j++) {
				if (bytes + fmap->u.byte.byte_runs[j].num_bytes < bytes) {
					DPRINTF(("byte run overflow detected for disk %d\n", i));
					return B_ERROR;
				}
				bytes += fmap->u.byte.byte_runs[j].num_bytes;
			}
			if (bytes != fmap->u.byte.num_bytes) {
				DPRINTF(("byte count mismatch (%Lx != %Lx)\n", \
						bytes, fmap->u.byte.num_bytes));
				return B_ERROR;
			}
		}

		d = malloc(sizeof(*d));
		sprintf(d->name, DEVICE_PREFIX "%d/raw", i);
		d->host_device[0] = 0;
		d->fmap = fmap;
		d->next = NULL;
		if (!p)
			disks = d;
		else
			p->next = d;
		p = d;
	}

	if (!disks)
		return ENODEV;

	devices = calloc(i + 1, sizeof(char *));
	if (!devices) {
		free_resources();
		return ENOMEM;
	}
	for (d=disks,i=0;d;d=d->next,i++) {
		devices[i] = strdup(d->name);
		if (!devices[i]) {
			free_resources();
			return ENOMEM;
		}
	}
	devices[i] = NULL;

	return B_OK;
}

void uninit_driver()
{
	free_resources();
}

int32 api_version = B_CUR_DRIVER_API_VERSION;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <module.h>
#include <ByteOrder.h>

_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;

// ***
// Constants and Enums
// ***

static const char *kDeviceName = "disk/virtual/ram/0";

static const char *kDevices[] = {
	"disk/virtual/ram/0",
	NULL
};

// ***
// Structs
// ***

typedef struct dev_info
{
	area_id			area;
	void			*base;
	size_t			size;
} dev_info_t;

// Driver Entry Points
__declspec(dllexport) status_t init_hardware( void );
__declspec(dllexport) status_t init_driver( void );
__declspec(dllexport) void uninit_driver( void );
__declspec(dllexport) const char** publish_devices( void );
__declspec(dllexport) device_hooks *find_device( const char *name );

// Driver Hook Functions
static status_t open_hook( const char *name, uint32 flags, void **cookie );
static status_t close_hook( void *cookie );
static status_t free_hook( void *cookie );
static status_t control_hook( void *cookie, uint32 opcode, void *data, size_t length );
static status_t read_hook( void *cookie, off_t position, void *data, size_t *length );
static status_t write_hook( void *cookie, off_t position, const void *data, size_t *length );
static status_t select_hook( void *cookie, uint8 event, uint32 ref, selectsync *sync );
static status_t deselect_hook( void *cookie, uint8 event, selectsync *sync );
static status_t readv_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes );
static status_t writev_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes );

// ***
// Globals
// ***
static device_hooks gDeviceHooks = {
	open_hook,			// -> open entry point
	close_hook,			// -> close entry point
	free_hook,			// -> free cookie
	control_hook,		// -> control entry point
	read_hook,			// -> read entry point
	write_hook,			// -> write entry point
	select_hook,		// -> select
	deselect_hook,		// -> deselect
	readv_hook,			// -> readv
	writev_hook			// -> writev
};

dev_info_t		gDevice;

// ***
// Driver Hook Functions
// ***

status_t init_hardware( void )
{
	return B_OK;
}

status_t init_driver( void )
{
	memset( &gDevice, 0, sizeof(gDevice) );
	if( (gDevice.area = find_area(kDeviceName)) >= 0 )
	{
		area_info	ainfo;
		
		get_area_info( gDevice.area, &ainfo );
		gDevice.size = ainfo.size;
		gDevice.base = ainfo.address;
	}
	return B_OK;
}

void uninit_driver( void )
{
	
}

const char** publish_devices( void )
{
	return kDevices;
}

device_hooks *find_device( const char *name )
{
	if( strcmp( kDeviceName, name ) == 0 )
		return &gDeviceHooks;
	else
		return NULL;
}

static status_t open_hook( const char *name, uint32 flags, void **cookie )
{
	*cookie = &gDevice;
	
	return B_OK;
}

static status_t close_hook( void *cookie )
{
	return B_OK;
}

static status_t free_hook( void *cookie )
{
	return B_OK;
}

static status_t control_hook( void *cookie, uint32 opcode, void *data, size_t length )
{
	switch( opcode )
	{
		case B_GET_DEVICE_SIZE:
			return gDevice.size;
			
		case B_SET_DEVICE_SIZE:
			if( gDevice.area >= 0 && *((size_t *)data) == 0 )
			{
				delete_area( gDevice.area );
				gDevice.area = -1;
				gDevice.size = 0;
				return B_OK;
			}
			else if( gDevice.area < 0 )
			{
				if( (gDevice.area = create_area( kDeviceName, &gDevice.base, B_ANY_KERNEL_ADDRESS, *((size_t *)data), B_SLOWMEM, B_READ_AREA | B_WRITE_AREA )) < 0 )
					return gDevice.area;
				gDevice.size = *((size_t *)data);
				return B_OK;
			}
			else
				return B_ERROR;
			
		case B_GET_GEOMETRY:
		{
			device_geometry *geometry = (device_geometry *)data;
			
			if( gDevice.area < 0 )
				return B_ERROR;
			
			geometry->bytes_per_sector = 512;
			geometry->sectors_per_track = gDevice.size >> 9;
			geometry->cylinder_count = 1;
			geometry->head_count = 1;
			geometry->device_type = B_DISK;
			geometry->removable = false;
			geometry->read_only = false;
			geometry->write_once = false;
			return B_OK;
		}
		case B_SET_NONBLOCKING_IO:
		case B_SET_BLOCKING_IO:
		case B_GET_READ_STATUS:
		case B_GET_WRITE_STATUS:
		case B_FLUSH_DRIVE_CACHE:
		case B_FORMAT_DEVICE:
			return B_OK;
		
		case B_GET_ICON:
			
			return B_ERROR;
		default:
			return B_ERROR;
	}
	return B_ERROR;
}

static status_t read_hook( void *cookie, off_t position, void *data, size_t *length )
{
	if( position + *length > gDevice.size )
	{
		*length = 0;
		return B_IO_ERROR;
	}
	
	memcpy( data, ((char *)gDevice.base) + position, *length );
	return B_OK;
}

static status_t write_hook( void *cookie, off_t position, const void *data, size_t *length )
{
	if( position + *length > gDevice.size )
	{
		*length = 0;
		return B_IO_ERROR;
	}
	
	memcpy( ((char *)gDevice.base) + position, data, *length );
	return B_OK;
}

status_t select_hook( void *cookie, uint8 event, uint32 ref, selectsync *sync )
{
	return B_ERROR;
}

status_t deselect_hook( void *cookie, uint8 event, selectsync *sync )
{
	return B_ERROR;
}

status_t readv_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes )
{
	int32	i;
	size_t	offset;
	size_t	length;
	
	for( i=0, offset = 0; i<count; offset += vec[i].iov_len, i++ )
	{
		if( position + offset + vec[i].iov_len > gDevice.size )
			break;
		memcpy( vec[i].iov_base, ((char *)gDevice.base) + position + offset, vec[i].iov_len );
	}
	*numBytes = offset;
	return B_OK;
}

status_t writev_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes )
{
	int32	i;
	size_t	offset;
	size_t	length;
	
	for( i=0, offset = 0; i<count; offset += vec[i].iov_len, i++ )
	{
		if( position + offset + vec[i].iov_len > gDevice.size )
			break;
		memcpy( ((char *)gDevice.base) + position + offset, vec[i].iov_base, vec[i].iov_len );
	}
	*numBytes = offset;
	return B_OK;
}

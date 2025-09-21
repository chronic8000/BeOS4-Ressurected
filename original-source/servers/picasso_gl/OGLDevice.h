
#ifndef OGLDEVICE_H
#define OGLDEVICE_H

#include "TextureCache.h"
#include "RasterDevice.h"
#include "GLDefines.h"
#include "OGLPlane.h"


class BOGLDevice : public BRasterDevice, public TextureCache
{
	public:
										BOGLDevice();
		virtual							~BOGLDevice();

				status_t 				EnumerateDevices(
											uint32 monitor,
											uint32 min_color,
											uint32 min_depth,
											uint32 min_stencil,
											uint32 min_accum );
											
		virtual	void 					DeviceInfo(
											uint32 device_id,
											uint32 monitor,
											const char *name );
											
		virtual	void 					VideoModeInfo(
											uint32 mode_id,
											uint32 width, uint32 height,
											uint32 color, uint32 depth, uint32 stencil, uint32 accum
											);

		// Context Management
		virtual lock_status_t			Context_Lock();
		virtual void					Context_Unlock();
	
		// Plane management
		virtual	status_t				Plane_Create(
											int32 width, int32 height, uint32 flags,
											uint32 color, uint32 depth, uint32 stencil, uint32 accum,
											BRasterPlane **outPlane );
		virtual status_t				Plane_Destroy( BRasterPlane *plane );
		virtual status_t				Plane_Select( BRasterPlane *plane );
		virtual	status_t				Plane_Display( BRasterPlane *plane );


		// Texture Management
		// Cheap calls
		
		// Note, stide is in pixels and NOT bytes
		virtual status_t				Texture_SubLoad( uint32 width, uint32 height, uint32 xoff, uint32 yoff, color_space format, const void *data, uint32 stride=0 );
		virtual status_t				Texture_Select( uint32 texture_id );
		virtual status_t				Texture_Enable( bool enable );
		virtual status_t				Texture_GetSize( uint32 *w, uint32 *h );
		virtual status_t				Texture_FilterEnable( bool enable );

		// Expensive Calls		
		virtual status_t				Texture_Create( uint32 *texture_id );
		virtual status_t				Texture_Load( uint32 width, uint32 height, color_space format, const void *data, uint32 stride=0 );
		virtual void					Texture_Release( uint32 texture_id );
		virtual status_t				Texture_Get( uint32 width, uint32 height, uint32 xoff, uint32 yoff, color_space format, void *data, uint32 stride=0 );
		virtual status_t				Texture_SetTexel( uint32 x, uint32 y, color_space format, const void *data );
		virtual status_t				Texture_GetTexel( uint32 x, uint32 y, color_space format, void *data );
		
				void					ColorspaceToGL( color_space cs, uint32 *type, uint32 *format, uint32 *internal_format );
	private:
		
				uint32					Texture_GetGetSize( uint32 type, uint32 format );
				uint32					GetBytePP( uint32 type, uint32 format );

				bool					textureEnabled;
				bool					nonZeroStrideEnabled;
};

extern BOGLDevice OGLDevice;

#endif

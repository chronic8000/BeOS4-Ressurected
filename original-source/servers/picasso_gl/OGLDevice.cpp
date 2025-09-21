
#include "OGLDevice.h"
#include "input.h"
#include "mini.h"
#include "server.h"

BOGLDevice OGLDevice;

AppServerWindow * asWindow = NULL;
static void setProjMatrix( uint32 height, uint32 width );

status_t
BOGLDevice::Plane_Create(
		int32 width, int32 height, uint32 /*flags*/,
		uint32 color, uint32 depth, uint32 stencil, uint32 accum,
		BRasterPlane **outPlane)
{
	if (asWindow) return B_ERROR;

	BRect r(0,0,width,height);
	r.OffsetBy(100,100);
	asWindow = new AppServerWindow(r,"picasso");
	asWindow->Show();
	asWindow->InitializeGL( 0, color, depth, stencil, accum );

	asWindow->oglLock();
	setProjMatrix( width, height );
	glClearColor( 1,1,1,1 );
	glClear( GL_COLOR_BUFFER_BIT );
	asWindow->oglUnlock();


	BOGLPlane *plane = new BOGLPlane;
	*outPlane = plane;

	plane->m_Width = width;
	plane->m_Height = height;

	return B_OK;
}

status_t 
BOGLDevice::Plane_Display(BRasterPlane *)
{
	return B_OK;
}

BOGLDevice::BOGLDevice() : TextureCache()
{
}

BOGLDevice::~BOGLDevice()
{

}

status_t BOGLDevice::EnumerateDevices(
	uint32 monitor, uint32 min_color, uint32 min_depth,
	uint32 min_stencil, uint32 min_accum )
{
	printf( "BOGLDevice::EnumerateDevices  mon=%lx  color=%lx  depth=%lx,  stencil=%lx, accum=%lx\n",
		monitor, min_color, min_depth, min_stencil, min_accum );

	return B_ERROR;
}
											
void BOGLDevice::DeviceInfo( uint32 device_id, uint32 monitor, const char *name )
{
	printf( "BOGLDevice::DeviceInfo  %s,  id=%li, mon=%li \n", name, device_id, monitor );
}
											
void BOGLDevice::VideoModeInfo( uint32 /*mode_id*/, uint32 /*width*/, uint32 /*height*/,
	uint32 /*color*/, uint32 /*depth*/, uint32 /*stencil*/, uint32 /*accum*/ )
{
}
											

///////////////////////////////////////////////////////


static void setProjMatrix( uint32 width, uint32 height )
{
	float h = height;
	float w = width;
	
	glMatrixMode( GL_PROJECTION );

	float m[16];
	int ct;
	for( ct=0; ct<16; ct++ )
		m[ct] = 0;
	m[0] = 2.0 / w;
	m[12] = -1.0;
	
	m[5] = -2.0 / h;
	m[13] = 1.0;
	
	m[15] = 1.0;
	glLoadMatrixf( m );
	glMatrixMode( GL_MODELVIEW );
}


static void setTextureMatrix( uint32 width, uint32 height )
{
	float h = height;
	float w = width;
	
	glMatrixMode( GL_TEXTURE );
	float m[16];
	int ct;
	for( ct=0; ct<16; ct++ )
		m[ct] = 0;
	m[0] = 1.0 / width;
	m[5] = 1.0 / height;
	m[15] = 1.0;
	glLoadMatrixf( m );
	glMatrixMode( GL_MODELVIEW );
}

status_t BOGLDevice::Texture_Create( uint32 *id )
{
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	glGenTextures( 1, id );

	if( !current )
		asWindow->oglUnlock();
		
	return B_OK;
}

status_t BOGLDevice::Texture_Load( uint32 width, uint32 height, color_space cs, const void *data, uint32 stride )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	uint32 type;
	uint32 format;
	uint32 internal_format;
	ColorspaceToGL( cs, &type, &format, &internal_format );
	if( type )
	{
		if( stride )
		{
			if( !nonZeroStrideEnabled )
			{
				nonZeroStrideEnabled = true;
				glPixelStorei( GL_UNPACK_ROW_LENGTH, stride );
			}
		}
		else
		{
			if( nonZeroStrideEnabled )
			{
				nonZeroStrideEnabled = false;
				glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
			}
		}
		glTexImage2D( GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data );
	}
	else
	{
		ret = B_ERROR;
	}

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	
	setTextureMatrix( width, height );

	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

status_t BOGLDevice::Texture_Select( uint32 texture_id )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	glBindTexture( GL_TEXTURE_2D, texture_id );

	uint32 w, h;
	Texture_GetSize( &w, &h );
	setTextureMatrix( w, h );

	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

void BOGLDevice::Texture_Release( uint32 texture_id )
{
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	glDeleteTextures( 1, &texture_id );

	if( !current )
		asWindow->oglUnlock();
}

status_t BOGLDevice::Texture_SubLoad( uint32 width, uint32 height, uint32 xoff, uint32 yoff, color_space cs, const void *data, uint32 stride=0 )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	uint32 type;
	uint32 format;
	uint32 internal_format;
	
	ColorspaceToGL( cs, &type, &format, &internal_format );

	if( type )
	{
		if( stride )
		{
			if( !nonZeroStrideEnabled )
			{
				nonZeroStrideEnabled = true;
				glPixelStorei( GL_UNPACK_ROW_LENGTH, stride );
			}
		}
		else
		{
			if( nonZeroStrideEnabled )
			{
				nonZeroStrideEnabled = false;
				glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
			}
		}
		glTexSubImage2D( GL_TEXTURE_2D, 0, xoff, yoff, width, height, format, type, data );
	}
	else
	{
		ret = B_ERROR;
	}


	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

status_t BOGLDevice::Texture_FilterEnable( bool enable )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	if( enable )
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	else
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

status_t BOGLDevice::Texture_Get( uint32 width, uint32 height, uint32 xoff, uint32 yoff, color_space cs, void *dst_data, uint32 stride=0 )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	uint32 type;
	uint32 format;
	uint32 internal_format;
	uint32 w, h;
	uint32 bpp;
	uint32 size;
	uint8 *src_data;
	uint32 x,y;
	
	
	ColorspaceToGL( cs, &type, &format, &internal_format );
	if( !type )
	{
		ret = B_ERROR;
		goto err;
	}


	bpp = GetBytePP( type, format );
	if( !stride )
	{
		stride = ((width*bpp)+3) & (~3);
	}

	size = Texture_GetGetSize( type, format );
	if( !size )
	{
		ret = B_ERROR;
		goto err;
	}
	
	Texture_GetSize( &w, &h );
	if( ((width+xoff) > w) || ((height+yoff) > h ))
	{
		ret = B_ERROR;
		goto err;
	}
	
	
	src_data = new uint8[size];

	glGetTexImage( GL_TEXTURE_2D, 0, format, type, src_data );

	// Copy out the part requested
	for( y=0; y<height; y++ )
	{
		uint8 *src = &src_data[ (((w*bpp)+3) & (~3)) * (y+yoff) + x*bpp ];
		uint8 *dst = &((uint8 *)dst_data)[ y*stride ];
		uint8 count = width*bpp;
		while( count-- )
		{
			*dst++ = *src++;
		}
	}
	

	delete[] src_data;

	err:
	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

status_t BOGLDevice::Texture_GetSize( uint32 *w, uint32 *h )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	GLint t;
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &t );
	*w = t;

	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &t );
	*h = t;


	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

status_t BOGLDevice::Texture_SetTexel( uint32 x, uint32 y, color_space format, const void *data )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();


	Texture_SubLoad( 1, 1, x, y, format, data );


	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

status_t BOGLDevice::Texture_GetTexel( uint32 x, uint32 y, color_space format, void *data )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();


	Texture_Get( 1, 1, x, y, format, data );


	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

status_t BOGLDevice::Texture_Enable( bool enable )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

	if( enable )
		glEnable( GL_TEXTURE_2D );
	else
		glDisable( GL_TEXTURE_2D );

	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

uint32 BOGLDevice::GetBytePP( uint32 type, uint32 format )
{
	int byte_per_element = 0;
	
	switch (type)
	{
	case GL_BITMAP:
		if (format != GL_COLOR_INDEX)
			return 0;

	case GL_BYTE:				byte_per_element=1; break;
	case GL_UNSIGNED_BYTE:		byte_per_element=1; break;
	case GL_SHORT:				byte_per_element=2; break;
	case GL_UNSIGNED_SHORT:		byte_per_element=2; break;
	case GL_INT:				byte_per_element=4; break;
	case GL_UNSIGNED_INT:		byte_per_element=4; break;
	case GL_FLOAT:				byte_per_element=4; break;

	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
		if ( format != GL_RGB )
			return 0;
		else
			return 1;

	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
		if ( format != GL_RGB )
			return 0;
		else
			return 2;
	
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		if ( !((format == GL_RGBA) || (format == GL_BGRA) ))
			return 0;
		else
			return 2;

	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
		if ( !((format == GL_RGBA) || (format == GL_BGRA) ))
			return 0;
		else
			return 4;

		break;
	default:
		return 0;
	}

	switch (format)
	{
	case GL_COLOR_INDEX:
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
	case GL_ALPHA:
	case GL_LUMINANCE:
		return byte_per_element;
		
	case GL_LUMINANCE_ALPHA:
		return byte_per_element *2;

	case GL_RGB:
	case GL_BGR:
		return byte_per_element *3;

	case GL_RGBA:
	case GL_BGRA:
	case GL_ABGR_EXT:
		return byte_per_element *4;

	default:
		return 0;
	}
	
	return 0;
}

uint32 BOGLDevice::Texture_GetGetSize( uint32 type, uint32 format )
{
	uint32 w, h, size;
	
	Texture_GetSize( &w, &h );
	
	size = w * GetBytePP( type, format );
	size = (size+3) & (~3);
	size *= h;
	
	return size;
}


///////////////////////////////////////////////////

status_t BOGLDevice::Plane_Destroy( BRasterPlane */*plane */ )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();

//	delete plane;

	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

status_t BOGLDevice::Plane_Select( BRasterPlane */*plane */ )
{
	status_t ret = B_OK;
	bool current = asWindow->oglIsLocked();
	if( !current )
		asWindow->oglLock();



	if( !current )
		asWindow->oglUnlock();
		
	return ret;
}

lock_status_t BOGLDevice::Context_Lock()
{
	lock_status_t s = asWindow->Lock();
	if (s == B_OK) {
		asWindow->oglLock();
	}
	return s;
}

void BOGLDevice::Context_Unlock()
{
	asWindow->oglUnlock();
	asWindow->Unlock();
}

void BOGLDevice::ColorspaceToGL( color_space cs, uint32 *type, uint32 *format, uint32 *internal_format )
{
	switch( cs )
	{
		case B_RGB32:
			*type = GL_UNSIGNED_INT_8_8_8_8_REV;
			*format = GL_BGRA;
			*internal_format = GL_RGB;
			break;

		case B_RGBA32:
			*type = GL_UNSIGNED_INT_8_8_8_8_REV;
			*format = GL_BGRA;
			*internal_format = GL_RGBA;
			break;

		case B_RGB32_BIG:
			*type = GL_UNSIGNED_INT_8_8_8_8;
			*format = GL_BGRA;
			*internal_format = GL_RGB;
			break;

		case B_RGBA32_BIG:
			*type = GL_UNSIGNED_INT_8_8_8_8;
			*format = GL_BGRA;
			*internal_format = GL_RGBA;
			break;
			
		case B_RGB24:
			*type = GL_UNSIGNED_BYTE;
			*format = GL_BGR;
			break;

		case B_RGB24_BIG:
			*type = GL_UNSIGNED_BYTE;
			*format = GL_RGB;
			*internal_format = GL_RGBA;
			break;
			
		case B_RGB15:
			*type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			*format = GL_RGBA;
			*internal_format = GL_RGB;
			break;

		case B_RGBA15:
			*type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			*format = GL_RGBA;
			*internal_format = GL_RGBA;
			break;
		
		case B_RGB16:
			*type = GL_UNSIGNED_SHORT_5_6_5_REV;
			*format = GL_RGB;
			*internal_format = GL_RGB;
			break;

		case B_RGB15_BIG:
			*type = GL_UNSIGNED_SHORT_5_5_5_1;
			*format = GL_RGBA;
			*internal_format = GL_RGB;
			break;

		case B_RGBA15_BIG:
			*type = GL_UNSIGNED_SHORT_5_5_5_1;
			*format = GL_RGBA;
			*internal_format = GL_RGBA;
			break;
		
		case B_RGB16_BIG:
			*type = GL_UNSIGNED_SHORT_5_6_5_REV;
			*format = GL_BGR;
			*internal_format = GL_RGB;
			break;
			
//		case B_GRAY8:
//			*type = GL_UNSIGNED_BYTE;
//			*format = GL_LUMINANCE;
//			*internal_format = GL_LUMINANCE;
//			break;

		case B_GRAY8:
			*type = GL_UNSIGNED_BYTE;
			*format = GL_ALPHA;
			*internal_format = GL_ALPHA8;
			break;

		default:
		case B_CMAP8:
		case B_GRAY1:
			*type = 0;
			*format = 0;
			*internal_format = 0;
			break;
	}
			
#if 0
	/* linear color space (little endian is the default) */
	B_RGB32 = 			0x0008,	/* B[7:0]  G[7:0]  R[7:0]  -[7:0]					*/
	B_RGBA32 = 			0x2008,	/* B[7:0]  G[7:0]  R[7:0]  A[7:0]					*/
	B_RGB24 = 			0x0003,	/* B[7:0]  G[7:0]  R[7:0]							*/
	B_RGB16 = 			0x0005,	/* G[2:0],B[4:0]  R[4:0],G[5:3]						*/
	B_RGB15 = 			0x0010,	/* G[2:0],B[4:0]  	   -[0],R[4:0],G[4:3]			*/
	B_RGBA15 = 			0x2010,	/* G[2:0],B[4:0]  	   A[0],R[4:0],G[4:3]			*/
	B_CMAP8 = 			0x0004,	/* D[7:0]  											*/
	B_GRAY8 = 			0x0002,	/* Y[7:0]											*/
	B_GRAY1 = 			0x0001,	/* Y0[0],Y1[0],Y2[0],Y3[0],Y4[0],Y5[0],Y6[0],Y7[0]	*/

	/* big endian version, when the encoding is not endianess independant */
	B_RGB32_BIG =		0x1008,	/* -[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
	B_RGBA32_BIG = 		0x3008,	/* A[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
	B_RGB24_BIG = 		0x1003,	/* R[7:0]  G[7:0]  B[7:0]							*/
	B_RGB16_BIG = 		0x1005,	/* R[4:0],G[5:3]  G[2:0],B[4:0]						*/
	B_RGB15_BIG = 		0x1010,	/* -[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/
	B_RGBA15_BIG = 		0x3010,	/* A[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/
	
	
	}

	B_NO_COLOR_SPACE =	0x0000,	/* byte in memory order, high bit first				*/
	

	/* little-endian declarations, for completness */
	B_RGB32_LITTLE = 	B_RGB32,
	B_RGBA32_LITTLE =	B_RGBA32,
	B_RGB24_LITTLE =	B_RGB24,
	B_RGB16_LITTLE =	B_RGB16,
	B_RGB15_LITTLE =	B_RGB15,
	B_RGBA15_LITTLE =	B_RGBA15,

	/* non linear color space -- note that these are here for exchange purposes;	*/
	/* a BBitmap or BView may not necessarily support all these color spaces.	*/

	/* Loss/Saturation points are Y 16-235 (absoulte); Cb/Cr 16-240 (center 128) */

	B_YCbCr422 = 		0x4000,	/* Y0[7:0]  Cb0[7:0]  Y1[7:0]  Cr0[7:0]  Y2[7:0]...	*/
								/* Cb2[7:0]  Y3[7:0]  Cr2[7:0]						*/
	B_YCbCr411 = 		0x4001,	/* Cb0[7:0]  Y0[7:0]  Cr0[7:0]  Y1[7:0]  Cb4[7:0]...*/
								/* Y2[7:0]  Cr4[7:0]  Y3[7:0]  Y4[7:0]  Y5[7:0]...	*/
								/* Y6[7:0]  Y7[7:0]	 								*/
	B_YCbCr444 = 		0x4003,	/* Y0[7:0]  Cb0[7:0]  Cr0[7:0]		*/
	B_YCbCr420 = 		0x4004,	/* Non-interlaced only, Cb0  Y0  Y1  Cb2 Y2  Y3  on even scan lines ... */
								/* Cr0  Y0  Y1  Cr2 Y2  Y3  on odd scan lines */

	/* Extrema points are Y 0 - 207 (absolute) U -91 - 91 (offset 128) V -127 - 127 (offset 128) */
	/* note that YUV byte order is different from YCbCr */
	/* USE YCbCr, not YUV, when that's what you mean! */
	B_YUV422 =			0x4020, /* U0[7:0]  Y0[7:0]   V0[7:0]  Y1[7:0] ... */
								/* U2[7:0]  Y2[7:0]   V2[7:0]  Y3[7:0]  */
	B_YUV411 =			0x4021, /* U0[7:0]  Y0[7:0]  Y1[7:0]  V0[7:0]  Y2[7:0]  Y3[7:0]  */
								/* U4[7:0]  Y4[7:0]  Y5[7:0]  V4[7:0]  Y6[7:0]  Y7[7:0]  */
	B_YUV444 =			0x4023,	/* U0[7:0]  Y0[7:0]  V0[7:0]  U1[7:0]  Y1[7:0]  V1[7:0] */
	B_YUV420 = 			0x4024,	/* Non-interlaced only, U0  Y0  Y1  U2 Y2  Y3  on even scan lines ... */
								/* V0  Y0  Y1  V2 Y2  Y3  on odd scan lines */
	B_YUV9 = 			0x402C,	/* planar?	410?								*/
	B_YUV12 = 			0x402D,	/* planar?	420?								*/

	B_UVL24 =			0x4030,	/* U0[7:0] V0[7:0] L0[7:0] ... */
	B_UVL32 =			0x4031,	/* U0[7:0] V0[7:0] L0[7:0] X0[7:0]... */
	B_UVLA32 =			0x6031,	/* U0[7:0] V0[7:0] L0[7:0] A0[7:0]... */

	B_LAB24 =			0x4032,	/* L0[7:0] a0[7:0] b0[7:0] ...  (a is not alpha!) */
	B_LAB32 =			0x4033,	/* L0[7:0] a0[7:0] b0[7:0] X0[7:0] ... (b is not alpha!) */
	B_LABA32 =			0x6033,	/* L0[7:0] a0[7:0] b0[7:0] A0[7:0] ... (A is alpha) */

	/* red is at hue = 0 */

	B_HSI24 =			0x4040,	/* H[7:0]  S[7:0]  I[7:0]							*/
	B_HSI32 =			0x4041,	/* H[7:0]  S[7:0]  I[7:0]  X[7:0]					*/
	B_HSIA32 =			0x6041,	/* H[7:0]  S[7:0]  I[7:0]  A[7:0]					*/

	B_HSV24 =			0x4042,	/* H[7:0]  S[7:0]  V[7:0]							*/
	B_HSV32 =			0x4043,	/* H[7:0]  S[7:0]  V[7:0]  X[7:0]					*/
	B_HSVA32 =			0x6043,	/* H[7:0]  S[7:0]  V[7:0]  A[7:0]					*/

	B_HLS24 =			0x4044,	/* H[7:0]  L[7:0]  S[7:0]							*/
	B_HLS32 =			0x4045,	/* H[7:0]  L[7:0]  S[7:0]  X[7:0]					*/
	B_HLSA32 =			0x6045,	/* H[7:0]  L[7:0]  S[7:0]  A[7:0]					*/

	B_CMY24 =			0xC001,	/* C[7:0]  M[7:0]  Y[7:0]  			No gray removal done		*/
	B_CMY32 =			0xC002,	/* C[7:0]  M[7:0]  Y[7:0]  X[7:0]	No gray removal done		*/
	B_CMYA32 =			0xE002,	/* C[7:0]  M[7:0]  Y[7:0]  A[7:0]	No gray removal done		*/
	B_CMYK32 =			0xC003,	/* C[7:0]  M[7:0]  Y[7:0]  K[7:0]					*/
#endif

}




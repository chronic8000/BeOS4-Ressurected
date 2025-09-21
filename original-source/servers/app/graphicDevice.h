
#ifndef GRAPHIC_DEVICE_H
#define GRAPHIC_DEVICE_H

#include <image.h>
#include <Accelerant.h>
#include <video_overlay.h>
#include "render.h"
#include "lock.h"
#include "heap.h"
#include "display_rotation.h"

struct vague_display_mode {
	int32			x,y;
	uint32			pixelFormat;
	float			rate;
};

class SApp;

class Overlay {

	public:
						Overlay(int32 device, SApp *team, const overlay_buffer *buffer, overlay_token reserved);
						~Overlay();
	
		int32			Width();
		int32			Height();
		int32			BytesPerRow();
		color_space		ColorSpace();
		Area *			SharedData();
		rgb_color		ColorKey();

		void *			Bits();
		status_t		LockBits();
		status_t		UnlockBits();
		
		status_t		Shadow();
		status_t		Unshadow();

		status_t		Toss();
		status_t		AssertResident();

		status_t		StealChannelFrom(Overlay *victim, uint32 options);
		status_t		Configure(rect src, rect dst, uint32 options);
		status_t		Unconfigure();
		
		status_t		GetConstraints(overlay_constraints *c);
#if ROTATE_DISPLAY
		inline	void	SetRotater(DisplayRotater *r) { m_rotater = r;}
#endif

	private:

		void			ResetSharedData();

		friend class BGraphicDevice;
	
		enum {
			CHANNEL_RESERVED	=	0x00000001,
			CHANNEL_IN_USE 		=	0x00000002,
			BITS_LOCKED			=	0x00000004
		};

		color_space				m_space;
		int32					m_width,m_height,m_bytesPerRow;
		const overlay_buffer *	m_buffer;
		Area					m_ROData;
		Area *					m_shadow;
		sem_id					m_lock;
		overlay_view			m_view;
		overlay_window			m_window;
		uint32					m_state;
		status_t				m_initErr;
		int32					m_device;
		SApp *					m_client;
		overlay_token			m_channelToken;
		rgb_color				m_colorKey;
		Overlay *				m_next;
#if ROTATE_DISPLAY
		DisplayRotater			*m_rotater;
#endif			
};

#define MAX_SCREENS 32

status_t mode2parms(uint32 mode, uint32 *cs, uint16 *width, uint16 *height);

class TCursor;

enum {
	OVERLAY_RESERVE_CHANNEL = 0x01
};

class BGraphicDevice
{

	private:

									BGraphicDevice(int32 fd, image_id imageID, GetAccelerantHook hook, const char *path, const char *devicepath);
									BGraphicDevice(const BGraphicDevice *device);
			void					InitData();

	public:

									~BGraphicDevice();
	
	static	void					Init();
	static	BGraphicDevice *		PickDevice();
	static	BGraphicDevice *		MakeDevice(const char *path);
	static	void					FakeMoveCursor(uint16, uint16);


			void					ResetFromGame(BGraphicDevice *gd);
			void					DisconnectForGame(BGraphicDevice *gd);

			uint32					ModeCount(void);
			const display_mode		*ModeList(void);
			status_t				SetVagueMode(vague_display_mode *vmode);
			status_t				GetVagueMode(vague_display_mode *vmode);
			status_t				SetMode(display_mode *mode);
			status_t				GetMode(display_mode *mode);
			status_t				ProposeDisplayMode(display_mode *target, display_mode *low, display_mode *high);
			status_t				MoveDisplay(uint16 x, uint16 y);
			status_t				SetDPMS(uint32 state);
			uint32					GetDPMS(void);
			uint32					DPMSStates(void);
			status_t				GetRefreshRateRange(display_mode *mode, float *low, float *high);
			status_t				GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
			status_t				GetTimingConstraints(display_timing_constraints *dtc);
			status_t				SetBaseMode();
			status_t				GetBaseVagueMode(vague_display_mode *vmode);
			status_t				GetBaseMode(display_mode *mode);
			void					SetIndexedColors(uint count, uint8 first, uint8 *data, uint32 flags);
			status_t				Vagueify(display_mode *mode, vague_display_mode *vmode);
			status_t				Concretize(vague_display_mode *vmode, display_mode *mode);

			status_t				CreateOverlay(SApp *team,
										color_space space, int32 width, int32 height,
										uint32 flags, Overlay **overlay);

			bool					HasHardwareCursor();
			void					ShowCursor(		bool visible);
			void					MoveCursor(		uint16 x, uint16 y);
			void					SetCursorShape(	uint16 width, uint16 height,
													uint16 hotX, uint16 hotY,
													uint8 *andMask, uint8 *xorMask);

			static	BGraphicDevice *Device(int32 index);
			static	BGraphicDevice *GameDevice(int32 index);
			static	BGraphicDevice *ApparentDevice(int32 index, team_id id);
			static void				SetGameDevice(int32 index, BGraphicDevice *gd);
			RenderContext *			MouseContext();
			RenderContext *			Context();
#ifdef APP_SERVER_DEMO_CD
			RenderContext *			EmergencyContext();
#endif
			RenderPort *			Port();
			RenderPort *			MousePort();
#ifdef APP_SERVER_DEMO_CD
			RenderPort *			EmergencyPort();
#endif
			RenderCanvas *			Canvas();

			TCursor *				Cursor();
			
			int32					Height();
			int32					Width();

			int32					PortToken();
			int32					ScreenToken();

			uint32					ModeMask();
			int32					EngineCount();

			sem_id					RetraceSemaphore();
			bool					IsLameCard();
			CountedBenaphore &		LameAssLock();
			
			const char *			ImagePath();
			const char *			DevicePath();
			ssize_t					CloneInfoSize();
			const void *			CloneInfo();
			status_t				PerformHack(void *in, ssize_t *size, void **output);
#if ROTATE_DISPLAY
			DisplayRotater			m_rotater;
#endif			

#if ROTATE_DISPLAY
			static void				RotateCursorMask(	uint16	width,
														uint16	height,
														uint8	*mask,
														uint8	*rotatedMask);
#endif

	private:

	friend	class 					Overlay;
	friend	void 					ResizeDebugScreen(int32 delta);
	friend	int32 					lock_direct_screen(int32, int32);
	friend	bool					set_screen_saver_mode(bool enable);

	static	BGraphicDevice *		m_screens[MAX_SCREENS];
	static	BGraphicDevice *		m_game_screens[MAX_SCREENS];

			status_t				ColorTranslateOverlay(rgb_color key, overlay_window *w);
			status_t				DeallocOverlay(Overlay *overlay);
			status_t				ReallocOverlay(Overlay *overlay);
			status_t				RemoveOverlay(Overlay *overlay);
			status_t				AllocOverlayChannel(overlay_token *token,
										rgb_color *key, overlay_window *w);

			void					AssertColorMap();
			uint32					MakeModeMask();

			bool					m_master;
			int						m_fd;
			image_id				m_image;
			GetAccelerantHook		m_gah;
			char *					m_imagePath;
			char *					m_devicePath;
			ssize_t					m_cloneInfoSize;
			void *					m_cloneInfo;
			Overlay *				m_overlays;

			uint32					m_modeMask;
			uint32					m_modeCount;
			display_mode *			m_modeList;
			display_mode			m_currentMode;

			accelerant_mode_count			m_amc;
			get_mode_list					m_gml;
			set_display_mode				m_sdm;
			get_display_mode				m_gdm;
			move_display_area				m_mda;
			propose_display_mode			m_pdm;
			get_frame_buffer_config			m_fbc;
			get_pixel_clock_limits			m_pcl;
			accelerant_retrace_semaphore	m_ars;
			overlay_count					m_overlayCount;
			overlay_supported_spaces		m_overlaySupportedSpaces;
			allocate_overlay_buffer			m_allocOverlayBuffer;
			release_overlay_buffer			m_releaseOverlayBuffer;
			get_overlay_constraints			m_overlayConstraints;
			allocate_overlay				m_allocOverlay;
			release_overlay					m_releaseOverlay;
			configure_overlay				m_configOverlay;

			set_cursor_shape		m_cursorShape;
			move_cursor				m_cursorMove;
			show_cursor				m_cursorShow;

			RenderContext			m_context;
			RenderContext			m_mouseContext;
#ifdef APP_SERVER_DEMO_CD
			RenderContext			m_emergencyContext;
#endif
			RenderPort				m_port;
			RenderPort				m_mousePort;
#ifdef APP_SERVER_DEMO_CD
			RenderPort				m_emergencyPort;
#endif
			RenderCanvas			m_canvas;
			AccelPackage			m_accelPackage;
			
			int32					m_formatInc;
			int32					m_regionInc;
			TCursor *				m_cursor;
			int32					m_engineCount;
			
			CountedBenaphore		m_lameAssBenaphore;
};

inline CountedBenaphore & BGraphicDevice::LameAssLock()
{
	return m_lameAssBenaphore;
};

inline bool BGraphicDevice::IsLameCard()
{
	return (m_currentMode.flags & B_IO_FB_NA);
};

inline int32 BGraphicDevice::EngineCount()
{
	return m_engineCount;
};

inline TCursor * BGraphicDevice::Cursor()
{
	return m_cursor;
};

inline uint32 BGraphicDevice::ModeMask()
{
	return m_modeMask;
};

inline int32 BGraphicDevice::PortToken()
{
	return m_regionInc;
};

inline int32 BGraphicDevice::ScreenToken()
{
	return m_formatInc;
};

#if ROTATE_DISPLAY
inline int32 BGraphicDevice::Height()	{ return m_canvas.pixels.w; };
inline int32 BGraphicDevice::Width()	{ return m_canvas.pixels.h; };
#else
inline int32 BGraphicDevice::Height()	{ return m_canvas.pixels.h; };
inline int32 BGraphicDevice::Width()	{ return m_canvas.pixels.w; };
#endif

inline RenderContext * BGraphicDevice::MouseContext()
{
	return &m_mouseContext;
};

#ifdef APP_SERVER_DEMO_CD
inline RenderContext * BGraphicDevice::EmergencyContext()
{
	return &m_emergencyContext;
};
#endif

inline RenderContext * BGraphicDevice::Context()
{
	return &m_context;
};

inline RenderPort * BGraphicDevice::Port()
{
	return &m_port;
};

inline RenderPort * BGraphicDevice::MousePort()
{
	return &m_mousePort;
};

#ifdef APP_SERVER_DEMO_CD
inline RenderPort * BGraphicDevice::EmergencyPort()
{
	return &m_emergencyPort;
};
#endif

inline RenderCanvas * BGraphicDevice::Canvas()
{
	return &m_canvas;
};

inline BGraphicDevice * BGraphicDevice::Device(int32 index)
{
	return m_screens[index];
};

inline BGraphicDevice * BGraphicDevice::GameDevice(int32 index)
{
	return m_game_screens[index];
};

inline void BGraphicDevice::SetGameDevice(int32 index, BGraphicDevice *gd)
{
	m_game_screens[index] = gd;
};

inline bool	BGraphicDevice::HasHardwareCursor()
{
	return m_cursorShape && m_cursorMove && m_cursorShow;
};

inline void BGraphicDevice::ShowCursor(bool visible)
{
	m_cursorShow(visible);
};

inline void BGraphicDevice::MoveCursor(uint16 x, uint16 y)
{
#if ROTATE_DISPLAY
	const uint16 rx = m_mousePort.rotater->RotateV(y);
	const uint16 ry = m_mousePort.rotater->RotateH(x);
	m_cursorMove(rx, ry);
#else
	m_cursorMove(x, y);
#endif
};

inline void BGraphicDevice::SetCursorShape( uint16 width, uint16 height,
											uint16 hotX, uint16 hotY,
											uint8 *andMask, uint8 *xorMask)
{
#if ROTATE_DISPLAY
	uint8 andRotatedMask[128*128/8];
	uint8 xorRotatedMask[128*128/8];
	RotateCursorMask(width, height, andMask, andRotatedMask);
	RotateCursorMask(width, height, xorMask, xorRotatedMask); 
	m_cursorShape(height, width, height-hotY-1, hotX, andRotatedMask, xorRotatedMask);
#else
	m_cursorShape(width,height,hotX,hotY,andMask,xorMask);
#endif
};

inline RenderPort * ScreenPort(int32 index=0)
{
	return BGraphicDevice::Device(index)->Port();
};

inline RenderContext * ScreenContext(int32 index=0)
{
	return BGraphicDevice::Device(index)->Context();
};

inline RenderCanvas * ScreenCanvas(int32 index=0)
{
	return BGraphicDevice::Device(index)->Canvas();
};

inline int32 ScreenX(int32 index=0)
{
	return BGraphicDevice::Device(index)->Width();
};

inline int32 ScreenY(int32 index=0)
{
	return BGraphicDevice::Device(index)->Height();
};

#if ROTATE_DISPLAY
inline long cursor_screen_width() { return ScreenY(0)-1; };
inline long cursor_screen_height() { return ScreenX(0)-1; };
#else
inline long cursor_screen_width() { return ScreenX(0)-1; };
inline long cursor_screen_height() { return ScreenY(0)-1; };
#endif

extern RenderCanvas *	LockScreenCanvas(BGraphicDevice *device, region *willChange, uint32 *token, uint32 *flags);
extern void				UnlockScreenCanvas(BGraphicDevice *device, region *doneChange, sync_token *token);
extern RenderCanvas *	LockScreenCanvasLameAssCard(BGraphicDevice *device, region *willChange, uint32 *token, uint32 *flags);
extern void				UnlockScreenCanvasLameAssCard(BGraphicDevice *device, region *doneChange, sync_token *token);

inline sem_id BGraphicDevice::RetraceSemaphore()
{
	return m_ars ? m_ars() : B_ERROR;
}

inline uint32 BGraphicDevice::ModeCount(void) { return m_modeCount; }

inline const display_mode * BGraphicDevice::ModeList(void) { return m_modeList; }

inline const char *BGraphicDevice::ImagePath() { return m_imagePath; }

inline const char *BGraphicDevice::DevicePath() { return m_devicePath; }


#endif

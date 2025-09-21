
//******************************************************************************
//
//	File:			font.h
//
//	Description:	High-level font representation, supporting font
//					replacement.
//	
//	Written by:		Dianne Hackborn
//
//	Copyright 2000, Be Incorporated
//
//******************************************************************************/

#ifndef FONT_H
#define FONT_H

#ifndef ATOM_H
#include "atom.h"
#endif

#include <Locker.h>

#include "font_defs.h"
#include "shared_fonts.h"

struct font_body;

class TSession;

class SFont : public BFlattenable, public BAbstractFont
{
public:
	/* Global font state */
	static void			Start();
	static void			Stop();
	static void			LoadPrefs();
	static void			SavePrefs();
	
	static void			GetStandardFont(font_which which, SFont* into);
	static void			SetStandardFont(font_which which, const SFont& from);
	static bool			FontsChanged();
	
	static void			GetSystemFont(font_which which, SFont* into);
	
	static void			SetPreferredAntialiasing(int32 mode);
	static int32		PreferredAntialiasing();
	
	static void			SetAliasingMinLimit(int32 min_size);
	static void			SetAliasingMaxLimit(int32 max_size);
	static int32		AliasingMinLimit();
	static int32		AliasingMaxLimit();
	
	static void			SetHintingMaxLimit(int32 max_size);
	static int32		HintingMaxLimit();
	
	/* Object operations */
						SFont();
						SFont(const void* packet, size_t size);
						SFont(const SFont& o);
	virtual				~SFont();

	SFont&				operator=(const SFont& o);
	bool				operator==(const SFont& o) const;
	bool				operator!=(const SFont& o) const;
	
	/* Packet assignment and retrieval */
	status_t			SetTo(const void* packet, size_t size,
							  uint32 mask = B_FONT_ALL);
	
	status_t			GetBasicPacket(fc_context_packet* packet) const;
	
	ssize_t				PacketLength() const;
	status_t			GetPacket(void* buffer) const;
	
	status_t			ReadPacket(TSession* a_session, bool include_mask=false);
	void				WritePacket(TSession* a_session) const;
	
	/* Abstract font interface */
virtual	bool				IsValid() const;

virtual	BAbstractFont&		SetTo(const BAbstractFont &source, uint32 mask = B_FONT_ALL);
virtual	BAbstractFont&		SetTo(font_which which, uint32 mask = B_FONT_ALL);

virtual	status_t			ApplyCSS(const char* css_style, uint32 mask = B_FONT_ALL);
		
virtual	status_t			SetFamilyAndStyle(const font_family family, 
											  const font_style style);
virtual	void				SetFamilyAndStyle(uint32 code, uint16 face);
virtual	status_t			SetFamilyAndFace(const font_family family, uint16 face);
		
virtual	void				SetSize(float size);
virtual	void				SetShear(float shear);
virtual	void				SetRotation(float rotation);
virtual	void				SetSpacing(uint8 spacing);
virtual	void				SetEncoding(uint8 encoding);
virtual	void				SetFace(uint16 face);
virtual	void				SetFlags(uint32 flags);

virtual	void				GetFamilyAndStyle(font_family *family,
											  font_style *style) const;
virtual	uint32				FamilyAndStyle() const;
virtual	float				Size() const;
virtual	float				Shear() const;
virtual	float				Rotation() const;
virtual	uint8				Spacing() const;
virtual	uint8				Encoding() const;
virtual	uint16				Face() const;
virtual	uint32				Flags() const;

virtual	FontOverlayMap*		EditOverlay();
virtual	const FontOverlayMap* Overlay() const;
virtual	void				ClearOverlay();

	/* Other state access and modification */
	void				SetBitsPerPixel(int depth);
	int					BitsPerPixel() const;
	void				SetFontIDs(	int family_id, int style_id,
									bool allow_disable = false, uint16 faces = 0);
	void				SetFamilyID(int id);
	int					FamilyID() const;
	const char*			FamilyName() const;
	void				SetStyleID(int id);
	int					StyleID() const;
	const char*			StyleName() const;
	void				SetShearRaw(float shear);
	float				ShearRaw() const;
	void				SetRotationRaw(float rotation);
	float				RotationRaw() const;
	
	/* Add additional font flags to this font.  These only last until
	   the next call to SetFlags() or anything else that changes the
	   base flags. */
	void				ForceFlags(uint32 flags);
	
	/* Update bits per pixel and hinting from global preferences */
	bool				Refresh();
	
	void				GetHeight(font_height* height) const;
	float				Ascent() const;
	float				Descent() const;
	float				Leading() const;
	
	void				SetOrigin(float x, float y);
	float				OriginX() const;
	float				OriginY() const;
	
	/* Operations */
	float				StringWidth(const uchar* str);
	void				TruncateString(float width, int32 mode,
										const uchar* str, uchar* result);
	const uchar*		GetEscapement(fc_escape_table* table, const uchar* string,
										float delta_nsp = 0.0, float delta_sp = 0.0,
										bool use_real_size = false);
	int32				HasGlyphs(const uchar* str, bool* has);
	const uchar*		GetGlyphs(fc_glyph_table* g_table, const uchar* string);
	const uchar*		GetEdge(fc_edge_table* table, const uchar* string);
	const uchar*		GetBBox(fc_bbox_table* b_table, const uchar* string,
								float delta_nsp, float delta_sp, int mode, int string_mode,
								double* offset_h, double* offset_v);
	void				GetStringBBox(const uchar* str, float delta_nsp, float delta_sp,
									  int mode, fc_bbox* bbox);
	
	/* Font set control (only apply to primary font) */
	void				FlushFontSet();
	status_t			SaveFullSet(ushort* dir_id, char* filename, char* pathname);
	
	/* Rendering */
	const uchar*		LockString(fc_string* draw, const uchar* string,
									float non_space_delta, float space_delta,
									double* xx, double* yy, uint32 lock_mode = 0);
	void				UnlockString();
	
	/* BFont compatibility */
	inline float		StringWidth(const char* str)
						{ return StringWidth(reinterpret_cast<const uchar*>(str)); }
	
	/* BFlattenable interface */
virtual	bool			IsFixedSize() const;
virtual	type_code		TypeCode() const;
virtual	ssize_t			FlattenedSize() const;
virtual	status_t		Flatten(void *buffer, ssize_t size) const;
virtual	bool			AllowsTypeCode(type_code code) const;
virtual	status_t		Unflatten(type_code c, const void *buf, ssize_t size);

	// Shut up the compiler.
virtual	status_t		Unflatten(type_code c, const void *buf, ssize_t size,
									bool force_valid);

private:
						SFont(int family_id, int style_id, int size, bool allow_disable = false);
	static void			LoadOldPrefs(FILE* fp);
						
	const font_body*	fBody;
	void*				fLocked;
};

#endif

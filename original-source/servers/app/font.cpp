#include "font.h"

#include "font_cache.h"
#include "font_data.h"
#include "lock.h"
#include "proto.h"
#include "as_support.h"
#include "session.h"

#include <OS.h>
#include <Autolock.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <Path.h>
#include <SupportDefs.h>

#include <fcntl.h>
#include <math.h>
#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define SHOW_REFS 0

#if SHOW_REFS
static int32 bodyCount = 0;
#endif

#define PRINT_CMD(x)
//#define PRINT_CMD(x) printf x

// This is the access control for all global SFont state.  Note that
// you MUST NOT be holding a font cache lock when you acquire this;
// our lock ordering is this lock before any lower-level font cache locks.
static RWLock gFontLock("SFont Globals");

// Current standard fonts that are defined.
static SFont* gStandardFonts[B__NUM_FONT] = {
	NULL, NULL, NULL, NULL, NULL
};

// This variable is incremented every time any of the global fonts
// change, so that others can know they need to refresh their state.
static int32 gFontsChanged = 0;

// Hooks for doing font family/style-id conversions, the app_server way.
static status_t as_font_family_name_to_id(const font_family name, uint16* out_id)
{
	*out_id = fc_get_family_id(name);
	return *out_id != 0xffff ? B_OK: B_ERROR;
}

static status_t as_font_family_id_to_name(uint16 id, font_family* out_name)
{
	memset(*out_name, 0, sizeof(*out_name));
	const char* name = fc_get_family_name(id);
	if (name == NULL || *name == 0)
		return B_ERROR;
	strncat(*out_name, name, sizeof(font_family));
	return B_OK;
}

// The currently defined global overlay.  The overlay ID is incremented
// every time it changes, so others know when to refresh their state.
static int32 gOverlayID = 0;
static FontOverlayMap gGlobalOverlay;

// This is incremented every time any other settings change.
static int32 gSettingsID = 0;

// This class is used to cache fc_context objects for an overlay.
// The first time a particular overlay range is needed, it generates
// the fc_context for it; later requests for that range will return
// the same context.
class overlay_cache
{
public:
		overlay_cache()
			: items(NULL), count(0), id(0)
		{
		}
		~overlay_cache()
		{
			clear();
		}
		
		void clear();
		fc_context* retrieve(int32 cur_count, int32 cur_id,
							 int32 index, const font_overlay_entry* entry,
							 fc_context* base);
		
private:
		fc_context**	items;
		int32			count;
		int32			id;
};

// This is a helper class for splitting a string across overlay
// boundaries.
struct overlay_range {
	uint32			start;
	uint32			end;
	bool			missing;
	
	bool			is_valid() const		{ return start <= end; }
	
	fc_context*		look_up(uint32 symbol, font_body* font);
	bool			verify(uint32 symbol) const;
	
	overlay_range()
		:	start(0xffffffff), end(0), missing(false), num_files(0), locked(false)
	{
	}
	~overlay_range()
	{
		if (locked) fc_unlock_file();
	}

private:
	enum {
		MAX_ACTIVE_OVERLAY = 3
	};
	int32			num_files;
	fc_file_entry*	file_entries[MAX_ACTIVE_OVERLAY];
	uint16			file_ids[MAX_ACTIVE_OVERLAY];
	bool			locked;
	
	fc_context*		add_overlay(uint32 symbol, font_body* font,
								FontOverlayMap& map, overlay_cache& cache);
	void			refresh_files();
};

// --------------------- SFont Object Body ---------------------

struct font_body
{
	fc_context		context;
	
	FontOverlayMap	local_overlay;
	overlay_cache	local_cache;
	
	overlay_cache	global_cache;
	
	int32			seq;
	
	int32			acquire() const
	{
		return atomic_add(&refs, 1);
	}
	
	int32			release() const
	{
		const int32 c = atomic_add(&refs, -1);
		if (c == 1) delete this;
		return c;
	}
	
	font_body*		edit() const;
	font_body*		use() const;
	
	bool			refresh_needed() const
	{
		return (seq != gSettingsID);
	}
	
	bool			refresh();
	
	font_body*		set_to(const fc_context_packet* p) const;
	
	void			clear_cache()
	{
		local_cache.clear();
		global_cache.clear();
	}
	
	font_body()
		: refs(0), seq(gSettingsID)
	{
		#if SHOW_REFS
		DebugPrintf(("Created font_body #%ld\n", atomic_add(&bodyCount, 1)));
		#endif
	}
	font_body(const font_body* o)
		: refs(0), context(&(o->context)), local_overlay(o->local_overlay),
		  seq(o->seq)
	{
		#if SHOW_REFS
		DebugPrintf(("Created font_body #%ld\n", atomic_add(&bodyCount, 1)));
		#endif
	}
	font_body(int family_id, int style_id, int size, bool allow_disable)
		: refs(0), context(family_id, style_id, size, allow_disable),
		  seq(gSettingsID)
	{
		#if SHOW_REFS
		DebugPrintf(("Created font_body #%ld\n", atomic_add(&bodyCount, 1)));
		#endif
	}
	
	font_body(const fc_context_packet* p)
		: refs(0), context(p), seq(gSettingsID)
	{
		#if SHOW_REFS
		DebugPrintf(("Created font_body #%ld\n", atomic_add(&bodyCount, 1)));
		#endif
	}
	~font_body()
	{
		#if SHOW_REFS
		DebugPrintf(("Deleted font_body #%ld\n", atomic_add(&bodyCount, -1)-1));
		#endif
		clear_cache();
	}

private:
	mutable int32	refs;
	font_body(const font_body& o);
	font_body& operator=(const font_body& o);
	bool operator==(const font_body& o);
};

// --------------------- Unicode Parsing and Substitution ---------------------

struct unicode_run {
	uint16			string[FC_MAX_STRING];
	int32			length;
	fc_context*		context;
};

static
const uchar *get_unicode_run(unicode_run* run,
							 const uchar* string,
							 font_body*   body)
{
	overlay_range	range;
	int				max, square;
	ushort*			symbol_set;

	uint16*			uni_string = run->string;
	
	square = body->context.size()*body->context.size();
	if (square <= (FC_SIZE_MAX_CUSTOM*FC_SIZE_MAX_CUSTOM))
		max = FC_MAX_STRING;
	else {
		max = 1+((FC_MAX_STRING*FC_SIZE_MAX_CUSTOM*FC_SIZE_MAX_CUSTOM)/square);
		if (max > FC_MAX_STRING)
			max = FC_MAX_STRING;
	}
	
	gFontLock.ReadLock();
	
	run->length = 0;
	run->context = NULL;
	if (body->context.encoding() == 0) {
		/* convert UTF8 to Unicode */
		while ((run->length < max) && (*string != 0)) {
			int32 step = 0;
			if ((string[0]&0x80) == 0) {
				*uni_string = *string;
				step = 1;
			} else if ((string[1] & 0xC0) != 0x80) {
		        *uni_string = 0xfffd;
				step = 1;
			} else if ((string[0]&0x20) == 0) {
				*uni_string = ((string[0]&31)<<6) | (string[1]&63);
				step = 2;
			} else if ((string[2] & 0xC0) != 0x80) {
		        *uni_string = 0xfffd;
				step = 2;
			} else if ((string[0]&0x10) == 0) {
				*uni_string = ((string[0]&15)<<12) | ((string[1]&63)<<6) | (string[2]&63);
				step = 3;
			} else if ((string[3] & 0xC0) != 0x80) {
		        *uni_string = 0xfffd;
				step = 3;
			} else {
				if (run->length >= (max-1))
					break;
				uint32 val = ((string[0]&7)<<18) | ((string[1]&63)<<12)
							| ((string[2]&63)<<6) | (string[3]&63);
				uni_string[0] = 0xd7c0+(val>>10);
				uni_string[1] = 0xdc00+(val&0x3ff);
				step = 4;
			}
			if (!range.is_valid()) {
				run->context = range.look_up(*uni_string, body);
			} else if (!range.verify(*uni_string)) {
				break;
			}
			string += step;
			if (!range.missing) {
				if (step < 4) {
					uni_string += 1;
					run->length += 1;
				} else {
					uni_string += 2;
					run->length += 2;
				}
			} else {
				*uni_string++ = 0xfffd;
				run->length += 1;
			}
		}
	}
	else {
		/* convert 8 bits symbol set to Unicode */
		symbol_set = b_symbol_set_table[body->context.encoding()];
		while ((run->length < max) && (*string != 0)) {
			*uni_string = symbol_set[*string];
			if (!range.is_valid()) {
				run->context = range.look_up(*uni_string, body);
			} else if (!range.verify(*uni_string)) {
				break;
			}
			if (range.missing)
				*uni_string = 0xfffd;
			string++;
			uni_string++;
			run->length++;
		}
	}
	
	gFontLock.ReadUnlock();
	
	return *string ? string : NULL;
}

// -------------------------------------------------------------------------

enum {
	kGlobalOverlay = 'Glov',
	kSettingsStatus = 'Stts'
};

static FILE* open_settings_file(const char* mode)
{
	status_t res;
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) < B_OK)
		return NULL;
	if (path.Append("fonts_status") < B_OK)
		return NULL;
	return fopen(path.Path(), mode);
}

void SFont::Start()
{
	gFontLock.WriteLock();
	/* attach app_server implementation of these libbe functions */
	font_family_name_to_id = as_font_family_name_to_id;
	font_family_id_to_name = as_font_family_id_to_name;
	/* init the cache status entries */
	fc_gray_system_status.set(FC_GRAY_SCALE,
							  -FC_HIGH_PRIORITY_BONUS,
							  -FC_HIGH_PRIORITY_BONUS,
							  20, -FC_LOW_PRIORITY_BONUS,
							  256*1024L, 18);
	fc_bw_system_status.set(FC_BLACK_AND_WHITE,
							-FC_LOW_PRIORITY_BONUS,
							-FC_LOW_PRIORITY_BONUS,
							256, -FC_HIGH_PRIORITY_BONUS,
							256*1024L, 0);
	fc_start();
	gFontLock.WriteUnlock();
	
	LoadPrefs();
}

void SFont::Stop()
{
	gFontLock.WriteLock();
	for (int32 i=0; i<B__NUM_FONT; i++) {
		delete gStandardFonts[i];
		gStandardFonts[i] = NULL;
	}
	fc_stop();
	gFontLock.WriteUnlock();
}

void SFont::LoadPrefs()
{
	int32 findex = 0;
	int32 sindex = 0;
	int32 type;
	int32 len;
	void* buffer = NULL;
	int32 bufferLen = 0;
	bool foundOverlay = false;
	
	gFontLock.WriteLock();
	
	FILE* fp = open_settings_file("rb");
	if (fp) {
		// Check file header.  It is stored in big-endian format
		// for backwards compatibility; all other data in the file
		// is little-endian.
		if (fread(&type, 1, 4, fp) != 4) goto finish;
		if (B_BENDIAN_TO_HOST_INT32(type) != FC_FONT_STATUS_ID) {
			if (B_BENDIAN_TO_HOST_INT32(type) == FC_FONT_STATUS_ID_1) {
				LoadOldPrefs(fp);
			}
			goto finish;
		}
		
		while (1) {
			
			// Read next chunk from file.
			if (fread(&type, 1, 4, fp) != 4) goto finish;
			if (fread(&len, 1, 4, fp) != 4) goto finish;
			type = B_LENDIAN_TO_HOST_INT32(type);
			len = B_LENDIAN_TO_HOST_INT32(len);
			//+DebugPrintf(("Found chunk: type=%4.4s, length=%ld\n", (char*)&type, len));
			if (len < 0)
				continue;
			
			if (bufferLen < len) {
				buffer = grRealloc(buffer, len, "SFont::LoadPrefs() buffer", MALLOC_MEDIUM);
				bufferLen = len;
			}
			if (buffer == NULL) goto finish;
			if (fread(buffer, 1, len, fp) != len) goto finish;
			
			switch (type) {
				// Load next font setting from file.
				case B_FONT_TYPE: {
					if (findex < B__NUM_FONT) {
						if (!gStandardFonts[findex])
							gStandardFonts[findex] = new(std::nothrow) SFont();
						if (gStandardFonts[findex]) {
							GetSystemFont((font_which)(findex+B_PLAIN_FONT),
											gStandardFonts[findex]);
							if (gStandardFonts[findex]->IsValid())
								gStandardFonts[findex]->Unflatten((type_code)type, buffer,
																   len, false);
						}
						xprintf("Loaded settings font #%ld: %s / %s\n", findex+1,
								gStandardFonts[findex]->FamilyName(),
								gStandardFonts[findex]->StyleName());
						findex++;
					}
				} break;
				// Load the global font overlay.
				case kGlobalOverlay: {
					foundOverlay = true;
					SFont font;
					font.Unflatten(B_FONT_TYPE, buffer, len, false);
					if (font.fBody) {
						gGlobalOverlay = font.fBody->local_overlay;
						gOverlayID++;
					}
				} break;
				// Load next cache status setting from file.
				case kSettingsStatus: {
					fc_cache_status* s = NULL;
					if (sindex == 0)		s = &fc_bw_system_status;
					else if (sindex == 1)	s = &fc_gray_system_status;
					if (s) {
						s->unflatten((char*)buffer, len);
						sindex++;
					}
				} break;
			}
		}
	}
	
finish:
	if (buffer)
		grFree(buffer);
	if (fp)
		fclose(fp);
	
	// Set any fonts not found to their defaults, and make sure all are
	// set to bitmap spacing mode.
	for (findex=0; findex < B__NUM_FONT; findex++) {
		if (!gStandardFonts[findex]) {
			//DebugPrintf(("Creating font #%ld\n", findex));
			gStandardFonts[findex] = new(std::nothrow) SFont();
		}
		if (gStandardFonts[findex] && !gStandardFonts[findex]->IsValid()) {
			//DebugPrintf(("Getting font #%ld\n", findex));
			GetSystemFont((font_which)(findex+B_PLAIN_FONT), gStandardFonts[findex]);
		}
		if (gStandardFonts[findex]) {
			SFont& s = *gStandardFonts[findex];
			s.SetSpacing(B_BITMAP_SPACING);
			if (s.Size() < FC_SYSTEM_FONT_MIN)
				s.SetSize(FC_SYSTEM_FONT_MIN);
			if (s.Size() > FC_SYSTEM_FONT_MAX)
				s.SetSize(FC_SYSTEM_FONT_MAX);
		}
	}
	
	if (!foundOverlay) {
		SFont font;
		GetSystemFont((font_which)99, &font);
		if (font.fBody) {
			gGlobalOverlay = font.fBody->local_overlay;
			gOverlayID++;
		}
	}
	
	// Forget about what happened.
	FontsChanged();
	
	gFontLock.WriteUnlock();
}

// Must be called with fFontLock write locked.
void SFont::LoadOldPrefs(FILE* fp)
{
	font_family family;
	font_style style;
	char buffer[28];
	uint32 buf32;
	float size;
	
	for (int32 i=0; i<3; i++) {
		if (fread(family, 1, sizeof(family), fp) != sizeof(family))
			return;
		if (fread(style, 1, sizeof(style), fp) != sizeof(style))
			return;
		if (fread(buffer, 1, 4, fp) != 4)
			return;
		buf32 = fc_read32(buffer);
		if (!gStandardFonts[i])
			gStandardFonts[i] = new(std::nothrow) SFont();
		if (gStandardFonts[i]) {
			gStandardFonts[i]->SetFamilyAndStyle(family, style);
			gStandardFonts[i]->SetSize(*((float*)&buf32));
			xprintf("Loaded old settings font #%ld: %s / %s\n", i,
					gStandardFonts[i]->FamilyName(),
					gStandardFonts[i]->StyleName());
		}
	}
	
	// 28 bytes is the size of the fc_cache_status at this file's version.
	if (fread(buffer, 1, 28, fp) != 28)
		return;
	fc_bw_system_status.unflatten(buffer, 28);
	if (fread(buffer, 1, 28, fp) != 28)
		return;
	fc_gray_system_status.unflatten(buffer, 28);
}

void SFont::SavePrefs()
{
	int32 i;
	SFont* f;
	int32 type;
	int32 len;
	void* buffer = NULL;
	int32 bufferLen = 0;
	
	gFontLock.ReadLock();
	
	FILE* fp = open_settings_file("wb");
	if (fp) {
		// Write file header.
		type = B_HOST_TO_BENDIAN_INT32(FC_FONT_STATUS_ID);
		if (fwrite(&type, 1, 4, fp) != 4) goto finish;
		
		// Write all fonts.
		for (i=0; i<B__NUM_FONT; i++) {
			f = gStandardFonts[i];
			len = f ? f->FlattenedSize() : 0;
			if (bufferLen < len) {
				buffer = grRealloc(buffer, len, "SFont::SavePrefs() buffer", MALLOC_MEDIUM);
				bufferLen = len;
			}
			if (buffer == NULL && len > 0) goto finish;
			if (f && len > 0 && f->Flatten(buffer, len) == B_OK) {
				type = B_HOST_TO_LENDIAN_INT32(B_FONT_TYPE);
				len = B_HOST_TO_LENDIAN_INT32(len);
				if (fwrite(&type, 1, 4, fp) != 4) goto finish;
				if (fwrite(&len, 1, 4, fp) != 4) goto finish;
				if (fwrite(buffer, 1, len, fp) != len) goto finish;
			}
		}
		
		// Write global overlay if it exists.
		if (gGlobalOverlay.CountEntries() > 0 || gGlobalOverlay.IsNothing()) {
			SFont font;
			font_body* fb = font.fBody->edit();
			font.fBody = fb;
			if (fb) {
				fb->local_overlay = gGlobalOverlay;
				len = font.FlattenedSize();
				if (bufferLen < len) {
					buffer = grRealloc(buffer, len, "SFont::SavePrefs() buffer", MALLOC_MEDIUM);
					bufferLen = len;
				}
				if (buffer == NULL && len > 0) goto finish;
				if (len > 0 && font.Flatten(buffer, len) == B_OK) {
					type = B_HOST_TO_LENDIAN_INT32(kGlobalOverlay);
					len = B_HOST_TO_LENDIAN_INT32(len);
					if (fwrite(&type, 1, 4, fp) != 4) goto finish;
					if (fwrite(&len, 1, 4, fp) != 4) goto finish;
					if (fwrite(buffer, 1, len, fp) != len) goto finish;
				}
			}
		}
		
		// Write cache status settings.
		for (i=0; i<2; i++) {
			fc_cache_status* s = NULL;
			if (i == 0)			s = &fc_bw_system_status;
			else if (i == 1)	s = &fc_gray_system_status;
			len = s ? s->flattened_size() : 0;
			if (bufferLen < len) {
				buffer = grRealloc(buffer, len, "SFont::SavePrefs() buffer", MALLOC_MEDIUM);
				bufferLen = len;
			}
			if (buffer == NULL && len > 0) goto finish;
			if (s && !s->flatten((char*)buffer))
				len = 0;
			type = B_HOST_TO_LENDIAN_INT32(kSettingsStatus);
			len = B_HOST_TO_LENDIAN_INT32(len);
			if (fwrite(&type, 1, 4, fp) != 4) goto finish;
			if (fwrite(&len, 1, 4, fp) != 4) goto finish;
			if (len > 0)
				if (fwrite(buffer, 1, len, fp) != len) goto finish;
		}
	}
	
finish:
	gFontLock.ReadUnlock();
	
	if (buffer)
		grFree(buffer);
	if (fp)
		fclose(fp);
}

void SFont::GetStandardFont(font_which which, SFont* into)
{
	if ((which < B_PLAIN_FONT || (which-B_PLAIN_FONT) >= B__NUM_FONT) && which != 99)
		which = B_PLAIN_FONT;
		
	gFontLock.ReadLock();
	if (which == 99) {
		*into = SFont();
		font_body* fb = into->fBody->edit();
		into->fBody = fb;
		if (fb)
			fb->local_overlay = gGlobalOverlay;
	} else {
		if (gStandardFonts[which-1]) {
			gStandardFonts[which-1]->Refresh();
			*into = *gStandardFonts[which-1];
		} else *into = SFont();
	}
	gFontLock.ReadUnlock();
}

void SFont::SetStandardFont(font_which which, const SFont& from)
{
	if ((which < B_PLAIN_FONT || (which-B_PLAIN_FONT) >= B__NUM_FONT) && which != 99)
		return;
		
	gFontLock.WriteLock();
	if (which == 99) {
		if (from.fBody) {
			gGlobalOverlay = from.fBody->local_overlay;
			gOverlayID++;
		}
	} else {
		SFont req(from);
		req.SetSpacing(which == B_FIXED_FONT ? B_FIXED_SPACING : B_BITMAP_SPACING);
		req.SetEncoding(B_UNICODE_UTF8);
		req.SetRotation(0);
		req.SetShear(90);
		
		if (gStandardFonts[which-B_PLAIN_FONT]) {
			if (*gStandardFonts[which-B_PLAIN_FONT] != req) {
				*gStandardFonts[which-B_PLAIN_FONT] = from;
				atomic_or(&gFontsChanged, 1);
			}
		} else {
			gStandardFonts[which-B_PLAIN_FONT] = new(std::nothrow) SFont(from);
			atomic_or(&gFontsChanged, 1);
		}
	}
	gFontLock.WriteUnlock();
}

bool SFont::FontsChanged()
{
	return atomic_and(&gFontsChanged, 0) ? true : false;
}

void SFont::GetSystemFont(font_which which, SFont* into)
{
	// -------------- Default Standard Fonts --------------
	
	typedef struct {
		font_family   family;
		font_family   alt_family;
		uint16        face;
		float         size;
	} default_font_setting;
	
	static const default_font_setting system_default_fonts[B__NUM_FONT] = {
		{ "Swis721 BT",		"Dutch801 Rm BT",	B_REGULAR_FACE,	10.0 },
		{ "Swis721 BT",		"Dutch801 Rm BT",	B_BOLD_FACE,	12.0 },
		{ "Courier10 BT",	"Monospac821 BT",	B_REGULAR_FACE,	12.0 },
		{ "SymbolProp BT",	"Swis721 BT",		B_REGULAR_FACE,	12.0 },
		{ "Dutch801 Rm BT",	"Swis721 BT",		B_REGULAR_FACE,	10.0 }
	};

	// -------------- Default Overlays --------------
	
	typedef struct {
		uint32        first;
		uint32        last;
	} default_overlay_range;
	
	// These are the fonts we know about for ideograph characters, in order
	// of preference.  That is, we are going to pick the Japanese-oriented
	// ideograph font over the chinese one, if both exist.
	static const font_family ideograph_fonts[] = {
		"Bitstream SB Gothic JCK1", "Bitstream SB Gothic CJK1"
	};
	
	// These are the overlay ranges will we define for one of the above
	// ideograph fonts.
	static const default_overlay_range ideograph_ranges[] = {
		{ 0x2e80, 0xd7ff },		// ideographs
		{ 0xe000, 0xf8ff },		// private (why is there stuff here?)
		{ 0xf900, 0xfaff },		// compatibility ideographs
		{ 0xfe20, 0xfe2f },		// combining half marks
		{ 0xfe30, 0xfe4f },		// compatibility forms
		{ 0xfe50, 0xfe6f },		// small forms
		{ 0xff00, 0xffef },		// half and full forms
		
		// Here are some other characters that happen to be in the ideograph
		// fonts, so we might as well take advantage of them.
		{ 0x0400, 0x04ff },		// cyrillic
		{ 0x2000, 0x206f },		// general punctuation
		{ 0x2070, 0x209f },		// superscripts and subscripts
		{ 0x2100, 0x214f },		// letterlike symbols
		{ 0x2150, 0x218f },		// number forms
		{ 0x2190, 0x21ff },		// arrows
		{ 0x2200, 0x22ff },		// mathematical operators
		{ 0x2460, 0x24ff },		// enclosed alphanumerics
		{ 0x2500, 0x257f },		// box drawing
		{ 0x2580, 0x259f },		// block elements
		{ 0x25a0, 0x25ff },		// geometric shapes
		{ 0x2600, 0x26ff }		// miscellaneous symbols
	};
	
	if (which == 99) {
		*into = SFont();
		font_body* fb = into->fBody->edit();
		into->fBody = fb;
		if (fb) {
			fb->local_overlay.MakeEmpty();
			int family_id = -1;
			int32 i;
			for (i=0; i<(sizeof(ideograph_fonts)/sizeof(ideograph_fonts[0])); i++) {
				family_id = fc_get_family_id(ideograph_fonts[i]);
				if (family_id >= 0)
					break;
			}
			if (family_id >= 0) {
				for (i=0; i<(sizeof(ideograph_ranges)/sizeof(ideograph_ranges[0])); i++) {
					font_overlay_entry* e = fb->local_overlay.AddEntry(
						ideograph_ranges[i].first, ideograph_ranges[i].last);
					if (e) e->f_id = family_id;
				}
			}
		}
		return;
	}
	
	if (which < B_PLAIN_FONT || (which-B_PLAIN_FONT) >= B__NUM_FONT)
		which = B_PLAIN_FONT;
	const int32 index = which-B_PLAIN_FONT;
	*into = SFont();
	const char* family = system_default_fonts[index].family;
	if (into->SetFamilyAndFace(family, system_default_fonts[index].face) < B_OK) {
		family = system_default_fonts[index].alt_family;
		into->SetFamilyAndFace(family, system_default_fonts[index].face);
	}
	DebugPrintf(("Creating system font %s / 0x%x\n",
			family, system_default_fonts[index].face));
	if (into->IsValid())
		into->SetSize(system_default_fonts[index].size);
	else if (which != B_PLAIN_FONT)
		GetSystemFont(B_PLAIN_FONT, into);
	else
		*into = SFont();
	into->SetSpacing(B_BITMAP_SPACING);
	DebugPrintf(("Returning system font: family=%d, style=%d\n",
			into->FamilyID(), into->StyleID()));
}

void SFont::SetPreferredAntialiasing(int32 mode)
{
	// Convert from public codes into font cache codes.
	switch (mode) {
		case B_DISABLE_ANTIALIASING:	mode = FC_BLACK_AND_WHITE;	break;
		case B_NORMAL_ANTIALIASING:		mode = FC_GRAY_SCALE;		break;
		case B_TV_ANTIALIASING:			mode = FC_TV_SCALE;			break;
		default:						return;
	}
	
	if (preferred_antialiasing != mode) {
		preferred_antialiasing = mode;
		gSettingsID++;	// no need to be thread-safe.
	}
}

int32 SFont::PreferredAntialiasing()
{
	// Convert from private codes back to public.
	switch (preferred_antialiasing) {
		case FC_BLACK_AND_WHITE:		return B_DISABLE_ANTIALIASING;
		case FC_GRAY_SCALE:				return B_NORMAL_ANTIALIASING;
		case FC_TV_SCALE:				return B_TV_ANTIALIASING;
	}
	return B_NORMAL_ANTIALIASING;
}

void SFont::SetAliasingMinLimit(int32 min_size)
{
	if (aliasing_min_limit != min_size) {
		aliasing_min_limit = min_size;
		gSettingsID++; // no need to be thread-safe.
	}
}

void SFont::SetAliasingMaxLimit(int32 max_size)
{
	if (aliasing_max_limit != max_size) {
		aliasing_max_limit = max_size;
		gSettingsID++; // no need to be thread-safe.
	}
}

int32 SFont::AliasingMinLimit()
{
	return aliasing_min_limit;
}

int32 SFont::AliasingMaxLimit()
{
	return aliasing_max_limit;
}

void SFont::SetHintingMaxLimit(int32 max_size)
{
	if (hinting_limit != max_size) {
		hinting_limit = max_size;
		gSettingsID++; // no need to be thread-safe.
	}
}

int32 SFont::HintingMaxLimit()
{
	return hinting_limit;
}

// --------------------------------------------------------------------

SFont::SFont()
	: fBody(NULL), fLocked(NULL)
{
	//DebugPrintf(("Creating font %p\n", this));
}

SFont::SFont(const void* packet, size_t size)
	: fBody(NULL), fLocked(NULL)
{
	//DebugPrintf(("Creating font %p from packet\n", this));
	SetTo(packet, size);
}

SFont::SFont(const SFont& o)
	: fLocked(NULL)
{
	//DebugPrintf(("Creating font %p from other\n", this));
	fBody = o.fBody;
	if (fBody) fBody->acquire();
}

SFont::SFont(int family_id, int style_id, int size, bool allow_disable)
	: fBody(NULL), fLocked(NULL)
{
	//DebugPrintf(("Creating font %p from IDs\n", this));
	fBody = new font_body(family_id, style_id, size, allow_disable);
	if (fBody) fBody->acquire();
}

SFont::~SFont()
{
	//DebugPrintf(("Destroying font %p\n", this));
	if (fLocked)
		UnlockString();
	if (fBody) {
		fBody->release();
		fBody = NULL;
	}
}

SFont& SFont::operator=(const SFont& o)
{
	if (this == &o)
		return *this;
	if (fBody) fBody->release();
	fBody = o.fBody;
	if (fBody) fBody->acquire();
	return *this;
}

bool SFont::operator==(const SFont& o) const
{
	if (this == &o) return true;
	if (!fBody || !o.fBody) {
		return fBody == o.fBody;
	}
	
	return fBody->context.check_params(o.fBody->context.params());
}

bool SFont::operator!=(const SFont& o) const
{
	return !operator==(o);
}

status_t SFont::SetTo(const void* packet, size_t size, uint32 mask)
{
	// Bad packet data: Reset object and return.
	if (size < sizeof(fc_context_packet)) {
		if (fBody) {
			fBody->release();
			fBody = NULL;
		}
		return B_BAD_VALUE;
	}
	
	// No existing body: created a new body with the given data.
	if (!fBody) {
		fBody = new font_body((fc_context_packet*)packet);
		if (!fBody)
			return B_NO_MEMORY;
		fBody->acquire();
		return B_OK;
	}
	
	// A body exists but everything is being changed: force-set current
	// body to new data.
	if ((mask&B_FONT_ALL) == B_FONT_ALL) {
		fBody  = fBody->set_to((fc_context_packet*)packet);
		return fBody ? B_OK : B_NO_MEMORY;
	}
	
	// Otherwise, edit the current body and update it with the given data.
	font_body* fb = fBody->edit();
	fBody = fb;
	if (!fb)
		return B_NO_MEMORY;
	fb->context.set_context_from_packet((fc_context_packet*)packet, mask);
	
	size -= sizeof(fc_context_packet);
	if (size > 0 && (mask&B_FONT_OVERLAY) != 0) {
		void* data = ((fc_context_packet*)packet)+1;
		fb->local_overlay.Unpacketize(((fc_context_packet*)packet)+1, size);
	}
	
	return B_OK;
}

status_t SFont::GetBasicPacket(fc_context_packet* packet) const
{
	if (fBody) {
		fBody->context.get_packet_from_context(packet);
		packet->overlay_size = 0;
	} else {
		memset(packet, 0, sizeof(fc_context_packet));
		packet->f_id = 0xffff;
		packet->s_id = 0xffff;
	}
	return B_OK;
}

ssize_t SFont::PacketLength() const
{
	ssize_t size = sizeof(fc_context_packet);
	if (fBody) size += fBody->local_overlay.PacketSize();
	return size;
}

status_t SFont::GetPacket(void* buffer) const
{
	GetBasicPacket((fc_context_packet*)buffer);
	
	if (fBody) {
		const ssize_t osize = fBody->local_overlay.PacketSize();
		((fc_context_packet*)buffer)->overlay_size = osize;
		if (osize > 0) {
			fBody->local_overlay.Packetize(((fc_context_packet*)buffer)+1, osize);
		}
	}
	return B_OK;
}

status_t SFont::ReadPacket(TSession* a_session, bool include_mask)
{
	long		        mask;
	fc_context_packet   packet;
	session_buffer		buffer;

//	_sPrintf("--------set_font_context--------\n");

	a_session->sread(sizeof(fc_context_packet), (void*)&packet);
	// For now, just skip overlay information.
	if (packet.overlay_size > 0) {
		a_session->sread(packet.overlay_size, &buffer);
	}
	if (include_mask)
		a_session->sread(4, &mask);
	else
		mask = B_FONT_ALL;
/*
	_sPrintf("mask = %08x\n",mask);
	_sPrintf("f_id = %d\n",packet.f_id);
	_sPrintf("s_id = %d\n",packet.s_id);
	_sPrintf("size = %f\n",packet.size);
	_sPrintf("rotation = %f\n",packet.rotation);
	_sPrintf("shear = %f\n",packet.shear);
	_sPrintf("encoding = %d\n",packet.encoding);
	_sPrintf("spacing_mode = %f\n",packet.spacing_mode);
	_sPrintf("faces = %d\n",packet.faces);
	_sPrintf("flags = %d\n",packet.flags);
	_sPrintf("--------------------------------\n");
*/

	// We have to set the overlay information on our own, since it
	// isn't contiguous with the packet.
	packet.overlay_size = 0;
	status_t result = SetTo(&packet, sizeof(packet), mask);
	if (buffer.size() > 0 && (mask&B_FONT_OVERLAY) && fBody) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			fb->local_overlay.Unpacketize(buffer.retrieve(), buffer.size());
		}
	}
	return B_OK;
}

void SFont::WritePacket(TSession* a_session) const
{
	void*				tmpData = NULL;
	const size_t		len = PacketLength();
	
	void* data = a_session->inplace_write(len);
	if (!data) {
		tmpData = grMalloc(len, "get_font_context tmp buf", MALLOC_MEDIUM);
		data = tmpData;
	}
	
	if (data) {
		GetPacket(data);
	} else {
		fc_context_packet p;
		p.f_id = 0xffff;
		p.s_id = 0xffff;
		p.overlay_size = 0;
		a_session->swrite(sizeof(p), &p);
	}
	
	if (tmpData) {
		a_session->swrite(len, tmpData);
		grFree(tmpData);
	}
}

// ------------------------------------------------------------------------

bool SFont::IsValid() const
{
	return (fBody &&
			fBody->context.family_id() != 0xffff && fBody->context.style_id() != 0xffff);
}

BAbstractFont& SFont::SetTo(const BAbstractFont &source, uint32 mask)
{
	if (mask == B_FONT_ALL) {
		const SFont* f = dynamic_cast<const SFont*>(&source);
		if (f) {
			*this = *f;
			return *this;
		}
	}
	return BAbstractFont::SetTo(source, mask);
}

BAbstractFont& SFont::SetTo(font_which which, uint32 mask)
{
	SFont font;
	GetStandardFont(which, &font);
	return SetTo(font, mask);
}

status_t SFont::ApplyCSS(const char* css_style, uint32 mask)
{
	const status_t result = BAbstractFont::ApplyCSS(css_style, mask);
	if ((mask & B_FONT_OVERLAY) && fBody) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) fb->local_overlay.MakeEmpty();
	}
	return result;
}

status_t SFont::SetFamilyAndStyle(const font_family family, const font_style style)
{
	int fam = fc_get_family_id(family);
	if (fam < 0)
		return B_ERROR;
	int sty = fc_get_style_id(fam, style);
	if (sty < 0)
		return B_ERROR;
	SetFontIDs(fam, sty);
	return B_OK;
}

status_t SFont::SetFamilyAndFace(const font_family family, uint16 face)
{
	int fam = fc_get_family_id(family);
	if (fam < 0)
		return B_ERROR;
	int sty = fc_get_closest_face_style(fam, face);
	if (sty < 0)
		return B_ERROR;
	SetFontIDs(fam, sty, false, face);
	return B_OK;
}

void SFont::SetFamilyAndStyle(uint32 code, uint16 face)
{
	SetFontIDs((code >> 16) & 0xFFFF, code & 0xFFFF);
	if (!fBody || fBody->context.faces() != face) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			fb->context.set_faces(face);
			fb->refresh();
		}
	}
}

void SFont::SetSize(float val)
{
	if (!fBody || fBody->context.escape_size() != val) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			if (val < 0.0)
				val = 0.0;
			else if (val > FC_BW_MAX_SIZE)
				val = FC_BW_MAX_SIZE;
			if (fb->context.escape_size() != val) {
				fb->context.set_escape_size(val);
				fb->context.update_bits_per_pixel();
				fb->context.update_hinting();
			}
			fb->refresh();
		}
	}
}

void SFont::SetShear(float shear)
{
	if (shear < 45.0)
		SetShearRaw(-3.1415925635*0.25);
	else if (shear > 135.0)
		SetShearRaw(3.1415925635*0.25);
	else
		SetShearRaw((shear-90.0)*(3.1415925635/180.0));
}

void SFont::SetRotation(float rotation)
{
	SetRotationRaw(rotation*(3.1415925635/180.0));
}

void SFont::SetEncoding(uint8 val)
{
	if (!fBody || fBody->context.encoding() != val) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) fb->context.set_encoding(val < FC_SYMBOL_SET_COUNT ? val : 0);
	}
}

void SFont::SetSpacing(uint8 val)
{
	if (!fBody || fBody->context.spacing_mode() != val) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			fb->context.set_spacing_mode(val < B_SPACING_COUNT ? val : B_CHAR_SPACING);
		}
	}
}

void SFont::SetFace(uint16 val)
{
	if (fBody && fBody->context.faces() != val) {
		int sty = fc_get_closest_face_style(fBody->context.family_id(), val);
		if (sty) SetFontIDs(fBody->context.family_id(), sty, false, val);
	}
}

void SFont::SetFlags(uint32 val)
{
	if (!fBody || fBody->context.flags() != val) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			if (fb->context.flags() != val) {
				fb->context.set_flags(val);
				fb->context.update_bits_per_pixel();
				fb->context.update_hinting();
			}
			fb->refresh();
		}
	}
}

void SFont::GetFamilyAndStyle(font_family *family,
							  font_style *style) const
{
	if (family) {
		memset(*family, 0, sizeof(*family));
		const char* name = FamilyName();
		if (name)
			strncat(*family, name, sizeof(font_family));
	}
	if (style) {
		memset(*style, 0, sizeof(*style));
		const char* name = StyleName();
		if (name)
			strncat(*style, name, sizeof(font_style));
	}
}

uint32 SFont::FamilyAndStyle() const
{
	if (fBody)
		return ((fBody->context.family_id() << 16) | (fBody->context.style_id()));
	else
		return 0xffffffff;
}

float SFont::Size() const
{
	return fBody ? fBody->context.escape_size() : 9;
}

float SFont::Shear() const
{
	return (ShearRaw()*(180.0/3.1415925635)+90.0);
}

float SFont::Rotation() const
{
	return RotationRaw()*(180.0/3.1415925635);
}

uint8 SFont::Encoding() const
{
	return fBody ? fBody->context.encoding() : 0;
}

uint8 SFont::Spacing() const
{
	return fBody ? fBody->context.spacing_mode() : B_CHAR_SPACING;
}

uint16 SFont::Face() const
{
	return fBody ? fBody->context.faces() : 0;
}

uint32 SFont::Flags() const
{
	return fBody ? fBody->context.flags() : 0;
}

FontOverlayMap* SFont::EditOverlay()
{
	font_body* fb = fBody->edit();
	fBody = fb;
	if (fb) return &fb->local_overlay;
	return NULL;
}

const FontOverlayMap* SFont::Overlay() const
{
	if (fBody) return &fBody->local_overlay;
	return NULL;
}

void SFont::ClearOverlay()
{
	if (fBody && (fBody->local_overlay.CountEntries() || fBody->local_overlay.IsNothing())) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) fb->local_overlay.MakeEmpty();
	}
}

// ------------------------------------------------------------------------

void SFont::SetBitsPerPixel(int depth)
{
	if (!fBody || fBody->context.bits_per_pixel() != depth) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			fb->context.set_bits_per_pixel(depth);
			fb->refresh();
		}
	}
}

int SFont::BitsPerPixel() const
{
	return fBody ? fBody->context.bits_per_pixel() : FC_BLACK_AND_WHITE;
}

void SFont::SetFontIDs(int family_id, int style_id, bool allow_disable,
					   uint16 faces)
{
	if (!fBody || fBody->context.family_id() != family_id ||
			fBody->context.style_id() != style_id) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			fb->context.set_font_ids(family_id, style_id, allow_disable);
			if (faces) fb->context.set_faces(faces);
			fb->refresh();
		}
	}
}

void SFont::SetFamilyID(int id)
{
	if (!fBody || fBody->context.family_id() != id) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			fb->context.set_family_id(id);
			fb->refresh();
		}
	}
}

int SFont::FamilyID() const
{
	return fBody ? fBody->context.family_id() : 0xffff;
}

const char* SFont::FamilyName() const
{
	return fc_get_family_name(FamilyID());
}

void SFont::SetStyleID(int id)
{
	if (!fBody || fBody->context.style_id() != id) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			fb->context.set_style_id(id);
			fb->refresh();
		}
	}
}

int SFont::StyleID() const
{
	return fBody ? fBody->context.style_id() : 0xffff;
}

const char* SFont::StyleName() const
{
	return fc_get_style_name(FamilyID(), StyleID());
}

void SFont::SetRotationRaw(float val)
{
	if (!fBody || fBody->context.rotation() != val) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) fb->context.set_rotation(val);
	}
}

float SFont::RotationRaw() const
{
	return fBody ? fBody->context.rotation() : 0;
}

void SFont::SetShearRaw(float val)
{
	if (!fBody || fBody->context.shear() != val) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) fb->context.set_shear(val);
	}
}

float SFont::ShearRaw() const
{
	return fBody ? fBody->context.shear() : 90;
}

void SFont::ForceFlags(uint32 flags)
{
	if (!fBody)
		return;
	
	const uint32 final_flags = fBody->context.combine_flags(flags);
	if (final_flags != fBody->context.final_flags()) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) {
			fb->context.set_final_flags(final_flags);
			if (!fb->refresh()) {
				fb->context.update_bits_per_pixel();
				fb->context.update_hinting();
			}
		}
	}
}

bool SFont::Refresh()
{
	if (fBody && fBody->refresh_needed()) {
		font_body* fb = fBody->edit();
		fBody = fb;
		if (fb) fb->refresh();
		return true;
	}
	return false;
}

void SFont::GetHeight(font_height* height) const
{
	if (fBody)
		fBody->context.get_metrics(&height->ascent, &height->descent, &height->leading);
	else height->ascent = height->descent = height->leading = 0.0;
}

float SFont::Ascent() const
{
	if (fBody) return fBody->context.ascent();
	return 0.0;
}

float SFont::Descent() const
{
	font_height h;
	GetHeight(&h);
	return h.descent;
}

float SFont::Leading() const
{
	font_height h;
	GetHeight(&h);
	return h.leading;
}

void SFont::SetOrigin(float x, float y)
{
	font_body* fb = fBody->edit();
	if (fb) fb->context.set_origin(x, y);
	fBody = fb;
}

float SFont::OriginX() const
{
	return fBody ? fBody->context.origin_x() : 0;
}

float SFont::OriginY() const
{
	return fBody ? fBody->context.origin_y() : 0;
}

// ---------------------------------------------------------------------

float SFont::StringWidth(const uchar* str)
{
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		PRINT_CMD(("StringWidth(): %s\n", (const char*)str));
		
		/* read the pen position in local variables */
		float x = 0.0;
		float y = 0.0;
		/* calculate the escapement factor */
		const float f0 = fb->context.escape_size();
		const float f1 = (float)fb->context.size();
		const float escape_factor = (f0 != f1) ? (f0/f1) : 1.0;
	
		/* sum up length of characters in string */
		unicode_run run;
		while (str) {
			str = get_unicode_run(&run, str, fb);
			if (run.length > 0)
				fc_add_string_width(run.context, run.string, run.length,
									escape_factor, &x, &y);
		}
		
		/* return width */
		if (y == 0.0) {
			if (x >= 0.0)
				return x;
			else
				return -x;
		}
		return sqrt(x*x+y*y);
	}
	return 0;
}

static void truncate_name(const uchar *from, uchar *to, int length,
						  uint16 *tags, int level) {
	int     i;

	i = 0;
	/* do not copy the first cut portion */
	if (tags[i] >= level) {
		while (i < length)
			if (tags[++i] < level)
				break;
		*to++ = 0xe2;
		*to++ = 0x80;
		*to++ = 0xa6;
	}
	while (i < length) {
		/* copy the allowed portion */
		while (i < length) {
			*to++ = from[i++];
			if (tags[i] >= level)
				break;
		}
		if (i >= length) break;
		/* do not copy the cut portion */
		while (i < length)
			if (tags[++i] < level)
				break;
		/* add an ellipsis */
		*to++ = 0xe2;
		*to++ = 0x80;
		*to++ = 0xa6;
	}
	/* terminator */
	*to = 0;
}

static void truncate_string(SFont      &font,
							float      width,
							const uchar *str,
							int32      length,
							uint16     *tags,
							int32      tag_count,
							uchar      *result) {
	int32      min, med, max;
	float      w_min, w_med, w_max;

	/* test empty string */
	if (length == 0) {
		result[0] = 0;
		return;
	}
	/* test the full string first */
	w_max = font.StringWidth(str);
	if (w_max <= width) {
		memcpy(result, str, length+1);
		return;
	}
	/* init the recursive search. Check if there is space available at all */
	min = 1;
	med = 0;
	max = tag_count;
	result[0] = 0xe2;
	result[1] = 0x80;
	result[2] = 0xa6;
	result[3] = 0x00;
	w_min = font.StringWidth(result);
	if (w_min >= width) {
		result[0] = 0;
		return;
	}
	/* recursive search */
	while (min < max-1) {
		med = (min+max+1)>>1;
		truncate_name(str, result, length, tags, med);
		w_med = font.StringWidth(result);
		if (w_med < width) {
			min = med;
			w_min = w_med;
		}
		else {
			max = med;
			w_max = w_med;
		}
	}
	/* refresh the good string if necessary */
	if (med != min)
		truncate_name(str, result, length, tags, min);
}

void SFont::TruncateString(float width, int32 mode,
							const uchar* str, uchar* result)
{
	int         i, j, length;
	uint16      count;   // count of UTF8 char+1.
	uint16      *tags;
	uint16      b_tags[64];

	PRINT_CMD(("TruncateString(): %s\n", (const char*)str));
	
	/* alloc tags buffer */
	length = strlen((const char*)str);
	if (length > 64) {
		tags = (uint16*)grMalloc(length*sizeof(uint16),"tmp tags",MALLOC_CANNOT_FAIL);
		if (!tags)
			goto fail;
	} else
		tags = b_tags;

	/* preprocess tags index, depending mode */
	if (mode == B_TRUNCATE_END) {
		count = 0;
		for (i=0; i<length; i++) {
			if ((str[i] & 0xc0) != 0x80)
				count++;
			tags[i] = count;
		}
		count++;
	}
	else if (mode == B_TRUNCATE_BEGINNING) {
		count = 1;
		for (i=length-1; i>=0; i--) {
			tags[i] = count;
			if ((str[i] & 0xc0) != 0x80)
				count++;
		}
	}
	else {
		i = 0;
		j = length;
		count = 1;
		while (i<j) {
			while (TRUE) {
				j--;
				tags[j] = count;				
				if ((str[j] & 0xc0) != 0x80)
					break;
			}
			count++;
			if (i>=j)
				break;
			while (TRUE) {
				tags[i] = count;				
				i++;
				if ((str[i] & 0xc0) != 0x80)
					break;
			}
			count++;
		}		
	}
	/* truncate */
	truncate_string(*this, width, str, length, tags, count, result);

	/* free tags buffer */
	if (tags != b_tags)
		grFree((void*)tags);
	return;

fail:
	strcpy((char*)result, (const char*)str);
}

const uchar* SFont::GetEscapement(fc_escape_table* table, const uchar* string,
								  float delta_nsp, float delta_sp,
								  bool use_real_size)
{
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		PRINT_CMD(("GetEscapement(): %s\n", (const char*)string));
		
		unicode_run run;
		const uchar* next = get_unicode_run(&run, string, fb);
		if (run.length > 0) {
			fc_string draw;
			fc_get_escapement(run.context, table, run.string, run.length,
							  &draw, delta_nsp, delta_sp, use_real_size);
		} else { // 1/16/2001 - mathias: initialize fc_escape_table
			table->count = 0;
		}
		return next;
	}
	
	table->count = 0;
	return 0;
}

int32 SFont::HasGlyphs(const uchar* str, bool* has)
{
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		int32				length = 0;
		unicode_run			run;
		PRINT_CMD(("HasGlyphs(): %s\n", (const char*)str));
		
		while (str) {
			str = get_unicode_run(&run, str, fb);
			if (run.length > 0) {
				if (!fc_has_glyphs(run.context, run.string, run.length, has+length))
					return 0;
				length += run.length;
			}
		}
		return length;
	}
	return 0;
}

const uchar* SFont::GetGlyphs(fc_glyph_table* g_table, const uchar* string)
{
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		PRINT_CMD(("GetGlyphs(): %s\n", (const char*)string));
		
		unicode_run run;
		const uchar* next = get_unicode_run(&run, string, fb);
		if (run.length > 0) {
			fc_get_glyphs(run.context, g_table, run.string, run.length);
		} else { // 1/16/2001 - mathias: initialize fc_glyph_table
			g_table->count = 0;
		}
		return next;
	}
	g_table->count = 0;
	return 0;
}

const uchar* SFont::GetEdge(fc_edge_table* table, const uchar* string)
{
	table->count = 0;
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		PRINT_CMD(("GetEdge(): %s\n", (const char*)string));
		
		unicode_run run;
		const uchar* next = get_unicode_run(&run, string, fb);
		if (run.length > 0) {
			fc_get_edge(run.context, table, run.string, run.length);
		} else { // 1/16/2001 - mathias: initialize fc_edge_table
			table->count = 0;
		}
		return next;
	}
	return 0;
}

const uchar* SFont::GetBBox(fc_bbox_table* b_table, const uchar* string,
							float delta_nsp, float delta_sp, int mode, int string_mode,
							double* offset_h, double* offset_v)
{
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		PRINT_CMD(("GetBBox(): %s\n", (const char*)string));
		
		unicode_run run;
		const uchar* next = get_unicode_run(&run, string, fb);
		if (run.length > 0) {
			fc_get_bbox(run.context, b_table, run.string, run.length,
						delta_nsp, delta_sp, mode, string_mode, offset_h, offset_v);
		} else { // 1/16/2001 - mathias: initialize fc_bbox_table
			b_table->count = 0;
		}
		return next;
	}
	b_table->count = 0;
	return 0;
}

void SFont::GetStringBBox(const uchar* str, float delta_nsp, float delta_sp,
							int mode, fc_bbox* bbox)
{
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		int						i;
		double					offset_h, offset_v;
		fc_bbox_table			b_table;
		unicode_run				run;
	
		PRINT_CMD(("GetStringBBox(): %s\n", (const char*)str));
		
		/* init an empty inverted bounding box */
		bbox->left = 1000000.0;
		bbox->top = 1000000.0;
		bbox->right = -1000000.0;
		bbox->bottom = -1000000.0;
		
		offset_h = 0.5;
		offset_v = 0.5;
		
		while (str != NULL) {
			str = get_unicode_run(&run, str, fb);
			if (run.length <= 0)
				continue;
			fc_get_bbox(run.context, &b_table, run.string, run.length,
						delta_nsp, delta_sp, mode, true, &offset_h, &offset_v);
			for (i=0; i<b_table.count; i++) {
				if (bbox->left > b_table.bbox[i].left)
					bbox->left = b_table.bbox[i].left;
				if (bbox->top > b_table.bbox[i].top)
					bbox->top = b_table.bbox[i].top;
				if (bbox->right < b_table.bbox[i].right)
					bbox->right = b_table.bbox[i].right;
				if (bbox->bottom < b_table.bbox[i].bottom)
					bbox->bottom = b_table.bbox[i].bottom;
			}
		}
		
		/* test for an empty bbox, then return a normalized result */
		if ((bbox->left >= bbox->right) || (bbox->top >= bbox->bottom)) {
			bbox->left = 0.0;
			bbox->top = 0.0;
			bbox->right = -1.0;
			bbox->bottom = -1.0;
		}
	
	} else {
		bbox->left = 0.0;
		bbox->top = 0.0;
		bbox->right = -1.0;
		bbox->bottom = -1.0;
	}
	
}

void SFont::FlushFontSet()
{
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		fc_flush_font_set(&fb->context);
	}
}

status_t SFont::SaveFullSet(ushort* dir_id, char* filename, char* pathname)
{
	font_body* fb = fBody->use();
	fBody = fb;
	
	status_t ret;
	if (fb) {
		fc_file_entry* fe;
		fc_lock_font_list();
		ret = fc_save_full_set(&fb->context, dir_id, filename, pathname, &fe);
		if (ret != B_OK) {
			*pathname = 0;
		}
		fc_unlock_font_list();
		fc_flush_open_file_cache();
	} else {
		*dir_id = 0;
		*filename = 0;
		*pathname = 0;
		ret = B_BAD_VALUE;
	}
	
	return ret;
}

const uchar* SFont::LockString(fc_string* draw, const uchar* string,
							   float non_space_delta, float space_delta,
							   double* xx, double* yy, uint32 lock_mode)
{
	if (fLocked) {
		_sPrintf("SFont::LockString() called while already locked !!\n");
		return NULL;
	}
	
	font_body* fb = fBody->use();
	fBody = fb;
	if (fb) {
		PRINT_CMD(("LockString(): %s\n", (const char*)string));
		
		unicode_run run;
		const uchar* next = get_unicode_run(&run, string, fb);
		
		fLocked = run.context ? run.context : &fb->context;
		fc_lock_string((fc_context*)fLocked, draw, run.string, run.length,
					   non_space_delta, space_delta, xx, yy, lock_mode);
		return next;
	}
	return NULL;
}

void SFont::UnlockString()
{
	if (fLocked) {
		PRINT_CMD(("UnlockString()\n"));
		fc_unlock_string((fc_context*)fLocked);
		fLocked = NULL;
	} else {
		_sPrintf("SFont::UnlockString() called but not locked !!\n");
	}
}

// ----------------------- BFlattenable Interface -----------------------

bool SFont::IsFixedSize() const
{
	return true;
}

type_code SFont::TypeCode() const
{
	return B_FONT_TYPE;
}

ssize_t SFont::FlattenedSize() const
{
	return BAbstractFont::FlattenedSize();
}

bool SFont::AllowsTypeCode(type_code code) const
{
	return code == B_FONT_TYPE;
}

status_t
SFont::Flatten(void *buffer, ssize_t size) const
{
	return BAbstractFont::Flatten(buffer, size);
}

status_t
SFont::Unflatten(type_code c, const void *buf, ssize_t size)
{
	return BAbstractFont::Unflatten(c, buf, size, true);
}

status_t
SFont::Unflatten(type_code c, const void *buf, ssize_t size, bool force_valid)
{
	return BAbstractFont::Unflatten(c, buf, size, force_valid);
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------

static fc_file_entry* grab_file_entry(const fc_context* context, uint16* file_id);

fc_context* overlay_range::look_up(uint32 symbol, font_body* font)
{
	if (locked) fc_unlock_file();
	locked = true;
	fc_lock_file();
	
	//printf("Looking up symbol 0x%lx\n", symbol);
	
	// Default range is all characters.
	start = 0; end = 0xffffffff;
		
	// Where might we find this character?
	
	// First, check in the base font.
	
	num_files = 1;
	file_entries[0] = grab_file_entry(&font->context, &file_ids[0]);
	if (!file_entries[0]) {
		// If we can't find out anything about the characters in
		// the base font, then just disable overlays.
		//printf("No character map!\n");
		num_files = 0;
		return &font->context;
	}
	
	if (fc_fast_has_glyph(file_entries[0], symbol)) {
		// The base font has this character, so just continue running
		// as long as the remaining characters in the string are
		// contained in it.
		//printf("Use base font!\n");
		return &font->context;
	}
	
	// Now check for a local overlay.
	
	fc_context* context = add_overlay(symbol, font,
									  font->local_overlay, font->local_cache);
	if (context) {
		// add_overlay() had to unlock the file list, so now make sure it
		// is still valid.
		refresh_files();
		return context;
	}
	
	// Now check for a global overlay.
	
	if ((font->context.flags()&B_DISABLE_GLOBAL_OVERLAY) == 0) {
		context = add_overlay(symbol, font,
								gGlobalOverlay, font->global_cache);
		if (context) {
			// add_overlay() had to unlock the file list, so now make sure it
			// is still valid.
			refresh_files();
			return context;
		}
	}
	
	// Whoops!  Nothing has this character.  Mark as running in "missing
	// character" mode, using the base font's context but searching through
	// all the files we found for someone with later characters.
	//printf("Character not found!\n");
	missing = true;
	refresh_files();
	return &font->context;
}

fc_context* overlay_range::add_overlay(uint32 symbol, font_body* font,
									   FontOverlayMap& map, overlay_cache& cache)
{
	uint32 lstart, lend;
	const font_overlay_entry* entry;
	int32 i = map.Lookup(symbol, &entry, &lstart, &lend);
	
	// The range of whatever we have is the minimum of all sets.
	if (start < lstart) start = lstart;
	if (end > lend) end = lend;
	
	//printf("Adding file #%ld: range=(0x%lx-0x%lx), combined=(0x%lx-0x%lx), item=%ld\n",
	//		num_files, lstart, lend, start, end, i);
	
	if (i >= 0) {
		// Have a substitution, so retrieve it.  Note that we have to
		// unlock the font list before doing so, since this will try to
		// acquire that lock when constructing the font context.
		fc_unlock_file();
		fc_context* context = cache.retrieve(map.CountEntries(),
												0, i, entry, &font->context);
		fc_lock_file();
		
		file_entries[num_files] = grab_file_entry(context, &file_ids[num_files]);
		if (file_entries[num_files]) {
			if (fc_fast_has_glyph(file_entries[num_files], symbol)) {
				// This overlay has the character, so run as long as this
				// file contains characters and none before do.
				//printf("Use this context!\n");
				num_files++;
				return context;
			}
			
			// The overlay exists, but doesn't have the character.  Add it
			// to the search set so that we will continue to check for
			// characters in it.
			//printf("Adding file!\n");
			num_files++;
		}
	}
	
	return NULL;
}

void overlay_range::refresh_files()
{
	for (int32 i=0; i<num_files; i++) {
		if (file_ids[i] < file_table_count) {
			file_entries[i] = file_table + file_ids[i];
		} else {
			file_entries[i] = NULL;
		}
	}
}

static fc_file_entry* grab_file_entry(const fc_context* context, uint16* file_id)
{
	if (context->file_id < file_table_count) {
		fc_file_entry* fe = file_table + context->file_id;
		if (fe->block_bitmaps != NULL) {
			*file_id = context->file_id;
			return fe;
		}
	}
	return NULL;
}

bool overlay_range::verify(uint32 symbol) const
{
	//printf("Verifying %scharacter: 0x%lx in range (0x%lx-0x%lx)\n",
	//		missing ? "missing " : "", symbol, start, end);
	
	// First, bail immediately if this character is outside of the
	// overlay range.
	if (symbol < start || symbol > end) {
		//printf("Out of range!\n");
		return false;
	}
	
	if (num_files > 0) {
		// Now check to make sure that any of the preceeding fonts don't
		// have a character to override the original one we had selected.
		for (int32 i=0; i<(num_files-1); i++) {
			if (file_entries[i] != NULL && fc_fast_has_glyph(file_entries[i], symbol)) {
				//printf("Previous file has the character!\n");
				return false;
			}
		}
		// And check that the active font matches the missing state -- that
		// is, if we are showing missing characters, the active must still
		// be missing; otherwise, the active must have a valid character.
		if (file_entries[num_files-1] != NULL &&
				fc_fast_has_glyph(file_entries[num_files-1], symbol) == missing) {
			//if (missing) printf("Active file has the character!\n");
			//else printf("Active file doesn't have the character!\n");
			return false;
		}
	}
	
	return true;
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------

font_body* font_body::edit() const
{
	if (this == NULL || refs > 1) {
		font_body* fb = this ? new font_body(this) : new font_body();
		if (fb) fb->acquire();
		if (this) release();
		return fb;
	}
	const_cast<font_body*>(this)->clear_cache();
	return const_cast<font_body*>(this);
}

font_body* font_body::use() const
{
	if (this == NULL)
		return NULL;
	if (refs > 1) {
		font_body* fb = this ? new font_body(this) : new font_body();
		if (fb) fb->acquire();
		if (this) release();
		return fb;
	}
	return const_cast<font_body*>(this);
}

bool font_body::refresh()
{
	if (seq == gSettingsID)
		return false;
		
	clear_cache();
	context.update_bits_per_pixel();
	context.update_hinting();
	return true;
}

font_body* font_body::set_to(const fc_context_packet* p) const
{
	if (this == NULL || refs > 1) {
		font_body* fb = new font_body(p);
		if (fb) fb->acquire();
		if (this) release();
		return fb;
	}
	font_body* fb = const_cast<font_body*>(this);
	fb->context.set_context_from_packet(p, B_FONT_ALL);
	fb->clear_cache();
	return fb;
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------

void overlay_cache::clear()
{
	if (items) {
		for (int32 i=0; i<count; i++)
			delete items[i];
		free(items);
		items = NULL;
	}
}

fc_context* overlay_cache::retrieve(int32 cur_count, int32 cur_id,
					 int32 index, const font_overlay_entry* entry,
					 fc_context* base)
{
	if (cur_id != id || cur_count != count)
		clear();
	if (!items) {
		items = (fc_context**)malloc(sizeof(fc_context*)*cur_count);
		memset(items, 0, sizeof(fc_context*)*cur_count);
		count = cur_count;
		id = cur_id;
	}
	
	if (items[index]) return items[index];
	fc_context* c = new(std::nothrow) fc_context(base);
	if (!c)
		return base;
	int sty = fc_get_closest_face_style(entry->f_id, base->faces());
	if (!c->set_font_ids(entry->f_id, sty)) {
		delete c;
		return base;
	}
	c->set_faces(base->faces());
	items[index] = c;
	return c;
}

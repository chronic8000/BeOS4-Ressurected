/*	MFormatRegistry.cpp	*/

#include <list>
#include <set>
#include <string>
#include <MediaFormats.h>
#include <sys/stat.h>

#include "debug.h"
#include "MFormatRegistry.h"


uint32 MFormatRegistry::s_priority;


MFormatRegistry::MFormatRegistry() :
	MSavable("MFormatRegistry", 0)
{
	m_next_fmt = 1000;
}

MFormatRegistry::~MFormatRegistry()
{
}

static uint64 format_magic = 0x5544101213150000ULL;
#define ERR(n,a) { status_t err = (a) ; if (err != (n)) return (err < 0) ? err : B_ERROR; }

status_t 
MFormatRegistry::LoadState()
{
#if 0
	bool created = false;
	BFile * f = OpenFile(&created);
	StDeleve<BFile> d(f);
	if (!f) return B_OK;
	if (f->InitCheck()) return f->InitCheck();
	if ((f->Size() == 0) && created) return B_OK;

	BMallocIO buf;
	int32 code;
	if ((code = buf.SetSize(f->Size())) < 0) return code;
	ERR(f->Size(), f->ReadAt(0LL, buf.Buffer(), buf.Size()));
	uint64 magic;
	ERR(sizeof(magic), buf.Read(&magic, sizeof(magic)));
	if (magic != format_magic) {
		return B_IO_ERROR;
	}
	ERR(sizeof(code), buf.Read(&code, sizeof(code)));
	//	ignore version -- magic will change if we're not compatible
	//	int32 version = code;
	ERR(sizeof(code), buf.Read(&code, sizeof(code)));
	if ((code < sizeof(m_next_fmt)+sizeof(s_priority)) || (code > buf.Size())) return B_IO_ERROR;
	ERR(sizeof(m_next_fmt), buf.Read(&m_next_fmt, sizeof(m_next_fmt)));
	ERR(sizeof(s_priority), buf.Read(&s_priority, sizeof(s_priority)));
	code -= sizeof(m_next_fmt)+sizeof(s_priority);
	if ((code = buf.Seek(code, SEEK_SET)) < 0) return code;

	//	path count
	ERR(sizeof(code), buf.Read(&code, sizeof(code)));
	int32 pathcount = code;
	char * ss = 0;
	time_t tt;
	struct stat st;
	for (int ix=0; ix<pathcount; ix++) {
		if ((code = buf.Read(&code, sizeof(code))) < 0) {
			free(ss);
			return code;
		}
		char * ns;
		if (!(ns = realloc(ss, code+1))) {
			free(ss);
			return B_NO_MEMORY;
		}
		ss = ns;
		if ((code = buf.Read(ss, code)) != code) {
			free(ss);
			return (code < 0) ? code : B_IO_ERROR;
		}
		ss[code] = 0;
		if (stat(ss, &st) < 0) {
			m_next_fmt = 1000;
			s_priority = 0;
			free(ss);
			return errno;
		}
	}
	free(ss);

	//	read registered formats
	ERR(sizeof(code), buf.Read(&code, sizeof(code)));
	if ((code = buf.Seek(code, SEEK_SET)) < 0) return code;
	ERR(sizeof(code), buf.Read(&code, sizeof(code)));
	int32 formatcount = code;
#endif	
	return B_OK;
}

status_t 
MFormatRegistry::SaveState()
{
#if 0
	bool created = false;
	BFile * f = OpenFile(&created);
	StDeleve<BFile> d(f);
	if (!f) return EPERM;
	if (f->InitCheck()) return f->InitCheck();

	BMallocIO buf;
	int32 code = 1;
	ERR(sizeof(format_magic), buf.Write(&format_magic, sizeof(format_magic)));	//	magic
	ERR(sizeof(code), buf.Write(&code, sizeof(code)));		//	version
	code = sizeof(m_next_fmt) + sizeof(s_priority);
	ERR(sizeof(code), buf.Write(&code, sizeof(code)));		//	header size
	ERR(sizeof(m_next_fmt), buf.Write(&m_next_fmt, sizeof(m_next_fmt)));
	ERR(sizeof(s_priority), buf.Write(&s_priority, sizeof(s_priority)));

	//	figure out unique paths
	std::set<std::string> paths;
	for (map_type::iterator item(m_formats.begin()); item != m_formats.end(); item++) {
		for (list<std::string>::iterator ptr((*item).addons.begin()); ptr != (*item).addons.end(); ptr++) {
			std::string::size_type off = (*ptr).rfind('/');
			if (off > 0) {
				paths.insert((*ptr).substr(0, off));
			}
		}
	}
	code = paths.size();
	ERR(sizeof(code), buf.Write(&code, sizeof(code)));		//	number of paths
	for (std::set<std::string>::iterator ptr(paths.begin()); ptr != paths.end(); ptr++) {
		code = (*ptr).size();
		ERR(sizeof(code), buf.Write(&code, sizeof(code)));
		ERR(code, buf.Write((*ptr).c_str(), code));
		struct stat st;
		if (stat((*ptr).c_str(), &st) < 0) return errno;
		ERR(sizeof(time_t), buf.Write(&st.st_mtime, sizeof(time_t)));
	}
	code = 0;		//	delimiter
	ERR(sizeof(code), buf.Write(&code, sizeof(code)));
	code = m_formats.size();
	ERR(sizeof(code), buf.Write(&code, sizeof(code)));		//	number of format descriptions
	for (map_type::iterator ptr(m_formats.begin()); ptr != m_formats.end(); ptr++) {
		ERR(sizeof(media_format_description), buf.Write(&(*ptr).first, sizeof(media_format_description)));
		ERR(sizeof(media_format), buf.Write(&(*ptr).second.format, sizeof(media_format)));
		ERR(sizeof(int32), buf.Write(&(*ptr).second.priority, sizeof(media_format)));
		code = (*ptr).second.addons.count();
		ERR(sizeof(code), buf.Write(&code, sizeof(code)));
		for (list<std::string>::iterator path((*ptr).second.addons.begin()); path != (*ptr).second.addons.end(); path++) {
			code = (*path).size();
			ERR(sizeof(code), buf.Write(&code, sizeof(code)));
			ERR(code, buf.Write((*path).c_str(), code));
		}
		code = 0;
		ERR(sizeof(code), buf.Write(&code, sizeof(code)));	//	delimiter
	}
	code = 0;
	ERR(sizeof(code), buf.Write(&code, sizeof(code)));	//	delimiter

	if ((code = f->SetSize(buf.BufferLength())) < 0) return code;
	if ((code = f->WriteAt(0L, buf.Buffer(), buf.BufferLength())) != buf.BufferLength()) {
		f->WriteAt(0L, &code, sizeof(code));	//	make sure magic doesn't match
		return (code < 0) ? code : B_ERROR;
	}
#endif
	return B_OK;
}

status_t 
MFormatRegistry::AddFormats(
	const media_format_description *descriptions,
	int32 count,
	media_format *io_format,
	uint32 flags)
{
	status_t err;
	std::list<std::pair<const media_format_description, media_format> > found;

	//	The priority assignment may seem like madness, but there is method to it.
	//	It's used to determine which format_description to return in GetCodeFor().
	uint32 base = atomic_add(reinterpret_cast<int32*>(&s_priority), count)+count;

	for (int ix=0; ix<count; ix++) {
		map_type::iterator ptr(m_formats.find(descriptions[ix]));
		if (ptr != m_formats.end()) {
			if ((io_format->type > 0) && ((*ptr).second.format.type > 0) &&
					(io_format->type != (*ptr).second.format.type)) {
				return B_MISMATCHED_VALUES;
			}
			found.push_back(std::pair<const media_format_description, media_format>((*ptr).first, (*ptr).second.format));
		}
	}
	if (found.size() == 0) {
		//	new registration
		//	allocate a new format if it's clear
		switch (io_format->type) {
		case B_MEDIA_ENCODED_AUDIO:
			if (io_format->u.encoded_audio.encoding == 0) {
				io_format->u.encoded_audio.encoding = 
					(media_encoded_audio_format::audio_encoding)
					atomic_add(&m_next_fmt, 1);
			}
			break;
		case B_MEDIA_ENCODED_VIDEO:
			if (io_format->u.encoded_video.encoding == 0) {
				io_format->u.encoded_video.encoding = 
					(media_encoded_video_format::video_encoding)
					atomic_add(&m_next_fmt, 1);
			}
			break;
		case B_MEDIA_MULTISTREAM:
			if (io_format->u.multistream.format == 0) {
				io_format->u.multistream.format = atomic_add(&m_next_fmt, 1);
			}
			break;
		case B_MEDIA_RAW_AUDIO:
		case B_MEDIA_RAW_VIDEO:
			//	these are OK to register, too
			break;
		default:
			return B_MEDIA_BAD_FORMAT;
		}
		//	insert them all
		for (int ix=0; ix<count; ix++) {
			m_formats[descriptions[ix]].format = *io_format;
			m_formats[descriptions[ix]].priority = base-ix;
#if B_HOST_IS_LENDIAN
#define C4(x) ((char*)&descriptions[x].u)[3], ((char*)&descriptions[x].u)[2], ((char*)&descriptions[x].u)[1], ((char*)&descriptions[x].u)[0]
#else
#define C4(x) ((char*)&descriptions[x].u)[0], ((char*)&descriptions[x].u)[1], ((char*)&descriptions[x].u)[2], ((char*)&descriptions[x].u)[3]
#endif
			debug(2, "%c%c%c%c -> %ld\n", C4(ix), base-ix);
		}
	}
	else if (found.size() == 1) {
		if (flags & BMediaFormats::B_EXCLUSIVE) {
			return B_MEDIA_DUPLICATE_FORMAT;
		}
		//	existing registration
		//	make sure all descriptions are in there if not already
		if (count > 1) {
			goto force_registration;
		}
		*io_format = m_formats[descriptions[0]].format;
		if (flags & BMediaFormats::B_SET_DEFAULT) {
			m_formats[descriptions[0]].priority = base;
			debug(2, "%c%c%c%c -> %ld\n", C4(0), base);
		}
		else {
			debug(2, "%c%c%c%c retains %ld\n", C4(0), m_formats[descriptions[0]].priority);
		}
	}
	else {
		if (flags & BMediaFormats::B_EXCLUSIVE) {
			return B_MEDIA_DUPLICATE_FORMAT;
		}
		if (flags & BMediaFormats::B_NO_MERGE) {
			return B_MISMATCHED_VALUES;
		}
		//	oops! we need to coalesce existing codec IDs
		//	pick the first one
		//	update all the other ones
		//	make sure all new registrations are there, too
force_registration:
		std::list<std::pair<const media_format_description, media_format> >::iterator i(found.begin());
		for (std::list<std::pair<const media_format_description, media_format> >::iterator j(i);
				j != found.end(); j++) {
			if (io_format->type != (*j).second.type) {
				return B_MEDIA_BAD_FORMAT;
			}
		}
		*io_format = (*i).second;
		for (int ix=0; ix<count; ix++) {
			if ((flags & BMediaFormats::B_SET_DEFAULT) ||
					(m_formats.find(descriptions[ix]) == m_formats.end())) {
				m_formats[descriptions[ix]].priority = base-ix;
				debug(2, "%c%c%c%c -> %ld\n", C4(ix), base-ix);
			}
			else {
				debug(2, "%c%c%c%c retains %ld\n", C4(ix), m_formats[descriptions[ix]].priority);
			}
			m_formats[descriptions[ix]].format = *io_format;
		}
	}
	return B_OK;
}

#if 0
status_t 
MFormatRegistry::AddFormat(
	const media_format_description & description, 
	media_format & format)
{
	map_type::iterator ptr(m_formats.find(description));
	switch (format.type) {
	case B_MEDIA_ENCODED_AUDIO:
		if (format.u.encoded_audio.encoding == 0) {
			format.u.encoded_audio.encoding = 
				(media_encoded_audio_format::audio_encoding)
				atomic_add(&m_next_fmt, 1);
		}
		break;
	case B_MEDIA_ENCODED_VIDEO:
		if (format.u.encoded_video.encoding == 0) {
			format.u.encoded_video.encoding = 
				(media_encoded_video_format::video_encoding)
				atomic_add(&m_next_fmt, 1);
		}
		break;
	case B_MEDIA_MULTISTREAM:
		if (format.u.multistream.format == 0) {
			format.u.multistream.format = atomic_add(&m_next_fmt, 1);
		}
		break;
	default:
		/* these are the only ones we assign values to for now */
		;
	}
	if (ptr != m_formats.end()) {
		return B_MEDIA_DUPLICATE_FORMAT;
	}
	m_formats.insert(map_type::value_type(description, format));
	return B_OK;
}
#endif

status_t 
MFormatRegistry::GetFormats(
	BMessage & out_message,
	const char * format_name,
	const char * description_name,
	const char * priority_name,
	const char * addon_name)
{
	for (map_type::iterator ptr(m_formats.begin()); ptr != m_formats.end(); ptr++) {
		out_message.AddData(format_name, B_RAW_TYPE, &(*ptr).second.format, sizeof(media_format));
		out_message.AddData(description_name, B_RAW_TYPE, &(*ptr).first, sizeof(media_format_description));
		out_message.AddInt32(priority_name, (*ptr).second.priority);
		BMessage msg;
		for (std::list<std::string>::iterator zz((*ptr).second.addons.begin()); zz != (*ptr).second.addons.end(); zz++) {
			msg.AddString("be:_path", (*zz).c_str());
		}
		out_message.AddMessage(addon_name, &msg);
	}
	return B_OK;
}

status_t
MFormatRegistry::BindFormats(
	const media_format * formats,
	int32 count,
	const char * addon)
{
	map_type::iterator ptr(m_formats.begin());
	//	for each format in the db, check whether it matches any of the formats we have here
	while (ptr != m_formats.end()) {
		int32 enc = (*ptr).second.format.Encoding();
		for (int ix=0; ix<count; ix++) {
			//	test the encoding ID
			if (enc == formats[ix].Encoding()) {
				std::list<std::string> & l((*ptr).second.addons);
				for (std::list<std::string>::iterator zz(l.begin()); zz != l.end(); zz++) {
					if (!strcmp((*zz).c_str(), addon)) {
						goto had_it;
					}
				}
				l.push_back(addon);	//	implicit construction of string
				break;		//	we have now added this add-on to the format; move to next format
			}
		}
had_it:
		ptr++;
	}
	return B_OK;
}


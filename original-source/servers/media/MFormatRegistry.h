/*	MFormatRegistry.h	*/

#if !defined(M_FORMAT_REGISTRY_H)
#define M_FORMAT_REGISTRY_H


#include "MSavable.h"
#include "MediaFormats.h"
#include <map>
#include <list>
#include <string>


class MFormatRegistry : public MSavable {
public:
		MFormatRegistry();
virtual	~MFormatRegistry();

virtual	status_t LoadState();
virtual	status_t SaveState();

		status_t AddFormats(
				const media_format_description * descriptions,
				int32 count,
				media_format * io_format,
				uint32 flags);
/*
		status_t AddFormat(
				const media_format_description & description, 
				media_format & format);
*/
		status_t GetFormats(
				BMessage & out_message,
				const char * format_name,
				const char * description_name,
				const char * priority_name,
				const char * addon_name);
		status_t BindFormats(
				const media_format * list,
				int32 count,
				const char * addon);
private:
		struct info {
			media_format	format;
			uint32			priority;
			std::list<std::string>	addons;
		};
		typedef std::map<media_format_description, info> map_type;
		map_type m_formats;
		int32 m_next_fmt;
static	uint32 s_priority;
};


#endif /* M_FORMAT_REGISTRY_H */

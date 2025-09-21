/*	MMediaFilesManager.h	*/

#if !defined(M_MEDIA_FILES_MANAGER_H)
#define M_MEDIA_FILES_MANAGER_H

#include <Entry.h>
#include "str.h"

#include "MSavable.h"
#include "trinity_p.h"

#include <set>
#include <map>


class MMediaFilesManager :
	public MSavable
{
public:
		MMediaFilesManager();
		~MMediaFilesManager();

		status_t SaveState();
		status_t LoadState();

		status_t GetTypes(
				BMessage & request,
				BMessage & reply);
		status_t AddType(
				BMessage & request,
				BMessage & reply);
		status_t GetItems(
				BMessage & request,
				BMessage & reply);
		status_t SetItem(
				BMessage & request,
				BMessage & reply);
		status_t ClearItem(
				BMessage & request,
				BMessage & reply);
		status_t RemoveItem(
				BMessage & request,
				BMessage & reply);

private:

		static const float kDefaultAudioGain;

		struct mf_item {
			str_ref type;
			str_ref name;
			str_ref path;
			mf_item()
				{
				}
			mf_item(const mf_item & item)
				{
					type = item.type;
					name = item.name;
					path = item.path;
				}
			~mf_item()
				{
				}
			mf_item & operator=(const mf_item & other)
				{
					type = other.type;
					name = other.name;
					path = other.path;
					return *this;
				}
			bool operator<(const mf_item & other) const
				{
					return (type < other.type) || (type == other.type && (name < other.name));
				}
			bool operator==(const mf_item & other) const
				{
					return (type == other.type) && (name == other.name);
				}
		};
		struct mf_ref {
			mf_ref() : audio_gain(kDefaultAudioGain) {}
			mf_ref(const entry_ref& _ref, float _gain = kDefaultAudioGain) : ref(_ref), audio_gain(_gain) {}
			mf_ref(const mf_ref & clone) { operator=(clone); }
			mf_ref & operator=(const mf_ref & clone) { ref = clone.ref; audio_gain = clone.audio_gain; return *this; }
			~mf_ref() {}
			entry_ref ref;
			float audio_gain;
		};

		void create_default_sound(const char* event, const char* sound =0);
		void create_default_settings();

		std::set<str_ref> m_types;
		std::map<mf_item, mf_ref> m_items;		
};


#endif /* M_MEDIA_FILES_MANAGER_H */

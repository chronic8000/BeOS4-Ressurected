/*	MLatentManager.h	*/

#if !defined(M_LATENT_MANAGER_H)
#define M_LATENT_MANAGER_H

#include <Handler.h>
#include <Locker.h>
#include <MediaDefs.h>
#include <MediaNode.h>
#include "MSavable.h"
#include "MediaAddOn.h"

#include <map>
#include <vector>

#include <Message.h>
#include <image.h>

#include "str.h"

struct node_ref;

struct x_flavor_info :
	public flavor_info
{
	bool enabled;

	x_flavor_info & operator=(const flavor_info & i);
	x_flavor_info & operator=(const x_flavor_info & i);
};


struct latent_info {

	latent_info();
	latent_info(
		bool enabled,
		bool auto_load,
		image_id loaded,
		BMediaAddOn * node,
		int32 count,
		const char * path,
		media_addon_id maid);
	~latent_info();
	latent_info(const latent_info & i);
	latent_info & operator=(const latent_info & i);
	bool operator==(const latent_info & i);
	bool operator<(const latent_info & i);
	void clear_flavors();

	int32 * ref_count;	
	std::vector<x_flavor_info> * flavors;
	str_ref path;

	BMediaAddOn * add_on;
	image_id loaded;
	media_addon_id id;
	bool enabled;
	bool auto_start;
};


struct addon_ref
{
	str_ref path;

	addon_ref();
	addon_ref(const addon_ref & r);
	addon_ref(const char * str);
	~addon_ref();
	addon_ref & operator=(const addon_ref & ref);
};

struct addon_info
{
	addon_info();
	~addon_info();
	bool enabled;
};

struct addon_flavor_ref
{
	str_ref path;
	int32 id;

	addon_flavor_ref();
	addon_flavor_ref(const addon_flavor_ref & r);
	addon_flavor_ref(str_ref & handle, int32 id);
	~addon_flavor_ref();
	addon_flavor_ref & operator=(const addon_flavor_ref & ref);
};

struct addon_flavor_info
{
	addon_flavor_info();
	~addon_flavor_info();
	bool enabled;
};

bool operator==(const addon_ref & a, const addon_ref & b);
bool operator<(const addon_ref & a, const addon_ref & b);
bool operator==(const addon_flavor_ref & a, const addon_flavor_ref & b);
bool operator<(const addon_flavor_ref & a, const addon_flavor_ref & b);



class MLatentManager :
	public MSavable,
	public BHandler
{
public:
		MLatentManager();
		~MLatentManager();

		status_t SaveState();
		status_t LoadState();

		status_t DiscoverAddOns();
		status_t LoadDirectory(
				entry_ref & directory,
				bool recursive = true);
		status_t LoadAddOn(
				const char * path);
		status_t UnloadAddOn(media_addon_id);

		status_t GetAddonID(
				const char * path,
				media_addon_id * out_id);
			/* pointer returned is only good until other operation on Latent Manager */
		status_t FindLatent(
				media_addon_id id,
				latent_info ** out_latent);
		status_t FindLatent(
				node_ref*,
				latent_info *out_latent);

		status_t QueryLatents(
				const media_format * input,
				const media_format * output,
				const char * name,
				BMessage & reply,
				int max_hits,
				uint64 require_kinds = 0,
				uint64 deny_kind = 0);

		void SetNotify(
				status_t (*hook)(void *, BMediaAddOn *),
				void * cookie);

		status_t AutoStartNodes();

		status_t SniffRef(
				const entry_ref & file,
				uint64 kinds,
				BMessage & out_message,
				const char * attr_node,
				const char * attr_mime);

		status_t SniffType(
				const char * type,
				uint64 kinds,
				BMessage & out_message,
				const char * attr_node);

		status_t GetFlavorFor(
				const dormant_node_info & info,
				dormant_flavor_info * out_flavor,
				char * outPath,
				size_t outPathSize);
		status_t GetFileFormatsFor(
				const dormant_node_info & info,
				media_file_format * out_read_formats,
				int32 in_read_count,
				int32 * out_read_count,
				media_file_format * out_write_formats,
				int32 in_write_count,
				int32 * out_write_count);
		status_t StartDefaultNodes();
		status_t TranslateIDAndFlavor(
				const char * path,
				int32 * io_id,
				const char * flavor_name);
		status_t InstantiateLatent(
				BMessage & message,
				media_node & out);
		status_t InstantiateLatent(
				const char * path,
				int32 id,
				BMessage & message,
				media_node & out);
		status_t MakeNode(
				BMediaAddOn * addon,
				flavor_info & info,
				BMessage & message,
				BMediaNode ** out_node);
		status_t FindTemporaryDefault(
				media_node_id what,
				status_t err);

		const char * FindFlavorName(
				const char * path,
				int32 internal_id);
		void RescanFlavors(
				media_addon_id addon);

		virtual void MessageReceived(BMessage *message);


		inline media_node SoundCard() const;
		inline void SetSoundCard(media_node);
		
private:

		void HandleNodeMonitor(BMessage*);
		void HandleEntryCreated(entry_ref *newEntry);
		void HandleEntryRemoved(node_ref*);
		void HandleEntryChanged(node_ref*);
		void HandleEntryMoved(ino_t oldDir, ino_t node, dev_t device, entry_ref *newEntry);

		bool IsEntryUnder(const char *dir, entry_ref*);

		BLocker mLock;
		media_node m_soundcard;

		void GetAddonName(image_id, char*);
		void AddOnError(image_id, status_t, const char *msg);


		int32 fAddonID;

		typedef std::map<media_addon_id, latent_info> LatentMap;
		LatentMap mLatents;

		/* these are only used to remember state when starting up */
		std::map<addon_ref, addon_info> mAddOnInfo;
		std::map<addon_flavor_ref, addon_flavor_info> mAddOnFlavorInfo;
		std::map<media_node_id, BMediaNode *> mDefaultNodes;

		void GetToKnowFlavor(
				x_flavor_info & info,
				str_ref & path);

		status_t (*mNotifyHook)(void *, BMediaAddOn *);
		void * mNotifyCookie;
};

inline media_node
MLatentManager::SoundCard() const
{
	return m_soundcard;
}


inline void
MLatentManager::SetSoundCard(media_node node)
{
	m_soundcard = node;
}


#endif /* M_LATENT_MANAGER_H */


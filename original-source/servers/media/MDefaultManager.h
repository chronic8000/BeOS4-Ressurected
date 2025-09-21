/*	MDefaultManager.h	*/

#if !defined(M_DEFAULT_MANAGER_H)
#define M_DEFAULT_MANAGER_H

#include <MediaDefs.h>
#include "MSavable.h"
#include "MediaNode.h"

#include <map>
#include <Message.h>


class MDefaultManager :
	public MSavable
{
public:
		MDefaultManager();
		~MDefaultManager();

		status_t SaveState();
		status_t LoadState();

		status_t SetDefault(
				media_node_id for_what,
				BMessage & replicate_info);
		status_t GetDefault(
				media_node_id for_what,
				BMessage & replicate_info);
		status_t RemoveDefault(
				media_node_id for_what);

		status_t SetRunningDefault(
				media_node_id for_what,
				const media_node & node);
		status_t GetRunningDefault(
				media_node_id for_what,
				media_node & out_node);
		status_t RemoveRunningDefault(
				const media_node & node);
		uint32 GetRealtimeFlags();
		void SetRealtimeFlags(
				uint32 flags);
private:

		std::map <media_node_id, BMessage> mDefaults;
		std::map <media_node_id, media_node> mRunning;
		uint32 mRealtimeFlags;
};


#endif /* M_DEFAULT_MANAGER_H */

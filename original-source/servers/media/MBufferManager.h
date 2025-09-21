/*	MBufferManager.h	*/

#if !defined(M_BUFFER_MANAGER_H)
#define M_BUFFER_MANAGER_H

#include "MSavable.h"
#include "trinity_p.h"
#include "Buffer.h"

#include <map>
#include <set>


class MBufferManager :
	public MSavable
{
public:

					MBufferManager();
					~MBufferManager();

		status_t	PrintToStream();

		status_t	SaveState();
		status_t	LoadState();

		status_t	RegisterBuffer(team_id owner, const buffer_clone_info *info,
						media_buffer_id *out_id, const media_source &whereabouts);
		status_t	AcquireBuffer(team_id team, media_buffer_id id, media_source where);
		status_t	ReleaseBuffer(team_id team, media_buffer_id id, bool wasOwner, 
						BMessage *msg, const char *name=NULL);

		status_t	PurgeTeamBufferGroups(team_id team);

		status_t	AddBuffersTo(BMessage *message, const char *name=NULL);

		status_t	ReclaimBuffers(const media_buffer_id * ids, int32 count, team_id team, area_id groupArea);

		status_t	RegisterBufferGroup(BMessage * message, const char * name, team_id team);
		status_t	UnregisterBufferGroup(BMessage * message, const char * name, team_id team);
		void		RecycleBuffersWithOwner(port_id owner, team_id team);
		void		CleanupPurgedBufferGroup(team_id team, area_id orig, area_id clone, void * addr, bool wasOwner, BMessage & outBadBufs);

private:

		struct group_info {
			group_info() : clone(-1), addr(0) { }
			group_info(area_id a, void * p) : clone(a), addr(p) { }
			area_id		clone;
			void *		addr;
		};
		typedef		std::map<std::pair<team_id, area_id>, group_info> group_map;
		group_map	mBufferGroups;

		struct claimant_info {
			team_id		team;
			area_id		area;
			area_id		clone;
			void *		addr;
		};
		typedef		std::list<claimant_info> claimant_list;
		typedef		std::map<team_id, int32> holder_map;
		struct store_type {
			buffer_clone_info first;	/*	where it lives */
			int32 second;				/*	ref count */
			media_source third;			/*	current output owner */
//			area_id claim;				/*	area of buffer group that has issued a reclaim */
			team_id owner;				/*	team that actually owns the buffers */
			holder_map holders;			/*	teams that are currently using the buffers */
			claimant_list claimants;	/*	buffer groups currently requesting to get the buffers back */
			inline store_type()
				{
					second = 0;
					owner = B_BAD_VALUE;
				}
			inline store_type(
				const buffer_clone_info & a,
				int32 b,
				const media_source & c,
				team_id d)
			{
				first = a;
				second = b;
				third = c;
				owner = d;
//				claim = B_BAD_VALUE;
				holders[owner] = 1;
			}
			inline ~store_type() { }
			inline store_type(const store_type & clone) {
				(*this).operator=(clone);
			}
			inline store_type & operator=(const store_type & clone) {
				first = clone.first;
				second = clone.second;
				third = clone.third;
				owner = clone.owner;
//				claim = clone.claim;
				holders = clone.holders;
				claimants = clone.claimants;
				return (*this);
			};
		};

		typedef 	std::map<media_buffer_id, store_type> buffer_map;

		void		_RemoveGroupFromClaimants(
							team_id team,
							area_id area,
							void * addr);
		status_t	_UnregisterBufferGroup(area_id orig, team_id team);

		buffer_map	mBuffers;
		int32		mNextID;
		int32		mGroupIDs;
		BLocker		mLock;
};


#endif /* M_BUFFER_MANAGER_H */

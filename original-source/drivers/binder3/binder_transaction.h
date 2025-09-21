
#ifndef BINDER2_TRANSACTION_H
#define BINDER2_TRANSACTION_H

#include "binder_defs.h"

//#define TRACK_TRANSACTIONS 1

class binder_transaction
{
	public:

				enum {
					tfIsReply			= 0x0100,
					tfIsEvent			= 0x0200,
					tfIsAcquireReply	= 0x0400,
					
					tfAttemptAcquire	= 0x1000,
					tfRelease			= 0x2000,
					tfDecRefs			= 0x3000,
					tfRefTransaction	= 0xF000,
					
					tfPrivReferenced	= 0x0800
				};

				void *					operator new(size_t size, int32 dataSize=4, int32 offsetsSize=0);
				void					operator delete(void *p);

										binder_transaction(uint16 refFlags, void *ptr);
										binder_transaction(	uint32 _code,
															size_t _dataSize,
															const void *_data,
															size_t _offsetsSize,
															const void *_offsetsData);
										~binder_transaction();

				void					Init();

				status_t				ConvertToNodes(binder_team *from, iobuffer *io);
				status_t				ConvertFromNodes(binder_team *to);
				void					DetachTarget();

				uint8*					Data() { return &data[0]; }
				uint8*					Offsets() { return data+data_size; }
				
				uint16					refFlags() { return flags & tfRefTransaction; };
				bool					isInline() { return flags & tfInline; };
				bool					isRootObject() { return flags & tfRootObject; };
				bool					isReply() { return flags & tfIsReply; };
				bool					isEvent() { return flags & tfIsEvent; };
				bool					isAcquireReply() { return flags & tfIsAcquireReply; };
				bool					isAnyReply() { return flags & (tfIsReply|tfIsAcquireReply); };

				void					setInline(bool f) { if (f) flags |= tfInline; else flags &= ~tfInline; };
				void					setRootObject(bool f) { if (f) flags |= tfRootObject; else flags &= ~tfRootObject; };
				void					setReply(bool f) { if (f) flags |= tfIsReply; else flags &= ~tfIsReply; };
				void					setEvent(bool f) { if (f) flags |= tfIsEvent; else flags &= ~tfIsEvent; };
				void					setAcquireReply(bool f) { if (f) flags |= tfIsAcquireReply; else flags &= ~tfIsAcquireReply; };

				binder_team *			team;
				binder_transaction *	next;		// next in the transaction queue
				binder_node *			target;		// the receiving binder
				binder_thread *			sender;
				binder_thread *			receiver;
				uint32					code;
				int32					offsets_size;
				int32					data_size;
				uint16					flags;
				int16					priority;
			
				#if TRACK_TRANSACTIONS
				binder_transaction *	chain;		// debugging
				#endif
				
				// This is the actual transaction data.  The binder offsets
				// appear at (data + data_size).
				uint8					data[1];
};

#endif

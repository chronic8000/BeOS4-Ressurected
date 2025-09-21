
#ifndef BINDER2_TRANSACTION_H
#define BINDER2_TRANSACTION_H

#include "binder_defs.h"

class binder_transaction
{
	public:

				enum {
					tfIsReply = 0x0100,
					tfIsEvent = 0x0200,
					tfRelease = 0x2000,
					tfDecRefs = 0x8000,
					tfRefTransaction = 0xF000
				};

				void *					operator new(size_t size, int32 dataSize=4);
				void					operator delete(void *p);

										binder_transaction(int32 unrefType, void *ptr);
										binder_transaction(int32 _numValues, int32 _size, void *_data);
										~binder_transaction();

				void					Init();

				status_t				ConvertToNodes(binder_team *from, iobuffer *io);
				status_t				ConvertFromNodes(binder_team *to);

				uint16					refFlags() { return flags & tfRefTransaction; };
				bool					isSync() { return flags & tfSynchronous; };
				bool					isInline() { return flags & tfInline; };
				bool					isRootObject() { return flags & tfRootObject; };
				bool					isReply() { return flags & tfIsReply; };
				bool					isEvent() { return flags & tfIsEvent; };

				void					setSync(bool f) { if (f) flags |= tfSynchronous; else flags &= ~tfSynchronous; };
				void					setInline(bool f) { if (f) flags |= tfInline; else flags &= ~tfInline; };
				void					setRootObject(bool f) { if (f) flags |= tfRootObject; else flags &= ~tfRootObject; };
				void					setReply(bool f) { if (f) flags |= tfIsReply; else flags &= ~tfIsReply; };
				void					setEvent(bool f) { if (f) flags |= tfIsEvent; else flags &= ~tfIsEvent; };

				binder_team *			team;
				binder_transaction *	next;		// next in the transaction queue
				binder_node *			target;		// the receiving binder
				binder_thread *			sender;
				binder_thread *			receiver;
				int32 					size;
				int32 					numValues;
				uint16					flags;
				uint8					data[1];	// the actual values
};

class transaction_list
{
	public:
		binder_transaction *	head;
		binder_transaction **	tail;
		int32					num;
		
		inline void push(binder_transaction *t)
		{
			t->next = head;
			if (tail == &head) tail = &t->next;
			head = t;
			num++;
		}
		
		inline void enqueue(binder_transaction *t)
		{
			*tail = t;
			tail = &t->next;
			num++;
		}
		
		inline binder_transaction * dequeue()
		{
			binder_transaction *top = head;
			if (top) {
				if (tail == &top->next) tail = &head;
				head = top->next;
				num--;
				ASSERT(num >= 0);
			} else {
				ASSERT(num == 0);
			}
			return top;
		}
		
		inline void remove(binder_transaction **itemPtr)
		{
			if (tail == &(*itemPtr)->next) tail = itemPtr;
			*itemPtr = (*itemPtr)->next;
			num--;
			ASSERT(num == 0);
		}
		
		inline transaction_list()	: head(NULL), tail(&head), num(0) { }
};

#endif

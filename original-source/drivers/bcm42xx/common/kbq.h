#ifndef _KBQ_H
#define _KBQ_H
#include "kbuf.h"

#ifdef KBQ_SPINLOCKS
#error Spinlocks are not yet implemented
#endif

typedef struct kbq
{
	struct kbuf   *head; // head and tail have to be at the same resp-
	struct kbuf   *tail; // ective offset as kbuf::next and kbuf::prev.

	size_t        len;
	#ifdef KBQ_SPINLOCKS
	spinlock_t    lock;
	#endif
} kbq_t;


/*
 * lock_kbq/unlock_kbq
 *
 * Get/release a mutex lock on the specified queue of kbufs.  Calls
 * to this can be nested within a thread, and are lightweight (they
 * use benaphores).
 *
 * Be sure to check the return value of kbq_lock()!  Otherwise the lock
 * could fail (eg, if someone deletes the kbq) and you could be messing
 * with freed memory.
 */
#ifdef KBQ_SPINLOCKS
static inline status_t lock_kbq(kbq_t* q) { return lock_spin(&q->lock); }
static inline void unlock_kbq(kbq_t* q) { unlock_spin(&q->lock); }
#else
static inline status_t lock_kbq(kbq_t* q) { return B_OK; }
static inline void unlock_kbq(kbq_t* q) { }
#endif


inline void init_kbq(kbq_t *q, const char* name)
{
	q->head = q->tail = (struct kbuf*)q;
	q->len = 0;
	
	#ifdef KBQ_SPINLOCKS
	init_spinlock(&q->lock,name);
	#endif
}

inline kbq_t* create_kbq(const char* name)
{
	kbq_t *ret = (kbq_t*)malloc(sizeof(kbq_t));
	if (ret==NULL) return NULL;
	init_kbq(ret,name);
	
	return ret;
}

/*
 *	purge_kbq
 *
 *	Delete all kbufs in the queue.
 */
static inline void purge_kbq(kbq_t *q)
{
	if (lock_kbq(q)==B_OK)
	{
		struct kbuf *b = q->head, *tmp;
		
		while (b!=(struct kbuf*)q)
		{
			tmp = b;
			b = b->next;
			delete_kb(tmp);
		}
		q->head = q->tail = (struct kbuf*)q;
		
		unlock_kbq(q);
	}
}

void print_kbq(kbq_t* q, int (*printf_func)(const char*, ...))
{
	kbuf_t* b;
	
	printf_func("kbq %p (len=%ld):\n    head=%p",q,q->len,q->head);
	for (b=q->head; b!=(kbuf_t*)q; b=b->next)
		printf_func("->%p",b->next);
	
	printf_func("\n    tail=%p",q->tail);
	for (b=q->tail; b!=(kbuf_t*)q; b=b->prev)
		printf_func("<-%p",b->prev);
	
	printf_func("\n");
}
#define printf_kbq(q) print_kbq(q,printf)
#define kprintf_kbq(q) print_kbq(q,kprintf)
#define dprintf_kbq(q) print_kbq(q,dprintf)

/*
 * kbq_pushall_front
 *
 * Move any and all the kbufs in src to the front of dest,
 * in the order traversed from src->head (ie src becomes a
 * subsequence of dest, from both directions).  Returns the
 * number of items transferred.
 */
static inline size_t kbq_pushall_front(kbq_t *src, kbq_t *dest)
{
	size_t ret = 0;
	
	if (lock_kbq(src)!=B_OK) return 0;
	if (src->len)
	{
		if (lock_kbq(dest)!=B_OK) {unlock_kbq(src); return 0;}
		
		dest->head->prev = src->tail;
		src->tail->next = dest->head;
		dest->head = src->head;
		dest->head->prev = (kbuf_t*)dest;
		dest->len += src->len;
		
		ret = src->len;
		src->head = src->tail = (kbuf_t*)src;
		src->len = 0;
		
		unlock_kbq(dest);
	}
	unlock_kbq(src);
	
	return ret;
}

/*
 * kbq_pushall_back
 *
 * Move any and all the kbufs in src to the back of dest,
 * in the order traversed from src->head (ie src becomes
 * a subsequence of dest, from both directions).  Returns
 * the number of items transferred.
 */
static inline size_t kbq_pushall_back(kbq_t *src, kbq_t *dest)
{
	size_t ret = 0;
	
	if (lock_kbq(src)!=B_OK) return 0;
	if (src->len)
	{
		if (lock_kbq(dest)!=B_OK) {unlock_kbq(src); return 0;}
		
		dest->tail->next = src->head;
		src->head->prev = dest->tail;
		dest->tail = src->tail;
		dest->tail->next = (kbuf_t*)dest;
		dest->len += src->len;
		
		ret = src->len;
		src->head = src->tail = (kbuf_t*)src;
		src->len = 0;
		
		unlock_kbq(dest);
	}
	unlock_kbq(src);
	
	return ret;
}

/*
 * kbq_isempty
 *
 * Returns true iff the queue has no kbufs.
 */
static inline bool kbq_isempty(kbq_t* q){ return q->len==0; }
/*
 * kbq_notempty
 *
 * Returns true iff the queue has kbufs.
 */
static inline bool kbq_notempty(kbq_t* q){ return q->len; }

/*
 * kbq_front
 *
 * Returns the kbuf at the front of the specified kbq, or NULL if
 * the kbq is empty.  Of course, lock the kbq first if you don't
 * want to get screwed by this.
 */
static inline kbuf_t* kbq_front(kbq_t* q)
{
	kbuf_t* ret = q->head;
	return ret==(kbuf_t*)q? NULL:ret;
}

/*
 * kbq_back
 *
 * Returns the kbuf at the end of the specified kbq, or NULL if
 * the kbq is empty.  Of course, lock the kbq first if you don't
 * want to get screwed by this.
 */
static inline kbuf_t* kbq_back(kbq_t* q)
{
	kbuf_t* ret = q->tail;
	return ret==(kbuf_t*)q? NULL:ret;
}

/*
 * kbq_unshift_kb
 *
 * Insert the specified kbuf at the front of the specified queue.
 * This does not stitch together any previous lists the kbuf was on.
 */
static inline void kbq_push_front(kbq_t* q, kbuf_t* b)
{
	if (lock_kbq(q)==B_OK)
	{
		++q->len;
		kb_prefix(b,q->head);
		unlock_kbq(q);
	}
}

/*
 * kbq_shift_kb
 *
 * Remove and return the kbuf at the front of the specified queue.
 * Returns NULL if the kbq is empty.
 */

static inline kbuf_t* kbq_pop_front(kbq_t* q)
{
	kbuf_t *ret = NULL;
	
	if (lock_kbq(q)==B_OK)
	{
		if (q->head != (kbuf_t*)q)
		{
			ret = q->head;
			kb_unlink(ret);
			--q->len;
			
			ret->next = ret->prev = NULL;
			ret->list = q;
		}
		unlock_kbq(q);
	}
	
	return ret;
}


/*
 * kbq_push_kb
 *
 * Insert the specified kbuf at the back of the specified queue.
 * This does not stitch together any previous lists the kbuf was on.
 */	
static inline void kbq_push_back(kbq_t* q, kbuf_t* b)
{
	if (lock_kbq(q)==B_OK)
	{
		++q->len;
		kb_affix(b,q->tail);
		unlock_kbq(q);
	}
}

/*
 * kbq_pop_kb
 *
 * Remove and return the kbuf at the end of the specified queue.
 * Returns NULL if the kbq is empty.
 */
static inline kbuf_t* kbq_pop_back(kbq_t* q)
{
	kbuf_t *ret = NULL;
	
	if (lock_kbq(q)==B_OK)
	{
		if (q->tail != (kbuf_t*)q)
		{
			ret = q->tail;
			kb_unlink(ret);
			--q->len;
			
			ret->next = ret->prev = NULL;
			ret->list = q;
		}
		unlock_kbq(q);
	}
	
	return ret;
}
#endif	/* KBQ_H */

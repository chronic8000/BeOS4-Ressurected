#ifndef _KBUF_H
#define _KBUF_H

#include <malloc.h>
#include <string.h>
#include <SupportDefs.h>

struct kbq;

typedef struct kbuf
{
	struct kbuf* next;
	struct kbuf* prev;
	// in case somebody wants to use this with the atomic
	// list routines, you might not want to move this away
	// from the front of the structure.  Unless you really
	// want to, of course.
	
	struct kbq* list; // the last kbq the buffer was popped from.
	
	bool owned;
	unsigned char *head, *data;
	unsigned char* pa;
	size_t space, len;
	size_t reserved_head, reserved_tail;
} kbuf_t;



static inline size_t kb_headroom(const kbuf_t *b){ return b->data - b->head; }
static inline size_t kb_tailroom(const kbuf_t *b){ return b->space - b->len - kb_headroom(b); }

static inline unsigned char* kb_data(const kbuf_t* b){return b->data;}
static inline unsigned char* kb_data_pa(const kbuf_t* b){return b->pa + kb_headroom(b);}

static inline void kb_clear(kbuf_t *b)
{
	b->data = b->head;
	b->len = b->reserved_head = b->reserved_tail = 0;
}


/*
 * kb_init
 *
 * Initialize a kbuf for a block of memory
 */
static inline void
kb_init(kbuf_t* b, unsigned char* buf, size_t space, unsigned char* pa)
{
	b->len = b->reserved_head = b->reserved_tail = 0;
	b->space = space;
	b->owned = false;
	b->head = b->data = buf;
	b->pa = pa;
}

/*
 * create_kb/delete_kb
 *
 * Allocate and initialize/deallocate a kbuf for a block of
 * memory
 */
static inline kbuf_t*
create_kb(unsigned char* buf, size_t space, unsigned char* pa)
{
	kbuf_t *ret;
	
	ret = (kbuf_t*)malloc(sizeof(kbuf_t));
	if (ret==NULL) return NULL;
	
	kb_init(ret,buf,space,pa);
	return ret;
}
inline void delete_kb(kbuf_t* b)
{
	if (b==NULL) return;
	if (b->owned) free(b->head);
	free(b);
}

/*
 * clone_kb
 *
 * Return a new kbuf with its own (malloc()ed) copy of the data,
 * or NULL if no memory.
 */
extern kbuf_t *clone_kb(const struct kbuf *b);

/*
 * copy_kb
 *
 * Copy a kbuf into a new kbuf, with the specified amounts of head-
 * and tailroom.  Returns the new kbuf or NULL if no memory.
 */
extern kbuf_t *copy_kb(const struct kbuf *b, size_t hroom, size_t troom);

/*
 * kb_reserve_front
 *
 * Make sure that a later call to kb_alloc_front(b,len) will succeed
 * and be fast (no memory shuffling).  Returns B_ERROR if there isn't
 * enough room in the buffer and B_OK if there is.
 */
static inline status_t kb_reserve_front(kbuf_t* b, size_t len)
{
	if (b->reserved_head + b->len + len + b->reserved_tail > b->space)
		return B_ERROR;
	
	b->reserved_head += len;
	
	if (kb_headroom(b) < b->reserved_head)
	{
		memmove(b->head+b->reserved_head, b->data, b->len);
		b->data = b->head+b->reserved_head;
	}
	return B_OK;
}

/*
 * kb_reserve_back
 *
 * Make sure that a later call to kb_alloc_back(b,len) will succeed
 * and be fast (no memory shuffling).  Return B_ERROR if there isn't
 * enough room in the buffer and B_OK if there is.
 */
static inline status_t kb_reserve_back(kbuf_t* b, size_t len)
{
	if (b->reserved_head + b->len + len + b->reserved_tail > b->space)
		return B_ERROR;
	
	b->reserved_tail += len;
	
	if (kb_tailroom(b) < b->reserved_tail)
	{
		memmove(b->head + b->space - b->reserved_tail - b->len, b->data, b->len);
		b->data = b->head + b->space - b->reserved_tail - b->len;
	}
	return B_OK;
}

/*
 * kb_alloc_front
 *
 * "allocate" len bytes at the start of the buffer, and return
 * a pointer to that memory or NULL if there isn't enough room.
 * A successful kb_reserve() below guarantees that this will
 * succeed and will be is fast.
 */
static inline unsigned char* kb_alloc_front(kbuf_t* b, size_t len)
{
	if (len + b->len + b->reserved_tail > b->space)
		return NULL;
	
	if (len > b->reserved_head)
		b->reserved_head = 0;
	else b->reserved_head -= len;
	
	if (kb_headroom(b) < b->reserved_head + len)
	{
		memmove(b->head+b->reserved_head+len, b->data, b->len);
		b->data = b->head+b->reserved_head;
	}
	else b->data -= len;
	
	b->len += len;
	return b->data;
}

/*
 * kb_alloc_back
 *
 * "allocate" len bytes of memory at the end of the buffer.
 * Return a pointer to this memory.
 */
static inline unsigned char* kb_alloc_back(kbuf_t* b, size_t len)
{
	if (b->reserved_head + b->len + len > b->space)
		return NULL;
	
	if (len > b->reserved_tail)
		b->reserved_tail = 0;
	else b->reserved_tail -= len;
	
	if (kb_tailroom(b) < b->reserved_tail + len)
	{
		size_t used = b->len + b->reserved_tail + len;
		memmove(b->head + b->space - used, b->data, b->len);
		b->data = b->head + b->space - used;
	}
	b->len += len;
	
	return b->data + b->len - len;
}

/*
 * kb_free_front
 *
 * "deallocate" len bytes at the start of the buffer.  Return
 * a pointer to the start of the buffer, or NULL if there aren't
 * len bytes in the buffer.
 */
static inline unsigned char* kb_free_front(kbuf_t* b, size_t len)
{
	if (b->len < len) return NULL;
	b->data += len;
	b->len -= len;
	return b->data;
}

/*
 * kb_free_back
 *
 * "deallocate" len bytes of memory from the end of the buffer.
 * returns a pointer to the new end of the buffer, or NULL if
 * there isn't enough room.
 */
static inline unsigned char* kb_free_back(kbuf_t* b, size_t len)
{
	if (len > b->len) return NULL;
	b->len -= len;
	return b->head + b->len;
}


/*
 * kb_prefix/kb_affix
 *
 * Insert the kbuf n before/after the specified kbuf b.
 */
static inline void kb_prefix(kbuf_t *n, kbuf_t* b)
{ n->next = b; n->prev = b->prev; b->prev->next = n; b->prev = n; }
static inline void kb_affix(kbuf_t *n, kbuf_t* b)
{ n->next = b->next; n->prev = b; b->next->prev = n; b->next = n; }

/*
 * kb_unlink
 *
 * Remove the specified kbuf from a chain of kbufs.
 * this works if the kbuf is the only one in a queue,
 * where the que head and tail sentinels are both the
 * queue (like kbq).
 */
static inline void kb_unlink(kbuf_t *b)
{ b->prev->next = b->next; b->next->prev = b->prev; }
#endif

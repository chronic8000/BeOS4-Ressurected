#include <malloc.h>

#if USE_DMALLOC
	#include "dmalloc.h"
#endif

#if USE_MAGIC
	/* ASSERT */
	#include <KernelExport.h>
	#include <OS.h>
	#include "aha8940.h"
#endif

#include "list.h"

#define LIST_MAGIC 'tsil'
#define LIST_MAGIC_X 'list'

struct list *
list_alloc(int datalen, void (*free_hook)(void *))
{
	struct list *l;
	l = calloc(sizeof(*l), 1);
	if (l) {
		if (datalen) {
			l->data = calloc(datalen, 1);
			if (!l->data) {
				free(l);
				return NULL;
			}
		}
#if USE_MAGIC
		l->magic = LIST_MAGIC;
#endif
		l->free_hook = free_hook;
	}
	return l;
}

void
list_free(struct list **l)
{
	struct list *p = *l, *q;

#if USE_MAGIC
	ASSERT(!(*l) || ((*l)->magic == LIST_MAGIC));
#endif
	
	while (p) {
		q = p->next;
		if (p->free_hook) (p->free_hook)(p->data);
#if USE_MAGIC
		p->magic = LIST_MAGIC_X;
#endif
		free(p);
		p = q;
	}

	*l = NULL;
}

void
list_prepend(struct list **l, struct list *item)
{
#if USE_MAGIC
	ASSERT(item->prev == NULL);
#endif

	item->next = *l;
	if (item->next)
		item->next->prev = item;
	*l = item;
}

void
list_append(struct list **l, struct list *item)
{
	struct list *p = *l;

#if USE_MAGIC
	ASSERT(item && (item->magic == LIST_MAGIC));
	ASSERT(!(*l) || ((*l)->magic == LIST_MAGIC));
#endif

	item->prev = item->next = NULL;

	if (!p) {
		*l = item;
		return;
	}

	while (p->next)
		p = p->next;

	p->next = item;
	item->prev = p;
}

void
list_move_item(struct list *item, struct list **from, struct list **to)
{
#if USE_MAGIC
	ASSERT(item && (item->magic == LIST_MAGIC));
	ASSERT(!(*from) || ((*from)->magic == LIST_MAGIC));
	ASSERT(!(*to) || ((*to)->magic == LIST_MAGIC));
#endif

	if (item->prev)
		item->prev->next = item->next;
	else
		*from = item->next;

	if (item->next)
		item->next->prev = item->prev;

	item->prev = NULL;
	item->next = *to;
	if (item->next)
		item->next->prev = item;
	*to = item;
}

struct list *
list_find(struct list *l,
		int (*compare)(const void *, const void *, size_t),
		const void *cookie1, size_t cookie2)
{
#if USE_MAGIC
	ASSERT(!l || (l->magic == LIST_MAGIC));
#endif
	while (l) {
		if (compare(l->data, cookie1, cookie2) == 0)
			break;
		l = l->next;
	}
	return l;
}

struct list *
list_remove_item(struct list *item, struct list **from)
{
#if USE_MAGIC
	ASSERT(item && (item->magic == LIST_MAGIC));
	ASSERT(!(*from) || ((*from)->magic == LIST_MAGIC));
#endif

	if (item->prev)
		item->prev->next = item->next;
	else
		*from = item->next;

	if (item->next)
		item->next->prev = item->prev;

	item->prev = item->next = NULL;

	return item;
}

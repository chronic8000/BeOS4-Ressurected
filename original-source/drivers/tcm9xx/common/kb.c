#include "kbuf.h"

// future features:
//   buffer pools (list w/ free_list)
//   refcounts

/*
 * kb_clone
 *
 * Allocate and return a new kbuf_t that has its own copy
 * of the specified kbuf_t's data (same head and tailroom).
 * Returns NULL if malloc fails.
 */
kbuf_t *clone_kb(const struct kbuf *b)
{
	kbuf_t *n = (kbuf_t*)malloc(sizeof(struct kbuf));
	if (n==NULL) return NULL;
	
	n->space = b->space;
	n->head = (unsigned char*)malloc(b->space);
	if (n->head == NULL){ free(n); return NULL; }
	n->owned=true;
	n->len = b->len;
	
	n->list = NULL;
	n->next = n->prev = NULL;
	n->data = n->head + kb_headroom(b);
	
	memcpy(n->data, b->data, n->len);
	return n;
}

/*
 * kb_copy
 *
 * Copy a kbuf (including data) and allocate more (or less)
 * head- or tailroom.  Returns the new buffer or NULL if
 * malloc() fails.
 */
kbuf_t *copy_kb(const struct kbuf *b, size_t hroom, size_t troom)
{
	struct kbuf *n = (struct kbuf*)malloc(sizeof(struct kbuf));
	if (n==NULL) return NULL;
	
	n->space = b->len + hroom + troom;
	n->head = (unsigned char*)malloc(n->space);
	if (n->head == NULL){ free(n); return NULL; }
	n->owned=true;
	n->len = b->len;
	
	n->list = NULL;
	n->next = n->prev = NULL;
	n->data = n->head + hroom;
	
	memcpy(n->data, b->data, n->len);
	return n;
}

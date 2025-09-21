#ifndef _AHA8940_LIST_H
#define _AHA8940_LIST_H

struct list {
#if USE_MAGIC
	uint32 magic;
#endif
	void *data;
	void (*free_hook)(void *);
	struct list *prev, *next;
};

struct list *list_alloc(int datalen, void (*free_hook)(void *));
void list_free(struct list **l);
void list_prepend(struct list **l, struct list *item);
void list_append(struct list **l, struct list *item);
void list_move_item(struct list *item, struct list **from, struct list **to);

struct list *list_find(struct list *l,
		int (*compare)(const void *, const void *, size_t),
		const void *cookie1, size_t cookie2);

/* doesn't free; just removes it from the list */
struct list *list_remove_item(struct list *item, struct list **from);

#endif

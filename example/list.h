#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

#define LIST_POISON1 ((void *) 0x00100100)
#define LIST_POISON2 ((void *) 0x00200200)

struct list_head 
{
    struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}

static inline int list_del_repeated(struct list_head *entry)
{
    if(entry->next == LIST_POISON1 && entry->prev == LIST_POISON2)
		return 1;
	return 0;	
}


static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

static inline void prefetch(const void *x) {;}


#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)


#endif


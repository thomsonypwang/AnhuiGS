#ifndef __SYS_LIST_H__
#define __SYS_LIST_H__

#include <stddef.h>
//#include <wm_utils.h>

typedef struct list_head {
	struct list_head *next, *prev;
} list_head_t;

/*
 * Doubly linked list implementation.
 */

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) list_head_t name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(list_head_t *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(list_head_t *new,
			      list_head_t *prev,
			      list_head_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * Insert a new entry after the specified head.
 */
static inline void list_add(list_head_t *new, list_head_t *head)
{
	__list_add(new, head, head->next);
}

/**
 * Insert a new entry before the specified head.
 */
static inline void list_add_tail(list_head_t *new, list_head_t *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 */
static inline void __list_del(list_head_t * prev, list_head_t * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __list_del_entry(list_head_t *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void list_del(list_head_t *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace(list_head_t *old,
				list_head_t *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 */
static inline int list_is_last(const list_head_t *list,
				const list_head_t *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 */
static inline int list_empty(const list_head_t *head)
{
	return head->next == head;
}

/**
 * list_entry - Get the struct for this entry
 * @head_ptr:	 Pointer to &list_head_t.
 * @struct_type: Type of the struct holding this member.
 * @member_name: Name of member in the struct that is of type list_struct.
 */
#define list_entry(__head_ptr, struct_type, member_name) \
	(struct_type *)((char *)__head_ptr - offsetof(struct_type, member_name))

/**
 * list_for_each - Traverse given list
 * @current:	Cursor of type &list_head_t.
 * @start:	Head of list.
 */
#define list_for_each(current, start) \
	for (current = (start)->next; current != (start); current = current->next)

/**
 * list_for_each_prev - Backward traversal of list
 * @current:	Cursor of type &list_head_t.
 * @start:	Head of list.
 */
#define list_for_each_prev(current, start) \
	for (current = (start)->prev; current != (start); current = current->prev)

/**
 * list_for_each_entry - iterate over list of given type
 * @current:	 Cursor of type &list_head_t.
 * @start:	 Head of list.
 * @member_name: Name of member in the struct that is of type list_struct.
 */
#define list_for_each_entry(current, current_type, start, member_name, member_type) \
	const member_type *__head_ptr = (start)->next;	\
	for (current = list_entry(__head_ptr, current_type, member_name); \
	     &current->member_name != (start); 	\
	     current = list_entry((const member_type *) current->member_name.next, current_type, member_name))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @current:	 Cursor of type &list_head_t.
 * @start:	 Head of list.
 * @member_name: Name of member in the struct that is of type list_struct.
 */
#define list_for_each_entry_reverse(current, current_type, start, member_name, member_type) \
	const member_type *__head_ptr = (start)->prev;	\
	for (current = list_entry(__head_ptr, current_type, member_name); \
	     &current->member_name != (start); 	\
	     current = list_entry((const member_type *) current->member_name.prev, current_type, member_name))

#endif /* __WMSDK_LIST_H__ */

/*
 *  COPYRIGHT (c) 1997 by Philips Semiconductors
 *
 *   +-----------------------------------------------------------------+
 *   | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *   | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *   | A LICENSE AND WITH THE INCLUSION OF THIS COPY RIGHT NOTICE.     |
 *   | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *   | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *   | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *   +-----------------------------------------------------------------+
 *
 *  Module name              : list.h 1.1
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : List
 *
 *  Last update              : 13:00:01 - 97/02/05
 *
 *  Description              :  
 *
 *              This module provides a simple list package that is
 *		useful for maintaining small lists of objects.
 *
 *		The caller keeps a pointer to a list (List *), initially
 *		set to nil.  This pointer is always passed by reference
 *		to any List package method.  The location of the list is
 *		not guaranteed to remain the same.
 *
 *		Elements may be pushed sequentially and retrieved either
 *		randomly by (0-based) index, or by popping the last element.
 *
 *		list_count	return the population of a list.
 *		list_get	return the element at the index in the list.
 *		list_pop	pop the last element from the list and
 *				return it.
 *		list_push	push an element onto the end of the list.
 *		list_set	set the value of the element at the index.
 *		list_size	change the size of the list as specified.
 */

#ifndef	_List_h
#define	_List_h

typedef struct _List	List;

int		list_count(List **list);
void *		list_get(List **list, int index);
void *		list_pop(List **list);
void		list_push(List **list, void *elt);
void		list_set(List **list, int index, void *elt);
void		list_size(List **list, int size);

#endif

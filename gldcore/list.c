/** $Id: list.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file list.c
	@addtogroup list List management routines
	@ingroup core
	
	Manage lists of objects in the rank indexes.
 @{
 **/
#include <stdlib.h>
#include <errno.h>
#include "list.h"

/** Create a list item and attach it to a list
	@return a pointer to the LISTITEM structure,
	\p NULL on error, errno
	- \p ENOMEM memory allocation failed
 **/
static LISTITEM *create_item(void *data,	/* a pointer to the data */
							 LISTITEM *prev,	/* the item preceding the new item */
							 LISTITEM *next)	/* the item following the new item */
{
	LISTITEM *item = malloc(sizeof(LISTITEM));
	if (item!=NULL)
	{
		item->data = data;
		item->next = next;
		item->prev = prev;
	}
	else
		errno = ENOMEM;
	return item;
}

/** Destroys an item in a list
 **/
static void destroy_item(LISTITEM *item) /**< a pointer to the LISTITEM structure of the item */
{
	if (item->prev!=NULL) 
		item->prev->next = item->next;
	if (item->next!=NULL) 
		item->next->prev = item->prev;
	free(item);
	item = NULL;
}

/** Create a new list
	@return a pointer to the new LIST structure, 
	\p NULL on error, errno
	- \p ENOMEM memory allocation failed
 **/
GLLIST *list_create(void)
{
	GLLIST *list = malloc(sizeof(GLLIST));
	if (list!=NULL)
	{
		list->first = NULL;
		list->last = NULL;
		list->size = 0;
	}
	else
		errno = ENOMEM;
	return list;
}

/** Destroy a list
 **/
void list_destroy(GLLIST *list) /**< a pointer to the LIST structure to destroy */
{
	LISTITEM *item = list->first;
	while ( item!=NULL )
	{
		LISTITEM *target = item;
		item = item->next;
		list->size--;
		destroy_item(target);
	}
	/* list size should be zero here */
}

/** Append an item to a list
	@return a pointer to the LISTITEM created and appended,
 	\p NULL on error, errno
	- \p ENOMEM memory allocation failed
 **/
LISTITEM *list_append(GLLIST *list, /**< a pointer to the LIST structure to which the item it to be appended */
					  void *data) /**< a pointer to the data which the new LISTITEM will contain */
{
	LISTITEM *item = create_item(data,list->last,NULL);
	if (item!=NULL) {
		if (list->first==NULL)
			list->first = item;
		if (list->last!=NULL)
			list->last->next = item;
		list->last = item;
		list->size++;
	}
	return item;
}

/** Shuffle a list
 **/
void list_shuffle(GLLIST *list)
{
	LISTITEM **index;
	unsigned int i=0;
	LISTITEM *item;

	if (list == NULL)
		return;
	if (list->size < 2)
		return;

	index = (LISTITEM**)malloc(sizeof(LISTITEM*)*list->size);
	for (item=list->first; item!=NULL; item=item->next)
		index[i++] = item;
	for (i=0; i<list->size; i++)
	{
		LISTITEM *from = index[i], *to;
		unsigned int j = rand() % list->size;
		void *temp = from->data;
		to = index[j];
		from->data = to->data;
		to->data = temp;
	}
	free(index);
}
/**@}*/

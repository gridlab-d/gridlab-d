/** $Id: list.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file list.h
	@addtogroup list
	@ingroup core
@{
 **/

#ifndef _LIST_H
#define _LIST_H

typedef struct s_listitem {
	void *data;
	struct s_listitem *prev;
	struct s_listitem *next;
} LISTITEM;

typedef struct s_list {
	unsigned int size;
	LISTITEM *first;
	LISTITEM *last;
} GLLIST;

GLLIST *list_create(void);
void list_destroy(GLLIST *list);
LISTITEM *list_append(GLLIST *list, void *data);
void list_shuffle(GLLIST *list);

#endif

/**@}*/

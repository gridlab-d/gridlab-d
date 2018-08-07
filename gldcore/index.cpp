/** $Id: index.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file index.c
	@addtogroup index Indexing routines
	@ingroup core
	
	Indexing routines allow objects to be organized in ranks.
 @{
 **/

#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include "output.h"
#include "index.h"

static unsigned int next_index_id = 0;

/** Create an index
	@return a pointer to the index structure
 **/
INDEX *index_create(int first_ordinal, /**< the first ordinal */
					int last_ordinal) /**< the last ordinal */
{
	int size = (unsigned int)(last_ordinal - first_ordinal + 1);
	INDEX *index = malloc(sizeof(INDEX));
	if (index!=NULL)
	{
		if (size<1)
		{
			errno = EINVAL;
			goto Undo;
		}
		index->ordinal = malloc(sizeof(GLLIST*)*size);
		if (index->ordinal==NULL)
		{
			errno = ENOMEM;
			goto Undo;
		}
		index->id = next_index_id++;
		index->first_ordinal = first_ordinal;
		index->last_ordinal = last_ordinal;
		memset(index->ordinal,0,sizeof(GLLIST*)*size);
		output_verbose("creating index %d", index->id);
	}
	else
	{
		errno = ENOMEM;
		return NULL;
	}

	/* reversal forces adjustment when first item is received */
	index->first_used=last_ordinal;
	index->last_used=first_ordinal;
	return index;
Undo:
	free(index);
	index = NULL;
	return NULL;
}

/** Insert an item into an index
	@return STATUS on SUCCESS, FAILED otherwise
 **/
STATUS index_insert(INDEX *index,	/**< the index to which the item is added */
					void *data,		/**< a pointer to the item to be added */
					int ordinal)	/**< the ordinal to which the item belongs */
{
	int pos = ordinal - index->first_ordinal;

	if (ordinal<index->first_ordinal) /* grow on bottom end */
	{
		/** @todo allow resizing indexes when ordinal is before first (ticket #28) */
		/* (this is very unusual and shouldn't happen the way things are now) */
		output_fatal("ordinal %d is too small (first is %d)",ordinal, index->first_ordinal);
		/*	TROUBLESHOOT
			This is an internal error caused by an attempt to index something improperly.
			Because the indexing system start at index 0, this means a negative index
			number was attempting, which is most likely a bug.  This usually occurs
			during object initialization and might be caused by invalid object numbers,
			either negative or greater than about 2 billion.  If your model is using a
			the <b><i>class</i>:<i>id</i></b> naming convention, check the object id numbers
			to make sure they are correct.
		*/
		errno = EINVAL;
		return FAILED;
	}
	else if (ordinal>=index->last_ordinal) /* grow on to end */
	{	
		int oldsize = index->last_ordinal - index->first_ordinal;
		int newsize = oldsize;
		GLLIST **newblock = NULL;
		while (ordinal >= index->first_ordinal + newsize)
			newsize *= 2;	/* double until it fits */
		newblock = malloc(sizeof(GLLIST*)*newsize);
		if (newblock==NULL)
		{
			output_fatal("unable to grow index %d: %s",index->id, strerror(errno));
			/*	TROUBLESHOOT
				An internal memory allocation error cause an index operation to fail.
				The message will usually include some explanation as to what cause
				the failure.  Remedy the indicated memory problem to fix the indexing problem.
			*/
			return FAILED;
		}
		output_verbose("growing index %d to %d ordinals", index->id, newsize);
		memset(newblock,0,sizeof(GLLIST*)*newsize);
		memcpy(newblock,index->ordinal,sizeof(GLLIST*)*oldsize);
		free(index->ordinal);
		index->ordinal = newblock;
		index->last_ordinal = index->first_ordinal + newsize;
	
	}
	if (index->ordinal[pos]==NULL) /* new list at ordinal */
	{
		index->ordinal[pos] = list_create();
		if (index->ordinal[pos]==NULL)
			return FAILED;
	}
	if (list_append(index->ordinal[pos],data)==NULL)
		return FAILED;
	if (ordinal<index->first_used) index->first_used=ordinal;
	if (ordinal>index->last_used) index->last_used=ordinal;
	return SUCCESS;
}

/** Shuffle an index to avoid excessive lock contention in models 
	where topologically adjacent objects are loaded sequentially
 **/
void index_shuffle(INDEX *index)	/**< the index to shuffle */
{
	int i, size = index->last_used - index->first_used;
	for (i=0; i<size; i++)
		list_shuffle(index->ordinal[i]);
	output_verbose("shuffled %d lists in index %d", size, index->id);
}

/**@}*/

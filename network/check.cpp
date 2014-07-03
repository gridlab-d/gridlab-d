/** $Id: check.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file check.cpp
	@addtogroup check Network check
	@ingroup network
	
	The check() function implements a network validation check on the
	current model.  The following checks are performed
	- <b>Connectivity</b>: this verifies that all nodes have a least one link
	  and that all links have both a \p to and \p from node.
    - <b>Swing bus</b>: this verifies that all islands have at least one swing bus.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"

EXPORT int check(void)
{
	unsigned int errcount=0;
	OBJECT *obj;
	FINDLIST *nodes = gl_find_objects(FL_NEW,FT_CLASS,SAME,"node",FT_END);
	FINDLIST *links = gl_find_objects(FL_NEW,FT_CLASS,SAME,"link",FT_END);
	FINDLIST *swings = gl_find_objects(FL_NEW,FT_CLASS,SAME,"node",AND,FT_PROPERTY,"type",EQ,"3",FT_END);

	// check links for connectivity
	int linkcount[10000];
	memset(linkcount,0,sizeof(linkcount));
	obj=NULL;
	while ((obj=gl_find_next(links,obj))!=NULL)
	{
		link *branch=OBJECTDATA(obj,link);
		if (branch->from==NULL && branch->to==NULL)
		{
			gl_error("link:%d is not connected on either end", obj->id);
			errcount++;
		}
		else if (branch->from==NULL) 
		{
			gl_error("link:%d is not connected on 'from' end", obj->id);
			errcount++;
			linkcount[branch->to->id]++;
		}
		else if (branch->to==NULL) 
		{
			gl_error("link:%d is not connected on 'to' end", obj->id);
			errcount++;
			linkcount[branch->from->id]++;
		}
		else
		{
			linkcount[branch->to->id]++;
			linkcount[branch->from->id]++;
			if (branch->Y.Mag()==0)
				gl_warning("link:%d is open", obj->id);
		}
	}

	// find swing buses and check link connectivity
	struct {
		OBJECT *swing;
		int count;
	} areas[1000];
	memset(areas,0,sizeof(areas));
	obj=NULL;
	while ( (obj=gl_find_next(nodes,obj))!=NULL)
	{
		node *bus=OBJECTDATA(obj,node);
		int n = bus->flow_area_num;
		areas[n].count++;
		if (bus->type==SWING)
		{
			if (areas[n].swing!=NULL)
				gl_warning("flow area %d has more than one swing bus (node:%d and node:%d)", n, areas[n].swing->id, obj->id);
			else
				areas[n].swing = obj;
		}
		if (linkcount[obj->id]==0)
			gl_warning("node:%d is not connected to anything", obj->id);
	}

	// check each area
	int i;
	for (i=0; i<sizeof(areas)/sizeof(areas[0]); i++)
	{
		if (areas[i].count>0 && areas[i].swing==NULL)
		{
			gl_error("flow area %d has no swing bus", i);
			errcount++;
		}

	}

	// check for islands without swing buses
	obj=NULL;
	OBJECTNUM bus[10000];
	memset(bus,0xff,sizeof(bus));
	while ( (obj=gl_find_next(swings,obj))!=NULL)
	{
		// mark swing bus
		bus[obj->id]=obj->id;

		// scan all links to spread swing info until no changes made
		bool changed;
		do {
			OBJECT *p=NULL;
			changed=false;
			while ((p=gl_find_next(links,p))!=NULL)
			{
				link *q=OBJECTDATA(p,link);
				OBJECT *f = q->from;
				OBJECT *t = q->to;
				if (f==NULL || t==NULL)
					continue;
				if (bus[f->id]==obj->id && bus[t->id]==0xffffffff)
				{
					changed = true;
					bus[t->id]=obj->id;
				}
				else if (bus[t->id]==obj->id && bus[f->id]==0xffffffff)
				{
					changed = true;
					bus[f->id]=obj->id;
				}
			}
		} while (changed);
	}
	obj=NULL;
	while ( (obj=gl_find_next(nodes,obj))!=NULL)
	{
		if (bus[obj->id]==0xffffffff)
		{
			gl_warning("node:%d is not connected to any swing bus", obj->id);
			errcount++;
		}
	}
	gl_free(nodes);
	gl_free(links);

	gl_output("Network check complete: %d errors found",errcount);
	return errcount;
}

/**@}*/

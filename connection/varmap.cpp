/// $Id$
///
/// Maintains a list of variables that are to be sent or received during an event
///

#include "connection.h"

varmap::varmap() 
{
	map = NULL;
}

int varmap::add(char *spec, COMMUNICATIONTYPE comtype)
{
	gl_verbose("varmap.add(char *spec='%s')",spec);

	// parse the spec
	char local[256], remote[1025], dir[9], remName[1025], threshold[1024];
	if ( sscanf(spec,"%255[^-<> ]%*[ ]%8[-<>]%*[ ]%1024[^\n]",local,dir,remote)!=3 &&
		 sscanf(spec,"%255[^-<> ]%8[-<>]%*[ ]%1024[^\n]",local,dir,remote)!=3 &&
		 sscanf(spec,"%255[^-<> ]%*[ ]%8[-<>]%1024[^\n]",local,dir,remote)!=3 &&
		 sscanf(spec,"%255[^-<> ]%8[-<>]%1024[^\n]",local,dir,remote)!=3 
		 )
	{
		gl_error("varmap::add(char *spec='%s'): varmap spec is invalid", spec);
		return 0;
	}

	// parse the direction
	DATAEXCHANGEDIRECTION dxd;
	if ( strcmp(dir,"<-")==0 ){
		dxd=DXD_READ;
	}
	else if ( strcmp(dir,"->")==0 ) dxd=DXD_WRITE;
	else
	{
		gl_error("varmap::add(char *spec='%s'): '%s' is not a valid data exchange direction", spec, dir);
		return 0;
	}

	// load the new varmap
	VARMAP *next = new VARMAP;
	if ( next==NULL )
	{
		gl_error("varmap::add(char *spec='%s'): unable to allocate memory for new VARMAP", spec);
		return 0;
	}
	next->local_name = new char[strlen(local)+1];
	if ( next->local_name==NULL )
	{
		gl_error("varmap::add(char *local='%s', char *remote='%s'): unable to allocate memory for VARMAP local name", local, remote);
		return 0;
	}
	strcpy(next->local_name,local);
	if(sscanf(remote, "%1024[^;];%*[ ]%1023[^\n]",remName, threshold) != 2)
	{
		next->remote_name = new char[strlen(remote)+1];
		if ( next->remote_name==NULL )
		{
			gl_error("varmap::add(char *local='%s', char *remote='%s'): unable to allocate memory for VARMAP remote name", local, remote);
			return 0;
		}
		strcpy(next->remote_name,remote);
		strcpy(next->threshold,"");
	} else {
		next->remote_name = new char[strlen(remName)+1];
		if ( next->remote_name==NULL )
		{
			gl_error("varmap::add(char *local='%s', char *remote='%s'): unable to allocate memory for VARMAP remote name", local, remName);
			return 0;
		}
		strcpy(next->remote_name,remName);
		strcpy(next->threshold,threshold);
	}
	next->obj = NULL;
	next->next = map;
	next->dir = dxd;
	next->ctype = comtype;
	map = next;
	return 1;
}
void varmap::resolve(void)
{
	VARMAP *item;
	for ( item=getfirst() ; item!=NULL ; item=getnext(item) )
	{	
		item->obj = new gld_property(item->local_name);
		if ( item->obj->is_valid() )
			gl_debug("connection: local variable '%s' resolved ok, object id %d", item->local_name, item->obj->get_object()->id);
		else
			gl_error("connection: local variable '%s' cannot be resolved", item->local_name);
	}
}

void varmap::linkcache(class connection_mode *connection, void *xlate)
{
	VARMAP *item;
	for ( item=getfirst() ; item!=NULL ; item=getnext(item) )
	{
		cacheitem *cache = connection->create_cache(item);
		if ( cache==NULL )
			gl_error("unable to create cache item for %s", item->local_name);
		else
			cache->set_translator((TRANSLATOR*)xlate);
	}
}

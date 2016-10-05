// $Id$
// 
// Maintains the data exchange cache
//
#include "connection.h"

cache::cache(void)
{
	size = 0x100;
	tail = 0;
	list = new CACHEID[size];
	memset(list,0,sizeof(list[0])*size);
	cacheitem::init();
}

cache::~cache(void)
{
	delete [] list;
}

int cache::option(char *command)
{
	char cmd[1024], arg[1024];
	switch ( sscanf(command,"%1023s %1023[^\n]",cmd,arg) ) {
	case 2:
		if ( strcmp(cmd,"size")==0 )
		{
			set_size(atoi(arg));
			return 1;
		}
	case 1:
	default:
		gl_error("cache::option(char *command='%s'): invalid command");
		return 0;
	}
}

void cache::set_size(size_t n)
{
	if ( n>size ) // only grow cache (shrinking it is too much trouble)
	{
		CACHEID *grown = new CACHEID[n];
		memset(grown,0,sizeof(grown[0])*n);
		if ( tail>0 ) memcpy(grown,list,tail*sizeof(list[0]));
		delete [] list;
		list = grown;
		size = n;
	}
	else
		gl_error("cache::set_size(size_t n=%d): invalid size is ignored", n);
}

bool cache::write(VARMAP *var, TRANSLATOR *xltr)
{
	OBJECT *obj = var->obj->get_object();
	PROPERTY *prop = var->obj->get_property();
	char *remote = var->remote_name;
	CACHEID id = cacheitem::get_id(var);
	cacheitem *item = cacheitem::get_item(id);
	if ( item==NULL ) item = new cacheitem(var);
	char buffer[1025];
	{	gld_rlock lock(var->obj->get_object());
		var->obj->to_string(buffer,sizeof(buffer));
	}
	return true;
	//return item->write(buffer);
}

bool cache::read(VARMAP *var, TRANSLATOR *xltr)
{
	OBJECT *obj = var->obj->get_object();
	PROPERTY *prop = var->obj->get_property();
	char *remote = var->remote_name;
	CACHEID id = cacheitem::get_id(var);
	cacheitem *item = cacheitem::get_item(id);
	if ( item==NULL ) item = new cacheitem(var);
	char buffer[1025];
	if ( item->read(buffer,sizeof(buffer)) )
	{
		if ( strlen(buffer)>0 )
		{
			gld_wlock lock(var->obj->get_object());
			return var->obj->from_string(buffer)>0; // convert incoming data
		}
		else
			return true; // no incoming data to process
	}
	else
		return false; // unable to read incoming data
}

cacheitem *cache::add_item(VARMAP *var)
{
	// create the cache item
	CACHEID id = cacheitem::get_id(var);
	cacheitem *item = cacheitem::get_item(id);
	if ( item==NULL ) 
		item = new cacheitem(var);

	// assign it to the cache list
	id = item->get_id();
	if ( id > size )
	{
		gl_error("cache id index overrun");
		delete item;
		return NULL;
	}
	if ( list[id]!=0 )
	{
		gl_error("cache id index collision");
		delete item;
		return NULL;
	}
	list[tail++] = id;

	// copy the initial value from the object
	item->copy_from_object();

	return item;
}

cacheitem *cache::find_item(VARMAP *var)
{
	int n;
	cacheitem *m;
	cacheitem *item = NULL;
	for(n=0; n < tail; n++){
		cacheitem *m = get_item(n);
		if(strcmp((const char*)var->local_name, (const char*)m->get_var()->local_name) == 0 && strcmp((const char*)var->remote_name, (const char*)m->get_var()->remote_name) == 0){
			item = m;
			break;
		}
	}
	return item;
}

void cache::dump(void)
{
	int n;
	gl_debug("ID     LOCAL/REMOTE     VALUE                          ");
	gl_debug("------ ---------------- -------------------------------");
	for ( n=0; n<tail ; n++ )
	{
		cacheitem *item = get_item(n);
		char name[1024];
		sprintf(name,"%s.%s/%s", item->get_object()->name, item->get_property()->name, item->get_remote());
		gl_debug("%6d %-16s %-32s", item->get_id(), name, item->get_buffer());
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// cacheitem implementation
////////////////////////////////////////////////////////////////////////////////////////
size_t cacheitem::indexsize = 0x100;
cacheitem **cacheitem::index = NULL;
cacheitem *cacheitem::first = NULL;
cacheitem::cacheitem(VARMAP *v)
{
	OBJECT *o = v->obj->get_object();
	PROPERTY *p = v->obj->get_property();
	char *r = v->remote_name;
Retry:
	id = get_id(v);

	// id already in use
	if ( index[id]!=NULL ) 
	{
		// item is the same
		cacheitem *item = get_item(id);
		if ( item->get_object()==o && item->get_property()==p && strcmp(item->get_remote(),r)==0 )
			throw "attempt to create a duplication cache item";
		else // different item hash collision
		{
			gl_debug("cache collison, growing cache size");
			grow();
			goto Retry;
		}
	}

	// setup new item
	marked = false;
	index[id] = this;
	var = v;
	value = new char[1025]; // TODO look into using prop->width instead to save some memory
	memset(value,0,1025);
	xltr = NULL;
	next = NULL;
	if ( first!=NULL )
		first->next = this;
	first = this;
}

void cacheitem::init(void)
{
	if ( index==NULL )
	{
		gl_verbose("cacheitem::init(): initial cache index size is %d", indexsize);
		index = new cacheitem*[indexsize];
		memset(index,0,sizeof(index[0])*indexsize);
	}
}

void cacheitem::grow(void)
{
	size_t newsize = indexsize * 0x100; // 16 times bigger
	gl_verbose("cacheitem::grow(): double cache index size to %d entries", newsize);
	cacheitem **newindex = new cacheitem*[newsize];
	memset(newindex,0,sizeof(newindex[0])*newsize);

	// TODO this takes a while--put a mechanism in place to keep/use a list of non-null entries
	// copy old ids to new ids
	cacheitem *n;
	for ( n=first ; n!=NULL ; n=n->next )
	{
		CACHEID newid = get_id(n->get_var());
		newindex[newid] = n;
	}
	delete index;
	index = newindex;
}

CACHEID cacheitem::get_id(VARMAP *v, size_t m)
{
	OBJECT *o = v->obj->get_object();
	PROPERTY *p = v->obj->get_property();
	char *r = v->remote_name;
	if ( m==0 ) m = indexsize;
	// TODO check uniformity of this hash function
	int64 a = o->id; // theoretically unlimited size
	int64 b = (int64)(p->addr); 
	int64 nb = (int64)(p->oclass->size);
	int64 c = 0; 
	int64 nc = strlen(r)*128;
	while ( *r!='\0' ) c += *r++;
	return (CACHEID)(((a*nb+b)*nc+c)%(int64)m);
}

bool cacheitem::read(char *buffer, size_t len)
{
	if ( len>strlen(value)+1 )
	{
		// copy/xlate cache to buffer
		if ( xltr==NULL )
			strcpy(buffer,value);
		else
			xltr(buffer,len,value,1025,TF_DATA,var);
		unmark();

		// clear cache
		*value = '\0';
		return true;
	}
	else
		return false;
}

bool cacheitem::write(char *buffer)
{
	size_t len = strlen(value);
	if ( len<1025 ) // TODO look into using prop->width instead to save memory
	{
		// check cache
		if ( *value!='\0' && strcmp(buffer,value)!=0 )
			gl_warning("cacheitem::write(): cache already contains different data for local '%s'/remote '%s'", var->local_name,var->remote_name);

		// copy/xlate buffer to cache
		if ( xltr==NULL )
			strcpy(value,buffer);
		else
			xltr(value,1025,buffer,strlen(buffer)+1,TF_DATA,var);
		mark();
		return true;
	}
	else
		return false;
}

bool cacheitem::copy_from_object(void)
{
	gld_property prop(get_object(),get_property());
	return prop.to_string(value,1024)<0 ? false : true;
}
bool cacheitem::copy_to_object(void)
{
	gld_property prop(get_object(),get_property());
	return prop.from_string(value)<0 ? false : true;
}

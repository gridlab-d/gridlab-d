/* $Id$
 */

#include <stdio.h>
#include "gridlabd.h"
#include "output.h"
#include "find.h"
#include "link.h"
#include "platform.h"

#ifdef WIN32
#define PREFIX ""
#define DLEXT "dll"
#else
#define PREFIX "lib"
#define DLEXT "so"
#endif

typedef struct s_linklist {
	void *ptr;
	struct s_linklist *next;
} LINKLIST;

class link {
private:
	LINKLIST *globals;
	LINKLIST *objects;
	char command[1024];
	char function[1024];
	LINKLIST *arguments;

private:
	void *handle;
	bool (*settag)(char *,char*);
	bool (*init)(void);
	TIMESTAMP (*sync)(TIMESTAMP t0);
	bool (*term)(void);

private:
	class link *next; ///< pointer to next link target
	static class link *first; ///< pointer for link target

public:
	inline static class link *get_first() { return first; }
	inline class link *get_next() { return next; };

public:
	link(char *file);
	~link(void);

private:
	void add_global(void *var);
	void add_object(void *obj);
	void set_command(char *cmd);
	void set_function(char *cmd);
	void add_argument(char *arg);

private:
	bool set_target(char *data);

	TIMESTAMP matlab_sync(TIMESTAMP t0);
	
	// TODO: add other target handlers here
};

// MATLAB
extern "C" {
	bool matlab_tag(char *tag, char *data);
	bool matlab_init(void);
}

link *link::first = NULL;

void link::add_global(void*val)
{
	LINKLIST *item = new LINKLIST;
	item->next = globals;
	item->ptr = val;
	globals = item;
}

void link::add_object(void*val)
{
	LINKLIST *item = new LINKLIST;
	item->next = objects;
	item->ptr = val;
	objects = item;
}

void link::add_argument(char*val)
{
	LINKLIST *item = new LINKLIST;
	item->next = arguments;
	item->ptr = val;
	arguments = item;
}

/* create a link module */
int link_create(char *file)
{
	try {
		link *lt = new link(file);
		return 1;
	}
	catch (char *msg)
	{
		output_error("link '%s' failed: %s", file, msg);
		return 0;
	}
	catch (...)
	{
		output_error("link '%s' failed: unhandled exception", file);
		return 0;
	}
}

/* initialize modules */
int link_init(void)
{
	link *mod;
	for ( mod=link::get_first() ; mod!=NULL ; mod=mod->get_next() )
	{
	}
	return 1;
}

TIMESTAMP link_sync(TIMESTAMP t0)
{
	return TS_NEVER;
}

link::link(char *filename)
{
	globals = NULL;
	objects = NULL;
	arguments = NULL;
	memset(command,0,sizeof(command));
	memset(function,0,sizeof(function));
	handle = NULL;
	settag = NULL;
	init = NULL;
	sync = NULL;
	term = NULL;

	FILE *fp = fopen(filename,"r");
	if ( fp==NULL )
		throw "file open failed";
	output_debug("opened link '%s'", filename);

	char line[1024];
	int linenum=0;
	while ( fgets(line,sizeof(line),fp)!=NULL )
	{
		linenum++;
		if ( line[0]=='#' ) continue;
		char tag[64], data[1024];
		if ( sscanf(line,"%s %[^\n]",tag,data)==2 )
		{
			output_debug("%s(%d): %s %s", filename, linenum, tag,data);
			if ( settag!=NULL )
			{
				if ( strcmp(tag,"global")==0 )
				{
					GLOBALVAR *var = global_find(data);
					if ( var )
						add_global(var);
					else
						output_error("%s(%d): global variable '%s' not found", filename, linenum, data);
				}
				else if ( strcmp(tag,"objects")==0 )
				{
					char *find = (char*)malloc(strlen(data)+1);
					strcpy(find,data);
					add_object(find);
				}
				else if ( strcmp(tag,"command")==0 )
				{
					strcpy(command,data);
				}
				else if ( strcmp(tag,"function")==0 )
				{
					strcpy(function,data);
				}
				else if ( strcmp(tag,"argument")==0 )
				{
					// TODO link arg data
					char *arg = (char*)malloc(strlen(data)+1);
					strcpy(arg,data);
					add_argument(arg);
				}
				else if ( !(*settag)(tag,data) )
					output_error("%s(%d): tag '%s' not accepted", filename, linenum, tag);
			}
			else if ( strcmp(tag,"target")==0)
			{
				if ( !set_target(data) )
					output_error("%s(%d): target '%s' is not valid", filename, linenum, data);
			}
			else
				output_warning("%s(%d): tag '%s' cannot be processed until target module is loaded", filename, linenum, tag);
		}
	}

	fclose(fp);

	// append to link list
	next = first;
	first = this;

	output_verbose("link '%s' ok", filename);
}

bool link::set_target(char *data)
{
	char libname[1024];
	char path[1024];
	sprintf(libname,"%sglx%s.%s",PREFIX,data,DLEXT);
	if ( find_file(libname,NULL,2,path,sizeof(path))!=NULL )
	{
		settag = &matlab_tag;
		return true;
	}
	else
	{
		output_error("target library '%s' not found", libname);
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// MATLAB LINKAGE
///////////////////////////////////////////////////////////////////////////////

#include <engine.h>

bool matlab_tag(char *tag, char *data)
{
	output_error("tag '%s' not valid for matlab target", tag);
	return false;
}

bool matlab_init()
{
	char data[] = "TODO";
	char objname[256], propertyname[64];
	if ( sscanf(data,"%[^.].%s",objname,propertyname)!=2 )
	{
		output_error("argument '%s' is not a valid object.property specification", data);
		return false;
	}

	OBJECT *obj = object_find_name(objname);
	if ( obj==NULL )
	{
		output_error("object '%s' not found", objname);
		return false;
	}
	
	PROPERTY *prop = class_find_property(obj->oclass,propertyname);
	if ( prop==NULL )
	{
		output_error("property '%s' not found in object '%s'", propertyname, objname);
		return false;
	}
	return true;
}
// TODO: add other tag handler implementations here


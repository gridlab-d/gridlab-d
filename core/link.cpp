/* $Id$
 */

#include <stdio.h>
#include "gridlabd.h"
#include "output.h"
#include "find.h"
#include "link.h"

typedef struct s_linklist {
	void *ptr;
	struct s_linklist *next;
} LINKLIST;

// returns the next head of the list to which the item was added
LINKLIST *list_add(LINKLIST*lst,void*val)
{
	LINKLIST *item = new LINKLIST;
	item->next = lst;
	item->ptr = val;
	return item;
}

typedef enum { 
	LT_NONE=0, 
	LT_MATLAB, 
	// TODO: add other targets here
} LINKTARGET;

class link {
private:
	LINKLIST *globals;
	LINKLIST *objects;
	char command[1024];
	union {
		struct {
			char function[1024];
			LINKLIST *arguments;
		} matlab;
	} specs;
private:
	LINKTARGET target; ///< specify which target app
	char cmd[1024]; ///< command to start target app
	class link *next; ///< pointer to next link target
public:
	link(char *file);
	~link(void);
private:
	bool set_target(char *data);

	// MATLAB
	bool matlab_tag(char *tag, char *data);
	bool matlab_init(void);
	TIMESTAMP matlab_sync(TIMESTAMP t0);
	
	// TODO: add other target handlers here
};

static link *linklist = NULL;

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

int link_init(void)
{
	return 1;
}

TIMESTAMP link_sync(TIMESTAMP t0)
{
	return TS_NEVER;
}

link::link(char *filename)
: target(LT_NONE)
{
	globals = NULL;
	objects = NULL;

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
			if ( strcmp(tag,"global")==0 )
			{
				GLOBALVAR *var = global_find(data);
				if ( var )
					globals = list_add(globals,var);
				else
					output_error("%s(%d): global variable '%s' not found", filename, linenum, data);
			}
			else if ( strcmp(tag,"objects")==0 )
			{
				char *find = (char*)malloc(strlen(data)+1);
				strcpy(find,data);
				objects = list_add(objects,data);
			}
			else if ( target==LT_MATLAB )
			{
				if ( !matlab_tag(tag,data) )
					output_error("%s(%d): tag '%s' not accepted", filename, linenum, tag);
			}
			// TODO: add other targets here
			else if ( strcmp(tag,"target")==0)
			{
				if ( !set_target(data) )
					output_error("%s(%d): target '%s' is not valid", filename, linenum, data);
			}
			else
				output_warning("%s(%d): tag '%s' cannot be processed until tag 'target' is given", filename, linenum, tag);
		}
	}

	fclose(fp);

	// append to link list
	this->next = linklist;
	linklist = this;

	output_verbose("link '%s' ok", filename);
}

bool link::set_target(char *data)
{
	if ( strcmp(data,"matlab")==0 )
	{
		target=LT_MATLAB;
		return 1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// MATLAB LINKAGE
///////////////////////////////////////////////////////////////////////////////

bool link::matlab_tag(char *tag, char *data)
{
	if ( strcmp(tag,"command")==0 )
	{
		strcpy(command,data);
		return true;
	}
	if ( strcmp(tag,"function")==0 )
	{
		strcpy(specs.matlab.function,data);
		return true;
	}
	else if ( strcmp(tag,"argument")==0 )
	{
		// TODO link arg data
		char *arg = (char*)malloc(strlen(data)+1);
		strcpy(arg,data);
		specs.matlab.arguments = list_add(specs.matlab.arguments,arg);
		return true;
	}
	else
	{
		output_error("tag '%s' not valid for matlab target", tag);
		return false;
	}
}

bool link::matlab_init()
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


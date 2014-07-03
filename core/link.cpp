/* $Id: link.cpp 4738 2014-07-03 00:55:39Z dchassin $
 */

#include "gridlabd.h"
#include <stdio.h>
#include "platform.h"
#include "output.h"
#include "find.h"
#include "timestamp.h"

#include "link.h"

#if defined WIN32 && ! defined MINGW
	#define _WIN32_WINNT 0x0400
	#undef int64 // wtypes.h also used int64
	#include <windows.h>
	#define int64 _int64
	#define PREFIX ""
	#ifndef DLEXT
		#define DLEXT ".dll"
	#endif
	#define DLLOAD(P) LoadLibrary(P)
	#define DLSYM(H,S) GetProcAddress((HINSTANCE)H,S)
	#define snprintf _snprintf
#else /* ANSI */
#ifndef MINGW
	#include "dlfcn.h"
#endif
	#define PREFIX ""
	#ifndef DLEXT
		#define DLEXT ".so"
	#endif
#ifndef MINGW
	#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#else
	#define DLLOAD(P) dlopen(P)
#endif
	#define DLSYM(H,S) dlsym(H,S)
#endif

glxlink *glxlink::first = NULL;

LINKLIST * glxlink::add_global(char *name)
{
	LINKLIST *item = new LINKLIST;
	if ( item==NULL ) return NULL;
	item->next = globals;
	item->data = NULL;
	item->name = (char*)malloc(strlen(name)+1);
	item->addr = NULL;
	item->size = 0;
	if ( item->name==NULL ) return NULL;
	strcpy(item->name,name);
	globals = item;
	return item;
}

LINKLIST * glxlink::add_object(char *name)
{
	LINKLIST *item = new LINKLIST;
	item->next = objects;
	item->data = NULL;
	item->name = (char*)malloc(strlen(name)+1);
	item->addr = NULL;
	item->size = 0;
	strcpy(item->name,name);
	objects = item;
	return item;
}

LINKLIST * glxlink::add_export(char *name)
{
	LINKLIST *item = new LINKLIST;
	item->next = exports;
	item->data = NULL;
	item->name = (char*)malloc(strlen(name)+1);
	item->addr = NULL;
	item->size = 0;
	strcpy(item->name,name);
	exports = item;
	return item;
}

LINKLIST * glxlink::add_import(char *name)
{
	LINKLIST *item = new LINKLIST;
	item->next = imports;
	item->data = NULL;
	item->name = (char*)malloc(strlen(name)+1);
	item->addr = NULL;
	item->size = 0;
	strcpy(item->name,name);
	imports = item;
	return item;
}

/* create a link module */
int link_create(char *file)
{
	try {
		glxlink *lt = new glxlink(file);
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
int link_initall(void)
{
	output_debug("link_initall(): link startup in progress...");
	glxlink *mod;
	for ( mod=glxlink::get_first() ; mod!=NULL ; mod=mod->get_next() )
	{
		LINKLIST *item;

		output_debug("link_initall(): setting up %s link", mod->get_target());

		// set default global list (if needed)
		if ( mod->get_globals()==NULL )
		{
			GLOBALVAR *var = NULL;
			while ( (var=global_getnext(var))!=NULL )
			{
				if ( var->prop!=NULL && var->prop->name!=NULL )
				{
					LINKLIST *item = mod->add_global(var->prop->name);
					if ( item!=NULL )
						item->data = (void*)var;
					else
						output_error("link_initall(): unable to link %s", var->prop->name);
				}
				else
					output_warning("link_initall(): a variable property definition is null"); 
			}
		}
		else 
		{
			// link global variables
			for ( item=mod->get_globals() ; item!=NULL ; item=mod->get_next(item) )
			{
				if ( strcmp(item->name,"")==0 ) continue;
				item->data = (void*)global_find(item->name);
				if ( item->data==NULL )
					output_error("link_initall(target='%s'): global '%s' is not found", mod->get_target(), item->name);
			}
		}

		// link objects
		if ( mod->get_objects()==NULL )
		{
			// set default object list
			OBJECT *obj = NULL;
			for ( obj=object_get_first() ; obj!=NULL ; obj=object_get_next(obj) )
			{
				// add named objects
				LINKLIST *item = NULL;
				if ( obj->name!=NULL )
					item = mod->add_object(obj->name);
				else
				{
					char id[256];
					sprintf(id,"%s:%d",obj->oclass->name,obj->id);
					item = mod->add_object(id);
				}
				item->data = (void*)obj;
			}
		}
		else
		{
			LINKLIST *item;

			// link global variables
			for ( item=mod->get_objects() ; item!=NULL ; item=mod->get_next(item) )
			{
				if ( strcmp(item->name,"")==0 ) continue;
				OBJECT *obj = NULL;
				item->data = (void*)object_find_name(item->name);
				if ( item->data==NULL)
					output_error("link_initall(target='%s'): object '%s' is not found", mod->get_target(), item->name);
			}
		}

		// link exports
		for ( item=mod->get_exports() ; item!=NULL ; item=mod->get_next(item) )
		{
			char objname[64], propname[64], varname[64];
			if ( sscanf(item->name,"%[^.].%s %s",objname,propname,varname)==3 )
			{
				OBJECTPROPERTY *objprop = (OBJECTPROPERTY*)malloc(sizeof(OBJECTPROPERTY));
				objprop->obj = object_find_name(objname);
				if ( objprop->obj )
				{
					objprop->prop = class_find_property(objprop->obj->oclass,propname);
					if ( objprop->prop==NULL )
						output_error("link_initall(target='%s'): export '%s' property not found", mod->get_target(), item->name);
					else
					{
						item->data = objprop;
						strcpy(item->name,varname);
					}
				}
				else
					output_error("link_initall(target='%s'): export '%s' object not found", mod->get_target(), item->name);
			}
			else
				output_error("link_initall(target='%s'): '%s' is not a valid export specification", mod->get_target(), item->name);
		}

		// link imports
		for ( item=mod->get_imports() ; item!=NULL ; item=mod->get_next(item) )
		{
			char objname[64], propname[64], varname[64];
			if ( sscanf(item->name,"%[^.].%s %s",objname,propname,varname)==3 )
			{
				OBJECTPROPERTY *objprop = (OBJECTPROPERTY*)malloc(sizeof(OBJECTPROPERTY));
				objprop->obj = object_find_name(objname);
				if ( objprop->obj )
				{
					objprop->prop = class_find_property(objprop->obj->oclass,propname);
					if ( objprop->prop==NULL )
						output_error("link_initall(target='%s'): import '%s' property not found", mod->get_target(), item->name);
					else
					{
						item->data = objprop;
						strcpy(item->name,varname);
					}
				}
				else
					output_error("link_initall(target='%s'): import '%s' object not found", mod->get_target(), item->name);
			}
			else
				output_error("link_initall(target='%s'): '%s' is not a valid import specification", mod->get_target(), item->name);
		}

		// initialize link module
		if ( !mod->do_init() )
		{
			output_error("link_initall(): link startup failed");
			link_termall();
			return 0;
		}
	}
	output_debug("link_initall(): link startup done ok");
	atexit((void(*)(void))link_termall);
	return 1;
}

TIMESTAMP link_syncall(TIMESTAMP t0)
{
	TIMESTAMP t1 = TS_NEVER;
	glxlink *mod;
	for ( mod=glxlink::get_first() ; mod!=NULL ; mod=mod->get_next() )
	{
		TIMESTAMP t2 = mod->do_sync(t0);
		if ( absolute_timestamp(t2)<absolute_timestamp(t1) ) t1 = t2;
	}
	return t1;
}

int link_termall(void)
{
	bool ok = true;
	glxlink *mod;
	for ( mod=glxlink::get_first() ; mod!=NULL ; mod=mod->get_next() )
	{
		output_debug("link_initall(): terminating %s link...",mod->get_target());
		if ( !mod->do_term() ) ok = false;
	}
	return ok;
}


glxlink::glxlink(char *filename)
{
	bool ok = true;
	globals = NULL;
	imports = NULL;
	exports = NULL;
	objects = NULL;
	handle = NULL;
	settag = NULL;
	init = NULL;
	sync = NULL;
	term = NULL;

	FILE *fp = fopen(filename,"rt");
	if ( fp==NULL )
		throw "file open failed";
	output_debug("opened link '%s'", filename);

	char line[1024];
	int linenum=0;
	while ( fgets(line,sizeof(line),fp)!=NULL )
	{
		linenum++;
		if ( line[0]=='#' ) continue;
		char tag[64], data[1024]="";
		if ( sscanf(line,"%s %[^\r\n]",tag,data)>0 )
		{
			output_debug("%s(%d): %s %s", filename, linenum, tag,data);
			if ( settag!=NULL )
			{
				if ( strcmp(tag,"global")==0 )
				{
					add_global(data);
				}
				else if ( strcmp(tag,"object")==0 )
				{
					add_object(data);
				}
				else if ( strcmp(tag,"export")==0 )
				{
					add_export(data);
				}
				else if ( strcmp(tag,"import")==0 )
				{
					add_import(data);
				}
				else if ( !(*settag)(this,tag,data) )
					output_error("%s(%d): tag '%s' not accepted", filename, linenum, tag);
			}
			else if ( strcmp(tag,"target")==0)
			{
				if ( !set_target(data) )
				{
					output_error("%s(%d): target '%s' is not valid", filename, linenum, data);
					ok = false;
				}
			}
			else
				output_warning("%s(%d): tag '%s' cannot be processed until target module is loaded", filename, linenum, tag);
		}
	}

	fclose(fp);

	// append to link list
	next = first;
	first = this;

	if ( ok )
		output_verbose("link '%s' ok", filename);
	else
		throw "cannot establish link";
}

bool glxlink::set_target(char *name)
{
	char libname[1024];
	char path[1024];
	sprintf(libname,PREFIX "glx%s" DLEXT,name);
	if ( find_file(libname,NULL,X_OK,path,sizeof(path))!=NULL )
	{
		// load library
		handle = DLLOAD(path);
		if ( handle==NULL )
		{
			output_error("unable to load '%s' for target '%s'", path,name);
			return false;
		}

		// attach functions
		settag = (bool(*)(glxlink*,char*,char*))DLSYM(handle,"glx_settag");
		init = (bool(*)(glxlink*))DLSYM(handle,"glx_init");
		sync = (TIMESTAMP(*)(glxlink*,TIMESTAMP))DLSYM(handle,"glx_sync");
		term = (bool(*)(glxlink*))DLSYM(handle,"glx_term");

		// call create routine
		bool (*create)(glxlink*,CALLBACKS*) = (bool(*)(glxlink*,CALLBACKS*))DLSYM(handle,"glx_create");
		if ( create!=NULL && create(this,module_callbacks()) )
		{
			strcpy(target,name);
			return true;
		}
		else
		{
			output_error("library '%s' for target '%s' does not define/export glx_create properly", path,name);
			return false;
		}
	}
	else
	{
		output_error("library '%s' for target '%s' not found", libname, name);
		return false;
	}
}


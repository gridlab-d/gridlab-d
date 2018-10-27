/** $Id: json.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file json.c
	@addtogroup mapping JSON formatting
	@ingroup core

	JSON files may be extracted using using the
	\p --output file.json command line option.  The function
	json_dump() is called and when a JSON dump
	is performed, the simulation is not run.

 @{
 **/

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "class.h"
#include "object.h"
#include "output.h"
#include "exception.h"
#include "module.h"
#include "timestamp.h"
#include "json.h"

FILE *json = NULL;
static int json_write(const char *fmt,...)
{
	int len;
	va_list ptr;
	va_start(ptr,fmt);
	len = vfprintf(json,fmt,ptr);
	va_end(ptr);
	return len;
}

#define FIRST(NAME,FORMAT,VALUE) json_write("\n\t\t\t\"%s\" : \""FORMAT"\"",NAME,VALUE);
#define TUPLE(NAME,FORMAT,VALUE) json_write(",\n\t\t\t\"%s\" : \""FORMAT"\"",NAME,VALUE);

static int json_modules(FILE *fp)
{
	MODULE *mod;
	json_write(",\n\t\"modules\" : {");
	for ( mod = module_get_first() ; mod != NULL ; mod = mod->next )
	{
		if ( mod != module_get_first() )
			json_write(",");
		json_write("\n\t\t\"%s\" : {",mod->name);
		FIRST("major","%d",mod->major);
		TUPLE("minor","%d",mod->minor);
		// TODO more info
		json_write("\n\t\t}");
	}
	json_write("\n\t}");
}

static int json_classes(FILE *fp)
{
	CLASS *oclass;
	json_write(",\n\t\"classes\" : {");
	for ( oclass = class_get_first_class() ; oclass != NULL ; oclass = oclass->next )
	{
		PROPERTY *prop;
		if ( oclass != class_get_first_class() )
			json_write(",");
		json_write("\n\t\t\"%s\" : {",oclass->name);
		FIRST("size","%u",oclass->size);
		if ( oclass->parent && oclass->parent->name )
		{
			TUPLE("parent","%s",oclass->parent->name );
		}
		TUPLE("trl","%u",oclass->trl);
		TUPLE("profiler.numobjs","%u",oclass->profiler.numobjs);
		TUPLE("profiler.clocks","%llu",oclass->profiler.clocks);
		TUPLE("profiler.count","%u",oclass->profiler.count);
		if ( oclass->has_runtime ) TUPLE("runtime","%s",oclass->runtime);
		if ( oclass->pmap != NULL )
			json_write(",");
		for ( prop = oclass->pmap ; prop != NULL ; prop=(prop->next?prop->next:(prop->oclass->parent?prop->oclass->parent->pmap:NULL)) )
		{
			KEYWORD *key;
			char *ptype = class_get_property_typename(prop->ptype);
			if ( ptype == NULL )
				continue;
			if ( prop != oclass->pmap )
				json_write(",");
			json_write("\n\t\t\t\"%s\" : {",prop->name);
			json_write("\n\t\t\t\t\"type\" : \"%s\"",ptype);
			for ( key = prop->keywords ; key != NULL ; key = key->next )
			{
				if ( key == prop->keywords )
				{
					json_write(",\n\t\t\t\t\"keywords\" : {");
				}
				json_write("\n\t\t\t\t\t\"%s\" : \"0x%x\"",key->name,key->value);
				if ( key->next == NULL )
					json_write("\n\t\t\t\t}");
				else
					json_write(",");
			}
			json_write("\n\t\t\t}");
		}
		json_write("\n\t\t}");
	}
	json_write("\n\t}");
}

static int json_globals(FILE *fp)
{
	GLOBALVAR *var;
	json_write(",\n\t\"globals\" : {");

	/* for each module */
	for ( var = global_find(NULL) ; var != NULL ; var = global_getnext(var) )
	{
		char buffer[1024];
		if ( global_getvar(var->prop->name,buffer,sizeof(buffer)-1) ) // only write globals that can be extracted
		{
			KEYWORD *key;
			PROPERTYSPEC *pspec = property_getspec(var->prop->ptype);
			if ( var != global_find(NULL) )
				json_write(",");
			json_write("\n\t\t\"%s\" : {", var->prop->name);
			json_write("\n\t\t\t\"type\" : \"%s\",", pspec->name);
			for ( key = var->prop->keywords ; key != NULL ; key = key->next )
			{
				if ( key == var->prop->keywords )
				{
					json_write("\n\t\t\t\"keywords\" : {");
				}
				json_write("\n\t\t\t\t\"%s\" : \"0x%x\"",key->name,key->value);
				if ( key->next == NULL )
					json_write("\n\t\t\t}");
				json_write(",");
			}
			if ( buffer[0] == '\"' )
				json_write("\n\t\t\t\"value\" : %s",buffer);
			else
				json_write("\n\t\t\t\"value\" : \"%s\"", buffer);
			json_write("\n\t\t}");
		}
	}
	json_write("\n\t}");
}

static int json_objects(FILE *fp)
{
	OBJECT *obj;
	json_write(",\n\t\"objects\" : {");

	/* scan each object in the model */
	for ( obj = object_get_first() ; obj != NULL ; obj = obj->next )
	{
		PROPERTY *prop;
		if ( obj != object_get_first() )
			json_write(",");
		if ( obj->oclass == NULL ) // ignore objects with no defined class
			continue;
		if ( obj->name ) 
			json_write("\n\t\t\"%s\" : {",obj->name);
		else if ( obj->oclass->name == NULL )
			continue; // ignore anonymous classes	
		json_write("\n\t\t\"%s:%d\" : {", obj->oclass->name, obj->id);
		FIRST("class","%s",obj->oclass->name);
		TUPLE("id","%d",obj->id);
		if ( ! isnan(obj->latitude) ) TUPLE("latitude","%d",obj->latitude);
		if ( ! isnan(obj->longitude) ) TUPLE("longitude","%d",obj->longitude);
		if ( obj->groupid[0] != '\0' ) TUPLE("groupid","%s",obj->groupid);
		TUPLE("rank","%u",(unsigned int)obj->rank);
		TUPLE("clock","%llu",clock);
		if ( obj->valid_to > TS_ZERO && obj->valid_to < TS_NEVER ) TUPLE("valid_to","%llu",obj->valid_to);
		TUPLE("schedule_skew","%llu",obj->schedule_skew);
		if ( obj->in_svc > TS_ZERO && obj->in_svc < TS_NEVER ) TUPLE("in","%llu",obj->in_svc);
		if ( obj->out_svc > TS_ZERO && obj->out_svc < TS_NEVER ) TUPLE("out","%llu",obj->out_svc);
		TUPLE("rng_state","%u",obj->rng_state);
		TUPLE("heartbeat","%llu",obj->heartbeat);
		TUPLE("guid","0x%llx",obj->guid[0]);
		TUPLE("flags","0x%llx",obj->flags);
		for ( prop = obj->oclass->pmap ; prop != NULL && prop->oclass == obj->oclass ; prop = prop->next )
		{
			char buffer[1024];
			char *value = object_property_to_string(obj,prop->name, buffer, sizeof(buffer)-1);
			if ( value == NULL )
				continue; // ignore values that don't convert propertly
			int len = strlen(value);
			if ( value[0] == '{' && value[len-1] == '}')
				json_write(",\n\t\t\t\"%s\" : %s", prop->name, value);
			else
				TUPLE(prop->name,"%s",value);
		}
		json_write("\n\t\t}");
	}

	json_write("\n\t}");
	return 0;
}

int json_output(FILE *fp)
{
	json = fp;
	json_write("{\t\"application\": \"gridlabd\",\n");
	json_write("\t\"version\" : \"4.1.0\"");
	json_modules(fp);
	json_classes(fp);
	json_globals(fp);
	json_objects(fp);
	json_write("}\n");
	return 0;
}

int json_dump(char *filename)
{
	char *ext, *basename;
	size_t b;
	char fname[1024];
	FILE *fp;

	/* handle default filename */
	if (filename==NULL)
		filename="gridlabd.json";

	/* find basename */
	b = strcspn(filename,"/\\:");
	basename = filename + (b<strlen(filename)?b:0);

	/* find extension */
	ext = strrchr(filename,'.');

	/* if extension is valid */
	if (ext!=NULL && ext>basename)

		/* use filename verbatim */
		strcpy(fname,filename);

	else

		/* append default extension */
		strcat(strcpy(fname,filename),".json");

	/* open file */
	fp = fopen(fname,"w");
	if (fp==NULL)
		throw_exception("json_dump(char *filename='%s'): %s", filename, strerror(errno));
		/* TROUBLESHOOT
			The system was unable to output the json data to the specified file.  
			Follow the recommended solution based on the error message provided and try again.
		 */

	/* output data */
	json_output(fp);

	/* close file */
	fclose(fp);

	return 0;
}

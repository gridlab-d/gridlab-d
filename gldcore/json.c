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

#define TUPLE(NAME,FMT,VALUE) json_write("\t{\"%s\": \""FMT"\"},\n",NAME,VALUE)

static int json_modules(FILE *fp)
{
	json_write("{\"modules\":[\n");

	/* for each module */
	for (MODULE *mod=module_get_first(); mod!=NULL; mod=mod->next)
	{
		json_write("\t[\n");
		TUPLE("name","%s",mod->name);
		json_write("\t],\n");
	}

	json_write("]},\n");
}

static int json_object(FILE *fp)
{
	json_write("{\"objects\":[\n");

	/* scan each object in the model */
	for (OBJECT *obj=object_get_first(); obj!=NULL; obj=obj->next)
	{
		json_write("\t[\n");
		/* basic json output of published variables */
		PROPERTY *prop;
		if (obj->oclass) TUPLE("class","%s",obj->oclass->name);
		if (obj->name) TUPLE("name","%s",obj->name);
		TUPLE("id","%d",obj->id);
		if ( !isnan(obj->latitude) ) TUPLE("latitude","%d",obj->latitude);
		if ( !isnan(obj->longitude) ) TUPLE("longitude","%d",obj->longitude);
		for (prop=obj->oclass->pmap;prop!=NULL && prop->oclass==obj->oclass; prop=prop->next)
		{
			char buffer[1024];
			char *value = object_property_to_string(obj,prop->name, buffer, sizeof(buffer)-1);
			if (value!=NULL) TUPLE(prop->name,"%s",value);
		}
		json_write("\t],\n");
	}

	json_write("]},\n");
	return 0;
}

int json_output(FILE *fp)
{
	json = fp;
	json_write("{ \"gridlabd\": {\n");
	json_modules(fp);
	json_object(fp);
	json_write("}}\n");
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

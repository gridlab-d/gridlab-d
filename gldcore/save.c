/* save.c
 * Copyright (C) 2008 Battelle Memorial Institute
 * Top level save routine.  Dispatches saves to subcomponents.  Format of save must be compatible with load module.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "output.h"
#include "stream.h"
#include "save.h"
#include "exec.h"
#include "gui.h"

#define DEFAULT_FORMAT "gld"

static int savetxt(char *filename, FILE *fp);
static int savexml(char *filename, FILE *fp);
static int savexml_strict(char *filename, FILE *fp);

int saveall(char *filename)
{
	FILE *fp;
	char *ext = strrchr(filename,'.');
	struct {
		char *format;
		int (*save)(char*,FILE*);
	} map[] = {
		{"txt", savetxt},
		{"gld", savetxt},
		//{"xml", savexml_strict},
		{"xml", savexml},
	};
	int i;

	/* identify output format */
	if (ext==NULL)
	{	/* no extension given */
		if (filename[0]=='-') /* stdout */
			ext=filename+1; /* format is specified after - */
		else
			ext=DEFAULT_FORMAT;
	}
	else
		ext++;

	/* setup output stream */
	if (filename[0]=='-')
		fp = stdout;
	else if ((fp=fopen(filename,"wb"))==NULL){
		output_error("saveall: unable to open stream \'%s\' for writing", filename);
		return 0;
	}

	/* internal streaming used */
	if (global_streaming_io_enabled)
	{
		int res = stream(fp,SF_OUT)>0 ? SUCCESS : FAILED;
		if (res==FAILED)
			output_error("stream context is %s",stream_context());
		return res;
	}

	/* general purpose format used */
	for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
	{
		if (strcmp(ext,map[i].format)==0)
		{
			return (*(map[i].save))(filename,fp);
		}
	}

	output_error("saveall: extension '.%s' not a known format", ext);
	/*	TROUBLESHOOT
		Only the format extensions ".txt", ".gld", and ".xml" are recognized by
		GridLAB-D.  Please end the specified output field accordingly, or omit the
		extension entirely to force use of the default format.
	*/
	errno = EINVAL;
	return FAILED;
}

int savetxt(char *filename,FILE *fp)
{
	unsigned int count = 0;
	time_t now = time(NULL);
	char buffer[1024];

	count += fprintf(fp,"########################################################\n");
	count += fprintf(fp,"# BEGIN");
	count += fprintf(fp,"\n########################################################\n");
	count += fprintf(fp,"# filename... %s\n", filename);
	count += fprintf(fp,"# workdir.... %s\n", global_workdir);
	count += fprintf(fp,"# command.... %s\n", global_command_line);
	count += fprintf(fp,"# created.... %s", asctime(localtime(&now)));
	count += fprintf(fp,"# user....... %s\n", 
#ifdef WIN32
		getenv("USERNAME")
#else
		getenv("USER")
#endif
		);
	count += fprintf(fp,"# host....... %s\n", 
#ifdef WIN32
		getenv("COMPUTERNAME")
#else
		getenv("HOSTNAME")
#endif
		);
	count += fprintf(fp,"# classes.... %d\n", class_get_count());
	count += fprintf(fp,"# objects.... %d\n", object_get_count());

	/* save gui, if any */
	if (gui_get_root()!=NULL)
	{
		count += fprintf(fp,"\n########################################################\n");
		count += fprintf(fp,"\n# GUI\n");
		count += (int)gui_glm_write_all(fp);
		count += fprintf(fp,"\n");
	}

	/* save clock */
		count += fprintf(fp,"\n########################################################\n");
		count += fprintf(fp,"\n# CLOCK\n");
	count += fprintf(fp,"clock {\n");
	count += fprintf(fp,"\ttick 1e%+d;\n",TS_SCALE);
	count += fprintf(fp,"\ttimezone %s;\n", timestamp_current_timezone());
	count += fprintf(fp,"\ttimestamp %s;\n", convert_from_timestamp(global_clock,buffer,sizeof(buffer))>0?buffer:"(invalid)");
	if (getenv("TZ"))
		count += fprintf(fp,"\ttimezone %s;\n", getenv("TZ"));
	count += fprintf(fp,"}\n");

	/* save parts */
	module_saveall(fp);
	class_saveall(fp);
	object_saveall(fp);

	count += fprintf(fp,"\n########################################################\n");
	count += fprintf(fp,"# END");
	count += fprintf(fp,"\n########################################################\n");
	if (fp!=stdout)
		fclose(fp);
	return count;
}

int savexml_strict(char *filename,FILE *fp)
{
	unsigned int count = 0;
	char buffer[1024];
	GLOBALVAR *global=NULL;
	MODULE *module;
	GLOBALVAR *stylesheet = global_find("stylesheet");

	int old_suppress_deprecated = global_suppress_deprecated_messages;
	global_suppress_deprecated_messages = 1;

	count += fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
	if (stylesheet==NULL || stylesheet->prop->ptype!=PT_char1024) /* only char1024 is allowed */
		count += fprintf(fp,"<?xml-stylesheet href=\"%sgridlabd-%d_%d.xsl\" type=\"text/xsl\"?>\n",global_urlbase,global_version_major,global_version_minor);
	else 
		count += fprintf(fp,"<?xml-stylesheet href=\"%s.xsl\" type=\"text/xsl\"?>\n",stylesheet->prop->addr);
	count += fprintf(fp,"<gridlabd>\n");
	
		/* globals */
		while ((global=global_getnext(global))!=NULL)
		{
			/* ignore module globals */
			if (strchr(global->prop->name,':'))
				continue;
			count += fprintf(fp,"\t<%s>%s</%s>\n", global->prop->name, global_getvar(global->prop->name,buffer,sizeof(buffer))==NULL?"[error]":buffer,global->prop->name);
		}
		count += fprintf(fp,"\t<timezone>%s</timezone>\n", timestamp_current_timezone());

		/* rank index */
		fprintf(fp,"\t<sync-order>\n");
		{
#define PASSINIT(p) (p % 2 ? ranks[p]->first_used : ranks[p]->last_used)
#define PASSCMP(i, p) (p % 2 ? i <= ranks[p]->last_used : i >= ranks[p]->first_used)
#define PASSINC(p) (p % 2 ? 1 : -1)
			INDEX **ranks = exec_getranks();
			LISTITEM *item;
			int i;
			PASSCONFIG pass;
			for (pass=0; ranks!=NULL && ranks[pass]!=NULL; pass++)
			{
				char *passname[]={"pretopdown","bottomup","posttopdown"};
				int lastrank=-1;
				fprintf(fp,"\t\t<pass>\n\t\t\t<name>%s</name>\n",passname[pass]);
				for (i = PASSINIT(pass); PASSCMP(i, pass); i += PASSINC(pass))
				{
					if (ranks[pass]->ordinal[i]!=NULL)
					{
						for (item=ranks[pass]->ordinal[i]->first; item!=NULL; item=item->next)
						{
							OBJECT *obj = item->data;
							if (obj->rank!=lastrank)
							{
								if (lastrank>=0)
									fprintf(fp,"\t\t\t</rank>\n");
								fprintf(fp,"\t\t\t<rank>\n");
								fprintf(fp,"\t\t\t\t<ordinal>%d</ordinal>\n",obj->rank);
								lastrank = obj->rank;
							}
							fprintf(fp,"\t\t\t\t<object>\n");
							if (obj->name) 
								fprintf(fp,"\t\t\t\t\t<name>%s</name>\n",obj->name);
							else
								fprintf(fp,"\t\t\t\t\t<name>(%s:%d)</name>\n",obj->oclass->name, obj->id);
							fprintf(fp,"\t\t\t\t\t<id>%d</id>\n",obj->id);
							fprintf(fp,"\t\t\t\t</object>\n");
						}
					}
				}
				if (lastrank>=0)
					fprintf(fp,"\t\t\t</rank>\n");
				fprintf(fp,"\t\t</pass>\n");
			}
		}
		fprintf(fp,"\t</sync-order>\n");

		/* modules */
		for (module=module_get_first(); module!=NULL; module=module->next)
		{
			CLASS *oclass;
			count += fprintf(fp, "\t<%s>\n",module->name);
			count += fprintf(fp,"\t\t<version.major>%d</version.major>\n", module->major);
			count += fprintf(fp,"\t\t<version.minor>%d</version.minor>\n", module->minor);

			/* globals */
			while ((global=global_getnext(global))!=NULL)
			{
				/* ignore globals not belonging to this module */
				char modname[64], name[64];
				if (sscanf(global->prop->name,"%s:%s",modname,name)<2 || strcmp(modname,module->name)!=0)
					continue;
				count += fprintf(fp,"\t\t<%s>%s</%s>\n", name, global_getvar(global->prop->name,buffer,sizeof(buffer))==NULL?"[error]":buffer,name);
			}

			/* objects */
			for (oclass=module->oclass; oclass!=NULL && oclass->module==module; oclass=oclass->next)
			{
				OBJECT *obj;
				count += fprintf(fp,"\t\t<%s_list>\n", oclass->name);
				if (oclass->parent) count += fprintf(fp,"\t\t\t<inherits_from>%s</inherits_from>\n",oclass->parent->name);
				for (obj=object_get_first(); obj!=NULL; obj=obj->next)
				{
					if (obj->oclass==oclass)
					{
						CLASS *pclass;
						PROPERTY *prop;
						DATETIME dt;
						count += fprintf(fp,"\t\t\t<%s>\n",oclass->name);
						count += fprintf(fp,"\t\t\t\t<id>%d</id>\n",obj->id);
						count += fprintf(fp,"\t\t\t\t<rank>%d</rank>\n",obj->rank);
						if (isfinite(obj->latitude)) count += fprintf(fp,"\t\t\t\t<latitude>%.6f</latitude>\n",obj->latitude);
						if (isfinite(obj->longitude)) count += fprintf(fp,"\t\t\t\t<longitude>%.6f</longitude>\n",obj->longitude);
						strcpy(buffer,"NEVER");
						if (obj->in_svc==TS_NEVER || (obj->in_svc>0 && local_datetime(obj->in_svc,&dt) && strdatetime(&dt,buffer,sizeof(buffer))>0)) 
							count += fprintf(fp,"\t\t\t\t<in_svc>%s</in_svc>\n",buffer);
						strcpy(buffer,"NEVER");
						if (obj->out_svc==TS_NEVER || (obj->out_svc>0 && local_datetime(obj->out_svc,&dt) && strdatetime(&dt,buffer,sizeof(buffer))>0))
							count += fprintf(fp,"\t\t\t\t<out_svc>%s</out_svc>\n",buffer);
						if (obj->parent)
						{
							if (obj->parent->name)
								count+=fprintf(fp,"\t\t\t\t<parent>%s</parent>\n",obj->parent->name);
							else
								count+=fprintf(fp,"\t\t\t\t<parent>%s:%d</parent>\n",obj->parent->oclass->name, obj->parent->id);
						}
						strcpy(buffer,"NEVER");
						if (obj->clock==TS_NEVER || (obj->clock>0 && local_datetime(obj->clock,&dt) && strdatetime(&dt,buffer,sizeof(buffer))>0)) 
							count += fprintf(fp,"\t\t\t\t<clock>%s</clock>\n",buffer);
						if (obj->name!=NULL) 
							count += fprintf(fp,"\t\t\t\t<name>%s</name>\n",obj->name);
						else
							count += fprintf(fp,"\t\t\t\t<name>(%s:%d)</name>\n",obj->oclass->name,obj->id);
						for (pclass=oclass; pclass!=NULL; pclass=pclass->parent)
						{
							for (prop=pclass->pmap; prop!=NULL && prop->oclass==pclass->pmap->oclass; prop=prop->next)
							{
								if (prop->unit!=NULL && strcmp(prop->unit->name,"V")==0 && prop->ptype==PT_complex)
								{
									complex *pval = object_get_complex(obj,prop);
									if (pval)
										pval->f = A;
								}
								if (object_get_value_by_name(obj,prop->name,buffer,sizeof(buffer))>0 && strcmp(buffer,"")!=0)
									count += fprintf(fp,"\t\t\t\t<%s>%s</%s>\n",prop->name,buffer,prop->name);
							}
						}
						count += fprintf(fp,"\t\t\t</%s>\n",oclass->name);
					}
				}
				count += fprintf(fp,"\t\t</%s_list>\n", oclass->name);
			}

			count += fprintf(fp, "\t</%s>\n",module->name);
		}

	count += fprintf(fp,"</gridlabd>\n");

	if (fp!=stdout)
		fclose(fp);

	global_suppress_deprecated_messages = old_suppress_deprecated;

	return count;
}

/*
 *	savexml() results in an XML file that can be reflexively read by the XML loader.  Note that the output
 *	of savexml_strict results in output that cannot be parsed back in, but can be parsed by automatically
 *	generated XSD files.
 */
int savexml(char *filename,FILE *fp)
{
	unsigned int count = 0;
	time_t now = time(NULL);
	char buffer[1024];
	GLOBALVAR *gvptr = global_getnext(NULL);

	if (global_xmlstrict)
		return savexml_strict(filename, fp);

	count += fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
	count += fprintf(fp,"<load>\n");
	count += fprintf(fp, "\t<global>\n");
	count += fprintf(fp,"\t\t<class_count>%d</class_count>\n", class_get_count());
	count += fprintf(fp,"\t\t<object_count>%d</object_count>\n", object_get_count());
	/* add global variables */
	while(gvptr != NULL){
		char *testp = strchr(gvptr->prop->name, ':');
		if(testp == NULL){
			count += fprintf(fp, "\t\t<%s>%s</%s>\n", gvptr->prop->name, class_property_to_string(gvptr->prop,(void*)gvptr->prop->addr,buffer,1024)>0 ? buffer : "...", gvptr->prop->name);
		} // else we have a module::prop name
		gvptr = global_getnext(gvptr);
	}
	count += fprintf(fp, "\t</global>\n");

	/* save clock */
	count += fprintf(fp,"\t<clock>\n");
	count += fprintf(fp,"\t\t<tick>1e%+d</tick>\n",TS_SCALE);
	count += fprintf(fp,"\t\t<timestamp>%s</timestamp>\n", convert_from_timestamp(global_clock,buffer,sizeof(buffer))>0?buffer:"(invalid)");
	if (getenv("TZ"))
		count += fprintf(fp,"\t\t<timezone>%s</timezone>\n", getenv("TZ"));
	count += fprintf(fp,"\t</clock>\n");

	/* save parts */
	module_saveall_xml(fp);

	count += fprintf(fp,"</load>\n");
	if (fp!=stdout)
		fclose(fp);
	return count;
}

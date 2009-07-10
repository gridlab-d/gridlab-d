/** $Id: kml.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file kml.h
	@addtogroup mapping Mapping tools
	@ingroup core

	KML mapping files may be extracted using using the
	\p --kml=file command line option.  The function
	kml_dump() is called and when a KML dump
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

#include "kml.h"

int kml_document(FILE *fp)
{
	CLASS *oclass, *openclass=NULL;
	MODULE *mod;
	char buffer[1024];
	time_t now = time(NULL);
	fprintf(fp,"  <Document>\n");
	fprintf(fp,"    <name>%s</name>\n", global_modelname);
	fprintf(fp,"    <description>GridLAB-D results for %s</description>\n",
		convert_from_timestamp(global_clock,buffer,sizeof(buffer))?buffer:"unknown date/time");

	/* for each module */
	for (mod=module_get_first(); mod!=NULL; mod=mod->next)
	{
		if (mod->kmldump)
			mod->kmldump(fp,NULL); /* dump styles */
	}

	/* scan each class in the model */
	for (oclass=class_get_first_class(); oclass!=NULL; oclass=oclass->next)
	{
		OBJECT *obj;

		/* scan each object in the model */
		for (obj=object_get_first(); obj!=NULL; obj=obj->next)
		{
			int has_location = !(isnan(obj->latitude) || isnan(obj->longitude));
			MODULE *mod;

			/* class does not match current object */
			if (obj->oclass!=oclass)
				continue;

			/* first instance of this class needs folder */
			else if (openclass==NULL)
			{
				fprintf(fp,"  <Folder><name>Class %s</name>\n", oclass->name);
				fprintf(fp,"    <description>Module %s",oclass->module->name);
				if (oclass->module->minor!=0 || oclass->module->major!=0)
					fprintf(fp," (V%d.%02d)",oclass->module->major,oclass->module->minor);
				fprintf(fp,"</description>\n",oclass->module->name);
				openclass=oclass;
			}			

			/* module overrides KML output */
			mod = (MODULE*)(obj->oclass->module);
			if (mod->kmldump!=NULL)
				(*(mod->kmldump))(fp,obj);
			else if (has_location)
			{
				/* basic KML output of published variables */
				PROPERTY *prop;
				fprintf(fp,"    <Placemark>\n");
				if (obj->name)
					fprintf(fp,"      <name>%s</name>\n", obj->name);
				else
					fprintf(fp,"      <name>%s %d</name>\n", obj->oclass->name, obj->id);
				fprintf(fp,"      <description>\n");
				fprintf(fp,"        <![CDATA[\n");
				fprintf(fp,"          <TABLE><TR>\n");
				for (prop=oclass->pmap;prop!=NULL && prop->oclass==oclass; prop=prop->next)
				{
					char *value = object_property_to_string(obj,prop->name);
					if (value!=NULL)
						fprintf(fp,"<TR><TH ALIGN=LEFT>%s</TH><TD ALIGN=RIGHT>%s</TD></TR>",
							prop->name, value);
				}
				fprintf(fp,"          </TR></TABLE>\n");
				fprintf(fp,"        ]]>\n");
				fprintf(fp,"      </description>\n");
				fprintf(fp,"      <Point>\n");
				fprintf(fp,"        <coordinates>%f,%f</coordinates>\n",obj->longitude,obj->latitude);
				fprintf(fp,"      </Point>\n");
				fprintf(fp,"    </Placemark>\n");
			}
		}

		/* close folder if any */
		if (openclass!=NULL)
		{
			fprintf(fp,"  </Folder>\n");
			openclass=NULL;
		}
	}

	fprintf(fp,"  </Document>\n");
	return 0;
}

int kml_output(FILE *fp)
{
	fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(fp,"<kml xmlns=\"http://earth.google.com/kml/2.2\">\n");
	kml_document(fp);
	fprintf(fp,"</kml>\n");
	return 0;
}

int kml_dump(char *filename)
{
	char *ext, *basename;
	size_t b;
	char fname[1024];
	FILE *fp;

	/* handle default filename */
	if (filename==NULL)
		filename="gridlabd.kml";

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
		strcat(strcpy(fname,filename),".kml");

	/* open file */
	fp = fopen(fname,"w");
	if (fp==NULL)
		throw_exception("kml_dump(char *filename='%s'): %s", filename, strerror(errno));
		/* TROUBLESHOOT
			The system was unable to output the KML data to the specified file.  
			Follow the recommended solution based on the error message provided and try again.
		 */

	/* output data */
	kml_output(fp);

	/* close file */
	fclose(fp);

	return 0;
}

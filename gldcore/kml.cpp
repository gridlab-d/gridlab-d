/** $Id: kml.c 4738 2014-07-03 00:55:39Z dchassin $
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

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "class.h"
#include "object.h"
#include "output.h"
#include "exception.h"
#include "module.h"

#include "kml.h"

FILE *kml = nullptr;
int kml_write(const char *fmt,...)
{
	int len;
	va_list ptr;
	va_start(ptr,fmt);
	len = vfprintf(kml,fmt,ptr);
	va_end(ptr);
	return len;
}

int kml_document(FILE *fp)
{
	CLASS *oclass, *openclass=nullptr;
	MODULE *mod;
	char buffer[1024];
	time_t now = time(nullptr);
	kml_write("%s","  <Document>\n");
	kml_write("    <name>%s</name>\n", global_modelname);
	kml_write("    <description>GridLAB-D results for %s</description>\n",
		convert_from_timestamp(global_clock,buffer,sizeof(buffer))?buffer:"unknown date/time");

	/* for each module */
	for (mod=module_get_first(); mod!=nullptr; mod=mod->next)
	{
		if (mod->kmldump)
			mod->kmldump(kml_write,nullptr); /* dump styles */
	}

	/* scan each class in the model */
	for (oclass=class_get_first_class(); oclass!=nullptr; oclass=oclass->next)
	{
		OBJECT *obj;

		/* scan each object in the model */
		for (obj=object_get_first(); obj!=nullptr; obj=obj->next)
		{
			int has_location = !(isnan(obj->latitude) || isnan(obj->longitude));
			if ( !has_location )
				continue;
			MODULE *mod;

			/* class does not match current object */
			if (obj->oclass!=oclass)
				continue;

			/* first instance of this class needs folder */
			else if (openclass==nullptr)
			{
				kml_write("  <Folder><name>Class %s</name>\n", oclass->name);
				kml_write("    <description>Module %s",oclass->module ? oclass->module->name : "(runtime)");
				if ( oclass->module!=nullptr )
					kml_write(" (V%d.%02d)",oclass->module->major,oclass->module->minor);
				kml_write("</description>\n");
				openclass=oclass;
			}			

			/* module overrides KML output */
			mod = (MODULE*)(obj->oclass->module);
			if (mod!=nullptr && mod->kmldump!=nullptr)
				(*(mod->kmldump))(kml_write,obj);
			else
			{
				/* basic KML output of published variables */
				PROPERTY *prop;
				kml_write("    <Placemark>\n");
				if (obj->name)
					kml_write("      <name>%s</name>\n", obj->name);
				else
					kml_write("      <name>%s %d</name>\n", obj->oclass->name, obj->id);
				kml_write("      <description>\n");
				kml_write("        <![CDATA[\n");
				kml_write("          <TABLE><TR>\n");
				for (prop=oclass->pmap;prop!=nullptr && prop->oclass==oclass; prop=prop->next)
				{
					char *value = object_property_to_string(obj,prop->name, buffer, 1023);
					if (value!=nullptr)
						kml_write("<TR><TH ALIGN=LEFT>%s</TH><TD ALIGN=RIGHT>%s</TD></TR>",
							prop->name, value);
				}
				kml_write("          </TR></TABLE>\n");
				kml_write("        ]]>\n");
				kml_write("      </description>\n");
				kml_write("      <Point>\n");
				kml_write("        <coordinates>%f,%f</coordinates>\n",obj->longitude,obj->latitude);
				kml_write("      </Point>\n");
				kml_write("    </Placemark>\n");
			}
		}

		/* close folder if any */
		if (openclass!=nullptr)
		{
			kml_write("  </Folder>\n");
			openclass=nullptr;
		}
	}

	kml_write("  </Document>\n");
	return 0;
}

int kml_output(FILE *fp)
{
	kml = fp;
	kml_write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	kml_write("<kml xmlns=\"http://earth.google.com/kml/2.2\">\n");
	kml_document(fp);
	kml_write("</kml>\n");
	return 0;
}

int kml_dump(char *filename)
{
	char *ext, *basename;
	size_t b;
	char fname[1024];
	FILE *fp;

	/* handle default filename */
	if (filename==nullptr)
		filename=const_cast<char*>("gridlabd.kml");

	/* find basename */
	b = strcspn(filename,"/\\:");
	basename = filename + (b<strlen(filename)?b:0);

	/* find extension */
	ext = strrchr(filename,'.');

	/* if extension is valid */
	if (ext!=nullptr && ext>basename)

		/* use filename verbatim */
		strcpy(fname,filename);

	else

		/* append default extension */
		strcat(strcpy(fname,filename),".kml");

	/* open file */
	fp = fopen(fname,"w");
	if (fp==nullptr)
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

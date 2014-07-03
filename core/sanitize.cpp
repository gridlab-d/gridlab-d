// $Id: sanitize.cpp 4738 2014-07-03 00:55:39Z dchassin $
// Copyright (C) 2012 Battelle Memorial Institute
// Author: DP Chassin
//

#include "gridlabd.h"
#include "output.h"
#include "sanitize.h"
#include "globals.h"

typedef struct s_safename {
	char *name;
	char *old;
	struct s_safename *next;
} SAFENAME;
static SAFENAME *safename_list = NULL;
static char *sanitize_name(OBJECT *obj)
{
	SAFENAME *safe = (SAFENAME*)malloc(sizeof(SAFENAME));
	if ( !safe ) return NULL;
	safe->old = obj->name;
	char buffer[1024];
	sprintf(buffer,"%s%llX",global_sanitizeprefix.get_string(),(unsigned int64)safe);
	safe->name = object_set_name(obj,buffer);
	safe->next = safename_list;
	safename_list = safe;
	return safe->name;
}

/** sanitize

	Sanitizes a gridlabd model by clear names and position from object headers

    @returns 0 on success, -2 on error
 **/
extern "C" int sanitize(int argc, char *argv[])
{
	OBJECT *obj;
	FILE *fp;
	double delta_latitude, delta_longitude;

	// lat/lon change
	if ( strcmp(global_sanitizeoffset,"")==0 )
	{
		delta_latitude = random_uniform(NULL,-5,+5);
		delta_longitude = random_uniform(NULL,-180,+180);
	}
	else if ( global_sanitizeoffset=="destroy" )
		delta_latitude = delta_longitude = QNAN;
	else if ( sscanf(global_sanitizeoffset.get_string(),"%lf%*[,/]%lf",&delta_latitude,&delta_longitude)!=2 )
	{
		output_error("sanitize_offset lat/lon '%s' is not valid", global_sanitizeoffset.get_string());
		return -2;
	}

	// sanitize object names
	for ( obj=object_get_first() ; obj!=NULL ; obj=object_get_next(obj) )
	{
		if ( obj->name!=NULL && (global_sanitizeoptions&SO_NAMES)==SO_NAMES )
			sanitize_name(obj);
		if ( isfinite(obj->latitude) && (global_sanitizeoptions&SO_GEOCOORDS)==SO_GEOCOORDS )
		{
			obj->latitude += delta_latitude;
			if ( obj->latitude<-90 ) obj->latitude = -90;
			if ( obj->latitude>+90 ) obj->latitude = +90;
		}
		if ( isfinite(obj->longitude) && (global_sanitizeoptions&SO_GEOCOORDS)==SO_GEOCOORDS )
			obj->longitude = fmod(obj->longitude+delta_longitude,360);
	}

	// dump object name index
	if ( strcmp(global_sanitizeindex,".xml")==0 )
	{
		strcpy(global_sanitizeindex,global_modelname);
		char *ext = strrchr(global_sanitizeindex,'.');
		if ( ext && strcmp(ext,".glm")==0 ) strcpy(ext,"-index.xml");
		else strcat(global_sanitizeindex,"-index.xml");
	}
	else if ( strcmp(global_sanitizeindex,".txt")==0 )
	{
		strcpy(global_sanitizeindex,global_modelname);
		char *ext = strrchr(global_sanitizeindex,'.');
		if ( ext && strcmp(ext,".glm")==0 ) strcpy(ext,"-index.txt");
		else strcat(global_sanitizeindex,"-index.txt");
	}
	else if ( global_sanitizeindex[0]=='.' )
	{
		output_error("sanitization index file spec '%s' is not recognized", global_sanitizeindex.get_string());
		return -2;
	}
	if ( strcmp(global_sanitizeindex,"")!=0 )
	{
		char *ext = strrchr(global_sanitizeindex,'.');
		bool use_xml = (ext && strcmp(ext,".xml")==0) ;
		fp = fopen(global_sanitizeindex,"w");
		if ( fp )
		{
			SAFENAME *item;
			if ( use_xml )
			{
				fprintf(fp,"<data>\n");
				fprintf(fp,"\t<modelname>%s</modelname>\n",global_modelname);
				fprintf(fp,"\t<geographic_offsets>\n");
				fprintf(fp,"\t\t<latitude>%.6f</latitude>\n",delta_latitude);
				fprintf(fp,"\t\t<longitude>%.6f</longitude>\n",delta_longitude);
				fprintf(fp,"\t</geographic_offsets>\n");
				fprintf(fp,"\t<safename_list>\n");
				for ( item=safename_list ; item!=NULL ; item=item->next )
					fprintf(fp,"\t\t<name>\n\t\t\t<safe>%s</safe>\n\t\t\t<unsafe>%s</unsafe>\n\t\t</name>\n", item->name, item->old);
				fprintf(fp,"\t</safename_list>\n");
				fprintf(fp,"</data>\n");
			}
			else
			{
				fprintf(fp,"modelname\t= %s\n", global_modelname);
				fprintf(fp,"\n[POSITIONS]\n");
				fprintf(fp,"latitude\t= %.6f\n",delta_latitude);
				fprintf(fp,"longitude\t= %.6f\n",delta_longitude);
				fprintf(fp,"\n[NAMES]\n");
				for ( item=safename_list ; item!=NULL ; item=item->next )
					fprintf(fp,"%s\t= %s\n", item->name, item->old);
			}
			fclose(fp);
		}
	}

	return 0;
}

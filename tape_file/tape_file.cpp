/** $Id: tape_file.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file tape_file.cpp
	@addtogroup tape_file File-based tapes
	@ingroup tapes

	Tape Files read or write comma-separated values in a flat file on the local computer.  Output
	tape files will typically clobber existing files, but the user can change the default file mode
	to write+ or append in the tape definition in the input file.
	The "#" symbol at the beginning of a line denotes a comment.  These comments and blank lines are
	skipped in input files.  Leading whitespace is ignored.
@{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "gridlabd.h"
#include "../tape/tape.h"
#include "../tape/histogram.h"
#include "tape_file.h"

int csv_data_only = 0; /* enable this option to suppress addition of lines starting with # in CSV */
int csv_keep_clean = 0; /* enable this option to keep data flushed at end of line */
EXPORT void set_csv_data_only()
{
	csv_data_only = 1;
}
EXPORT void set_csv_keep_clean()
{
	csv_keep_clean = 1;
}

/*******************************************************************
 * players 
 */
EXPORT int open_player(struct player *my, char *fname, char *flags)
{
	//char *ff = gl_findfile(fname,NULL,FF_READ);
	char *ff = fname;

	/* "-" means stdin */
	my->fp = (strcmp(fname,"-")==0?stdin:(ff?fopen(ff,flags):NULL));
	if (my->fp==NULL)
	{
		sprintf(my->lasterr, "player file %s: %s", fname, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	else
	{
		my->loopnum = my->loop;
		my->status=TS_OPEN;
		my->type = FT_FILE;
		return 1;
	}
}

EXPORT char *read_player(struct player *my,char *buffer,unsigned int size)
{
	return fgets(buffer,size,my->fp);
}

EXPORT int rewind_player(struct player *my)
{
	return fseek(my->fp,SEEK_SET,0);
}

EXPORT void close_player(struct player *my)
{
	fclose(my->fp);
	my->status = TS_DONE;
}

/*******************************************************************
 * shape generators 
 */
#define MAPSIZE(N) ((N-1)/8+1)
#define SET(X,B) ((X)[(B)/8]|=(1<<((B)&7)))
#define ISSET(X,B) (((X)[(B)/8]&(1<<((B)&7)))==(1<<((B)&7)))
char *file=NULL;
int linenum=0;
static int setmap(char *spec, unsigned char *map, int size)
{
	char *p=spec;
	int last=-1;
	memset(map,0,MAPSIZE(size));
	while (*p!='\0')
	{
		if (*p=='*')
		{
			int i;
			for (i=0;i<size;i++) 
				SET(map,i); 
			p++;
		}
		else if (isdigit(*p))
		{
			int i=atoi(p);
			if (last<0)
			/* no spanning */
				SET(map,i);
			else if (i>last) /* span to i */
				do { 
					SET(map,last); 
				} while (++last<=i);
			else
			{
				/* span to i w/wrap around */
				do { 
					SET(map,last); 
				} while (++last<size);
				last=0;
				do { 
					SET(map,last); 
				} while (++last<=i);
			}
			last = i;
			while (isdigit(*p)) p++;
		}
		else if (*p=='-')
		{	/* spanning enabled */
			p++;
		}
		else if (*p==';')
		{	/* spanning disabled */
			last = -1;
			p++;
		}
		else
			return 0;
	}
	return 1;
}
static unsigned char *hourmap(char *spec)
{
	static unsigned char hours[MAPSIZE(24)];
	if (setmap(spec,hours,24))
		return hours;
	else
		return NULL;
}
static unsigned char *daymap(char *spec)
{
	static unsigned char days[MAPSIZE(31)];
	if (setmap(spec,days,31))
		return days;
	else
		return NULL;
}
static unsigned char *monthmap(char *spec)
{
	static unsigned char months[MAPSIZE(12)];
	if (setmap(spec,months,12))
		return months;
	else
		return NULL;
}
static unsigned char *weekdaymap(char *spec)
{
	static unsigned char weekdays[MAPSIZE(7)];
	if (setmap(spec,weekdays,7))
		return weekdays;
	else
		return NULL;
}

EXPORT int open_shaper(struct shaper *my, char *fname, char *flags)
{
	char line[1024], group[256]="(unnamed)";
	float sum=0, load=0, peak=0;
	float scale[12][31][7][24];
	//char *ff = gl_findfile(fname,NULL,FF_READ);
	char *ff = fname;

	/* clear everything */
	memset(scale,0,sizeof(scale));
	linenum=0; 
	file=fname;

	/* "-" means stdin */
	my->fp = (strcmp(fname,"-")==0?stdin:(ff?fopen(ff,flags):NULL));
	if (my->fp==NULL)
	{
		sprintf(my->lasterr, "shaper file %s: %s", fname, strerror(errno));
		goto Error;
	}
	my->status=TS_OPEN;
	my->type = FT_FILE;
	/* TODO: these should be read from the shape file, or better yet, inferred from it */
	my->step = 3600; /* default interval step is one hour */
	my->interval = 24; /* default unint shape integrated over one day */
	memset(my->shape,0,sizeof(my->shape));
	/* load the file into the shape */
	while (fgets(line,sizeof(line),my->fp)!=NULL)
	{
		unsigned char *hours, *days, *months, *weekdays;
		char min[256],hour[256],day[256],month[256],weekday[256],value[32];
		char *p=line;
		linenum++;
		while (isspace(*p)) p++;
		if (p[0]=='\0' || p[0]=='#') continue;
		if (strcmp(group,"")!=0 && (isdigit(p[0]) || p[0]=='*'))
		{	/* shape value */
			int h, d, m, w;
			if (sscanf(line,"%s %s %s %s %[^,],%[^,\n]",min,hour,day,month,weekday,value)<6)
			{
				sprintf(my->lasterr, "%s(%d) : shape '%s' missing specification '%s'", file, linenum, group, line);
				goto Error;
			}
			/* minutes are ignored right now */
			if (min[0]!='*') //gl_warning
			{
				sprintf(my->lasterr, "%s(%d) : minutes are ignored in '%s'", file, linenum, line);
				goto Error;
			}
			hours=hourmap(hour);
			if (hours==NULL)
			{
				sprintf(my->lasterr,"%s(%d): hours in '%s' not valid", file, linenum, line);
				goto Error;
			}
			days=daymap(day);
			if (days==NULL)
			{
				sprintf(my->lasterr,"%s(%d): days in '%s' not valid", file, linenum, line);
				goto Error;
			}
			months=monthmap(month);
			if (months==NULL)
			{
				sprintf(my->lasterr,"%s(%d): months in '%s' not valid", file, linenum, line);
				goto Error;
			}
			weekdays=weekdaymap(weekday);
			if (weekdays==NULL)
			{
				sprintf(my->lasterr,"%s(%d): weekdays in '%s' not valid", file, linenum, line);
				goto Error;
			}
			load = (float)atof(value);
			for (m=0; m<12; m++)
			{
				if (!ISSET(months,m)) continue;
				for (w=0; w<7; w++)
				{
					if (!ISSET(weekdays,w)) continue;
					for (d=0; d<31; d++)
					{
						if (!ISSET(days,d)) continue;
						for (h=0; h<24; h++)
						{
							if (!ISSET(hours,h)) continue;
							scale[m][d][w][h] = -load; /* negative indicates unscaled value */
						}
					}
				}
			}
			sum += load; /* integrate over shape */
			if (load>peak) peak=load; /* keep the highest load in the shape (that's going to be 255) */
		}
		else if (p[0]=='}')
		{	/* end shape group */
			int h, d, m, w;
			my->scale = peak/255/sum;
			/* rescale group */
			for (m=0; m<12; m++)
			{
				for (w=0; w<7; w++)
				{
					for (d=0; d<31; d++)
					{
						for (h=0; h<24; h++)
						{
							if (scale[m][d][w][h]<0)
								my->shape[m][d][w][h] = (unsigned char)(-scale[m][d][w][h] / peak * 255 +0.5); /* negative removes scaled value indicator */
						}
					}
				}
			}
			strcpy(group,"");
		}
		else if (sscanf(p,"%s {",group)==1)
		{	/* new shape group */
			sum=0;
		}
		else
		{	/* syntax error */
			//gl_error(
			fprintf(stderr, "%s(%d) : shape specification '%s' is not valid", file, linenum, line);
		}
	}
	fclose(my->fp);
	my->fp=NULL;
	my->status = TS_OPEN;
	return 1;
Error:
	if (my->fp)
	{
		fclose(my->fp);
		my->fp = NULL;
	}
	my->status = TS_ERROR;
	return 0;
}

EXPORT char *read_shaper(struct shaper *my,char *buffer,unsigned int size)
{
	return NULL;
}

EXPORT int rewind_shaper(struct shaper *my)
{
	return -1;
}

EXPORT void close_shaper(struct shaper *my)
{
}

/*******************************************************************
 * recorders 
 */
EXPORT int open_recorder(struct recorder *my, char *fname, char *flags)
{
	time_t now=time(NULL);
	OBJECT *obj=OBJECTHDR(my);
	
	my->fp = (strcmp(fname,"-")==0?stdout:fopen(fname,flags));
	if (my->fp==NULL)
	{
		//gl_error(
		fprintf(stderr, "recorder file %s: %s", fname, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	my->type = FT_FILE;
	my->last.ts = TS_ZERO;
	my->status=TS_OPEN;
	my->samples=0;

	if (!csv_data_only)
	{
		/* put useful header information in file first */
		fprintf(my->fp,"# file...... %s\n", my->file.get_string());
		fprintf(my->fp,"# date...... %s", asctime(localtime(&now)));
#ifdef WIN32
		fprintf(my->fp,"# user...... %s\n", getenv("USERNAME"));
		fprintf(my->fp,"# host...... %s\n", getenv("MACHINENAME"));
#else
		fprintf(my->fp,"# user...... %s\n", getenv("USER"));
		fprintf(my->fp,"# host...... %s\n", getenv("HOST"));
#endif
		if(obj->parent){
			fprintf(my->fp,"# target.... %s %d\n", obj->parent->oclass->name, obj->parent->id);
		}
		fprintf(my->fp,"# trigger... %s\n", my->trigger[0]=='\0'?"(none)":my->trigger.get_string());
		fprintf(my->fp,"# interval.. %d\n", my->interval);
		fprintf(my->fp,"# limit..... %d\n", my->limit);
		fprintf(my->fp,"# timestamp,%s\n", my->property.get_string());
	}

	return 1;
}

EXPORT int write_recorder(struct recorder *my, char *timestamp, char *value)
{ 
	int count = fprintf(my->fp,"%s,%s\n", timestamp, value);
	if (csv_keep_clean) fflush(my->fp);
	return count;
}

EXPORT void close_recorder(struct recorder *my)
{
	if (my->fp)
	{
		if (!csv_data_only) fprintf(my->fp,"# end of tape\n");
		fclose(my->fp);
		my->fp = NULL; // Defensive programming. For some reason GridlabD was 
		// closing the same pointer twice, causing it to crash.
	}
}

/*******************************************************************
 * histograms 
 */
EXPORT int open_histogram(histogram *my, char *fname, char *flags)
{
	time_t now=time(NULL);
	OBJECT *obj=OBJECTHDR(my);
	
	my->fp = (strcmp(fname,"-")==0?stdout:fopen(fname,"w"));
	if (my->fp==NULL)
	{
		//gl_error(
		fprintf(stderr, "histogram file %s: %s", fname, strerror(errno));
		return 0;
	}
	my->type = FT_FILE;
	my->t_count = TS_ZERO;
	my->t_sample = TS_ZERO;

	if (!csv_data_only)
	{
		/* put useful header information in file first */
		fprintf(my->fp,"# file...... %s\n", my->fname.get_string());
		fprintf(my->fp,"# date...... %s", asctime(localtime(&now)));
#ifdef WIN32
		fprintf(my->fp,"# user...... %s\n", getenv("USERNAME"));
		fprintf(my->fp,"# host...... %s\n", getenv("MACHINENAME"));
#else
		fprintf(my->fp,"# user...... %s\n", getenv("USER"));
		fprintf(my->fp,"# host...... %s\n", getenv("HOST"));
#endif
		if(obj->parent != NULL){
			fprintf(my->fp,"# target.... %s %d\n", obj->parent->oclass->name, obj->parent->id);
		} else {
			fprintf(my->fp,"# group.... %s\n", my->group.get_string());
		}
	//	fprintf(my->fp,"# trigger... %s\n", my->trigger[0]=='\0'?"(none)":my->trigger);
		fprintf(my->fp,"# counting interval.. %f\n", my->counting_interval);
		fprintf(my->fp,"# sampling interval.. %f\n", my->sampling_interval);
		fprintf(my->fp,"# limit..... %d\n", my->limit);
		if(my->bins[0] != 0){
			fprintf(my->fp,"# timestamp,%s\n", my->bins.get_string());
		} else {
			int i = 0;
			for(i = 0; i < my->bin_count; ++i){
				fprintf(my->fp,"%s%f..%f%s",(my->bin_list[i].low_inc ? "[" : "("), my->bin_list[i].low_val, my->bin_list[i].high_val, (my->bin_list[i].high_inc ? "]" : ")"));
				if(i+1 < my->bin_count){
					fprintf(my->fp,",");
				}
			}
			fprintf(my->fp, "\n");
		}
	}	

	return 1;
}

EXPORT int write_histogram(histogram *my, char *timestamp, char *value)
{ 
	int count = fprintf(my->fp,"%s,%s\n", timestamp, value);
	if (csv_keep_clean) fflush(my->fp);
	return count;
}

EXPORT void close_histogram(histogram *my)
{
	if (my->fp)
	{
		fprintf(my->fp,"# end of tape\n");
		fclose(my->fp);
		my->fp = NULL;
		/* Defensive programming. For some reason GridlabD was 
		 * closing the same pointer twice, causing it to crash.
		 */
	}
}

/*******************************************************************
 * collectors 
 */
EXPORT int open_collector(struct collector *my, char *fname, char *flags)
{
	unsigned int count=0;
	time_t now=time(NULL);

	my->fp = (strcmp(fname,"-")==0?stdout:fopen(fname,flags));
	if (my->fp==NULL)
	{
		//gl_error(
		fprintf(stderr, "collector file %s: %s", fname, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	my->last.ts = TS_ZERO;
	my->status=TS_OPEN;
	my->type = FT_FILE;
	my->samples=0;

	if (!csv_data_only)
	{
		/* put useful header information in file first */
		count += fprintf(my->fp,"# file...... %s\n", my->file.get_string());
		count += fprintf(my->fp,"# date...... %s", asctime(localtime(&now)));
#ifdef WIN32
		count += fprintf(my->fp,"# user...... %s\n", getenv("USERNAME"));
		count += fprintf(my->fp,"# host...... %s\n", getenv("MACHINENAME"));
#else
		count += fprintf(my->fp,"# user...... %s\n", getenv("USER"));
		count += fprintf(my->fp,"# host...... %s\n", getenv("HOST"));
#endif
		count += fprintf(my->fp,"# group..... %s\n", my->group.get_string());
		count += fprintf(my->fp,"# trigger... %s\n", my->trigger[0]=='\0'?"(none)":my->trigger.get_string());
		count += fprintf(my->fp,"# interval.. %d\n", my->interval);
		count += fprintf(my->fp,"# limit..... %d\n", my->limit);
		count += fprintf(my->fp,"# property.. timestamp,%s\n", my->property.get_string());
	}

	return 1;
}

EXPORT int write_collector(struct collector *my, char *timestamp, char *value)
{
	int count = fprintf(my->fp,"%s,%s\n", timestamp, value);
	if (csv_keep_clean) fflush(my->fp);
	return count;
}

EXPORT void close_collector(struct collector *my)
{
	if (my->fp)
	{
		if (!csv_data_only) fprintf(my->fp,"# end of tape\n");
		fclose(my->fp);
	}
	my->fp = 0;
}

/**@}*/

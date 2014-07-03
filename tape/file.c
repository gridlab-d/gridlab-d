/* $Id: file.c 4738 2014-07-03 00:55:39Z dchassin $
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "gridlabd.h"
#include "tape.h"
#include "file.h"

/*******************************************************************
 * players 
 */
int file_open_player(struct player *my, char *fname, char *flags)
{
	char ff[1024];

	/* "-" means stdin */
	my->fp = (strcmp(fname,"-")==0?stdin:(gl_findfile(fname,NULL,R_OK,ff,sizeof(ff))?fopen(ff,flags):NULL));
	if (my->fp==NULL)
	{
		gl_error("player file %s: %s", fname, strerror(errno));
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

char *file_read_player(struct player *my,char *buffer,unsigned int size)
{
	return fgets(buffer,size,my->fp);
}

int file_rewind_player(struct player *my)
{
	return fseek(my->fp,SEEK_SET,0);
}

void file_close_player(struct player *my)
{
}

/*******************************************************************
 * shape generators 
 */
#define MAPSIZE(N) ((N-1)/8+1)
#define SET(X,B) ((X)[(B)/8]|=(1<<((B)&7)))
#define ISSET(X,B) (((X)[(B)/8]&(1<<((B)&7)))==(1<<((B)&7)))
char *file=NULL;
int linenum=0;
static void setmap(char *spec, unsigned char *map, int size)
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
	}
}
static unsigned char *hourmap(char *spec)
{
	static unsigned char hours[MAPSIZE(24)];
	setmap(spec,hours,24);
	return hours;
}
static unsigned char *daymap(char *spec)
{
	static unsigned char days[MAPSIZE(31)];
	setmap(spec,days,31);
	return days;
}
static unsigned char *monthmap(char *spec)
{
	static unsigned char months[MAPSIZE(12)];
	setmap(spec,months,12);
	return months;
}
static unsigned char *weekdaymap(char *spec)
{
	static unsigned char weekdays[MAPSIZE(7)];
	setmap(spec,weekdays,7);
	return weekdays;
}

int file_open_shaper(struct shaper *my, char *fname, char *flags)
{
	char line[1024], group[256]="(unnamed)";
	float sum=0, load=0, peak=0;
	float scale[12][31][7][24];
	char ff[1024];

	/* clear everything */
	memset(scale,0,sizeof(scale));
	linenum=0; 
	file=fname;

	/* "-" means stdin */
	my->fp = (strcmp(fname,"-")==0?stdin:(gl_findfile(fname,NULL,R_OK,ff,sizeof(ff))?fopen(ff,flags):NULL));
	if (my->fp==NULL)
	{
		gl_error("shaper file %s: %s", fname, strerror(errno));
		my->status = TS_DONE;
		return 0;
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
				gl_error("%s(%d) : shape '%s' has specification '%s'", file, linenum, group, line);
				continue;
			}
			/* minutes are ignored right now */
			if (min[0]!='*') gl_warning("%s(%d) : minutes are ignored in '%s'", file, linenum, line);
			hours=hourmap(hour);
			days=daymap(day);
			months=monthmap(month);
			weekdays=weekdaymap(weekday);
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
			gl_error("%s(%d) : shape specification '%s' is not valid", file, linenum, line);
		}
	}
	return 1;
}

char *file_read_shaper(struct shaper *my,char *buffer,unsigned int size)
{
	return fgets(buffer,size,my->fp);
}

int file_rewind_shaper(struct shaper *my)
{
	return fseek(my->fp,SEEK_SET,0);
}

void file_close_shaper(struct shaper *my)
{
}

/*******************************************************************
 * recorders 
 */
int file_open_recorder(struct recorder *my, char *fname, char *flags)
{
	time_t now=time(NULL);
	OBJECT *obj=OBJECTHDR(my);
	
	my->fp = (strcmp(fname,"-")==0?stdout:fopen(fname,flags));
	if (my->fp==NULL)
	{
		gl_error("recorder file %s: %s", fname, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	my->type = FT_FILE;
	my->last.ts = TS_ZERO;
	my->status=TS_OPEN;
	my->samples=0;

	/* put useful header information in file first */
	fprintf(my->fp,"# file...... %s\n", my->file);
	fprintf(my->fp,"# date...... %s", asctime(localtime(&now)));
#ifdef WIN32
	fprintf(my->fp,"# user...... %s\n", getenv("USERNAME"));
	fprintf(my->fp,"# host...... %s\n", getenv("MACHINENAME"));
#else
	fprintf(my->fp,"# user...... %s\n", getenv("USER"));
	fprintf(my->fp,"# host...... %s\n", getenv("HOST"));
#endif
	fprintf(my->fp,"# target.... %s %d\n", obj->parent->oclass->name, obj->parent->id);
	fprintf(my->fp,"# trigger... %s\n", my->trigger[0]=='\0'?"(none)":my->trigger);
	fprintf(my->fp,"# interval.. %d\n", my->interval);
	fprintf(my->fp,"# limit..... %d\n", my->limit);
	fprintf(my->fp,"# timestamp,%s\n", my->property);

	return 1;
}

int file_write_recorder(struct recorder *my, char *timestamp, char *value)
{ 
	return fprintf(my->fp,"%s,%s\n", timestamp, value);
}

void file_close_recorder(struct recorder *my)
{
	fprintf(my->fp,"# end of tape\n");
	fclose(my->fp);
}

/*******************************************************************
 * collectors 
 */
int file_open_collector(struct collector *my, char *fname, char *flags)
{
	unsigned int count=0;
	time_t now=time(NULL);

	my->fp = (strcmp(fname,"-")==0?stdout:fopen(fname,flags));
	if (my->fp==NULL)
	{
		gl_error("collector file %s: %s", fname, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	my->last.ts = TS_ZERO;
	my->status=TS_OPEN;
	my->type = FT_FILE;
	my->samples=0;

	/* put useful header information in file first */
	count += fprintf(my->fp,"# file...... %s\n", my->file);
	count += fprintf(my->fp,"# date...... %s", asctime(localtime(&now)));
#ifdef WIN32
	count += fprintf(my->fp,"# user...... %s\n", getenv("USERNAME"));
	count += fprintf(my->fp,"# host...... %s\n", getenv("MACHINENAME"));
#else
	count += fprintf(my->fp,"# user...... %s\n", getenv("USER"));
	count += fprintf(my->fp,"# host...... %s\n", getenv("HOST"));
#endif
	count += fprintf(my->fp,"# group..... %s\n", my->group);
	count += fprintf(my->fp,"# trigger... %s\n", my->trigger[0]=='\0'?"(none)":my->trigger);
	count += fprintf(my->fp,"# interval.. %d\n", my->interval);
	count += fprintf(my->fp,"# limit..... %d\n", my->limit);
	count += fprintf(my->fp,"# property.. timestamp,%s\n", my->property);

	return count;
}

int file_write_collector(struct collector *my, char *timestamp, char *value)
{
	return fprintf(my->fp,"%s,%s\n", timestamp, value);
}

void file_close_collector(struct collector *my)
{
	fprintf(my->fp,"# end of tape\n");
	fclose(my->fp);

}

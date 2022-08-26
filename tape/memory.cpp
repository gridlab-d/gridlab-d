/* $Id: memory.c 4738 2014-07-03 00:55:39Z dchassin $
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "gridlabd.h"
#include "tape.h"
#include "memory.h"

#include "collector.h"
#include "player.h"
#include "recorder.h"
#include "shaper.h"

/*******************************************************************
 * players 
 */
int memory_open_player(struct player *my, char *fname, char *flags)
{
	/* "-" means stdin */
	my->memory = (MEMORY*)malloc(sizeof(MEMORY));
	if (my->memory==NULL)
	{
		gl_error("player memory %s: %s", fname, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	my->memory->buffer = gl_global_find(fname);
	if (my->memory->buffer!=NULL)
	{
		my->memory->index = 0;
		my->loopnum = my->loop;
		my->status=TS_OPEN;
		my->type = FT_MEMORY;
		return 1;
	}
	gl_error("memory_open_player(struct player *my='{...}', char *fname='%s', char *flags='%s'): global '%s' not defined", fname,flags,fname);
	return 0;
}

char *memory_read_player(struct player *my,char *buffer,unsigned int size)
{
	static char temp[256];
	
	/* if no more data in current loop buffer */
	if (my->memory->index >= my->memory->buffer->prop->size-1) /* need two values */
		return NULL;
	else
	{
		double *ptr = (double*)my->memory->buffer->prop->addr + my->memory->index;
		my->memory->index += 2;
		sprintf(temp,"%" FMT_INT64 "d,%lg",(TIMESTAMP)ptr[0],ptr[1]);
		//TODO: Review use of std::min<unsigned long> here
		strncpy(buffer,temp,std::min<unsigned long>(size,sizeof(temp)));
		return buffer;
	}
}

int memory_rewind_player(struct player *my)
{
	my->memory->index = 0;
	return 1;
}

void memory_close_player(struct player *my)
{
	free(my->memory);
}

/*******************************************************************
 * shape generators 
 */
#define MAPSIZE(N) ((N-1)/8+1)
#define SET(X,B) ((X)[(B)/8]|=(1<<((B)&7)))
#define ISSET(X,B) (((X)[(B)/8]&(1<<((B)&7)))==(1<<((B)&7)))
static char *memory=NULL;
static int linenum=0;
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

int memory_open_shaper(struct shaper *my, char *fname, char *flags)
{
	char line[1024], group[256]="(unnamed)";
	float sum=0, load=0, peak=0;
	float scale[12][31][7][24];

	/* clear everything */
	memset(scale,0,sizeof(scale));
	linenum=0; 
	memory=fname;

	/** @todo finish write memory shaper -- it still uses files (no ticket) */
	/* "-" means stdin */
	my->fp = (strcmp(fname,"-")==0?stdin:fopen(fname,flags));
	if (my->fp==NULL)
	{
		gl_error("shaper memory %s: %s", fname, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	my->status=TS_OPEN;
	my->type = FT_MEMORY;
	/** @todo shape defaults should be read from the shape memory, or better yet, inferred from it (no ticket) */
	my->step = 3600; /* default interval step is one hour */
	my->interval = 24; /* default unint shape integrated over one day */
	memset(my->shape,0,sizeof(my->shape));
	/* load the memory into the shape */
	while (fgets(line,sizeof(line),my->fp)!=NULL)
	{
		unsigned char *hours, *days, *months, *weekdays;
		char minute[256],hour[256],day[256],month[256],weekday[256],value[32];
		char *p=line;
		linenum++;
		while (isspace(*p)) p++;
		if (p[0]=='\0' || p[0]=='#') continue;
		if (strcmp(group,"")!=0 && (isdigit(p[0]) || p[0]=='*'))
		{	/* shape value */
			int h, d, m, w;
			if (sscanf(line,"%s %s %s %s %[^,],%[^,\n]",minute,hour,day,month,weekday,value)<6)
			{
				gl_error("%s(%d) : shape '%s' has specification '%s'", memory, linenum, group, line);
				continue;
			}
			/* minutes are ignored right now */
			if (minute[0]!='*') gl_warning("%s(%d) : minutes are ignored in '%s'", memory, linenum, line);
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
			gl_error("%s(%d) : shape specification '%s' is not valid", memory, linenum, line);
		}
	}
	return 1;
}

char *memory_read_shaper(struct shaper *my,char *buffer,unsigned int size)
{
	return fgets(buffer,size,my->fp);
}

int memory_rewind_shaper(struct shaper *my)
{
	return fseek(my->fp,SEEK_SET,0);
}

void memory_close_shaper(struct shaper *my)
{
}

/*******************************************************************
 * recorders 
 */
int memory_open_recorder(struct recorder *my, char *fname, char *flags)
{
	time_t now=time(NULL);
	OBJECT *obj=OBJECTHDR(my);

	my->memory = (MEMORY*)malloc(sizeof(MEMORY));
	if (my->memory==NULL)
	{
		gl_error("memory_open_recorder(struct recorder *my={...}, char *fname='%s', char *flags='%s'): %s", fname, flags, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	my->memory->buffer = gl_global_find(fname);
	if (my->memory->buffer==NULL)
	{
		gl_error("memory_open_recorder(struct recorder *my={...}, char *fname='%s', char *flags='%s'): global '%s' not found", fname, flags, fname);
		my->status = TS_DONE;
		return 0;
	}
	my->memory->index = 0;
	my->type = FT_MEMORY;
	my->last.ts = TS_ZERO;
	my->status=TS_OPEN;
	my->samples=0;
	return 1;
}

int memory_write_recorder(struct recorder *my, char *timestamp, char *value)
{
	if (my->memory->index >= my->memory->buffer->prop->size-1)
		return 0;
	else
	{
		double *ptr = (double*)my->memory->buffer->prop->addr + my->memory->index;
		my->memory->index += 2;
		ptr[0] = (double)gl_parsetime(timestamp);
		ptr[1] = (double)(float)atof(value);
		return 2;
	}
}

void memory_close_recorder(struct recorder *my)
{
	free(my->memory);
}

/*******************************************************************
 * collectors 
 */
int memory_open_collector(struct collector *my, char *fname, char *flags)
{
	time_t now=time(NULL);
	OBJECT *obj=OBJECTHDR(my);

	my->memory = (MEMORY*)malloc(sizeof(MEMORY));
	if (my->memory==NULL)
	{
		gl_error("memory_open_collector(struct recorder *my={...}, char *fname='%s', char *flags='%s'): %s", fname, flags, strerror(errno));
		my->status = TS_DONE;
		return 0;
	}
	my->memory->buffer = gl_global_find(fname);
	if (my->memory->buffer==NULL)
	{
		gl_error("memory_open_collector(struct recorder *my={...}, char *fname='%s', char *flags='%s'): global '%s' not found", fname, flags, fname);
		my->status = TS_DONE;
		return 0;
	}
	my->memory->index = 0;
	my->type = FT_MEMORY;
	my->last.ts = TS_ZERO;
	my->status=TS_OPEN;
	my->samples=0;
	return 1;

}

int memory_write_collector(struct collector *my, char *timestamp, char *value)
{
	if (my->memory->index >= my->memory->buffer->prop->size-1)
		return 0;
	else
	{
		double *ptr = (double*)my->memory->buffer->prop->addr + my->memory->index;
		my->memory->index += 2;
		ptr[0] = (double)gl_parsetime(timestamp);
		ptr[1] = (double)(float)atof(value);
		return 2;
	}
}

void memory_close_collector(struct collector *my)
{
	free(my->memory);
}

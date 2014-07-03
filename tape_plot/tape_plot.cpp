/** $Id: tape_plot.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file tape_plot.cpp
	@addtogroup tape_plot File-based tapes
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
#include "tape_plot.h"
#define MAXCOLUMNS 50

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
		//gl_error(
		fprintf(stderr, "player file %s: %s", fname, strerror(errno));
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
		//gl_error(
		fprintf(stderr, "shaper file %s: %s", fname, strerror(errno));
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
				//gl_error
				fprintf(stderr, "%s(%d) : shape '%s' has specification '%s'", file, linenum, group, line);
				continue;
			}
			/* minutes are ignored right now */
			if (min[0]!='*') //gl_warning
				fprintf(stdout, "%s(%d) : minutes are ignored in '%s'", file, linenum, line);
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
			//gl_error(
			fprintf(stderr, "%s(%d) : shape specification '%s' is not valid", file, linenum, line);
		}
	}
	return 1;
}

EXPORT char *read_shaper(struct shaper *my,char *buffer,unsigned int size)
{
	return fgets(buffer,size,my->fp);
}

EXPORT int rewind_shaper(struct shaper *my)
{
	return fseek(my->fp,SEEK_SET,0);
}

EXPORT void close_shaper(struct shaper *my)
{
}

/*******************************************************************
 * recorders 
 */

EXPORT void write_default_plot_commands_rec(struct recorder *my, char32 extension)
{
	char fname[sizeof(char32)];
	char type[sizeof(char32)];
	char ext[sizeof(char32)];
	char buf[sizeof(char1024)];
	char plotcommands[sizeof(char1024)];
	char *item;
	char list[sizeof(char1024)];

	int i, j, k;	
	i = j = k = 0;
	sscanf(my->file,"%32[^:]:%32[^.].[^\n;:]",type,fname,ext);

	/////////////////////////////////////////////////////////////////////////////
	// Default behavior for directive plotcommands
	/////////////////////////////////////////////////////////////////////////////
	if (my->plotcommands[0]=='\0' || strcmp(my->plotcommands,"")==0) {
		j= strlen(my->columns)>0 ? 0: fprintf(my->fp, "set xdata time;\n");
		fprintf(my->fp, "set datafile separator \",\";\n");
		if(my->output != SCREEN){
			fprintf(my->fp, "set output \"%s.%s\"; \n", fname,extension.get_string());
		}
		fprintf(my->fp, "show output; \n");
		fprintf(my->fp, "set timefmt \"%%Y-%%m-%%d %%H:%%M:%%S\";\n");
		fprintf(my->fp, "set datafile missing 'NaN'\n");
		//j = strlen(my->columns)>0  ? 0: fprintf(my->fp, "plot \'-\' using 1:2 title '%s' with lines;\n", my->property);
		if(strlen(my->columns) > 0){
			j = 0;
		} else {
			strcpy(list,my->property); /* avoid destroying orginal list */
			k = 2;
			for (item=strtok(list,","); item!=NULL; item=strtok(NULL,",")){
				if(k == 2){
					fprintf(my->fp, "plot \'-\' using 1:%i title \'%s\' with lines", k, item);
					/* only handles one column properly as-is, will be fixed with the tape module update -MH */
					fprintf(my->fp, "\n");
					break;
				}// else {
				//	fprintf(my->fp, ", \\\n\t\'-\' using 1:%i title \'%s\' with lines", k, item);
				//}
				++k;
			}
			fprintf(my->fp, "\n");
		}

		return;
	}
	strcpy(plotcommands,  my->plotcommands);

	while ( plotcommands[i] != '\0' )
	{
		while (plotcommands[i] != '|')
			buf[j++] = plotcommands[i++];
		buf[j] = '\0';
		fprintf(my->fp, "%s;\n", buf);
		j = 0;
		i++;
	}
}

EXPORT int open_recorder(struct recorder *my, char *fname, char *flags)
{
	char extension[33];
	char columnlist[1025];
	char **columns;
	time_t now=time(NULL);
	OBJECT *obj=OBJECTHDR(my);
	static int block=0;

	set_recorder(my);

	if (!block) {
		atexit(close_recorder_wrapper);
		block=1;
	}

	columns = (char **)calloc(MAXCOLUMNS, sizeof(char *));
	for(int i=0; i<MAXCOLUMNS; i++){
		columns[i] = (char *)malloc(33);
		memset(columns[i],'\0',32);
	}

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
	fprintf(my->fp,"# target.... %s %d\n", obj->parent->oclass->name, obj->parent->id);
	fprintf(my->fp,"# trigger... %s\n", my->trigger[0]=='\0'?"(none)":my->trigger.get_string());
	fprintf(my->fp,"# interval.. %d\n", my->interval);
	fprintf(my->fp,"# limit..... %d\n", my->limit);
	fprintf(my->fp,"# timestamp,%s\n", my->property.get_string());

	/* Split the property list into items */
  splitString(my->property.get_string(), columns);
	/* Array 'columns' contains the separated items from the property list
	for (int i=0;i<MAXCOLUMNS;i++){
		if (strlen(columns[i])>0)
			fprintf(my->fp,"%s\n",columns[i]);
	}
	*/

	/* Put gnuplot commands in the header portion */
	fprintf(my->fp,"# GNUplot commands below:     \n");
	
	switch (my->output) {
		case SCREEN:
#ifdef WIN32
			fprintf(my->fp, "set terminal windows color;\n");
#else
			fprintf(my->fp, "set terminal x11;\n");
#endif
			break;
		case EPS:
			fprintf(my->fp, "set terminal epslatex;\n");
			strcpy(extension, "EPS");
			break;
		case GIF:
			fprintf(my->fp, "set terminal gif;\n");
			strcpy(extension, "GIF");
			break;
		case JPG:
			fprintf(my->fp, "set terminal jpeg;\n");
			strcpy(extension, "JPG");
			break;
		case PDF:
			fprintf(my->fp, "set terminal pdf;\n");
			strcpy(extension, "PDF");
			break;
		case PNG:
			fprintf(my->fp, "set terminal png;\n");
			strcpy(extension, "PNG");
			break;
		case SVG:
			fprintf(my->fp, "set terminal svg;\n");
			strcpy(extension, "SVG");
			break;
		default:
			fprintf(my->fp, "set terminal jpeg;\n");
			strcpy(extension, "JPG");
			break;
	}

	write_default_plot_commands_rec(my, extension);
	if (my->columns[0]){
		sscanf(my->columns,"%s %s", columnlist);
		fprintf(my->fp, "plot \'-\' using %s with lines;\n", columnlist);
	}
	
	free(columns);

	return 1;
}

void splitString(char *propertyStr, char *columns[])
{
	unsigned int i=0,j=0,k=0;
	char buf[32];

	while( propertyStr[i] != '\0'){
		while ( (propertyStr[i] != ',') && (i < strlen(propertyStr)) )
			buf[j++] = propertyStr[i++];
		  buf[j] = '\0';
			memcpy(columns[k], buf, 32);
			k++;
			i++;
			j=0;
	}
}
		    



EXPORT int write_recorder(struct recorder *my, char *timestamp, char *value)
{
	return fprintf(my->fp,"%s,%s\n", timestamp, value);
}
 
void set_recorder(struct recorder *my)
{
	global_rec = my;
}

void close_recorder_wrapper(void)
{
	struct recorder *my;
	my = global_rec;
	close_recorder(my);
}

EXPORT void close_recorder(struct recorder *my)
{
	char gnuplot[1024];
#ifdef WIN32
	char *plotcmd = "start wgnuplot";
#else
	char *plotcmd = "gnuplot";
#endif
	if(my->output == SCREEN)
		sprintf(gnuplot,"%s -persist", plotcmd);
	else
		strcpy(gnuplot,plotcmd);
	char fname[sizeof(char32)];
	char type[sizeof(char32)];
	char command[sizeof(char1024)];

	my->status = TS_DONE;

	if (my->fp == NULL)
		return;
	else {
		fprintf(my->fp,"e\n");
		//fprintf(my->fp,"pause -1");// \"PRESS RETURN TO CONTINUE\" \n" );
		fprintf(my->fp,"# end of tape\n");
		fclose(my->fp);
		sscanf(my->file,"%32[^:]:%32[^:]",type,fname);
		sprintf(command,"%s %s", gnuplot, fname);
		system( command );
	}
 	my->fp = NULL;
}

/*******************************************************************
 * collectors 
 */

EXPORT void write_default_plot_commands_col(struct collector *my, char32 extension)
{
	char fname[sizeof(char32)];
	char type[sizeof(char32)];
	char buf[sizeof(char1024)];
	char plotcommands[sizeof(char1024)];
	int i, j;	
	i = j = 0;
	sscanf(my->file,"%32[^:]:%32[^:]",type,fname);

	/////////////////////////////////////////////////////////////////////////////
	// Default behavior for directive plotcommands
	/////////////////////////////////////////////////////////////////////////////
	if (my->plotcommands[0]=='\0' || strcmp(my->plotcommands,"")==0) {
		j= strlen(my->columns)>0 ? 0: fprintf(my->fp, "set xdata time;\n");
		fprintf(my->fp, "set datafile separator \",\";\n");
		fprintf(my->fp, "set output \"%s.%s\"; \n", fname,extension.get_string());
		fprintf(my->fp, "show output; \n");
		fprintf(my->fp, "set timefmt \"%%Y-%%m-%%d %%H:%%M:%%S\";\n");
		j = strlen(my->columns)>0  ? 0: fprintf(my->fp, "plot \'-\' using 1:2 with lines;\n");
		return;
	}
	strcpy(plotcommands,  my->plotcommands);

	while ( plotcommands[i] != '\0' )
	{
		while (plotcommands[i] != '|')
			buf[j++] = plotcommands[i++];
		buf[j] = '\0';
		fprintf(my->fp, "%s;\n", buf);
		j = 0;
		i++;
	}
}
EXPORT int open_collector(struct collector *my, char *fname, char *flags)
{
	unsigned int count=0;
	time_t now=time(NULL);

	char extension[sizeof(char32)];
	char columnlist[sizeof(char1024)];
	char **columns;
	OBJECT *obj=OBJECTHDR(my);

	columns = (char **)calloc(MAXCOLUMNS, sizeof(char *));
	for(int i=0; i<MAXCOLUMNS; i++){
		columns[i] = (char *)malloc(33);
		memset(columns[i],'\0',32);
	}

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

  splitString(my->property.get_string(), columns);

	/* Put gnuplot commands in the header portion */
	fprintf(my->fp,"# GNUplot commands below:     \n");
	
	switch (my->output) {
		case SCREEN:
#ifdef WIN32
			fprintf(my->fp, "set terminal windows color;\n");
#else
			fprintf(my->fp, "set terminal x11;\n");
#endif
			break;
		case EPS:
			fprintf(my->fp, "set terminal epslatex;\n");
			strcpy(extension, "EPS");
			break;
		case GIF:
			fprintf(my->fp, "set terminal gif;\n");
			strcpy(extension, "GIF");
			break;
		case JPG:
			fprintf(my->fp, "set terminal jpeg;\n");
			strcpy(extension, "JPG");
			break;
		case PDF:
			fprintf(my->fp, "set terminal pdf;\n");
			strcpy(extension, "PDF");
			break;
		case PNG:
			fprintf(my->fp, "set terminal png;\n");
			strcpy(extension, "PNG");
			break;
		case SVG:
			fprintf(my->fp, "set terminal svg;\n");
			strcpy(extension, "SVG");
			break;
		default:
			fprintf(my->fp, "set terminal jpeg;\n");
			strcpy(extension, "JPG");
			break;
	}

	write_default_plot_commands_col(my, extension);
	if (my->columns){
		sscanf(my->columns,"%s %s", columnlist);
		fprintf(my->fp, "plot \'-\' using %s with lines;\n", columnlist);
	}

	free(columns);
	
	return count;
}

EXPORT int write_collector(struct collector *my, char *timestamp, char *value)
{
	return fprintf(my->fp,"%s,%s\n", timestamp, value);
}

EXPORT void close_collector(struct collector *my)
{
#ifdef WIN32
	char gnuplot[sizeof(char32)];
	strcpy(gnuplot,"wgnuplot ");
	_putenv("PATH=%PATH%;C:\\wgnuplot");
#else
	char *gnuplot = "gnuplot ";
#endif
	char fname[sizeof(char32)];
	char type[sizeof(char32)];
	char command[sizeof(char1024)];

	my->status = TS_DONE;

	if (my->fp == NULL)
		return;
	else {
		fprintf(my->fp,"e\n");
		fprintf(my->fp,"pause -1");// \"PRESS RETURN TO CONTINUE\" \n" );
		fprintf(my->fp,"# end of tape\n");
		fclose(my->fp);
		sscanf(my->file,"%32[^:]:%32[^:]",type,fname);
		sprintf(command,"%s %s", gnuplot, fname);
		system( command );
	}
 	my->fp = NULL;
}
/**@}*/

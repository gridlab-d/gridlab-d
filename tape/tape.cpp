/** $Id: tape.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file tape.c
	@addtogroup tapes Players and recorders (tape)
	@ingroup modules

	Tape players and recorders are used to manage the boundary conditions
	and record properties of objects during simulation.  There are two kinds
	of players and two kinds of recorders:
	- \p player is used to play a recording of a single value to property of an object
	- \p shaper is used to play a periodic scaled shape to a property of groups of objects
	- \p recorder is used to collect a recording of one of more properties of an object
	- \p collector is used to collect an aggregation of a property from a group of objects
 @{
 **/

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "gridlabd.h"
#include "object.h"
#include "aggregate.h"
#include "histogram.h"
#include "group_recorder.h"

#include "tape.h"
#include "file.h"
#include "odbc.h"
#include "violation_recorder.h"
#include "metrics_collector.h"
#include "metrics_collector_writer.h"


#include "shaper.h"
#include "collector.h"
#include "player.h"
#include "recorder.h"

#define MAP_DOUBLE(X,LO,HI) {#X,VT_DOUBLE,&X,LO,HI}
#define MAP_INTEGER(X,LO,HI) {#X,VT_INTEGER,&X,LO,HI}
#define MAP_STRING(X) {#X,VT_STRING,X,sizeof(X),0}
#define MAP_END {nullptr}

VARMAP varmap[] = {
	/* add module variables you want to be available using module_setvar in core */
	MAP_STRING(timestamp_format),
	MAP_END
};

extern CLASS *player_class;
extern CLASS *shaper_class;
extern CLASS *recorder_class;
extern CLASS *multi_recorder_class;
extern CLASS *collector_class;

/* delta mode control */
TIMESTAMP delta_mode_needed = TS_NEVER; /* the time at which delta mode needs to start */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#include <windows.h>
#ifndef DLEXT
#define DLEXT ".dll"
#endif
#define DLLOAD(P) LoadLibrary(P)
#define DLSYM(H,S) GetProcAddress((HINSTANCE)H,S)
#define snprintf _snprintf
#else /* ANSI */
#include "dlfcn.h"
#include "violation_recorder.h"
#include "metrics_collector.h"
#include "metrics_collector_writer.h"

#ifndef DLEXT
#define DLEXT ".so"
#else
#endif
#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#define DLSYM(H,S) dlsym(H,S)
#endif

#ifndef WIN32
#define strtok_s strtok_r
#else
#ifdef __MINGW32__
char* strtok_t(char *str, const char *delim, char **nextp)
{
	char *ret;

	if (str == nullptr)
	{
		str = *nextp;
	}

	str += strspn(str, delim);

	if (*str == '\0')
	{
		return nullptr;
	}

	ret = str;

	str += strcspn(str, delim);

	if (*str)
	{
		*str++ = '\0';
	}

	*nextp = str;

	return ret;
}
#undef strtok_s
#define strtok_s strtok_t
#endif
#endif

static TAPEFUNCS *funcs = nullptr;
static char1024 tape_gnuplot_path;
int32 flush_interval = 0;
int csv_data_only = 0; /* enable this option to suppress addition of lines starting with # in CSV */
int csv_keep_clean = 0; /* enable this option to keep data flushed at end of line */
void (*update_csv_data_only)(void)=nullptr;
void (*update_csv_keep_clean)(void)=nullptr;

void set_csv_options(void)
{
	if (csv_data_only && update_csv_data_only)
		(*update_csv_data_only)();
	if (csv_keep_clean && update_csv_keep_clean)
		(*update_csv_keep_clean)();
}

typedef int (*OPENFUNC)(void *, char *, char *);
typedef char *(*READFUNC)(void *, char *, unsigned int);
typedef int (*WRITEFUNC)(void *, char *, char *);
typedef int (*REWINDFUNC)(void *);
typedef void (*CLOSEFUNC)(void *);
//TODO verify not a typo
typedef void (*VOIDCALL)(void);
typedef void (*FLUSHFUNC)(void *);

TAPEFUNCS *get_ftable(char *mode){
	/* check what we've already loaded */
	char256 modname;
	TAPEFUNCS *fptr = funcs;
	TAPEOPS *ops = nullptr;
	void *lib = nullptr;
	CALLBACKS **c = nullptr;
	char tpath[1024];
	while(fptr != nullptr){
		if(strcmp(fptr->mode, mode) == 0)
			return fptr;
		fptr = fptr->next;
	}
	/* fptr = nullptr */
	fptr = static_cast<TAPEFUNCS *>(malloc(sizeof(TAPEFUNCS)));
	if(fptr == nullptr)
	{
		gl_error("get_ftable(char *mode='%s'): out of memory", mode);
		return nullptr; /* out of memory */
	}
	snprintf(modname, sizeof(modname), "tape_%s" DLEXT, mode);

	if(gl_findfile(modname, nullptr, 0|4, tpath,sizeof(tpath)) == nullptr){
		gl_error("unable to locate %s", modname.get_string());
		return nullptr;
	}
	lib = fptr->hLib = DLLOAD(tpath);
	if(fptr->hLib == nullptr){
		gl_error("tape module: unable to load DLL for %s", modname.get_string());
		return nullptr;
	}
	c = (CALLBACKS **)DLSYM(lib, "callback");
	if(c)
		*c = callback;

	//	nonfatal ommission
	ops = fptr->collector = static_cast<TAPEOPS *>(malloc(sizeof(TAPEOPS)));
	memset(ops,0,sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_collector");
	ops->read = nullptr;
	ops->write = (WRITEFUNC)DLSYM(lib, "write_collector");
	ops->rewind = nullptr;
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_collector");
	ops->flush = (FLUSHFUNC)DLSYM(lib, "flush_collector");

	ops = fptr->player = static_cast<TAPEOPS *>(malloc(sizeof(TAPEOPS)));
	memset(ops,0,sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_player");
	ops->read = (READFUNC)DLSYM(lib, "read_player");
	ops->write = nullptr;
	ops->rewind = (REWINDFUNC)DLSYM(lib, "rewind_player");
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_player");
	ops->flush = nullptr;

	ops = fptr->recorder = static_cast<TAPEOPS *>(malloc(sizeof(TAPEOPS)));
	memset(ops,0,sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_recorder");
	ops->read = nullptr;
	ops->write = (WRITEFUNC)DLSYM(lib, "write_recorder");
	ops->rewind = nullptr;
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_recorder");
	ops->flush = (FLUSHFUNC)DLSYM(lib, "flush_recorder");

	ops = fptr->histogram = static_cast<TAPEOPS *>(malloc(sizeof(TAPEOPS)));
	memset(ops,0,sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_histogram");
	ops->read = nullptr;
	ops->write = (WRITEFUNC)DLSYM(lib, "write_histogram");
	ops->rewind = nullptr;
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_histogram");
	ops->flush = (FLUSHFUNC)DLSYM(lib, "flush_histogram");

	ops = fptr->shaper = static_cast<TAPEOPS *>(malloc(sizeof(TAPEOPS)));
	memset(ops,0,sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_shaper");
	ops->read = (READFUNC)DLSYM(lib, "read_shaper");
	ops->write = nullptr;
	ops->rewind = (REWINDFUNC)DLSYM(lib, "rewind_shaper");
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_shaper");

	fptr->next = funcs;
	funcs = fptr;

	update_csv_data_only = (VOIDCALL)DLSYM(lib,"set_csv_data_only");
	update_csv_keep_clean = (VOIDCALL)DLSYM(lib,"set_csv_keep_clean");
	return funcs;
}

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	struct recorder my;
	struct collector my2;

	if (set_callback(fntable)==nullptr)
	{
		errno = EINVAL;
		return nullptr;
	}

	/* globals for the tape module*/
#ifdef _WIN32
	sprintf(tape_gnuplot_path, "c:/Program Files/GnuPlot/bin/wgnuplot.exe");
#else
	sprintf(tape_gnuplot_path,"/usr/bin/gnuplot");
#endif
	gl_global_create(const_cast<char *>("tape::gnuplot_path"), PT_char1024, &tape_gnuplot_path, nullptr);
	gl_global_create(const_cast<char *>("tape::flush_interval"), PT_int32, &flush_interval, nullptr);
	gl_global_create(const_cast<char *>("tape::csv_data_only"), PT_int32, &csv_data_only, nullptr);
	gl_global_create(const_cast<char *>("tape::csv_keep_clean"), PT_int32, &csv_keep_clean, nullptr);

	/* control delta mode */
	gl_global_create(const_cast<char *>("tape::delta_mode_needed"), PT_timestamp, &delta_mode_needed, nullptr);

	/* register the first class implemented, use SHARE to reveal variables */
	player_class = gl_register_class(module, const_cast<char *>("player"), sizeof(struct player), PC_PRETOPDOWN);
	player_class->trl = TRL_PROVEN;
	PUBLISH_STRUCT(player,char256,property);
	PUBLISH_STRUCT(player,char1024,file);
	PUBLISH_STRUCT(player,char8,filetype);
	PUBLISH_STRUCT(player,char32,mode);
	PUBLISH_STRUCT(player,int32,loop);
	PUBLISH_STRUCT(player,bool,all_events_delta);

	/* register the first class implemented, use SHARE to reveal variables */
	shaper_class = gl_register_class(module, const_cast<char *>("shaper"), sizeof(struct shaper), PC_PRETOPDOWN);
	shaper_class->trl = TRL_QUALIFIED;
	PUBLISH_STRUCT(shaper,char1024,file);
	PUBLISH_STRUCT(shaper,char8,filetype);
	PUBLISH_STRUCT(shaper,char32,mode);
	PUBLISH_STRUCT(shaper,char256,group);
	PUBLISH_STRUCT(shaper,char256,property);
	PUBLISH_STRUCT(shaper,double,magnitude);
	PUBLISH_STRUCT(shaper,double,events);

	/* register the other classes as needed, */
	recorder_class = gl_register_class(module, const_cast<char *>("recorder"), sizeof(struct recorder), PC_POSTTOPDOWN | PC_OBSERVER);
	recorder_class->trl = TRL_PROVEN;
	PUBLISH_STRUCT(recorder,char1024,property);
	PUBLISH_STRUCT(recorder,char32,trigger);
	PUBLISH_STRUCT(recorder,char1024,file);
	PUBLISH_STRUCT(recorder,char8,filetype);
	PUBLISH_STRUCT(recorder,char32,mode);
	PUBLISH_STRUCT(recorder,char1024,multifile);
	//PUBLISH_STRUCT(recorder,int64,interval);
	PUBLISH_STRUCT(recorder,int32,limit);
	PUBLISH_STRUCT(recorder,char1024,plotcommands);
	PUBLISH_STRUCT(recorder,char32,xdata);
	PUBLISH_STRUCT(recorder,char32,columns);
	PUBLISH_STRUCT(recorder,int32,flush);
    PUBLISH_STRUCT(recorder,bool,format);

	if(gl_publish_variable(recorder_class,
		PT_double, "interval[s]", ((char*)&(my.dInterval) - (char *)&my),
		PT_enumeration, "output", ((char*)&(my.output) - (char *)&my),
			PT_KEYWORD, "SCREEN", SCREEN,
			PT_KEYWORD, "EPS",    EPS,
			PT_KEYWORD, "GIF",    GIF,
			PT_KEYWORD, "JPG",    JPG,
			PT_KEYWORD, "PDF",    PDF,
			PT_KEYWORD, "PNG",    PNG,
			PT_KEYWORD, "SVG",    SVG,
		PT_enumeration, "header_units", ((char*)&(my.header_units) - (char *)&my),
			PT_KEYWORD, "DEFAULT", HU_DEFAULT,
			PT_KEYWORD, "ALL", HU_ALL,
			PT_KEYWORD, "NONE", HU_NONE,
		PT_enumeration, "line_units", ((char*)&(my.line_units) - (char *)&my),
			PT_KEYWORD, "DEFAULT", LU_DEFAULT,
			PT_KEYWORD, "ALL", LU_ALL,
			PT_KEYWORD, "NONE", LU_NONE,
			nullptr) < 1)
		GL_THROW(const_cast<char *>("Could not publish property output for recorder"));

		/* register the other classes as needed, */
	multi_recorder_class = gl_register_class(module, const_cast<char *>("multi_recorder"), sizeof(struct recorder), PC_POSTTOPDOWN | PC_OBSERVER);
	multi_recorder_class->trl = TRL_QUALIFIED;
	if(gl_publish_variable(multi_recorder_class,
		PT_double, "interval[s]", ((char*)&(my.dInterval) - (char *)&my),
		PT_char1024, "property", ((char*)&(my.property) - (char *)&my),
		PT_char32, "trigger", ((char*)&(my.trigger) - (char *)&my),
		PT_char1024, "file", ((char*)&(my.file) - (char *)&my),
		PT_char8, "filetype", ((char*)&(my.filetype) - (char *)&my),
		PT_char32, "mode", ((char*)&(my.mode) - (char *)&my),
		PT_char1024, "multifile", ((char*)&(my.multifile) - (char *)&my),
		PT_int32, "limit", ((char*)&(my.limit) - (char *)&my),
		PT_char1024, "plotcommands", ((char*)&(my.plotcommands) - (char *)&my),
		PT_char32, "xdata", ((char*)&(my.xdata) - (char *)&my),
		PT_char32, "columns", ((char*)&(my.columns) - (char *)&my),
        PT_char32, "format", ((char*)&(my.format) - (char *)&my),
		PT_enumeration, "output", ((char*)&(my.output) - (char *)&my),
			PT_KEYWORD, "SCREEN", SCREEN,
			PT_KEYWORD, "EPS",    EPS,
			PT_KEYWORD, "GIF",    GIF,
			PT_KEYWORD, "JPG",    JPG,
			PT_KEYWORD, "PDF",    PDF,
			PT_KEYWORD, "PNG",    PNG,
			PT_KEYWORD, "SVG",    SVG,
		PT_enumeration, "header_units", ((char*)&(my.header_units) - (char *)&my),
			PT_KEYWORD, "DEFAULT", HU_DEFAULT,
			PT_KEYWORD, "ALL", HU_ALL,
			PT_KEYWORD, "NONE", HU_NONE,
		PT_enumeration, "line_units", ((char*)&(my.line_units) - (char *)&my),
			PT_KEYWORD, "DEFAULT", LU_DEFAULT,
			PT_KEYWORD, "ALL", LU_ALL,
			PT_KEYWORD, "NONE", LU_NONE,
			nullptr) < 1)
		GL_THROW(const_cast<char *>("Could not publish property output for multi_recorder"));

	/* register the other classes as needed, */
	collector_class = gl_register_class(module, const_cast<char *>("collector"), sizeof(struct collector), PC_POSTTOPDOWN | PC_OBSERVER);
	collector_class->trl = TRL_PROVEN;
	PUBLISH_STRUCT(collector,char1024,property);
	PUBLISH_STRUCT(collector,char32,trigger);
	PUBLISH_STRUCT(collector,char1024,file);
	//PUBLISH_STRUCT(collector,int64,interval);
	PUBLISH_STRUCT(collector,int32,limit);
	PUBLISH_STRUCT(collector,char256,group);
	PUBLISH_STRUCT(collector,int32,flush);
	if(gl_publish_variable(collector_class,
		PT_double, "interval[s]", ((char*)&(my2.dInterval) - (char *)&my2),
			nullptr) < 1)
		GL_THROW(const_cast<char *>("Could not publish property output for collector"));

	/* new histogram() */
  new_histogram(module);

	/* new group_recorder() */
  new_group_recorder(module);

	/* new violation_recorder() */
  new_violation_recorder(module);

	/* new metrics_collector() */
  new_metrics_collector(module);

	/* new metrics_collector_writer() */
  new_metrics_collector_writer(module);

#if 0
	new_loadshape(module);
#endif // zero

//	new_schedule(module);

	/* always return the first class registered */
	return player_class;
}

EXPORT int check(void)
{
	unsigned int errcount=0;
	char fpath[1024];
	/* check players */
	{	OBJECT *obj=nullptr;
		FINDLIST *players = gl_find_objects(FL_NEW,FT_CLASS,SAME,"tape",FT_END);
		while ((obj=gl_find_next(players,obj))!=nullptr)
		{
			struct player *pData = OBJECTDATA(obj,struct player);
			if (gl_findfile(pData->file,nullptr,F_OK,fpath,sizeof(fpath))==nullptr)
			{
				errcount++;
				gl_error("player %s (id=%d) uses the file '%s', which cannot be found", obj->name?obj->name:"(unnamed)", obj->id, pData->file.get_string());
			}
		}
	}

	/* check shapers */
	{	OBJECT *obj=nullptr;
		FINDLIST *shapers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"shaper",FT_END);
		while ((obj=gl_find_next(shapers,obj))!=nullptr)
		{
			struct shaper *pData = OBJECTDATA(obj,struct shaper);
			if (gl_findfile(pData->file,nullptr,F_OK,fpath,sizeof(fpath))==nullptr)
			{
				errcount++;
				gl_error("shaper %s (id=%d) uses the file '%s', which cannot be found", obj->name?obj->name:"(unnamed)", obj->id, pData->file.get_string());
			}
		}
	}

	return errcount;
}

/* DELTA MODE SUPPORT */
/*
	Delta mode is supported by maintaining a list of recorders that are enabled
	for high-speed sampling.  This list is populated for sync_recorder based on
	whether OF_DELTAMODE is set.  Objects in the list are recorded to the output
	during the interupdate() operation and not during the object update (which is
	too slow and could happen at the wrong time).
 */

double recorder_delta_clock = 0.0;
DELTAOBJ_LIST *delta_tape_objects = nullptr;

/* Function to add a tape module object into a deltamode list */
int delta_add_tape_device(OBJECT *obj, DELTATAPEOBJ tape_type)
{
	DELTAOBJ_LIST *temp_ll_item, *index_item;

	/* Allocate one */
	temp_ll_item = (DELTAOBJ_LIST*)gl_malloc(sizeof(DELTAOBJ_LIST));

	/* Make sure it worked */
	if (temp_ll_item == nullptr)
	{
		gl_error("tape object:%d - unable to allocate space for deltamode",obj->id);
		/*  TROUBLESHOOT
		While attempting to allocate space for a tape module deltamode-capable object, an error occurred.  Please try again.
		If the error persists, please submit your code and a bug report via the ticketing system.
		*/
		return FAILED;
	}

	/* Populate it */
	temp_ll_item->obj = obj;
	temp_ll_item->obj_type = tape_type;
	temp_ll_item->next = nullptr;

	/* Find the place */
	if (delta_tape_objects != nullptr)
	{
		/* Get initial value */
		index_item = delta_tape_objects;

		while (index_item->next != nullptr)
		{
			index_item = index_item->next;
		}

		//This should be the place to insert
		index_item->next = temp_ll_item;
	}
	else /* Only one item, just add us in there */
	{
		delta_tape_objects = temp_ll_item;
	}

	/* If we made it this far, must be successful */
	return SUCCESS;
}

void enable_deltamode(TIMESTAMP t)
{
	if ((t<delta_mode_needed) && ((t-gl_globalclock)<0x7fffffff )) // cannot exceed 31 bit integer
		delta_mode_needed = t;
}

EXPORT unsigned long deltamode_desired(int *flags)
{
	TIMESTAMP tdiff = delta_mode_needed-gl_globalclock;

	/* delta mode is desired if any tape have subsecond data next */
	/* Prevents a DT_INVALID, since no real need for it here (>0 check) */
	if ((delta_mode_needed!=TS_NEVER) && (tdiff<0x7fffffff) && (tdiff>=0)) /* 31 bit limit */
		return (unsigned long)tdiff;
	else
		return DT_INFINITY;
}

EXPORT unsigned long preupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	/* no need to step faster than any other object does */
	return DT_INFINITY;
}

EXPORT SIMULATIONMODE interupdate(MODULE *module, TIMESTAMP t0, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	DELTAOBJ_LIST *index_item;
	SIMULATIONMODE mode = SM_EVENT;

	/* determine the timestamp */
	double clock_val = (double)gl_globalclock + (double)delta_time/(double)DT_SECOND;
	char recorder_timestamp[64];

	/* prepare the timestamp */
	static char global_dateformat[8]="";

	auto integer_clock = (TIMESTAMP)clock_val;	/* Whole seconds - update from global clock because we could be in delta for over 1 second */
	int microseconds = (int)((clock_val-(int)(clock_val))*1000000+0.5);	/* microseconds roll-over - biased upward (by 0.5) */

	/* Recorders should only "fire" on the 0th iteration - may need to adjust if "0" is implemented in deltamode */
	if (iteration_count_val==0)
	{
		/* Set up recorder clock - ignore "first" entry since that is just steady state */
		if ((integer_clock != t0) || (microseconds != 0))
		{
			DATETIME rec_date_time;
			auto rec_integer_clock = (TIMESTAMP)recorder_delta_clock;	/* Whole seconds - update from global clock because we could be in delta for over 1 second */
			int rec_microseconds = (int)((recorder_delta_clock-(int)(recorder_delta_clock))*1000000+0.5);	/* microseconds roll-over - biased upward (by 0.5) */
			if ( gl_localtime(rec_integer_clock,&rec_date_time)!=0 )
			{
				if ( global_dateformat[0]=='\0')
					gl_global_getvar(const_cast<char *>("dateformat"), global_dateformat, sizeof(global_dateformat));
				if ( strcmp(global_dateformat,"ISO")==0)
					sprintf(recorder_timestamp,"%04d-%02d-%02d %02d:%02d:%02d.%.06d %s",rec_date_time.year,rec_date_time.month,rec_date_time.day,rec_date_time.hour,rec_date_time.minute,rec_date_time.second,rec_microseconds,rec_date_time.tz);
				else if ( strcmp(global_dateformat,"US")==0)
					sprintf(recorder_timestamp,"%02d-%02d-%04d %02d:%02d:%02d.%.06d %s",rec_date_time.month,rec_date_time.day,rec_date_time.year,rec_date_time.hour,rec_date_time.minute,rec_date_time.second,rec_microseconds,rec_date_time.tz);
				else if ( strcmp(global_dateformat,"EURO")==0)
					sprintf(recorder_timestamp,"%02d-%02d-%04d %02d:%02d:%02d.%.06d %s",rec_date_time.day,rec_date_time.month,rec_date_time.year,rec_date_time.hour,rec_date_time.minute,rec_date_time.second,rec_microseconds,rec_date_time.tz);
				else
					sprintf(recorder_timestamp,"%.09f",recorder_delta_clock);
			}
			else
				sprintf(recorder_timestamp,"%.09f",recorder_delta_clock);

			/* Initialize loop */
			index_item = delta_tape_objects;

			/* Loop through and find a recorder */
			while (index_item != nullptr)
			{
				if (index_item->obj_type == RECORDER)
				{
					OBJECT *obj = index_item->obj;
					struct recorder *my = OBJECTDATA(obj,struct recorder);
					char value[1024];
					//extern int read_properties(struct recorder *my, OBJECT *obj, PROPERTY *prop, char *buffer, int size);

					/* See if we're in service */
					if ((obj->in_svc_double <= gl_globaldeltaclock) && (obj->out_svc_double >= gl_globaldeltaclock))
					{
						if( read_properties(my, obj->parent,my->target,value,sizeof(value)) )
						{
							if ( !my->ops->write(my, recorder_timestamp, value) )
							{
								gl_error("recorder:%d: unable to write sample to file", obj->id);
								return SM_ERROR;
							}
						}
						else /* Failed - handle accordingly */
						{
							/* Assume message was in read_properties */
							return SM_ERROR;
						}
						
					}
					/* Defaulted else, not in service, do nothing */
				}/* End recorder */
				/* Default else -- not a recorder, so skip */

				/* Grab the next item */
				index_item = index_item->next;
			}//End list loop
		}

		/* Store recorder clock (to be used next time) */
		recorder_delta_clock = clock_val;
	}/* End Recorder only on 0th iteration loop */

	/*** Keep going below here ***/
	/* input samples from players */
	/* Initialize loop */
	index_item = delta_tape_objects;

	/* Loop through and find a recorder */
	while (index_item != nullptr)
	{
		if (index_item->obj_type == PLAYER)
		{
			OBJECT *obj = index_item->obj;
			OBJECT *temp_obj = nullptr;
			struct player *my = OBJECTDATA(obj,struct player);
			int y=0,m=0,d=0,H=0,M=0,S=0,ms=0, n=0;
			char *fmt = const_cast<char *>("%d/%d/%d %d:%d:%d.%d,%*s");
			double t = (double)my->next.ts + (double)my->next.ns/1e9;
			char256 curr_value;
			TIMESTAMP return_value;
			int ret_value;

			/* See if we're in service */
			if ((obj->in_svc_double <= gl_globaldeltaclock) && (obj->out_svc_double >= gl_globaldeltaclock))
			{
				strcpy(curr_value,my->next.value);

				/* post the current value */
				if ( t<=clock_val )
				{
					//extern TIMESTAMP player_read(OBJECT *obj);

					/* Check to make sure we've be initialized -- if a deltamode timestep is first, it may be before this was initialized */
					if (my->target == nullptr)	/* Not set yet */
					{
						/*  This fails on Mac builds under 4.0 for some odd reason.  Commenting code and putting an error for now.
						Will be investigated further for 4.1 release.
						*/

						gl_error("deltamode player: player '%s' has a deltamode timestep starting it - please avoid this",(obj->name ? obj->name : "(anon)"));
						/*  TROUBLESHOOT
						Starting a player with a deltamode timestep causes segmentation faults.  A fix was developed, but still has issues with
						certain platforms and must be investigated further.  For now, start all player files with non-deltamode timesteps.  This
						issues is documented at #1001 on the GridLAB-D Git site, and it expected to be resolved in release 4.1.
						*/

						return SM_ERROR;

						/* *** Commenting -- all existing comment braces turned to ***
						*** Make sure we actually opened the file (this part is new - paranoia check ***
						if (my->status != TS_OPEN)
						{
							gl_error("deltamode player: player '%s' has not been opened yet!",obj->name?obj->name:"(anon)");
							***  TROUBLESHOOT
							While attempting to start a player with a deltamode timestep, it was found the player hasn't been opened yet.
							Please submit your code and a bug report via the issue system.  To work around this, put a non-deltamode timestep
							at the beginning of your player file
							***

							*** Set our status, just in case ***
							my->status = TS_ERROR;

							return SM_ERROR;
						}

						*** Code copied from player sync ***
						my->target = player_link_properties(my, obj->parent, my->property);

						*** Make sure it worked ***
						if (my->target==nullptr){
							gl_error("deltamode player: Unable to find property '%s' in object %s", my->property, obj->name?obj->name:"(anon)");
							***  TROUBLESHOOT
							While attempting to link up the property of a player in deltamode, the property could not be found.  Make sure the object
							exists and has the specified property and try again.
							***
							my->status = TS_ERROR;
							return SM_ERROR;
						}

						*** Do an initialization of it ***
						if (my->target!=nullptr)
						{
							OBJECT *target = obj->parent ? obj->parent : obj; *** target myself if no parent ***
							player_write_properties(my, obj, target, my->target, my->next.value);
						}

						*** Copy the current value into our "tracking" variable ***
						my->delta_track.ns = my->next.ns;
						my->delta_track.ts = my->next.ts;
						memcpy(my->delta_track.value,my->next.value,sizeof(char1024));
						*/
					}/* Target not already set */

					/* Behave similar to "supersecond" players */
					while ( t<=clock_val )
					{
						//Figure out the reference - for value transforms (not parent)
						temp_obj = obj->parent ? obj->parent : obj;

						//Set the value
						ret_value = gl_set_value(temp_obj,GETADDR(temp_obj,my->target),my->next.value,my->target); /* pointer => int64 */

						if (ret_value == 0)
						{
							gl_error("player:%d: %s - failure to set or convert property", obj->id, (obj->name?obj->name:"Unnamed"));
							/*  TROUBLESHOOT
							While trying to set the value from a player, an error occurred.  Check your file and try again.
							*/
							return SM_ERROR;
						}

						/* read the next value */
						return_value = player_read(obj);

						/* Make sure it worked */
						if (return_value == TS_INVALID)
						{
							return SM_ERROR;
						}

						/* update time */
						t = (double)my->next.ts + (double)my->next.ns/1e9;
					}
				}

				/* Determine if we are at the end of the player or not (time check) */
				if (my->next.ts != TS_NEVER)
				{
					/* Update time */
					t = (double)my->next.ts + (double)my->next.ns/1e9;

					/* Make sure we haven't passed this time already (last value) */
					/* Check to see if we're within the next second too, so we aren't stuck in delta */
					if ((t>=clock_val) && (t<(clock_val+1.0)))
					{
						/* determine whether deltamode remains necessary */
						if ((my->next.ns!=0) || my->all_events_delta)
						{
							mode = SM_DELTA;
							gl_verbose("Tape object:%d - %s - requested deltamode to continue",obj->id,(obj->name ? obj->name : "Unnamed"));
						}
					}
				}/*End not TS_NEVER */
				else	/* Player is done - set the flag variables, just to prevent some other issues in sync_player */
				{
					/* These prevent the code on player.c:488 from overwriting with an older value */
					my->delta_track.ns = 0;
					my->delta_track.ts = TS_NEVER;
				}
			}
			/* Default else - not in service */
		}/* End of player loop */
		/* Default else - not a player */

		/*Next item in the loop */
		index_item = index_item->next;
	}

	return mode;
}

EXPORT STATUS postupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	DELTAOBJ_LIST *index_item;
	OBJECT *obj = nullptr;
	struct recorder *myrec = nullptr;
	struct player *myplayer = nullptr;
	FUNCTIONADDR temp_fxn;
	char value[1024];

	/* Perform one final update of recorder - otherwise it misses the last "value" */
	/* Code copied out of interupdate above */
	/* determine the timestamp */
	char recorder_timestamp[64];
	DATETIME rec_date_time;
	TIMESTAMP rec_integer_clock = TS_INVALID; // FIXME: make sure this makes sense.
	int rec_microseconds;
	bool recorder_init_items = false;
	char global_dateformat[8]="";
	int return_val;

	/* Initialize loop */
	index_item = delta_tape_objects;

	/* Loop through objects and look for recorder */
	/* Only look for recorders first -- ensures they pull the last value before a player overwrites it */
	while (index_item != nullptr)
	{
		if (index_item->obj_type == RECORDER)
		{
			if (!recorder_init_items)
			{
				/* Set up recorder clock */
				rec_integer_clock = (TIMESTAMP)recorder_delta_clock;	/* Whole seconds - update from global clock because we could be in delta for over 1 second */
				rec_microseconds = (int)((recorder_delta_clock-(int)(recorder_delta_clock))*1000000+0.5);	/* microseconds roll-over - biased upward (by 0.5) */
				if ( gl_localtime(rec_integer_clock,&rec_date_time)!=0 )
				{
					if ( global_dateformat[0]=='\0')
						gl_global_getvar(const_cast<char *>("dateformat"), global_dateformat, sizeof(global_dateformat));
					if ( strcmp(global_dateformat,"ISO")==0)
						sprintf(recorder_timestamp,"%04d-%02d-%02d %02d:%02d:%02d.%.06d %s",rec_date_time.year,rec_date_time.month,rec_date_time.day,rec_date_time.hour,rec_date_time.minute,rec_date_time.second,rec_microseconds,rec_date_time.tz);
					else if ( strcmp(global_dateformat,"US")==0)
						sprintf(recorder_timestamp,"%02d-%02d-%04d %02d:%02d:%02d.%.06d %s",rec_date_time.month,rec_date_time.day,rec_date_time.year,rec_date_time.hour,rec_date_time.minute,rec_date_time.second,rec_microseconds,rec_date_time.tz);
					else if ( strcmp(global_dateformat,"EURO")==0)
						sprintf(recorder_timestamp,"%02d-%02d-%04d %02d:%02d:%02d.%.06d %s",rec_date_time.day,rec_date_time.month,rec_date_time.year,rec_date_time.hour,rec_date_time.minute,rec_date_time.second,rec_microseconds,rec_date_time.tz);
					else
						sprintf(recorder_timestamp,"%.09f",recorder_delta_clock);
				}
				else
					sprintf(recorder_timestamp,"%.09f",recorder_delta_clock);

				/*Deflag */
				recorder_init_items = true;
			}/*End recorder time init */

			obj = index_item->obj;
			myrec = OBJECTDATA(obj,struct recorder);

			/* See if we're in service */
			if ((obj->in_svc_double <= gl_globaldeltaclock) && (obj->out_svc_double >= gl_globaldeltaclock))
			{
				if( read_properties(myrec, obj->parent,myrec->target,value,sizeof(value)) )
				{
					if ( !myrec->ops->write(myrec, recorder_timestamp, value) )
					{
						gl_error("recorder:%d: unable to write sample to file", obj->id);
						return FAILED;
					}

					/* Update recorders so they don't "minimum timestep up" to where we exited! */
					myrec->last.ts = rec_integer_clock;
					myrec->last.ns = rec_microseconds;

					/*  Copy in the last value, just in case */
					strcpy(myrec->last.value,value);
				}
				else	/* Error state */
				{
					/* Assume message was in read_properties */
					return FAILED;
				}
			}
			/* Defaulted else - not in service */
		}//End deltamode recorders requested check
		else if (index_item->obj_type == GROUPRECORDER)
		{
			/* Extract the object */
			obj = index_item->obj;

			/* Map up function - have to do a modified version (C call), which is why this looks odd*/
			temp_fxn = (FUNCTIONADDR)(gl_get_function(obj,"obj_postupdate_fxn"));

			if (temp_fxn == nullptr)
			{
				gl_error("Unable to map group_recorder postupdate function");
				/*  TROUBLESHOOT
				While trying to map up a deltamode function for group_recorders, the function was not found.
				Please try again.  If the error persists, please submit your code and a bug report via the ticketing
				system.
				*/

				return FAILED;
			}

			/* Call the function */
			return_val = ((int (*)(OBJECT *,double))(*temp_fxn))(index_item->obj,gl_globaldeltaclock);

			/* Make sure it worked */
			if (return_val != 1)
			{
				gl_error("Failed to perform postupdate for group_recorder object");
				/*  TROUBLESHOOT
				While trying to perform the final write for a group recorder deltamode call, an error occurred.  Please
				look at the output console for more details.
				*/

				return FAILED;
			}

			/* Renull the function */
			temp_fxn = nullptr;
		}
		/* Default else - not a recorder */

		/* Update pointer */
		index_item = index_item->next;
	}/* End while loop for recorders */

	/* If any existed, do a flush */
	if (recorder_init_items)
	{
		/* flush the output streams */
		fflush(nullptr);
	}

	/* check if any players still need delta mode */
	delta_mode_needed = TS_NEVER;
	/* Initialize loop */
	index_item = delta_tape_objects;

	/* Loop through objects and look for players */
	while (index_item != nullptr)
	{
		/* See if it is a player */
		if (index_item->obj_type == PLAYER)
		{
			obj = index_item->obj;
			myplayer = OBJECTDATA(obj,struct player);

			/* See if we're in service */
			if ((obj->in_svc_double <= gl_globaldeltaclock) && (obj->out_svc_double >= gl_globaldeltaclock))
			{
				if ((( myplayer->next.ns!=0 ) && (myplayer->next.ts != t0)) || ((myplayer->next.ts != t0) && myplayer->all_events_delta))	/* See if we need to go back into deltamode, but make sure we aren't stuck! */
					enable_deltamode(myplayer->next.ts);
			}
		}
		/* Default else, not a player */

		/* update pointer */
		index_item = index_item->next;
	}/* End object loop */

	return SUCCESS;
}

extern "C" int do_kill(void*)
{
	/* if global memory needs to be released, this is the time to do it */
	return 0;
}
/**@}*/

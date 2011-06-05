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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "gridlabd.h"
#include "object.h"
#include "aggregate.h"
#include "histogram.h"

#define _TAPE_C

#include "tape.h"
#include "file.h"
#include "odbc.h"

#define MAP_DOUBLE(X,LO,HI) {#X,VT_DOUBLE,&X,LO,HI}
#define MAP_INTEGER(X,LO,HI) {#X,VT_INTEGER,&X,LO,HI}
#define MAP_STRING(X) {#X,VT_STRING,X,sizeof(X),0}
#define MAP_END {NULL}

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


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
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
#ifndef DLEXT
#define DLEXT ".so"
#else
#endif
#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#define DLSYM(H,S) dlsym(H,S)
#endif

static TAPEFUNCS *funcs = NULL;
static char1024 tape_gnuplot_path;
int32 flush_interval = 0;
int csv_data_only = 0; /* enable this option to suppress addition of lines starting with # in CSV */
int csv_keep_clean = 0; /* enable this option to keep data flushed at end of line */
void (*update_csv_data_only)(void)=NULL;
void (*update_csv_keep_clean)(void)=NULL;

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
typedef void (*VOIDCALL)(void);

TAPEFUNCS *get_ftable(char *mode){
	/* check what we've already loaded */
	char256 modname;
	TAPEFUNCS *fptr = funcs;
	TAPEOPS *ops = NULL;
	void *lib = NULL;
	CALLBACKS **c = NULL;
	char tpath[1024];
	while(fptr != NULL){
		if(strcmp(fptr->mode, mode) == 0)
			return fptr;
		fptr = fptr->next;
	}
	/* fptr = NULL */
	fptr = malloc(sizeof(TAPEFUNCS));
	if(fptr == NULL)
	{
		gl_error("get_ftable(char *mode='%s'): out of memory", mode);
		return NULL; /* out of memory */
	}
	snprintf(modname, 1024, "tape_%s" DLEXT, mode);
	
	if(gl_findfile(modname, NULL, 0|4, tpath,sizeof(tpath)) == NULL){
		gl_error("unable to locate %s", modname);
		return NULL;
	}
	lib = fptr->hLib = DLLOAD(tpath);
	if(fptr->hLib == NULL){
		gl_error("tape module: unable to load DLL for %s", modname);
		return NULL;
	}
	c = (CALLBACKS **)DLSYM(lib, "callback");
	if(c)
		*c = callback;
	//	nonfatal ommission
	ops = fptr->collector = malloc(sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_collector");
	ops->read = NULL;
	ops->write = (WRITEFUNC)DLSYM(lib, "write_collector");
	ops->rewind = NULL;
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_collector");
	ops = fptr->player = malloc(sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_player");
	ops->read = (READFUNC)DLSYM(lib, "read_player");
	ops->write = NULL;
	ops->rewind = (REWINDFUNC)DLSYM(lib, "rewind_player");
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_player");
	ops = fptr->recorder = malloc(sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_recorder");
	ops->read = NULL;
	ops->write = (WRITEFUNC)DLSYM(lib, "write_recorder");
	ops->rewind = NULL;
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_recorder");
	ops = fptr->histogram = malloc(sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_histogram");
	ops->read = NULL;
	ops->write = (WRITEFUNC)DLSYM(lib, "write_histogram");
	ops->rewind = NULL;
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_histogram");
	ops = fptr->shaper = malloc(sizeof(TAPEOPS));
	ops->open = (OPENFUNC)DLSYM(lib, "open_shaper");
	ops->read = (READFUNC)DLSYM(lib, "read_shaper");
	ops->write = NULL;
	ops->rewind = (REWINDFUNC)DLSYM(lib, "rewind_shaper");
	ops->close = (CLOSEFUNC)DLSYM(lib, "close_shaper");
	fptr->next = funcs;
	funcs = fptr;

	update_csv_data_only = (VOIDCALL)DLSYM(lib,"set_csv_data_only");
	update_csv_keep_clean = (VOIDCALL)DLSYM(lib,"set_csv_keep_clean");
	return funcs;
}

EXPORT CLASS *init(CALLBACKS *fntable, void *module, int argc, char *argv[])
{
	struct recorder my;
	struct collector my2;

	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	/* globals for the tape module*/
	sprintf(tape_gnuplot_path, "c:/Program Files/GnuPlot/bin/wgnuplot.exe");
	gl_global_create("tape::gnuplot_path",PT_char1024,&tape_gnuplot_path,NULL);
	gl_global_create("tape::flush_interval",PT_int32,&flush_interval,NULL);
	gl_global_create("tape::csv_data_only",PT_int32,&csv_data_only,NULL);
	gl_global_create("tape::csv_keep_clean",PT_int32,&csv_keep_clean,NULL);

	/* register the first class implemented, use SHARE to reveal variables */
	player_class = gl_register_class(module,"player",sizeof(struct player),PC_PRETOPDOWN); 
	player_class->trl = TRL_PROVEN;
	PUBLISH_STRUCT(player,char256,property);
	PUBLISH_STRUCT(player,char1024,file);
	PUBLISH_STRUCT(player,char8,filetype);
	PUBLISH_STRUCT(player,int32,loop);

	/* register the first class implemented, use SHARE to reveal variables */
	shaper_class = gl_register_class(module,"shaper",sizeof(struct shaper),PC_PRETOPDOWN); 
	shaper_class->trl = TRL_QUALIFIED;
	PUBLISH_STRUCT(shaper,char1024,file);
	PUBLISH_STRUCT(shaper,char8,filetype);
	PUBLISH_STRUCT(shaper,char256,group);
	PUBLISH_STRUCT(shaper,char256,property);
	PUBLISH_STRUCT(shaper,double,magnitude);
	PUBLISH_STRUCT(shaper,double,events);

	/* register the other classes as needed, */
	recorder_class = gl_register_class(module,"recorder",sizeof(struct recorder),PC_POSTTOPDOWN);
	recorder_class->trl = TRL_PROVEN;
	PUBLISH_STRUCT(recorder,char1024,property);
	PUBLISH_STRUCT(recorder,char32,trigger);
	PUBLISH_STRUCT(recorder,char1024,file);
	PUBLISH_STRUCT(recorder,char1024,multifile);
	//PUBLISH_STRUCT(recorder,int64,interval);
	PUBLISH_STRUCT(recorder,int32,limit);
	PUBLISH_STRUCT(recorder,char1024,plotcommands);
	PUBLISH_STRUCT(recorder,char32,xdata);
	PUBLISH_STRUCT(recorder,char32,columns);
	
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
			NULL) < 1)
		GL_THROW("Could not publish property output for recorder");

		/* register the other classes as needed, */
	multi_recorder_class = gl_register_class(module,"multi_recorder",sizeof(struct recorder),PC_POSTTOPDOWN);
	multi_recorder_class->trl = TRL_QUALIFIED;
	if(gl_publish_variable(multi_recorder_class,
		PT_double, "interval[s]", ((char*)&(my.dInterval) - (char *)&my),
		PT_char1024, "property", ((char*)&(my.property) - (char *)&my),
		PT_char32, "trigger", ((char*)&(my.trigger) - (char *)&my),
		PT_char1024, "file", ((char*)&(my.file) - (char *)&my),
		PT_char1024, "multifile", ((char*)&(my.multifile) - (char *)&my),
		PT_int32, "limit", ((char*)&(my.limit) - (char *)&my),
		PT_char1024, "plotcommands", ((char*)&(my.plotcommands) - (char *)&my),
		PT_char32, "xdata", ((char*)&(my.xdata) - (char *)&my),
		PT_char32, "columns", ((char*)&(my.columns) - (char *)&my),
		PT_enumeration, "output", ((char*)&(my.output) - (char *)&my),
			PT_KEYWORD, "SCREEN", SCREEN,
			PT_KEYWORD, "EPS",    EPS,
			PT_KEYWORD, "GIF",    GIF,
			PT_KEYWORD, "JPG",    JPG,
			PT_KEYWORD, "PDF",    PDF,
			PT_KEYWORD, "PNG",    PNG,
			PT_KEYWORD, "SVG",    SVG, 
			NULL) < 1)
		GL_THROW("Could not publish property output for multi_recorder");

	/* register the other classes as needed, */
	collector_class = gl_register_class(module,"collector",sizeof(struct collector),PC_POSTTOPDOWN);
	collector_class->trl = TRL_PROVEN;
	PUBLISH_STRUCT(collector,char1024,property);
	PUBLISH_STRUCT(collector,char32,trigger);
	PUBLISH_STRUCT(collector,char1024,file);
	//PUBLISH_STRUCT(collector,int64,interval);
	PUBLISH_STRUCT(collector,int32,limit);
	PUBLISH_STRUCT(collector,char256,group);
	if(gl_publish_variable(collector_class,
		PT_double, "interval[s]", ((char*)&(my2.dInterval) - (char *)&my2),
			NULL) < 1)
		GL_THROW("Could not publish property output for collector");

	/* new histogram() */
	new_histogram(module);

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
	{	OBJECT *obj=NULL;
		FINDLIST *players = gl_find_objects(FL_NEW,FT_CLASS,SAME,"tape",FT_END);
		while ((obj=gl_find_next(players,obj))!=NULL)
		{
			struct player *pData = OBJECTDATA(obj,struct player);
			if (gl_findfile(pData->file,NULL,FF_EXIST,fpath,sizeof(fpath))==NULL)
			{
				errcount++;
				gl_error("player %s (id=%d) uses the file '%s', which cannot be found", obj->name?obj->name:"(unnamed)", obj->id, pData->file);
			}
		}
	}

	/* check shapers */
	{	OBJECT *obj=NULL;
		FINDLIST *shapers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"shaper",FT_END);
		while ((obj=gl_find_next(shapers,obj))!=NULL)
		{
			struct shaper *pData = OBJECTDATA(obj,struct shaper);
			if (gl_findfile(pData->file,NULL,FF_EXIST,fpath,sizeof(fpath))==NULL)
			{
				errcount++;
				gl_error("shaper %s (id=%d) uses the file '%s', which cannot be found", obj->name?obj->name:"(unnamed)", obj->id, pData->file);
			}
		}
	}

	return errcount;
}

int do_kill()
{
	/* if global memory needs to be released, this is the time to do it */
	return 0;
}

/**@}*/

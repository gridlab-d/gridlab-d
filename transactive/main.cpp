// $Id: main.cpp 5216 2015-07-05 17:37:37Z dchassin $
//
// Transactive Control Module
//

#define DLMAIN // enables module-core linkage code in gridlabd.h

#include "transactive.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}
	new interconnection(module);
	new scheduler(module);
	new weather(module);
	new controlarea(module);
	new intertie(module);
	new generator(module);
	new load(module);
	new dispatcher(module);

	gl_global_create("transactive::nominal_frequency[Hz]",
		PT_double,&nominal_frequency, 
		PT_ACCESS,PA_PUBLIC,PT_UNITS,"Hz",
		NULL);
	gl_global_create("transactive::minimum_frequency[Hz]",
		PT_double,&minimum_frequency,PT_UNITS,"Hz",
		PT_ACCESS,PA_PUBLIC,
		NULL);
	gl_global_create("transactive::maximum_frequency[Hz]",
		PT_double,&maximum_frequency,PT_UNITS,"Hz",
		PT_ACCESS,PA_PUBLIC,
		NULL);
	gl_global_create("transactive::droop_control[pu]",
		PT_double,&droop_control,
		PT_ACCESS,PA_PUBLIC,
		NULL);
	gl_global_create("transactive::lift_control[pu]",
		PT_double,&lift_control,
		PT_ACCESS,PA_PUBLIC,
		NULL);
	gl_global_create("transactive::frequency_bounds",
		PT_enumeration,&frequency_bounds,
		PT_ACCESS,PA_PUBLIC,
			PT_KEYWORD,"NONE",FB_NONE,
			PT_KEYWORD,"SOFT",FB_SOFT,
			PT_KEYWORD,"HARD",FB_HARD,
		NULL);
	gl_global_create("transactive::solver_check",
		PT_char1024,&solver::solver_check,
		PT_ACCESS,PA_PUBLIC,
		NULL);
	gl_global_create("transactive::trace_messages",
		PT_enumeration,&trace_messages,
		PT_ACCESS,PA_PUBLIC,
			PT_KEYWORD,"NONE",TMT_NONE,
			PT_KEYWORD,"INTERCONNECTION",TMT_INTERCONNECTION,
			PT_KEYWORD,"CONTROLAREA",TMT_CONTROLAREA,
			PT_KEYWORD,"ALL",TMT_ALL,
		NULL);
	gl_global_create("transactive::verbose_options",
		PT_set,&verbose_options,
		PT_ACCESS,PA_PUBLIC,
			PT_KEYWORD,"NONE",VO_NONE,
			PT_KEYWORD,"SOLVER",VO_SOLVER,
			PT_KEYWORD,"INTERCONNECTION",VO_INTERCONNECTION,
			PT_KEYWORD,"CONTROLAREA",VO_CONTROLAREA,
			PT_KEYWORD,"INTERTIE",VO_INTERTIE,
			PT_KEYWORD,"GENERATOR",VO_GENERATOR,
			PT_KEYWORD,"LOAD",VO_LOAD,
			PT_KEYWORD,"ALL",VO_ALL,
		NULL);
	gl_global_create("transactive::allow_constraint_violations",
		PT_set,&allow_constraint_violation,
		PT_ACCESS,PA_PUBLIC,
			PT_KEYWORD,"NONE",CV_NONE,
			PT_KEYWORD,"INTERCONNECTION",CV_INTERCONNECTION,
			PT_KEYWORD,"CONTROLAREA",CV_CONTROLAREA,
			PT_KEYWORD,"INTERTIE",CV_INTERTIE,
			PT_KEYWORD,"GENERATOR",CV_GENERATOR,
			PT_KEYWORD,"LOAD",CV_LOAD,
			PT_KEYWORD,"ALL",CV_ALL,
		NULL);
	gl_global_create("transactive::schedule_interval",
		PT_double,PA_PUBLIC,PT_UNITS,"s",NULL);
	gl_global_create("transactive::dispatch_ramptime",
		PT_double,PA_PUBLIC,PT_UNITS,"s",NULL);

	/* always return the first class registered */
	return interconnection::oclass;
}


EXPORT int do_kill(void*)
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	/* if any assert objects have bad filenames, they'll fail on init() */
	return 0;
}

void trace_message(TRANSACTIVEMESSAGETYPE type, const char *msg)
{
	// override suppress repeat messages
	static gld_global repeat("suppress_repeat_messages");
	bool suppressed = repeat.get_bool();
	if ( suppressed )
		repeat.from_string("0");

	static const char *msgtype[_TMT_LAST] = {
		"",
		"interconnection",
		"controlarea    ",
	};
	if ( type>0 && type<_TMT_LAST && msgtype[type]!=NULL )
	{
		if ( (type&trace_messages)!=0 )
			gl_output("TMT[%s]: %s", msgtype[type]?msgtype[type]:"???", msg);
			/* TROUBLESHOOT
			   TODO
			*/
	}
	else 
		gl_error("TMT[%d]: %s (invalid message type)",type,msg);
		/* TROUBLESHOOT
		   TODO
		*/

	// restore suppress repeat messages
	if ( suppressed ) repeat.from_string("1");
}

EXPORT int kmldump(int (*stream)(const char *,...), OBJECT *obj)
{
	if ( obj==NULL ) // styles only
	{
#define URL "http://gridlabd.me.uvic.ca/kml/icons/transactive/"
#define ADDMARK(X) stream("    <Style id=\"%s\">\n      <IconStyle>\n        <Icon>\n          <href>"URL"%s.png</href>\n        </Icon>\n    </IconStyle>\n  </Style>\n",X,X);
#define ADDPATH(X,C,W) stream("    <Style id=\"%s\">\n      <LineStyle>\n        <color>%x</color>\n        <width>%d</width>\n      </LineStyle>\n    </Style>\n",X,C,W)
		
		ADDMARK("interconnection_mark_OK");
		ADDMARK("interconnection_mark_OVERCAPACITY");
		ADDMARK("interconnection_mark_OVERFREQUENCY");
		ADDMARK("interconnection_mark_UNDERFREQUENCY");
		ADDMARK("interconnection_mark_BLACKOUT");

		ADDMARK("controlarea_mark_OK");
		ADDMARK("controlarea_mark_OVERCAPACITY");
		ADDMARK("controlarea_mark_CONSTRAINED");
		ADDMARK("controlarea_mark_ISLAND");
		ADDMARK("controlarea_mark_UNSCHEDULED");
		ADDMARK("controlarea_mark_BLACKOUT");

		ADDMARK("intertie_mark_OFFLINE");
		ADDMARK("intertie_mark_CONSTRAINED");
		ADDMARK("intertie_mark_UNCONSTRAINED");
		ADDMARK("intertie_mark_OVERLIMIT");
		ADDMARK("intertie_mark_NOFLOW");

		ADDPATH("intertie_path_OFFLINE",0x7d000000,10); // black
		ADDPATH("intertie_path_CONSTRAINED",0x7d00ffff,10); // yellow
		ADDPATH("intertie_path_UNCONSTRAINED",0x7d00ff00,10); // green
		ADDPATH("intertie_path_OVERLIMIT",0x7d0000ff,10); // red
		ADDPATH("intertie_path_NOFLOW",0x7dff0000,10); // blue

		return 2; // means don't do class level
	}
#define DOKML(CLASS)  if ( gl_object_isa(obj,#CLASS) ) return OBJECTDATA(obj,CLASS)->kmldump(stream);
	DOKML(interconnection);
	return 0;
	DOKML(controlarea);
	DOKML(intertie);
	DOKML(load);
	DOKML(generator);
    return 0;
}

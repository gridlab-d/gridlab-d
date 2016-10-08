// $Id$

#include "transactive.h"

EXPORT_IMPLEMENT(scheduler)
EXPORT_SYNC(scheduler)
IMPLEMENT_ISA(scheduler)
IMPLEMENT_NUL(scheduler,presync,TS_NEVER)
IMPLEMENT_NUL(scheduler,postsync,TS_NEVER)

scheduler::scheduler(MODULE *module)
{
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"scheduler",sizeof(scheduler),PC_BOTTOMUP);

		if (oclass==NULL)
			exception("unable to register class");

		// publish the class properties
	if (gl_publish_variable(oclass,
		PT_double,"interval[s]",get_interval_offset(),PA_PUBLIC,PT_DESCRIPTION,"scheduling interval (s)",
		PT_double,"ramptime[s]",get_ramptime_offset(),PA_PUBLIC,PT_DESCRIPTION,"dispatcher ramp duration (s)",
		PT_enumeration,"method",get_method_offset(),PA_PUBLIC,PT_DESCRIPTION,"scheduling method",
			PT_KEYWORD,"NONE",(enumeration)SM_NONE,
			PT_KEYWORD,"PLAY",(enumeration)SM_PLAY,
			PT_KEYWORD,"MATPOWER",(enumeration)SM_MATPOWER,
		PT_set,"options",get_options_offset(),PA_PUBLIC,PT_DESCRIPTION,"scheduling options",
			PT_KEYWORD,"NONE",(set)SO_NONE,
			PT_KEYWORD,"NORAMP",(set)SO_NORAMP,
			PT_KEYWORD,"ALL",(set)SO_ALL,
		PT_double,"supply[MW]",get_supply_offset(),PA_PUBLIC,PT_DESCRIPTION,"scheduled supply (MW)",
		PT_double,"demand[MW]",get_demand_offset(),PA_PUBLIC,PT_DESCRIPTION,"scheduled demand (MW)",
		PT_double,"losses[MW]",get_losses_offset(),PA_PUBLIC,PT_DESCRIPTION,"scheduled losses (MW)",
		PT_double,"cost[$/h]",get_cost_offset(),PA_PUBLIC,PT_DESCRIPTION,"scheduled cost ($/h)",
		PT_double,"price[$/MWh]",get_price_offset(),PA_PUBLIC,PT_DESCRIPTION,"scheduled price ($/MWh)",
		NULL) < 1)
		exception("unable to publish properties");
	}
}

int scheduler::create()
{
	interval = schedule_interval;
	ramptime = dispatch_ramptime;
	return 1;
}

int scheduler::init(OBJECT *parent)
{
	// check parent
	system = get_object(parent);
	if ( !system->isa("interconnection") )
		error("parent must be an interconnection object");

	// check methods
	if ( get_method(SM_MATPOWER) )
		error("matpower scheduling not implemented");

	// TODO: check options
	
	return 1;
}

TIMESTAMP scheduler::sync(TIMESTAMP t1)
{
	TIMESTAMP dt = (TIMESTAMP)interval;
	TIMESTAMP t2 = ((t1/dt)+1)*dt;
	if ( t1==t2 )
	{
		switch ( get_method() ) {
		case SM_PLAY: schedule_play(t1);
		case SM_MATPOWER: schedule_matpower(t1);
		default: break;
		}
	}
	return t2;
}

void scheduler::schedule_play(TIMESTAMP t1)
{
	// TODO: implement played schedule method
}
void scheduler::schedule_matpower(TIMESTAMP t1)
{
	// TODO: implement matpower schedule method
}





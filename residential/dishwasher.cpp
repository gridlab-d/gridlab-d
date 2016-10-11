/** $Id: dishwasher.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dishwasher.cpp
	@addtogroup dishwasher
	@ingroup residential


 @{
 **/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "dishwasher.h"

EXPORT_CREATE(dishwasher)
EXPORT_INIT(dishwasher)
EXPORT_PRECOMMIT(dishwasher)
EXPORT_SYNC(dishwasher)
EXPORT_ISA(dishwasher)

extern "C" void decrement_queue(statemachine *machine)
{
	gld_object *obj = get_object(machine->context);
	gld_property queue(obj,"state_queue");
	double value = queue.get_double();
	queue.setp(value-1);
}
extern "C" void fill_unit(statemachine *machine)
{
	gld_object *obj = get_object(machine->context);
	gld_property gpm(obj,"hotwater_demand");
	gpm.setp(5.0);
}
extern "C" void check_fill(statemachine *machine)
{
	gld_object *obj = get_object(machine->context);
	gld_property gpm(obj,"hotwater_demand");

	// stop filling after 1 minute
	if ( gpm.get_double()>0 && machine->runtime>60 )
	{
		gld_object *obj = get_object(machine->context);
		gld_property gpm(obj,"hotwater_demand");
		gpm.setp(0.0);
	}
}
extern "C" void drain_unit(statemachine *machine)
{
	// TODO nothing here?
}
extern "C" void heat_water(statemachine *machine)
{
	// TODO turn on heater
}
extern "C" void check_water(statemachine *machine)
{
	// TODO turn off heater when water reaches setpoint
}
extern "C" void heat_air(statemachine *machine)
{
	// TODO turn on heater
}
extern "C" void check_air(statemachine *machine)
{
	// TODO turn off heater when water reaches setpoint
}

//////////////////////////////////////////////////////////////////////////
// dishwasher CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* dishwasher::oclass = NULL;
CLASS* dishwasher::pclass = NULL;
dishwasher *dishwasher::defaults = NULL;

dishwasher::dishwasher(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;
		
		// register the class definition
		oclass = gl_register_class(module,"dishwasher",sizeof(dishwasher),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_enumeration,"washmode",PADDR(washmode), PT_DESCRIPTION, "the washing mode selected",
				PT_KEYWORD, "QUICK", QUICK,
				PT_KEYWORD, "NORMAL", NORMAL,
				PT_KEYWORD, "HEAVY", HEAVY,
			PT_enumeration,"drymode",PADDR(drymode), PT_DESCRIPTION, "the drying mode selected",
				PT_KEYWORD, "AIR", AIR,
				PT_KEYWORD, "HEAT", HEAT,
			PT_enumeration, "controlmode", PADDR(controlmode), PT_DESCRIPTION, "the current state of the dishwasher",
				PT_KEYWORD, "OFF",				OFF,
				PT_KEYWORD, "CONTROLSTART",		CONTROLSTART,
				PT_KEYWORD, "PUMPPREWASH",		PUMPPREWASH,
				PT_KEYWORD, "HEATWASHQUICK",	HEATWASHQUICK,
				PT_KEYWORD, "PUMPWASHQUICK",	PUMPWASHQUICK,
				PT_KEYWORD, "HEATWASHNORMAL",	HEATWASHNORMAL,
				PT_KEYWORD, "PUMPWASHNORMAL",	PUMPWASHNORMAL,
				PT_KEYWORD, "HEATWASHHEAVY",	HEATWASHHEAVY,
				PT_KEYWORD, "PUMPWASHHEAVY",	PUMPWASHHEAVY,
				PT_KEYWORD, "CONTROLWASH",		CONTROLWASH,
				PT_KEYWORD, "PUMPRINSE",		PUMPRINSE,
				PT_KEYWORD,	"HEATRINSE",		HEATRINSE,
				PT_KEYWORD,	"CONTROLRINSE",		CONTROLRINSE,
				PT_KEYWORD, "HEATDRYON",			HEATDRYON,
				PT_KEYWORD, "HEATDRYOFF",			HEATDRYOFF,
				PT_KEYWORD, "CONTROLDRY",		CONTROLDRY,
				PT_KEYWORD, "CONTROLEND",		CONTROLEND,
			PT_double_array,"state_duration",PADDR(state_duration), PT_DESCRIPTION, "the duration of each state",
			PT_complex_array,"state_power",PADDR(state_power), PT_DESCRIPTION, "the power components of each state (row state, column ZIP component)",
			PT_double_array,"state_heatgain",PADDR(state_heatgain), PT_DESCRIPTION, "the heat gain to the house air for each state",
			PT_statemachine,"state_machine",PADDR(state_machine), PT_DESCRIPTION, "the state machine",
			PT_complex,"pump_power",PADDR(pump_power), PT_DESCRIPTION, "the power consumed by the pump when it runs",
			PT_complex,"coil_power_wet",PADDR(coil_power_wet), PT_DESCRIPTION, "the power consumed by the coil when it is on and wet",
			PT_complex,"coil_power_dry",PADDR(coil_power_dry), PT_DESCRIPTION, "the power consumed by the coil when it is on and dry",
			PT_complex,"control_power",PADDR(control_power), PT_DESCRIPTION, "the power consumed by the controller when it is on",

			PT_double,"demand_rate[unit/day]",PADDR(demand_rate), PT_DESCRIPTION, "the rate at which dishwasher loads accumulate per day",
			PT_double,"state_queue",PADDR(state_queue), PT_DESCRIPTION, "the accumulate demand (units)",

			PT_double,"hotwater_demand[gpm]", PADDR(hotwater_demand), PT_DESCRIPTION, "the rate at which hotwater is being consumed",
			PT_double,"hotwater_temperature[degF]", PADDR(hotwater_temperature), PT_DESCRIPTION, "the incoming hotwater temperature",
			PT_double,"hotwater_temperature_drop[degF]", PADDR(hotwater_temperature_drop), PT_DESCRIPTION, "the temperature drop from the plumbing bus to the dishwasher",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

dishwasher::~dishwasher()
{
}

int dishwasher::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;

	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.voltage_factor = 1.0;
	load.power_factor = 1.0;
	load.power_fraction = 1;

	controlmode = OFF;
	washmode = QUICK;
	drymode = AIR;
	demand_rate = 1.0;

	gl_warning("explicit %s model is experimental and has not been validated", OBJECTHDR(this)->oclass->name);
	/* TROUBLESHOOT
		The dishwasher explicit model has some serious issues and should be considered for complete
		removal.  It is highly suggested that this model NOT be used.
	*/

	return res;
}

int dishwasher::init(OBJECT *parent)
{
	// @todo This class has serious problems and should be deleted and started from scratch. Fuller 9/27/2013.

	OBJECT *hdr = OBJECTHDR(this);
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("dishwasher::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	hdr->flags |= OF_SKIPSAFE; // TODO is this still needed

	load.power_factor = 1.0;
	load.breaker_amps = 30;

	// defaults
	if ( pump_power.Mag()==0 ) pump_power.SetRect(250,82);
	if ( coil_power_wet.Mag()==0 ) coil_power_wet.SetRect(950,0);
	if ( coil_power_dry.Mag()==0 ) coil_power_dry.SetRect(695,0);
	if ( control_power.Mag()==0 ) control_power.SetRect(10,0);

	// count the number of states
	gld_property statelist = get_controlmode_property();
	n_states = 0;
	for ( gld_keyword *keyword=statelist.get_first_keyword() ; keyword!=NULL ; keyword=keyword->get_next() )
	{
		if ( keyword->get_value()>n_states )
			n_states = keyword->get_value();
	}
	n_states++;

	// set up the default duration array if not set
	if ( state_duration.get_rows()==0 )
	{
		state_duration.grow_to(n_states-1);
		double duration[] = {
			0, // OFF
			5, // CONTROLSTART
			15, // PUMPREWASH
			10, // HEATWASHQUICK
			30, // PUMPWASHQUICK
			10, // HEATWASHNORMAL
			30,	// PUMPWASHNORMAL
			10, // HEATWASHHEAVY
			30,	// PUMPWASHHEAVY
			20, // CONTROLWASH
			15, // PUMPRINSE
			10, // HEATRINSE
			25, // CONTROLRINSE
			23,	// HEATDRYON
			23, // HEATDRYOFF
			1, // CONTROLDRY
			5, // CONTROLEND
		};
		for ( size_t n=0 ; n<sizeof(duration)/sizeof(duration[0]) ; n++ )
			state_duration.set_at(n,0,duration[n]*60);
	}

	// check that the duration has the correct number of entries
	if ( state_duration.get_rows()!=n_states)
		exception("state_duration array does not have the same number of rows as the controlmode enumeration has possible values");
	else
		state_machine.transition_holds = &state_duration;

	// setup default state power array
	if ( state_power.get_rows()==0 && state_power.get_cols()==0 )
	{
		state_power.grow_to(n_states-1,2);
#define Z 0
#define I 1
#define P 2
		for ( size_t n=0; n<n_states ; n++ ) // each state
		{
			for ( size_t m=0 ; m<3 ; m++ ) // ZIP components
			{
				if ( n>OFF && m==P ) // on states is controller load by default
					state_power.set_at(n,m,&control_power);
				else
					state_power.set_at(n,m,complex(0,0,J));
			}
		}
		state_power.set_at(PUMPPREWASH,P,&pump_power);
		state_power.set_at(HEATWASHQUICK,Z,&coil_power_wet);
		state_power.set_at(HEATWASHNORMAL,Z,&coil_power_wet);
		state_power.set_at(HEATWASHHEAVY,Z,&coil_power_wet);
		state_power.set_at(HEATWASHQUICK,P,&pump_power);
		state_power.set_at(HEATWASHNORMAL,P,&pump_power);
		state_power.set_at(HEATWASHHEAVY,P,&pump_power);
		state_power.set_at(PUMPWASHQUICK,P,&pump_power);
		state_power.set_at(PUMPWASHNORMAL,P,&pump_power);
		state_power.set_at(PUMPWASHHEAVY,P,&pump_power);
		state_power.set_at(HEATWASHQUICK,P,&pump_power);
		state_power.set_at(HEATWASHNORMAL,P,&pump_power);
		state_power.set_at(HEATWASHHEAVY,P,&pump_power);
		state_power.set_at(PUMPRINSE,P,&pump_power);
		state_power.set_at(HEATRINSE,Z,&coil_power_wet);
		state_power.set_at(HEATRINSE,P,&coil_power_wet);
		state_power.set_at(HEATDRYON,Z,&coil_power_dry);
		state_power.set_at(HEATDRYOFF,Z,&coil_power_dry);
	}

	// check the size of the state_power array 
	if ( state_power.get_rows()!=n_states )
		exception("state_power array does not have the same number of rows as the controlmode enumeration has possible values");
	if ( state_power.get_cols()!=3 )
		exception("state_power array does not have the 3 columns for Z, I and P components ");

	// check whether the machine programmed by user
	if ( state_machine.variable==NULL ) 
	{	
		// install default program
		gld_property fsm = get_state_machine_property();
		fsm.from_string("state:controlmode");
		fsm.from_string("rule:OFF->CONTROLSTART=state_queue>1"); 
		fsm.from_string("rule:CONTROLSTART->PUMPPREWASH=$timer>state_duration#CONTROLSTART");
		fsm.from_string("rule:PUMPPREWASH->HEATWASHQUICK=$timer>state_duration#PUMPPREWASH");
		fsm.from_string("rule:HEATWASHQUICK->PUMPWASHQUICK=$timer>state_duration#HEATWASHQUICK");
		fsm.from_string("rule:PUMPWASHQUICK->CONTROLWASH=$timer>state_duration#PUMPWASHQUICK");
		fsm.from_string("rule:CONTROLWASH->HEATRINSE=$timer>state_duration#CONTROLWASH, washmode==0");

		fsm.from_string("rule:CONTROLWASH->HEATWASHNORMAL=$timer>state_duration#CONTROLWASH, washmode==1");
		fsm.from_string("rule:HEATWASHNORMAL->PUMPWASHNORMAL=$timer>state_duration#HEATWASHNORMAL");
		fsm.from_string("rule:PUMPWASHNORMAL->CONTROLWASH=$timer>state_duration#PUMPWASHNORMAL");
		fsm.from_string("rule:CONTROLWASH->HEATWASHHEAVY=$timer>state_duration#CONTROLWASH, washmode==2");
		fsm.from_string("rule:HEATWASHHEAVY->PUMPWASHHEAVY=$timer>state_duration#HEATWASHHEAVY");
		fsm.from_string("rule:PUMPWASHHEAVY->CONTROLWASH=$timer>state_duration#PUMPWASHHEAVY");

		fsm.from_string("rule:HEATRINSE->PUMPRINSE=$timer>state_duration#HEATRINSE");
		fsm.from_string("rule:PUMPRINSE->CONTROLRINSE=$timer>state_duration#PUMPRINSE");
		fsm.from_string("rule:CONTROLRINSE->OFF=drymode==0");
		// TODO change this temperature driven duty-cycle, not time-driver
		fsm.from_string("rule:CONTROLRINSE->HEATDRYON=$timer>state_duration#CONTROLRINSE");
		fsm.from_string("rule:HEATDRYON->CONTROLDRY=$timer>state_duration#HEATDRYON");
		fsm.from_string("rule:CONTROLDRY->HEATDRYOFF=$timer>state_duration#CONTROLDRY");
		fsm.from_string("rule:HEATDRYOFF->CONTROLEND=$timer>state_duration#HEATDRYOFF");
		fsm.from_string("rule:CONTROLEND->OFF=$timer>state_duration#CONTROLEND");
	}

	// setup transition default call to decrement queue
	struct {
		DISHWASHERSTATE state;
		FSMCALL call;
	} *f, map[] = {
		{OFF,				decrement_queue,	NULL,			NULL},
		{PUMPPREWASH,		fill_unit,			check_fill,		drain_unit}, // note: may use cold water
		{PUMPWASHQUICK,		fill_unit,			check_fill,		NULL},
		{HEATWASHQUICK,		heat_water,			check_water,		drain_unit},
		{PUMPWASHNORMAL,	fill_unit,			check_fill,		NULL},
		{HEATWASHNORMAL,	heat_water,			check_water,		drain_unit},
		{PUMPWASHHEAVY,		fill_unit,			check_fill,		NULL},
		{HEATWASHHEAVY,		heat_water,			check_water,		drain_unit},
		{CONTROLWASH,		NULL,				NULL,			NULL},
		{PUMPRINSE,			fill_unit,			check_fill,		NULL},
		{HEATRINSE,			heat_water,			check_water,		drain_unit},
		{CONTROLRINSE,		NULL,				NULL,			NULL},
		{HEATDRYON,			heat_air,			check_air,		NULL},
		{HEATDRYOFF,		NULL,				NULL,			NULL},
		{CONTROLDRY,		NULL,				NULL,			NULL},
		{CONTROLEND,		NULL,				NULL,			NULL},
	};
	for ( size_t n=0 ; n<sizeof(map)/sizeof(map[0]) ; n++ )
	{
		f = map+n;
		state_machine.transition_calls[f->state].entry = f->call.entry;
		state_machine.transition_calls[f->state].during = f->call.during;
		state_machine.transition_calls[f->state].exit = f->call.exit;
	}

	return residential_enduse::init(parent);
}

int dishwasher::isa(char *classname)
{
	return (strcmp(classname,"dishwasher")==0 || residential_enduse::isa(classname));
}

int dishwasher::precommit(TIMESTAMP t1)
{
	//TO DO: GPM FOR DISHWASHER
	state_queue += demand_rate * (double)(t1 - my()->clock) / 86400;
	return 1;
}

TIMESTAMP dishwasher::sync(TIMESTAMP t1) 
{
	load.admittance = state_power.get_at((size_t)controlmode,0);
	load.current = state_power.get_at((size_t)controlmode,1);
	load.power = state_power.get_at((size_t)controlmode,2);
	load.demand = load.power + (load.current + load.admittance*load.voltage_factor)*load.voltage_factor;
	load.total = load.power + load.current + load.admittance;
	load.heatgain = load.total.Mag() * load.heatgain_fraction;
	// TODO add these into variable list and document
	//total_power = (load.power.Re() + (load.current.Re() + load.admittance.Re()*load.voltage_factor)*load.voltage_factor) * 1000;
	return residential_enduse::sync(my()->clock,t1);
}

TIMESTAMP dishwasher::presync(TIMESTAMP t1)
{
	//
	return TS_NEVER; // fsm takes care of reporting next event time
}

/**@}**/
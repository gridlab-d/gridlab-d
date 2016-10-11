/** $Id: clotheswasher.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file clotheswasher.cpp
	@addtogroup clotheswasher
	@ingroup residential


 @{
 **/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "clotheswasher.h"

EXPORT_CREATE(clotheswasher)
EXPORT_INIT(clotheswasher)
EXPORT_PRECOMMIT(clotheswasher)
EXPORT_SYNC(clotheswasher)
EXPORT_ISA(clotheswasher)

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
// clotheswasher CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* clotheswasher::oclass = NULL;
CLASS* clotheswasher::pclass = NULL;
clotheswasher *clotheswasher::defaults = NULL;

clotheswasher::clotheswasher(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;
		
		// register the class definition
		oclass = gl_register_class(module,"clotheswasher",sizeof(clotheswasher),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_enumeration,"washmode",PADDR(washmode), PT_DESCRIPTION, "the washing mode selected",
				PT_KEYWORD, "QUICK", QUICK,
				PT_KEYWORD, "NORMAL", NORMAL,
				PT_KEYWORD, "HEAVY", HEAVY,
			PT_enumeration,"washtemp",PADDR(drymode), PT_DESCRIPTION, "the water temperature of wash cycle selected",
				PT_KEYWORD, "HOTWASH", HOTWASH,
				PT_KEYWORD, "WARMWASH", WARMWASH,
				PT_KEYWORD, "COLDWASH", COLDWASH,
			PT_enumeration,"rinsetemp",PADDR(drymode), PT_DESCRIPTION, "the water temperature of rinse cycle selected",
				PT_KEYWORD, "HOTPINSE", HOTRINSE,
				PT_KEYWORD, "WARMRINSE", WARMRINSE,
				PT_KEYWORD, "COLDRINSE", COLDRINSE,
			PT_enumeration, "controlmode", PADDR(controlmode), PT_DESCRIPTION, "the current state of the dishwasher",
				PT_KEYWORD, "OFF",					  OFF,
				PT_KEYWORD, "CONTROLSTART",			  CONTROLSTART,
				PT_KEYWORD, "PUMPWASHLOWHEATCOLD",	  PUMPWASHLOWHEATCOLD,
				PT_KEYWORD, "PUMPWASHLOWHEATWARM",	  PUMPWASHLOWHEATWARM,
				PT_KEYWORD, "PUMPWASHLOWHEATHOT",	  PUMPWASHLOWHEATHOT,
				PT_KEYWORD, "WASHAGITATION",		  WASHAGITATION,
				PT_KEYWORD, "PUMPWASHMEDIUM",		  PUMPWASHMEDIUM,
				PT_KEYWORD, "CONTROLRINSE",			  CONTROLRINSE,
				PT_KEYWORD, "PUMPRINSELOWHEATCOLD",	  PUMPRINSELOWHEATCOLD,
				PT_KEYWORD, "PUMPRINSELOWHEATWARM",	  PUMPRINSELOWHEATWARM,
				PT_KEYWORD, "PUMPRINSELOWHEATHOT",	  PUMPRINSELOWHEATHOT,
				PT_KEYWORD,	"RINSEAGITATION",		  RINSEAGITATION,
				PT_KEYWORD,	"PUMPRINSEMEDIUM",		  PUMPRINSEMEDIUM,
				PT_KEYWORD, "POSTRINSEAGITATION",	  POSTRINSEAGITATION,
				PT_KEYWORD, "CONTROLRINSEHIGH",		  CONTROLRINSEHIGH,
				PT_KEYWORD, "PUMPRINSEHIGH",		  PUMPRINSEHIGH,
				PT_KEYWORD, "POSTRINSEHIGHAGITATION", POSTRINSEHIGHAGITATION,
				PT_KEYWORD, "CONTROLEND",			  CONTROLEND,


			
			PT_double_array,"state_duration",PADDR(state_duration), PT_DESCRIPTION, "the duration of each state",
			PT_complex_array,"state_power",PADDR(state_power), PT_DESCRIPTION, "the power components of each state (row state, column ZIP component)",
			PT_double_array,"state_heatgain",PADDR(state_heatgain), PT_DESCRIPTION, "the heat gain to the house air for each state",
			PT_statemachine,"state_machine",PADDR(state_machine), PT_DESCRIPTION, "the state machine",
			PT_complex,"pump_power_low",PADDR(pump_power_low), PT_DESCRIPTION, "the power consumed by the pump when it runs at low speed",
			PT_complex,"pump_power_medium",PADDR(pump_power_medium), PT_DESCRIPTION, "the power consumed by the pump when it runs at medium speed",
			PT_complex,"pump_power_high",PADDR(pump_power_high), PT_DESCRIPTION, "the power consumed by the pump when it runs at high speed",
			PT_complex,"coil_power",PADDR(coil_power), PT_DESCRIPTION, "the power consumed by the coil while heating water",
			PT_complex,"control_power",PADDR(control_power), PT_DESCRIPTION, "the power consumed by the controller when it is on",

			PT_double,"demand_rate[unit/day]",PADDR(demand_rate), PT_DESCRIPTION, "the rate at which dishwasher loads accumulate per day",
			PT_double,"state_queue",PADDR(state_queue), PT_DESCRIPTION, "the accumulate demand (units)",

			PT_double,"hotwater_demand[gpm]", PADDR(hotwater_demand), PT_DESCRIPTION, "the rate at which hotwater is being consumed",
			PT_double,"hotwater_temperature[degF]", PADDR(hotwater_temperature), PT_DESCRIPTION, "the incoming hotwater temperature",
			PT_double,"hotwater_temperature_drop[degF]", PADDR(hotwater_temperature_drop), PT_DESCRIPTION, "the temperature drop from the plumbing bus to the clotheswasher",
			NULL)<1) 
			PT_double, "warmwater_demand[gpm]", PADDR(warmwater_demand), PT_DESCRIPTION, "the rate at which warmwater is being consumed",
			PT_double, "warmwater_temperature[degF]", PADDR(warmwater_temperature), PT_DESCRIPTION, "the incoming warm water temperature"
			PT_double, "warmwater_temperature_drop[degF]", PADDR(warmwater_temperature_drop). PT_DESCRIPTION, "the temperature drop from the plumbing bus to the clotheswasher"
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

clotheswasher::~clotheswasher()
{
}

int clotheswasher::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;

	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.voltage_factor = 1.0;
	load.power_factor = 1.0;
	load.power_fraction = 1;

	controlmode = OFF;
	washmode = HEAVY;
	washtemp = WARM;
	rinsetemp = WARM;
	demand_rate = 1.0;

	gl_warning("explicit %s model is experimental and has not been validated", OBJECTHDR(this)->oclass->name);
	/* TROUBLESHOOT
		The dishwasher explicit model has some serious issues and should be considered for complete
		removal.  It is highly suggested that this model NOT be used.
	*/

	return res;
}

int clotheswasher::init(OBJECT *parent)
{
	// @todo This class has serious problems and should be deleted and started from scratch. Fuller 9/27/2013.

	OBJECT *hdr = OBJECTHDR(this);
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("clotheswasher::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	hdr->flags |= OF_SKIPSAFE; // TODO is this still needed

	load.power_factor = 1.0;
	load.breaker_amps = 30;

	// defaults // to fix - values
	if ( pump_power_low.Mag()==0 ) pump_power.SetRect(150,82);
	if ( pump_power_medium.Mag()==0 ) pump_power.SetRect(200,82);
	if ( pump_power_high.Mag()==0 ) pump_power.SetRect(250,82);
	if ( coil_power.Mag()==0 ) coil_power.SetRect(950,0);
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
			0,	// OFF
			5,	// CONTROLSTART
			15, // PUMPWASHLOWHEATCOLD
			20, // PUMPWASHLOWHEATWARM
			25, // PUMPWASHLOWHEATHOT
			5,	// WASHAGITATION
			10,	// PUMPWASHMEDIUMQUICK
			20,	// PUMPWASHMEDIUMNORMAL
			30,	// PUMPWASHMEDIUMHEAVY
			5,	// CONTROLRINSE
			10,	// PUMPRINSELOWHEATCOLD
			20, // PUMPRINSELOWHEATWARM
			25, // PUMPRINSELOWHEATHOT
			5,	// RINSEAGITATION
			15, // PUMPRINSEMEDIUM
			5,	// POSTRINSEAGITATION
			5,	// CONTROLRINSEHIGH
			10, // PUMPRINSEHIGH
			5,	// POSTRINSEHIGHAGITATION
			5,	// CONTROLEND
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
		state_power.set_at(PUMPWASHLOWHEATCOLD,P,&pump_power_low);
		state_power.set_at(PUMPWASHLOWHEATWARM,P,&pump_power_low);
		state_power.set_at(PUMPWASHLOWHEATHOT,P,&pump_power_low);
		state_power.set_at(WASHAGITATION,P,&pump_power_low);
		state_power.set_at(PUMPWASHMEDIUM,P,&pump_power_medium);
		state_power.set_at(PUMPRINSELOWHEATCOLD,P,&pump_power_low);
		state_power.set_at(PUMPRINSELOWHEATWARM,P,&pump_power_low);
		state_power.set_at(PUMPRINSELOWHEATHOT,P,&pump_power_low);
		state_power.set_at(RINSEAGITATION,P,&pump_power_low);
		state_power.set_at(PUMPRINSEMEDIUM,P,&pump_power_medium);
		state_power.set_at(POSTRINSEAGITATION,P,&pump_power_low);
		state_power.set_at(PUMPRINSEHIGH,P,&pump_power_high);
		state_power.set_at(POSTRINSEHIGHAGITATION,P,&pump_power_low);
		state_power.set_at(PUMPWASHLOWHEATWARM,Z,&coil_power);
		state_power.set_at(PUMPWASHLOWHEATHOT,Z,&coil_power);
		state_power.set_at(WASHAGITATION,Z,&coil_power);
		state_power.set_at(PUMPRINSELOWHEATWARM,Z,&coil_power);
		state_power.set_at(PUMPRINSELOWHEATHOT,Z,&coil_power);
		state_power.set_at(RINSEAGITATION,Z,&coil_power);
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
		fsm.from_string("rule:CONTROLSTART->PUMPWASHLOWHEATCOLD=$timer>state_duration#CONTROLSTART, washtemp==2");
		fsm.from_string("rule:PUMPWASHLOWHEATCOLD->WASHAGITATION=$timer>state_duration#PUMPWASHLOWHEATCOLD");
		fsm.from_string("rule:CONTROLSTART->PUMPWASHLOWHEATWARM=$timer>state_duration#CONTROLSTART, washtemp==1");
		fsm.from_string("rule:PUMPWASHLOWHEATWARM->WASHAGITATION=$timer>state_duration#PUMPWASHLOWHEATWARM");
		fsm.from_string("rule:CONTROLSTART->PUMPWASHLOWHEATHOT=$timer>state_duration#CONTROLSTART, washtemp==0");
		fsm.from_string("rule:PUMPWASHLOWHEATHOT->WASHAGITATION=$timer>state_duration#PUMPWASHLOWHEATWARM");

		fsm.from_string("rule:WASHAGITATION->PUMPWASHMEDIUM=$timer>state_duration#WASHAGITATION");
		fsm.from_string("rule:PUMPWASHMEDIUM->CONTROLRINSE=$timer>state_duration#PUMPWASHMEDIUMQUICK, washmode=0");
		fsm.from_string("rule:PUMPWASHMEDIUM->CONTROLRINSE=$timer>state_duration#PUMPWASHMEDIUMNORMAL, washmode=1");
		fsm.from_string("rule:PUMPWASHMEDIUM->CONTROLRINSE=$timer>state_duration#PUMPWASHMEDIUMHEAVY, washmode=2");
		fsm.from_string("rule:CONTROLRINSE->PUMPRINSELOWHEATCOLD=$timer>state_duration#CONTROLRINSE, rinsetemp==2");
		fsm.from_string("rule:PUMPRINSELOWHEATCOLD->RINSEAGITATION=$timer>state_duration#PUMPRINSELOWHEATCOLD");
		fsm.from_string("rule:CONTROLRINSE->PUMPRINSELOWHEATWARM=$timer>state_duration#CONTROLRINSE, rinsetemp==1");
		fsm.from_string("rule:PUMPRINSELOWHEATWARM->RINSEAGITATION=$timer>state_duration#PUMPRINSELOWHEATWARM");
		fsm.from_string("rule:CONTROLRINSE->PUMPRINSELOWHEATHOT=$timer>state_duration#CONTROLRINSE, rinsetemp==0");
		fsm.from_string("rule:PUMPRINSEHEATHOT->RINSEAGITATION=$timer>state_duration#PUMPRINSEHEATHOT");

		fsm.from_string("rule:RINSEAGITATION->PUMPRINSEMEDIUM=$timer>state_duration#RINSEAGITATION");
		fsm.from_string("rule:PUMPRINSEMEDIUM->POSTRINSEAGITATION=$timer>state_duration#PUMPRINSEMEDIUM");
		fsm.from_string("rule:POSTRINSEAGITATION->CONTROLRINSEHIGH=$timer>state_duration#POSTRINSEAGITATION, washmode==2");
		fsm.from_string("rule:CONTROLRINSEHIGH->PUMPRINSEHIGH=$timer>state_duration#CONTROLRINSEHIGH");
		fsm.from_string("rule:PUMPRINSEHIGH->POSTRINSEHIGHAGITATION=$timer>state_duration#PUMPRINSEHIGH");
		fsm.from_string("rule:POSTRINSEHIGHAGITATION->OFF=$timer>state_duration#POSTRINSEHIGHAGITATION");

		fsm.from_string("rule:POSTRINSEAGITATION->CONTROLEND=$timer>state_duration#POSTRINSEAGITATION, washmode==1");
		fsm.from_string("rule:POSTRINSEAGITATION->CONTROLEND=$timer>state_duration#POSTRINSEAGITATION, washmode==0");
		fsm.from_string("rule:CONTROLEND->OFF=$timer>state_duration#CONTROLEND");
		// TODO change this temperature driven duty-cycle, not time-driver


	}

	// setup transition default call to decrement queue
	struct {
		CLOTHESWASHERSTATE state;
		FSMCALL call;
	} *f, map[] = {
		{OFF,					decrement_queue,	NULL,			NULL},
		{CONTROLSTART			NULL,				NULL,			NULL},
		{PUMPWASHLOWHEATCOLD,	fill_unit,			check_fill,		NULL}, // note: may use cold water
		{PUMPWASHLOWHEATWARM,	fill_unit,			check_fill, 	NULL},
		{PUMPWASHLOWHEATHOT,	fill_unit,			check_fill,		NULL},
		{WASHAGITATION,			heat_unit,			check_water,	NULL},
		{PUMPWASHMEDIUM,		NULL,				NULL,			drain_unit},
		{CONTROLRINSE,			NULL,				NULL,			NULL},
		{PUMPRINSELOWHEATCOLD,	fill_unit,			check_fill,		NULL},
		{PUMPRINSELOWHEATWARM,	fill_unit,			check_fill,		NULL},
		{PUMPRINSELOWHEATHOT,	fill_unit,			check_fill,		NULL},
		{RINSEAGITATION,		heat_water,			check_water,	NULL},
		{PUMPRINSEMEDIUM,		NULL,				NULL,			drain_unit},
		{POSTRINSEAGITATION,	NULL,				NULL,			drain_unit},
		{CONTROLRINSEHIGH,		NULL,				NULL,			NULL},
		{PUMPRINSEHIGH,			NULL,				NULL,			drain_unit},
		{POSTRINSEHIGHAGITATION,NULL,				NULL,			drain_unit},
		{CONTROLEND,			NULL,				NULL,			NULL},
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

int clotheswasher::isa(char *classname)
{
	return (strcmp(classname,"clotheswasher")==0 || residential_enduse::isa(classname));
}

int clotheswasher::precommit(TIMESTAMP t1)
{
	//TO DO: GPM FOR CLOTHESWASHER
	state_queue += demand_rate * (double)(t1 - my()->clock) / 86400;
	return 1;
}

TIMESTAMP clotheswasher::sync(TIMESTAMP t1) 
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

TIMESTAMP clotheswasher::presync(TIMESTAMP t1)
{
	//
	return TS_NEVER; // fsm takes care of reporting next event time
}

/**@}**/
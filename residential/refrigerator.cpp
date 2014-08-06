/** $Id: refrigerator.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file refrigerator.cpp
	@addtogroup refrigerator
	@ingroup residential

	The refrigerator model is based on performance profile that can specified by a tape and using 
	a method developed by Ross T. Guttromson
	
	Original ODE:
	
	\f$ \frac{C_f}{UA_r + UA_f} \frac{dT_{air}}{dt} + T_{air} = T_{out} + \frac{Q_r}{UA_r} \f$ 
	
	 where

		 \f$ T_{air} \f$ is the temperature of the water

		 \f$ T_{out} \f$ is the ambient temperature around the refrigerator

		 \f$ UA_r \f$ is the UA of the refrigerator itself

		 \f$ UA_f \f$ is the UA of the food-air

		 \f$ C_f \f$ is the heat capacity of the food

		\f$ Q_r \f$ is the heat rate from the cooling system


	 General form:

	 \f$ T_t = (T_o - C_2)e^{\frac{-t}{C_1}} + C_2 \f$ 
	
	where

		t is the elapsed time

		\f$ T_o \f$ is the initial temperature

		\f$ T_t\f$  is the temperature at time t

		\f$ C_1 = \frac{C_f}{UA_r + UA_f} \f$ 

		\f$ C_2 = T_out + \frac{Q_r}{UA_f} \f$ 
	
	Time solution
	
		\f$ t = -ln\frac{T_t - C_2}{T_o - C_2}*C_1 \f$ 
	
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "refrigerator.h"

//////////////////////////////////////////////////////////////////////////
// underground_line_conductor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* refrigerator::oclass = NULL;
CLASS *refrigerator::pclass = NULL;

refrigerator::refrigerator(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass == NULL)
	{
		pclass = residential_enduse::oclass;

		// register the class definition
		oclass = gl_register_class(module,"refrigerator",sizeof(refrigerator),PC_PRETOPDOWN | PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The file that implements the lights in the residential module cannot register the class.
				This is an internal error.  Contact support for assistance.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double, "size[cf]", PADDR(size),PT_DESCRIPTION,"volume of the refrigerator",
			PT_double, "rated_capacity[Btu/h]", PADDR(rated_capacity),
			PT_double,"temperature[degF]",PADDR(Tair),
			PT_double,"setpoint[degF]",PADDR(Tset),
			PT_double,"deadband[degF]",PADDR(thermostat_deadband),
			PT_double,"cycle_time[s]",PADDR(cycle_time),
			PT_double,"output",PADDR(Qr),
			PT_double,"event_temp",PADDR(Tevent),
			PT_double,"UA[Btu/degF*h]",PADDR(UA),					
			PT_double,"compressor_off_normal_energy",PADDR(compressor_off_normal_energy),
			PT_double,"compressor_off_normal_power[W]",PADDR(compressor_off_normal_power),
			PT_double,"compressor_on_normal_energy",PADDR(compressor_on_normal_energy),
			PT_double,"compressor_on_normal_power[W]",PADDR(compressor_on_normal_power),
			PT_double,"defrost_energy",PADDR(defrost_energy),
			PT_double,"defrost_power[W]",PADDR(defrost_power),
			PT_double,"icemaking_energy",PADDR(icemaking_energy),
			PT_double,"icemaking_power[W]",PADDR(icemaking_power),
			PT_double,"ice_making_probability",PADDR(ice_making_probability),	
			PT_int32,"FF_Door_Openings",PADDR(FF_Door_Openings),
			PT_int32,"door_opening_energy",PADDR(door_opening_energy),
			PT_int32,"door_opening_power",PADDR(door_opening_power),
			PT_double,"DO_Thershold",PADDR(DO_Thershold),
			PT_double,"dr_mode_double",PADDR(dr_mode_double),
			PT_double,"energy_needed",PADDR(energy_needed),	
			PT_double,"energy_used",PADDR(energy_used),	
			PT_double,"refrigerator_power",PADDR(refrigerator_power),		
			PT_bool,"icemaker_running",PADDR(icemaker_running),	
			PT_int32,"check_DO",PADDR(check_DO),
			PT_bool,"is_240",PADDR(is_240),
			PT_double,"defrostDelayed",PADDR(defrostDelayed),
			PT_bool,"long_compressor_cycle_due",PADDR(long_compressor_cycle_due),
			PT_double,"long_compressor_cycle_time",PADDR(long_compressor_cycle_time),
			PT_double,"long_compressor_cycle_power",PADDR(long_compressor_cycle_power),
			PT_double,"long_compressor_cycle_energy",PADDR(long_compressor_cycle_energy),
			PT_double,"long_compressor_cycle_threshold",PADDR(long_compressor_cycle_threshold),
			PT_enumeration,"defrost_criterion",PADDR(defrost_criterion),
				PT_KEYWORD,"TIMED",(enumeration)DC_TIMED,	
				PT_KEYWORD,"DOOR_OPENINGS",(enumeration)DC_DOOR_OPENINGS,				
				PT_KEYWORD,"COMPRESSOR_TIME",(enumeration)DC_COMPRESSOR_TIME,
			PT_bool,"run_defrost",PADDR(run_defrost),
			PT_double,"door_opening_criterion",PADDR(door_opening_criterion),	
			PT_double,"compressor_defrost_time",PADDR(compressor_defrost_time),
			PT_double,"delay_defrost_time",PADDR(delay_defrost_time),
			PT_int32,"daily_door_opening",PADDR(daily_door_opening),			
			PT_enumeration,"state",PADDR(state),
				PT_KEYWORD,"DEFROST",(enumeration)RS_DEFROST,	
				PT_KEYWORD,"COMPRESSSOR_OFF_NORMAL",(enumeration)RS_COMPRESSSOR_OFF_NORMAL,				
				PT_KEYWORD,"COMPRESSSOR_ON_LONG",(enumeration)RS_COMPRESSSOR_ON_LONG,
				PT_KEYWORD,"COMPRESSSOR_ON_NORMAL",(enumeration)RS_COMPRESSSOR_ON_NORMAL,
			NULL) < 1)
			GL_THROW("unable to publish properties in %s", __FILE__);
	}
}

int refrigerator::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;

	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.voltage_factor = 1.0;
	load.power_factor = 0.95;
	load.power_fraction = 1;
	is_240 = true;	

	gl_warning("explicit %s model is experimental", OBJECTHDR(this)->oclass->name);
	/* TROUBLESHOOT
		The refrigerator explicit model has some serious issues and should be considered for complete
		removal.  It is highly suggested that this model NOT be used.
	*/

	return res;
}

int refrigerator::init(OBJECT *parent)
{

	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("refrigerator::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	// defaults for unset values */
	if (size==0)				size = gl_random_uniform(&hdr->rng_state,20,40); // cf
	if (thermostat_deadband==0) thermostat_deadband = gl_random_uniform(&hdr->rng_state,2,3);
	if (Tset==0)				Tset = gl_random_uniform(&hdr->rng_state,35,39);
	if (UA == 0)				UA = 0.6;
	if (UAr==0)					UAr = UA+size/40*gl_random_uniform(&hdr->rng_state,0.9,1.1);
	if (UAf==0)					UAf = gl_random_uniform(&hdr->rng_state,0.9,1.1);
	if (COPcoef==0)				COPcoef = gl_random_uniform(&hdr->rng_state,0.9,1.1);
	if (Tout==0)				Tout = 59.0;
	if (load.power_factor==0)		load.power_factor = 0.95;

	pTout = (double*)gl_get_addr(parent, "air_temperature");
	if (pTout==NULL)
	{
		static double default_air_temperature = 72;
		gl_warning("%s (%s:%d) parent object lacks air temperature, using %0f degF instead", hdr->name, hdr->oclass->name, hdr->id, default_air_temperature);
		pTout = &default_air_temperature;
	}

	/* derived values */
	Tair = gl_random_uniform(&hdr->rng_state,Tset-thermostat_deadband/2, Tset+thermostat_deadband/2);

	// size is used to couple Cw and Qrated
	Cf = size/10.0 * RHOWATER * CWATER;  // cf * lb/cf * BTU/lb/degF = BTU / degF

	rated_capacity = BTUPHPW * size*10; // BTU/h ... 10 BTU.h / cf (34W/cf, so ~700 for a full-sized refrigerator)

	start_time = 0;

	if(compressor_off_normal_energy==0) compressor_off_normal_energy=15*45*60; //watt-secs
	if(compressor_off_normal_power==0) compressor_off_normal_power=15; //watt

	if(long_compressor_cycle_energy==0) long_compressor_cycle_energy=120*100*60; //watt-secs
	if(long_compressor_cycle_power==0) long_compressor_cycle_power=120; //watt

	if(compressor_on_normal_energy==0) compressor_on_normal_energy=120*35*60; //watt-secs
	if(compressor_on_normal_power==0) compressor_on_normal_power=120; //watt

	if(defrost_energy==0) defrost_energy=40*550*60; //watt-secs
	if(defrost_power==0) defrost_power=550; //watt

	if(icemaking_energy==0) icemaking_energy=300*60; //watt-secs
	if(icemaking_power==0) icemaking_power=300; //watt

	if(ice_making_probability==0) ice_making_probability=0.02; //watt
	
	if(DO_Thershold==0) DO_Thershold=24; 	
	if(long_compressor_cycle_threshold==0) long_compressor_cycle_threshold=0.05;

	if(FF_Door_Openings==0) FF_Door_Openings=0;

	if(door_opening_power==0) door_opening_power=16;

	if(delay_defrost_time==0) delay_defrost_time=28800;	

	if(defrost_criterion==0) defrost_criterion=DC_TIMED;	
	
	refrigerator_power = 0;

	return_time = 0;

	no_of_defrost = 0;

	total_compressor_time = 0;

	if(door_open_time==0) door_open_time=7;
	
	long_compressor_cycle_due=false;
	door_energy_calc = false;

	ice_making_time = new double[1,2,3]; 

	icemaker_running = false;
	check_defrost = false;

	switch(state){
		case RS_DEFROST:
			if(energy_needed==0) energy_needed = defrost_energy;
			cycle_time = ceil((energy_needed - energy_used)/defrost_power);
		break;
		case RS_COMPRESSSOR_OFF_NORMAL:
			if(energy_needed==0) energy_needed = compressor_off_normal_energy;
			cycle_time = ceil((energy_needed - energy_used)/compressor_off_normal_power);
		break;		
		case RS_COMPRESSSOR_ON_NORMAL:
			if(energy_needed==0) energy_needed = compressor_on_normal_energy;
			cycle_time = ceil((energy_needed - energy_used)/compressor_on_normal_power);
		break;		
	}

	run_defrost = false;

	if (is_240)
	{
		load.config = EUC_IS220;
	}

	load.total = Qr * KWPBTUPH;

	return residential_enduse::init(parent);
}

int refrigerator::isa(char *classname)
{
	return (strcmp(classname,"refrigerator")==0 || residential_enduse::isa(classname));
}

TIMESTAMP refrigerator::presync(TIMESTAMP t0, TIMESTAMP t1){

	OBJECT *hdr = OBJECTHDR(this);

	if(start_time==0)
	{
		start_time = int32(t0);
		DO_random_opening = int32(gl_random_uniform(&hdr->rng_state,0,1800));
	}	

	return TS_NEVER;
}

/* occurs between presync and sync */
/* exclusively modifies Tevent and motor_state, nothing the reflects current properties
 * should be affected by the PLC code. */
void refrigerator::thermostat(TIMESTAMP t0, TIMESTAMP t1){

}

TIMESTAMP refrigerator::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
		double dt0 = gl_toseconds(t0>0?t1-t0:0);

		// if advancing from a non-init condition
		if (t0>TS_ZERO && t1>t0)
		{
			// compute the total energy usage in this interval
			load.energy += load.total * dt0/3600.0;
		
		}
			
		double dt1 = update_refrigerator_state(dt0, t1);

		return dt1>0?-(TIMESTAMP)(dt1*TS_SECOND+t1):TS_NEVER; 	

}

TIMESTAMP refrigerator::postsync(TIMESTAMP t0, TIMESTAMP t1){
	
	return TS_NEVER;

}

double refrigerator::update_refrigerator_state(double dt0,TIMESTAMP t1)
{
	OBJECT *hdr = OBJECTHDR(this);
	
	// accumulate the energy
	energy_used += refrigerator_power*dt0;

	if(cycle_time>0)
	{
		cycle_time -= dt0;
	}	

	if(defrostDelayed>0)
	{
		defrostDelayed -= dt0;
	}	

	if(defrostDelayed<=0){
		if(DC_TIMED==defrost_criterion){
			run_defrost = true;
		}
		else if(DC_COMPRESSOR_TIME==defrost_criterion && total_compressor_time>=compressor_defrost_time){
			
			run_defrost = true;
			total_compressor_time = 0;
			check_defrost = false;
			
		}
		else if(no_of_defrost>0 && DC_DOOR_OPENINGS==defrost_criterion){

			run_defrost = true;
			FF_Door_Openings = 0;
			check_defrost = false;
		}
	}
	else{
		if(DC_TIMED==defrost_criterion){
			check_defrost = true;
		}
		else if(DC_COMPRESSOR_TIME==defrost_criterion && total_compressor_time>=compressor_defrost_time){
			check_defrost = true;
			
		}
		else if(no_of_defrost>0 && DC_DOOR_OPENINGS==defrost_criterion){
			check_defrost = true;
		}
	}

	
	if(long_compressor_cycle_time>0)
	{
		long_compressor_cycle_time -= dt0;
	}	

	if (dr_mode_double == 0)
		dr_mode = DM_UNKNOWN;
	else if (dr_mode_double == 1)
		dr_mode = DM_LOW;
	else if (dr_mode_double == 2)
		dr_mode = DM_NORMAL;
	else if (dr_mode_double == 3)
		dr_mode = DM_HIGH;
	else if (dr_mode_double == 4)
		dr_mode = DM_CRITICAL;
	else
		dr_mode = DM_UNKNOWN;	

	if(door_return_time > 0){
		door_return_time = door_return_time - dt0;
	}
		
	if(door_next_open_time > 0){
		door_next_open_time = door_next_open_time - dt0;
	}

	if(door_return_time<=0 && true==door_open){
		door_return_time = 0;
		door_open = false;				
	}

	if(door_next_open_time<=0 && hourly_door_opening>0){
		door_next_open_time = 0;
		door_open = true;
		FF_Door_Openings++;
		door_return_time += ceil(gl_random_uniform(&hdr->rng_state,0, door_open_time));
		door_energy_calc = true;
		hourly_door_opening--;	
		
		if(hourly_door_opening > 0){
			door_next_open_time = int(gl_random_uniform(&hdr->rng_state,0,(3600-(t1-start_time)%3600))); // next_door_openings[hourly_door_opening-1] - ((t1-start_time)%3600);	
			door_to_open = true;
		}
		else{
			door_to_open = false;
		}	

		check_DO = FF_Door_Openings%DO_Thershold;

		if(check_DO==0){ 	
			no_of_defrost++;
			
		}		
	}


	if(((t1-start_time)%3600)==0 && start_time>0)
	{
		double temp_hourly_door_opening = (door_opening_criterion*daily_door_opening) - floor(door_opening_criterion*daily_door_opening);
		if(temp_hourly_door_opening >= 0.5){
			 hourly_door_opening = ceil(door_opening_criterion*daily_door_opening);					 
		}
		else{
			 hourly_door_opening = floor(door_opening_criterion*daily_door_opening);
		}

		hourly_door_opening = floor(gl_random_normal(&hdr->rng_state,hourly_door_opening, ((hourly_door_opening)/3)));
			
		//Clip the hourly door openings
		if(hourly_door_opening<0){
			hourly_door_opening = 0;
		}
		else if(hourly_door_opening>2*hourly_door_opening){
			hourly_door_opening = 2*hourly_door_opening;
		}

		if(hourly_door_opening > long_compressor_cycle_threshold*daily_door_opening)
		{
			long_compressor_cycle_due = true;
			long_compressor_cycle_time = int(gl_random_uniform(&hdr->rng_state,0,(3600)));	
		}

		if(hourly_door_opening>0){	
			door_to_open = true;
			door_next_open_time = int(gl_random_uniform(&hdr->rng_state,0,(3600)));			
		}
		else{
			door_to_open = false;
		}
	}
	
	double dt1 = 0;
	
	switch(state){

		case RS_DEFROST:
			
			if ((energy_used >= energy_needed || cycle_time <= 0))
			{				
				state = RS_COMPRESSSOR_ON_LONG;
				refrigerator_power = long_compressor_cycle_power;
				energy_needed = long_compressor_cycle_energy;
				energy_used = 0;

				cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);							
			}			
			else
			{
				refrigerator_power = defrost_power;
			}

		break;

		case RS_COMPRESSSOR_ON_NORMAL:	

			if(run_defrost==true)
			{
				state = RS_DEFROST;
				refrigerator_power = defrost_power;
				energy_needed = defrost_energy;
				energy_used = 0;

				cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);

				run_defrost=false;
				no_of_defrost--;

				defrostDelayed = delay_defrost_time;
								
			}
			else if ((energy_used >= energy_needed || cycle_time <= 0))
			{						
				state = RS_COMPRESSSOR_OFF_NORMAL;
				refrigerator_power = compressor_off_normal_power;
				energy_needed = compressor_off_normal_energy;				 		
				
				energy_used = 0;
				cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);	
				//new_running_state = true;
			}			
			else
			{
				refrigerator_power = compressor_on_normal_power;
				total_compressor_time += dt0;
				if(door_energy_calc==true){
					door_energy_calc = false;

					energy_needed += door_return_time*refrigerator_power;
					cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);
				
				}
			}

		break;

		case RS_COMPRESSSOR_ON_LONG:	

			if(run_defrost==true)
			{
				state = RS_DEFROST;
				refrigerator_power = defrost_power;
				energy_needed = defrost_energy;
				energy_used = 0;

				cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);

				run_defrost=false;
				no_of_defrost--;

				defrostDelayed = delay_defrost_time;
								
			}
			else if ((energy_used >= energy_needed || cycle_time <= 0))
			{						
				state = RS_COMPRESSSOR_OFF_NORMAL;
				refrigerator_power = compressor_off_normal_power;
				energy_needed = compressor_off_normal_energy;				 		
				
				energy_used = 0;
				cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);	
				//new_running_state = true;
			}			
			else
			{
				refrigerator_power = long_compressor_cycle_power;		
				if(door_energy_calc==true){
					door_energy_calc = false;

					energy_needed += door_return_time*refrigerator_power;
					cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);
				
				}
			}

		break;

		case RS_COMPRESSSOR_OFF_NORMAL:

			if(run_defrost==true)
			{
				state = RS_DEFROST;
				refrigerator_power = defrost_power;
				energy_needed = defrost_energy;
				energy_used = 0;

				cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);

				run_defrost=false;
				no_of_defrost--;

				defrostDelayed = delay_defrost_time;
								
			}
			else if ((energy_used >= energy_needed || cycle_time <= 0))
			{										
				if(long_compressor_cycle_due==true && long_compressor_cycle_time<=0)
				{
					state = RS_COMPRESSSOR_ON_LONG;
					refrigerator_power = long_compressor_cycle_power;
					energy_needed = long_compressor_cycle_energy;

					long_compressor_cycle_due=false;
					
					energy_used = 0;
					cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);		
				}
				else{
					
					state = RS_COMPRESSSOR_ON_NORMAL;
					refrigerator_power = compressor_on_normal_power;
					energy_needed = compressor_on_normal_energy;	
										
					energy_used = 0;
					cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);
				}
			}			
			else
			{				
				refrigerator_power = compressor_off_normal_power;	

				if(door_energy_calc==true){
					door_energy_calc = false;

					energy_needed -= door_return_time*compressor_off_normal_power;
					cycle_time = ceil((energy_needed - energy_used)/refrigerator_power);
					
					if(cycle_time<0)
						cycle_time = 0;
				}
			}

		break;		
		
	}

	double posted_power = refrigerator_power;

	//***************************
	if(door_open==true)
	{
		posted_power += door_opening_power;
	}
	//***************************

	if(check_icemaking == true)
	{
		posted_power += icemaking_power;
		icemaker_running = true;

		return_time = 60-dt0;

		if(0==((t1-start_time)%60) || return_time<=0){			
			
			ice_making_no--;
			
			if(ice_making_no == 0)
			{
				check_icemaking = false;					
			}

			return_time = 60;
		}		
		
	}
	else
	{
		if(((t1-start_time)%60)==0) 
		{
			ice_making = double(gl_random_uniform(&hdr->rng_state,0,1));			

			if(ice_making <=ice_making_probability)
			{			
				check_icemaking = true;

				icemaker_running = true;

			//	ice_making_no = gl_random_sampled(3,ice_making_time);

				ice_making_no = 1;

				posted_power += icemaking_power;
				return_time = 60;			
				ice_making_no--;	

				if(ice_making_no == 0)
				{
					check_icemaking = false;
				}
			}
			else
			{
				icemaker_running = false;
				return_time = 60;	
			}
		}	
		else
		{
			icemaker_running = false;

		}
	}	
	
	load.power.SetPowerFactor(posted_power/1000, load.power_factor);
	load.admittance = complex(0,0,J); 
	load.current = complex(0,0,J);		

	if(true==door_open && true==door_to_open)
	{
		door_time = door_return_time<door_next_open_time?door_return_time:door_next_open_time;
	}
	else if(true==door_to_open)
	{
		door_time = door_next_open_time;
	}
	else if(true==door_open){
		
		door_time = door_return_time;

	}
	else if(start_time>0)
	{
		door_time = (3600 - ((t1-start_time)%3600));
	}

	if(0.0==return_time)
	{
		dt1 = cycle_time>door_time?door_time:cycle_time;		
	}
	else
	{
		if(cycle_time>return_time)
			if(return_time>door_time)
				if(door_time>long_compressor_cycle_time)
					dt1 = long_compressor_cycle_time;
				else
					dt1 = door_time;					
			else 
				if(return_time>long_compressor_cycle_time)
					dt1 = long_compressor_cycle_time;
				else
					dt1 = return_time;
		else
			if(cycle_time>door_time)
				if(door_time>long_compressor_cycle_time)
					dt1 = long_compressor_cycle_time;
				else
					dt1 = door_time;
			else
				if(cycle_time>long_compressor_cycle_time)
					dt1 = long_compressor_cycle_time;
				else
					dt1 = cycle_time;		 
	}
	if(check_defrost == true){
		if(dt1 > defrostDelayed){
			dt1 = defrostDelayed;
		}
	}


	load.total = load.power + load.current + load.admittance;
	total_power = (load.power.Re() + (load.current.Re() + load.admittance.Re()*load.voltage_factor)*load.voltage_factor);

	last_dr_mode = dr_mode;

	if ((dt1 > 0) && (dt1 < 1)){
		dt1 = 1;
	}

	return dt1;

}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
EXPORT int create_refrigerator(OBJECT **obj, OBJECT *parent)
{
	
	*obj = gl_create_object(refrigerator::oclass);
	if (*obj!=NULL)
	{
		refrigerator *my = OBJECTDATA(*obj,refrigerator);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_refrigerator(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass,  TIMESTAMP t1)
{
	refrigerator *my = OBJECTDATA(obj,refrigerator);
	TIMESTAMP next_time = TS_NEVER;

	// obj->clock = 0 is legit

	try {
		switch (pass) 
		{
			case PC_PRETOPDOWN:
				t1 = my->presync(obj->clock, t0);
				break;

			case PC_BOTTOMUP:
				t1 = my->sync(obj->clock, t0);
				obj->clock = t0;
				break;

			case PC_POSTTOPDOWN:
				t1 = my->postsync(obj->clock, t0);
				break;

			default:
				gl_error("refrigerator::sync- invalid pass configuration");
				t1 = TS_INVALID; // serious error in exec.c
		}
	} 
	catch (char *msg)
	{
		gl_error("refrigerator::sync exception caught: %s", msg);
		t1 = TS_INVALID;
	}
	catch (...)
	{
		gl_error("refrigerator::sync exception caught: no info");
		t1 = TS_INVALID;
	}
		
	return t1;
	

}

EXPORT int init_refrigerator(OBJECT *obj)
{
	refrigerator *my = OBJECTDATA(obj,refrigerator);
	return my->init(obj->parent);
}

EXPORT int isa_refrigerator(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,refrigerator)->isa(classname);
	} else {
		return 0;
	}
}

/*	determine if we're turning the motor on or off and nothing else. */
EXPORT TIMESTAMP plc_refrigerator(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the refrigerator

	refrigerator *my = OBJECTDATA(obj,refrigerator);
	my->thermostat(obj->clock, t0);

	return TS_NEVER;  
}


/**@}**/

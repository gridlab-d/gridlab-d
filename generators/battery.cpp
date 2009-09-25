/** $Id: battery.cpp,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file battery.cpp
	@defgroup battery
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"

#include "energy_storage.h"
#include "battery.h"
#include "gridlabd.h"




#define HOUR 3600 * TS_SECOND

CLASS *battery::oclass = NULL;
battery *battery::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
battery::battery(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"battery",sizeof(battery),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__); 

		if (gl_publish_variable(oclass,
			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",SUPPLY_DRIVEN, //PV must operate in this mode
				PT_KEYWORD,"POWER_DRIVEN",POWER_DRIVEN,

			PT_enumeration,"generator_status",PADDR(gen_status_v),
				PT_KEYWORD,"OFFLINE",OFFLINE,
				PT_KEYWORD,"ONLINE",ONLINE,	

			PT_enumeration,"rfb_size", PADDR(rfb_size_v),
				PT_KEYWORD, "SMALL", SMALL, 
				PT_KEYWORD, "MED_COMMERCIAL", MED_COMMERCIAL,
				PT_KEYWORD, "MED_HIGH_ENERGY", MED_HIGH_ENERGY,
				PT_KEYWORD, "LARGE", LARGE,

			PT_enumeration,"power_type",PADDR(power_type_v),
				PT_KEYWORD,"AC",AC,
				PT_KEYWORD,"DC",DC,

			PT_double, "power_set_high[W]", PADDR(power_set_high),
			PT_double, "power_set_low[W}", PADDR(power_set_low),
			PT_double, "Rinternal[Ohm]", PADDR(Rinternal),
			PT_double, "V_Max[V]", PADDR(V_Max), 
			PT_complex, "I_Max[A]", PADDR(I_Max),
			PT_double, "E_Max[Wh]", PADDR(E_Max),
			PT_double, "Energy[Wh]",PADDR(Energy),
			PT_double, "efficiency[unit]", PADDR(efficiency),
			PT_double, "base_efficiency[unit]", PADDR(base_efficiency),
			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),

			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			//PT_double, "Rated_kV[kV]", PADDR(Rated_kV),
			PT_complex, "V_Out[V]", PADDR(V_Out),
			PT_complex, "I_Out[A]", PADDR(I_Out),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),
			PT_complex, "V_In[V]", PADDR(V_In),
			PT_complex, "I_In[A]", PADDR(I_In),
			PT_complex, "V_Internal[V]", PADDR(V_Internal),
			PT_complex, "I_Internal[A]",PADDR(I_Internal),
			PT_complex, "I_Prev[A]", PADDR(I_Prev),

			//resistasnces and max P, Q

			PT_set, "phases", PADDR(phases),

				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;




		memset(this,0,sizeof(battery));
		/* TODO: set the default values of all properties here */
	}
}

/* Object creation is called once for each object that is created by the core */
int battery::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	
	
	//generator_mode_choice = CONSTANT_PQ;
	gen_status_v = ONLINE;
	rfb_size_v = SMALL;

	Rinternal = 10;
	V_Max = 0;
	I_Max = 0;
	E_Max = 0;
	Energy = 0;
	recalculate = true;
	margin = 1000;
	number_of_phases_out = 0;
	
	Max_P = 0;//< maximum real power capacity in kW
    Min_P = 0;//< minimus real power capacity in kW
	
	//double Max_Q;//< maximum reactive power capacity in kVar
    //double Min_Q;//< minimus reactive power capacity in kVar
	Rated_kVA = 1; //< nominal capacity in kVA
	//double Rated_kV; //< nominal line-line voltage in kV
	
	efficiency =  0;
	base_efficiency = 0;

	E_Next = 0;
	connected = true;
	complex VA_Internal;
	
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}



/*
double battery::timestamp_to_hours(TIMESTAMP t)
{
	return (double)t/HOUR;
}
*/


/* Object initialization is called once after all object have been created */
int battery::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	//gl_verbose("battery init: finished initializing variables");

	
	static complex default_line123_voltage[1], default_line1_current[1];
	int i;

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=NULL && gl_object_isa(parent,"meter"))
	{
		// attach meter variables to each circuit
		struct {
			complex **var;
			char *varname;
		}
		map[] = {
		// local object name,	meter object name
			{&pCircuit_V,			"voltage_A"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"current_A"}, // assumes 2 and 3(N) follow immediately in memory
			{&pPower,				"measured_power"},
		};
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);

		node *par = OBJECTDATA(parent, node);
		number_of_phases_out = 0;
		if (par->has_phase(PHASE_A))
			number_of_phases_out += 1;
		if (par->has_phase(PHASE_B))
			number_of_phases_out += 1;
		if (par->has_phase(PHASE_C))
			number_of_phases_out += 1;
	}
	else if (parent!=NULL && gl_object_isa(parent,"triplex_meter"))
	{
		number_of_phases_out = 4; //Indicates it is a triplex node and should be handled differently
		struct {
			complex **var;
			char *varname;
		}
		map[] = {
			// local object name,	meter object name
			{&pCircuit_V,			"voltage_12"}, // assumes 1N and 2N follow immediately in memory
			{&pLine_I,				"current_1"}, // assumes 2 and 3(N) follow immediately in memory
			{&pLine12,				"current_12"}, // maps current load 1-2 onto triplex load
			/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
		};
		// attach meter variables to each circuit
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			if ((*(map[i].var) = get_complex(parent,map[i].varname))==NULL)
			{
				GL_THROW("%s (%s:%d) does not implement triplex_meter variable %s for %s (battery:%d)", 
				/*	TROUBLESHOOT
					The battery requires that the triplex_meter contains certain published properties in order to properly connect
					the battery to the triplex-meter.  If the triplex_meter does not contain those properties, GridLAB-D may
					suffer fatal pointer errors.  If you encounter this error, please report it to the developers, along with
					the version of GridLAB-D that raised this error.
				*/
				parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, map[i].varname, obj->name?obj->name:"unnamed", obj->id);
			}
		}
	}
	else if (parent != NULL && strcmp(parent->oclass->name,"inverter") == 0)
	{
		struct {
			complex **var;
			char *varname;
		} 
	
		map[] = {
			// map the V & I from the inverter
			{&pCircuit_V,			"V_In"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"I_In"}, // assumes 2 and 3(N) follow immediately in memory
		};

	 // construct circuit variable map to meter
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			*(map[i].var) = get_complex(parent,map[i].varname);
		}
	}
	else if	((parent != NULL && strcmp(parent->oclass->name,"meter") != 0)||(parent != NULL && strcmp(parent->oclass->name,"triplex_meter") != 0)||(parent != NULL && strcmp(parent->oclass->name,"inverter") != 0))
	{
		throw("Battery must have a meter or triplex meter or inverter as it's parent");
		/*  TROUBLESHOOT
		Check the parent object of the inverter.  The battery is only able to be childed via a meter or 
		triplex meter when connecting into powerflow systems.  You can also choose to have no parent, in which
		case the battery will be a stand-alone application using default voltage values for solving purposes.
		*/
	}
	else
	{
		number_of_phases_out = 3;
		struct {
			complex **var;
			char *varname;
		}
		map[] = {
		// local object name,	meter object name
			{&pCircuit_V,			"voltage_A"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"current_A"}, // assumes 2 and 3(N) follow immediately in memory
		};

		gl_warning("Battery:%d has no parent meter object defined; using static voltages", obj->id);

		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line123_voltage[0];
		*(map[1].var) = &default_line1_current[0];

		// provide initial values for voltages
		default_line123_voltage[0] = complex(480,0);
		default_line123_voltage[1] = complex(480*cos(2*PI/3),480*sin(2*PI/3));
		default_line123_voltage[2] = complex(480*cos(-2*PI/3),480*sin(-2*PI/3));
	}


	//gl_verbose("battery init: finished connecting with meter");


	/* TODO: set the context-dependent initial value of properties */
	//double ZB, SB, EB;
	//complex tst;
	if (gen_mode_v==UNKNOWN)
	{
		OBJECT *obj = OBJECTHDR(this);
		throw("Generator control mode is not specified");
	}
	if (gen_status_v==0)
	{
		//OBJECT *obj = OBJECTHDR(this);
		throw("Generator is out of service!");
	}
	else
	{
		switch(rfb_size_v){
			case SMALL:
				V_Max = 75.2; //V
				//V_Max = 500;
				I_Max = 250; //A
				Max_P = 18800; //W
				E_Max = 160000; //Wh
				base_efficiency = 0.7; // roundtrip
				break;
			case MED_COMMERCIAL:
				V_Max = 115; //V
				I_Max = 435; //A
				Max_P = 50000; //W
				E_Max = 175000; //Wh
				base_efficiency = 0.8; // roundtrip
				break;
			case MED_HIGH_ENERGY:
				V_Max = 115; //V
				I_Max = 435; //A
				Max_P = 50000; //W
				E_Max = 400000; //Wh
				base_efficiency = 0.8; // roundtrip
				break;
			case LARGE:
				V_Max = 8000; //V  
				I_Max = 30; //A
				Max_P = V_Max.Mag()*I_Max.Mag(); 
				E_Max = 24*Max_P; //Wh		 
				base_efficiency = 0.9; // roundtrip
				break;
			default:
				V_Max = 115; //V
				I_Max = 435; //A
				Max_P = 50000; //W
				E_Max = 175000; //Wh
				base_efficiency = 0.8; // roundtrip
				break;
	}

	if ((power_set_high-power_set_low) <= 2*Max_P)
		gl_warning("Your high and low power setpoints are set close enough together that oscillation may occur. You may want to set to wider limits.");
	Energy = gl_random_normal(E_Max/2,E_Max/20);
	//I_Max = I_Max / 5;
	recalculate = true;
	power_transferred = 0;

	first_time_step = 0;

	////gl_verbose("battery init: about to exit");
	return 1; /* return 1 on success, 0 on failure */
}

}


complex battery::calculate_v_terminal(complex v, complex i){
	return v - (i * Rinternal);
}


complex *battery::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}






/** This method has the potential to become much more complex, as it starts taking into account the effect of
internal resistances and a separate internal voltage.  For now, it is a simple linear march from time t0 to 
a steady state endpoint.

*/

TIMESTAMP battery::rfb_event_time(TIMESTAMP t0, complex power, double e){
	if((e < margin) && (power < 0)){
		return TS_NEVER;
	}
	if((e > E_Max - margin)&& (power > 0)){
		return TS_NEVER;
	}
	
	if(power < 0){ //dscharging
		double t1 = (e) / power.Re(); // time until depleted in hours
		t1 = t1 * 60 * 60; //time until depleted in seconds
		return t0 + (TIMESTAMP)(t1 * TS_SECOND);
	}else if(power > 0){ //charging
		double t1 = (E_Max - e) / power.Re(); // time until full in hours
		t1 = t1 * 60 * 60; //time until full in seconds
		return t0 + (TIMESTAMP)(t1 * TS_SECOND);
	}else{ //p ==0
		return TS_NEVER;
	}

}


double battery::calculate_efficiency(complex voltage, complex current){
	if(voltage.Mag() == 0 || current.Mag() == 0){
		return 1;
	}
	else if(current.Mag() < 5){
		return 0.95;
	}
	else if(current.Mag() < 10){
		return 0.9;
	}
	else if(current.Mag() < 20){
		return 0.8;
	}else{
		return 0.7;
	}

}



/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP battery::presync(TIMESTAMP t0, TIMESTAMP t1)
{

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP battery::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	if (gen_mode_v == POWER_DRIVEN && number_of_phases_out == 3)
	{
		complex volt[3];
		double check_power;
		TIMESTAMP dt,t_energy_limit;

		for (int i=0;i<3;i++)
		{
			volt[i] = pCircuit_V[i];
		}

		if (first_time_step == 0)
		{
			dt = 0;
		}
		else if (prev_time == 0)
		{
			prev_time = t1;
			dt = 0;
		}
		else if (prev_time < t1)
		{
			dt = t1 - prev_time;
			prev_time = t1;
		}
		else
			dt = 0;
		
		if (prev_state == -1) //discharge
		{
			Energy = Energy - (1 / base_efficiency) * power_transferred * (double)dt / 3600;
			if (Energy < 0)
				Energy = 0;
		}
		else if (prev_state == 1) //charge
			Energy = Energy + base_efficiency * power_transferred * (double)dt / 3600;

		check_power = (*pPower).Mag();

		if (first_time_step > 0)
		{
			if (volt[0].Mag() > V_Max.Mag() || volt[1].Mag() > V_Max.Mag() || volt[2].Mag() > V_Max.Mag())
			{
				gl_warning("The voltages at the batteries meter are higher than rated. No power output.");
				pLine_I[0] += complex(0,0);
				pLine_I[1] += complex(0,0);
				pLine_I[2] += complex(0,0);

				last_current[0] = complex(0,0);
				last_current[1] = complex(0,0);
				last_current[2] = complex(0,0);

				return TS_NEVER;
			}
			else if (check_power > power_set_high && Energy > 0) //discharge
			{
				prev_state = -1;

				pLine_I[0] += complex(-I_Max.Mag()/3,0,A); //generation, pure real power
				pLine_I[1] += complex(-I_Max.Mag()/3,-2*PI/3,A);
				pLine_I[2] += complex(-I_Max.Mag()/3,2*PI/3,A);

				last_current[0] = complex(-I_Max.Mag()/3,0,A);;
				last_current[1] = complex(-I_Max.Mag()/3,-2*PI/3,A);
				last_current[2] = complex(-I_Max.Mag()/3,2*PI/3,A);

				power_transferred = 0;

				for (int i=0;i<3;i++)
				{	
					power_transferred += last_current[i].Mag()*volt[i].Mag();
				}

				VA_Out = power_transferred;
				
				t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);
				if (t_energy_limit == 0)
					t_energy_limit = 1;

				return -(t1 + t_energy_limit);
			}
			else if (check_power < power_set_low && Energy < E_Max) //charge
			{
				prev_state = 1;

				pLine_I[0] += complex(I_Max.Mag()/3,0,A); //load, pure real power
				pLine_I[1] += complex(I_Max.Mag()/3,-2*PI/3,A);
				pLine_I[2] += complex(I_Max.Mag()/3,2*PI/3,A);

				last_current[0] = complex(I_Max.Mag()/3,0,A);
				last_current[1] = complex(I_Max.Mag()/3,-2*PI/3,A);
				last_current[2] = complex(I_Max.Mag()/3,2*PI/3,A);

				power_transferred = 0;

				for (int i=0;i<3;i++)
				{	
					power_transferred += last_current[i].Mag()*volt[i].Mag();
				}
				
				VA_Out = power_transferred;

				t_energy_limit = TIMESTAMP(3600 * (E_Max - Energy) / power_transferred);
				if (t_energy_limit == 0)
					t_energy_limit = 1;

				return -(t1 + t_energy_limit);
			}
			else if (check_power < (power_set_low + Max_P) && prev_state == 1 && Energy < E_Max) //keep charging until out of "deadband"
			{
				prev_state = 1;

				pLine_I[0] += complex(I_Max.Mag()/3,0,A); //load, pure real power
				pLine_I[1] += complex(I_Max.Mag()/3,-2*PI/3,A);
				pLine_I[2] += complex(I_Max.Mag()/3,2*PI/3,A);

				last_current[0] = complex(I_Max.Mag()/3,0,A);
				last_current[1] = complex(I_Max.Mag()/3,-2*PI/3,A);
				last_current[2] = complex(I_Max.Mag()/3,2*PI/3,A);

				power_transferred = 0;

				for (int i=0;i<3;i++)
				{	
					power_transferred += last_current[i].Mag()*volt[i].Mag();
				}
				
				VA_Out = power_transferred;

				t_energy_limit = TIMESTAMP(3600 * (E_Max - Energy) / power_transferred);
				if (t_energy_limit == 0)
					t_energy_limit = 1;

				return -(t1 + t_energy_limit);
			}
			else if (check_power > (power_set_high - Max_P) && prev_state == -1 && Energy > 0) //keep discharging until out of "deadband"
			{
				prev_state = -1;

				pLine_I[0] += complex(-I_Max.Mag()/3,0,A); //generation, pure real power
				pLine_I[1] += complex(-I_Max.Mag()/3,-2*PI/3,A);
				pLine_I[2] += complex(-I_Max.Mag()/3,2*PI/3,A);

				last_current[0] = complex(-I_Max.Mag()/3,0,A);
				last_current[1] = complex(-I_Max.Mag()/3,-2*PI/3,A);
				last_current[2] = complex(-I_Max.Mag()/3,2*PI/3,A);

				power_transferred = 0;

				for (int i=0;i<3;i++)
				{	
					power_transferred += last_current[i].Mag()*volt[i].Mag();
				}

				VA_Out = -power_transferred;
				
				t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

				if (t_energy_limit == 0)
					t_energy_limit = 1;

				return -(t1 + t_energy_limit);
			}
			else
			{
				pLine_I[0] += complex(0,0);
				pLine_I[1] += complex(0,0);
				pLine_I[2] += complex(0,0);

				last_current[0] = complex(0,0);
				last_current[1] = complex(0,0);
				last_current[2] = complex(0,0);

				prev_state = 0;
				power_transferred = 0;
				VA_Out = power_transferred;
				return TS_NEVER;
			}
		}
		else
		{
			first_time_step = 1;
			return TS_NEVER;
		}

	}
	else
	{
		//gather V_Out
		//gather I_Out
		//gather P_Out
		////gl_verbose("battery sync: entered");
		V_Out = pCircuit_V[0];
		I_Out = pLine_I[0];

		//gl_verbose("battery sync: V_Out from parent is: (%f , %f)", V_Out.Re(), V_Out.Im());
		//gl_verbose("battery sync: I_Out from parent is: (%f , %f)", I_Out.Re(), V_Out.Im());


		V_In = V_Out;

		V_Internal = calculate_v_terminal(V_Out, I_Out);
		//V_Out = V_internal;

		//finds the CURRENT efficiency
		efficiency = base_efficiency * calculate_efficiency(V_Out, I_Out);

		//gl_verbose("battery sync: base_efficiency is: %f", base_efficiency);
		//gl_verbose("battery sync: efficiency is: %f", efficiency);

		if (I_Out > 0){ // charging
			I_Internal = I_Out * efficiency; //actual stored current is less than what was put in
		}

		if(I_Out < 0){ // discharging
			I_Internal = I_Out / efficiency; //actual released current is more than what is obtained by parent
		}


		//gl_verbose("battery sync: V_In to push is: (%f , %f)", V_In.Re(), V_In.Im());
		//gl_verbose("battery sync: V_Internal to push is: (%f , %f)", V_Internal.Re(), V_Internal.Im());
		//gl_verbose("battery sync: I_Internal to push is: (%f , %f)", I_Internal.Re(), I_Internal.Im());

		VA_Out = V_Out * (~I_Out);
		VA_Internal = V_Internal * I_Internal;  // charging rate is slower than termainl voltage suggests, similarly,
											//discharge rate is faster than the terminal voltage suggests
			
		//gl_verbose("battery sync: VA_Out from parent is: (%f , %f)", VA_Out.Re(), VA_Out.Im());
		//gl_verbose("battery sync: VA_Internal calculated is: (%f , %f)", VA_Internal.Re(), VA_Internal.Im());
		

		if(!recalculate){//if forced recalculate is false, check where the time is
		//gl_verbose("battery sync: don't recalculate");
			if(t0 == prev_time){
				//gl_verbose("battery sync: reached expected time, set energy and don't recalculate");
				Energy = E_Next; // we're all set this time around, just set the energy level onward
				//gl_verbose("battery sync: Energy level is %f", Energy);
				//gl_verbose("battery sync: V_In to push is: (%f , %f)", V_In.Re(), V_In.Im());
				//gl_verbose("battery sync: I_In to push is: (%f , %f)", I_In.Re(), I_In.Im());
				recalculate = true; //recalculate the next time around
				
				return battery::sync(t0, t1);
			}else{
				//gl_verbose("battery sync: Didn't reach expected time, force recalculate");
				//gl_verbose("battery sync: V_In to push is: (%f , %f)", V_In.Re(), V_In.Im());
				//gl_verbose("battery sync: I_In to push is: (%f , %f)", I_In.Re(), I_In.Im());
				Energy = E_Next;
				//TIMESTAMP prev_time2 = prev_time;
				//prev_time = t0;
				recalculate = true; //set forced recalculate to true becuase we've reached an unexpected time
				//return battery::sync(prev_time2, t0);
				return battery::sync(prev_time, t1);
			}
			
		}else{
			//////////////////// need this in hours //////////////////
			//TIMESTAMP t2 = t1 - t0;
			//gl_verbose("battery sync: recalculate!");
			//gl_verbose("battery sync: energy level pre changes is: %f", Energy);
			//gl_verbose("battery sync: energy level to go to next is: %f", E_Next);

			///need to use timestamp_to_hours from the core, not here, how to reference core?
			//double t2 = (timestamp_to_hours((TIMESTAMP)t1) - timestamp_to_hours((TIMESTAMP)t0));
			double t2 = (gl_tohours((TIMESTAMP)t1) - gl_tohours((TIMESTAMP)t0));

			if(abs((double)V_Out.Re()) > abs((double)V_Max.Re())){
				//gl_verbose("battery sync: V_Out exceeded allowable V_Out, setting to max");
				V_Out = V_Max;
				V_In = V_Out;
				V_Internal = V_Out - (I_Out * Rinternal);
			}

			if(abs((double)I_Out.Re()) > abs((double)I_Max.Re())){
				//gl_verbose("battery sync: I_Out exceeded allowable I_Out, setting to max");
				I_Out = I_Max;
			}

			if(abs((double)VA_Out.Re()) > abs((double)Max_P)){
				//gl_verbose("battery sync: VA_Out exceeded allowable VA_Out, setting to max");
				VA_Out = complex(Max_P , 0);
				VA_Internal = VA_Out - (I_Out * I_Out * Rinternal);
			}



		prev_time = t1;

		if(VA_Out < 0){ //discharging
			//gl_verbose("battery sync: discharging");
			if(Energy == 0 || Energy <= margin){ 
				//gl_verbose("battery sync: battery is empty!");
				if(connected){
					//gl_verbose("battery sync: empty BUT it is connected, passing request onward");
					I_In = I_Max + complex(abs(I_Out.Re()), abs(I_Out.Im())); //power was asked for to discharge but battery is empty, forward request along the line
					I_Prev = I_Max / efficiency;
					//Get as much as you can from the microturbine --> the load asked for as well as the max
					recalculate = false;
					E_Next = Energy + (((I_In - complex(abs(I_Out.Re()), abs(I_Out.Im()))) * V_Internal / efficiency) * t2).Re();  // the energy level at t1
					TIMESTAMP t3 = rfb_event_time(t0, (I_In - complex(abs(I_Out.Re()), abs(I_Out.Im()))) * V_Internal / efficiency, Energy);
					return t3;
				}
				else{
					//gl_verbose("battery sync: battery is empty with nothing connected!  drop request!");
					I_In = 0;
					I_Prev = 0;
					V_In = V_Out;
					VA_Out = 0;
					E_Next = 0;
					recalculate = false;
					return TS_NEVER;
				}
			}

			if((Energy + (V_Internal * I_Prev.Re()).Re() * t2) <= margin){ //headed to empty
				//gl_verbose("battery sync: battery is headed to empty");
				if(connected){
					//gl_verbose("battery sync: BUT battery is connected, so pass request onward");
					I_In = I_Max + complex(abs(I_Out.Re()), abs(I_Out.Im())); //this won't let the battery go empty... change course 
					I_Prev = I_Max / efficiency;
					//if the battery is connected to something which serves the load and charges the battery
					E_Next = margin;
					recalculate = false;
					TIMESTAMP t3 = rfb_event_time(t0, (I_In - complex(abs(I_Out.Re()), abs(I_Out.Im()))) * V_Internal / efficiency, Energy);
					return t3;
				}else{
					//gl_verbose("battery sync: battery is about to be empty with nothing connected!!");
					TIMESTAMP t3 = rfb_event_time(t0, VA_Internal, Energy);
					E_Next = 0; //expecting return when time is empty
					I_In = 0; 
					I_Prev = 0;
					recalculate = false;
					return t3;
				}
			}else{ // doing fine
				//gl_verbose("battery sync: battery is not empty, demand supplied from the battery");
				E_Next = Energy + (VA_Internal.Re() * t2);
				I_In = 0; //nothign asked for
				I_Prev = 0;
				recalculate = false;
				TIMESTAMP t3 = rfb_event_time(t0, VA_Internal, Energy);
				return t3;
			}
		}else if (VA_Out > 0){ //charging
			if(Energy >= (E_Max - margin)){
				//gl_verbose("battery sync: battery is full!");
				if(connected){
					//attempt to let other items serve the load if the battery is full instead of draining the battery
					//gl_verbose("battery sync: battery is full and connected, passing the power request onward");
					E_Next = Energy;
					I_In = I_Out = 0; //drop the request to charge
					I_Prev = 0;
					recalculate = false;
					return TS_NEVER;
				}else{
					//gl_verbose("battery sync: battery is full, and charging");
					I_In = I_Out = 0; //can't charge any more... drop the current somehow?
					I_Prev = 0;
					V_Out = V_Out; // don't drop V_Out in this case
					VA_Out = 0;
					E_Next = Energy;
					return TS_NEVER;
				}
			}

			if(Energy + ((V_Internal * I_Prev.Re()) * efficiency * t2).Re() >= (E_Max - margin)){ //if it is this far, it is charging at max
				//gl_verbose("battery sync: battery is about to be full");
				TIMESTAMP t3 = rfb_event_time(t0, VA_Internal, Energy);
				I_In = 0;
				I_Prev = 0;
				E_Next = E_Max - margin;
				recalculate = false;
				return t3;
			}else{
				if(connected){
					//gl_verbose("battery sync: battery is charging but not yet full, connected");
					//if it is connected, use whatever is connected to help it charge;
					I_In = I_Max - I_Out; // total current in is now I_Max
					I_Prev = I_Max * efficiency;
					E_Next = Energy + ((I_Max * V_Internal) * efficiency * t2).Re();
					recalculate = false;
					TIMESTAMP t3 = rfb_event_time(t0, (I_Max * V_Internal * efficiency), Energy);
					return t3;
				}else{
					//gl_verbose("battery sync: battery is charging but not yet full, not connected");
					I_In = 0;
					I_Prev = 0;
					E_Next = Energy + (VA_Internal * t2).Re();
					recalculate = false;
					TIMESTAMP t3 = rfb_event_time(t0, VA_Internal, Energy);
					return t3;
				}
			}
		}else{// VA_Out = 0
			//gl_verbose("battery sync: battery is not charging or discharging");
			recalculate = false;
			I_In = 0;
			I_Prev = 0;
			E_Next = Energy;
			return TS_NEVER;
		}

		}
	}

return TS_NEVER;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP battery::postsync(TIMESTAMP t0, TIMESTAMP t1)
{

	pLine_I[0] -= last_current[0];
	pLine_I[1] -= last_current[1];
	pLine_I[2] -= last_current[2];

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_battery(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(battery::oclass);
		if (*obj!=NULL)
		{
			battery *my = OBJECTDATA(*obj,battery);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	} 
	catch (char *msg) 
	{
		gl_error("create_battery: %s", msg);
	}
	return 0;
}

EXPORT int init_battery(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,battery)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_battery(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_battery(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	battery *my = OBJECTDATA(obj,battery);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;		
	}
	catch (char *msg)
	{
		gl_error("sync_battery(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return t2;
}

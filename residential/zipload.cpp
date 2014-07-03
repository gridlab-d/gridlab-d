/** $Id: zipload.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file ZIPloadload.cpp
	@addtogroup ZIPload
	@ingroup residential
	
	The ZIPload simulation is based on demand profile of the connected ZIPload loads.
	Individual power fractions and power factors are used.
	Heat fraction ratio is used to calculate the internal gain from ZIPload loads.

	The demand response model is based from a paper written by David Chassin and Jason Fuller
	"On the Equilibrium Dynamics of Demand Response in Thermostatic Loads" accepted by HICSS
	 - This model is very much in the preliminary stage and only looks at the equilibrium solutions
	   of a population of devices as a function of customer demand and thermostatic duty cycles

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <memory.h>

#include "house_a.h"
#include "zipload.h"

//////////////////////////////////////////////////////////////////////////
// ZIPload CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* ZIPload::oclass = NULL;
CLASS* ZIPload::pclass = NULL;

ZIPload::ZIPload(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"ZIPload",sizeof(ZIPload),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"heat_fraction",PADDR(load.heatgain_fraction), PT_DESCRIPTION, "fraction of ZIPload that is transferred as heat",
			PT_double,"base_power[kW]",PADDR(base_power), PT_DESCRIPTION, "base real power of the overall load",
			PT_double,"power_pf",PADDR(power_pf), PT_DESCRIPTION, "power factor for constant power portion",
			PT_double,"current_pf",PADDR(current_pf), PT_DESCRIPTION, "power factor for constant current portion",
			PT_double,"impedance_pf",PADDR(impedance_pf), PT_DESCRIPTION, "power factor for constant impedance portion",
			PT_bool,"is_240",PADDR(is_240), PT_DESCRIPTION, "load is 220/240 V (across both phases)",
			PT_double,"breaker_val[A]",PADDR(breaker_val), PT_DESCRIPTION, "Amperage of connected breaker",
			PT_complex,"actual_power[kVA]",PADDR(actual_power),PT_DESCRIPTION,"variable to pull actual load as function of voltage",
			PT_double,"multiplier",PADDR(multiplier),PT_DESCRIPTION,"this variable is used to modify the base power as a function of multiplier x base_power",
			PT_bool,"heatgain_only",PADDR(heatgain_only),PT_DESCRIPTION,"allows the zipload to generate heat only (no kW), not activated by default",

			// Variables for demand response mode
			PT_bool,"demand_response_mode",PADDR(demand_response_mode), PT_DESCRIPTION, "Activates equilibrium dynamic representation of demand response",
			PT_int16,"number_of_devices",PADDR(N), PT_DESCRIPTION, "Number of devices to model - base power is the total load of all devices",
			PT_int16,"thermostatic_control_range",PADDR(L),PT_DESCRIPTION, "Range of the thermostat's control operation",
			PT_double,"number_of_devices_off",PADDR(N_off),PT_DESCRIPTION, "Total number of devices that are off",
			PT_double,"number_of_devices_on",PADDR(N_on),PT_DESCRIPTION, "Total number of devices that are on",
			//PT_double,"density_of_devices_off[1/K]",PADDR(noff),PT_DESCRIPTION, "Density of devices that are off per unit of temperature",
			//PT_double,"density_of_devices_on[1/K]",PADDR(non),PT_DESCRIPTION, "Density of devices that are on per unit of temperature",
			PT_double,"rate_of_cooling[K/h]",PADDR(roff),PT_DESCRIPTION, "rate at which devices cool down",
			PT_double,"rate_of_heating[K/h]",PADDR(ron),PT_DESCRIPTION, "rate at which devices heat up",
			//PT_double,"total_cycle_time[h]",PADDR(t), PT_DESCRIPTION, "total cycle time of a thermostatic device",
			//PT_double,"total_off_time[h]",PADDR(toff),PT_DESCRIPTION, "total off time of device",
			//PT_double,"total_on_time[h]",PADDR(ton),PT_DESCRIPTION, "total on time of device",
			PT_int16,"temperature",PADDR(x),PT_DESCRIPTION, "temperature of the device's controlled media (eg air temp or water temp)",
			PT_double,"phi",PADDR(phi),PT_DESCRIPTION, "duty cycle of the device",
			//PT_double,"diversity_of_population",PADDR(PHI),PT_DESCRIPTION, "diversity of a population of devices",
			PT_double,"demand_rate[1/h]",PADDR(eta),PT_DESCRIPTION, "consumer demand rate that prematurely turns on a device or population",
			//PT_double,"effective_rate[K/h]",PADDR(rho),PT_DESCRIPTION, "effect rate at which devices heats up or cools down under consumer demand",
			PT_double,"nominal_power",PADDR(nominal_power),PT_DESCRIPTION, "the rated amount of power demanded by devices that are on",

			//Variables for creating a duty cycle
			PT_double,"duty_cycle[pu]",PADDR(duty_cycle),PT_DESCRIPTION, "fraction of time in the on state",
			PT_double,"recovery_duty_cycle[pu]",PADDR(recovery_duty_cycle),PT_DESCRIPTION, "fraction of time in the on state, while in recovery interval",
			PT_double,"period[h]",PADDR(period),PT_DESCRIPTION, "time interval to apply duty cycle",
			PT_double,"phase[pu]",PADDR(phase),PT_DESCRIPTION, "initial phase of load; duty will be assumed to occur at beginning of period",
			NULL)<1) 

			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

ZIPload::~ZIPload()
{
}

int ZIPload::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power = load.admittance = load.current = load.total = 0.0;
	base_power = 0.0;
	load.heatgain_fraction = 0.90;
	load.power_factor = 1.00;
	load.power_fraction = 1.0;
	load.current_fraction = load.impedance_fraction = 0.0;
	load.config = 0x0;	//110 by default
	breaker_val = 1000.0;	//Obscene value so it never trips
	multiplier = 1;
	heatgain_only = false;

	demand_response_mode = false;
	ron = roff = -1;
	phi = eta = 0.2;	
	next_time = 0;

	duty_cycle = recovery_duty_cycle = -1;
	period = 0;
	phase = 0;

	//Default values of other properties
	power_pf = current_pf = impedance_pf = 1.0;

	load.voltage_factor = 1.0; // assume 'even' voltage, initially

	first_pass = 1;
	return res;
}

int ZIPload::init(OBJECT *parent)
{
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("zipload::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (demand_response_mode == true)
	{
		gl_warning("zipload_init: The demand response zipload is an experimental model at this time.");
		/*  TROUBLESHOOT
		The warning is pretty obvious.  However, over time, we will be developing this model further.  If you have any questions 
		about it, please see the Matlab files found in ../design/dr_model/ or read the paper titled "On the Equilibrium Dynamics of Demand Response"
		submitted to HICSS 2011 or post your questions on the WIKI forum.
		*/
		
		// Initial error checks
		if (abs(eta) > 1)
		{
			GL_THROW("zipload_init: demand_rate (eta) must be between -1 and 1.");
			/*  TROUBLESHOOT
			The demand rate is limited to values between -1 and 1 (inclusive).  Please reset to an appropriate value.
			*/
		}
		if (phi < 0 || phi > 1)
		{
			GL_THROW("zipload_init: duty_cycle (phi) must be between 0 and 1.");
			/*  TROUBLESHOOT
			The duty cycle is only explicitly used if ron and roff are not set.  In normal operation, phi will be calculated from
			ron and roff as a function of time.  However, if ron and roff are not set, the initial values for ron and roff are calculated
			from phi.  Please set to a value between 1 and 0 (inclusive).
			*/
		}
		
		// Set up the buffers and perform some error checks
		if (L > 0)
			if (L < 70)
				drm.nbins = L;
			else
			{
				gl_warning("zipload_init: Using a value for thermostatic_control_range (L) greater than 50 may cause some instabilities.");
				/*  TROUBLESHOOT
				This warning is shown only as a reminder.  Large values of L (thermostatic_control_range) can cause instabilities for some
				combinations of ron and roff.  If you receive inderminant numbers as part of the solution, try reducing L.
				*/
			}
		else
		{
			GL_THROW("zipload_init: thermostatic_control_range (L) must be greater than 0.");
			/*  TROUBLESHOOT
			The thermostatic control range must be a positive integer value, since this is used to create the number of bins 
			for the discrete solution.
			*/
		}

		drm.off = (double*)malloc(sizeof(double)*L);
		drm.on = (double*)malloc(sizeof(double)*L);
		if (drm.off==NULL || drm.on==NULL)
		{
			GL_THROW("zipload_init: memory allocation error.  Please report this error.");
			/*  TROUBLESHOOT
			If you receive this error, something went horribly wrong with a memory allocation. Please report this to TRAC and provide
			the glm file that caused it.
			*/
		}

		/* setup the initial population */
		if (ron == -1 || roff == -1)
		{
			if (phi <= 0.5)
			{
				roff = phi/(1-phi);
				ron = 1;
				gl_warning("ron or roff was not set.  Using phi to calculate.  Step changes in demand rates as a function of time will be ignored.");
				/*  TROUBLESHOOT
				Ideally, in the method currently being used, ron and roff (heating and cooling rates) should be used to calculate phi.
				If you receive this error, the phi is being used to calculate ron and roff initially.  However, phi does not update  
				ron and roff at each time step, so you will not be able to perform disturbances of demand.  If you wish this, please use
				ron and roff as a function of time instead.  This can also be caused by using a schedule or player transform to describe 
				the ron or roff values - essentially during the initialization, the value is not described yet.  There is no current fix for
				this, but can be "faked" by setting phi to the correct initial value and waiting a couple of timesteps for things to settle.
				*/
			}
			else
			{
				roff = 1;
				ron = (1-phi)/phi;
				gl_warning("ron or roff was not set.  Using phi to calculate. Step changes in demand rates as a function of time will be ignored.");
				/*  TROUBLESHOOT
				Ideally, in the method currently being used, ron and roff (heating and cooling rates) should be used to calculate phi.
				If you receive this error, the phi is being used to calculate ron and roff initially.  However, phi does not update  
				ron and roff at each time step, so you will not be able to perform disturbances of demand.  If you wish this, please use
				ron and roff as a function of time instead.  This can also be caused by using a schedule or player transform to describe 
				the ron or roff values - essentially during the initialization, the value is not described yet.  There is no current fix for
				this, but can be "faked" by setting phi to the correct initial value and waiting a couple of timesteps for things to settle.
				*/
			}
		}
		else
			phi = roff / (ron + roff);

		if (roff < 0 || ron < 0)
		{
			GL_THROW("zipload_init: rate_of_cooling and rate_of_heating must be greater than or equal to 0.");
			/*  TROUBLESHOOT
			Rates of heating and cooling should be positive or zero values.  These values describe how fast objects transition from a 
			cooler to hotter temperature, or vice versa, and have been defined as positive values in this model.
			*/
		}

		non = noff = 0;
		double test_N = 0;

		for (x=0; x<L; x++)
		{
			/* exponential distribution */
			if (eta != 0)
			{
				drm.on[x] = N * eta * (1-phi) * exp(eta*(L-0.5-x)/roff) / (exp(eta*L/roff)-1);
				drm.off[x] = drm.on[x] * ron/roff;
				test_N += drm.on[x] + drm.off[x];
				//non += drm.on[x] = eta * (1-phi) * exp(eta*(L-x+0.5)/roff) / (exp(eta*L/roff)-1);
				//noff += drm.off[x] = drm.on[x]*ron/roff;
			}

			/* uniform distribution */
			else
			{
				non += drm.on[x] = N * phi/L;
				noff += drm.off[x] = N * (1-phi)/L;
			}
		}

		/* check for valid population */
		if (abs(test_N - N) != 0)
		{
			double extra = N - test_N;
			drm.off[0] += extra;
		}
		
	}

	if (duty_cycle > 1 || duty_cycle < 0)
	{
		if (duty_cycle != -1)
		{
			GL_THROW("Value of duty cycle is set to a value outside of 0-1.");
			/*  TROUBLESHOOT
			By definition, a duty cycle must be between, and including, 0 and 1.  Zero will turn the
			duty cycle function OFF.  Please specify a duty_cycle value between 0 and 1.
			*/
		}
	}

	// We're using a duty cycle mode
	if (duty_cycle != -1)
	{
		if (period <= 0)
		{
			GL_THROW("Period is less than or equal to zero.");
			/*  TROUBLESHOOT
			When using duty cycle mode, the period must be a positive value.
			*/
		}
		if (phase < 0 || phase > 1)
		{
			GL_THROW("Phase is not between zero and one.");
			/*  TROUBLESHOOT
			When using duty cycle mode, the phase must be specified as a value between 0 and 1.  This
			will set the initial phase as a percentage of the period.  The "duty" will assume to be
			applied at the beginning of each period.  Randomizing this input value will prevent syncing of
			objects.
			*/
		}
	}

	if (heatgain_only == true)
	{
		load.config = EUC_HEATLOAD;
		load.power_fraction = load.current_fraction = load.impedance_fraction = 0.0;
	}
	if (is_240)	//See if the 220/240 flag needs to be set
	{
		load.config |= EUC_IS220;
	}

	load.breaker_amps = breaker_val;

	return residential_enduse::init(parent);
}

int ZIPload::isa(char *classname)
{
	return (strcmp(classname,"ZIPload")==0 || residential_enduse::isa(classname));
}

TIMESTAMP ZIPload::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	TIMESTAMP t2 = TS_NEVER;
	double real_power = 0.0;
	double imag_power = 0.0;
	double angleval;

	double test = multiplier;

	if (demand_response_mode == true && next_time <= t1)
	{
		double dNon,dNoff;		
		double hold_off;	

		N_on = N_off = 0;

		for (int jj=0; jj<L; jj++)
		{
			//previous_drm.on[jj] = drm.on[jj];
			//previous_drm.off[jj] = drm.off[jj];

			if (eta > 0)
			{			
				if (jj != (L-1))
					dNon = -ron * drm.on[jj] + eta * drm.off[jj] + ron * drm.on[jj+1];
				else
					dNon = -ron * drm.on[jj] + (1 - eta) * drm.off[jj] * roff + eta * drm.off[jj];
				
				if (jj != 0)
					dNoff = -(1 - eta) * drm.off[jj] * roff - eta * drm.off[jj] + (1 - eta) * hold_off * roff;
				else
					dNoff = -(1 - eta) * drm.off[jj] * roff - eta * drm.off[jj] + ron * drm.on[jj];
			}
			else
			{
				if (jj != (L-1))
					dNon = -ron * (1 + eta) * drm.on[jj] + eta * drm.on[jj] + ron * (1 + eta) * drm.on[jj+1];
				else
					dNon = -ron * (1 + eta) * drm.on[jj] + eta * drm.on[jj] + roff * drm.off[jj];
				
				if (jj != 0)
					dNoff = -roff * drm.off[jj] - eta * drm.on[jj] + hold_off * roff;
				else
					dNoff = -roff * drm.off[jj] - eta * drm.on[jj] + ron * (1 + eta) * drm.on[jj];
			}

			hold_off = drm.off[jj];

			drm.on[jj] = drm.on[jj] + dNon;
			N_on += drm.on[jj];
			drm.off[jj] = drm.off[jj] + dNoff;
		}

		N_off = N - N_on;
		phi = roff / (ron + roff);
		nominal_power = base_power * N_on / N;
		double R = ron;
		int dt = 0;
		if (ron < roff)
			R = roff;
		dt = int(1/R * 3600);

		next_time = t1 + dt;
	}

	// We're in duty cycle mode
	if (duty_cycle != -1)
	{
		double phase_shift = 0;

		if (first_pass == 0) // after first time step
		{
			phase_shift = (t1 - last_time) / (period * 3600);
			phase = phase + phase_shift;
			last_time = t1;
		}
		else
			first_pass = 0;
			last_time = t1;

		if (this->re_override == OV_NORMAL) // Normal operation
		{
			if (phase >= 1)
				phase = 0;

			// Track the time of 2 transistions
			if (t1 >= next_time || last_duty_cycle != duty_cycle)
			{
				if (duty_cycle > phase) // OFF->ON
				{
					multiplier = 1;
					next_time = t1 + (period * 3600) * (duty_cycle - phase) + 1; // +1 a bit of an offset for rounding
				}
				else					// ON->OFF
				{
					multiplier = 0;
					next_time = t1 + (period * 3600) * (1 - phase) + 1;
				}
			}
			last_duty_cycle = duty_cycle;
		}
		else if (this->re_override == OV_OFF) // After release or recovery time
		{
			if (phase <= 1 && t1>=next_time)
			{
				this->re_override = OV_NORMAL;
				next_time = t1;
			}
			else if (t1 >= next_time || next_time == TS_NEVER) // we just came from override ON
			{
				if (recovery_duty_cycle > fmod(phase,1)) // OFF->ON 
				{
					multiplier = 1;

					next_time = t1 + (period * 3600) * (recovery_duty_cycle - fmod(phase,1)) + 1; // +1 a bit of an offset for rounding
					if (duty_cycle != 0.0)
						phase -= 1 * recovery_duty_cycle / duty_cycle * (1 - fmod(phase,1)); // Track everything by the original duty cycle
					else 
						phase = 0; //Start over

					/*if (phase < 0)
					{
						next_time = t1 + (period * 3600) * (recovery_duty_cycle - 0) + 1;
						phase = 0;
					}*/
				}			
				else					// ON->OFF
				{
					//if (multiplier == 1) // we just transitioned
					//{
						//if (phase >= 1 * recovery_duty_cycle / duty_cycle)
						//	phase -= 1 * recovery_duty_cycle / duty_cycle; // Track everything by the original duty cycle
						//else if (phase < 1)
						//	this->re_override = OV_NORMAL;
					//}
					multiplier = 0;
					next_time = t1 + (period * 3600) * (1 - fmod(phase,1)) + 1;
				}
			}
			last_duty_cycle = duty_cycle;

		}
		else // override is ON, so no power
		{
			if (multiplier == 1 && duty_cycle > phase)
			{
				// do nothing
				last_duty_cycle = duty_cycle;
			}
			else
			{
				multiplier = 0;
				next_time = TS_NEVER;
				if (recovery_duty_cycle > 0) // in TOU/CPP mode
					last_duty_cycle = duty_cycle;
				else // DLC mode
				{
					if (phase >= 1)
						phase -= 1;
					last_duty_cycle = 0;
				}
			}
		}
		// last_duty_cycle = duty_cycle;
	}

	if (pCircuit!=NULL){
		if (is_240)
		{
			load.voltage_factor = pCircuit->pV->Mag() / 240; // update voltage factor - not really used for anything
		}
		else //120
		{
			load.voltage_factor = pCircuit->pV->Mag() / 120; // update voltage factor - not really used for anything
		}
	}

	t2 = residential_enduse::sync(t0,t1);

	if (pCircuit->status==BRK_CLOSED) 
	{
		//All values placed as kW/kVAr values - to be consistent with other loads
		double demand_power = multiplier * base_power;
		
		if (demand_response_mode == true)
			demand_power = nominal_power;

		if (heatgain_only == false)
		{
			//Calculate power portion
			real_power = demand_power * load.power_fraction;

			imag_power = (power_pf == 0.0) ? 0.0 : real_power * sqrt(1.0/(power_pf * power_pf) - 1.0);

			if (power_pf < 0)
			{
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}

			load.power.SetRect(real_power,imag_power);

			//Calculate current portion
			real_power = demand_power * load.current_fraction;

			imag_power = (current_pf == 0.0) ? 0.0 : real_power * sqrt(1.0/(current_pf * current_pf) - 1.0);

			if (current_pf < 0)
			{
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}

			load.current.SetRect(real_power,imag_power);

			//Calculate impedance portion
			real_power = demand_power * load.impedance_fraction;

			imag_power = (impedance_pf == 0.0) ? 0.0 : real_power * sqrt(1.0/(impedance_pf * impedance_pf) - 1.0);

			if (impedance_pf < 0)
			{
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}

			load.admittance.SetRect(real_power,imag_power);	//Put impedance in admittance.  From a power point of view, they are the same

			//Compute total power - not sure if needed, but will use below
			load.total = load.power + load.current + load.admittance;
			actual_power = load.power + load.current * load.voltage_factor + load.admittance * load.voltage_factor * load.voltage_factor;

			//Update power factor, just in case
			angleval = load.total.Arg();
			load.power_factor = (angleval < 0) ? -1.0 * cos(angleval) : cos(angleval);

			//Determine the heat contributions - percentage of real power
			load.heatgain = load.total.Re() * load.heatgain_fraction * BTUPHPKW;
		}
		else
		{
			load.power = load.current = load.admittance = actual_power = load.total = 0.0;
			load.heatgain = demand_power * BTUPHPKW;
			return TS_NEVER;
		}
	}
	else	//Breaker's open - nothing happens
	{
		load.total = 0.0;
		load.power = 0.0;
		load.current = 0.0;
		load.admittance = 0.0;
		load.heatgain = 0.0;
		load.power_factor = 0.0;
	}

	if (next_time < t2 && next_time > 0)
		t2 = next_time;
	return t2;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_ZIPload(OBJECT **obj, OBJECT *parent)
{
	try 
	{
		*obj = gl_create_object(ZIPload::oclass);
		if (*obj!=NULL)
		{
			ZIPload *my = OBJECTDATA(*obj,ZIPload);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(ZIPload);
}

EXPORT int init_ZIPload(OBJECT *obj)
{
	try
	{
		ZIPload *my = OBJECTDATA(obj,ZIPload);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(ZIPload);
}

EXPORT int isa_ZIPload(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,ZIPload)->isa(classname);
	} else {
		return 0;
	}
}


EXPORT TIMESTAMP sync_ZIPload(OBJECT *obj, TIMESTAMP t0)
{
	try
	{
		ZIPload *my = OBJECTDATA(obj, ZIPload);
		TIMESTAMP t1 = my->sync(obj->clock, t0);
		obj->clock = t0;
		return t1;
	}
	SYNC_CATCHALL(ZIPload);
}

/**@}**/

/** $Id: ZIPloadload.cpp $
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
		oclass = gl_register_class(module,"ZIPload",sizeof(ZIPload),PC_BOTTOMUP);
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
			PT_double,"duty_cycle",PADDR(phi),PT_DESCRIPTION, "duty cycle of the device",
			//PT_double,"diversity_of_population",PADDR(PHI),PT_DESCRIPTION, "diversity of a population of devices",
			PT_double,"demand_rate[1/h]",PADDR(eta),PT_DESCRIPTION, "consumer demand rate that prematurely turns on a device or population",
			//PT_double,"effective_rate[K/h]",PADDR(rho),PT_DESCRIPTION, "effect rate at which devices heats up or cools down under consumer demand",
			PT_double,"nominal_power",PADDR(nominal_power),PT_DESCRIPTION, "the rated amount of power demanded by devices that are on",
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

	demand_response_mode = false;
	ron = roff = -1;
	phi = eta = 0.2;	
	next_time = 0;

	//Default values of other properties
	power_pf = current_pf = impedance_pf = 1.0;

	load.voltage_factor = 1.0; // assume 'even' voltage, initially
	return res;
}

int ZIPload::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (demand_response_mode == true)
	{
		gl_warning("zipload_init: The demand response zipload is an experimental model at this time.");
		
		// Initial error checks
		if (abs(eta) > 1)
			GL_THROW("zipload_init: demand_rate (eta) must be between -1 and 1.");
		if (phi <= 0 || phi >= 1)
			GL_THROW("zipload_init: duty_cycle (phi) must be between (and not include) 0 and 1.");
		
		
		// Set up the buffers and perform some error checks
		if (L > 0)
			drm.nbins = L;
		else
			GL_THROW("zipload_init: thermostatic_control_range (L) must be greater than 0.");

		drm.off = (double*)malloc(sizeof(double)*L);
		drm.on = (double*)malloc(sizeof(double)*L);
		if (drm.off==NULL || drm.on==NULL)
			GL_THROW("zipload_init: memory allocation error.  Please report this error.");

		/* setup the initial population */
		if (ron == -1 || roff == -1)
		{
			if (phi <= 0.5)
			{
				roff = phi/(1-phi);
				ron = 1;
				gl_warning("ron or roff was not set.  Using phi to calculate.");
			}
			else
			{
				roff = 1;
				ron = (1-phi)/phi;
				gl_warning("ron or roff was not set.  Using phi to calculate.");
			}
		}
		else
			phi = roff / (ron + roff);

		if (roff < 0 || ron < 0)
			GL_THROW("zipload_init: rate_of_cooling and rate_of_heating must be greater than or equal to 0.");

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

		nominal_power = base_power * N_on / N;
		double R = ron;
		int dt = 0;
		if (ron < roff)
			R = roff;
		dt = int(1/R * 3600);

		next_time = t1 + dt;
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
		double demand_power = base_power;
		
		if (demand_response_mode == true)
			demand_power = nominal_power;

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

		//Update power factor, just in case
		angleval = load.total.Arg();
		load.power_factor = (angleval < 0) ? -1.0 * cos(angleval) : cos(angleval);

		//Determine the heat contributions - percentage of real power
		load.heatgain = load.total.Re() * load.heatgain_fraction * BTUPHPKW;
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

	if (next_time < t2)
		t2 = next_time;
	return t2;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_ZIPload(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(ZIPload::oclass);
	if (*obj!=NULL)
	{
		ZIPload *my = OBJECTDATA(*obj,ZIPload);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_ZIPload(OBJECT *obj)
{
	ZIPload *my = OBJECTDATA(obj,ZIPload);
	return my->init(obj->parent);
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
	ZIPload *my = OBJECTDATA(obj, ZIPload);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/

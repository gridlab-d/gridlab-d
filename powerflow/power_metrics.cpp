/** $Id: power_metrics.cpp 2010-06-14 15:00:00Z d3x593 $
	Copyright (C) 2009 Battelle Memorial Institute
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>

using namespace std;

#include "power_metrics.h"

//////////////////////////////////////////////////////////////////////////
// power_metrics CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* power_metrics::oclass = NULL;

CLASS* power_metrics::pclass = NULL;

power_metrics::power_metrics(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"power_metrics",sizeof(power_metrics),0x00);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		
		if(gl_publish_variable(oclass,
			PT_double, "SAIFI", PADDR(SAIFI),
			PT_double, "SAIDI", PADDR(SAIDI),
			PT_double, "CAIDI", PADDR(CAIDI),
			PT_double, "ASAI", PADDR(ASAI),
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
		gl_publish_function(oclass, "calc_metrics", (FUNCTIONADDR)calc_pfmetrics);
		gl_publish_function(oclass,	"reset_interval_metrics", (FUNCTIONADDR)reset_pfinterval_metrics);
		gl_publish_function(oclass,	"reset_annual_metrics", (FUNCTIONADDR)reset_pfannual_metrics);
    }
}

int power_metrics::isa(char *classname)
{
	return strcmp(classname,"power_metrics")==0;
}

int power_metrics::create(void)
{
	//Initialize metrics and storage variables
	SAIFI = 0.0;
	SAIDI = 0.0;
	CAIDI = 0.0;
	ASAI = 0.0;

	num_cust_interrupted = 0;
	num_cust_total = 0;
	outage_length = 0;
	outage_length_normalized = 0.0;

	//intermediate variables
	SAIFI_num = 0.0;
	SAIDI_num = 0.0;
	ASAI_num = 0.0;

	//Mapping variable
	fault_check_object_lnk = NULL;

	return 1;
}

int power_metrics::init(OBJECT *parent)
{
	//Nothing in here - all variables are outputs for now, so if user overrode them, we'll override them back!
	return 1;
}

//ASAI Calculation - Average Service Availability Index
//ASAI = ((total number of customers * number of hours/year) - sum(restoration time per event * number of customers interrupted per event)) / (total number of customers * number of hours/year)
double power_metrics::calc_ASAI(void)
{
	double calcval, yearhours;

	if (num_cust_total != 0)
	{
		yearhours = num_cust_total * 8760.0;	//Nt * hours/year
		ASAI_num += ((double)(outage_length * num_cust_interrupted))/3600.0;	//In hours (TIMESTAMP assumed to be seconds-level)

		calcval = (yearhours - ASAI_num) / yearhours;
	}
	else
	{
		calcval = 0.0;	//No customers = no index
	}

	return calcval;
}

//CAIDI Calculation - Customer Average Interruption Duration Index
//CAIDI = SAIDI / SAIFI
double power_metrics::calc_CAIDI(void)
{
	double calcval;
	if (SAIFI != 0.0)
	{
		calcval = SAIDI / SAIFI;
	}
	else
	{
		calcval = 0.0;
	}

	return calcval;
}

//SAIDI Calculation - System Average Interruption Duration Index
//SAIDI = sum(restoration time per event * number of customers interrupted per event) / total number of customers
double power_metrics::calc_SAIDI(void)
{
	double calcval;

	if (num_cust_total != 0.0)
	{
		SAIDI_num += outage_length_normalized * ((double)(num_cust_interrupted));
		calcval = SAIDI_num / num_cust_total;
	}
	else
	{
		calcval = 0.0;
	}

	return calcval;
}

//SAIFI Calculation - System Average Interruption Frequency Index
//SAIFI = sum(number of customers interrupted per event) / total number of customers
double power_metrics::calc_SAIFI(void)
{
	double calcval;

	if (num_cust_total != 0.0)
	{
		SAIFI_num += (double)(num_cust_interrupted);
		calcval = SAIFI_num / ((double)(num_cust_total));
	}
	else
	{
		calcval = 0.0;
	}

	return calcval;
}

//Class function to calculate metrics - makes memory exchanges easier and such
void power_metrics::perform_rel_calcs(int number_int, int total_cust, TIMESTAMP rest_time_val, TIMESTAMP base_time_val)
{
	//Assign relevant quantities first
	num_cust_interrupted = number_int;
	num_cust_total = total_cust;
	outage_length = rest_time_val;
	outage_length_normalized = ((double)(rest_time_val) / (double)(base_time_val));

	//Perform calculations
	SAIFI = calc_SAIFI();
	SAIDI = calc_SAIDI();
	CAIDI = calc_CAIDI();
	ASAI = calc_ASAI();

}

//Class function to reset various metrics - makes memory exchanges easier
void power_metrics::reset_metrics_variables(bool annual_metrics)
{
	//"interval" get reset no matter what
	SAIFI_num = 0.0;
	SAIDI_num = 0.0;

	//Now check annual
	if (annual_metrics == true)
	{
		ASAI_num = 0.0;
	}
}

//Function to check for fault_check object (needed) and to make sure it is in a proper mode
void power_metrics::check_fault_check(void)
{
	if (fault_check_object_lnk == NULL)
	{
		if (fault_check_object == NULL)
		{
			GL_THROW("power_metrics failed to map fault_check object!");
			/*  TROUBLESHOOT
			power_metrics encountered an error while trying to map the fault_check
			object's location.  Please ensure that a fault_check object exists in the
			system and that fault_check is defined before the relevant metrics object.
			For best results, put the fault_check object right after all module definitions.
			If the error persists, please submit your code and a bug report using the trac website.
			*/
		}
		else	//One is mapped, let's check it
		{
			fault_check_object_lnk = OBJECTDATA(fault_check_object,fault_check);

			//Make sure it mapped
			if (fault_check_object_lnk == NULL)
			{
				GL_THROW("power_metrics failed to map fault_check object!");
				//Defined above
			}

			//Make sure it is in ONCHANGE or ALL mode
			if ((fault_check_object_lnk->fcheck_state!=fault_check::ALLT) && (fault_check_object_lnk->fcheck_state!=fault_check::ONCHANGE))
			{
				GL_THROW("fault_check must be in the proper mode for reliabilty to work!");
				/*  TROUBLESHOOT
				To properly work with reliability, fault_check must be in the ALL or ONCHANGE
				check_mode.  Please set it appropriately and try again.
				*/
			}

			//Once we're sure it is in the proper mode, set the reliability flag so it doesn't stop us dead
			fault_check_object_lnk->reliability_mode = true;

		}//Mapping of fault_check object
	}//end fault_check_object_lnk is NULL
	//Defaulted else - must already be populated
}

//Exported function for reliability module to call to calculate metrics
EXPORT int calc_pfmetrics(OBJECT *callobj, OBJECT *calcobj, int number_int, int total_customers, TIMESTAMP rest_time_val, TIMESTAMP base_time_val)
{
	//Link to it
	power_metrics *pmetrics_obj;
	pmetrics_obj = OBJECTDATA(calcobj,power_metrics);

	//Perform the calculation - pass into a class function so variables can be written willy-nilly
	pmetrics_obj->perform_rel_calcs(number_int, total_customers, rest_time_val, base_time_val);

	return 1;	//Success!
}

//Exported function for reliability module to call to reset "interval (base_time_val) metrics
EXPORT int reset_pfinterval_metrics(OBJECT *callobj, OBJECT *calcobj)
{
	//Link to us
	power_metrics *pmetrics_obj;
	pmetrics_obj = OBJECTDATA(calcobj,power_metrics);

	//Make sure fault_check is present and in a viable mode
	pmetrics_obj->check_fault_check();

	//Perform the calculation - class function for ease
	pmetrics_obj->reset_metrics_variables(false);
		
	return 1;	//Always successful in theory
}

//Exported function for reliability module to call to reset annual metrics
EXPORT int reset_pfannual_metrics(OBJECT *callobj, OBJECT *calcobj)
{
	//Link to us
	power_metrics *pmetrics_obj;
	pmetrics_obj = OBJECTDATA(calcobj,power_metrics);

	//Perform the calculation - class function for ease
	pmetrics_obj->reset_metrics_variables(true);

	return 1;	//Always successful - theoretically
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: power_metrics
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_power_metrics(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(power_metrics::oclass);
		if (*obj!=NULL)
		{
			power_metrics *my = OBJECTDATA(*obj,power_metrics);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("%s %s (id=%d): %s", (*obj)->name?(*obj)->name:"unnamed", (*obj)->oclass->name, (*obj)->id, msg);
		return 0;
	}
	return 0;
}

EXPORT int init_power_metrics(OBJECT *obj, OBJECT *parent)
{
	try {
			return OBJECTDATA(obj,power_metrics)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("%s %s (id=%d): %s", obj->name?obj->name:"unnamed", obj->oclass->name, obj->id, msg);
		return 0;
	}
	return 1;
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_power_metrics(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}
EXPORT int isa_power_metrics(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,power_metrics)->isa(classname);
}

/**@}**/

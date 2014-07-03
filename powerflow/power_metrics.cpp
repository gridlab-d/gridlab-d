/** $Id: power_metrics.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute

	The metric analysis object reports reliability metrics for a system by observing the
	duration and number of conditions when meter objects are not normal.  The
	following metrics are calculated annual and reported to the \p report_file

	-# \e SAIFI (System average interrupts frequency index) indicates how often 
	   the average customer experiences a system interruption over a predefined period
	   of time.  This is calculated using the formula

	   \f[ SAIFI = \frac{\Sigma N_{customer\_interruptions}}{N_{customers}} \f]

    -# \e SAIDI (System average interrupts duration index) indicates the total
		duration of interrupt for the average customer during a predefined period of
		time.  This is calculated using the formula

	   \f[ SAIDI = \frac{\Sigma T_{customer\_interruptions}}{N_{customers}} \f]

    -# \e CAIDI (Customer average interrupt duration index) represents the average
		time required to restore service.  This is calculated using the formula

		\f[ CAIDI = \frac{\Sigma T_{customer\_interruptions}}{N_{customers\_interrupted}} \f]
    
    -# \e MAIFI (Momentary average interruption frequency index) represents the average
		frequency of momentary interruptions.  This is calculated using the formula

		\f[ MAIFI = \frac{\Sigma T_{{momentary customer}\_interruptions}}{N_{customers}} \f]

	See <b>IEEE Std 1366-2003</b> <i>IEEE Guide for Electric Power Distribution Reliability Indices</i>
	for more information on how to compute distribution system reliability indices.

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

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
		if (oclass==NULL)
			throw "unable to register class power_metrics";
		else
			oclass->trl = TRL_QUALIFIED;

		if(gl_publish_variable(oclass,
			PT_double, "SAIFI", PADDR(SAIFI),PT_DESCRIPTION, "Displays annual SAIFI values as per IEEE 1366-2003",
			PT_double, "SAIFI_int", PADDR(SAIFI_int),PT_DESCRIPTION, "Displays SAIFI values over the period specified by base_time_value as per IEEE 1366-2003",
			PT_double, "SAIDI", PADDR(SAIDI),PT_DESCRIPTION, "Displays annual SAIDI values as per IEEE 1366-2003",
			PT_double, "SAIDI_int", PADDR(SAIDI_int),PT_DESCRIPTION, "Displays SAIDI values over the period specified by base_time_value as per IEEE 1366-2003",
			PT_double, "CAIDI", PADDR(CAIDI),PT_DESCRIPTION, "Displays annual CAIDI values as per IEEE 1366-2003",
			PT_double, "CAIDI_int", PADDR(CAIDI_int),PT_DESCRIPTION, "Displays SAIDI values over the period specified by base_time_value as per IEEE 1366-2003",
			PT_double, "ASAI", PADDR(ASAI),PT_DESCRIPTION, "Displays annual AISI values as per IEEE 1366-2003",
			PT_double, "ASAI_int", PADDR(ASAI_int),PT_DESCRIPTION, "Displays AISI values over the period specified by base_time_value as per IEEE 1366-2003",
			PT_double, "MAIFI", PADDR(MAIFI),PT_DESCRIPTION, "Displays annual MAIFI values as per IEEE 1366-2003",
			PT_double, "MAIFI_int", PADDR(MAIFI_int),PT_DESCRIPTION, "Displays MAIFI values over the period specified by base_time_value as per IEEE 1366-2003",
			PT_double, "base_time_value[s]", PADDR(stat_base_time_value), PT_DESCRIPTION,"time period over which _int values are claculated",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
			if (gl_publish_function(oclass, "calc_metrics", (FUNCTIONADDR)calc_pfmetrics)==NULL)
				GL_THROW("Unable to publish metrics calculation function");
			if (gl_publish_function(oclass,	"reset_interval_metrics", (FUNCTIONADDR)reset_pfinterval_metrics)==NULL)
				GL_THROW("Unable to publish interval metrics reset function");
			if (gl_publish_function(oclass,	"reset_annual_metrics", (FUNCTIONADDR)reset_pfannual_metrics)==NULL)
				GL_THROW("Unable to publish annual metrics reset function");
			if (gl_publish_function(oclass,	"init_reliability", (FUNCTIONADDR)init_pf_reliability_extra)==NULL)
				GL_THROW("Unable to publish powerflow reliability initialization function");
			if (gl_publish_function(oclass,	"logfile_extra", (FUNCTIONADDR)logfile_extra)==NULL)
				GL_THROW("Unable to publish powerflow reliability metrics extra header function");
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
	ASAI = 1.0;	//Start at full reliability
	MAIFI = 0.0;

	//Same goes for interval stats
	SAIFI_int = 0.0;
	SAIDI_int = 0.0;
	CAIDI_int = 0.0;
	ASAI_int = 1.0;	//Start at full reliability
	MAIFI_int = 0.0;

	num_cust_interrupted = 0;
	num_cust_total = 0;
	outage_length = 0;
	outage_length_normalized = 0.0;

	//basis times for statistics - defaults to minutes
	stat_base_time_value = 60;

	//intermediate variables
	SAIFI_num = 0.0;
	SAIDI_num = 0.0;
	ASAI_num = 0.0;
	MAIFI_num = 0.0;
	SAIFI_num_int = 0.0;
	SAIDI_num_int = 0.0;
	ASAI_num_int = 0.0;
	MAIFI_num_int = 0.0;

	//Mapping variables
	fault_check_object_lnk = NULL;
	rel_metrics = NULL;

	//Extra variable (recloser count)
	Extra_PF_Data = 0.0;

	return 1;
}

int power_metrics::init(OBJECT *parent)
{
	//Nothing to do in here - all variables are outputs for now, so if user overrode them, we'll override them back!
	return 1;
}

//ASAI Calculation - Average Service Availability Index
//ASAI = ((total number of customers * number of hours/year) - sum(restoration time per event * number of customers interrupted per event)) / (total number of customers * number of hours/year)
void power_metrics::calc_ASAI(void)
{
	double yearhours;

	if (num_cust_total != 0)
	{
		yearhours = num_cust_total * 8760.0;	//Nt * hours/year
		ASAI_num += ((double)(outage_length * num_cust_interrupted))/3600.0;	//In hours (TIMESTAMP assumed to be seconds-level)
		ASAI_num_int += ((double)(outage_length * num_cust_interrupted))/3600.0;	//In hours (TIMESTAMP assumed to be seconds-level)

		ASAI = (yearhours - ASAI_num) / yearhours;
		ASAI_int = (yearhours - ASAI_num_int) / yearhours;
	}
	else
	{
		ASAI = 0.0;	//No customers = no index
		ASAI_int = 0.0;	//No customers = no index
	}
}

//CAIDI Calculation - Customer Average Interruption Duration Index
//CAIDI = SAIDI / SAIFI
void power_metrics::calc_CAIDI(void)
{
	if (SAIFI != 0.0)
	{
		CAIDI = SAIDI / SAIFI;
	}
	else
	{
		CAIDI = 0.0;
	}

	if (SAIFI_int != 0.0)
	{
		CAIDI_int = SAIDI_int / SAIFI_int;
	}
	else
	{
		CAIDI_int = 0.0;
	}
}

//SAIDI Calculation - System Average Interruption Duration Index
//SAIDI = sum(restoration time per event * number of customers interrupted per event) / total number of customers
void power_metrics::calc_SAIDI(void)
{
	if (num_cust_total != 0)
	{
		SAIDI_num += outage_length_normalized * ((double)(num_cust_interrupted));
		SAIDI_num_int += outage_length_normalized * ((double)(num_cust_interrupted));
		
		SAIDI = SAIDI_num / ((double)num_cust_total);
		SAIDI_int = SAIDI_num_int / ((double)num_cust_total);
	}
	else
	{
		SAIDI = 0.0;
		SAIDI_int = 0.0;
	}
}

//SAIFI Calculation - System Average Interruption Frequency Index
//SAIFI = sum(number of customers interrupted per event) / total number of customers
void power_metrics::calc_SAIFI(void)
{
	if (num_cust_total != 0)
	{
		SAIFI_num += (double)(num_cust_interrupted);
		SAIFI_num_int += (double)(num_cust_interrupted);

		SAIFI = SAIFI_num / ((double)(num_cust_total));
		SAIFI_int = SAIFI_num_int / ((double)(num_cust_total));
	}
	else
	{
		SAIFI = 0.0;
		SAIFI_int = 0.0;
	}
}

//MAIFI Calculation - Momentary Average Interruption Frequency Index
//MAIFI = sum(number momentary interrupts * number customers affected) / total number of customers
void power_metrics::calc_MAIFI(void)
{
	if (num_cust_total != 0)
	{
		MAIFI_num += Extra_PF_Data*num_cust_momentary_interrupted;
		MAIFI_num_int += Extra_PF_Data*num_cust_momentary_interrupted;

		//See if it was a "momentary" outage - if so, add extra data
		if (outage_length < 300)	//Less than 5 minutes - "normal" failures are considered momentary
		{
			MAIFI_num += (double)(num_cust_interrupted);
			MAIFI_num_int += (double)(num_cust_interrupted);
		}

		MAIFI = MAIFI_num / ((double)(num_cust_total));
		MAIFI_int = MAIFI_num_int / ((double)(num_cust_total));
	}
	else
	{
		MAIFI = 0.0;
		MAIFI_int = 0.0;
	}
}

//Class function to calculate metrics - makes memory exchanges easier and such
void power_metrics::perform_rel_calcs(int number_int, int number_int_secondary, int total_cust, TIMESTAMP rest_time_val, TIMESTAMP base_time_val)
{
	//Assign relevant quantities first
	num_cust_interrupted = number_int;
	num_cust_momentary_interrupted = number_int_secondary;
	num_cust_total = total_cust;
	outage_length = rest_time_val;
	outage_length_normalized = ((double)(rest_time_val) / stat_base_time_value);

	//Perform calculations
	if (outage_length > 300)	//"Momentary" by IEEE standards
	{
		calc_SAIFI();
		calc_SAIDI();
		calc_CAIDI();
		calc_ASAI();
	}

	//Momentary gets done all times - just in case a "sustained" outage had momentary connotations
	calc_MAIFI();
}

//Class function to reset various metrics - makes memory exchanges easier
void power_metrics::reset_metrics_variables(bool annual_metrics)
{
	//"interval" get reset no matter what
	SAIFI_num_int = 0.0;
	SAIDI_num_int = 0.0;
	MAIFI_num_int = 0.0;

	//Reset interval outputs as well
	SAIFI_int = 0.0;
	SAIDI_int = 0.0;
	CAIDI_int = 0.0;
	ASAI_int = 1.0;	//Start at full reliability
	MAIFI_int = 0.0;
	
	//Now check annual
	if (annual_metrics == true)	//ASAI for whole simulation should really be reset here as well, but no rolling window implemented
	{
		ASAI_num_int = 0.0;	//Default ASAI to perfect reliability (no interruptions)
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

			//While we're in here, see if we can do secondary events - otherwise warn us
			if (fault_check_object_lnk->rel_eventgen == NULL)
			{
				gl_warning("No eventgen object mapped up to %s, unscheduled faults are not allowed",fault_check_object->name);
				/*  TROUBLESHOOT
				No eventgen object is specified on the fault_check field eventgen_object.  Without this specified, "unscheduled"
				faults can not occur and will simply stop the simulation.  This includes fuses blowing and "non-faulted" switch
				operations.
				*/
			}
		}//Mapping of fault_check object
	}//end fault_check_object_lnk is NULL
	//Defaulted else - must already be populated
}

//Exported function for reliability module to call to calculate metrics
EXPORT int calc_pfmetrics(OBJECT *callobj, OBJECT *calcobj, int number_int, int number_int_secondary, int total_customers, TIMESTAMP rest_time_val, TIMESTAMP base_time_val)
{
	//Link to it
	power_metrics *pmetrics_obj;
	pmetrics_obj = OBJECTDATA(calcobj,power_metrics);

	//Perform the calculation - pass into a class function so variables can be written willy-nilly
	pmetrics_obj->perform_rel_calcs(number_int, number_int_secondary, total_customers, rest_time_val, base_time_val);

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

//Exported function to initialize extra reliability variables - in this case, only a recloser reclose counter
EXPORT void *init_pf_reliability_extra(OBJECT *myhdr, OBJECT *callhdr)
{
	void *Data_Index;
	
	//Link to us
	power_metrics *pmetrics_obj;
	pmetrics_obj = OBJECTDATA(myhdr,power_metrics);

	//Map calling object
	pmetrics_obj->rel_metrics = callhdr;

	//Map the proper data index
	Data_Index = (void *)&(pmetrics_obj->Extra_PF_Data);

	//Check success
	if (Data_Index != NULL)
		return Data_Index;	//Success
	else
		return 0;	//Failure
}

//Exposed function for extra reliability log file information
EXPORT int logfile_extra(OBJECT *myhdr, char *BufferArray)
{
	double time_interval_val;

	//Map us
	power_metrics *pmetrics_obj;
	pmetrics_obj = OBJECTDATA(myhdr,power_metrics);

	//Get the interval data
	time_interval_val = pmetrics_obj->stat_base_time_value;

	//See what it is and print appropriately
	if (time_interval_val == 1.0)	//Seconds
		sprintf(BufferArray,"1366 metrics based around 1-second base (outage-seconds)\n\n");
	else if (time_interval_val == 60.0)	//Minutes
		sprintf(BufferArray,"1366 metrics based around 1-minute base (outage-minutes)\n\n");
	else if (time_interval_val == 3600.0)	//Hours
		sprintf(BufferArray,"1366 metrics based around 1-hour base (outage-hours)\n\n");
	else if (time_interval_val == 86400.0)	//Days
		sprintf(BufferArray,"1366 metrics based around 1-day base (outage-days)\n\n");
	else if (time_interval_val == 31536000.0)	//Years
		sprintf(BufferArray,"1366 metrics based around 1-year base (outage-years)\n\n");
	else	//Some "odd" interval - represent in seconds
		sprintf(BufferArray,"1366 metrics uses time base of %.0f seconds\n\n",time_interval_val);

	return 1;	//Always succeeds
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
		else
			return 0;
	}
	CREATE_CATCHALL(power_metrics);
}

EXPORT int init_power_metrics(OBJECT *obj, OBJECT *parent)
{
	try {
		return OBJECTDATA(obj,power_metrics)->init(parent);
	}
	INIT_CATCHALL(power_metrics);
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

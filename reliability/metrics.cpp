/** $Id: metrics.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file metrics.cpp
	@class metrics Metrics analysis
	@ingroup reliability

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
    
	-# \e CAIFI (Customer average interruption frequency index) gives the average freqeuncy
		of sustained interruptions.  The customer is counted once, regardless of the number
		of times interrupted.  This is calculated using the formula

		\f[ CAIFI = \frac{\Sigma N_{customers\_interrupted}}{N_{customers\_interrupted}} \f]

	See <b>IEEE Std 1366-2003</b> <i>IEEE Guide for Electric Power Distribution Reliability Indices</i>
	for more information on how to compute distribution system reliability indices.

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "metrics.h"

CLASS *metrics::oclass = NULL;
metrics *metrics::defaults = NULL;

bool metrics::report_event_log = true; /* dumps detailed event log by default */

static PASSCONFIG clockpass = PC_POSTTOPDOWN;

/* Class registration is only called once to register the class with the core */
metrics::metrics(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"metrics",sizeof(metrics),PC_POSTTOPDOWN);
		if (oclass==NULL)
			throw "unable to register class metrics";
		else
			oclass->trl = TRL_DEMONSTRATED;

		if (gl_publish_variable(oclass,
			PT_char1024, "report_file", PADDR(report_file),
			PT_char1024, "customer_group", PADDR(customer_group),
			PT_object, "module_metrics_object", PADDR(module_metrics_obj),
			PT_char1024, "metrics_of_interest", PADDR(metrics_oi),
			PT_double, "metric_interval[s]", PADDR(metric_interval_dbl),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

/* Object creation is called once for each object that is created by the core */
int metrics::create(void)
{
	customer_group[0] = '\0';
	module_metrics_obj = NULL;
	metrics_oi[0] = '\0';
	metric_interval_dbl = 0.0;

	//Internal variables
	num_indices = 0;
	CalcIndices = NULL;
	next_metric_interval = 0;
	metric_interval = 0;
	CustomerCount = 0;
	Customers = NULL;
	curr_time = TS_NEVER;	//Flagging value
	metric_interval_event_count = 0;
	annual_interval_event_count = 0;
	metric_equal_annual = false;

	reset_interval_func = NULL;
	reset_annual_func = NULL;
	compute_metrics = NULL;
	
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int metrics::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	int index, indexa, returnval;
	char1024 work_metrics;
	char *startVal, *endVal, *workVal;
	char1024 workbuffer;
	FILE *FPVal;
	FINDLIST *CandidateObjs;
	OBJECT *temp_obj;
	bool *temp_bool;

	//Ensure our "module metrics" object is populated
	if (module_metrics_obj == NULL)
	{
		GL_THROW("Please specify a module metrics object for metrics:%s",hdr->name);
		/*  TROUBLESHOOT
		To operate properly, the metrics object must have the corresponding module's
		metrics calculator linked.  If this object is missing, metrics has no idea
		what to calculate and will not proceed.
		*/
	}

	//Figure out how many indices we should be finding
	index = 0;

	while ((metrics_oi[index] != '\0') && (index < 1024))
	{
		if (metrics_oi[index] == ',')
		{
			num_indices++;	//Increment counter
		}

		index++;	//Increment pointer
	}

	//Make sure we didn't blow the top off
	if (index == 1024)
	{
		GL_THROW("Maximum length exceeded on metrics_of_interest in metrics:%s",hdr->name);
		/*  TROUBLESHOOT
		While parsing the metrics_of_interest list, the maximum length (1024 characters) was
		reached.  This can cause undesired behavior.  Ensure the metrics list is within this
		character count.  If it is, please try again.  If the error persists, please submit
		your code and a bug report via the trac website.
		*/
	}

	//See if at least one was found.  If it was, that means there are 2 (it counts commas)
	if (num_indices >= 1)
		num_indices++;
	
	//See if we were a solitary one - if so, increment accordingly
	if ((num_indices == 0) && (index > 1))
		num_indices = 1;

	//Make sure at least one was found
	if (num_indices == 0)
	{
		GL_THROW("No indices of interest specified for metrics:%s",hdr->name);
		/*  TROUBLESHOOT
		No indices were found to read.  Please specify the proper indices and try again.
		*/
	}

	//Malloc us up!
	CalcIndices = (INDEXARRAY*)gl_malloc(num_indices * sizeof(INDEXARRAY));

	//Make sure it worked
	if (CalcIndices == NULL)
	{
		GL_THROW("Failure to allocate indices memory in metrics:%s",hdr->name);
		/*  TROUBLESHOOT
		While allocating the storage matrix for the indices to calculate, an error occurred.
		Please try again.  If the error persists, please submit you code and a bug report
		using the trac website.
		*/
	}

	//Initialize these, just in case
	for (index=0; index<num_indices; index++)
	{
		CalcIndices[index].MetricLoc = NULL;	//No address by default
		
		for (indexa=0; indexa<257; indexa++)	//+1 due to end \0
			CalcIndices[index].MetricName[indexa]='\0';
	}

	//Populate it up - copy it first so we don't destroy the original
	memcpy(work_metrics,metrics_oi,1024*sizeof(char));

	//Set initial pointers
	startVal = work_metrics;
	endVal = work_metrics;

	//Loop through and find them
	for (index=0; index<num_indices; index++)	//Loop through
	{
		//Find the next comma or end point
		while ((*endVal != ',') && (*endVal != '\0'))
		{
			endVal++;
		}

		//Replace us (comma) with EOS (\0)
		*endVal='\0';

		//Copy us into the structure
		workVal = startVal;
		indexa = 0;
		while ((workVal<=endVal) && (indexa < 256))
		{
			//Copy the value in
			CalcIndices[index].MetricName[indexa] = *workVal;

			//Increment pointers
			indexa++;
			workVal++;
		}

		//Now update pointers appropriately and proceed
		endVal++;
		startVal = endVal;
		
		//Now try to find this variable
		CalcIndices[index].MetricLoc = get_metric(module_metrics_obj,CalcIndices[index].MetricName);

		//Make sure it worked
		if (CalcIndices[index].MetricLoc == NULL)
		{
			GL_THROW("Unable to find metric %s in object %s for metric:%s",CalcIndices[index].MetricName,module_metrics_obj->name,hdr->name);
			/*  TROUBLESHOOT
			While attempting to map out a reliability metric, the desired metric was not found.  Please check the variable
			name and ensure the metric is being published in the module metrics object and try again.  If the error persists,
			please submit your code and a bug report to the trac website.
			*/
		}
	}//end metric traversion
	
	//Map our reset functions for ease
	reset_interval_func = (FUNCTIONADDR)(gl_get_function(module_metrics_obj,"reset_interval_metrics"));
	
	//Make sure it worked
	if (reset_interval_func == NULL)
	{
		GL_THROW("Failed to map interval reset in metrics object %s for metrics:%s",module_metrics_obj->name,hdr->name);
		/*  TROUBLESHOOT
		While attempting to map the interval statistics reset function, the metrics object encountered a problem.
		Please make sure the module metrics object supports a "reset_interval_metrics" function.  If so, please try again.
		If the error persists, please submit your code and a bug report using the trac website.
		*/
	}

	reset_annual_func = (FUNCTIONADDR)(gl_get_function(module_metrics_obj,"reset_annual_metrics"));

	//Make sure it worked
	if (reset_annual_func == NULL)
	{
		GL_THROW("Failed to map annual reset in metrics object %s for metrics:%s",module_metrics_obj->name,hdr->name);
		/*  TROUBLESHOOT
		While attempting to map the annual statistics reset function, the metrics object encountered a problem.
		Please make sure the module metrics object supports a "reset_interval_metrics" function.  If so, please try again.
		If the error persists, please submit your code and a bug report using the trac website.
		*/
	}

	compute_metrics = (FUNCTIONADDR)(gl_get_function(module_metrics_obj,"calc_metrics"));

	//Make sure it worked
	if (compute_metrics == NULL)
	{
		GL_THROW("Failed to map metric computation function in metrics object %s for metrics:%s",module_metrics_obj->name,hdr->name);
		/*  TROUBLESHOOT
		While attempting to map the function to compute the desired metrics, the metrics object encountered a problem.
		Please make sure the module metrics object supports a "calc_metrics" function.  If so, please try again.
		If the error persists, please submit your code and a bug report using the trac website.
		*/
	}

	//Call the resets - interval
	returnval = ((int (*)(OBJECT *, OBJECT *))(*reset_interval_func))(hdr,module_metrics_obj);

	if (returnval != 1)	//See if it failed
	{
		GL_THROW("Failed to reset interval metrics for %s by metrics:%s",module_metrics_obj->name,hdr->name);
		/*  TROUBLESHOOT
		The metrics object encountered an error while attempting to reset the interval statistics variables.
		Please try again.  If the error persists, submit your code and a bug report via the trac website.
		*/
	}

	//Call the resets - annual
	returnval = ((int (*)(OBJECT *, OBJECT *))(*reset_annual_func))(hdr,module_metrics_obj);

	if (returnval != 1)	//See if it failed
	{
		GL_THROW("Failed to reset annual metrics for %s by metrics:%s",module_metrics_obj->name,hdr->name);
		/*  TROUBLESHOOT
		The metrics object encountered an error while attempting to reset the annual statistics variables.
		Please try again.  If the error persists, submit your code and a bug report via the trac website.
		*/
	}

	//Convert our calculation interval to a timestamp
	metric_interval = (TIMESTAMP)metric_interval_dbl;

	//See if it is a year - flag appropriately
	if (metric_interval == 31536000)
		metric_equal_annual = true;

	//Make sure we have a file name provided
	if (report_file[0] == '\0')	//None specified
	{
		GL_THROW("Please specify a proper report file name if you would like an output file from metrics:%s",hdr->name);
		/*  TROUBLESHOOT
		While attempting to write the report file, an invalid file name was provided.  Please provide a valid file name
		and try again.
		*/
	}

	//Open the file to clear it
	FPVal = fopen(report_file,"wt");

	//Make sure it worked
	if (FPVal == NULL)
	{
		GL_THROW("Unable to create the report file '%s' for metrics:%s",report_file,hdr->name);
		/*  TROUBLESHOOT
		While attempting to write the metrics output file, an error occurred.  Please make sure you
		have write permissions at that location and try again.  If the error persists, please submit
		your code and a bug report using the trac website.
		*/
	}

	//It must exist - write the typical header nonsense
	fprintf(FPVal,"Reliability report for %s\n", gl_global_getvar("modelname",workbuffer,(1025*sizeof(char))));

	//Find our lucky candidate objects
	CandidateObjs = gl_find_objects(FL_GROUP,customer_group);
	if (CandidateObjs==NULL)
	{
		GL_THROW("Failure to find devices for %s specified as: %s",hdr->name,customer_group);
		/*  TROUBLESHOOT
		While attempting to populate the list of devices to check for reliability metrics, the metrics
		object failed to find any desired objects.  Please make sure the objects exist and try again.
		If the bug persists, please submit your code using the trac website.
		*/
	}

	//Do a zero-find check as well
	if (CandidateObjs->hit_count == 0)
	{
		GL_THROW("Failure to find devices for %s specified as: %s",hdr->name,customer_group);
		//Defined above
	}

	//Pull the count
	CustomerCount = CandidateObjs->hit_count;

	//Make us an array!
	Customers = (CUSTARRAY*)gl_malloc(CustomerCount*sizeof(CUSTARRAY));

	//Make sure it worked
	if (Customers == NULL)
	{
		GL_THROW("Failure to allocate customer list memory in metrics:%s",hdr->name);
		/*  TROUBLESHOOT
		While allocating the memory for the list of customers, GridLAB-D encountered a problem.
		Please try again.  If the error persists, please submit your code and a bug report via the
		trac website.
		*/
	}

	//Let's populate the beast now!
	temp_obj = NULL;
	for (index=0; index<CustomerCount; index++)
	{
		//Find the object
		temp_obj = gl_find_next(CandidateObjs, temp_obj);

		if (temp_obj == NULL)
		{
			GL_THROW("Failed to populate customer list in metrics: %s",hdr->name);
			/*  TROUBLESHOOT
			While populating the metrics customer list, an object failed to be
			located.  Please try again.  If the error persists, please submit your
			code and a bug report to the trac website.
			*/
		}

		Customers[index].CustomerObj = temp_obj;

		//Try to find our "outage" indicator and map its address
		temp_bool = get_outage_flag(temp_obj, "customer_interrupted");

		//make sure it found it
		if (temp_bool == NULL)
		{
			GL_THROW("Unable to find interrupted flag for customer object %s in metrics:%s",temp_obj->name,hdr->name);
			/*  TROUBLESHOOT
			While attempting to link to the 'customer interrupted' flag, an error occurred.  Please ensure the object
			supports being polled by reliability as a customer (customer_interrupted exists as a published property) and
			try again.  If the error persists, please submit your code and a bug report via the trac website.
			*/
		}

		//Write this value in
		Customers[index].CustInterrupted = temp_bool;

	}//end population loop

	//Free up list
	gl_free(CandidateObjs);

	//Write the customer count and header information to the file we have going
	fprintf(FPVal,"Number of customers = %d\n\n",CustomerCount);

	//Close the file handle
	fclose(FPVal);

	return 1; /* return 1 on success, 0 on failure - We're so awesome we always assume we work */
}

TIMESTAMP metrics::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	DATETIME dt;
	int returnval;
	OBJECT *hdr = OBJECTHDR(this);
	int valid = gl_localtime(t1, &dt);
	FILE *FPVal;

	//Initialization
	if (curr_time == TS_NEVER)
	{
		//Update time value trackers
		next_metric_interval = t1 + metric_interval;
		next_annual_interval = t1 + 31536000;	//t1 + 365 days of seconds

		//Outputs in CSV format - solves issue of column alignment - assuming we want event log
		if (report_event_log == true)
		{
			//Open the file
			FPVal = fopen(report_file,"at");

			//Figure out when we are, roughly (use t1 here)
			gl_localtime(t1,&dt);

			//Write header stuffs
			fprintf(FPVal,"Events for year starting at %04d-%02d-%02d %02d:%02d:%02d\n",dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second);
			fprintf(FPVal,"Annual Event #,Metric Interval Event #,Starting DateTime (YYYY-MM-DD hh:mm:ss),Ending DateTime (YYYY-MM-DD hh:mm:ss),Object type,Object Name,Inducing Object,Fault type,Number customers affected\n");

			//Close the file handle
			fclose(FPVal);
		}

		//First run - t1 = t0 (t0 is zero)
		curr_time = t1;
	}

	//Only worry about checks once per timestep
	if (curr_time != t0)
	{
		//See if it is time to write an update
		if (t0 >= next_metric_interval)
		{
			//Update the interval
			next_metric_interval = t0 + metric_interval;

			//Reset the stat variables
			returnval = ((int (*)(OBJECT *, OBJECT *))(*reset_interval_func))(hdr,module_metrics_obj);

			if (returnval != 1)	//See if it failed
			{
				GL_THROW("Failed to reset interval metrics for %s by metrics:%s",module_metrics_obj->name,hdr->name);
				//Defined above
			}

			//Reset the counter
			metric_interval_event_count = 0;

			//Indicate a new interval is going on - if we aren't a year (otherwise it happens below)
			if ((report_event_log == true) && (metric_equal_annual == false))
			{
				//Open the file
				FPVal = fopen(report_file,"at");

				//Figure out when we are
				gl_localtime(t0,&dt);

				//Write header stuffs
				fprintf(FPVal,"\nNew Metric Interval started at %04d-%02d-%02d %02d:%02d:%02d\n\n",dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second);

				//Close the file handle
				fclose(FPVal);
			}
		}//End interval metrics update

		//See if we've exceeded a year
		if (t0 >= next_annual_interval)
		{
			next_annual_interval = t0 + 31536000;	//t0 + 365 days of seconds

			//Reset the stats
			returnval = ((int (*)(OBJECT *, OBJECT *))(*reset_annual_func))(hdr,module_metrics_obj);

			if (returnval != 1)	//See if it failed
			{
				GL_THROW("Failed to reset annual metrics for %s by metrics:%s",module_metrics_obj->name,hdr->name);
				//Defined above
			}

			//Reset the counter
			annual_interval_event_count = 0;

			//Indicate a new interval is going on
			if (report_event_log == true)
			{
				//Open the file
				FPVal = fopen(report_file,"at");

				//Figure out when we are
				gl_localtime(t0,&dt);

				//Write header stuffs
				fprintf(FPVal,"\nNew Annual Interval started at %04d-%02d-%02d %02d:%02d:%02d\n\n",dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second);

				//Close the file handle
				fclose(FPVal);
			}
		}//End annual update

		//Update tracking variable
		curr_time = t0;
	}

	//See who to return
	if (next_metric_interval < next_annual_interval)
		return -next_metric_interval;
	else
		return -next_annual_interval;
}

//Perform post-event analysis (update computations, write event file if necessary)
void metrics::event_ended(OBJECT *event_obj, OBJECT *fault_obj, TIMESTAMP event_start_time, TIMESTAMP event_end_time, char *fault_type, int number_customers_int)
{
	DATETIME start_dt, end_dt;
	TIMESTAMP outage_length;
	OBJECT *hdr = OBJECTHDR(this);
	int returnval;
	FILE *FPVal;

	//Determine the actual outage length (may be off due to iterations and such)
	outage_length = event_end_time - event_start_time;

	//Perform the calculation
	returnval = ((int (*)(OBJECT *, OBJECT *, int, int, TIMESTAMP, TIMESTAMP))(*compute_metrics))(hdr,module_metrics_obj,number_customers_int,CustomerCount,outage_length,metric_interval);

	//Make sure it worked
	if (returnval != 1)
	{
		GL_THROW("Metrics:%s failed to perform a post-event metric update",hdr->name);
		/*  TROUBLESHOOT
		While attempting to provide a post-fault update of relevant metrics, the metrics
		object encountered an error.  Please try your code again.  If the error persists,
		please submit your code and a bug report using the trac website.
		*/
	}

	//Increment the counters
	metric_interval_event_count++;
	annual_interval_event_count++;

	//Now that that's done, let's see if we need to write a file output
	if (report_event_log == true)
	{
		//Convert the times
		gl_localtime(event_start_time,&start_dt);
		gl_localtime(event_end_time,&end_dt);

		//Open the file handle
		FPVal = fopen(report_file,"at");

		//Print the details out
		fprintf(FPVal,"%d,%d,%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%s,%s,%s,%s,%d\n",annual_interval_event_count,metric_interval_event_count,start_dt.year,start_dt.month,start_dt.day,start_dt.hour,start_dt.minute,start_dt.second,end_dt.year,end_dt.month,end_dt.day,end_dt.hour,end_dt.minute,end_dt.second,fault_obj->oclass->name,fault_obj->name,hdr->name,fault_type,number_customers_int);

		//Close the file
		fclose(FPVal);
	}
}

//Function to obtain number of customers experiencing outage condition
int metrics::get_interrupted_count(void)
{
	int index, in_outage;

	//Reset counter
	in_outage = 0;

	//Loop through the list and get the number of customer objects reported as interrupted
	for (index=0; index<CustomerCount; index++)
	{
		if (*Customers[index].CustInterrupted == true)
			in_outage++;
	}

	//Pass the value back
	return in_outage;
}

//static void set_values(OBJECT *obj, const char *targets, char *new_values, char *old_values=NULL, int size=0)
//{
//	const char *v=new_values, *p=targets;
//	char propname[64];
//	char value[64];
//	while (p!=NULL && v!=NULL)
//	{
//		bool valid = sscanf(p,"%[A-Za-z._0-9]",propname)>0 && sscanf(v,"%[^;]",value)>0;
//		if (valid) {
//			gl_verbose("%s %s <- %s", obj->name?obj->name:"(null)",propname,value);
//
//			/* get the old value if desired */
//			if (old_values)
//			{
//				if (strcmp(old_values,"")!=0)
//					strcat(old_values,";");
//				if (!gl_get_value_by_name(obj,propname,old_values+strlen(old_values),(int)(size-strlen(old_values))))
//				{
//					gl_error("eventgen refers to non-existant property %s in object %s (%s:%d)", propname,obj->name?obj->name:"anonymous",obj->oclass->name,obj->id);
//					goto GetNext;
//				}
//			}
//
//			/* set the new value */
//			gl_set_value_by_name(obj,propname,value);
//GetNext:
//			/* move to next property/value pair */
//			p = strchr(p,';');
//			v = strchr(v,';');
//			while (p!=NULL && *p==';') p++;
//			while (v!=NULL && *v==';') v++;
//		}
//		else
//			p = v = NULL;
//	}
//}

/** Initiates an event and records it in the event log
	@return a pointer to the object affected by the event
 **/
//OBJECT *metrics::start_event(EVENT *pEvent,			/**< a pointer to the EVENT structure that defines this event */ 
//							 FINDLIST *objectlist,	/**< the list of objects to be affected by the event */
//							 const char *targets,	/**< the list of object properties to be modified */
//							 char *event_values,	/**< the list of values to be used when modifying the properties */
//							 char *old_values,		/**< storage space for the old values before modification */
//							 int size)				/**< the size of the storage space for old values */
//{
//	/* check whether no event is pending (old_value is unpopulated) */
//	if (strcmp(old_values,"")==0)
//	{
//		totals.Nevents++;
//		gl_verbose("event %d starts", totals.Nevents); 
//		gl_verbose("%d objects eligible", objectlist->hit_count);
//
//		/* pick an object at random */
//		unsigned int nth = (unsigned int)gl_random_uniform(0,objectlist->hit_count);
//		OBJECT *obj=gl_find_next(objectlist,NULL);
//		while (nth-->0)
//			obj=gl_find_next(objectlist,obj);
//		gl_verbose("object %s (%s:%d) chosen", obj->name?obj->name:"anonymous", obj->oclass->name,obj->id);
//		set_values(obj,targets,event_values,old_values,size);
//		gl_verbose("old values = '%s'", old_values);
//
//		if (report_event_log)
//		{
//			/* record event in log */
//			DATETIME dt; char buffer[64]="(na)"; 
//			gl_localtime(pEvent->event_time,&dt);
//			if (totals.Nevents==1)
//			{
//				fprintf(fp, "\nEVENT LOG FOR YEAR %d\n", dt.year);
//				fprintf(fp, "=======================\n");
//				fprintf(fp,"Event\tDateTime\t\tAffects\tProperties\t\tValues\tT_interrupted\tL_interrupted\tN_interrupted\n");
//			}
//			fprintf(fp,"%d\t", totals.Nevents);
//			gl_strtime(&dt,buffer,sizeof(buffer));
//			fprintf(fp,"%s\t", buffer);
//			double t = pEvent->ri/60;
//			if (obj->name)
//				fprintf(fp,"%s\t", obj->name);
//			else
//				fprintf(fp,"%s:%d\t", obj->oclass->name, obj->id);
//			fprintf(fp,"%s\t\t", targets);
//			fprintf(fp,"%s\t", event_values);
//			fprintf(fp,"%7.2f\t\t", t);
//		}
//		//find the interrupted load
//		FINDLIST *load_meters = gl_findlist_copy(customers);
//		OBJECT *interrupted_meters=NULL;
//		complex *kva_in;
//		complex *pload;
//		totals.LT=0;//reset the total load value
//		while ((interrupted_meters = gl_find_next(load_meters,interrupted_meters))!=NULL)
//		{
//			kva_in = gl_get_complex_by_name(interrupted_meters,"measured_power");
//			totals.LT += (*kva_in).Mag();
//			pload = gl_get_complex_by_name(interrupted_meters,"pre_load");
//			(*pload) = (*kva_in);
//		}
//		
//
//		/* rest will be written when event ends */
//		return obj;
//	}
//	return NULL;
//}

///** Restores service after an event **/
//void metrics::end_event(OBJECT* obj,			/**< the object which is to be restored */
//						const char *targets,	/**< the properties to be restored */
//						char *old_values)		/**< the old values to be restored */
//{
//
//
//	/* check whether an event is already started (old_value is populated) */
//	if (strcmp(old_values,"")!=0)
//	{
//		gl_verbose("event ends");
//		gl_verbose("old values = '%s'",old_values);
//
//		FINDLIST *candidates = gl_findlist_copy(customers);
//
//		FINDLIST *unserved = gl_find_objects(candidates,FT_PROPERTY,"voltage_A",EQ,"0.0",AND,FT_PROPERTY,"voltage_B",EQ,"0.0",AND,FT_PROPERTY,"voltage_C",EQ,"0.0",NULL);
//		
//		FINDLIST *candidatessecond = gl_findlist_copy(customers);
//
//		FINDLIST *unservedtrip = gl_find_objects(candidatessecond,FT_PROPERTY,"voltage_1",EQ,"0.0",AND,FT_PROPERTY,"voltage_2",EQ,"0.0",NULL);
//		
//		FINDLIST *pmetercounter = gl_findlist_copy(unserved);
//
//		FINDLIST *tmetercounter = gl_findlist_copy(unservedtrip);
//
//		double S_lost=0;
//		
//		OBJECT *tripcust_mad=NULL;
//		while ((tripcust_mad=gl_find_next(unservedtrip,tripcust_mad))!=NULL)
//			gl_findlist_add(unserved,tripcust_mad);
//
//		OBJECT *int_load=NULL;
//		complex *pload;
//		while ((int_load = gl_find_next(unserved,int_load))!=NULL)
//			{
//				pload = gl_get_complex_by_name(int_load,"pre_load");
//				S_lost += (*pload).Mag();
//			}
//		if (eventlist->ri>300){
//			//calculate the total interrupted load over the entire year
//			totals.Li +=S_lost;
//			//calculate the total load interrupted minutes
//			totals.LDI += S_lost * eventlist->ri/60;
//		}
//		
//		if (report_event_log){
//			fprintf(fp,"%9.3f\t", S_lost);
//			fprintf(fp,"%8d\n", unserved->hit_count);
//		}
//		//sustained interruption variable calculations
//		if (eventlist->ri > 300) { 
//			//count the number of meters interrupted by the event
//			totals.CI += unserved->hit_count;
//			//calculate the customer minutes interrupted
//			if (eventlist!=NULL) {
//				totals.CMI += unserved->hit_count * eventlist->ri/60;
//			}
//		
//			//increment sustained event counter for each power meter
//			OBJECT *sust_pmeter=NULL;
//			int16 *sust_pcount;
//			while ((sust_pmeter=gl_find_next(pmetercounter,sust_pmeter))!=NULL){
//				sust_pcount = gl_get_int16_by_name(sust_pmeter,"sustained_count");
//				(*sust_pcount)++;
//			}
//			
//			//increment sustained event counter for each triplex meter
//			OBJECT *sust_tmeter=NULL;
//			int16 *sust_tcount;
//			while ((sust_tmeter=gl_find_next(tmetercounter,sust_tmeter))!=NULL){
//				sust_tcount = gl_get_int16_by_name(sust_tmeter,"sustained_count");
//				(*sust_tcount)++;
//			}
//
//		//momentary interruption variable calculations
//		} else {
//			//determine the number of momentary events
//			totals.CMIE += unserved->hit_count;
//			//increment momentary event counter for each power meter
//			OBJECT *momt_pmeter=NULL;
//			int16 *momt_pcount;
//			while ((momt_pmeter=gl_find_next(pmetercounter,momt_pmeter))!=NULL){
//				momt_pcount = gl_get_int16_by_name(momt_pmeter,"momentary_count");
//				(*momt_pcount)++;
//			}
//			
//			//increment momentary event counter for each triplex meter
//			OBJECT *momt_tmeter=NULL;
//			int16 *momt_tcount;
//			while ((momt_tmeter=gl_find_next(tmetercounter,momt_tmeter))!=NULL){
//				momt_tcount = gl_get_int16_by_name(momt_tmeter,"momentary_count");
//				(*momt_tcount)++;
//			}
//			
//		}
//		
//		//determine the total amount of events experienced by each triplex meter
//		OBJECT *totl_tmeter=NULL;
//		int16 *totl_tcount;
//		while ((totl_tmeter=gl_find_next(tmetercounter,totl_tmeter))!=NULL){
//			totl_tcount = gl_get_int16_by_name(totl_tmeter,"total_count");
//			(*totl_tcount)++;
//		}
//		
//		//determine the total amount of events experienced by each power meter
//		OBJECT *totl_pmeter=NULL;
//		int16 *totl_pcount;
//		while ((totl_pmeter=gl_find_next(pmetercounter,totl_pmeter))!=NULL){
//			totl_pcount = gl_get_int16_by_name(totl_pmeter,"total_count");
//			(*totl_pcount)++;
//		}
//		//determine the number of individual meters that experienced at least 1 sustained interruption
//		candidates = gl_findlist_copy(customers);
//		FINDLIST *sust_intr_cust = gl_find_objects(candidates,FT_PROPERTY,"sustained_count",GT,"0.0",NULL);
//		//calculate the total number of customers that were interrupted by 
//		totals.CN = sust_intr_cust->hit_count;
//
//		//set the CEMI and CEMSMI flags for each type of meter
//		candidates = gl_findlist_copy(customers);
//		OBJECT *k_sust_cust=NULL;
//		int16 *s_count;
//		int16 *sust_flag;
//		while ((k_sust_cust=gl_find_next(candidates,k_sust_cust))!=NULL){
//			s_count = gl_get_int16_by_name(k_sust_cust,"sustained_count");
//			//if ((*s_count)>n){
//			//	sust_flag = gl_get_int16_by_name(k_sust_cust,"s_flag");
//			//	(*sust_flag)=1;
//			//}
//		}
//
//		candidates = gl_findlist_copy(customers);
//		OBJECT *k_totl_cust=NULL;
//		int16 *t_count;
//		int16 *totl_flag;
//		while ((k_totl_cust=gl_find_next(candidates,k_totl_cust))!=NULL){
//			t_count = gl_get_int16_by_name(k_totl_cust,"total_count");
//			//if ((*t_count)>n)
//			//{
//			//	totl_flag = gl_get_int16_by_name(k_totl_cust,"t_flag");
//			//	(*totl_flag)=1;
//			//}
//		}
//
//		//find the number of meter who experienced more than n sustained and total events
//		candidates = gl_findlist_copy(customers);
//		FINDLIST *cemi_cust = gl_find_objects(candidates,FT_PROPERTY,"s_flag",EQ,"1",NULL);
//		candidates = gl_findlist_copy(customers);
//		FINDLIST *cemsmi_cust = gl_find_objects(candidates,FT_PROPERTY,"t_flag",EQ,"1",NULL);
//		totals.CNK=cemi_cust->hit_count;
//		totals.CNT=cemsmi_cust->hit_count;
//
//		//find how many times the recloser tried to close durring the event
//		double relay_ops = 0;
//		if ((gl_object_isa(obj,"relay","powerflow")))
//		{
//			if (eventlist->ri<=300)
//			{
//				int16 *r_operations;
//				r_operations = gl_get_int16_by_name(obj,"recloser_tries");
//				relay_ops = (*r_operations);
//				totals.IMi += relay_ops * unserved->hit_count;
//			}
//		}
//		if (!(gl_object_isa(obj,"relay","powerflow")))
//		{
//			relay_ops = 0;
//			totals.IMi += relay_ops * unserved->hit_count;
//		}
//
//		/* done */
//		gl_free(candidates);
//		gl_free(candidatessecond);
//		fflush(fp);
//
//		/* restore previous values */
//		set_values(obj,targets,old_values);
//		strcpy(old_values,"");
//	}
//}

//Retrieve the address of a metric
double *metrics::get_metric(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
}

//Function to extract address of outage flag
bool *metrics::get_outage_flag(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_bool)
		return NULL;
	return (bool*)GETADDR(obj,p);
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_metrics(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(metrics::oclass);
		if (*obj!=NULL)
		{
			metrics *my = OBJECTDATA(*obj,metrics);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(metrics);
}

EXPORT int init_metrics(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,metrics)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(metrics);
}

EXPORT TIMESTAMP sync_metrics(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	metrics *my = OBJECTDATA(obj,metrics);
	try
	{
		switch (pass) {
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		case PC_PRETOPDOWN:
		case PC_BOTTOMUP:
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
		return t2;
	}
	SYNC_CATCHALL(metrics);
}

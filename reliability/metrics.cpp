/** $Id: metrics.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file metrics.cpp
	@class metrics Metrics analysis
	@ingroup reliability

**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "gridlabd.h"
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
		oclass = gl_register_class(module,"metrics",sizeof(metrics),PC_POSTTOPDOWN|PC_AUTOLOCK);
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
			PT_double, "report_interval[s]", PADDR(report_interval_dbl),
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
	report_interval_dbl = 31536000.0;	//Defaults to a year

	//Internal variables
	num_indices = 0;
	CalcIndices = NULL;
	next_metric_interval = 0;
	next_report_interval = 0;
	next_annual_interval = 0;

	metric_interval = 0;
	report_interval = 0;
	CustomerCount = 0;
	Customers = NULL;
	curr_time = TS_NEVER;	//Flagging value
	metric_interval_event_count = 0;
	annual_interval_event_count = 0;
	metric_equal_annual = false;

	reset_interval_func = NULL;
	reset_annual_func = NULL;
	compute_metrics = NULL;

	secondary_interruptions_count = false;	//By default, we don't look for the secondary interruptions flag

	Extra_Data = NULL;	//Start "extra" variable as null
	
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int metrics::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	int index, indexa, indexb, returnval;
	char work_metrics[1025];
	char *startVal, *endVal, *workVal;
	char workbuffer[1025];
	char metricbuffer[257];
	FILE *FPVal;
	FINDLIST *CandidateObjs;
	OBJECT *temp_obj;
	bool *temp_bool;
	FUNCTIONADDR funadd = NULL;

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

	//It's not null, map up the init function and call it (this must exist, even if it does nothing)
	funadd = (FUNCTIONADDR)(gl_get_function(module_metrics_obj,"init_reliability"));
					
	//Make sure it was found
	if (funadd == NULL)
	{
		GL_THROW("Unable to map reliability init function on %s in %s",module_metrics_obj->name,hdr->name);
		/*  TROUBLESHOOT
		While attempting to initialize the reliability module in the "module of interest" metrics device,
		a error was encountered.  Ensure this object fully supports reliability and try again.  If the bug
		persists, please submit your code and a bug report to the trac website.
		*/
	}

	Extra_Data = ((void *(*)(OBJECT *, OBJECT *))(*funadd))(module_metrics_obj,hdr);

	//Make sure it worked
	if (Extra_Data==NULL)
	{
		GL_THROW("Unable to map reliability init function on %s in %s",module_metrics_obj->name,hdr->name);
		//defined above
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
	metrics_oi.copy_to(work_metrics);

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

			//Copy into secondary
			metricbuffer[indexa] = *workVal;

			//Increment pointers
			indexa++;
			workVal++;
		}

		//apply the "_int" portion for the interval search
		indexb = indexa-1;

		//Now update pointers appropriately and proceed
		endVal++;
		startVal = endVal;
		
		//Now try to find this variable
		CalcIndices[index].MetricLoc = get_metric(module_metrics_obj,CalcIndices[index].MetricName);

		//Make sure it worked
		if (CalcIndices[index].MetricLoc == NULL)
		{
			GL_THROW("Unable to find metric %s in object %s for metric:%s",CalcIndices[index].MetricName.get_string(),module_metrics_obj->name,hdr->name);
			/*  TROUBLESHOOT
			While attempting to map out a reliability metric, the desired metric was not found.  Please check the variable
			name and ensure the metric is being published in the module metrics object and try again.  If the error persists,
			please submit your code and a bug report to the trac website.
			*/
		}

		//Get the interval metric - if it exists - and there is room
		if ((indexb+4) <= 256)
		{
			metricbuffer[indexb] = '_';
			metricbuffer[(indexb+1)] = 'i';
			metricbuffer[(indexb+2)] = 'n';
			metricbuffer[(indexb+3)] = 't';
			metricbuffer[(indexb+4)] = '\0';

			//Try to map it
			CalcIndices[index].MetricLocInterval = get_metric(module_metrics_obj,metricbuffer);

			//No NULL check - if it wasn't found, we won't deal with it
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
	report_interval = (TIMESTAMP)report_interval_dbl;

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
	CandidateObjs = gl_find_objects(FL_GROUP,customer_group.get_string());
	if (CandidateObjs==NULL)
	{
		GL_THROW("Failure to find devices for %s specified as: %s",hdr->name,customer_group.get_string());
		/*  TROUBLESHOOT
		While attempting to populate the list of devices to check for reliability metrics, the metrics
		object failed to find any desired objects.  Please make sure the objects exist and try again.
		If the bug persists, please submit your code using the trac website.
		*/
	}

	//Do a zero-find check as well
	if (CandidateObjs->hit_count == 0)
	{
		GL_THROW("Failure to find devices for %s specified as: %s",hdr->name,customer_group.get_string());
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

		if (index == 0)	//First customer, handle slightly different
		{
			//Populate the secondary index - needs to exist, even if never used
			//Try to find our secondary "outage" indicator and map its address
			temp_bool = get_outage_flag(temp_obj, "customer_interrupted_secondary");

			//make sure it found it
			if (temp_bool == NULL)	//Not found, assume no one else wants one
			{
				gl_warning("Unable to find secondary interruption flag, no secondary interruptions recorded in metrics:%s",hdr->name);
				/*  TROUBLESHOOT
				While attempting to link to the 'secondary customer interrupted' flag, it was not found.  THe object may not support
				"secondary interruption counts" and this message is valid.  If a secondary count was desired, ensure the object
				supports being polled by reliability as a customer (customer_interrupted_secondary exists as a published property) and
				try again.  If the error persists, please submit your code and a bug report via the trac website.
				*/
			}
			else	//One found, assume all want one now
			{
				secondary_interruptions_count = true;
				
				//Write this value in
				Customers[index].CustInterrupted_Secondary = temp_bool;
			}
		}
		else if (secondary_interruptions_count == true)	//Decided we want it
		{
			//Populate the secondary index - needs to exist, even if never used
			//Try to find our secondary "outage" indicator and map its address
			temp_bool = get_outage_flag(temp_obj, "customer_interrupted_secondary");

			//make sure it found it
			if (temp_bool == NULL)
			{
				GL_THROW("Unable to find secondary interruption flag for customer object %s in metrics:%s",temp_obj->name,hdr->name);
				/*  TROUBLESHOOT
				While attempting to link to the 'secondary customer interrupted' flag, an error occurred.  Please ensure the object
				supports being polled by reliability as a customer (customer_interrupted_secondary exists as a published property) and
				try again.  If the error persists, please submit your code and a bug report via the trac website.
				*/
			}

			//Write this value in
			Customers[index].CustInterrupted_Secondary = temp_bool;
		}
		//Defaulted else - unwanted

	}//end population loop

	//Free up list
	gl_free(CandidateObjs);

	//Write the customer count and header information to the file we have going
	fprintf(FPVal,"Number of customers = %d\n\n",CustomerCount);

	//See if the particular metrics object has any "comments" to add to the file header (units, notes, etc.)
	funadd = NULL;	//Reset function pointer - just in case

	//Map up the "extra print" function - if it isn't there, well nothing is done
	funadd = (FUNCTIONADDR)(gl_get_function(module_metrics_obj,"logfile_extra"));

	//See if it was found
	if (funadd != NULL)
	{
		//Do the extra printing
		returnval = ((int (*)(OBJECT *, char *))(*funadd))(module_metrics_obj,workbuffer);

		//Make sure it worked
		if (returnval==0)
		{
			GL_THROW("Failed to write extra header material for %s in %s",module_metrics_obj->name,hdr->name);
			/*  TROUBLESHOOT
			While attempting to write the extra material into the file header, an error occurred.  Please try again.
			If the error persists, please submit your code and a bug report via the trac website.
			*/
		}

		//Print it out
		fprintf(FPVal,"%s",workbuffer);
	}

	//Close the file handle
	fclose(FPVal);

	return 1; /* return 1 on success, 0 on failure - We're so awesome we always assume we work */
}

TIMESTAMP metrics::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	DATETIME dt;
	int returnval;
	bool metrics_written;
	OBJECT *hdr = OBJECTHDR(this);
	FILE *FPVal;

	//Initialization
	if (curr_time == TS_NEVER)
	{
		//Update time value trackers
		if (metric_interval == 0)	//No metric update interval - ensure never goes off
		{
			next_metric_interval = TS_NEVER;
		}
		else	//OK, proceed
		{
			next_metric_interval = t1 + metric_interval;
		}

		next_report_interval = t1 + report_interval;
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

			//Determine which header to write
			if (secondary_interruptions_count == true)
			{
				fprintf(FPVal,"Annual Event #,Metric Interval Event #,Starting DateTime (YYYY-MM-DD hh:mm:ss),Ending DateTime (YYYY-MM-DD hh:mm:ss),Object type,Object Name,Inducing Object,\"Protective\" Device,Desired Fault type,Implemented Fault Type,Number customers affected,Secondary number of customers affected\n");
			}
			else //Nope
			{
				fprintf(FPVal,"Annual Event #,Metric Interval Event #,Starting DateTime (YYYY-MM-DD hh:mm:ss),Ending DateTime (YYYY-MM-DD hh:mm:ss),Object type,Object Name,Inducing Object,\"Protective\" Device,Desired Fault type,Implemented Fault Type,Number customers affected\n");
			}

			//Close the file handle
			fclose(FPVal);
		}

		//First run - t1 = t0 (t0 is zero)
		curr_time = t1;
	}

	//Only worry about checks once per timestep
	if (curr_time != t0)
	{
		//Reset temp flag
		metrics_written = false;

		if (t0 >= next_report_interval)
		{
			//Write the output metric values
			write_metrics();

			//Update variable
			metrics_written = true;

			//Update the interval
			next_report_interval = t0 + report_interval;
		}

		//See if it is time to write an update
		if (t0 >= next_metric_interval)
		{
			//See if the metrics were written - if not, dumb them again for posterity
			if (metrics_written == false)
				write_metrics();

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
			//See if we need to write the file - if "metric_interval" just went off, no sense writing the same thing again
			if (metrics_written == false)
				write_metrics();

			//Update interval
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
	{
		if (next_metric_interval < next_report_interval)
			return -next_metric_interval;
		else
			return -next_report_interval;
	}
	else
	{
		if (next_annual_interval < next_report_interval)
			return -next_annual_interval;
		else
			return -next_report_interval;
	}
}

//Perform post-event analysis (update computations, write event file if necessary) - no secondary count
void metrics::event_ended(OBJECT *event_obj, OBJECT *fault_obj, OBJECT *faulting_obj, TIMESTAMP event_start_time, TIMESTAMP event_end_time, char *fault_type, char *impl_fault, int number_customers_int)
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

		//Print the name of the "safety" device?
		if (faulting_obj==NULL)
		{
		//Print the details out
			fprintf(FPVal,"%d,%d,%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%s,%s,%s,N/A,%s,%s,%d\n",annual_interval_event_count,metric_interval_event_count,start_dt.year,start_dt.month,start_dt.day,start_dt.hour,start_dt.minute,start_dt.second,end_dt.year,end_dt.month,end_dt.day,end_dt.hour,end_dt.minute,end_dt.second,fault_obj->oclass->name,fault_obj->name,event_obj->name,fault_type,impl_fault,number_customers_int);
		}
		else
		{
			//Print the details out
			fprintf(FPVal,"%d,%d,%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%s,%s,%s,%s,%s,%s,%d\n",annual_interval_event_count,metric_interval_event_count,start_dt.year,start_dt.month,start_dt.day,start_dt.hour,start_dt.minute,start_dt.second,end_dt.year,end_dt.month,end_dt.day,end_dt.hour,end_dt.minute,end_dt.second,fault_obj->oclass->name,fault_obj->name,event_obj->name,faulting_obj->name,fault_type,impl_fault,number_customers_int);
		}

		//Close the file
		fclose(FPVal);
	}
}

//Perform post-event analysis (update computations, write event file if necessary) - assumes secondary count wanted
void metrics::event_ended_sec(OBJECT *event_obj, OBJECT *fault_obj, OBJECT *faulting_obj, TIMESTAMP event_start_time, TIMESTAMP event_end_time, char *fault_type, char *impl_fault, int number_customers_int, int number_customers_int_secondary)
{
	DATETIME start_dt, end_dt;
	TIMESTAMP outage_length;
	OBJECT *hdr = OBJECTHDR(this);
	int returnval;
	FILE *FPVal;

	//Determine the actual outage length (may be off due to iterations and such)
	outage_length = event_end_time - event_start_time;

	//Perform the calculation
	returnval = ((int (*)(OBJECT *, OBJECT *, int, int, int, TIMESTAMP, TIMESTAMP))(*compute_metrics))(hdr,module_metrics_obj,number_customers_int,number_customers_int_secondary,CustomerCount,outage_length,metric_interval);

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

		//Print the details out - Print the name of the "safety" device?
		if (faulting_obj==NULL)
		{
			fprintf(FPVal,"%d,%d,%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%s,%s,%s,N/A,%s,%s,%d,%d\n",annual_interval_event_count,metric_interval_event_count,start_dt.year,start_dt.month,start_dt.day,start_dt.hour,start_dt.minute,start_dt.second,end_dt.year,end_dt.month,end_dt.day,end_dt.hour,end_dt.minute,end_dt.second,fault_obj->oclass->name,fault_obj->name,event_obj->name,fault_type,impl_fault,number_customers_int,number_customers_int_secondary);
		}
		else
		{
			fprintf(FPVal,"%d,%d,%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%s,%s,%s,%s,%s,%s,%d,%d\n",annual_interval_event_count,metric_interval_event_count,start_dt.year,start_dt.month,start_dt.day,start_dt.hour,start_dt.minute,start_dt.second,end_dt.year,end_dt.month,end_dt.day,end_dt.hour,end_dt.minute,end_dt.second,fault_obj->oclass->name,fault_obj->name,event_obj->name,faulting_obj->name,fault_type,impl_fault,number_customers_int,number_customers_int_secondary);
		}

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

//Function to obtain number of customers experiencing outage condition - with secondary count
void metrics::get_interrupted_count_secondary(int *in_outage, int *in_outage_secondary)
{
	int index, in_outage_temp, in_outage_temp_sec;

	//Reset counter
	in_outage_temp = 0;
	in_outage_temp_sec = 0;

	//Loop through the list and get the number of customer objects reported as interrupted
	for (index=0; index<CustomerCount; index++)
	{
		if (*Customers[index].CustInterrupted == true)
			in_outage_temp++;

		//Assumes secondary metric exists, otherwise we shouldn't be here
		if (*Customers[index].CustInterrupted_Secondary == true)
			in_outage_temp_sec++;
	}

	//Pass the value back
	*in_outage = in_outage_temp;
	*in_outage_secondary = in_outage_temp_sec;
}


//Function to write output values of metrics
void metrics::write_metrics(void)
{
	FILE *FPVAL;
	int index;
	bool first_written;

	//Open the file
	FPVAL = fopen(report_file,"at");

	//Put a line in it
	fprintf(FPVAL,"\n");

	//Loop through the metrics and output them
	for (index=0; index<num_indices; index++)
	{
		//First one slightly different
		if (index==0)
		{
			//Print metric name and value
			fprintf(FPVAL,"%s = %f",CalcIndices[index].MetricName.get_string(),*CalcIndices[index].MetricLoc);
		}
		else
		{
			//Print metric name and value
			fprintf(FPVAL,", %s = %f",CalcIndices[index].MetricName.get_string(),*CalcIndices[index].MetricLoc);
		}
	}

	fprintf(FPVAL,"\n");

	//Write interval metrics - if desired
	if (metric_interval != 0)
	{
		first_written = false;

		for (index=0; index<num_indices; index++)
		{
			if (CalcIndices[index].MetricLocInterval != NULL)
			{
				if (first_written == false)
				{
					//Print metric name and value
					fprintf(FPVAL,"Interval values: %s = %f",CalcIndices[index].MetricName.get_string(),*CalcIndices[index].MetricLocInterval);
					first_written = true;
				}
				else
				{
					//Print metric name and value
					fprintf(FPVAL,", %s = %f",CalcIndices[index].MetricName.get_string(),*CalcIndices[index].MetricLocInterval);
				}
			}
		}
		fprintf(FPVAL,"\n");
	}//End metric intervals desired

	//Close the file
	fclose(FPVAL);
}

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

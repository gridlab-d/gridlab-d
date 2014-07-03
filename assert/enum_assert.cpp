/* enum_assert

   Very simple test that compares either integers or can be used to compare enumerated values
   to their corresponding integer values.  If the test fails at any time, it throws a 'zero' to
   the commit function and breaks the simulator out with a failure code.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#include "enum_assert.h"

EXPORT_CREATE(enum_assert);
EXPORT_INIT(enum_assert);
EXPORT_COMMIT(enum_assert);

CLASS *enum_assert::oclass = NULL;
enum_assert *enum_assert::defaults = NULL;

enum_assert::enum_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"enum_assert",sizeof(enum_assert),PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class enum_assert";
		else
			oclass->trl = TRL_PROVEN;

		if (gl_publish_variable(oclass,
			// TO DO:  publish your variables here
			PT_enumeration,"status",get_status_offset(),PT_DESCRIPTION,"Conditions for the assert checks",
				PT_KEYWORD,"ASSERT_TRUE",(enumeration)ASSERT_TRUE,
				PT_KEYWORD,"ASSERT_FALSE",(enumeration)ASSERT_FALSE,
				PT_KEYWORD,"ASSERT_NONE",(enumeration)ASSERT_NONE,
			PT_int32, "value", get_value_offset(),PT_DESCRIPTION,"Value to assert",
			PT_char1024, "target", get_target_offset(),PT_DESCRIPTION,"Property to perform the assert upon",	
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		defaults = this;
		status = ASSERT_TRUE;
		value = 0;
	}
}

/* Object creation is called once for each object that is created by the core */
int enum_assert::create(void) 
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

int enum_assert::init(OBJECT *parent)
{
	return 1;
}

TIMESTAMP enum_assert::commit(TIMESTAMP t1, TIMESTAMP t2)
{

	gld_property target_prop(get_parent(),get_target());
	if ( !target_prop.is_valid() || target_prop.get_type()!=PT_enumeration ) 
	{
		gl_error("Specified target %s for %s is not valid.",get_target(),get_parent()->get_name());
		/*  TROUBLESHOOT
		Check to make sure the target you are specifying is a published variable for the object
		that you are pointing to.  Refer to the documentation of the command flag --modhelp, or 
		check the wiki page to determine which variables can be published within the object you
		are pointing to with the assert function.
		*/
		return 0;
	}

	int32 x; target_prop.getp(x);
	if (status == ASSERT_TRUE)
	{
		if (value != x) 
		{
			gl_error("Assert failed on %s: %s=%g did not match %g", 
				get_parent()->get_name(), get_target(), x, value);
			return 0;
		}
		else
		{
			gl_verbose("Assert passed on %s", get_parent()->get_name());
			return TS_NEVER;
		}
	}
	else if (status == ASSERT_FALSE)
	{
		if (value == x)
		{
			gl_error("Assert failed on %s: %s=%g did match %g", 
				get_parent()->get_name(), get_target(), x, value);
			return 0;
		}
		else
		{
			gl_verbose("Assert passed on %s", get_parent()->get_name());
			return TS_NEVER;
		}
	}
	else
	{
		gl_verbose("Assert test is not being run on %s", get_parent()->get_name());
		return TS_NEVER;
	}
}

//Deltamode compatible enumeration assert
EXPORT SIMULATIONMODE update_enum_assert(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	char buff[64];
	char dateformat[8]="";
	char error_output_buff[1024];
	char datebuff[64];
	enum_assert *da = OBJECTDATA(obj,enum_assert);
	DATETIME delta_dt_val;
	double del_clock;
	TIMESTAMP del_clock_int;
	int del_microseconds;
	int32 *x;

	//Iteration checker - assert only valid on the first timestep
	if (iteration_count_val == 0)
	{
		//Skip first timestep of any delta iteration -- nature of delta means it really isn't checking the right one
		if (delta_time>=dt)
		{
			//Get the value
			x = (int32*)gl_get_enum_by_name(obj->parent,da->get_target());

			if (x==NULL) 
			{
				gl_error("Specified target %s for %s is not valid.",da->get_target(),gl_name(obj->parent,buff,64));
				/*  TROUBLESHOOT
				Check to make sure the target you are specifying is a published variable for the object
				that you are pointing to.  Refer to the documentation of the command flag --modhelp, or 
				check the wiki page to determine which variables can be published within the object you
				are pointing to with the assert function.
				*/
				return SM_ERROR;
			}
			else if (da->get_status() == da->ASSERT_TRUE)
			{
				if (*x != da->get_value())
				{
					//Calculate time
					if (delta_time>=dt)	//After first iteration
						del_clock  = (double)t0 + (double)(delta_time-dt)/(double)DT_SECOND;
					else	//First second different, don't back out
						del_clock  = (double)t0 + (double)(delta_time)/(double)DT_SECOND;

					del_clock_int = (TIMESTAMP)del_clock;	/* Whole seconds - update from global clock because we could be in delta for over 1 second */
					del_microseconds = (int)((del_clock-(int)(del_clock))*1000000+0.5);	/* microseconds roll-over - biased upward (by 0.5) */
					
					//Convert out
					gl_localtime(del_clock_int,&delta_dt_val);

					//Determine output format
					gl_global_getvar("dateformat",dateformat,sizeof(dateformat));

					//Output date appropriately
					if ( strcmp(dateformat,"ISO")==0)
						sprintf(datebuff,"ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",delta_dt_val.year,delta_dt_val.month,delta_dt_val.day,delta_dt_val.hour,delta_dt_val.minute,delta_dt_val.second,del_microseconds,delta_dt_val.tz);
					else if ( strcmp(dateformat,"US")==0)
						sprintf(datebuff,"ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",delta_dt_val.month,delta_dt_val.day,delta_dt_val.year,delta_dt_val.hour,delta_dt_val.minute,delta_dt_val.second,del_microseconds,delta_dt_val.tz);
					else if ( strcmp(dateformat,"EURO")==0)
						sprintf(datebuff,"ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",delta_dt_val.day,delta_dt_val.month,delta_dt_val.year,delta_dt_val.hour,delta_dt_val.minute,delta_dt_val.second,del_microseconds,delta_dt_val.tz);
					else
						sprintf(datebuff,"ERROR    .09f : ",del_clock);

					//Actual error part
					sprintf(error_output_buff,"Assert failed on %s - %s (%d) did not match %d",gl_name(obj->parent, buff, 64),da->get_target(), *x, da->get_value());

					//Send it out
					gl_output("%s%s",datebuff,error_output_buff);

					return SM_ERROR;
				}
				else
				{
					gl_verbose("Assert passed on %s", gl_name(obj->parent, buff, 64));
					return SM_EVENT;
				}
			}
			else if (da->get_status() == da->ASSERT_FALSE)
			{
				if (*x == da->get_value())
				{
					//Calculate time
					if (delta_time>=dt)	//After first iteration
						del_clock  = (double)t0 + (double)(delta_time-dt)/(double)DT_SECOND;
					else	//First second different, don't back out
						del_clock  = (double)t0 + (double)(delta_time)/(double)DT_SECOND;

					del_clock_int = (TIMESTAMP)del_clock;	/* Whole seconds - update from global clock because we could be in delta for over 1 second */
					del_microseconds = (int)((del_clock-(int)(del_clock))*1000000+0.5);	/* microseconds roll-over - biased upward (by 0.5) */

					//Convert out
					gl_localtime(del_clock_int,&delta_dt_val);

					//Determine output format
					gl_global_getvar("dateformat",dateformat,sizeof(dateformat));

					//Output date appropriately
					if ( strcmp(dateformat,"ISO")==0)
						sprintf(datebuff,"ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",delta_dt_val.year,delta_dt_val.month,delta_dt_val.day,delta_dt_val.hour,delta_dt_val.minute,delta_dt_val.second,del_microseconds,delta_dt_val.tz);
					else if ( strcmp(dateformat,"US")==0)
						sprintf(datebuff,"ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",delta_dt_val.month,delta_dt_val.day,delta_dt_val.year,delta_dt_val.hour,delta_dt_val.minute,delta_dt_val.second,del_microseconds,delta_dt_val.tz);
					else if ( strcmp(dateformat,"EURO")==0)
						sprintf(datebuff,"ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",delta_dt_val.day,delta_dt_val.month,delta_dt_val.year,delta_dt_val.hour,delta_dt_val.minute,delta_dt_val.second,del_microseconds,delta_dt_val.tz);
					else
						sprintf(datebuff,"ERROR    .09f : ",del_clock);

					//Actual error part
					sprintf(error_output_buff,"Assert failed on %s - %s (%d) did not match %d",gl_name(obj->parent, buff, 64),da->get_target(), *x, da->get_value());

					//Send it out
					gl_output("%s%s",datebuff,error_output_buff);

					return SM_ERROR;
				}
				else
				{
					gl_verbose("Assert passed on %s", gl_name(obj->parent, buff, 64));
					return SM_EVENT;
				}
			}
			else
			{
				gl_verbose("Assert test is not being run on %s", gl_name(obj->parent, buff, 64));
				return SM_EVENT;
			}
		}
		else	//first timestep
			return SM_EVENT;
	}
	else	//Iteration, so don't care
		return SM_EVENT;
}

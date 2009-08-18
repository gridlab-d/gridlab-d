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

double metrics::major_event_threshold = 0.01; /* minutes */
bool metrics::report_event_log = false; /* dumps detailed event log */

static PASSCONFIG passconfig = PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_POSTTOPDOWN;

/* Class registration is only called once to register the class with the core */
metrics::metrics(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"metrics",sizeof(metrics),passconfig);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			PT_char1024, "report_file", PADDR(report_file),
			PT_double, "SAIFI", PADDR(SAIFI),
			PT_double, "SAIDI", PADDR(SAIDI),
			PT_double, "CAIDI", PADDR(CAIDI),
			PT_double, "CAIFI", PADDR(CAIFI),
			PT_double, "CTAIDI", PADDR(CTAIDI),
			PT_double, "ASAI", PADDR(ASAI),
			PT_double, "ASIFI", PADDR(ASIFI),
			PT_double, "ASIDI", PADDR(ASIDI),
			PT_double, "CEMI", PADDR(CEMI),
			PT_double, "MAIFI", PADDR(MAIFI),
			PT_double, "MAIFIE", PADDR(MAIFIE),
			PT_double, "CEMSMI", PADDR(CEMSMI),
			PT_int16, "n", PADDR(n),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(metrics));
		customers = interrupted = NULL;
		SAIFI = SAIDI = CAIDI = CAIFI = CTAIDI = ASAI = ASIFI = ASIDI = CEMI = MAIFI = MAIFIE = CEMSMI = n =0.0;
		strcpy(report_file,"reliability.txt");
	}
}

/* Object creation is called once for each object that is created by the core */
int metrics::create(void)
{
	memcpy(this,defaults,sizeof(metrics));
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int metrics::init(OBJECT *parent)
{
	fp = fopen(report_file,"w");
	if (fp==NULL)
	{
		gl_error("unable to create file '%s' for reliability metrics report",report_file);
		return 0;
	}
	char buffer[1024];
	fprintf(fp,"Reliability report for %s\n", gl_global_getvar("modelname",buffer,sizeof(buffer)));
	
	customers=gl_find_objects(FL_GROUP,("class=triplex_meter"));	
	
	FINDLIST *normmeter=gl_find_objects(FL_GROUP,("class=meter"));	
	
	OBJECT *metercust=NULL;
	while ((metercust=gl_find_next(normmeter,metercust))!=NULL)
		gl_findlist_add(customers,metercust);

	//customers=gl_find_objects(FL_GROUP,"class=meter");	

	interrupted = gl_find_objects(FL_NEW,NULL);
	fprintf(fp,"Number of customers: %d\n", customers->hit_count);
	first_year = true;
	return 1; /* return 1 on success, 0 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP metrics::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	DATETIME dt;
	int valid = gl_localtime(t1, &dt);

	/* perform annual metric calculation */
	if (totals.period!=dt.year)
	{
		if (totals.Nevents>0)
		{
			if (report_event_log)
			{
				fprintf(fp, "\nAnnual reliability metrics report for %d", dt.year-1);
				fprintf(fp, "\n=================================================================================================================\n");
				fprintf(fp, "SAIDI\tSAIFI\tCAIDI\tCAIFI\tCTAIDI\tCEMI\tCEMSMI\tASIFI\tASIDI\tMAIFI\tMAIFIE\tASAI\n");
			}
			else if (first_year)
			{	
				fprintf(fp, "\nYear\tSAIDI\tSAIFI\tCAIDI\tCAIFI\tCTAIDI\tCEMI\tCEMSMI\tASIFI\tASIDI\tMAIFI\tMAIFIE\tASAI\n");
				first_year=false;
			}

			// write reliability report
			if (!report_event_log)
				fprintf(fp,"%4d\t", dt.year-1);
			fprintf(fp, "%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.6f\n", saidi(), saifi(), caidi(), caifi(), ctaidi(), cemi(), cemsmi(),asifi(), asidi(), maifi(), maifie(),asai()*100);
		}
		
		// reset totals for new year
		memset(&totals,0,sizeof(totals));
		totals.period = dt.year;
	}
	dt.month=1; dt.day=1; dt.year++; dt.hour=0; dt.minute=0; dt.second=0; dt.microsecond=0;
	return valid?-gl_mktime(&dt):TS_NEVER; /* soft date */
}

static void set_values(OBJECT *obj, const char *targets, char *new_values, char *old_values=NULL, int size=0)
{
	const char *v=new_values, *p=targets;
	char propname[64];
	char value[64];
	while (p!=NULL && v!=NULL)
	{
		bool valid = sscanf(p,"%[A-Za-z._0-9]",propname)>0 && sscanf(v,"%[^;]",value)>0;
		if (valid) {
			gl_verbose("%s %s <- %s", obj->name?obj->name:"(null)",propname,value);

			/* get the old value if desired */
			if (old_values)
			{
				if (strcmp(old_values,"")!=0)
					strcat(old_values,";");
				if (!gl_get_value_by_name(obj,propname,old_values+strlen(old_values),(int)(size-strlen(old_values))))
				{
					gl_error("eventgen refers to non-existant property %s in object %s (%s:%d)", propname,obj->name?obj->name:"anonymous",obj->oclass->name,obj->id);
					goto GetNext;
				}
			}

			/* set the new value */
			gl_set_value_by_name(obj,propname,value);
GetNext:
			/* move to next property/value pair */
			p = strchr(p,';');
			v = strchr(v,';');
			while (p!=NULL && *p==';') p++;
			while (v!=NULL && *v==';') v++;
		}
		else
			p = v = NULL;
	}
}

/** Initiates an event and records it in the event log
	@return a pointer to the object affected by the event
 **/
OBJECT *metrics::start_event(EVENT *pEvent,			/**< a pointer to the EVENT structure that defines this event */ 
							 FINDLIST *objectlist,	/**< the list of objects to be affected by the event */
							 const char *targets,	/**< the list of object properties to be modified */
							 char *event_values,	/**< the list of values to be used when modifying the properties */
							 char *old_values,		/**< storage space for the old values before modification */
							 int size)				/**< the size of the storage space for old values */
{
	/* check whether no event is pending (old_value is unpopulated) */
	if (strcmp(old_values,"")==0)
	{
		totals.Nevents++;
		gl_verbose("event %d starts", totals.Nevents); 
		gl_verbose("%d objects eligible", objectlist->hit_count);

		/* pick an object at random */
		unsigned int nth = (unsigned int)gl_random_uniform(0,objectlist->hit_count);
		OBJECT *obj=gl_find_next(objectlist,NULL);
		while (nth-->0)
			obj=gl_find_next(objectlist,obj);
		gl_verbose("object %s (%s:%d) chosen", obj->name?obj->name:"anonymous", obj->oclass->name,obj->id);
		set_values(obj,targets,event_values,old_values,size);
		gl_verbose("old values = '%s'", old_values);

		if (report_event_log)
		{
			/* record event in log */
			DATETIME dt; char buffer[64]="(na)"; 
			gl_localtime(pEvent->event_time,&dt);
			if (totals.Nevents==1)
			{
				fprintf(fp, "\nEVENT LOG FOR YEAR %d\n", dt.year);
				fprintf(fp, "=======================\n");
				fprintf(fp,"Event\tDateTime\t\tAffects\tProperties\t\tValues\tT_interrupted\tL_interrupted\tN_interrupted\n");
			}
			fprintf(fp,"%d\t", totals.Nevents);
			gl_strtime(&dt,buffer,sizeof(buffer));
			fprintf(fp,"%s\t", buffer);
			double t = pEvent->ri/60;
			if (obj->name)
				fprintf(fp,"%s\t", obj->name);
			else
				fprintf(fp,"%s:%d\t", obj->oclass->name, obj->id);
			fprintf(fp,"%s\t\t", targets);
			fprintf(fp,"%s\t", event_values);
			fprintf(fp,"%7.2f\t\t", t);
			//find the interrupted load
			FINDLIST *load_meters = gl_findlist_copy(customers);
			OBJECT *interrupted_meters=NULL;
			complex *kva_in;
			complex *pload;
			totals.LT=0;//reset the total load value
			while ((interrupted_meters = gl_find_next(load_meters,interrupted_meters))!=NULL)
			{
				kva_in = gl_get_complex_by_name(interrupted_meters,"measured_power");
				totals.LT += (*kva_in).Mag();
				pload = gl_get_complex_by_name(interrupted_meters,"pre_load");
				(*pload) = (*kva_in);
			}
		}

		/* rest will be written when event ends */
		return obj;
	}
	return NULL;
}

/** Restores service after an event **/
void metrics::end_event(OBJECT* obj,			/**< the object which is to be restored */
						const char *targets,	/**< the properties to be restored */
						char *old_values)		/**< the old values to be restored */
{


	/* check whether an event is already started (old_value is populated) */
	if (strcmp(old_values,"")!=0)
	{
		gl_verbose("event ends");
		gl_verbose("old values = '%s'",old_values);

		FINDLIST *candidates = gl_findlist_copy(customers);

		FINDLIST *unserved = gl_find_objects(candidates,FT_PROPERTY,"voltage_A",EQ,"0.0",AND,FT_PROPERTY,"voltage_B",EQ,"0.0",AND,FT_PROPERTY,"voltage_C",EQ,"0.0",NULL);
		
		FINDLIST *candidatessecond = gl_findlist_copy(customers);

		FINDLIST *unservedtrip = gl_find_objects(candidatessecond,FT_PROPERTY,"voltage_1",EQ,"0.0",AND,FT_PROPERTY,"voltage_2",EQ,"0.0",NULL);
		
		FINDLIST *pmetercounter = gl_findlist_copy(unserved);

		FINDLIST *tmetercounter = gl_findlist_copy(unservedtrip);

		double S_lost=0;
		
		OBJECT *tripcust_mad=NULL;
		while ((tripcust_mad=gl_find_next(unservedtrip,tripcust_mad))!=NULL)
			gl_findlist_add(unserved,tripcust_mad);

		OBJECT *int_load=NULL;
		complex *pload;
		while ((int_load = gl_find_next(unserved,int_load))!=NULL)
			{
				pload = gl_get_complex_by_name(int_load,"pre_load");
				S_lost += (*pload).Mag();
			}
		if (eventlist->ri>300){
			//calculate the total interrupted load over the entire year
			totals.Li +=S_lost;
			//calculate the total load interrupted minutes
			totals.LDI += S_lost * eventlist->ri/60;
		}
		
		if (report_event_log){
			fprintf(fp,"%9.3f\t", S_lost);
			fprintf(fp,"%8d\n", unserved->hit_count);
		}
		//sustained interruption variable calculations
		if (eventlist->ri > 300) { 
			//count the number of meters interrupted by the event
			totals.CI += unserved->hit_count;
			//calculate the customer minutes interrupted
			if (eventlist!=NULL) {
				totals.CMI += unserved->hit_count * eventlist->ri/60;
			}
		
			//increment sustained event counter for each power meter
			OBJECT *sust_pmeter=NULL;
			int16 *sust_pcount;
			while ((sust_pmeter=gl_find_next(pmetercounter,sust_pmeter))!=NULL){
				sust_pcount = gl_get_int16_by_name(sust_pmeter,"sustained_count");
				(*sust_pcount)++;
			}
			
			//increment sustained event counter for each triplex meter
			OBJECT *sust_tmeter=NULL;
			int16 *sust_tcount;
			while ((sust_tmeter=gl_find_next(tmetercounter,sust_tmeter))!=NULL){
				sust_tcount = gl_get_int16_by_name(sust_tmeter,"sustained_count");
				(*sust_tcount)++;
			}

		//momentary interruption variable calculations
		} else {
			//determine the number of momentary events
			totals.CMIE += unserved->hit_count;
			//increment momentary event counter for each power meter
			OBJECT *momt_pmeter=NULL;
			int16 *momt_pcount;
			while ((momt_pmeter=gl_find_next(pmetercounter,momt_pmeter))!=NULL){
				momt_pcount = gl_get_int16_by_name(momt_pmeter,"momentary_count");
				(*momt_pcount)++;
			}
			
			//increment momentary event counter for each triplex meter
			OBJECT *momt_tmeter=NULL;
			int16 *momt_tcount;
			while ((momt_tmeter=gl_find_next(tmetercounter,momt_tmeter))!=NULL){
				momt_tcount = gl_get_int16_by_name(momt_tmeter,"momentary_count");
				(*momt_tcount)++;
			}
			
		}
		
		//determine the total amount of events experienced by each triplex meter
		OBJECT *totl_tmeter=NULL;
		int16 *totl_tcount;
		while ((totl_tmeter=gl_find_next(tmetercounter,totl_tmeter))!=NULL){
			totl_tcount = gl_get_int16_by_name(totl_tmeter,"total_count");
			(*totl_tcount)++;
		}
		
		//determine the total amount of events experienced by each power meter
		OBJECT *totl_pmeter=NULL;
		int16 *totl_pcount;
		while ((totl_pmeter=gl_find_next(pmetercounter,totl_pmeter))!=NULL){
			totl_pcount = gl_get_int16_by_name(totl_pmeter,"total_count");
			(*totl_pcount)++;
		}
		//determine the number of individual meters that experienced at least 1 sustained interruption
		candidates = gl_findlist_copy(customers);
		FINDLIST *sust_intr_cust = gl_find_objects(candidates,FT_PROPERTY,"sustained_count",GT,"0.0",NULL);
		//calculate the total number of customers that were interrupted by 
		totals.CN = sust_intr_cust->hit_count;

		//set the CEMI and CEMSMI flags for each type of meter
		candidates = gl_findlist_copy(customers);
		OBJECT *k_sust_cust=NULL;
		int16 *s_count;
		int16 *sust_flag;
		while ((k_sust_cust=gl_find_next(candidates,k_sust_cust))!=NULL){
			s_count = gl_get_int16_by_name(k_sust_cust,"sustained_count");
			if ((*s_count)>n){
				sust_flag = gl_get_int16_by_name(k_sust_cust,"s_flag");
				(*sust_flag)=1;
			}
		}

		candidates = gl_findlist_copy(customers);
		OBJECT *k_totl_cust=NULL;
		int16 *t_count;
		int16 *totl_flag;
		while ((k_totl_cust=gl_find_next(candidates,k_totl_cust))!=NULL){
			t_count = gl_get_int16_by_name(k_totl_cust,"total_count");
			if ((*t_count)>n)
			{
				totl_flag = gl_get_int16_by_name(k_totl_cust,"t_flag");
				(*totl_flag)=1;
			}
		}

		//find the number of meter who experienced more than n sustained and total events
		candidates = gl_findlist_copy(customers);
		FINDLIST *cemi_cust = gl_find_objects(candidates,FT_PROPERTY,"s_flag",EQ,"1",NULL);
		candidates = gl_findlist_copy(customers);
		FINDLIST *cemsmi_cust = gl_find_objects(candidates,FT_PROPERTY,"t_flag",EQ,"1",NULL);
		totals.CNK=cemi_cust->hit_count;
		totals.CNT=cemsmi_cust->hit_count;

		//find how many times the recloser tried to close durring the event
		double relay_ops = 0;
		if ((gl_object_isa(obj,"relay","powerflow")))
		{
			if (eventlist->ri<=300)
			{
				int16 *r_operations;
				r_operations = gl_get_int16_by_name(obj,"recloser_tries");
				relay_ops = (*r_operations);
				totals.IMi += relay_ops * unserved->hit_count;
			}
		}
		if (!(gl_object_isa(obj,"relay","powerflow")))
		{
			relay_ops = 0;
			totals.IMi += relay_ops * unserved->hit_count;
		}

		SAIFI = saifi();
		SAIDI = saidi();
		CAIFI = caifi();
		CAIDI = caidi();
		CTAIDI = ctaidi();
		ASAI = asai();
		MAIFI = maifi();
		MAIFIE = maifie();
		CEMI = cemi();
		CEMSMI = cemsmi();
		ASIDI = asidi();
		ASIFI = asifi();

		/* done */
		gl_free(candidates);
		gl_free(candidatessecond);
		fflush(fp);

		/* restore previous values */
		set_values(obj,targets,old_values);
		strcpy(old_values,"");
	}
}

//calculate the reliability indices
double metrics::asai(void)
{
	return customers->hit_count>0 ? (customers->hit_count*8760 - (totals.CMI/60))/(customers->hit_count*8760) : 1.0;
}
double metrics::ctaidi(void)
{
	return totals.CN>0 ? totals.CMI/totals.CN : 0;
}
double metrics::saidi(void)
{
	return customers->hit_count>0 ? totals.CMI / customers->hit_count : 0;
}
double metrics::caifi(void)
{	
	return totals.CN>0 ? totals.CI/totals.CN : 0;
}
double metrics::saifi(void)
{	
	return customers->hit_count>0 ? totals.CI/customers->hit_count:0;
}
double metrics::caidi(void)
{
	return totals.CI>0 ? saidi()/saifi() : 0;
}
double metrics::maifie(void)
{
	return totals.CMIE>0 ? totals.CMIE/customers->hit_count : 0;
}
double metrics::cemi(void)
{
	return totals.CNK>0 ? (double)totals.CNK/customers->hit_count : 0;
}
double metrics::cemsmi(void)
{
	return totals.CNT>0 ? (double)totals.CNT/customers->hit_count : 0;
}
double metrics::asifi(void)
{
	return totals.Li>0 ? totals.Li/totals.LT : 0;
}
double metrics::asidi(void)
{
	return totals.LDI>0 ? totals.LDI/totals.LT : 0;
}
double metrics::maifi(void)
{
	return totals.IMi>0 ? totals.IMi/customers->hit_count : 0;
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
	}
	catch (char *msg)
	{
		gl_error("create_metrics: %s", msg);
	}
	return 1;
}

EXPORT int init_metrics(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,metrics)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_metrics(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 1;
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
	}
	catch (char *msg)
	{
		gl_error("sync_metrics(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return t2;
}

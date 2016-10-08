// $Id: generator.cpp 5188 2015-06-23 22:57:40Z dchassin $

#include "transactive.h"

EXPORT_IMPLEMENT(generator)
EXPORT_SYNC(generator)
EXPORT_NOTIFY_PROP(generator,update)

generator::generator(MODULE *module)
{
	oclass = gld_class::create(module,"generator",sizeof(generator),PC_BOTTOMUP|PC_AUTOLOCK);
	oclass->trl = TRL_CONCEPT;
	defaults = this;
	if ( gl_publish_variable(oclass,
		PT_int64,"update",get_update_offset(),PT_HAS_NOTIFY_OVERRIDE,PT_DESCRIPTION,"incoming update handler",
		PT_double,"inertia[MJ]",get_inertia_offset(),PT_DESCRIPTION,"moment of inertia",
		PT_double,"capacity[MW]",get_capacity_offset(),PT_DESCRIPTION,"total rated generation capacity",
		PT_double,"schedule[MW]",get_schedule_offset(),PT_DESCRIPTION,"scheduled intertie exchange",
		PT_double,"actual[MW]",get_actual_offset(),PT_DESCRIPTION,"actual intertie exchange",
		PT_double,"control_gain[pu]",get_control_gain_offset(),PT_DESCRIPTION,"area control error gain",
		PT_double,"max_ramp[MW/s]",get_max_ramp_offset(),PT_DESCRIPTION,"ramp up rate limit",
		PT_double,"min_ramp[MW/s]",get_min_ramp_offset(),PT_DESCRIPTION,"ramp down rate limit",
		PT_double,"frequency_deadband[Hz]",get_frequency_deadband_offset(),PT_DESCRIPTION,"frequency control deadband",
		PT_set,"control_flags",get_control_flags_offset(),PT_DESCRIPTION,"generator control flags",
			PT_KEYWORD,"NONE",(set)GCF_NONE,
			PT_KEYWORD,"DROOP",(set)GCF_DROOP,
			PT_KEYWORD,"AGC",(set)GCF_AGC,
		NULL)<1 )
		exception("unable to publish transactive generator properties");
	memset(defaults,0,sizeof(generator));
}

int generator::create(void)
{
	memcpy(this,defaults,sizeof(*this));
	// TODO
	return 1;
}

int generator::init(OBJECT *parent)
{
	// check that parent is an control area
	if ( parent==NULL ) exception("parent must be defined");
	area = get_object(parent);
	if ( !area->isa("controlarea") ) exception("parent must be an controlarea");
	control = OBJECTDATA(parent,controlarea);

	// collect properties
	update_area = gld_property(area,"update");

	// initial state message to control area
	update_area.notify(TM_ACTUAL_GENERATION,capacity,actual,inertia);
	if ( verbose_options&VO_GENERATOR )
		fprintf(stderr,"GENERATOR      :   initial supply...... %8.3f (%s)\n",actual,get_name());

	// check min/max
	if ( max_output < min_output )
		throw "maximum output cannot be less than minimum output";

	// check frequency deadband
	if ( frequency_deadband<=0 )
		frequency_deadband = 0.025; // reasonable default for when frequency reg kicks in

	return 1;
}

int generator::isa(const char *type)
{
	return strcmp(type,"generator")==0;
}

TIMESTAMP generator::presync(TIMESTAMP t1)
{
	error("presync not implemented");
		/* TROUBLESHOOT
		   TODO
		*/
	return TS_INVALID;
}

TIMESTAMP generator::sync(TIMESTAMP t1)
{
	// only update generator that have non-zero capabilities
	if ( max_output>min_output )
	{
		double dt = (double)(t1 - gl_globalclock);
		double dP = 0;

		// implement droop control
		if ( control_flags&GCF_DROOP )
		{
			double df = nominal_frequency - control->get_frequency();
			if ( fabs(df)>frequency_deadband )
				dP += df/nominal_frequency*actual/droop_control;
		}

		// implement AGC
		if ( control_flags&GCF_AGC )
		{
			// simple first-order response to ACE signal
			double T = capacity>0 ? inertia/capacity : 0.0;
			double K = control_gain; // /s
			double r = capacity>0 ? control->get_ace()/capacity : 0.0; // MW
			double wr = T>0 ? r*K*(1-exp(-dt/T)) : 1.0; // pu
			dP += actual*wr;
		}

		// check ramp limits
		if ( min_ramp<max_ramp )
		{
			if ( fabs(dP)<min_ramp*dt ) dP = sign(dP)*min_ramp*dt;
			else if ( fabs(dP)>max_ramp*dt ) dP = sign(dP)*max_ramp*dt;
		}

		// check power limits
		double P = actual + dP;
		
		// check power limits
		if ( P<min_output )
			actual = min_output;
		else if ( max_output>min_output && P>max_output )
			actual = max_output;
		else
			actual = P;

		// update system info with SWING message
		update_area.notify(TM_ACTUAL_GENERATION,capacity,actual,inertia);

		// verbose output
		if ( verbose_options&VO_GENERATOR )
		{
			fprintf(stderr,"GENERATOR      : %s at %s\n", get_name(),(const char *)gld_clock().get_string());
			fprintf(stderr,"GENERATOR      :   inertia............. %8.3f\n", inertia);
			fprintf(stderr,"GENERATOR      :   capacity............ %8.3f\n", capacity);
			fprintf(stderr,"GENERATOR      :   control gain........ %8.3f\n", control_gain);
			fprintf(stderr,"GENERATOR      :   response............ %8.3f\n", dP);
			fprintf(stderr,"GENERATOR      :   power............... %8.3f\n", actual);
		}
	}

	return TS_NEVER;
}

TIMESTAMP generator::postsync(TIMESTAMP t1)
{
	error("postsync not implemented");
	return TS_INVALID;
}

int generator::notify_update(const char *message)
{
	error("update message '%s' is not recognized",message);
		/* TROUBLESHOOT
		   TODO
		*/
	return 0;
}

int generator::kmldump(int (*stream)(const char*, ...))
{
  if ( isnan(get_latitude()) || isnan(get_longitude()) ) return 0;
  stream("<Placemark>\n");
  stream("  <name>%s</name>\n", get_name());
  stream("  <description>\n<![CDATA[\n");
  // TODO add popup data here
  stream("    ]]>\n");
  stream("  </description>\n");
  stream("  <Point>\n");
  stream("    <coordinates>%f,%f</coordinates>\n", get_longitude(), get_latitude());
  stream("  </Point>\n");
  stream("</Placemark>");
  return 1;
}

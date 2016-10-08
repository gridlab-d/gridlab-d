// $Id: load.cpp 5188 2015-06-23 22:57:40Z dchassin $

#include "transactive.h"

EXPORT_IMPLEMENT(load)
EXPORT_SYNC(load)
EXPORT_NOTIFY_PROP(load,update)

load::load(MODULE *module)
{
	oclass = gld_class::create(module,"load",sizeof(load),PC_BOTTOMUP|PC_AUTOLOCK);
	oclass->trl = TRL_CONCEPT;
	defaults = this;
	if ( gl_publish_variable(oclass,
		PT_int64,"update",get_update_offset(),PT_HAS_NOTIFY_OVERRIDE,PT_DESCRIPTION,"incoming update handler",
		PT_double,"inertia[MJ]",get_inertia_offset(),PT_DESCRIPTION,"moment of inertia",
		PT_double,"capacity[MW]",get_capacity_offset(),PT_DESCRIPTION,"peak load capacity",
		PT_double,"schedule[MW]",get_schedule_offset(),PT_DESCRIPTION,"scheduled load demand",
		PT_double,"actual[MW]",get_actual_offset(),PT_DESCRIPTION,"actual load demand",
		PT_set,"control_flags",get_control_flags_offset(),PT_DESCRIPTION,"generator control flags",
			PT_KEYWORD,"NONE",(set)LCF_NONE,
			PT_KEYWORD,"UFLS",(set)LCF_UFLS,
			PT_KEYWORD,"LIFT",(set)LCF_LIFT,
			PT_KEYWORD,"ALC",(set)LCF_ALC,
		PT_double,"dr_fraction[pu]",get_dr_fraction_offset(),PT_DESCRIPTION,"load fraction under DR control",
		PT_double_array,"ufls",get_ufls_offset(),PT_DESCRIPTION,"underfrequency load shedding program ( f1 r1 ; f2 r2 ; ...) ",
		PT_double_array,"X",get_X_offset(),
		PT_double_array,"U",get_U_offset(),
		PT_double_array,"Y",get_Y_offset(),
		PT_double_array,"A",get_A_offset(),
		PT_double_array,"B",get_B_offset(),
		PT_double_array,"C",get_C_offset(),
		PT_double_array,"D",get_D_offset(),
		NULL)<1 )
		exception("unable to publish transactive load properties");
	memset(defaults,0,sizeof(load));
}

int load::create(void)
{
	memcpy(this,defaults,sizeof(*this));
	// TODO
	X.set_name("X");
	U.set_name("U");
	Y.set_name("Y");
	A.set_name("A");
	B.set_name("B");
	C.set_name("C");
	D.set_name("D");
	return 1;
}

int load::init(OBJECT *parent)
{
	// check that parent is an control area
	if ( parent==NULL ) exception("parent must be defined");
	area = get_object(parent);
	if ( !area->isa("controlarea") ) exception("parent must be an controlarea");
	control = OBJECTDATA(parent,controlarea);

	// collect properties
	update_area = gld_property(area,"update");

	// initial state message to control area
	update_area.notify(TM_ACTUAL_LOAD,actual,inertia);
	if ( verbose_options&VO_LOAD )
		fprintf(stderr,"LOAD           :   initial demand...... %8.3f (%s)\n",actual,get_name());

	return 1;
}

int load::isa(const char *type)
{
	return strcmp(type,"load")==0;
}

TIMESTAMP load::presync(TIMESTAMP t1)
{
	error("presync not implemented");
		/* TROUBLESHOOT
		   TODO
		*/
	return TS_INVALID;
}

TIMESTAMP load::sync(TIMESTAMP t1)
{
	// only do load model if load is non-zero
	if ( actual>0 )
	{
		double frequency = control->get_frequency();
		double ace = control->get_ace();
		double dP = 0;

		// implement UFLS program
		if ( control_flags&LCF_UFLS )
		{
			size_t r;
			for ( r=0 ; r<ufls.get_rows() ; r++ )
			{
				if ( frequency > ufls[r][0] )
					break;
				dP -= ufls[r][1];
			}
		}
		
		// implement lift control
		if ( control_flags&LCF_LIFT )
		{
			dP -= (nominal_frequency - frequency)/nominal_frequency / lift_control;
		}

		// implement DR model
		if ( (control_flags&LCF_ALC) )
		{
			Y = C*X + D*U;
			X += A*X + B*U;

			// curtail load
			dP -= Y[0][0];
		}

		// update system info with SWING message
		if ( actual<0 )
			exception("load is negative");
		else if ( actual>capacity )
			exception("load exceeds nameplate capacity");
		update_area.notify(TM_ACTUAL_LOAD,actual*(1+dP),inertia*(1+dP));
	}

	return TS_NEVER;
}

TIMESTAMP load::postsync(TIMESTAMP t1)
{
	error("postsync not implemented");
		/* TROUBLESHOOT
		   TODO
		*/
	return TS_INVALID;
}

int load::notify_update(const char *message)
{
	error("update message '%s' is not recognized",message);
		/* TROUBLESHOOT
		   TODO
		*/
	return 0;
}

int load::kmldump(int (*stream)(const char*, ...))
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

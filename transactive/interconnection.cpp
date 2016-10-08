// $Id: interconnection.cpp 5219 2015-07-06 01:41:36Z dchassin $

#include "transactive.h"

EXPORT_IMPLEMENT(interconnection)
EXPORT_PRECOMMIT(interconnection)
EXPORT_COMMIT(interconnection)
EXPORT_SYNC(interconnection)
EXPORT_NOTIFY_PROP(interconnection,update)

double interconnection::frequency_resolution = 0.001;

interconnection::interconnection(MODULE *module)
{
	oclass = gld_class::create(module,"interconnection",sizeof(interconnection),PC_PRETOPDOWN|PC_POSTTOPDOWN|PC_AUTOLOCK);
	oclass->trl = TRL_CONCEPT;
	defaults = this;
	if ( gl_publish_variable(oclass,
		PT_int64,"update",get_update_offset(),PT_HAS_NOTIFY_OVERRIDE,PT_DESCRIPTION,"incoming update handler",
		PT_double,"frequency[Hz]",get_frequency_offset(),PT_DESCRIPTION,"system frequency",
		PT_double,"inertia[MJ]",get_inertia_offset(),PT_DESCRIPTION,"system moment of inertia",
		PT_double,"capacity[MW]",get_capacity_offset(),PT_DESCRIPTION,"system rated power",
		PT_double,"damping[MW/Hz]",get_damping_offset(),PT_DESCRIPTION,"system damping",
		PT_double,"supply[MW]",get_supply_offset(),PT_DESCRIPTION,"system total supply",
		PT_double,"demand[MW]",get_demand_offset(),PT_DESCRIPTION,"system total demand",
		PT_double,"losses[MW]",get_losses_offset(),PT_DESCRIPTION,"system total losses",
		PT_double,"imbalance[MW]",get_imbalance_offset(),PT_DESCRIPTION,"system total imbalance",
		PT_enumeration,"initialize",get_initialize_offset(),PT_DESCRIPTION,"initial state desired (BALANCED,STEADY,TRANSIENT)",
			PT_KEYWORD,"BALANCED",(enumeration)IC_BALANCED,
			PT_KEYWORD,"STEADY",(enumeration)IC_STEADY,
			PT_KEYWORD,"TRANSIENT",(enumeration)IC_TRANSIENT,
		PT_object,"initial_balancing_unit",get_initial_balancing_unit_offset(),PT_DESCRIPTION,"generating unit to adjust for initial balancing",
		PT_enumeration,"status",get_status_offset(),PT_DESCRIPTION,"status flag",
			PT_KEYWORD,"OK",ICS_OK,
			PT_KEYWORD,"OVERCAPACITY",(enumeration)ICS_OVERCAPACITY,
			PT_KEYWORD,"OVERFREQUENCY",(enumeration)ICS_OVERFREQUENCY,
			PT_KEYWORD,"UNDERFREQUENCY",(enumeration)ICS_UNDERFREQUENCY,
			PT_KEYWORD,"BLACKOUT",(enumeration)ICS_BLACKOUT,
		NULL)<1 )
		exception("unable to publish transactive interconnection properties");
	memset(defaults,0,sizeof(interconnection));
	gl_global_create("transactive::frequency_resolution[Hz]",PT_double,&frequency_resolution,NULL);
}

int interconnection::create(void)
{
	memcpy(this,defaults,sizeof(*this));
	frequency = nominal_frequency; // Hz
	return 1;
}

int interconnection::init(OBJECT *parent)
{
	// check controlarea list
	if ( n_controlarea==0 ) 
	{
		if ( init_count++<2 )
			return 2; // defer until interties and control areas are registered
		else
			warning("interconnection with no control area");
			/* TROUBLESHOOT
			   TODO
			*/
	}


	// check frequency and damping values
	if ( frequency!=nominal_frequency ) 
		warning("initial off-nominal frequency");
		/* TROUBLESHOOT
		   TODO
		*/
	if ( damping<0 ) 
		exception("negative damping is not allowed");
		/* TROUBLESHOOT
		   TODO
		*/

	// build flow solver
	if ( n_intertie>0 )
	{
		engine = new solver;
		engine->create(n_controlarea,n_intertie);
		OBJECTLIST *item = NULL;
		for ( item=controlarea_list ; item!=NULL ; item=item->next )
			engine->add_node(item->obj);
		for ( item=intertie_list ; item!=NULL ; item=item->next )
			engine->add_line(item->obj);

		// check flow solver, setup and solve if ok
		if ( !engine->setup() || !engine->is_used() || !engine->is_ready() ) 
			exception("solver is not ready");

		last_solution_time = gl_globalclock;
		solve_powerflow();

		// balance system if desired
		if ( initialize==IC_BALANCED )
			return init_balanced();
		else if ( initialize==IC_STEADY )
			return init_steady();
		else
			return init_transient(); // done
	}
	else if ( n_controlarea>1 )
		exception("system has more than one controlarea but no interties"); 
	else
		return 1;
}
int interconnection::init_transient()
{
	// IC_TRANSIENT requires no further processing
	return 1;
}
int interconnection::init_steady()
{
	engine->update_b();
	const vector &b = engine->get_b();
	if ( b.sum()>0.001*b.norm(2) )
	{
		gl_error("interconnection is not at steady state");
		/* TROUBLESHOOT
		   The interconnection has be initialized with the STEADY option set.
		   When this option is used, the interconnection must be at steady state
		   before the clock is started.
		*/
		return 0;
	}
	else
		return 1;
}
int interconnection::init_balanced()
{
	gl_error("balanced state initialization not implemented");
	return 0;
}

int interconnection::isa(const char *type)
{
	return strcmp(type,"interconnection")==0;
}

int interconnection::precommit(TIMESTAMP t1)
{
	f0 = frequency;
	fr = frequency/nominal_frequency-1;
	last_solution_time = my()->clock;
	return 1;
}

TIMESTAMP interconnection::presync(TIMESTAMP t1)
{
	// reset accumulators only if control area updates are expected
	if ( n_controlarea>0 )
	{
		update = 0;
		inertia = 0.0;
		capacity = 0.0;
		supply = 0.0;
		demand = 0.0;
		losses = 0.0;
	}
	return TS_NEVER;
}

TIMESTAMP interconnection::sync(TIMESTAMP t1)
{
	error("sync must not be called");
	return TS_INVALID;
}

TIMESTAMP interconnection::postsync(TIMESTAMP t1)
{
	double dt = (double)(t1-last_solution_time);
	solve_powerflow(dt);
	last_solution_time = t1;

	// check inertia
	if ( inertia<0 ) warning("system inertia is negative");
		/* TROUBLESHOOT
		   TODO
		*/
	if ( inertia<1e-9 ) warning("system inertia is zero");
		/* TROUBLESHOOT
		   TODO
		*/

	// check capacity
	if ( capacity<0 ) warning("system capacity is negative");
		/* TROUBLESHOOT
		   TODO
		*/
	if ( capacity<1e-9 ) warning("system capacity is zero");
		/* TROUBLESHOOT
		   TODO
		*/

	// check supply
	if ( supply<0 ) warning("supply is negative (generators are acting like loads)");
		/* TROUBLESHOOT
		   TODO
		*/
	if ( supply<1e-9 ) warning("supply is zero (there are no generators)");
		/* TROUBLESHOOT
		   TODO
		*/

	// check demand
	if ( demand<0 ) warning("demand is negative (loads are acting like generators)");
		/* TROUBLESHOOT
		   TODO
		*/
	if ( demand<1e-9 ) warning("demand is zero (there are no loads)");
		/* TROUBLESHOOT
		   TODO
		*/

	// compute net load
	imbalance = supply-demand-losses;
	// compute frequency change [Source: Kunder (1994) p.135]
	if ( inertia<1e-9 ) return TS_NEVER;
	double wr = frequency/nominal_frequency;
	double df = 0.5 / (inertia*(wr*wr)) * ( imbalance - demand*damping*fr );
	frequency = f0 + df*dt;
	double dt1 = frequency_resolution/fabs(df);
	TIMESTAMP t2 = ( dt1>(double)TS_NEVER ) ? TS_NEVER : ( t1 + (TIMESTAMP)(dt1<1.0?1.0:dt1) );
	if ( verbose_options&VO_INTERCONNECTION )
	{
		fprintf(stderr,"INTERCONNECTION: %s at %s\n", get_name(),(const char *)gld_clock().get_string());
		fprintf(stderr,"INTERCONNECTION:    total supply........ %8.3f\n", supply);
		fprintf(stderr,"INTERCONNECTION:    total demand........ %8.3f\n", demand);
		fprintf(stderr,"INTERCONNECTION:    total losses........ %8.3f\n", losses);
		fprintf(stderr,"INTERCONNECTION:    total imbalance..... %8.3f (%.1f%%)\n", imbalance,imbalance/supply*100);
		fprintf(stderr,"INTERCONNECTION:    system frequency.... %8.3f (dt/dt=%.1f)\n", frequency,df);
		fprintf(stderr,"INTERCONNECTION:    steady until........ %s [dt<%.1fs]\n", (const char*)gld_clock(t2).get_string(),dt1);
	}

	// update status flag
	if ( supply>capacity ) status = ICS_OVERCAPACITY;
	else if ( frequency>maximum_frequency ) status = ICS_OVERFREQUENCY;
	else if ( frequency<minimum_frequency ) status = ICS_UNDERFREQUENCY;
	else if ( supply==0 || demand==0 ) status = ICS_BLACKOUT;
	else status = ICS_OK;
	return t2;
}

TIMESTAMP interconnection::commit(TIMESTAMP t1, TIMESTAMP t2)
{
	bool out_of_bound = frequency<minimum_frequency || frequency>maximum_frequency;
	if ( !out_of_bound ) return TS_NEVER;
	switch ( frequency_bounds ) {
	case FB_NONE:
		return TS_NEVER;
	case FB_SOFT:
		warning("frequency %.3f Hz out of bounds %.3f to %.3f",frequency,minimum_frequency,maximum_frequency);
		return TS_NEVER;
	case FB_HARD:
		error("frequency %.3f Hz out of bounds %.3f to %.3f",frequency,minimum_frequency,maximum_frequency);
		return TS_INVALID;
	default:
		error("frequency_bounds value %d is invalid",frequency_bounds);
		return TS_INVALID;
	}
}

int interconnection::notify_update(const char *message)
{
	trace_message(TMT_INTERCONNECTION,message);

	// AREA SWING
	// E is kinetic energy (1/2 J w^2)
	// R is rated power
	// G is input power
	// D is output power
	// L is losses
	double e,r,g,d,l;
	if ( sscanf(message,TM_AREA_STATUS,&e,&r,&g,&d,&l)==5 )
	{
		if ( n_controlarea>0 )
		{
			inertia += e;
			capacity += r;
			supply += g;
			demand += d;
			losses += l;
			update++;
			return 1;
		}
		else
		{
			exception("control area status message not expected because no control areas were registered during initialization");
			/* TROUBLESHOOT
			   TODO
			*/
			return 0;
		}
	}

	// REGISTER INTERTIE
	OBJECT *obj;
	if ( sscanf(message,TM_REGISTER_INTERTIE,&obj)==1 )
	{
		intertie_list = add_object(intertie_list,obj);
		gld_object *line = get_object(obj);
		if ( !line->isa("intertie") ) 
			exception("attempt by non-intertie object to register with interconnection as an intertie");
			/* TROUBLESHOOT
			   TODO
			*/
		n_intertie++;
		return 1;
	}

	// REGISTER CONTROLAREA
	if ( sscanf(message,TM_REGISTER_CONTROLAREA,&obj)==1 )
	{
		controlarea_list = add_object(controlarea_list,obj);
		gld_object *area = get_object(obj);
		if ( !area->isa("controlarea") ) 
			exception("attempt by non-controlarea object to register with interconnection as a controlarea");
			/* TROUBLESHOOT
			   TODO
			*/
		n_controlarea++;
		return 1;
	}

	error("update message '%s' is not recognized",message);
		/* TROUBLESHOOT
		   TODO
		*/
	return 0;
}

void interconnection::solve_powerflow(double dt)
{
	if ( engine==NULL || !engine->solve(dt) )
		exception("solver failed");
		/* TROUBLESHOOT
		   TODO
		*/
}

int interconnection::kmldump(int (*stream)(const char*, ...))
{
	if ( isnan(get_latitude()) || isnan(get_longitude()) ) return 0;
	stream("<Folder><name>%s</name>\n", get_name());
	stream("<Placemark>\n");
	stream("  <name>Interconnection</name>\n");
	stream("  <description>\n<![CDATA[<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=3 STYLE=\"font-size:10;\">\n");
#define TR "    <TR><TH ALIGN=LEFT>%s</TH><TD ALIGN=RIGHT>%s</TD></TR>\n"
#define HREF "    <TR><TH ALIGN=LEFT><A HREF=\"%s_%s.png\"  ONCLICK=\"window.open('%s_%s.png');\">%s</A></TH><TD ALIGN=RIGHT>%s</TD></TR>\n"
	gld_clock now(my()->clock);
	stream("    <caption>%s</caption>",(const char*)now.get_string());
	stream(TR,"Status",(const char*)get_status_string());
	stream(HREF,(const char*)get_name(),"frequency",(const char*)get_name(),"frequency","Frequency",(const char*)get_frequency_string());
	stream(HREF,(const char*)get_name(),"capacity",(const char*)get_name(),"capacity","Capacity",(const char*)get_capacity_string());
	stream(HREF,(const char*)get_name(),"supply",(const char*)get_name(),"supply","Supply",(const char*)get_supply_string());
	stream(HREF,(const char*)get_name(),"demand",(const char*)get_name(),"demand","Demand",(const char*)get_demand_string());
	stream(HREF,(const char*)get_name(),"losses",(const char*)get_name(),"losses","Losses",(const char*)get_losses_string());
	stream(HREF,(const char*)get_name(),"imbalance",(const char*)get_name(),"imbalance","Imbalance",(const char*)get_imbalance_string());
	stream(HREF,(const char*)get_name(),"inertia",(const char*)get_name(),"inertia","Inertia",(const char*)get_inertia_string());
	stream(HREF,(const char*)get_name(),"damping",(const char*)get_name(),"damping","Damping",(const char*)get_damping_string());
	stream("    </TABLE>]]></description>\n");
	stream("  <styleUrl>#%s_mark_%s</styleUrl>\n",my()->oclass->name, (const char*)get_status_string());
	stream("  <Point>\n");
	stream("    <coordinates>%f,%f</coordinates>\n", get_longitude(), get_latitude());
	stream("  </Point>\n");
	stream("</Placemark>\n");

	stream("  <Folder><name>%s</name>\n","Control areas");
	for ( OBJECTLIST *item=controlarea_list ; item!=NULL ; item=item->next )
		OBJECTDATA(item->obj,controlarea)->kmldump(stream);
	stream("%s","  </Folder>\n");

	stream("  <Folder><name>%s</name>\n","Interties");
	for ( OBJECTLIST *item=intertie_list ; item!=NULL ; item=item->next )
		OBJECTDATA(item->obj,intertie)->kmldump(stream);
	stream("%s","  </Folder>\n");

	stream("%s","</Folder>\n");
	return 1;
}

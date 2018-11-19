// powerflow/pole.cpp
// Copyright (C) 2018, Stanford University

#include "pole.h"
#include "line.h"
#include "overhead_line.h"

CLASS *pole::oclass = NULL;
CLASS *pole::pclass = NULL;

pole::pole(MODULE *mod) : node(mod)
{
	if ( oclass == NULL )
	{
		pclass = node::oclass;
		oclass = gl_register_class(mod,"pole",sizeof(pole),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if ( oclass == NULL )
			throw "unable to register class pole";
		oclass->trl = TRL_PROTOTYPE;
		if ( gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_enumeration, "pole_status", PADDR(pole_status), PT_DESCRIPTION, "pole status",
				PT_KEYWORD, "OK", (enumeration)PS_OK,
				PT_KEYWORD, "FAILED", (enumeration)PS_FAILED,
			PT_enumeration,"pole_type", PADDR(pole_type), PT_DESCRIPTION, "material from which pole is made",
				PT_KEYWORD, "WOOD", (enumeration)PT_WOOD,
				PT_KEYWORD, "CONCRETE", (enumeration)PT_CONCRETE,
				PT_KEYWORD, "STEEL", (enumeration)PT_STEEL,
			PT_double, "tilt_angle[rad]", PADDR(tilt_angle), PT_DESCRIPTION, "tilt angle of pole",
			PT_double, "tilt_direction[deg]", PADDR(tilt_direction), PT_DESCRIPTION, "tilt direction of pole",
			PT_object, "weather", PADDR(weather), PT_DESCRIPTION, "weather data",
			PT_object, "configuration", PADDR(configuration), PT_DESCRIPTION, "configuration data",
			PT_double, "equipment_area[sf]", PADDR(equipment_area), PT_DESCRIPTION, "equipment cross sectional area",
			PT_double, "equipment_height[ft]", PADDR(equipment_height), PT_DESCRIPTION, "equipment height on pole",
			PT_double, "pole_stress[pu]", PADDR(pole_stress), PT_DESCRIPTION, "ratio of actual stress to critical stress",
			PT_double, "susceptibility[pu*s/m]", PADDR(susceptibility), PT_DESCRIPTION, "susceptibility of pole to wind stress (derivative of pole stress w.r.t wind speed)",
			PT_double, "total_moment[ft*lb]", PADDR(total_moment), PT_DESCRIPTION, "the total moment on the pole.",
			PT_double, "resisting_moment[ft*lb]", PADDR(resisting_moment), PT_DESCRIPTION, "the resisting moment on the pole.",
			PT_double, "wind_failure[m/s]", PADDR(wind_failure), PT_DESCRIPTION, "wind speed at pole failure.",
			NULL) < 1 ) throw "unable to publish properties in " __FILE__;
	}
}

int pole::isa(char *classname)
{
	return strcmp(classname,"pole")==0 || node::isa(classname);
}

int pole::create(void)
{
	pole_status = PS_OK;
	weather = NULL;
	configuration = NULL;
	tilt_angle = 0.0;
	tilt_direction = 0.0;
	wind_speed = NULL;
	wind_direction = NULL;
	wind_gust = NULL;
	last_wind_speed = 0.0;
	wire_data = NULL;
	equipment_area = 0.0;
	equipment_height = 0.0;
	return node::create();
}

int pole::init(OBJECT *parent)
{
	OBJECT *my = (OBJECT*)(this)-1;

	// configuration
	if ( configuration == NULL || ! gl_object_isa(configuration,"pole_configuration") )
	{
		gl_error("configuration is not set to a pole_configuration object");
		return 0;		
	}
	config = OBJECTDATA(configuration,pole_configuration);

	// tilt
	if ( tilt_angle < 0 || tilt_angle > 90 )
	{
		gl_error("pole tilt angle is not between and 0 and 90 degrees");
		return 0;
	}
	if ( tilt_direction < 0 || tilt_direction >= 360 )
	{
		gl_error("pole tilt direction is not between 0 and 360 degrees");
		return 0;
	}

	// weather
	if ( weather == NULL || ! gl_object_isa(weather,"climate") )
	{
		gl_error("weather is not set to a climate object");
		return 0;
	}
	wind_speed = (double*)gl_get_addr(weather,"wind_speed");
	if ( wind_speed == NULL )
	{
		gl_error("weather object does not provide wind speed data");
		return 0;
	}
	wind_direction = (double*)gl_get_addr(weather,"wind_dir");
	if ( wind_direction == NULL )
	{
		gl_error("weather object does not provide wind direction data");
		return 0;
	}
	wind_gust = (double*)gl_get_addr(weather,"wind_gust");
	if ( wind_gust == NULL )
	{
		gl_error("weather object does not provide wind gust data");
		return 0;
	}

	// calculation resisting moment
	resisting_moment = 0.008186 
		* config->strength_factor_250b_wood 
		* config->fiber_strength
		* ( config->ground_diameter * config->ground_diameter * config->ground_diameter);
	verbose("resisting moment %.0f ft*lb",resisting_moment);

	// collect wire data
	static FINDLIST *all_ohls = NULL;
	if ( all_ohls == NULL )
		all_ohls = gl_find_objects(FL_NEW,FT_CLASS,SAME,"overhead_line",FT_END);
	OBJECT *obj = NULL;
	int n_lines = 0;
	while ( ( obj = gl_find_next(all_ohls,obj) ) != NULL )
	{
		overhead_line *line = OBJECTDATA(obj,overhead_line);
		if ( line->from == my || line->to == my )
		{
			n_lines++;
			line_configuration *line_config = OBJECTDATA(line->configuration,line_configuration);
			if ( line_config == NULL )
			{
				warning("line %s has no line configuration--skipping",line->get_name());
				break;
			}
			line_spacing *spacing = OBJECTDATA(line_config->line_spacing,line_spacing);
			if ( spacing == NULL )
			{
				warning("line configure %s has no line spacing data--skipping",line_config->get_name());
				break;
			}
			overhead_line_conductor *phaseA = OBJECTDATA(line_config->phaseA_conductor,overhead_line_conductor);
			if ( phaseA != NULL )
				add_wire(spacing->distance_AtoE,phaseA->cable_diameter,0.0,4430,line->length/2);
			overhead_line_conductor *phaseB = OBJECTDATA(line_config->phaseB_conductor,overhead_line_conductor);
			if ( phaseB != NULL )
				add_wire(spacing->distance_BtoE,phaseB->cable_diameter,0.0,4430,line->length/2);
			overhead_line_conductor *phaseC = OBJECTDATA(line_config->phaseC_conductor,overhead_line_conductor);
			if ( phaseC != NULL )
				add_wire(spacing->distance_CtoE,phaseC->cable_diameter,0.0,4430,line->length/2);
			overhead_line_conductor *phaseN = OBJECTDATA(line_config->phaseN_conductor,overhead_line_conductor);
			if ( phaseN != NULL )
				add_wire(spacing->distance_NtoE,phaseN->cable_diameter,0.0,2190,line->length/2);
			verbose("found link %s",(const char*)(line->get_name()));
		}
	}
	if ( wire_data == NULL )
	{
		warning("no wire data found--wire loading is not included");
	}
	is_deadend = ( n_lines < 2 );

	return node::init(parent);
}

TIMESTAMP pole::presync(TIMESTAMP t0)
{
	if ( pole_status == PS_FAILED && down_time+config->repair_time > gl_globalclock )
	{
		warning("pole repaired");
		tilt_angle = 0.0;
		tilt_direction = 0.0;
		pole_status = PS_OK;

	}
	if ( pole_status == PS_OK && last_wind_speed != *wind_speed )
	{
		gld_clock dt;
		wind_pressure = 0.00256*2.24 * (*wind_speed)*(*wind_speed);
		wind_failure = *wind_speed;
		double pole_height = config->pole_length - config->pole_depth;
		pole_moment = wind_pressure * pole_height * pole_height * (config->ground_diameter+2*config->top_diameter)/72 * config->overload_factor_transverse_general;
		equipment_moment = wind_pressure * equipment_area * equipment_height * config->overload_factor_transverse_general;

		WIREDATA *wire;
		wire_load = 0.0;
		wire_moment = 0.0;
		wire_tension = 0.0;
		wire_load_nowind = 0.0;
		wire_moment_nowind = 0.0;
		wire_tension_nowind = 0.0;
		for ( wire = get_first_wire() ; wire != NULL ; wire = get_next_wire(wire) )
		{
			double load = wind_pressure * (wire->diameter+2*ice_thickness)/12;
			wire_load += load;
			wire_moment += wire->span * load * wire->height * config->overload_factor_transverse_wire;
			wire_tension += wire->tension * config->overload_factor_transverse_wire * sin(tilt_angle/2) * wire->height;

			double load_nowind = (wire->diameter+2*ice_thickness)/12;
			wire_load_nowind += load_nowind;
			wire_moment_nowind += wire->span * load_nowind * wire->height * config->overload_factor_transverse_wire;
			wire_tension_nowind += wire->tension * config->overload_factor_transverse_wire * sin(tilt_angle/2) * wire->height;
		}
		total_moment = pole_moment + equipment_moment + wire_moment + wire_tension;
		pole_stress = total_moment/resisting_moment;
		if ( (*wind_speed) > 0 )
			susceptibility = 2*(pole_moment+equipment_moment+wire_moment)/resisting_moment/(*wind_speed);
		else
			susceptibility = 0.0;
		verbose("wind %4.1f psi, pole %4.0f ft*lb, equipment %4.0f ft*lb, wires %4.0f ft*lb, margin %.0f%%", (const char*)(dt.get_string()), wind_pressure, pole_moment, equipment_moment, wire_moment, pole_stress*100);
		pole_status = ( pole_stress < 1.0 ? PS_OK : PS_FAILED );
		if ( pole_status == PS_FAILED )
		{
			warning("pole failed at %.0f%% loading",total_moment/resisting_moment*100);
			down_time = gl_globalclock;
			
		}
		last_wind_speed = *wind_speed;

		pole_moment_nowind = pole_height * pole_height * (config->ground_diameter+2*config->top_diameter)/72 * config->overload_factor_transverse_general;
		equipment_moment_nowind = equipment_area * equipment_height * config->overload_factor_transverse_general;
		double wind_pressure_failure = (resisting_moment) / (pole_moment_nowind + equipment_moment_nowind + wire_moment_nowind + wire_tension_nowind);
		wind_failure = sqrt(wind_pressure_failure / (0.00256 * 2.24));

	}
	TIMESTAMP t1 = node::presync(t0);
	TIMESTAMP t2 = ( pole_status == PS_FAILED ? down_time + config->repair_time : TS_NEVER );
	return ( t1 > t0 && t2 < t1 ) ? t1 : t2;
}

TIMESTAMP pole::sync(TIMESTAMP t0)
{
	return node::sync(t0);
}

TIMESTAMP pole::postsync(TIMESTAMP t0)
{
	return node::postsync(t0);
}

EXPORT int create_pole(OBJECT **obj, OBJECT *parent)
{
       try
        {
                *obj = gl_create_object(pole::oclass);
                if (*obj!=NULL)
                {
                        pole *my = OBJECTDATA(*obj,pole);
                        gl_set_parent(*obj,parent);
                        return my->create();
                }
                else
                        return 0;
        }
        CREATE_CATCHALL(pole);
}

EXPORT int init_pole(OBJECT *obj)
{
        try {
                pole *my = OBJECTDATA(obj,pole);
                return my->init(obj->parent);
        }
        INIT_CATCHALL(pole);
}

EXPORT int isa_pole(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,pole)->isa(classname);
}

EXPORT TIMESTAMP sync_pole(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		pole *pObj = OBJECTDATA(obj,pole);
		TIMESTAMP t1 = TS_NEVER;
		switch ( pass ) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	}
	SYNC_CATCHALL(pole);
}

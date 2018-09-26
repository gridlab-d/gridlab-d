// powerflow/pole.cpp
// Copyright (C) 2018, Stanford University

#include "pole.h"

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
			PT_enumeration,"pole_type", PADDR(pole_type), PT_DESCRIPTION, "material from which pole is made",
				PT_KEYWORD, "WOOD", (enumeration)PT_WOOD,
				PT_KEYWORD, "CONCRETE", (enumeration)PT_CONCRETE,
				PT_KEYWORD, "STEEL", (enumeration)PT_STEEL,
			PT_double, "tilt_angle[deg]", PADDR(tilt_angle), PT_DESCRIPTION, "tilt angle of pole",
			PT_double, "tilt_direction[deg]", PADDR(tilt_direction), PT_DESCRIPTION, "tilt direction of pole",
			PT_object, "weather", PADDR(weather), PT_DESCRIPTION, "weather data",
			PT_object, "configuration", PADDR(configuration), PT_DESCRIPTION, "configuration data",
			PT_double, "equipment_area[sf]", PADDR(equipment_area), PT_DESCRIPTION, "equipment cross sectional area",
			PT_double, "equipment_height[ft]", PADDR(equipment_height), PT_DESCRIPTION, "equipment height on pole",
			PT_double, "resisting_moment[ft*lb]", PADDR(resisting_moment), PT_DESCRIPTION, "pole ultimate resisting moment",
			PT_double, "equipment_moment[ft*lb]", PADDR(equipment_moment), PT_DESCRIPTION, "equipment moment",
			PT_double, "wind_pressure[psi]", PADDR(wind_pressure), PT_DESCRIPTION, "wind pressure",
			NULL) < 1 ) throw "unable to publish properties in " __FILE__;
	}
}

int pole::isa(char *classname)
{
	return strcmp(classname,"pole")==0 || node::isa(classname);
}

int pole::create(void)
{
	weather = NULL;
	configuration = NULL;
	tilt_angle = 0.0;
	tilt_direction = 0.0;
	weather = NULL;
	wind_speed = NULL;
	wind_direction = NULL;
	wind_gust = NULL;
	last_wind_speed = 0.0;
	memset(wire_data,0,sizeof(wire_data));
	return node::create();
}

int pole::init(OBJECT *parent)
{
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
		gl_error("polt tilt direction is not between 0 and 360 degrees");
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

	resisting_moment = 0.008186 
		* config->strength_factor_250b_wood 
		* config->fiber_strength
		* ( config->ground_diameter * config->ground_diameter * config->ground_diameter);
	verbose("resisting moment %.0f ft*lb",resisting_moment);
	return node::init(parent);
}

TIMESTAMP pole::presync(TIMESTAMP t0)
{
	if ( last_wind_speed != *wind_speed )
	{
		gld_clock dt;
		wind_pressure = 0.00256*2.24 * (*wind_speed)*(*wind_speed);
		double pole_height = config->pole_length - config->pole_depth;
		pole_moment = wind_pressure * pole_height * pole_height * (config->ground_diameter+2*config->top_diameter)/72 * config->overload_factor_transverse_general;
		equipment_moment = wind_pressure * equipment_area * equipment_height * config->overload_factor_transverse_general;
		wire_load = 0.0;
		wire_moment = 0.0;

		unsigned int n;
		for ( n = 0 ; n < _N_WIRETYPES ; n++ )
		{
			wire_load += wind_pressure * (wire_data[n].diameter+2*ice_thickness)/12;
			wire_moment += wire_load * wire_data[n].height * config->overload_factor_transverse_wire;
		}
		double total_moment = pole_moment + equipment_moment + wire_moment;
		verbose("wind %4.1f psi, pole %4.0f ft*lb, equipment %4.0f ft*lb, wires %4.0f ft*lb, margin %.0f%%", (const char*)(dt.get_string()), wind_pressure, pole_moment, equipment_moment, wire_moment, total_moment/resisting_moment*100);
	}
	return node::presync(t0);
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

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
			PT_double, "design_ice_thickness[in]", PADDR(ice_thickness), PT_DESCRIPTION, "design ice thickness on conductors",
			PT_double, "design_wind_loading[psi]", PADDR(wind_loading), PT_DESCRIPTION, "design wind loading on pole",
			PT_double, "design_temperature[degF]", PADDR(temperature), PT_DESCRIPTION, "design temperature for pole",
			PT_double, "overload_factor", PADDR(overload_factor), PT_DESCRIPTION, "design overload factor",
			PT_double, "strength_factor", PADDR(strength_factor), PT_DESCRIPTION, "design strength factor",
			PT_double, "pole_length[ft]", PADDR(pole_length), PT_DESCRIPTION, "total length of pole (including underground portion)",
			PT_double, "ground_depth[ft]", PADDR(ground_depth), PT_DESCRIPTION, "depth of pole underground",
			PT_double, "ground_diameter[in]", PADDR(ground_diameter), PT_DESCRIPTION, "diameter of pole at ground level",
			PT_double, "top_diameter[in]", PADDR(top_diameter), PT_DESCRIPTION, "diameter of pole at top",
			PT_double, "fiber_strength[psi]", PADDR(fiber_strength), PT_DESCRIPTION, "pole structural strength",
			PT_double, "resisting_moment[ft*lb]", PADDR(resisting_moment), PT_DESCRIPTION, "pole ultimate resisting moment",
			NULL) < 1 ) throw "unable to publish properties in " __FILE__;
	}
}

int pole::isa(char *classname)
{
	return strcmp(classname,"pole")==0 || node::isa(classname);
}

int pole::create(void)
{
	pole_type = PT_WOOD;
	tilt_angle = 0.0;
	tilt_direction = 0.0;
	weather = NULL;
	wind_speed = NULL;
	wind_direction = NULL;
	wind_gust = NULL;
	return node::create();
}

double pole::get_pole_diameter(double height)
{
	return (pole_length-height)*(ground_diameter-top_diameter)/(pole_length-ground_depth);
}

int pole::init(OBJECT *parent)
{
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

	resisting_moment = 0.000264*strength_factor*fiber_strength*(ground_diameter*ground_diameter*ground_diameter*247.6);
	return node::init(parent);
}

TIMESTAMP pole::presync(TIMESTAMP t0)
{
	if ( *wind_speed > 0 || *wind_gust > 0 )
	{
		gld_clock dt;
		printf("%s: wind %g deg at %g m/s gusting to %g mph\n", (const char*)(dt.get_string()), *wind_direction, *wind_speed, *wind_gust);
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

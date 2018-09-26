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
			PT_double, "design_ice_thickness[in]", PADDR(design_ice_thickness), PT_DESCRIPTION, "design ice thickness on conductors",
			PT_double, "design_wind_loading[psi]", PADDR(design_wind_loading), PT_DESCRIPTION, "design wind loading on pole",
			PT_double, "design_temperature[degF]", PADDR(design_temperature), PT_DESCRIPTION, "design temperature for pole",
			PT_double, "overload_factor_vertical", PADDR(overload_factor_vertical), PT_DESCRIPTION, "design overload factor (vertical)",
			PT_double, "overload_factor_transverse_general", PADDR(overload_factor_transverse_general), PT_DESCRIPTION, "design overload factor (transverse general)",
			PT_double, "overload_factor_transverse_crossing", PADDR(overload_factor_transverse_crossing), PT_DESCRIPTION, "design overload factor (transverse crossing)",
			PT_double, "overload_factor_transverse_wire", PADDR(overload_factor_transverse_wire), PT_DESCRIPTION, "design overload factor (transverse wire)",
			PT_double, "overload_factor_longitudinal_general", PADDR(overload_factor_longitudinal_general), PT_DESCRIPTION, "design overload factor (longitudinal general)",
			PT_double, "overload_factor_longitudinal_deadend", PADDR(overload_factor_longitudinal_deadend), PT_DESCRIPTION, "design overload factor (longitudinal deadend)",
			PT_double, "strength_factor_250b_wood", PADDR(strength_factor_250b_wood), PT_DESCRIPTION, "design strength factor (Rule 250B wood structure)",
			PT_double, "strength_factor_250b_support", PADDR(strength_factor_250b_support), PT_DESCRIPTION, "design strength factor (Rule 250B support hardware)",
			PT_double, "strength_factor_250c_wood", PADDR(strength_factor_250c_wood), PT_DESCRIPTION, "design strength factor (Rule 250C wood structure)",
			PT_double, "strength_factor_250c_support", PADDR(strength_factor_250c_support), PT_DESCRIPTION, "design strength factor (Rule 250C support hardware)",
			PT_double, "pole_length[ft]", PADDR(pole_length), PT_DESCRIPTION, "total length of pole (including underground portion)",
			PT_double, "pole_depth[ft]", PADDR(pole_depth), PT_DESCRIPTION, "depth of pole underground",
			PT_double, "ground_diameter[in]", PADDR(ground_diameter), PT_DESCRIPTION, "diameter of pole at ground level",
			PT_double, "top_diameter[in]", PADDR(top_diameter), PT_DESCRIPTION, "diameter of pole at top",
			PT_double, "fiber_strength[psi]", PADDR(fiber_strength), PT_DESCRIPTION, "pole structural strength",
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
	pole_type = PT_WOOD;
	tilt_angle = 0.0;
	tilt_direction = 0.0;
	weather = NULL;
	wind_speed = NULL;
	wind_direction = NULL;
	wind_gust = NULL;

	// defaults from chart 1 - medium loading district
	design_ice_thickness = 0.25;
	design_wind_loading = 4.0;
	design_temperature = 15;

	// defaults from chart 2 - grade C overload capacity factors
	overload_factor_vertical = 1.9; 
	overload_factor_transverse_general = 1.75;
	overload_factor_transverse_crossing = 2.2;
	overload_factor_transverse_wire = 1.65;
	overload_factor_longitudinal_general = 1.0;
	overload_factor_longitudinal_deadend = 1.3;

	// defaults from chart 3 - grade C strength factors
	strength_factor_250b_wood = 0.85;
	strength_factor_250b_support = 1.0;
	strength_factor_250c_wood = 0.75;
	strength_factor_250c_support = 1.0;

	// defaults from chart 4 - type 45/5 pole dimensions
	pole_length = 45.0;
	pole_depth = 4.5;
	top_diameter = 19/6.28;
	ground_diameter = 32.5/6.28;

	// defaults from chart 5 - southern yellow pine
	fiber_strength = 8000;

	// equipment
	equipment_area = 0.0; 
	equipment_height = 28;

	// values to be computed
	resisting_moment = 0.0;

	return node::create();
}

double pole::get_pole_diameter(double height)
{
	return (pole_length-height)*(ground_diameter-top_diameter)/(pole_length-pole_depth);
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

	resisting_moment = 0.000264*strength_factor_250b_wood*fiber_strength*(ground_diameter*ground_diameter*ground_diameter*247.6);
	return node::init(parent);
}

TIMESTAMP pole::presync(TIMESTAMP t0)
{
	gld_clock dt;
	wind_pressure = 0.00256*2.24 * (*wind_speed)*(*wind_speed);
	double pole_height = pole_length - pole_depth;
	pole_moment = wind_pressure * pole_height*pole_height * (ground_diameter+2*top_diameter)/72*overload_factor_transverse_general;
	equipment_moment = wind_pressure * equipment_area * equipment_height * overload_factor_transverse_general;
	wire_load = wind_pressure * (cable_diameter+2*ice_thickness)/12;
	wire_moment = wire_load * cable_height * overload_factor_transverse_wire;
	printf("%s: wind %4.1f psi, pole %4.0f ft*lb, equipment %4.0f ft*lb, wires %4.0f ft*lb\n", (const char*)(dt.get_string()), wind_pressure, pole_moment, equipment_moment, wire_moment);
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

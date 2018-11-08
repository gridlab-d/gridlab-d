// powerflow/pole_configuration.cpp
// Copyright (C) 2018 Stanford University

#include "pole_configuration.h"


CLASS* pole_configuration::oclass = NULL;
CLASS* pole_configuration::pclass = NULL;

pole_configuration::pole_configuration(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == NULL)
	{
		oclass = oclass = gl_register_class(mod,"pole_configuration",sizeof(pole_configuration),0x00);
		if (oclass==NULL)
			throw "unable to register class pole_configuration";
		else
			oclass->trl = TRL_PROVEN;
        
        if(gl_publish_variable(oclass,
			PT_enumeration,"pole_type", PADDR(pole_type), PT_DESCRIPTION, "material from which pole is made",
				PT_KEYWORD, "WOOD", (enumeration)PT_WOOD,
				PT_KEYWORD, "CONCRETE", (enumeration)PT_CONCRETE,
				PT_KEYWORD, "STEEL", (enumeration)PT_STEEL,
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
			PT_double, "repair_time[s]", PADDR(repair_time), PT_DESCRIPTION, "pole repair time",
            NULL) < 1) GL_THROW("unable to publish pole_configuration properties in %s",__FILE__);
    }
}

int pole_configuration::create(void)
{
    // Set up defaults for 45/5
    

    // defaults from chart 1 - medium loading district
    design_ice_thickness = 0.25;
    design_wind_loading = 4.0;
    design_temperature = 15.0;
	
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
    top_diameter = 19/3.14;
    ground_diameter = 32.5/3.14;

    // defaults from chart 5 - southern yellow pine
    fiber_strength = 8000;

	repair_time = 86400;
	return 1;
}

int pole_configuration::isa(char *classname)
{
	return strcmp(classname,"pole_configuration") == 0;
}

double pole_configuration::get_pole_diameter(double height)
{
	return (pole_length-height)*(ground_diameter-top_diameter)/(pole_length-pole_depth);
}

EXPORT int create_pole_configuration(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(pole_configuration::oclass);
		if (*obj!=NULL)
		{
			pole_configuration *my = OBJECTDATA(*obj,pole_configuration);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(pole_configuration);
}

EXPORT int isa_pole_configuration(OBJECT *obj, char *classname)
{
	return strcmp(classname,"pole_configuration") == 0;
}


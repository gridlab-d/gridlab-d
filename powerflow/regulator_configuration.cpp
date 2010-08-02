// $Id: regulator_configuration.cpp 1182 2008-12-22 22:08:36Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file regulator_configuration.cpp
	@addtogroup regulator_configuration
	@ingroup regulator	
	
	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "regulator.h"
#include "node.h"

//////////////////////////////////////////////////////////////////////////
// regulator_configuration CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* regulator_configuration::oclass = NULL;
CLASS* regulator_configuration::pclass = NULL;

regulator_configuration::regulator_configuration(MODULE *mod) : powerflow_library(mod)
{
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"regulator_configuration",sizeof(regulator_configuration),0x00);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration, "connect_type",PADDR(connect_type),
				PT_KEYWORD, "UNKNOWN", UNKNOWN,
				PT_KEYWORD, "WYE_WYE", WYE_WYE,
				PT_KEYWORD, "OPEN_DELTA_ABBC", OPEN_DELTA_ABBC,
				PT_KEYWORD, "OPEN_DELTA_BCAC", OPEN_DELTA_BCAC,
				PT_KEYWORD, "OPEN_DELTA_CABA", OPEN_DELTA_CABA,
				PT_KEYWORD, "CLOSED_DELTA", CLOSED_DELTA,
			PT_double, "band_center[V]",PADDR(band_center),
			PT_double, "band_width[V]",PADDR(band_width),	
			PT_double, "time_delay[s]",PADDR(time_delay),
			PT_double, "dwell_time[s]",PADDR(dwell_time),
			PT_int16, "raise_taps",PADDR(raise_taps),
			PT_int16, "lower_taps",PADDR(lower_taps),
			PT_double, "current_transducer_ratio[pu]",PADDR(CT_ratio),	
			PT_double, "power_transducer_ratio[pu]",PADDR(PT_ratio),	
			PT_double, "compensator_r_setting_A[V]",PADDR(ldc_R_V_A),
			PT_double, "compensator_r_setting_B[V]",PADDR(ldc_R_V_B),
			PT_double, "compensator_r_setting_C[V]",PADDR(ldc_R_V_C),
			PT_double, "compensator_x_setting_A[V]",PADDR(ldc_X_V_A),
			PT_double, "compensator_x_setting_B[V]",PADDR(ldc_X_V_B),
			PT_double, "compensator_x_setting_C[V]",PADDR(ldc_X_V_C),
			PT_set, "CT_phase",PADDR(CT_phase),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
			PT_set, "PT_phase",PADDR(PT_phase),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
			PT_double, "regulation",PADDR(regulation),
			PT_enumeration, "control_level",PADDR(control_level),
				PT_KEYWORD, "INDIVIDUAL", INDIVIDUAL,
				PT_KEYWORD, "BANK", BANK,
			PT_enumeration, "Control",PADDR(Control),
				PT_KEYWORD, "MANUAL", MANUAL,
				PT_KEYWORD, "OUTPUT_VOLTAGE", OUTPUT_VOLTAGE,
				PT_KEYWORD, "LINE_DROP_COMP", LINE_DROP_COMP,
				PT_KEYWORD, "REMOTE_NODE", REMOTE_NODE,
			PT_enumeration, "Type",PADDR(Type),
				PT_KEYWORD, "A", A,
				PT_KEYWORD, "B", B,
			PT_int16, "tap_pos_A",PADDR(tap_posA),
			PT_int16, "tap_pos_B",PADDR(tap_posB),
			PT_int16, "tap_pos_C",PADDR(tap_posC),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

int regulator_configuration::isa(char *classname)
{
	return strcmp(classname,"regulator_configuration")==0;
}

int regulator_configuration::create(void)
{
	int result = powerflow_library::create();
	band_center = 0.0;
	band_width = 0.0;
	time_delay = 0.0;
	dwell_time = 0.0;
	raise_taps = 0;
	lower_taps = 0;
	CT_ratio = 0;
	PT_ratio = 0;
	ldc_R_V[0] = ldc_R_V[1] = ldc_R_V[2] = 0.0;
	ldc_X_V[0] = ldc_X_V[1] = ldc_X_V[2] = 0.0;
	CT_phase = PHASE_ABC;
	PT_phase = PHASE_ABC;
	Control = MANUAL;
	control_level = INDIVIDUAL;
	Type = B;
	regulation = 0.0;

	//Set initial taps to some obscene value so we know if they've been initialized or not
	tap_pos[0] = tap_pos[1] = tap_pos[2] = 999;
	
	return result;
}
int regulator_configuration::init(OBJECT *parent)
{

	if (Control == LINE_DROP_COMP) 
	{
		if (PT_ratio == 0)
			GL_THROW("power_transducer_ratio must be set as a non-zero value when operating in LINE_DROP_COMP mode");
			/* TROUBLESHOOT
			PT_ratio must be set as a positive, non-zero value when LINE_DROP_COMP is selected to properly scale the 
			voltage drop along the line being compensated.
			*/
		if (solver_method != SM_FBS)
			GL_THROW("Line drop compensation is only supported for regulators in FBS at this time.");
			/* TROUBLESHOOT
			At this time, line drop compensation is not supported by NR or GS solver methods, only FBS.  Future
			releases should support this in NR.  Please change either your solver method or your control method.
			*/
	}

	if (control_level == BANK)
	{
		if (CT_phase == 1 || CT_phase == 2 || CT_phase == 4)
		{	// It's okay 
		}
		else 
			GL_THROW("There are too many CT_phases specified for a BANK control_level (or it wasn't specified).  Should only be one phase monitored.");
		if (PT_phase == 1 || PT_phase == 2 || PT_phase == 4)
		{	// It's okay 
		}
		else 
			GL_THROW("There are too many PT_phases specified for a BANK control_level (or it wasn't specified).  Should only be one phase monitored.");
	}
	if (raise_taps <= 0 || lower_taps <= 0)
		GL_THROW("raise and lower taps must be specified to non-zero numbers");
		/* TROUBLESHOOT
		The number of tap positions available in the regulator must be known to determine the limits of the regulator.
		Typically, these are specified as equal values (amount up is the same as amount down), and a common value is 16.
		Please specify non-zero, positive tap limits.
		*/

	if (Control != MANUAL)
	{
		if (band_width == 0)
			gl_warning("band_width is set to zero in automatic control. May cause oscillations.");
		if (regulation == 0)
			GL_THROW("regulation must be set to a non-zero number when operating in an automatic controlled mode.");
			/* TROUBLESHOOT
			When specifying an automatic regulator control method, a positive, non-zero regulation value must be specified
			to indicate the value of each tap change.  A typical configuration would include a regulation of 0.1, which
			leads to a tap change of 0.625% of the band_center voltage. 
			*/
		if (connect_type != WYE_WYE)
			GL_THROW("At this time, only WYE_WYE regulators are supported in automatic control modes.");
			/* TROUBLESHOOT
			Unfortunately, at this time, only Wye-Wye connect type transformers are supported in the automatic control
			mode (LINE_DROP_COMP, OUTPUT_VOLTAGE, or REMOTE_NODE). Future versions should support these objects, but
			until then, please use MANUAL Control with connect types other than Wye-Wye.
			*/
	}

	if (connect_type != WYE_WYE && solver_method != SM_FBS)
		GL_THROW("Only WYE_WYE regulator connections are fully supported in FBS & NR solvers at this time.");

	if (solver_method == SM_GS)
		GL_THROW("Regulators are not supported in the Gauss-Seidel solver method.");

	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: regulator_configuration
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_regulator_configuration(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(regulator_configuration::oclass);
		if (*obj!=NULL)
		{
			regulator_configuration *my = OBJECTDATA(*obj,regulator_configuration);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_regulator_configuration: %s", msg);
	}
	return 0;
}

EXPORT int init_regulator_configuration(OBJECT *obj)
{
	regulator_configuration *my = OBJECTDATA(obj,regulator_configuration);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (regulator_configuration:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_regulator_configuration(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}

EXPORT int isa_regulator_configuration(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,regulator_configuration)->isa(classname);
}

/**@}*/

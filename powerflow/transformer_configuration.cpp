/** $Id: transformer_configuration.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file transformer_configuration.cpp
	@addtogroup transformer_configuration Transformer Configuration
	@ingroup powerflow_transformer

	The transformer configuration specifies a type of transformer.  The 
	total rating of the transformer must be the sum of the individual
	phase ratings of the transformer.  The defaults are worked out as
	follows:

	- if the /e kVA_rating is omitted and some or all of the phase ratings are
	  provided, then the /e kVA_rating is the sum of the phase ratings.
    - if the /e kVA_rating is provided and all of the phase ratings are
	  omitted, then the a 3 phase transformer is assumed and the phase ratings
	  are equal to 1/3 of the /e kVA_rating
	
	For the forward/backsweep solver, the primary voltage must be greater than
	the secondary voltage.

 @{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "transformer.h"

//////////////////////////////////////////////////////////////////////////
// transformer_configuration CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* transformer_configuration::oclass = NULL;
CLASS* transformer_configuration::pclass = NULL;

transformer_configuration::transformer_configuration(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"transformer_configuration",sizeof(transformer_configuration),NULL);
		if (oclass==NULL)
			throw "unable to register class transformer_configuration";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_enumeration,"connect_type",PADDR(connect_type),PT_DESCRIPTION,"connect type enum: Wye-Wye, single-phase, etc.",
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"WYE_WYE",(enumeration)WYE_WYE,
				PT_KEYWORD,"DELTA_DELTA",(enumeration)DELTA_DELTA,
				PT_KEYWORD,"DELTA_GWYE",(enumeration)DELTA_GWYE,
				PT_KEYWORD,"SINGLE_PHASE",(enumeration)SINGLE_PHASE,
				PT_KEYWORD,"SINGLE_PHASE_CENTER_TAPPED",(enumeration)SINGLE_PHASE_CENTER_TAPPED,
			PT_enumeration,"install_type",PADDR(install_type),PT_DESCRIPTION,"Defines location of the transformer installation",
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"POLETOP",(enumeration)POLETOP,
				PT_KEYWORD,"PADMOUNT",(enumeration)PADMOUNT,
				PT_KEYWORD,"VAULT",(enumeration)VAULT,
			PT_enumeration,"coolant_type", PADDR(coolant_type),PT_DESCRIPTION,"coolant type, used in life time model",
				PT_KEYWORD,"UNKNOWN", UNKNOWN,
				PT_KEYWORD,"MINERAL_OIL", (enumeration)MINERAL_OIL,
				PT_KEYWORD,"DRY",(enumeration)DRY,
			PT_enumeration, "cooling_type", PADDR(cooling_type),PT_DESCRIPTION,"type of coolant fluid used in life time model",
				PT_KEYWORD,"UNKNOWN", UNKNOWN,
				PT_KEYWORD,"OA", (enumeration)OA,
				PT_KEYWORD,"FA", (enumeration)FA,
				PT_KEYWORD,"NDFOA", (enumeration)NDFOA,
				PT_KEYWORD,"NDFOW", (enumeration)NDFOW,
				PT_KEYWORD,"DFOA", (enumeration)DFOA,
				PT_KEYWORD,"DFOW", (enumeration)DFOW,
			
			PT_double, "primary_voltage[V]", PADDR(V_primary),PT_DESCRIPTION,"primary voltage level in L-L value kV",
			PT_double, "secondary_voltage[V]",PADDR(V_secondary),PT_DESCRIPTION,"secondary voltage level kV",
			PT_double, "power_rating[kVA]",PADDR(kVA_rating),PT_DESCRIPTION,"kVA rating of transformer, total",
			PT_double, "powerA_rating[kVA]",PADDR(phaseA_kVA_rating),PT_DESCRIPTION,"kVA rating of transformer, phase A",
			PT_double, "powerB_rating[kVA]",PADDR(phaseB_kVA_rating),PT_DESCRIPTION,"kVA rating of transformer, phase B",
			PT_double, "powerC_rating[kVA]",PADDR(phaseC_kVA_rating),PT_DESCRIPTION,"kVA rating of transformer, phase C",
			PT_double, "resistance[pu*Ohm]",PADDR(impedance.Re()),PT_DESCRIPTION,"Series impedance, pu, real",
			PT_double, "reactance[pu*Ohm]",PADDR(impedance.Im()),PT_DESCRIPTION,"Series impedance, pu, imag",
			PT_complex, "impedance[pu*Ohm]",PADDR(impedance),PT_DESCRIPTION,"Series impedance, pu",
			PT_double, "resistance1[pu*Ohm]",PADDR(impedance1.Re()),PT_DESCRIPTION,"Secondary series impedance (only used when you want to define each individual winding seperately, pu, real",
			PT_double, "reactance1[pu*Ohm]",PADDR(impedance1.Im()),PT_DESCRIPTION,"Secondary series impedance (only used when you want to define each individual winding seperately, pu, imag",
			PT_complex, "impedance1[pu*Ohm]",PADDR(impedance1),PT_DESCRIPTION,"Secondary series impedance (only used when you want to define each individual winding seperately, pu",
			PT_double, "resistance2[pu*Ohm]",PADDR(impedance2.Re()),PT_DESCRIPTION,"Secondary series impedance (only used when you want to define each individual winding seperately, pu, real",	
			PT_double, "reactance2[pu*Ohm]",PADDR(impedance2.Im()),PT_DESCRIPTION,"Secondary series impedance (only used when you want to define each individual winding seperately, pu, imag",	
			PT_complex, "impedance2[pu*Ohm]",PADDR(impedance2),PT_DESCRIPTION,"Secondary series impedance (only used when you want to define each individual winding seperately, pu",
			PT_double, "shunt_resistance[pu*Ohm]",PADDR(shunt_impedance.Re()),PT_DESCRIPTION,"Shunt impedance on primary side, pu, real",
			PT_double, "shunt_reactance[pu*Ohm]",PADDR(shunt_impedance.Im()),PT_DESCRIPTION,"Shunt impedance on primary side, pu, imag",
			PT_complex, "shunt_impedance[pu*Ohm]",PADDR(shunt_impedance),PT_DESCRIPTION,"Shunt impedance on primary side, pu",
			//thermal aging model parameters
			PT_double, "core_coil_weight[lb]", PADDR(core_coil_weight),PT_DESCRIPTION,"The weight of the core and coil assembly in pounds",
			PT_double, "tank_fittings_weight[lb]", PADDR(tank_fittings_weight),PT_DESCRIPTION,"The weight of the tank and fittings in pounds",
			PT_double, "oil_volume[gal]", PADDR(oil_vol),PT_DESCRIPTION,"The number of gallons of oil in the transformer",
			PT_double, "rated_winding_time_constant[h]", PADDR(t_W),PT_DESCRIPTION,"The rated winding time constant in hours",
			PT_double, "rated_winding_hot_spot_rise[degC]", PADDR(dtheta_H_AR),PT_DESCRIPTION,"winding hottest-spot rise over ambient temperature at rated load, degrees C",
			PT_double, "rated_top_oil_rise[degC]", PADDR(dtheta_TO_R),PT_DESCRIPTION,"top-oil hottest-spot rise over ambient temperature at rated load, degrees C",
			PT_double, "no_load_loss[pu]", PADDR(no_load_loss),PT_DESCRIPTION,"Another method of specifying transformer impedances, defined as per unit power values (shunt)",
			PT_double, "full_load_loss[pu]", PADDR(full_load_loss),PT_DESCRIPTION,"Another method of specifying transformer impedances, defined as per unit power values (shunt and series)",
			PT_double, "reactance_resistance_ratio", PADDR(RX),PT_DESCRIPTION,"the reactance to resistance ratio (X/R)",
			PT_double, "installed_insulation_life[h]", PADDR(installed_insulation_life),PT_DESCRIPTION,"the normal lifetime of the transformer insulation at rated load, hours",

			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int transformer_configuration::isa(char *classname)
{
	return strcmp(classname,"transformer_configuration")==0;
}

int transformer_configuration::create(void)
{
	int result = powerflow_library::create();
	memset(&connect_type,0,sizeof(connect_type));
	memset(&install_type,0,sizeof(install_type));
	V_primary = 0.0;
	V_secondary = 0.0;	
	phaseA_kVA_rating = 0.0;
	phaseB_kVA_rating = 0.0;
	phaseC_kVA_rating = 0.0;
	kVA_rating = 0;
	impedance = impedance1 = impedance2 = complex(0.0,0.0);	//Lossless transformer by default
	shunt_impedance = complex(999999999,999999999);			//Very large number for infinity to approximate lossless
	no_load_loss = full_load_loss = 0.0;
	RX = 4.5;
	return result;
}

int transformer_configuration::init(OBJECT *parent)
{

	OBJECT *obj = OBJECTHDR(this);

	// if aggregate rating is not set and 3 phase is specified
	if (kVA_rating==0)

		// aggregate rating is some of phase ratings
		kVA_rating = phaseA_kVA_rating+phaseB_kVA_rating+phaseC_kVA_rating;

	// if none of the phase ratings are specified, then it implicitly 3 phase
	if (phaseA_kVA_rating+phaseB_kVA_rating+phaseC_kVA_rating==0 && kVA_rating>0)
		phaseA_kVA_rating = phaseB_kVA_rating = phaseC_kVA_rating = kVA_rating/3;

	// check connection type
	if (connect_type==UNKNOWN)
		GL_THROW("connection type not specified");
		/*  TROUBLESHOOT
		You must specify what type of transformer this is (i.e., WYE_WYE, DELTA_GWYE, etc.).  Please choose
		a supported connect_type.
		*/
	
	// check installation type
	if (install_type==UNKNOWN)
		gl_verbose("installation type not specified");
		/*  TROUBLESHOOT
		The type of installation (i.e., pad mounted versus pole top) was not specified. In most cases, this
		message can be ignored as it does not effect the solution.  However, if using the transformer aging 
		model, future implementation may need install_type to be specified.
		*/
	
	// check primary and second voltages
	if (V_primary==0)
		GL_THROW("V_primary must be positive");
		/*  TROUBLESHOOT
		For the purposes of specifying the equipment, V can only be a positive number. This helps define the turns ratio. 
		Please specify V_primary as a positive number.
		*/
	if (V_secondary==0)
		GL_THROW("V_secondary must be positive");
		/*  TROUBLESHOOT
		For the purposes of specifying the equipment, V can only be a positive number. This helps define the turns ratio. 
		Please specify V_secondary as a positive number.
		*/

	// check kVA rating
	if (kVA_rating<=0)
		GL_THROW("kVA_rating(s) must be positive");
		/*  TROUBLESHOOT
		By definition, kVA can only be a positive (or zero) number. But, for the sake of actual equipment,
		we'll assume this can only be a positive number.  Please specify kVA_rating as a positive number.
		*/
	if (fabs((kVA_rating-phaseA_kVA_rating-phaseB_kVA_rating-phaseC_kVA_rating)/kVA_rating)>0.01)
		GL_THROW("kVA rating mismatch across phases exceeds 1%");
		/*  TROUBLESHOOT
		Both the total kVA rating and the individual kVA phase ratings were set.  However, they differed by
		more than 1%, leaving the model in a state of confusion.  Please check your kVA ratings and either
		specify only the total rating (will evenly split rating between phases) or a by-phase rating.
		*/

	// check connection type and see if it support shunt or additional series impedance values
	if (connect_type!=SINGLE_PHASE_CENTER_TAPPED)
	{
		if ((impedance1.Re() != 0.0 && impedance1.Im() != 0.0) || (impedance2.Re() != 0.0 && impedance2.Im() != 0.0))
			gl_warning("This connection type on transformer:%d (%s) does not support impedance (impedance1 or impedance2) of secondaries at this time.",obj->id,obj->name);
			/*  TROUBLESHOOT
			At this time impedance1 and impedance2 are only defined and used for center-tap transformers to be
			explicitly defined.  These values will be ignored for now.
			*/
		if (connect_type!=WYE_WYE)
		{
			if (shunt_impedance.Re() != 999999999 || shunt_impedance.Im() != 999999999)
				gl_warning("This connection type on transformer:%d (%s) does not support shunt_impedance at this time.",obj->id,obj->name);
				/* TROUBLESHOOT
				At this time shunt_impedance is only defined and supported in center-tap and WYE-WYE transformers.  Theses values
				will be ignored for now.
				*/
		}
		if (no_load_loss != 0.0 || full_load_loss != 0.0)
			gl_warning("This connection type on transformer:%d (%s) does not support shunt_impedance at this time.",obj->id,obj->name);
			/* TROUBLESHOOT
			At this time no_load_loss and full_load_loss are only defined and supported in 
			center-tap transformers.  Theses values	will be ignored for now.
			*/
	}
	else
	{
		if (no_load_loss > 0 && full_load_loss > 0)// Using Jason's equations to determine shunt and series resistances based on no-load and full-load losses
		// determine shunt and series resistances based on no-load and full-load losses
		{
			if (RX == 4.5 && kVA_rating > 500)
				gl_warning("transormer_configuration:%d (%s) reactance_resistance_ratio was not set and defaulted to 4.5. This may cause issues with larger transformers (>0.5 MVA)",obj->id,obj->name);
				/* TROUBLESHOOT
				X/R ratios are highly dependent on the size of the transformer.  As the transformer rating goes up
				so does the X/R ratio.  For small residential transformers (<500 kVA), the values range from about 2-5 as good
				estimates. For a good X/R reference, recommend GE's GET-3550F document, Appendix Part II.
				*/
			impedance = complex(full_load_loss,RX*full_load_loss);
			shunt_impedance = complex(1/no_load_loss,RX/no_load_loss);
		}
		if ((impedance1.Re() == 0.0 && impedance1.Im() == 0.0) && (impedance2.Re() != 0.0 && impedance2.Im() != 0.0))
		{
			impedance1 = impedance2;
			gl_warning("impedance2 was defined, but impedance1 was not -- assuming they are equal");
			/* TROUBLESHOOT
			One of the secondary side impedances (impedance1 or impedance2, or resistance1 and reactance1) was set to a 
			non-zero value, indicating you wanted to use the secondary impedances, however, the other value was not also set.
			If you do not want to use the assumption that they are the same, please set the other impedance value.
			*/
		}
		else if ((impedance1.Re() != 0.0 && impedance1.Im() != 0.0) && (impedance2.Re() == 0.0 && impedance2.Im() == 0.0))
		{
			impedance2 = impedance1;
			gl_warning("impedance1 was defined, but impedance2 was not -- assuming they are equal");
			/* TROUBLESHOOT
			One of the secondary side impedances (impedance1 or impedance2, or resistance1 and reactance1) was set to a 
			non-zero value, indicating you wanted to use the secondary impedances, however, the other value was not also set.
			If you do not want to use the assumption that they are the same, please set the other impedance value.
			*/
		}
		
	}
	// check impedance
	if (impedance.Re()<=0 || impedance1.Re()<0 || impedance2.Re()<0)
		GL_THROW("resistance must be non-negative");
		/* TROUBLESHOOT
		Please specify either impedance (real portion) or resistance as a positive value.
		*/
	if (impedance.Im()<=0 || impedance1.Im()<0 || impedance2.Im()<0)
		GL_THROW("reactance must be non-negative");
		/* TROUBLESHOOT
		Please specify either impedance (imaginary portion)	or reactance as a positive value.
		*/
	if (shunt_impedance.Re()<0)
		GL_THROW("shunt_resistance must be non-negative");
		/* TROUBLESHOOT
		Please specify either shunt impedance (imaginary portion) or shunt resistance as a positive value.
		*/
	if (shunt_impedance.Im()<0)
		GL_THROW("shunt_reactance must be non-negative");
		/* TROUBLESHOOT
		Please specify either shunt impedance (imaginary portion) or shunt reactance as a positive value.
		*/
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: transformer_configuration
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_transformer_configuration(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(transformer_configuration::oclass);
		if (*obj!=NULL)
		{
			transformer_configuration *my = OBJECTDATA(*obj,transformer_configuration);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(transformer_configuration);
}

EXPORT int init_transformer_configuration(OBJECT *obj, OBJECT *parent)
{
	try {
		return OBJECTDATA(obj,transformer_configuration)->init(parent);
	}
	INIT_CATCHALL(transformer_configuration);
}

EXPORT TIMESTAMP sync_transformer_configuration(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}

EXPORT int isa_transformer_configuration(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,transformer_configuration)->isa(classname);
}

/**@}**/

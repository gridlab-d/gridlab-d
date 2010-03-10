/** $Id: transformer_configuration.cpp 1182 2008-12-22 22:08:36Z dchassin $
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
		oclass = gl_register_class(mod,"transformer_configuration",sizeof(transformer_configuration),PC_BOTTOMUP|PC_POSTTOPDOWN);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		
		if(gl_publish_variable(oclass,
			PT_enumeration,"connect_type",PADDR(connect_type),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"WYE_WYE",WYE_WYE,
				PT_KEYWORD,"DELTA_DELTA",DELTA_DELTA,
				PT_KEYWORD,"DELTA_GWYE",DELTA_GWYE,
				PT_KEYWORD,"SINGLE_PHASE",SINGLE_PHASE,
				PT_KEYWORD,"SINGLE_PHASE_CENTER_TAPPED",SINGLE_PHASE_CENTER_TAPPED,
			PT_enumeration,"install_type",PADDR(install_type),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"POLETOP",POLETOP,
				PT_KEYWORD,"PADMOUNT",PADMOUNT,
				PT_KEYWORD,"VAULT",VAULT,
			
			PT_double, "primary_voltage[V]", PADDR(V_primary),
			PT_double, "secondary_voltage[V]",PADDR(V_secondary),
			PT_double, "power_rating[kVA]",PADDR(kVA_rating),
			PT_double, "powerA_rating[kVA]",PADDR(phaseA_kVA_rating),
			PT_double, "powerB_rating[kVA]",PADDR(phaseB_kVA_rating),
			PT_double, "powerC_rating[kVA]",PADDR(phaseC_kVA_rating),
			PT_double, "resistance[pu.Ohm]",PADDR(impedance.Re()),	// was R_pu
			PT_double, "reactance[pu.Ohm]",PADDR(impedance.Im()),	// was X_pu
			PT_complex, "impedance[pu.Ohm]",PADDR(impedance),
			PT_double, "resistance1[pu.Ohm]",PADDR(impedance1.Re()),	
			PT_double, "reactance1[pu.Ohm]",PADDR(impedance1.Im()),	
			PT_complex, "impedance1[pu.Ohm]",PADDR(impedance1),
			PT_double, "resistance2[pu.Ohm]",PADDR(impedance2.Re()),	
			PT_double, "reactance2[pu.Ohm]",PADDR(impedance2.Im()),	
			PT_complex, "impedance2[pu.Ohm]",PADDR(impedance2),
			PT_double, "shunt_resistance[pu.Ohm]",PADDR(shunt_impedance.Re()),
			PT_double, "shunt_reactance[pu.Ohm]",PADDR(shunt_impedance.Im()),
			PT_complex, "shunt_impedance[pu.Ohm]",PADDR(shunt_impedance),

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
		throw "connection type not specified";
	
	// check installation type
	if (install_type==UNKNOWN)
		gl_warning("installation type not specified");
	
	// check primary and second voltages
	if (V_secondary==0)
		throw "V_secondary must be positive";
	// it is not true for step-up transformer
	//if (V_primary<=V_secondary)
	//	throw "V_primary must be greater than V_secondary";

	// check kVA rating
	if (kVA_rating<=0)
		throw "kVA_rating(s) must be positive";
	if (fabs((kVA_rating-phaseA_kVA_rating-phaseB_kVA_rating-phaseC_kVA_rating)/kVA_rating)>0.01)
		throw "kVA rating mismatch across phases exceeds 1%";

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
		if ((impedance1.Re() == 0.0 && impedance1.Im() == 0.0) && (impedance2.Re() != 0.0 && impedance2.Im() != 0.0))
		{
			impedance1 = impedance2;
			gl_warning("impedance2 was defined, but impedance1 was not -- assuming they are equal");
		}
		else if ((impedance1.Re() != 0.0 && impedance1.Im() != 0.0) && (impedance2.Re() == 0.0 && impedance2.Im() == 0.0))
		{
			impedance2 = impedance1;
			gl_warning("impedance1 was defined, but impedance2 was not -- assuming they are equal");
		}
		
	}
	// check impedance
	if (impedance.Re()<0 || impedance1.Re()<0 || impedance2.Re()<0)
		throw "resistance must be non-negative";
	if (impedance.Im()<0 || impedance1.Im()<0 || impedance2.Im()<0)
		throw "reactance must be non-negative";
	if (shunt_impedance.Re()<0)
		throw "shunt_resistance must be non-negative";
	if (shunt_impedance.Im()<0)
		throw "shunt_reactance must be non-negative";
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
	}
	catch (const char *msg)
	{
		gl_error("%s %s (id=%d): %s", (*obj)->name?(*obj)->name:"unnamed", (*obj)->oclass->name, (*obj)->id, msg);
		return 0;
	}
	return 1;
}

EXPORT int init_transformer_configuration(OBJECT *obj, OBJECT *parent)
{
	try {
			return OBJECTDATA(obj,transformer_configuration)->init(parent);
	}
	catch (const char *msg)
	{
		gl_error("%s %s (id=%d): %s", obj->name?obj->name:"unnamed", obj->oclass->name, obj->id, msg);
		return 0;
	}
	return 1;
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

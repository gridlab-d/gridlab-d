/** $Id: substation.cpp
	Copyright (C) 2009 Battelle Memorial Institute
	@file substation.cpp
	@addtogroup powerflow_meter Substation
	@ingroup powerflow

	Substation object serves as a connecting object between the powerflow and
	network solvers.  The three-phase unbalanced connections of the distribution
	solver are posted as an equivalent per-unit load on the network solver.  The
	network solver per-unit voltage is translated into a balanced three-phase voltage
	for the distribution solver.  This functionality assumes the substation is treated
	as the swing bus for the distribution solver.

	Total cumulative energy, instantantenous power and peak demand are metered.

	@{
 **/
//Built from meter object
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "substation.h"
#include "timestamp.h"

//////////////////////////////////////////////////////////////////////////
// SUBSTATION CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
// class management data
CLASS* substation::oclass = NULL;
CLASS* substation::pclass = NULL;

// the constructor registers the class and properties and sets the defaults
substation::substation(MODULE *mod) : node(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// link to parent class (used by isa)
		pclass = node::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"substation",sizeof(substation),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class substation";
		else
			oclass->trl = TRL_PROTOTYPE;

		// publish the class properties - based around meter object
		if (gl_publish_variable(oclass,
			PT_INHERIT, "node",
			//inputs
			PT_complex, "zero_sequence_voltage[V]", PADDR(seq_mat[0]), PT_DESCRIPTION, "The zero sequence representation of the voltage for the substation object.",
			PT_complex, "positive_sequence_voltage[V]", PADDR(seq_mat[1]), PT_DESCRIPTION, "The positive sequence representation of the voltage for the substation object.",
			PT_complex, "negative_sequence_voltage[V]", PADDR(seq_mat[2]), PT_DESCRIPTION, "The negative sequence representation of the voltage for the substation object.",
			PT_double, "base_power[VA]", PADDR(base_power), PT_DESCRIPTION, "The 3 phase VA power rating of the substation.",
			PT_double, "power_convergence_value[VA]", PADDR(power_convergence_value), PT_DESCRIPTION, "Default convergence criterion before power is posted to pw_load objects if connected, otherwise ignored",
			PT_enumeration, "reference_phase", PADDR(reference_phase), PT_DESCRIPTION, "The reference phase for the positive sequence voltage.",
				PT_KEYWORD, "PHASE_A", (enumeration)R_PHASE_A,
				PT_KEYWORD, "PHASE_B", (enumeration)R_PHASE_B,
				PT_KEYWORD, "PHASE_C", (enumeration)R_PHASE_C,
			PT_complex, "transmission_level_constant_power_load[VA]", PADDR(average_transmission_power_load), PT_DESCRIPTION, "the average constant power load to be posted directly to the pw_load object.",
			PT_complex, "transmission_level_constant_current_load[A]", PADDR(average_transmission_current_load), PT_DESCRIPTION, "the average constant current load at nominal voltage to be posted directly to the pw_load object.",
			PT_complex, "transmission_level_constant_impedance_load[Ohm]", PADDR(average_transmission_impedance_load), PT_DESCRIPTION, "the average constant impedance load at nominal voltage to be posted directly to the pw_load object.",
			PT_complex, "average_distribution_load[VA]", PADDR(average_distribution_load), PT_DESCRIPTION, "The average of the loads on all three phases at the substation object.",
			PT_complex, "distribution_power_A[VA]", PADDR(distribution_power_A),
			PT_complex, "distribution_power_B[VA]", PADDR(distribution_power_B),
			PT_complex, "distribution_power_C[VA]", PADDR(distribution_power_C),
			PT_complex, "distribution_voltage_A[V]", PADDR(voltageA),
			PT_complex, "distribution_voltage_B[V]", PADDR(voltageB),
			PT_complex, "distribution_voltage_C[V]", PADDR(voltageC),
			PT_complex, "distribution_voltage_AB[V]", PADDR(voltageAB),
			PT_complex, "distribution_voltage_BC[V]", PADDR(voltageBC),
			PT_complex, "distribution_voltage_CA[V]", PADDR(voltageCA),
			PT_complex, "distribution_current_A[A]", PADDR(current_inj[0]),
			PT_complex, "distribution_current_B[A]", PADDR(current_inj[1]),
			PT_complex, "distribution_current_C[A]", PADDR(current_inj[2]),
			PT_double, "distribution_real_energy[Wh]", PADDR(distribution_real_energy),
			//PT_double, "measured_reactive[kVar]", PADDR(measured_reactive), has not implemented yet
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		}
}

int substation::isa(char *classname)
{
	return strcmp(classname,"substation")==0 || node::isa(classname);
}

// Create a distribution meter from the defaults template, return 1 on success
int substation::create()
{
	int result = node::create();
	reference_phase = R_PHASE_A;
	has_parent = 0;
	seq_mat[0] = 0;
	seq_mat[1] = 0;
	seq_mat[2] = 0;
	volt_A = 0;
	volt_B = 0;
	volt_C = 0;
	base_power = 0;
	power_convergence_value = 0.0;
	pTransNominalVoltage = NULL;
	return result;
}

void substation::fetch_complex(complex **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_complex_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "substation:%i", hdr->id);
		if(*name == NULL)
			sprintf(msg, "%s: substation unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: substation unable to find %s", namestr, name);
		throw(msg);
	}
}

void substation::fetch_double(double **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "substation:%i", hdr->id);
		if(*name == NULL)
			sprintf(msg, "%s: substation unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: substation unable to find %s", namestr, name);
		throw(msg);
	}
}

// Initialize a distribution meter, return 1 on success
int substation::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	int i,n;

	//Base check higher so can be used below
	if(base_power <= 0){
		gl_warning("substation:%i is using the default base power of 100 VA. This could cause instability on your system.", hdr->id);
		base_power = 100;//default gives a max power error of 1 VA.
	}

	//Check to see if it has a parent (no sense to ISAs if it is empty)
	if (parent != NULL)
	{
		if (gl_object_isa(parent,"pw_load","network"))
		{
			//Make sure it is done, otherwise defer
			if((parent->flags & OF_INIT) != OF_INIT){
				char objname[256];
				gl_verbose("substation::init(): deferring initialization on %s", gl_name(parent, objname, 255));

				return 2; // defer
			}

			//Map up the appropriate variables- error checks mostly inside
			fetch_complex(&pPositiveSequenceVoltage,"load_voltage",parent);
			fetch_complex(&pConstantPowerLoad,"load_power",parent);
			fetch_complex(&pConstantCurrentLoad,"load_current",parent);
			fetch_complex(&pConstantImpedanceLoad,"load_impedance",parent);
			fetch_double(&pTransNominalVoltage,"bus_nom_volt",parent);

			//Do a general check for nominal voltages - make sure they match
			if (fabs(*pTransNominalVoltage-nominal_voltage)>0.001)
			{
				gl_error("Nominal voltages of tranmission node (%.1f V) and substation (%.1f) do not match!",*pTransNominalVoltage,nominal_voltage);
				/*  TROUBLESHOOT
				The nominal voltage of the transmission node in PowerWorld does not match
				that of the value inside GridLAB-D's substation's nominal_voltage.  This could
				cause information mismatch and is therefore not allowed.  Please set the
				substation to the same nominal voltage as the transmission node.  Use transformers
				to step the voltage down to an appropriate distribution or sub-transmission level.
				*/
				return 0;	//Fail
			}

			//Check our bustype - otherwise we may get overridden (NR-esque check)
			if (bustype != SWING)
			{
				gl_warning("substation attached to pw_load and not a SWING bus - forcing to SWING");
				/*  TROUBLESHOOT
				When a substation object is connected to PowerWorld via a pw_load object, the
				substation must be designated as a SWING bus.  This designation will now be forced upon
				the bus.
				*/
				bustype = SWING;
			}//End bus check

			//Flag us as pw_load connected
			has_parent = 1;

			//Check convergence-posting criterion
			if (power_convergence_value<=0.0)
			{
				gl_warning("power_convergence_value not set - defaulting to 0.01 base_power");
				/*  TROUBLESHOOT
				A value was not specified for the convergence criterion required before posting an 
				answer up to pw_load.  This value has defaulted to 1% of base_power.  If a different threshold
				is desired, set it explicitly.
				*/

				power_convergence_value = 0.01*base_power;
			}//End convergence value check
		}
		else	//Parent isn't a pw_load, so we just become a normal node - let it handle things
		{
			has_parent = 2;	//Flag as "normal" - let node checks sort out if we are really valid or not
		}
	}//End parent
	else	//Parent is null, see what mode we're in
	{
		//Check for sequence voltages - if not set, we're normal (let node checks handle if we're valid)
		if ((seq_mat[0] != 0.0) || (seq_mat[1] != 0.0) || (seq_mat[2] != 0.0))
		{
			//See if we're a swing, if not, this is meaningless
			if (bustype != SWING)
			{
				gl_warning("substation is not a SWING bus, so answers may be unexpected");
				/*  TROUBLESHOOT
				A substation object appears set to accept sequence voltage values, but it is not a SWING bus.  This
				may end up causing the voltages to be converted from sequence, but then overridden by the distribution
				powerflow solver.
				*/
			}

			//Explicitly set
			has_parent = 0;
		}	
		else	//Else, nothing set, we're a normal old node
		{
			has_parent = 2;	//Normal node

			//Warn that nothing was found
			gl_warning("substation:%s is set up as a normal node, no sequence values will be calculated",hdr->name);
			/*  TROUBLESHOOT
			A substation is currently behaving just like a normal powerflow node.  If it was desired that it convert a 
			schedule or player of sequence values, please initialize those values to non-zero along with the player attachment.
			*/
		}
	}//End no parent

	//Set up reference items if they are needed
	if (has_parent != 2)	//Not a normal node
	{
		//set the reference phase number to shift the phase voltages appropriatly with the positive sequence voltage
		if(reference_phase == R_PHASE_A){
			reference_number.SetPolar(1,0);
		} else if(reference_phase == R_PHASE_B){
			reference_number.SetPolar(1,2*PI/3);
		} else if(reference_phase == R_PHASE_C){
			reference_number.SetPolar(1,-2*PI/3);
		}

		//create the sequence to phase transformation matrix
		for(i=0; i<3; i++){
			for(n=0; n<3; n++){
				if((i==1 && n==1) || (i==2 && n==2)){
					transformation_matrix[i][n].SetPolar(1,-2*PI/3);
				} else if((i==2 && n==1) || (i==1 && n==2)){
					transformation_matrix[i][n].SetPolar(1,2*PI/3);
				} else {
					transformation_matrix[i][n].SetPolar(1,0);
				}
			}
		}

		//New requirement to maintain positive sequence ability - three phases must be had, unless
		//we're just a normal node.  Then, we don't care.
		if (!has_phase(PHASE_A|PHASE_B|PHASE_C))
		{
			gl_error("substation needs have all three phases!");
			/*  TROUBLESHOOT
			To meet the requirements for sequence voltage conversions, the substation node must have all three
			phases at the connection point.  If only a single phase or subset of full three phase is needed, those
			must be set in the distribution network, typically after a delta-ground wye transformer.
			*/
			return 0;
		}

		//TODO: Implement a check to make sure nominals match!

		//Not sure if the below is needed or not anymore
		if(has_phase(PHASE_A)){
			volt_A.SetPolar(nominal_voltage,0);
		}
		if(has_phase(PHASE_B)){
			volt_B.SetPolar(nominal_voltage,-PI*2/3);
		}
		if(has_phase(PHASE_C)){
			volt_C.SetPolar(nominal_voltage,PI*2/3);
		}
	}//End not a normal node
	
	return node::init(parent);
}

// Synchronize a distribution meter
TIMESTAMP substation::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	double dt = 0;
	double total_load;
	//calculate the energy used
	if(t1 != t0 && t0 != 0 && (last_power_A.Re() != 0 && last_power_B.Re() != 0 && last_power_C.Re() != 0)){
		total_load = last_power_A.Re() + last_power_B.Re() + last_power_C.Re();
		dt = ((double)(t1 - t0))/(3600 * TS_SECOND);
		distribution_real_energy += total_load*dt;
	}
	return node::presync(t1);
}
TIMESTAMP substation::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2;
	double dist_power_A_diff = 0;
	double dist_power_B_diff = 0;
	double dist_power_C_diff = 0;
	//set up the phase voltages for the three different cases
	if(has_parent == 1){//has a pw_load as a parent
		seq_mat[1] = *pPositiveSequenceVoltage;
		seq_mat[0] = 0.0;	//Force the other two to zero for now - just to make sure
		seq_mat[2] = 0.0;
	}

	//Check to see if a sequence conversion needs to be done
	if (has_parent != 2)
	{
		voltageA = (seq_mat[0] + seq_mat[1] + seq_mat[2]) * reference_number;
		voltageB = (seq_mat[0] + transformation_matrix[1][1] * seq_mat[1] + transformation_matrix[1][2] * seq_mat[2]) * reference_number;
		voltageC = (seq_mat[0] + transformation_matrix[2][1] * seq_mat[1] + transformation_matrix[2][2] * seq_mat[2]) * reference_number;
	}

	t2 = node::sync(t1);
	
	// update currents if in NR
	if(solver_method == SM_NR){
		int result = node::NR_current_update(true,false);

		if(result != 1){
			GL_THROW("Attempt to update current/power on substation:%s failed!",obj->name);
			//Defined elsewhere
		}
	}

	if((solver_method == SM_NR && NR_admit_change == false) || solver_method == SM_FBS){
		distribution_power_A = voltageA * (~current_inj[0]);
		distribution_power_B = voltageB * (~current_inj[1]);
		distribution_power_C = voltageC * (~current_inj[2]);
		dist_power_A_diff = distribution_power_A.Mag() - last_power_A.Mag();
		dist_power_B_diff = distribution_power_B.Mag() - last_power_B.Mag();
		dist_power_C_diff = distribution_power_C.Mag() - last_power_C.Mag();
		last_power_A = distribution_power_A;
		last_power_B = distribution_power_B;
		last_power_C = distribution_power_C;

		//Convergence check - only if pw_load connected
		if (has_parent == 1)
		{
			if((fabs(dist_power_A_diff) + fabs(dist_power_B_diff) + fabs(dist_power_C_diff)) <= power_convergence_value)
			{
				//Built on three-phase assumption, otherwise sequence components method falls apart - convert these to all use nominal!
				*pConstantCurrentLoad = ((~average_transmission_current_load) * (*pTransNominalVoltage)) / 1000000;
				if(average_transmission_impedance_load.Mag() > 0){
					*pConstantImpedanceLoad = (complex((*pTransNominalVoltage * (*pTransNominalVoltage))) / (~average_transmission_impedance_load) / 1000000);
				} else {
					*pConstantImpedanceLoad = 0;
				}
				*pConstantPowerLoad = (average_transmission_power_load + (distribution_power_A + distribution_power_B + distribution_power_C)) / 1000000;
			}
		}//End is pw_load connected
	}//End powerflow cycling check
	return t2;
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int isa_substation(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,substation)->isa(classname);
}

EXPORT int create_substation(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(substation::oclass);
		if (*obj!=NULL)
		{
			substation *my = OBJECTDATA(*obj,substation);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(substation);
}

EXPORT int init_substation(OBJECT *obj)
{
	try {
		substation *my = OBJECTDATA(obj,substation);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(substation);
}

EXPORT TIMESTAMP sync_substation(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		substation *pObj = OBJECTDATA(obj,substation);
		TIMESTAMP t1;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(obj->clock,t0);
		case PC_BOTTOMUP:
			return pObj->sync(obj->clock,t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
		throw "invalid pass request";
	} 
	SYNC_CATCHALL(substation);
}

EXPORT int notify_substation(OBJECT *obj, int update_mode, PROPERTY *prop, char *value){
	substation *n = OBJECTDATA(obj, substation);
	int rv = 1;

	rv = n->notify(update_mode, prop, value);

	return rv;
}

/**@}**/

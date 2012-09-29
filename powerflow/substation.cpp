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
			PT_enumeration, "reference_phase", PADDR(reference_phase), PT_DESCRIPTION, "The reference phase for the positive sequence voltage.",
				PT_KEYWORD, "PHASE_A", R_PHASE_A,
				PT_KEYWORD, "PHASE_B", R_PHASE_B,
				PT_KEYWORD, "PHASE_C", R_PHASE_C,
			PT_complex, "average_transmission_power_load[VA]", PADDR(average_transmission_power_load), PT_DESCRIPTION, "the average constant power load to be posted directly to the pw_load object.",
			PT_complex, "average_transmission_current_load[A]", PADDR(average_transmission_current_load), PT_DESCRIPTION, "the average constant current load to be posted directly to the pw_load object.",
			PT_complex, "average_transmission_impedance_load[Ohm]", PADDR(average_transmission_impedance_load), PT_DESCRIPTION, "the average constant impedance load to be posted directly to the pw_load object.",
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

// Initialize a distribution meter, return 1 on success
int substation::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	int i,n;
	
	//make sure the substation is the swing bus if it's parent is a pw_load object
	if(parent != NULL && gl_object_isa(parent,"pw_load")){
		if(bustype != SWING){
			gl_error("Substation connected to pw_load must be the swing bus.");
			return 0;
		}
		
		has_parent = 1;

		//create the pointers to the needed properties in pw_load
		fetch_complex(&pPositiveSequenceVoltage,"load_voltage",parent);
		fetch_complex(&pConstantPowerLoad,"load_power",parent);
		fetch_complex(&pConstantCurrentLoad,"load_current",parent);
		fetch_complex(&pConstantImpedanceLoad,"load_impedance",parent);
	} else if(parent != NULL && (seq_mat[0] != 0 || seq_mat[1] != 0 || seq_mat[2] != 0)){
		gl_warning("substation:%i is not the swing bus. Ignoring sequence voltage inputs.", hdr->id); 
	} else if(parent != NULL){
		has_parent = 2;
	} else {
		if(bustype != SWING){
			gl_error("Substation connected to pw_load must be the swing bus.");
			return 0;
		}
	}
	if(base_power <= 0){
		gl_warning("substation:%i is using the default base power of 100 VA. This could cause instability on your system.", hdr->id);
		base_power = 100;//default gives a max power error of 1 VA.
	}
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

	if(has_phase(PHASE_A)){
		volt_A.SetPolar(nominal_voltage,0);
	}
	if(has_phase(PHASE_B)){
		volt_B.SetPolar(nominal_voltage,-PI*2/3);
	}
	if(has_phase(PHASE_C)){
		volt_C.SetPolar(nominal_voltage,PI*2/3);
	}
	
	return node::init(parent);
}

// Synchronize a distribution meter
TIMESTAMP substation::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	double dt = 0;
	double total_load;
	//calculate the energy used
	if(t1 != t0 && t0 != 0 && (last_power_A.Re() != 0 && last_power_B.Re() != 0 && last_power_C.Re() != 0) && NR_cycle == TRUE){
		total_load = last_power_A.Re() + last_power_B.Re() + last_power_C.Re();
		dt = ((double)(t1 - t0))/(3600 * TS_SECOND);
		distribution_real_energy += total_load*dt;
	}
	return node::presync(t1);
}
TIMESTAMP substation::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2;
	double dist_power_A_diff = 0;
	double dist_power_B_diff = 0;
	double dist_power_C_diff = 0;
	//set up the phase voltages for the three different cases
	if((solver_method == SM_NR && NR_cycle == false) || solver_method == SM_FBS){
		if(has_parent == 1){//has a pw_load as a parent
			seq_mat[1] = *pPositiveSequenceVoltage;
			if(has_phase(PHASE_A)){
				voltageA = seq_mat[1] * reference_number;
			}
			if(has_phase(PHASE_B)){
				voltageB = (transformation_matrix[1][1] * seq_mat[1]) * reference_number;
			}
			if(has_phase(PHASE_C)){
				voltageC = (transformation_matrix[2][1] * seq_mat[1]) *reference_number;
			}
		} else {
			if(has_parent == 0){
				if(seq_mat[0] != 0 || seq_mat[1] != 0 || seq_mat[2] != 0){
					voltageA = (seq_mat[0] + seq_mat[1] + seq_mat[2]) * reference_number;
					voltageB = (seq_mat[0] + transformation_matrix[1][1] * seq_mat[1] + transformation_matrix[1][2] * seq_mat[2]) * reference_number;
					voltageC = (seq_mat[0] + transformation_matrix[2][1] * seq_mat[1] + transformation_matrix[2][2] * seq_mat[2]) * reference_number;
				}
			}
		}
	}
	t2 = node::sync(t1);
	if((solver_method == SM_NR && NR_cycle == true && NR_admit_change == false) || solver_method == SM_FBS){
		distribution_power_A = voltageA * (~current_inj[0]);
		distribution_power_B = voltageB * (~current_inj[1]);
		distribution_power_C = voltageC * (~current_inj[2]);
		dist_power_A_diff = distribution_power_A.Mag() - last_power_A.Mag();
		dist_power_B_diff = distribution_power_B.Mag() - last_power_B.Mag();
		dist_power_C_diff = distribution_power_C.Mag() - last_power_C.Mag();
		last_power_A = distribution_power_A;
		last_power_B = distribution_power_B;
		last_power_C = distribution_power_C;
		if((abs(dist_power_A_diff) + abs(dist_power_B_diff) + abs(dist_power_C_diff)) <= (base_power*0.01)){
			if(has_parent == 1){
				*pConstantCurrentLoad = ((volt_A + volt_B + volt_C) * (~average_transmission_current_load)) / (3 * 1000000);
				if(average_transmission_impedance_load.Mag() > 0){
					*pConstantImpedanceLoad = (((volt_A * volt_A) + (volt_B * volt_B) + (volt_C * volt_C)) / (average_transmission_impedance_load)) / ( 3 * 1000000);
				} else {
					*pConstantImpedanceLoad = 0;
				}
				*pConstantPowerLoad = (average_transmission_power_load + ((distribution_power_A + distribution_power_B + distribution_power_C) / 3)) / 1000000;
			}
		}
	}
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

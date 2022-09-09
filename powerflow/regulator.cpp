// $Id: regulator.cpp 1186 2009-01-02 18:15:30Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file regulator.cpp
	@addtogroup powerflow_regulator Regulator
	@ingroup powerflow
		
	@{
*/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
using namespace std;

#include "regulator.h"

CLASS* regulator::oclass = nullptr;
CLASS* regulator::pclass = nullptr;

regulator::regulator(MODULE *mod) : link_object(mod)
{
	if (oclass==nullptr)
	{
		// link to parent class (used by isa)
		pclass = link_object::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"regulator",sizeof(regulator),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class regulator";
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_object,"configuration",PADDR(configuration),PT_DESCRIPTION,"reference to the regulator_configuration object used to determine regulator properties",
			PT_int16, "tap_A",PADDR(tap[0]),PT_DESCRIPTION,"current tap position of tap A",
			PT_int16, "tap_B",PADDR(tap[1]),PT_DESCRIPTION,"current tap position of tap B",
			PT_int16, "tap_C",PADDR(tap[2]),PT_DESCRIPTION,"current tap position of tap C",
			PT_enumeration, "msg_mode", PADDR(msgmode),PT_DESCRIPTION,"messages regarding remote node voltage to come internally from gridlabd or externally through co-simulation. Set to EXTERNAL only if you have co-simulation enabled",
				PT_KEYWORD, "INTERNAL", (enumeration)msg_INTERNAL,
				PT_KEYWORD, "EXTERNAL", (enumeration)msg_EXTERNAL, 
			PT_complex, "remote_voltage_A[V]", PADDR(check_voltage[0]),PT_DESCRIPTION,"remote node voltage, Phase A to ground",
            PT_complex, "remote_voltage_B[V]", PADDR(check_voltage[1]),PT_DESCRIPTION,"remote node voltage, Phase B to ground",
            PT_complex, "remote_voltage_C[V]", PADDR(check_voltage[2]),PT_DESCRIPTION,"remote node voltage, Phase C to ground",
			PT_double, "tap_A_change_count",PADDR(tap_A_change_count),PT_DESCRIPTION,"count of all physical tap changes on phase A since beginning of simulation (plus initial value)",
			PT_double, "tap_B_change_count",PADDR(tap_B_change_count),PT_DESCRIPTION,"count of all physical tap changes on phase B since beginning of simulation (plus initial value)",
			PT_double, "tap_C_change_count",PADDR(tap_C_change_count),PT_DESCRIPTION,"count of all physical tap changes on phase C since beginning of simulation (plus initial value)",
			PT_object,"sense_node",PADDR(RemoteNode),PT_DESCRIPTION,"Node to be monitored for voltage control in remote sense mode",
			PT_double,"regulator_resistance[Ohm]",PADDR(regulator_resistance), PT_DESCRIPTION,"The resistance value of the regulator when it is not blown.",
			nullptr)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_regulator)==nullptr)
			GL_THROW("Unable to publish regulator deltamode function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==nullptr)
			GL_THROW("Unable to publish regulator external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==nullptr)
			GL_THROW("Unable to publish regulator external power limit calculation function");
		if (gl_publish_function(oclass,	"perform_current_calculation_pwr_link", (FUNCTIONADDR)currentcalculation_link)==nullptr)
			GL_THROW("Unable to publish regulator external current calculation function");
		
		//Other
		if (gl_publish_function(oclass, "pwr_object_kmldata", (FUNCTIONADDR)regulator_kmldata) == nullptr)
			GL_THROW("Unable to publish regulator kmldata function");
	}
}

int regulator::isa(char *classname)
{
	return strcmp(classname,"regulator")==0 || link_object::isa(classname);
}

int regulator::create()
{
	int result = link_object::create();
	configuration = nullptr;
	tap[0] = tap[1] = tap[2] = -999;
	offnominal_time = false;
	tap_A_change_count = -1;
	tap_B_change_count = -1;
	tap_C_change_count = -1;
	iteration_flag = true;
	regulator_resistance = -1.0;
	deltamode_reiter_request = false;	//By default, we're assumed to not want this
	msgmode = msg_INTERNAL;
	check_voltage[0] = check_voltage[1] = check_voltage[2] = 0.0;

	RNode_voltage[0] = RNode_voltage[1] = RNode_voltage[2] = nullptr;
	ToNode_voltage[0] = ToNode_voltage[1] = ToNode_voltage[2] = nullptr;

	return result;
}

int regulator::init(OBJECT *parent)
{
	bool TapInitialValue[3];
	char jindex;
	int result = link_object::init(parent);

	//Check for deferred
	if (result == 2)
		return 2;	//Return the deferment - no sense doing everything else!

	OBJECT *obj = OBJECTHDR(this);

	if (!configuration)
		throw "no regulator configuration specified.";
		/*  TROUBLESHOOT
		A regulator configuration was not provided.  Please use object regulator_configuration
		and define the necessary parameters of your regulator to continue.  See the media wiki for
		an example: http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Power_Flow_Guide
		*/

	if (!gl_object_isa(configuration, "regulator_configuration","powerflow"))
		throw "invalid regulator configuration";
		/*  TROUBLESHOOT
		An invalid regulator configuration was provided.  Ensure you have proper values in each field
		of the regulator_configuration object and that you haven't inadvertantly used a line or transformer
		configuration as the regulator configuration.  See the media wiki for an example:
		http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Power_Flow_Guide
		*/

	regulator_configuration *pConfig = OBJECTDATA(configuration, regulator_configuration);

	//See if any initial tap settings have been specified
	for (jindex=0;jindex<3;jindex++)
	{
		if (pConfig->tap_pos[jindex] != 999)
		{
			TapInitialValue[jindex] = true;	//Flag as an initial value
		}
		else
		{
			TapInitialValue[jindex] = false;	//Ensure is not flagged
			pConfig->tap_pos[jindex] = 0;		//Set back to default
		}
	}

	if (pConfig->Control == pConfig->REMOTE_NODE) 
	{
		if (RemoteNode == nullptr)
		{
			GL_THROW("Remote sensing node not found on regulator:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
			/* TROUBLESHOOT
			If you are trying to use REMOTE_NODE, then please specify a sense_node within the regulator
			object.  Otherwise, change your Control method.
			*/
		}
		else if (!gl_object_isa(RemoteNode,"node","powerflow") && !gl_object_isa(RemoteNode,"network_interface"))
		{
			GL_THROW("Remote sensing node is not a valid object in regulator:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The object specified in the sense_node property is not a node-type or network_interface object, so it will not work for the REMOTE_NODE.
			*/
		}

		//Map to the property of interest - voltage_A
	   if (msgmode == msg_INTERNAL)
	   {
			RNode_voltage[0] = new gld_property(RemoteNode,"voltage_A");
			//Make sure it worked
			if (!RNode_voltage[0]->is_valid() || !RNode_voltage[0]->is_complex())
			{
				GL_THROW("Regulator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
				/* TROUBLESHOOT
				While attempting to map a property for the sense_node, a property could not be properly mapped.
				Please try again.  If the error persists, please submit an issue in the ticketing system.
				*/

			}
			RNode_voltage[1] = new gld_property(RemoteNode,"voltage_B");
			//Make sure it worked
			if (!RNode_voltage[1]->is_valid() || !RNode_voltage[1]->is_complex())
			{
				GL_THROW("Regulator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}
				
			RNode_voltage[2] = new gld_property(RemoteNode,"voltage_C");










			//Make sure it worked

			if (!RNode_voltage[2]->is_valid() || !RNode_voltage[2]->is_complex())
			{

				GL_THROW("Regulator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}
			
	   
	   }
}
	//Map the to-node connections
	//Map to the property of interest - voltage_A
	ToNode_voltage[0] = new gld_property(to,"voltage_A");

	//Make sure it worked
	if (!ToNode_voltage[0]->is_valid() || !ToNode_voltage[0]->is_complex())
	{
		GL_THROW("Regulator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	//Map to the property of interest - voltage_B
	ToNode_voltage[1] = new gld_property(to,"voltage_B");

	//Make sure it worked
	if (!ToNode_voltage[1]->is_valid() || !ToNode_voltage[1]->is_complex())
	{
		GL_THROW("Regulator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	//Map to the property of interest - voltage_C
	ToNode_voltage[2] = new gld_property(to,"voltage_C");

	//Make sure it worked
	if (!ToNode_voltage[2]->is_valid() || !ToNode_voltage[2]->is_complex())
	{
		GL_THROW("Regulator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	// D_mat & W_mat - 3x3 matrix
	D_mat[0][0] = D_mat[1][1] = D_mat[2][2] = gld::complex(1,0);
	D_mat[0][1] = D_mat[2][0] = D_mat[1][2] = gld::complex(-1,0);
	D_mat[0][2] = D_mat[2][1] = D_mat[1][0] = gld::complex(0,0);
	
	W_mat[0][0] = W_mat[1][1] = W_mat[2][2] = gld::complex(2,0);
	W_mat[0][1] = W_mat[2][0] = W_mat[1][2] = gld::complex(1,0);
	W_mat[0][2] = W_mat[2][1] = W_mat[1][0] = gld::complex(0,0);
	
	multiply(1.0/3.0,W_mat,W_mat);
	
	tapChangePer = pConfig->regulation / (double) pConfig->raise_taps;
	Vlow = pConfig->band_center - pConfig->band_width / 2.0;
	Vhigh = pConfig->band_center + pConfig->band_width / 2.0;
	VtapChange = pConfig->band_center * tapChangePer;
	
	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
		{
			a_mat[i][j] = b_mat[i][j] = c_mat[i][j] = d_mat[i][j] =
					A_mat[i][j] = B_mat[i][j] = 0.0;
			base_admittance_mat[i][j] = gld::complex(0.0,0.0);
		}
	}

	for (int i = 0; i < 3; i++) 
	{	
		if (tap[i] == -999)
			tap[i] = pConfig->tap_pos[i];

		if (pConfig->Type == pConfig->A)
			a_mat[i][i] = 1/(1.0 + tap[i] * tapChangePer);
		else if (pConfig->Type == pConfig->B)
			a_mat[i][i] = 1.0 - tap[i] * tapChangePer;
		else
			throw "invalid regulator type";
			/*  TROUBLESHOOT
			Check the Type specification in your regulator_configuration object.  It can an only be type A or B.
			*/
	}

	gld::complex tmp_mat[3][3] = {{gld::complex(1,0)/a_mat[0][0],gld::complex(0,0),gld::complex(0,0)},
			                 {gld::complex(0,0), gld::complex(1,0)/a_mat[1][1],gld::complex(0,0)},
			                 {gld::complex(-1,0)/a_mat[0][0],gld::complex(-1,0)/a_mat[1][1],gld::complex(0,0)}};
	gld::complex tmp_mat1[3][3];

	switch (pConfig->connect_type) {
		case regulator_configuration::WYE_WYE:
			for (int i = 0; i < 3; i++)
				d_mat[i][i] = gld::complex(1.0,0) / a_mat[i][i];
			inverse(a_mat,A_mat);

			if (solver_method == SM_NR)
			{
				if(regulator_resistance == 0.0){
					gl_warning("Regulator:%s regulator_resistance has been set to zero. This will result singular matrix. Setting to the global default.",obj->name);
					/*  TROUBLESHOOT
					Under Newton-Raphson solution method the impedance matrix cannot be a singular matrix for the inversion process.
					Change the value of regulator_resistance to something small but larger that zero.
					*/
				}
				if(regulator_resistance < 0.0){
					regulator_resistance = default_resistance;
				}
				SpecialLnk = REGULATOR;
				//gld::complex Izt = gld::complex(1,0) / zt;
				if (has_phase(PHASE_A))
				{
					base_admittance_mat[0][0] = gld::complex(1.0/regulator_resistance,0.0);
					b_mat[0][0] = regulator_resistance;
				}
				if (has_phase(PHASE_B))
				{
					base_admittance_mat[1][1] = gld::complex(1.0/regulator_resistance,0.0);
					b_mat[1][1] = regulator_resistance;
				}
				if (has_phase(PHASE_C))
				{
					base_admittance_mat[2][2] = gld::complex(1.0/regulator_resistance,0.0);
					b_mat[2][2] = regulator_resistance;
				}
			}
			break;
		case regulator_configuration::OPEN_DELTA_ABBC:
			d_mat[0][0] = gld::complex(1,0) / a_mat[0][0];
			d_mat[1][0] = gld::complex(-1,0) / a_mat[0][0];
			d_mat[1][2] = gld::complex(-1,0) / a_mat[1][1];
			d_mat[2][2] = gld::complex(1,0) / a_mat[1][1];

			a_mat[2][0] = -a_mat[0][0];
			a_mat[2][1] = -a_mat[1][1];
			a_mat[2][2] = 0;

			multiply(W_mat,tmp_mat,tmp_mat1);
			multiply(tmp_mat1,D_mat,A_mat);

			gl_warning("Only WYE-WYE configurations are working in either Newton-Raphson or non-Manual controls");
			/*  TROUBLESHOOT
			For this portion of development, only WYE-WYE configurations are fully supported.  Later implementations
			will include support for other regulator configurations.  As it stands, WYE-WYE regulators work in SM_NR 
			for both Manual and Automatic controls, while OPEN_DELTA_ABBC is only supported in single powerflow simulations.
			*/
			break;
		case regulator_configuration::OPEN_DELTA_BCAC:
			throw "Regulator connect type not supported yet";
			/*  TROUBLESHOOT
			Check the connection type specified.  Only a few are available at this time.  Ones available can be
			found on the wiki website ( http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Power_Flow_Guide )
			*/
			break;
		case regulator_configuration::OPEN_DELTA_CABA:
			throw "Regulator connect type not supported yet";
			/*  TROUBLESHOOT
			Check the connection type specified.  Only a few are available at this time.  Ones available can be
			found on the wiki website ( http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Power_Flow_Guide )
			*/
			break;
		case regulator_configuration::CLOSED_DELTA:
			throw "Regulator connect type not supported yet";
			/*  TROUBLESHOOT
			Check the connection type specified.  Only a few are available at this time.  Ones available can be
			found on the wiki website ( http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Power_Flow_Guide )
			*/
			break;
		default:
			throw "unknown regulator connect type";
			/*  TROUBLESHOOT
			Check the connection type specified.  Only a few are available at this time.  Ones available can be
			found on the wiki website ( http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Power_Flow_Guide )
			*/
			break;
	}

	mech_t_next[0] = mech_t_next[1] = mech_t_next[2] = TS_NEVER_DBL;
	dwell_t_next[0] = dwell_t_next[1] = dwell_t_next[2] = TS_NEVER_DBL;

	//Now set first_run_flag appropriately
	for (jindex=0;jindex<3;jindex++)
	{
		if (!TapInitialValue[jindex])
		{
			if (solver_method == SM_NR)
				first_run_flag[jindex] = -2;
			else
				first_run_flag[jindex] = -1;	//Let the code find a value
		}
		else
		{
			first_run_flag[jindex] = 0;		//Set so the regulator will operate normally here
		}
	}

	//Update previous taps variable for NR
	prev_tap[0] = tap[0];
	prev_tap[1] = tap[1];
	prev_tap[2] = tap[2];

	//Get global_minimum_timestep value and set the appropriate flag
	unsigned int glob_min_timestep, temp_val;
	char temp_buff[128];
	int indexval;

	//Retrieve the global value, only does so as a text string for some reason
	gl_global_getvar("minimum_timestep",temp_buff,sizeof(temp_buff));

	//Initialize our parsing variables
	indexval = 0;
	glob_min_timestep = 0;

	//Loop through the buffer
	while ((indexval < 128) && (temp_buff[indexval] != 0))
	{
		glob_min_timestep *= 10;				//Adjust previous iteration value
		temp_val = (temp_buff[indexval]-48);	//Decode the ASCII
		glob_min_timestep += temp_val;			//Accumulate it

		indexval++;								//Increment the index
	}

	if (glob_min_timestep > 1)					//Now check us and set the flag if true
		offnominal_time=true;

	prev_tap_A = initial_tap_A = tap[0];
	prev_tap_B = initial_tap_B = tap[1];
	prev_tap_C = initial_tap_C = tap[2];
	if(tap_A_change_count < 0)
		tap_A_change_count = 0;
	if(tap_B_change_count < 0)
		tap_B_change_count = 0;
	if(tap_C_change_count < 0)
		tap_C_change_count = 0;

	tap_A_changed = tap_B_changed = tap_C_changed = 2;
	prev_time = gl_globalclock;
	new_reverse_flow_action[0] = new_reverse_flow_action[1] = new_reverse_flow_action[2] = false;
	return result;
}


TIMESTAMP regulator::presync(TIMESTAMP t0) 
{
	regulator_configuration *pConfig = OBJECTDATA(configuration, regulator_configuration);
	TIMESTAMP t1;
	double t1_dbl, t0_dbl;
	char phaseWarn;

	//Cast the timestamp
	t0_dbl = (double)t0;

	//Toggle the iteration variable -- only for voltage-type adjustments (since it's in presync now)
	if ((solver_method == SM_NR) && ((pConfig->Control == pConfig->OUTPUT_VOLTAGE) || (pConfig->Control == pConfig->REMOTE_NODE)))
		iteration_flag = !iteration_flag;

	//Call the pre-presync regulator code
	reg_prePre_fxn(t0_dbl);

	//Call the standard presync
	t1 = link_object::presync(t0);

	//Cast timestamp, for comparisons below
	t1_dbl = (double)t1;
	
	//Call the post-presync regulator code
	reg_postPre_fxn();

	//Check the time handling
	if (offnominal_time && (t0_dbl > next_time))
	{
		next_time = t0_dbl;
	}

	//Force a "reiteration" if we're checking voltage - consequence of this previously being in true pass of NR
	if ((solver_method == SM_NR) && ((pConfig->Control == pConfig->OUTPUT_VOLTAGE) || (pConfig->Control == pConfig->REMOTE_NODE)) && !iteration_flag)
	{
		return t0;
	}

	if ((first_run_flag[0] < 1) || (first_run_flag[1] < 1) || (first_run_flag[2] < 1)) return t1;
	else if (t1_dbl <= next_time) return t1;
	else if (next_time != TS_NEVER_DBL) return -next_time; //soft return to next tap change
	else return TS_NEVER;
}

//Postsync
TIMESTAMP regulator::postsync(TIMESTAMP t0)
{
	double function_return_time;

	TIMESTAMP t1 = link_object::postsync(t0);

	function_return_time = reg_postPost_fxn(double(t0));

	//See if it was an error or not
	if (function_return_time == -1.0)
	{
		return TS_INVALID;
	}
	else if (function_return_time != 0.0)
	{
		//Based on the code logic, this was always a return t0
		//If this changes, re-evaluate this code
		return t0;
	}
	//Default else -- no returns were hit, so just do t1

	return t1;
}

//Functionalized "presync before link::presync" portions, mostly for deltamode functionality
void regulator::reg_prePre_fxn(double curr_time_value)
{
	regulator_configuration *pConfig = OBJECTDATA(configuration, regulator_configuration);


	if (pConfig->Control == pConfig->MANUAL) {
		for (int i = 0; i < 3; i++) {
			if (pConfig->Type == pConfig->A)
			{	a_mat[i][i] = 1/(1.0 + tap[i] * tapChangePer);}
			else if (pConfig->Type == pConfig->B)
			{	a_mat[i][i] = 1.0 - tap[i] * tapChangePer;}
			else
			{
				GL_THROW("invalid regulator type");
				/*  TROUBLESHOOT
				Check the Type specification in your regulator_configuration object.  It can an only be type A or B.
				*/
			}
		}
		next_time = TS_NEVER_DBL;
	}
	else if (iteration_flag)
	{
		if (pConfig->control_level == pConfig->INDIVIDUAL)
		{
			//Set flags correctly for each pass, 1 indicates okay to change taps, 0 indicates no go
			for (int i = 0; i < 3; i++) {
				if (mech_t_next[i] <= curr_time_value) {
					mech_flag[i] = 1;
				}
				if (dwell_t_next[i] <= curr_time_value) {
					dwell_flag[i] = 1;
				}
				else if (dwell_t_next[i] > curr_time_value) {
					dwell_flag[i] = 0;
				}
			}

			get_monitored_voltage();

			if (pConfig->connect_type == pConfig->WYE_WYE)
			{	
				//Update first run flag - special solver during first time solved.
				if ((first_run_flag[0] + first_run_flag[1] + first_run_flag[2]) < 3 ) {
					for (int i = 0; i < 3; i++) {
						if (first_run_flag[i] < 1) {
							first_run_flag[i] += 1;
						}
					}
				}

				for (int i = 0; i < 3; i++) 
				{
					
					if (check_voltage[i].Mag() < Vlow)		//raise voltage
					{	
						//hit the band center for convergence on first run, otherwise bad initial guess on tap settings 
						//can fail on the first timestep
						if (first_run_flag[i] == 0) 
						{	
							if(toggle_reverse_flow[i]) {
								tap[i] = reverse_flow_tap[i];
							} else {
								tap[i] = tap[i] + (int16)ceil((pConfig->band_center - check_voltage[i].Mag())/VtapChange);
							}
							if (tap[i] > pConfig->raise_taps) 
							{
								tap[i] = pConfig->raise_taps;
							}
							dwell_t_next[i] = curr_time_value + pConfig->dwell_time;
							mech_t_next[i] = curr_time_value + pConfig->time_delay;
						}
						//dwelling has happened, and now waiting for actual physical change time
						else if (mech_flag[i] == 0 && dwell_flag[i] == 1 && (mech_t_next[i] - curr_time_value) >= pConfig->time_delay)
						{
							mech_t_next[i] = curr_time_value + pConfig->time_delay;
						}
						//if both flags say it's okay to change the tap, then change the tap
						else if (mech_flag[i] == 1 && dwell_flag[i] == 1) 
						{
							if(toggle_reverse_flow[i]) {
								tap[i] = reverse_flow_tap[i];
							} else {
								tap[i] = tap[i] + (int16) 1;
							}

							if (tap[i] > pConfig->raise_taps) 
							{
								tap[i] = pConfig->raise_taps;
								dwell_t_next[i] = curr_time_value + pConfig->dwell_time;
								mech_t_next[i] = curr_time_value + pConfig->time_delay;
								dwell_flag[i] = mech_flag[i] = 0;
							}
							else 
							{
								mech_t_next[i] = curr_time_value + pConfig->time_delay;
								dwell_t_next[i] = curr_time_value + pConfig->dwell_time;
								mech_flag[i] = 0;
							}
						}
						//only set the dwell time if we've reached the end of the previous dwell (in case other 
						//objects update during that time)
						else if (dwell_flag[i] == 0 && (dwell_t_next[i] - curr_time_value) >= pConfig->dwell_time) 
						{
							dwell_t_next[i] = curr_time_value + pConfig->dwell_time;
							mech_t_next[i] = dwell_t_next[i] + pConfig->time_delay;
						}														
					}
					else if (check_voltage[i].Mag() > Vhigh)  //lower voltage
					{
						if (first_run_flag[i] == 0) 
						{
							if(toggle_reverse_flow[i]) {
								tap[i] = reverse_flow_tap[i];
							} else {
								tap[i] = tap[i] - (int16)ceil((check_voltage[i].Mag() - pConfig->band_center)/VtapChange);
							}
							if (tap[i] < -pConfig->lower_taps) 
							{
								tap[i] = -pConfig->lower_taps;
							}
							dwell_t_next[i] = curr_time_value + pConfig->dwell_time;
							mech_t_next[i] = curr_time_value + pConfig->time_delay;
						}
						else if (mech_flag[i] == 0 && dwell_flag[i] == 1 && (mech_t_next[i] - curr_time_value) >= pConfig->time_delay)
						{
							mech_t_next[i] = curr_time_value + pConfig->time_delay;
						}
						else if (mech_flag[i] == 1 && dwell_flag[i] == 1) 
						{
							if(toggle_reverse_flow[i]) {
								tap[i] = reverse_flow_tap[i];
							} else {
								tap[i] = tap[i] - (int16) 1;
							}
							if (tap[i] < -pConfig->lower_taps) 
							{
								tap[i] = -pConfig->lower_taps;
								dwell_t_next[i] = curr_time_value + pConfig->dwell_time;
								mech_t_next[i] = curr_time_value + pConfig->time_delay;
								dwell_flag[i] = mech_flag[i] = 0;
							}
							else 
							{
								mech_t_next[i] = curr_time_value + pConfig->time_delay;
								dwell_t_next[i] = curr_time_value + pConfig->dwell_time;
								mech_flag[i] = 0;
							}
						}
						else if (dwell_flag[i] == 0 && (dwell_t_next[i] - curr_time_value) >= pConfig->dwell_time) 
						{
							dwell_t_next[i] = curr_time_value + pConfig->dwell_time;
							mech_t_next[i] = dwell_t_next[i] + pConfig->time_delay;
						}
					}
					//If no tap changes were needed, then this resets dwell_flag to 0 and indicates regulator has no
					//more changes unless system changes
					else 
					{	
						dwell_t_next[i] = mech_t_next[i] = TS_NEVER_DBL;
						//if (pConfig->dwell_time == 0)
						//	dwell_flag[i] = 1;
						//else
							dwell_flag[i] = 0;
						//if (pConfig->time_delay == 0)
						//	mech_flag[i] = 1;
						//else
							mech_flag[i] = 0;
					}

					//Use tap positions to solve for 'a' matrix
					if (pConfig->Type == pConfig->A)
					{	a_mat[i][i] = 1/(1.0 + tap[i] * tapChangePer);}
					else if (pConfig->Type == pConfig->B)
					{	a_mat[i][i] = 1.0 - tap[i] * tapChangePer;}
					else
					{	
						GL_THROW("invalid regulator type");
						/*  TROUBLESHOOT
						Check the Type of regulator specified.  Type can only be A or B at this time.
						*/
					}
				}
				//Determine how far to advance the clock
				double nt[3];
				nt[0] = nt[1] = nt[2] = curr_time_value;
				for (int i = 0; i < 3; i++) {
					if (mech_t_next[i] > curr_time_value)
						nt[i] = mech_t_next[i];
					if (dwell_t_next[i] > curr_time_value)
						nt[i] = dwell_t_next[i];
				}

				if (nt[0] > curr_time_value)
					next_time = nt[0];
				if (nt[1] > curr_time_value && nt[1] < next_time)
					next_time = nt[1];
				if (nt[2] > curr_time_value && nt[2] < next_time)
					next_time = nt[2];

				if (next_time <= curr_time_value)
					next_time = TS_NEVER_DBL;
			}
			else
				GL_THROW("Specified connect type is not supported in automatic modes at this time.");
				/* TROUBLESHOOT
				At this time only WYE-WYE regulators are supported in automatic control modes. 
				OPEN_DELTA_ABBC will only work in MANUAL control mode and in FBS at this time.
				*/
		}

		else if (pConfig->control_level == pConfig->BANK)
		{
			//Set flags correctly for each pass, 1 indicates okay to change taps, 0 indicates no go - we'll store all of banked stuff in index=0
			if (mech_t_next[0] <= curr_time_value) {
				mech_flag[0] = 1;
			}
			if (dwell_t_next[0] <= curr_time_value) {
				dwell_flag[0] = 1;
			}
			else if (dwell_t_next[0] > curr_time_value) {
				dwell_flag[0] = 0;
			}

			get_monitored_voltage();

			if (pConfig->connect_type == pConfig->WYE_WYE)
			{	
				//Update first run flag - special solver during first time solved.
				if (first_run_flag[0] < 1)
				{
					first_run_flag[0] += 1;	
					first_run_flag[1] += 1;
					first_run_flag[2] += 1;
				}			

				if (check_voltage[0].Mag() < Vlow)		//raise voltage
				{	
					//hit the band center for convergence on first run, otherwise bad initial guess on tap settings 
					//can fail on the first timestep
					if (first_run_flag[0] == 0) 
					{
						if(toggle_reverse_flow_banked) {
							tap[0] = reverse_flow_tap[0];
							tap[1] = reverse_flow_tap[1];
							tap[2] = reverse_flow_tap[2];
						} else {
							tap[0] = tap[1] = tap[2] = tap[0] + (int16)ceil((pConfig->band_center - check_voltage[0].Mag())/VtapChange);
						}
						if (tap[0] > pConfig->raise_taps) 
						{
							tap[0] = tap[1] = tap[2] = pConfig->raise_taps;
						}
						dwell_t_next[0] = curr_time_value + pConfig->dwell_time;
						mech_t_next[0] = curr_time_value + pConfig->time_delay;
					}
					//dwelling has happened, and now waiting for actual physical change time
					else if (mech_flag[0] == 0 && dwell_flag[0] == 1 && (mech_t_next[0] - curr_time_value) >= pConfig->time_delay)
					{
						mech_t_next[0] = curr_time_value + pConfig->time_delay;
					}
					//if both flags say it's okay to change the tap, then change the tap
					else if (mech_flag[0] == 1 && dwell_flag[0] == 1) 
					{		 
						if(toggle_reverse_flow_banked) {
							tap[0] = reverse_flow_tap[0];
							tap[1] = reverse_flow_tap[1];
							tap[2] = reverse_flow_tap[2];
						} else {
							tap[0] = tap[1] = tap[2] = tap[0] + (int16) 1;
						}
						if (tap[0] > pConfig->raise_taps) 
						{
							tap[0] = tap[1] = tap[2] = pConfig->raise_taps;
							dwell_t_next[0] = curr_time_value + pConfig->dwell_time;
							mech_t_next[0] = curr_time_value + pConfig->time_delay;
							dwell_flag[0] = mech_flag[0] = 0;
						}
						else 
						{
							mech_t_next[0] = curr_time_value + pConfig->time_delay;
							dwell_t_next[0] = curr_time_value + pConfig->dwell_time;
							mech_flag[0] = 0;
						}
					}
					//only set the dwell time if we've reached the end of the previous dwell (in case other 
					//objects update during that time)
					else if (dwell_flag[0] == 0 && (dwell_t_next[0] - curr_time_value) >= pConfig->dwell_time) 
					{
						dwell_t_next[0] = curr_time_value + pConfig->dwell_time;
						mech_t_next[0] = dwell_t_next[0] + pConfig->time_delay;
					}														
				}
				else if (check_voltage[0].Mag() > Vhigh)  //lower voltage
				{
					if (first_run_flag[0] == 0) 
					{
						if(toggle_reverse_flow_banked) {
							tap[0] = reverse_flow_tap[0];
							tap[1] = reverse_flow_tap[1];
							tap[2] = reverse_flow_tap[2];
						} else {
							tap[0] = tap[1] = tap[2] = tap[0] - (int16)ceil((check_voltage[0].Mag() - pConfig->band_center)/VtapChange);
						}
						if (tap[0] < -pConfig->lower_taps) 
						{
							tap[0] = tap[1] = tap[2] = -pConfig->lower_taps;
						}
						dwell_t_next[0] = curr_time_value + pConfig->dwell_time;
						mech_t_next[0] = curr_time_value + pConfig->time_delay;
					}
					else if (mech_flag[0] == 0 && dwell_flag[0] == 1 && (mech_t_next[0] - curr_time_value) >= pConfig->time_delay)
					{
						mech_t_next[0] = curr_time_value + pConfig->time_delay;
					}
					else if (mech_flag[0] == 1 && dwell_flag[0] == 1) 
					{
						if(toggle_reverse_flow_banked) {
							tap[0] = reverse_flow_tap[0];
							tap[1] = reverse_flow_tap[1];
							tap[2] = reverse_flow_tap[2];
						} else {
							tap[0] = tap[1] = tap[2] = tap[0] - (int16) 1;
						}
						if (tap[0] < -pConfig->lower_taps) 
						{
							tap[0] = tap[1] = tap[2] = -pConfig->lower_taps;
							dwell_t_next[0] = curr_time_value + pConfig->dwell_time;
							mech_t_next[0] = curr_time_value + pConfig->time_delay;
							dwell_flag[0] = mech_flag[0] = 0;
						}
						else 
						{
							mech_t_next[0] = curr_time_value + pConfig->time_delay;
							dwell_t_next[0] = curr_time_value + pConfig->dwell_time;
							mech_flag[0] = 0;
						}
					}
					else if (dwell_flag[0] == 0 && (dwell_t_next[0] - curr_time_value) >= pConfig->dwell_time) 
					{
						dwell_t_next[0] = curr_time_value + pConfig->dwell_time;
						mech_t_next[0] = dwell_t_next[0] + pConfig->time_delay;
					}
				}
				//If no tap changes were needed, then this resets dwell_flag to 0 and indicates regulator has no
				//more changes unless system changes
				else 
				{	
					dwell_t_next[0] = mech_t_next[0] = TS_NEVER_DBL;
					dwell_flag[0] = 0;
					mech_flag[0] = 0;
				}

				for (int i = 0; i < 3; i++) {
					//Use tap positions to solve for 'a' matrix
					if (pConfig->Type == pConfig->A)
					{	a_mat[i][i] = 1/(1.0 + tap[i] * tapChangePer);}
					else if (pConfig->Type == pConfig->B)
					{	a_mat[i][i] = 1.0 - tap[i] * tapChangePer;}
					else
					{
						GL_THROW("invalid regulator type");
						/*  TROUBLESHOOT
						Check the Type of regulator specified.  Type can only be A or B at this time.
						*/
					}
				}

				//Determine how far to advance the clock
				double nt[3];
				nt[0] = nt[1] = nt[2] = curr_time_value;
				if (mech_t_next[0] > curr_time_value)
					nt[0] = mech_t_next[0];
				if (dwell_t_next[0] > curr_time_value)
					nt[0] = dwell_t_next[0];

				if (nt[0] > curr_time_value)
					next_time = nt[0];

				if (next_time <= curr_time_value)
					next_time = TS_NEVER_DBL;
			}
			else
				GL_THROW("Specified connect type is not supported in automatic modes at this time.");
				/* TROUBLESHOOT
				At this time only WYE-WYE regulators are supported in automatic control modes. 
				OPEN_DELTA_ABBC will only work in MANUAL control mode and in FBS at this time.
				*/
		}
		
	}
			
	//Use 'a' matrix to solve appropriate 'A' & 'd' matrices
	gld::complex tmp_mat[3][3] = {{gld::complex(1,0)/a_mat[0][0],gld::complex(0,0),gld::complex(0,0)},
							 {gld::complex(0,0), gld::complex(1,0)/a_mat[1][1],gld::complex(0,0)},
							 {gld::complex(-1,0)/a_mat[0][0],gld::complex(-1,0)/a_mat[1][1],gld::complex(0,0)}};
	gld::complex tmp_mat1[3][3];

	switch (pConfig->connect_type) {
		case regulator_configuration::WYE_WYE:
			for (int i = 0; i < 3; i++)
			{	d_mat[i][i] = gld::complex(1.0,0) / a_mat[i][i]; }
			inverse(a_mat,A_mat);
			break;
		case regulator_configuration::OPEN_DELTA_ABBC:
			d_mat[0][0] = gld::complex(1,0) / a_mat[0][0];
			d_mat[1][0] = gld::complex(-1,0) / a_mat[0][0];
			d_mat[1][2] = gld::complex(-1,0) / a_mat[1][1];
			d_mat[2][2] = gld::complex(1,0) / a_mat[1][1];

			a_mat[2][0] = -a_mat[0][0];
			a_mat[2][1] = -a_mat[1][1];
			a_mat[2][2] = 0;

			multiply(W_mat,tmp_mat,tmp_mat1);
			multiply(tmp_mat1,D_mat,A_mat);
			break;
		case regulator_configuration::OPEN_DELTA_BCAC:
			break;
		case regulator_configuration::OPEN_DELTA_CABA:
			break;
		case regulator_configuration::CLOSED_DELTA:
			break;
		default:
			GL_THROW("unknown regulator connect type");
			/*  TROUBLESHOOT
			Check the connection type specified.  Only a few are available at this time.  Ones available can be
			found on the wiki website ( http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Power_Flow_Guide )
			*/
			break;
	}
}

//Functionalized version of the code for deltamode - "post-link::presync" portions
void regulator::reg_postPre_fxn(void)
{
	regulator_configuration *pConfig = OBJECTDATA(configuration, regulator_configuration);
	char phaseWarn;

	if (solver_method == SM_NR)
	{
		//Get matrices for NR
		int jindex,kindex;
		gld::complex Ylefttemp[3][3];
		gld::complex Yto[3][3];
		gld::complex Yfrom[3][3];

		//Pre-admittancized matrix
		equalm(base_admittance_mat,Yto);

		//Store value into YSto
		for (jindex=0; jindex<3; jindex++)
		{
			for (kindex=0; kindex<3; kindex++)
			{
				YSto[jindex*3+kindex]=Yto[jindex][kindex];
			}
		}
		
		for (jindex=0; jindex<3; jindex++)
		{
			Ylefttemp[jindex][jindex] = Yto[jindex][jindex] * gld::complex(1,0) / a_mat[jindex][jindex];
			Yfrom[jindex][jindex]=Ylefttemp[jindex][jindex] * gld::complex(1,0) / a_mat[jindex][jindex];
		}


		//multiply(invratio,Yto,Ylefttemp);		//Scale from admittance by turns ratio
		//multiply(invratio,Ylefttemp,Yfrom);

		//Store value into YSfrom
		for (jindex=0; jindex<3; jindex++)
		{
			for (kindex=0; kindex<3; kindex++)
			{
				YSfrom[jindex*3+kindex]=Yfrom[jindex][kindex];
			}
		}

		for (jindex=0; jindex<3; jindex++)
		{
			To_Y[jindex][jindex] = Yto[jindex][jindex] * gld::complex(1,0) / a_mat[jindex][jindex];
			From_Y[jindex][jindex]=Yfrom[jindex][jindex] * a_mat[jindex][jindex];
		}
		//multiply(invratio,Yto,To_Y);		//Incorporate turns ratio information into line's admittance matrix.
		//multiply(voltage_ratio,Yfrom,From_Y); //Scales voltages to same "level" for GS //uncomment me

		//Only perform update if a tap has changed.  Since the link calculations are replicated here and it directly
		//accesses the NR memory space, this won't cause any issues.
		if ((prev_tap[0] != tap[0]) || (prev_tap[1] != tap[1]) || (prev_tap[2] != tap[2]))	//Change has occurred
		{
			//Flag an update
			LOCK_OBJECT(NR_swing_bus);	//Lock SWING since we'll be modifying this
			NR_admit_change = true;
			UNLOCK_OBJECT(NR_swing_bus);	//Unlock

			//Update our previous tap positions
			prev_tap[0] = tap[0];
			prev_tap[1] = tap[1];
			prev_tap[2] = tap[2];
		}
	}

	//General warnings for if we're at a railed tap limit
	if (tap[0] == pConfig->raise_taps && has_phase(PHASE_A))
	{
		phaseWarn='A';	//Just so troubleshoot is generic

		gl_warning("Regulator %s has phase %c at the maximum tap value",OBJECTHDR(this)->name,phaseWarn);
		/*  TROUBLESHOOT
		The regulator has set its taps such that it is at the maximum setting.  This may indicate
		a problem with settings, or your system.
		*/
	}

	if (tap[1] == pConfig->raise_taps && has_phase(PHASE_B))
	{
		phaseWarn='B';	//Just so troubleshoot is generic

		gl_warning("Regulator %s has phase %c at the maximum tap value",OBJECTHDR(this)->name,phaseWarn);
		//Defined above
	}

	if (tap[2] == pConfig->raise_taps && has_phase(PHASE_C))
	{
		phaseWarn='C';	//Just so troubleshoot is generic

		gl_warning("Regulator %s has phase %c at the maximum tap value",OBJECTHDR(this)->name,phaseWarn);
		//Defined above
	}

	if (tap[0] == -pConfig->lower_taps && has_phase(PHASE_A))
	{
		phaseWarn='A';	//Just so troubleshoot is generic

		gl_warning("Regulator %s has phase %c at the minimum tap value",OBJECTHDR(this)->name,phaseWarn);
		/*  TROUBLESHOOT
		The regulator has set its taps such that it is at the minimum setting.  This may indicate
		a problem with settings, or your system.
		*/
	}

	if (tap[1] == -pConfig->lower_taps && has_phase(PHASE_B))
	{
		phaseWarn='B';	//Just so troubleshoot is generic

		gl_warning("Regulator %s has phase %c at the minimum tap value",OBJECTHDR(this)->name,phaseWarn);
		//Defined above
	}

	if (tap[2] == -pConfig->lower_taps && has_phase(PHASE_C))
	{
		phaseWarn='C';	//Just so troubleshoot is generic

		gl_warning("Regulator %s has phase %c at the minimum tap value",OBJECTHDR(this)->name,phaseWarn);
		//Defined above
	}
}

//Functionalized "postsyc after link::postsync" items -- mostly for deltamode compatibility
double regulator::reg_postPost_fxn(double curr_time_value)
{
	regulator_configuration *pConfig = OBJECTDATA(configuration, regulator_configuration);

	//Copied from postsync
	if (iteration_flag)
	{		
		if(prev_time < curr_time_value){
			prev_time = curr_time_value;
			initial_tap_A = prev_tap_A;
			initial_tap_B = prev_tap_B;
			initial_tap_C = prev_tap_C;
			tap_A_changed = 0;
			tap_B_changed = 0;
			tap_C_changed = 0;
			if(prev_tap_A != tap[0]){
				prev_tap_A = tap[0];
				tap_A_change_count++;
				tap_A_changed = 1;
			}
			if(prev_tap_B != tap[1]){
				prev_tap_B = tap[1];
				tap_B_change_count++;
				tap_B_changed = 1;
			}
			if(prev_tap_C != tap[2]){
				prev_tap_C = tap[2];
				tap_C_change_count++;
				tap_C_changed = 1;
			}
		}
		if(prev_time == curr_time_value){
			if(tap_A_changed == 0){
				if(prev_tap_A != tap[0]){
					prev_tap_A = tap[0];
					tap_A_change_count++;
					tap_A_changed = 1;
				}
			}
			if(tap_A_changed == 1){
				if(initial_tap_A == tap[0]){
					prev_tap_A = tap[0];
					tap_A_change_count--;
					if(tap_A_change_count < 0){
						gl_error("Unusual control of the regulator has resulted in a negative tap change count on phase A.");
						return -1.0;
					}
					tap_A_changed = 0;
				} else if(prev_tap_A != tap[0]){
					prev_tap_A = tap[0];
				}
			}
			if(tap_A_changed == 2){
				prev_tap_A = tap[0];
			}
			if(tap_B_changed == 0){
				if(prev_tap_B != tap[1]){
					prev_tap_B = tap[1];
					tap_B_change_count++;
					tap_B_changed = 1;
				}
			}
			if(tap_B_changed == 1){
				if(initial_tap_B == tap[1]){
					prev_tap_B = tap[1];
					tap_B_change_count--;
					if(tap_B_change_count < 0){
						gl_error("Unusual control of the regulator has resulted in a negative tap change count on phase B.");
						return -1.0;
					}
					tap_B_changed = 0;
				}else if(prev_tap_B != tap[1]){
					prev_tap_B = tap[1];
				}
			}
			if(tap_B_changed == 2){
				prev_tap_B = tap[1];
			}
			if(tap_C_changed == 0){
				if(prev_tap_C != tap[2]){
					prev_tap_C = tap[2];
					tap_C_change_count++;
					tap_C_changed = 1;
				}
			}
			if(tap_C_changed == 1){
				if(initial_tap_C == tap[2]){
					prev_tap_C = tap[2];
					tap_C_change_count--;
					if(tap_C_change_count < 0){
						gl_error("Unusual control of the regulator has resulted in a negative tap change count on phase C.");
						return -1.0;
					}
					tap_C_changed = 0;
				}else if(prev_tap_C != tap[2]){
					prev_tap_C = tap[2];
				}
			}
			if(tap_C_changed == 2){
				prev_tap_C = tap[2];
			}
		}

		if (pConfig->Control != pConfig->MANUAL) 
		{
			for (int i = 0; i < 3; i++) {
				if (mech_t_next[i] <= curr_time_value) {
					mech_flag[i] = 1;
				}
				if (dwell_t_next[i] <= curr_time_value) {
					dwell_flag[i] = 1;
				}
				else if (dwell_t_next[i] > curr_time_value) {
					dwell_flag[i] = 0;
				}
			}
	
			get_monitored_voltage();

			int i;
			for (i = 0; i < 3; i++) {
				new_reverse_flow_action[i] = false;
			}
			if (pConfig->reverse_flow_control == pConfig->LOCK_NEUTRAL) {
				if (pConfig->control_level == pConfig->INDIVIDUAL) {
					if ((flow_direction & FD_A_MASK) == FD_A_REVERSE && !toggle_reverse_flow[0]) {// Phase A power in reverse
						toggle_reverse_flow[0] = true;
						reverse_flow_tap[0] = 0;
						new_reverse_flow_action[0] = true;
					} else if ((flow_direction & FD_A_MASK) != FD_A_REVERSE && toggle_reverse_flow[0]) {
						toggle_reverse_flow[0] = false;
					}
					if ((flow_direction & FD_B_MASK) == FD_B_REVERSE && !toggle_reverse_flow[1]) {// Phase A power in reverse
						toggle_reverse_flow[1] = true;
						reverse_flow_tap[1] = 0;
						new_reverse_flow_action[1] = true;
					} else if ((flow_direction & FD_B_MASK) != FD_B_REVERSE && toggle_reverse_flow[1]) {
						toggle_reverse_flow[1] = false;
					}
					if ((flow_direction & FD_C_MASK) == FD_C_REVERSE && !toggle_reverse_flow[2]) {// Phase A power in reverse
						toggle_reverse_flow[2] = true;
						reverse_flow_tap[2] = 0;
						new_reverse_flow_action[2] = true;
					} else if ((flow_direction & FD_C_MASK) != FD_C_REVERSE && toggle_reverse_flow[2]) {
						toggle_reverse_flow[2] = false;
					}
				} else if (pConfig->control_level == pConfig->BANK) {
					if (((flow_direction & FD_A_MASK) == FD_A_REVERSE || (flow_direction & FD_B_MASK) == FD_B_REVERSE || (flow_direction & FD_C_MASK) == FD_C_REVERSE) && !toggle_reverse_flow_banked) {// Phase A power in reverse
						toggle_reverse_flow_banked = true;
						reverse_flow_tap[0] = 0;
						reverse_flow_tap[1] = 0;
						reverse_flow_tap[2] = 0;
						new_reverse_flow_action[0] = true;
						new_reverse_flow_action[1] = true;
						new_reverse_flow_action[2] = true;
					} else if (((flow_direction & FD_A_MASK) != FD_A_REVERSE && (flow_direction & FD_B_MASK) != FD_B_REVERSE && (flow_direction & FD_C_MASK) != FD_C_REVERSE) && toggle_reverse_flow_banked) {// Phase A power in reverse
						toggle_reverse_flow_banked = false;
					}
				}
			} else if (pConfig->reverse_flow_control == pConfig->LOCK_CURRENT) {
				if (pConfig->control_level == pConfig->INDIVIDUAL) {
					if ((flow_direction & FD_A_MASK) == FD_A_REVERSE && !toggle_reverse_flow[0]) {// Phase A power in reverse
						toggle_reverse_flow[0] = true;
						reverse_flow_tap[0] = tap[0];
						new_reverse_flow_action[0] = true;
					} else if ((flow_direction & FD_A_MASK) != FD_A_REVERSE && toggle_reverse_flow[0]) {
						toggle_reverse_flow[0] = false;
					}
					if ((flow_direction & FD_B_MASK) == FD_B_REVERSE && !toggle_reverse_flow[1]) {// Phase A power in reverse
						toggle_reverse_flow[1] = true;
						reverse_flow_tap[1] = tap[1];
						new_reverse_flow_action[1] = true;
					} else if ((flow_direction & FD_B_MASK) != FD_B_REVERSE && toggle_reverse_flow[1]) {
						toggle_reverse_flow[1] = false;
					}
					if ((flow_direction & FD_C_MASK) == FD_C_REVERSE && !toggle_reverse_flow[2]) {// Phase A power in reverse
						toggle_reverse_flow[2] = true;
						reverse_flow_tap[2] = tap[2];
						new_reverse_flow_action[2] = true;
					} else if ((flow_direction & FD_C_MASK) != FD_C_REVERSE && toggle_reverse_flow[2]) {
						toggle_reverse_flow[2] = false;
					}
				} else if (pConfig->control_level == pConfig->BANK) {
					if (((flow_direction & FD_A_MASK) == FD_A_REVERSE || (flow_direction & FD_B_MASK) == FD_B_REVERSE || (flow_direction & FD_C_MASK) == FD_C_REVERSE) && !toggle_reverse_flow_banked) {// Phase A power in reverse
						toggle_reverse_flow_banked = true;
						reverse_flow_tap[0] = tap[0];
						reverse_flow_tap[1] = tap[1];
						reverse_flow_tap[2] = tap[2];
						new_reverse_flow_action[0] = true;
						new_reverse_flow_action[1] = true;
						new_reverse_flow_action[2] = true;
					} else if (((flow_direction & FD_A_MASK) != FD_A_REVERSE && (flow_direction & FD_B_MASK) != FD_B_REVERSE && (flow_direction & FD_C_MASK) != FD_C_REVERSE) && toggle_reverse_flow_banked) {// Phase A power in reverse
						toggle_reverse_flow_banked = false;
					}
				}
			}

			for (i=0; i<3; i++)
			{
				if (first_run_flag[i] < 1)
					return curr_time_value;
				if (dwell_flag[i] == 1 && mech_flag[i] == 1)
				{			
					if (check_voltage[i].Mag() < Vlow && tap[i] != pConfig->lower_taps && !new_reverse_flow_action[i]) {
						if (pConfig->control_level == pConfig->INDIVIDUAL && !toggle_reverse_flow[i]) {
							return curr_time_value;
						} else if (pConfig->control_level == pConfig->BANK && !toggle_reverse_flow_banked) {
							return curr_time_value;
						}
					}

					if (check_voltage[i].Mag() > Vhigh && tap[i] != -pConfig->raise_taps && !new_reverse_flow_action[i]) {
						if (pConfig->control_level == pConfig->INDIVIDUAL && !toggle_reverse_flow[i]) {
							return curr_time_value;
						} else if (pConfig->control_level == pConfig->BANK && !toggle_reverse_flow_banked) {
							return curr_time_value;
						}
					}
				}
				if (new_reverse_flow_action[i] && (toggle_reverse_flow[i] || toggle_reverse_flow_banked))
					return curr_time_value;
			}
		}
	}

	//If we made it this far, just exit "like normal"
	return 0.0;
}

//Function to get the voltages of interest
void regulator::get_monitored_voltage()
{
	regulator_configuration *pConfig = OBJECTDATA(configuration, regulator_configuration);

	int testval = (int)(pConfig->Control);
	switch (testval)
	{
		case 4: //Line Drop Compensation
		{
			if (pConfig->control_level == pConfig->INDIVIDUAL)
			{
				volt[0] = ToNode_voltage[0]->get_complex();
				volt[1] = ToNode_voltage[1]->get_complex();
				volt[2] = ToNode_voltage[2]->get_complex();

				for (int i = 0; i < 3; i++) 
					V2[i] = volt[i] / ((double) pConfig->PT_ratio);

				if ((double) pConfig->CT_ratio != 0.0)
				{
					//Calculate outgoing currents
					gld::complex tmp_mat2[3][3];
					inverse(d_mat,tmp_mat2);

					curr[0] = tmp_mat2[0][0]*current_in[0]+tmp_mat2[0][1]*current_in[1]+tmp_mat2[0][2]*current_in[2];
					curr[1] = tmp_mat2[1][0]*current_in[0]+tmp_mat2[1][1]*current_in[1]+tmp_mat2[1][2]*current_in[2];
					curr[2] = tmp_mat2[2][0]*current_in[0]+tmp_mat2[2][1]*current_in[1]+tmp_mat2[2][2]*current_in[2];
				
					for (int i = 0; i < 3; i++) 
						check_voltage[i] = V2[i] - (curr[i] / (double) pConfig->CT_ratio) * gld::complex(pConfig->ldc_R_V[i], pConfig->ldc_X_V[i]);
				}
				else 
				{
					for (int i = 0; i < 3; i++)
						check_voltage[i] = V2[i];
				}
			}
			else if (pConfig->control_level == pConfig->BANK)
			{
				if (pConfig->PT_phase == PHASE_A)
					volt[0] = ToNode_voltage[0]->get_complex();
				else if (pConfig->PT_phase == PHASE_B)
					volt[0] = ToNode_voltage[1]->get_complex();
				else if (pConfig->PT_phase == PHASE_C)
					volt[0] = ToNode_voltage[2]->get_complex();

				V2[0] = volt[0] / ((double) pConfig->PT_ratio);

				if ((double) pConfig->CT_ratio != 0.0)
				{
					//Calculate outgoing currents
					gld::complex tmp_mat2[3][3];
					inverse(d_mat,tmp_mat2);

					curr[0] = tmp_mat2[0][0]*current_in[0]+tmp_mat2[0][1]*current_in[1]+tmp_mat2[0][2]*current_in[2];
					curr[1] = tmp_mat2[1][0]*current_in[0]+tmp_mat2[1][1]*current_in[1]+tmp_mat2[1][2]*current_in[2];
					curr[2] = tmp_mat2[2][0]*current_in[0]+tmp_mat2[2][1]*current_in[1]+tmp_mat2[2][2]*current_in[2];

					if (pConfig->CT_phase == PHASE_A)
						check_voltage[0] = check_voltage[1] = check_voltage[2] = V2[0] - (curr[0] / (double) pConfig->CT_ratio) * gld::complex(pConfig->ldc_R_V[0], pConfig->ldc_X_V[0]);
	
					else if (pConfig->CT_phase == PHASE_B)
						check_voltage[0] = check_voltage[1] = check_voltage[2] = V2[0] - (curr[1] / (double) pConfig->CT_ratio) * gld::complex(pConfig->ldc_R_V[1], pConfig->ldc_X_V[1]);
	
					else if (pConfig->CT_phase == PHASE_C)
						check_voltage[0] = check_voltage[1] = check_voltage[2] = V2[0] - (curr[2] / (double) pConfig->CT_ratio) * gld::complex(pConfig->ldc_R_V[2], pConfig->ldc_X_V[2]);
	
				}
				else 
				{
					check_voltage[0] = check_voltage[1] = check_voltage[2] = V2[0];
				}
			}
		}
			break;
		case 2: //Output voltage
		{
			if (pConfig->control_level == pConfig->INDIVIDUAL)
			{
				check_voltage[0] = ToNode_voltage[0]->get_complex();
				check_voltage[1] = ToNode_voltage[1]->get_complex();
				check_voltage[2] = ToNode_voltage[2]->get_complex();
			}
			else if (pConfig->control_level == pConfig->BANK)
			{
				if (pConfig->PT_phase == PHASE_A)
					check_voltage[0] = check_voltage[1] = check_voltage[2] = ToNode_voltage[0]->get_complex();
				else if (pConfig->PT_phase == PHASE_B)
					check_voltage[0] = check_voltage[1] = check_voltage[2] = ToNode_voltage[1]->get_complex();
				else if (pConfig->PT_phase == PHASE_C)
					check_voltage[0] = check_voltage[1] = check_voltage[2] = ToNode_voltage[2]->get_complex();
			}
		}
			break;
		case 3: //Remote Node
		{
			if (msgmode == msg_INTERNAL)

			{
				if (pConfig->control_level == pConfig->INDIVIDUAL)
				{

					for (int i = 0; i < 3; i++)
					{
						check_voltage[i] = RNode_voltage[i]->get_complex();
		//				gl_warning("check_voltage %f",check_voltage[i]);
		//				gl_warning("Regulator:%s regulator_resistance has been set to zero. This will result singular matrix. Setting to the global default.",obj->name);
					}
				}
				else if (pConfig->control_level == pConfig->BANK)
				{
					if (pConfig->PT_phase == PHASE_A)
						check_voltage[0] = check_voltage[1] = check_voltage[2] = RNode_voltage[0]->get_complex();
					else if (pConfig->PT_phase == PHASE_B)
						check_voltage[0] = check_voltage[1] = check_voltage[2] = RNode_voltage[1]->get_complex();
					else if (pConfig->PT_phase == PHASE_C)
						check_voltage[0] = check_voltage[1] = check_voltage[2] = RNode_voltage[2]->get_complex();
				}
			}
		}
			break;
		default:
			break;

	}
}

int regulator::kmldata(int (*stream)(const char*,...))
{
	int phase[3] = {has_phase(PHASE_A),has_phase(PHASE_B),has_phase(PHASE_C)};

	// tap position
	stream("<TR><TH ALIGN=LEFT>Tap position</TH>");
	for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
	{
		if ( phase[i] )
			stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\"><NOBR>%d</NOBR></TD>", tap[i]);
		else
			stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\">&mdash;</TD>");
	}
	stream("</TR>\n");

	// control input
	gld_global run_realtime("run_realtime");
	gld_global server("hostname");
	gld_global port("server_portnum");
	if ( run_realtime.get_bool() )
	{
		stream("<TR><TH ALIGN=LEFT>Raise to</TH>");
		for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
		{
			if ( phase[i] )
				stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\"><FORM ACTION=\"http://%s:%d/kml/%s\" METHOD=GET><INPUT TYPE=SUBMIT NAME=\"tap_%c\" VALUE=\"%d\" /></FORM></TD>",
						(const char*)server.get_string(), port.get_int16(), (const char*)get_name(), 'A'+i, tap[i]+1);
			else
				stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\">&mdash;</TD>");
		}
		stream("</TR>\n");
		stream("<TR><TH ALIGN=LEFT>Lower to</TH>");
		for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
		{
			if ( phase[i] )
				stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\"><FORM ACTION=\"http://%s:%d/kml/%s\" METHOD=GET><INPUT TYPE=SUBMIT NAME=\"tap_%c\" VALUE=\"%d\" /></FORM></TD>",
						(const char*)server.get_string(), port.get_int16(), (const char*)get_name(), 'A'+i, tap[i]-1);
			else
				stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\">&mdash;</TD>");
		}
		stream("</TR>\n");
	}
	return 2;
}

//Module-level deltamode call
SIMULATIONMODE regulator::inter_deltaupdate_regulator(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//OBJECT *hdr = OBJECTHDR(this);
	double curr_time_value;	//Current time of simulation
	double temp_time;
	regulator_configuration *pConfig = OBJECTDATA(configuration, regulator_configuration);

	//Get the current time
	curr_time_value = gl_globaldeltaclock;

	if (!interupdate_pos)	//Before powerflow call
	{
		//Replicate presync behavior
		//Toggle the iteration variable -- only for voltage-type adjustments (since it's in presync now)
		if ((pConfig->Control == pConfig->OUTPUT_VOLTAGE) || (pConfig->Control == pConfig->REMOTE_NODE))
			iteration_flag = !iteration_flag;

		//Call the pre-presync regulator code
		reg_prePre_fxn(curr_time_value);

		//Link sync stuff
		NR_link_sync_fxn();
		
		//Call the post-presync regulator code
		reg_postPre_fxn();

		//Force a "reiteration" if we're checking voltage - consequence of this previously being in true pass of NR
		if (((pConfig->Control == pConfig->OUTPUT_VOLTAGE) || (pConfig->Control == pConfig->REMOTE_NODE)) && !iteration_flag)
		{
			deltamode_reiter_request = true;	//Flag us for a reiter
		}

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Call postsync
		BOTH_link_postsync_fxn();

		//Call the regulator-specific post-postsync function
		temp_time = reg_postPost_fxn(curr_time_value);

		//Make sure it wasn't an error
		if (temp_time == -1.0)
		{
			return SM_ERROR;
		}
		else if ((temp_time == curr_time_value) || deltamode_reiter_request)	//See if this requested a reiter, or if above did
		{
			//Clear the flag, regardless
			deltamode_reiter_request = false;

			//Ask for a reiteration
			return SM_DELTA_ITER;
		}
		//Otherwise, it was a proceed forward -- probably returned a future state time

		//In-rush handling would go here, but regulator has no in-rush capabilities

		return SM_EVENT;	//Always prompt for an exit
	}
}//End module deltamode

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: regulator
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/



/* This can be added back in after tape has been moved to commit
EXPORT TIMESTAMP commit_regulator(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	if (solver_method==SM_FBS)
	{
		regulator *plink = OBJECTDATA(obj,regulator);
		plink->calculate_power();
	}
	return TS_NEVER;
}
*/
EXPORT int create_regulator(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(regulator::oclass);
		if (*obj!=nullptr)
		{
			regulator *my = OBJECTDATA(*obj,regulator);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(regulator);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_regulator(OBJECT *obj)
{
	try {
		regulator *my = OBJECTDATA(obj,regulator);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(regulator);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_regulator(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		regulator *pObj = OBJECTDATA(obj,regulator);
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
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
	SYNC_CATCHALL(regulator);
}

EXPORT int isa_regulator(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,regulator)->isa(classname);
}

//Export for deltamode
EXPORT SIMULATIONMODE interupdate_regulator(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	regulator *my = OBJECTDATA(obj,regulator);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_regulator(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_regulator(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//KML Export
EXPORT int regulator_kmldata(OBJECT *obj,int (*stream)(const char*,...))
{
	regulator *n = OBJECTDATA(obj, regulator);
	int rv = 1;

	rv = n->kmldata(stream);

	return rv;
}

/**@}*/

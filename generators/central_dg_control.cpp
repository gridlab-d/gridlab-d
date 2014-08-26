/** $Id: central_dg_control.cpp,v 1.0 2013/10/10 
	Copyright (C) 2008 Battelle Memorial Institute
	@file central_dg_control.cpp
	@defgroup central_dg_control
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"
#include "central_dg_control.h"

#define DEFAULT 1.0;

CLASS *central_dg_control::oclass = NULL;
central_dg_control *central_dg_control::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
central_dg_control::central_dg_control(MODULE *module)
{	
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"central_dg_control",sizeof(central_dg_control),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class central_dg_control";
		else
			oclass->trl = TRL_PROOF;
		
		if (gl_publish_variable(oclass,
			PT_char32,"controlled_dgs", PADDR(controlled_objects), PT_DESCRIPTION, "the group ID of the dg objects the controller controls.",
			PT_object,"feederhead_meter", PADDR(feederhead_meter), PT_DESCRIPTION, "the name of the meter.",

			PT_enumeration,"control_mode_0",PADDR(control_mode_setting[0]),
				PT_KEYWORD,"NO_CONTROL",(enumeration)NO_CONTROL,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"PEAK_SHAVING",(enumeration)PEAK_SHAVING,
			PT_enumeration,"control_mode_1",PADDR(control_mode_setting[1]),
				PT_KEYWORD,"NO_CONTROL",(enumeration)NO_CONTROL,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"PEAK_SHAVING",(enumeration)PEAK_SHAVING,
			PT_enumeration,"control_mode_2",PADDR(control_mode_setting[2]),
				PT_KEYWORD,"NO_CONTROL",(enumeration)NO_CONTROL,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"PEAK_SHAVING",(enumeration)PEAK_SHAVING,
			PT_enumeration,"control_mode_3",PADDR(control_mode_setting[3]),
				PT_KEYWORD,"NO_CONTROL",(enumeration)NO_CONTROL,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"PEAK_SHAVING",(enumeration)PEAK_SHAVING,

			
			
			PT_double, "peak_S[W]", PADDR(S_peak),
			PT_double, "pf_low[unit]", PADDR(pf_low),
			PT_double, "pf_high[unit]", PADDR(pf_high),

			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

			defaults = this;

			memset(this,0,sizeof(central_dg_control));


	}
}
/* Object creation is called once for each object that is created by the core */
int central_dg_control::create(void) 
{
	// Default values for Inverter object.
	control_mode_setting[0] = NO_CONTROL;
	control_mode_setting[1] = control_mode_setting[2] = control_mode_setting[3] = NO_SETTING;
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int central_dg_control::init(OBJECT *parent)
{
	FINDLIST *inverter_list;
	FINDLIST *battery_list;
	FINDLIST *solar_list;
	int index = 0;
	OBJECT *obj = 0;
	all_inverter_S_rated = 0;
	all_battery_S_rated = 0;
	all_solar_S_rated = 0;
	int inverter_filled_to = -1;

	
	//////////////////////////////////////////////////////////////////////////
	// Assemble object maps
	//////////////////////////////////////////////////////////////////////////
	if(controlled_objects[0] == '\0'){
		gl_error("No group id given for controlled DG objects.");
		return 0;
		
	}
	//Find all inverters with controller group id
	inverter_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"inverter",AND,FT_GROUPID,SAME,controlled_objects.get_string(),FT_END);
	if(inverter_list == NULL){
		gl_error("No inverters with given group id found.");
		/*  TROUBLESHOOT
		While trying to put together a list of all inverter objects with the specified controller groupid, no such inverter objects were found.
		*/
		
		return 0;
	}
	//Find all batteries whose parents are inverters with controller group id
	battery_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"battery",AND,FT_PARENT,FT_CLASS,SAME,"inverter",AND,FT_PARENT,FT_GROUPID,SAME,controlled_objects.get_string(),FT_END);
	if(battery_list == NULL){
		gl_error("No batteries with inverter parents with given group id found.");
		/*  TROUBLESHOOT
		While trying to put together a list of all battery objects with parent inverter objects with the specified controller groupid, no such battery objects were found.
		*/
		return 0;
	}
	//Find all solars whose parents are inverters with controller group id
	solar_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"solar",AND,FT_PARENT,FT_CLASS,SAME,"inverter",AND,FT_PARENT,FT_GROUPID,SAME,controlled_objects.get_string(),FT_END);
	if(solar_list == NULL){
		gl_error("no solars with inverter parents with given group id found.");
		/*  TROUBLESHOOT
		While trying to put together a list of all solar objects with parent inverter objects with the specified controller groupid, no such solar objects were found.
		*/
		return 0;
	}

	//Allocate pointer array for all inverters which for now includes those with battery and solar children
	inverter_set = (inverter **)gl_malloc((battery_list->hit_count*sizeof(battery*))+(solar_list->hit_count*sizeof(solar*)));
	if(inverter_set == NULL){
		gl_error("Failed to allocate inverter array.");
		/*  TROUBLESHOOT
		While trying to allocate the array of pointers to the controlled inverters, the pointer array came back null.
		*/
		return 0;
	}
	//Allocate battery pointer array
	battery_set = (battery **)gl_malloc(battery_list->hit_count*sizeof(battery*));
	if(battery_set == NULL){
		gl_error("Failed to allocate battery array.");
		/*  TROUBLESHOOT
		While trying to allocate the array of pointers to the controlled batteries, the pointer array came back null.		
		*/
		return 0;
	}
	//Allocate solar pointer array
	solar_set = (solar **)gl_malloc(solar_list->hit_count*sizeof(solar*));
	if(solar_set == NULL){
		gl_error("Failed to allocate solar array.");
		/*  TROUBLESHOOT
		While trying to allocate the array of pointers to the controlled solars, the pointer array came back null.
		*/
		return 0;
	}
	//Allocate pointer array for inverters with battery children
	battery_inverter_set = (inverter ***)gl_malloc(battery_list->hit_count*sizeof(inverter**));
	if(battery_inverter_set == NULL){
		gl_error("Failed to allocate battery array.");
		/*  TROUBLESHOOT
		While trying to allocate the array of pointers to the controlled inverters with battery children, the pointer array came back null.
		*/
		return 0;
	}
	//Allocate pointer array for inverters with solar children
	solar_inverter_set = (inverter ***)gl_malloc(solar_list->hit_count*sizeof(inverter**));
	if(solar_inverter_set == NULL){
		gl_error("Failed to allocate solar array.");
		/*  TROUBLESHOOT
		While trying to allocate the array of pointers to the controlled inverters with solar children, the pointer array came back null.
		*/
		return 0;
	}

	battery_count = battery_list->hit_count;
	solar_count = solar_list->hit_count;
	inverter_count = battery_count + solar_count;

	//Fill in addresses for pointer arrays relating to batteries using the battery findlist
	while(obj = gl_find_next(battery_list,obj)){
		if(index >= battery_count){
			break;
		}
		battery_set[index] = OBJECTDATA(obj,battery);
		if(battery_set[index] == NULL){
			gl_error("Unable to map object as battery.");
			/*  TROUBLESHOOT
			While trying to map a battery from the list as a battery object, a null pointer was returned.
			*/
			return 0;
		}
		inverter_set[inverter_filled_to + 1] = OBJECTDATA(obj->parent, inverter);
		if(inverter_set[inverter_filled_to + 1] == NULL){
			gl_error("Unable to map object as inverter.");
			/*  TROUBLESHOOT
			While trying to map an inverter from the list as an inveter object, a null pointer was returned.
			*/
			return 0;
		}
		inverter_filled_to++;
		battery_inverter_set[index] = &inverter_set[inverter_filled_to];
		if (battery_inverter_set[index] == NULL) {
			gl_error("Unable to map battery parent object as inverter.");
			/*  TROUBLESHOOT
			While trying to map an inverter from the listof inverters with battery children as an inverter object, a null pointer was returned.
			*/
			return 0;
		}
		//Aggregate (three-phase) battery inverter rated complex power
		all_battery_S_rated += (*(battery_inverter_set[index]))->bp_rated;
		++index;
	}
	
	//Fill in addresses for pointer arrays relating to solars using the solar findlist
	index = 0;
	while(obj = gl_find_next(solar_list,obj)){
		if(index >= solar_count){
			break;
		}
		solar_set[index] = OBJECTDATA(obj,solar);
		if(solar_set[index] == NULL){
			gl_error("Unable to map object as solar.");
			/*  TROUBLESHOOT
			While trying to map a solar from the list as a solar object, a null pointer was returned.
			*/
			return 0;
		}
		inverter_set[inverter_filled_to + 1] = OBJECTDATA(obj->parent, inverter);
		if(inverter_set[inverter_filled_to + 1] == NULL){
			gl_error("Unable to map object as inverter.");
			/*  TROUBLESHOOT
			While trying to map an inverter from the listof inverters an inverter object, a null pointer was returned.
			*/
			return 0;
		}
		inverter_filled_to++;
		solar_inverter_set[index] = &inverter_set[inverter_filled_to];
		if (solar_inverter_set[index] == NULL) {
			gl_error("Unable to map solar parent object as inverter.");
			/*  TROUBLESHOOT
			While trying to map an inverter from the listof inverters with solar children as an inverter object, a null pointer was returned.
			*/
			return 0;
		}
		//Aggregate (three-phase) solar inverter rated complex power
		all_solar_S_rated += solar_set[index]->Rated_kVA*1000.0;
		++index;
	}

	all_inverter_S_rated = all_solar_S_rated + all_battery_S_rated;

	


	P_disp_3p = 0.0;
	Q_disp_3p = 0.0;
	return 1;
	
}

TIMESTAMP central_dg_control::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	int i;
	//After contingency control actions have been taken and the network returns to 'normal' state, 
	//the inverter reference power or power factor values should return to their original values or their current 
	//scheduled values if a schedule is given. In the case of a schedule, the player values will be
	//played in during presync so no action need be taken. However, if only initial values are given,
	//they must be reassigned. This is done at each time step here in presync before any controller action
	//(in sync) may change these values.
	if(t0!=t1) {
		for(i = 0; i < inverter_count; i++)
		{
			inverter_set[i]->P_Out = inverter_set[i]->P_Out_t0;
			inverter_set[i]->Q_Out = inverter_set[i]->Q_Out_t0;
			inverter_set[i]->power_factor = inverter_set[i]->power_factor_t0;
		}
	}
		
	TIMESTAMP t2 = TS_NEVER;
	return t2; 
}

TIMESTAMP central_dg_control::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	//Need information on power flow for this time step so let it run once 
	//without any central control and then reiterate

	if (t0!=t1) {
		return t1;
	}
	int i;
	int n;
	
	complex *complex_power[3];
	complex_power[0] = gl_get_complex_by_name(feederhead_meter,"measured_power_A");
	complex_power[1] = gl_get_complex_by_name(feederhead_meter,"measured_power_B");
	complex_power[2] = gl_get_complex_by_name(feederhead_meter,"measured_power_C");
	P[0] = complex_power[0]->Re();
	P[1] = complex_power[1]->Re();
	P[2] = complex_power[2]->Re();
	Q[0] = complex_power[0]->Im();
	Q[1] = complex_power[1]->Im();
	Q[2] = complex_power[2]->Im();
	P_3p = P[0] + P[1] + P[2];
	Q_3p = Q[0] + Q[1] + Q[2];
	S_3p = complex(P_3p, Q_3p);
	double potential_pf = 0.0;
	double Q_disp_so_far = 0.0;
	double total_avail_soc = 0.0;
	double P_ref_3p;
	double Q_avail_3p = 0.0;
	double this_Q;
	double P_disp_3p_no_control = 0;
	double Q_disp_3p_no_control = 0;

	//The target power factor is the midpoint of the two values specifying the allowable band. However, due to the 
	//discontinuous/nonlinear nature of the signed power factor used in GridLAB-D, calculation of the midpoint is 
	//easiest by calculating the corresponding power factor angles.
	double pf_angle_goal = ((pf_low/fabs(pf_low))*acos(fabs(pf_low)) + (pf_high/fabs(pf_high))*acos(fabs(pf_high)))/2.0;
	double pf_goal;
	//Assign power factors using calculated goal and correct sign.
	if (pf_angle_goal < 0)
	{
		pf_goal = -cos(pf_angle_goal);
	}
	else 
	{
		pf_goal = cos(pf_angle_goal);
	}

	P_gen[0] = P_gen[1] = P_gen[2] = 0.0;
	Q_gen[0] = Q_gen[1] = Q_gen[2] = 0.0;
	//Calculate the real and reactive power dispatched to the controlled inverters when
	//no central control is used.
	for(i = 0; i < inverter_count; i++)
	{
		P_disp_3p_no_control += inverter_set[i]->P_Out;
		Q_disp_3p_no_control += inverter_set[i]->Q_Out;
	}

	
	P_gen_solar[0] = P_gen_solar[1] = P_gen_solar[2] = P_gen_solar_3p = 0.0;
	Q_gen_solar[0] = Q_gen_solar[1] = Q_gen_solar[2] = Q_gen_solar_3p =0.0;
	//Calculate solar inverter power output for this time step.
	//This will be either after the first iteration where no control was present
	//or after one iteration has run.
	for(i = 0; i < solar_count; i++)
	{
		P_gen_solar_3p += (*(solar_inverter_set[i]))->VA_Out.Re();
		Q_gen_solar_3p += (*(solar_inverter_set[i]))->VA_Out.Im();
	}

	
	P_gen_battery[0] = P_gen_battery[1] = P_gen_battery[2] = P_gen_battery_3p = 0.0;
	Q_gen_battery[0] = Q_gen_battery[1] = Q_gen_battery[2] = Q_gen_battery_3p = 0.0;
	//Calculate battery inverter power output for this time step.
	for(i = 0; i < battery_count; i++)
	{
		P_gen_battery_3p += (*(battery_inverter_set[i]))->VA_Out.Re();
		Q_gen_battery_3p += (*(battery_inverter_set[i]))->VA_Out.Im();
	}

	P_gen_3p = P_gen_solar_3p + P_gen_battery_3p;
	Q_gen_3p = Q_gen_solar_3p + Q_gen_battery_3p;

	//There are four control mode slots. The highest rank one is checked first.
	//If controller action is taken, t1 is returned to force the powerflow to reiterate
	//before checking lower rank control modes. If controller action is not taken, the next
	//lowest rank mode is checked. After all have been checked with no action taken, the
	//controller returns TS_NEVER indicating it is ok to move on.
	i=3;
	while (i>-1)
	{
		if (control_mode_setting[i]!=NULL)
		{
			switch(control_mode_setting[i])
			{
				//If we get here, exit the loop.
				case NO_CONTROL:
					break;
				//Control mode to maintain power factor measured at feederhead
				case CONSTANT_PF:
					//Calculate measured power factor at feederhead. Note this is signed power factor. The magnitude is 
					//equal to |P|/|S|. The sign is positive where the Q is positive (from a load perspective, i.e. Q is 
					//being consumed). For cases where Q is 0, a power factor of positive 1 is assigned.
					pf_meas_3p = (Q_3p == 0 ? 1.0 : Q_3p)/fabs(Q_3p == 0 ? 1.0 : Q_3p)*(S_3p.Mag() == 0 ? 1.0 : fabs(P_3p))/(S_3p.Mag() == 0 ? 1.0 : S_3p.Mag());

					//Due to diabolical confusing nature of signed power factor, many cases must be used to correctly handle
					//the various combinations of power factor limit and measurement cases. If power factor is outside the limit
					//the necessary Q dispatch to bring power factor (close) to the midpoint goal is calculated. Otherwise, the 
					//control mode exits.
					if (pf_low > 0.0) {
						if (pf_meas_3p < 0.0||(pf_meas_3p > 0.0 && pf_meas_3p > pf_low)) {
							Q_disp_3p = Q_3p + Q_gen_3p - fabs(P_3p)*tan(acos(pf_goal));
						}
						else if (pf_meas_3p < pf_high) {
							Q_disp_3p = Q_3p + Q_gen_3p - fabs(P_3p)*tan(acos(pf_goal));
						}
						//Power factor within limits.
						else {
							break;
						}
					}
					else if (pf_high > 0.0) {
						if (pf_meas_3p < 0.0 && pf_meas_3p > pf_low) {
							Q_disp_3p = Q_3p + Q_gen_3p - fabs(P_3p)*tan(acos(pf_goal));
						}
						else if (pf_meas_3p >= 0.0 && pf_meas_3p < pf_high) {
							Q_disp_3p = Q_3p + Q_gen_3p - fabs(P_3p)*tan(acos(pf_goal));
						}
						//Power factor within limits
						else {
							break;
						}
					}
					//limits must both be negative
					else { 
						if (pf_meas_3p < 0.0 && pf_low < pf_meas_3p) {
							Q_disp_3p = Q_3p + Q_gen_3p - fabs(P_3p)*tan(acos(pf_goal));
						}
						else if ((pf_meas_3p < 0.0 && pf_meas_3p < pf_high)||(pf_meas_3p > 0)) {
							Q_disp_3p = Q_3p + Q_gen_3p - fabs(P_3p)*tan(acos(pf_goal));
						}
						//Power factor within limits
						else {
							break;
						}
					}


					//Determine Dispatch
					
					//If we're still here then we've come up with a total Q dispatch to correct power factor.
					//Now we need to allocate the Q. For starters, look at total inverter rated S and current
					//real power output to get total available Q capacity.

					//Add battery available Q.
					Q_avail_3p = all_battery_S_rated*sin(acos(P_gen_battery_3p/all_battery_S_rated));

					//Things will get confusing if we try to allocate Q to solar inverters with no P output who operate in constant PF mode so 
					//for now we'll just not allow Q dispatch to solar inverters unless their solars are producing. Otherwise, add in the available
					//Q capacity.
					for (n=0; n< solar_count; n++) {
						if ((*(solar_inverter_set[n]))->VA_Out.Re() > 0.0) {
							Q_avail_3p +=(*(solar_inverter_set[n]))->p_rated*3.0*sin(acos((*(solar_inverter_set[n]))->VA_Out.Re()/((*(solar_inverter_set[n]))->p_rated*3.0)));
						}
					
					}
				
					//Can we meet the Q dispatch with our capacity? If yes allocate the whole dispatch.
					if (fabs(Q_avail_3p) >= fabs(Q_disp_3p)) {
						for (n=0; n < inverter_count; n++) {
							//Dispatch to solar inverters (those in constant PF mode) who are currently producing some real power.
							if ((inverter_set[n])->four_quadrant_control_mode==2 && (inverter_set[n])->VA_Out.Re() > 0.0) {
								//We need to deliver a power factor signal, so first calculate Q and then corresponding power factor.
								//Q dispatch portion calculated using ratio of this inverter's capacity factor to total capacity factor.
								this_Q = (inverter_set[n])->p_rated*3.0*sin(acos((inverter_set[n])->VA_Out.Re()/((inverter_set[n])->p_rated)*3.0))/Q_avail_3p*Q_disp_3p;
								//Calculate and correctly sign corresponding power factor.
								(inverter_set[n])->power_factor = -(this_Q/fabs(this_Q))*fabs((inverter_set[n])->VA_Out.Re())/complex((inverter_set[n])->VA_Out.Re(),this_Q).Mag();
							}
							//Dispatch to battery inverters (those in constant PQ mode)
							else if ((inverter_set[n])->four_quadrant_control_mode==1)
							{
								//Q dispatch portion calculated using ratio of this inverter's capacity factor to total capacity factor.
								(inverter_set[n])->Q_Out = (inverter_set[n])->p_rated*3.0*sin(acos((inverter_set[n])->VA_Out.Re()/((inverter_set[n])->p_rated)*3.0))/Q_avail_3p*Q_disp_3p;
							}
						}
					//Same concept as above but we'll only dispatch Q to max out inverter ratings (do the best we can)
					} else {
						for (n=0; n < inverter_count; n++) {
							
							if ((inverter_set[n])->four_quadrant_control_mode==2 && (inverter_set[n])->VA_Out.Re() > 0.0) {
								//This inverter QRef = (This inverter available Q/total Available Q)*Q to be dispatched
								this_Q = (inverter_set[n])->p_rated*3.0*sin(acos((inverter_set[n])->VA_Out.Re()/((inverter_set[n])->p_rated)*3.0));
								(inverter_set[n])->power_factor = -(this_Q/fabs(this_Q))*fabs((inverter_set[n])->VA_Out.Re())/complex((inverter_set[n])->VA_Out.Re(),this_Q).Mag();
							}
							else if ((inverter_set[n])->four_quadrant_control_mode==1)
							{
								//This inverter QRef = (This inverter available Q/total Available Q)*Q to be dispatched
								(inverter_set[n])->Q_Out = (inverter_set[n])->p_rated*3.0*sin(acos((inverter_set[n])->VA_Out.Re()/((inverter_set[n])->p_rated)*3.0));
							}
						}

					}
					
					//Reiterate to make sure it worked and also check lower control modes.
					return t1;
					break;
				//Control mode to keep real power below peak value.
				case PEAK_SHAVING:
					if ((S_3p-P_disp_3p_no_control).Mag() > S_peak) {
						P_ref_3p = S_peak*cos(asin(Q_3p/S_peak));
						P_disp_3p = P_3p - P_ref_3p;
							for (n=0; n<battery_count; n++) {
								total_avail_soc += (battery_set[n])->soc;
							}
							for (n=0; n<battery_count; n++) {
								(*(battery_inverter_set[n]))->P_Out = battery_set[n]->soc/total_avail_soc*P_disp_3p;
							}
							//Reiterate to make sure it worked and also check lower control modes.
							return t1;
					}
					break;
			}

		}
		i--;
	}


	
	return TS_NEVER;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP central_dg_control::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;		//By default, we're done forever!
	
	
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}






double central_dg_control::fmin(double a, double b)
{
	if (a < b)
		return a;
	else
		return b;
}
double central_dg_control::fmin(double a, double b, double c)
{
	if (a < b && a < c)
		return a;
	else if (b < c)
		return b;
	else
		return c;
		
}
double central_dg_control::fmax(double a, double b)
{
	if (a > b)
		return a;
	else
		return b;
}
double central_dg_control::fmax(double a, double b, double c)
{
	if (a > b && a > c)
		return a;
	else if (b > c)
		return b;
	else
		return c;
		
}


//Retrieves the pointer for a double variable from another object
double *central_dg_control::get_double(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
}
bool *central_dg_control::get_bool(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_bool)
		return NULL;
	return (bool*)GETADDR(obj,p);
}
int *central_dg_control::get_enum(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_enumeration)
		return NULL;
	return (int*)GETADDR(obj,p);
}
complex * central_dg_control::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}
//Function to perform exp(j*val)
//Basically a complex rotation
complex central_dg_control::complex_exp(double angle)
{
	complex output_val;

	//exp(jx) = cos(x)+j*sin(x)
	output_val = complex(cos(angle),sin(angle));

	return output_val;
}

	
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_central_dg_control(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(central_dg_control::oclass);
		if (*obj!=NULL)
		{
			central_dg_control *my = OBJECTDATA(*obj,central_dg_control);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(central_dg_control);
}

EXPORT int init_central_dg_control(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,central_dg_control)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(central_dg_control);
}

EXPORT TIMESTAMP sync_central_dg_control(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	central_dg_control *my = OBJECTDATA(obj,central_dg_control);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;		
	}
	SYNC_CATCHALL(central_dg_control);
	return t2;
}

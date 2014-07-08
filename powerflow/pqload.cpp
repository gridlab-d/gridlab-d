/** $Id: pqload.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file pqload.cpp
	@addtogroup pqload
	@ingroup node

	Load objects represent static loads and export both voltages and current.

	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "node.h"
#include "meter.h"
#include "pqload.h"

SCHED_LIST *new_slist(){
	SCHED_LIST *l = (SCHED_LIST *)gl_malloc(sizeof(SCHED_LIST));
	memset(l, 0, sizeof(SCHED_LIST));
	return l;
}

int delete_slist(SCHED_LIST *l){
	int rv = 0;
	if(l == NULL){
		return -1; /* error */
	}
	if(l->next != NULL){
		rv = delete_slist(l->next); /* shouldn't be non-negative but check anyway */
	}
	gl_free(l);
	return rv;
}

CLASS *pqload::oclass = NULL;
CLASS *pqload::pclass = NULL;
pqload *pqload::defaults = NULL;
double pqload::zero_F = -459.67; /* absolute zero, give or take */

pqload::pqload(MODULE *mod) : load(mod)
{
	if(oclass == NULL){
		pclass = load::oclass;

		oclass = gl_register_class(mod, "pqload", sizeof(pqload), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class pqload";
		else
			oclass->trl = TRL_PROTOTYPE;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "load",
			PT_object, "weather", PADDR(weather),
			PT_double, "T_nominal[degF]", PADDR(temp_nom),
			PT_double, "Zp_T[ohm/degF]", PADDR(imped_p[0]),
			PT_double, "Zp_H[ohm/%]", PADDR(imped_p[1]),
			PT_double, "Zp_S[ohm*h/Btu]", PADDR(imped_p[2]),
			PT_double, "Zp_W[ohm/mph]", PADDR(imped_p[3]),
			PT_double, "Zp_R[ohm*h/in]", PADDR(imped_p[4]),
			PT_double, "Zp[ohm]", PADDR(imped_p[5]),
			PT_double, "Zq_T[F/degF]", PADDR(imped_q[0]),
			PT_double, "Zq_H[F/%]", PADDR(imped_q[1]),
			PT_double, "Zq_S[F*h/Btu]", PADDR(imped_q[2]),
			PT_double, "Zq_W[F/mph]", PADDR(imped_q[3]),
			PT_double, "Zq_R[F*h/in]", PADDR(imped_q[4]),
			PT_double, "Zq[F]", PADDR(imped_q[5]),
			PT_double, "Im_T[A/degF]", PADDR(current_m[0]),
			PT_double, "Im_H[A/%]", PADDR(current_m[1]),
			PT_double, "Im_S[A*h/Btu]", PADDR(current_m[2]),
			PT_double, "Im_W[A/mph]", PADDR(current_m[3]),
			PT_double, "Im_R[A*h/in]", PADDR(current_m[4]),
			PT_double, "Im[A]", PADDR(current_m[5]),
			PT_double, "Ia_T[deg/degF]", PADDR(current_a[0]),
			PT_double, "Ia_H[deg/%]", PADDR(current_a[1]),
			PT_double, "Ia_S[deg*h/Btu]", PADDR(current_a[2]),
			PT_double, "Ia_W[deg/mph]", PADDR(current_a[3]),
			PT_double, "Ia_R[deg*h/in]", PADDR(current_a[4]),
			PT_double, "Ia[deg]", PADDR(current_a[5]),
			PT_double, "Pp_T[W/degF]", PADDR(power_p[0]),
			PT_double, "Pp_H[W/%]", PADDR(power_p[1]),
			PT_double, "Pp_S[W*h/Btu]", PADDR(power_p[2]),
			PT_double, "Pp_W[W/mph]", PADDR(power_p[3]),
			PT_double, "Pp_R[W*h/in]", PADDR(power_p[4]),
			PT_double, "Pp[W]", PADDR(power_p[5]),
			PT_double, "Pq_T[VAr/degF]", PADDR(power_q[0]),
			PT_double, "Pq_H[VAr/%]", PADDR(power_q[1]),
			PT_double, "Pq_S[VAr*h/Btu]", PADDR(power_q[2]),
			PT_double, "Pq_W[VAr/mph]", PADDR(power_q[3]),
			PT_double, "Pq_R[VAr*h/in]", PADDR(power_q[4]),
			PT_double, "Pq[VAr]", PADDR(power_q[5]),
			PT_double, "input_temp[degF]", PADDR(input[0]), PT_ACCESS, PA_REFERENCE,
			PT_double, "input_humid[%]", PADDR(input[1]), PT_ACCESS, PA_REFERENCE,
			PT_double, "input_solar[Btu/h]", PADDR(input[2]), PT_ACCESS, PA_REFERENCE,
			PT_double, "input_wind[mph]", PADDR(input[3]), PT_ACCESS, PA_REFERENCE,
			PT_double, "input_rain[in/h]", PADDR(input[4]), PT_ACCESS, PA_REFERENCE,
			PT_double, "output_imped_p[Ohm]", PADDR(output[0]), PT_ACCESS, PA_REFERENCE,
			PT_double, "output_imped_q[Ohm]", PADDR(output[1]), PT_ACCESS, PA_REFERENCE,
			PT_double, "output_current_m[A]", PADDR(output[2]), PT_ACCESS, PA_REFERENCE,
			PT_double, "output_current_a[deg]", PADDR(output[3]), PT_ACCESS, PA_REFERENCE,
			PT_double, "output_power_p[W]", PADDR(output[4]), PT_ACCESS, PA_REFERENCE,
			PT_double, "output_power_q[VAr]", PADDR(output[5]), PT_ACCESS, PA_REFERENCE,
			PT_complex, "output_impedance[ohm]", PADDR(kZ), PT_ACCESS, PA_REFERENCE,
			PT_complex, "output_current[A]", PADDR(kI), PT_ACCESS, PA_REFERENCE,
			PT_complex, "output_power[VA]", PADDR(kP), PT_ACCESS, PA_REFERENCE,
			NULL) < 1){
				GL_THROW("unable to publish properties in %s",__FILE__);
		}
	defaults = this;
	memset(this,0,sizeof(pqload));
	input[5] = 1.0; /* constant term */
	//imped_p[5] = 0;
	strcpy(schedule, "* * * * *:1.0;");
	temp_nom = zero_F;
	load_class = LC_UNKNOWN;
	sched_until = TS_NEVER;
    }
}

int pqload::build_sched(char *instr = NULL, SCHED_LIST *end = NULL){
	char *sc_end = 0;
	char *start = instr ? instr : schedule.get_string(); /* doesn't copy but doesn't use strtok */
	int rv = 0, scanct = 0;
	SCHED_LIST *endptr = NULL;

	/* eat whitespace between schedules */
	while(*start == ' '){
		++start;
	}

	/* check schedule string end */
	if(*start == '\n' || *start == 0){
		return 1; /* end of schedule ~ success */
	}

	/* extend list */
	if(sched == NULL){
		sched = endptr = new_slist();
	} else {
		endptr = new_slist();
	}

	/* find bounds */
	sc_end = strchr(start, ';');
	if(sc_end == NULL){
		gl_error("schedule token \"%s\" not semicolon terminated", start);
	}
	char moh_v[257], moh_w[257], hod_v[257], hod_w[257], dom_v[257], dom_w[257], moy_v[257], moy_w[257], dow_v[257], lpu[257], sc[257];
	scanct = sscanf(start, "%256[-0-9*]%256[ \t]%256[-0-9*]%256[ \t]%256[-0-9*]%256[ \t]%256[-0-9*]%256[ \t]%256[-0-9*]:%256[0-9\\.]%[;]",
		moh_v, moh_w, hod_v, hod_w, dom_v, dom_w, moy_v, moy_w, dow_v, lpu, sc);

	/* parse the individual fields */

	rv = build_sched(sc_end + 1, endptr);
	if(rv > 0)
		return rv + 1;
	else
		return rv - 1; /* error depth */
}

int pqload::create(void)
{
	int res = 0;
	
	memcpy(this, defaults, sizeof(pqload));

	res = node::create();

    return res;
}

int pqload::init(OBJECT *parent){
	int rv = 0;
	int w_rv = 0;

	// init_weather from house_e.cpp:init_weather
	static FINDLIST *climates = NULL;
	OBJECT *hdr = OBJECTHDR(this);
	int not_found = 0;
	if (climates==NULL && not_found==0) 
	{
		climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
		if (climates==NULL)
		{
			not_found = 1;
			gl_warning("pqload: no climate data found");
		}
		else if (climates->hit_count>1)
		{
			gl_warning("pqload: %d climates found, using first one defined", climates->hit_count);
		}
	}
	if (climates!=NULL)
	{
		if (climates->hit_count==0)
		{
			//default to mock data
		}
		else //climate data was found
		{
			// force rank of object w.r.t climate
			OBJECT *obj = gl_find_next(climates,NULL);
			if(weather == NULL){
				weather = NULL;
			} else {
				if (obj->rank<=hdr->rank)
					gl_set_dependent(obj,hdr);
				weather = obj;
			}
		}
	}

	// old check
	if(weather != NULL){
		temperature = gl_get_property(weather, "temperature");
		if(temperature == NULL){
			gl_error("Unable to find temperature property in weather object!");
			++w_rv;
		}
		
		humidity = gl_get_property(weather, "humidity");
		if(humidity == NULL){
			gl_error("Unable to find humidity property in weather object!");
			++w_rv;
		}

		solar = gl_get_property(weather, "solar_flux");
		if(solar == NULL){
			gl_error("Unable to find solar_flux property in weather object!");
			++w_rv;
		}

		wind = gl_get_property(weather, "wind_speed");
		if(wind == NULL){
			gl_error("Unable to find wind_speed property in weather object!");
			++w_rv;
		}

		rain = gl_get_property(weather, "rainfall"); /* not yet implemented in climate */
		if(rain == NULL){
			gl_error("Unable to find rainfall property in weather object!");
			//++w_rv;
		}
	}

	rv = load::init(parent);

	return w_rv ? 0 : rv;
}

TIMESTAMP pqload::presync(TIMESTAMP t0)
{
	TIMESTAMP result = 0;

	//Must be at the bottom, or the new values will be calculated after the fact
	result = load::presync(t0);

	return result;
}

TIMESTAMP pqload::sync(TIMESTAMP t0)
{
	TIMESTAMP result = TS_NEVER;
	double SysFreq = 376.991118431;		//System frequency in radians/sec, nominalized 60 Hz for now.

	int i = 0;

	/* must take place in sync in order to let the climate update itself */
	if(weather != NULL){
		if(temperature != NULL){
			input[0] = *gl_get_double(weather, temperature);
		} else {
			input[0] = 0.0;
		}
		if(humidity != NULL){
			input[1] = *gl_get_double(weather, humidity);
		} else {
			input[1] = 0.0;
		}
		if(solar != NULL){
			input[2] = *gl_get_double(weather, solar);
		} else {
			input[2] = 0.0;
		}
		if(wind != NULL){
			input[3] = *gl_get_double(weather, wind);
		} else {
			input[3] = 0.0;
		}
		if(rain != NULL){
			input[4] = *gl_get_double(weather, rain);
		} else {
			input[4] = 0.0;
		}
	} else {
		input[0] = input[1] = input[2] = input[3] = input[4] = 0.0;
	}
	input[5] = 1.0;

	output[0] = output[1] = output[2] = output[3] = output[4] = output[5] = 0.0;
	for(i = 0; i < 6; ++i){
		output[0] += imped_p[i] * input[i];
		output[1] += input[i] * imped_q[i];
		output[2] += current_m[i] * input[i];
		output[3] += current_a[i] * input[i];
		output[4] += power_p[i] * input[i];
		output[5] += power_q[i] * input[i];
	}

	kZ.SetRect(output[0], output[1]);
	kI.SetPolar(output[2], output[3] * PI/180.0);
	kP.SetRect(output[4], output[5]);

	/* ---scale by schedule here--- */

	constant_power[0] = constant_power[1] = constant_power[2] = kP;
	// rotate currents by voltage angles
	//constant_current[0] = constant_current[1] = constant_current[2] = kI;
	for(i = 0; i < 3; ++i){
		double v_angle = voltage[i].Arg();
		constant_current[i].SetPolar(output[2], output[3] * PI/180.0 + v_angle);
	}

	if (kZ != 0.0)
		constant_impedance[0] = constant_impedance[1] = constant_impedance[2] = kZ;

	//Must be at the bottom, or the new values will be calculated after the fact
	result = load::sync(t0);
	
	return result;
}

int pqload::isa(char *classname)
{
	return strcmp(classname,"pqload")==0 || load::isa(classname);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: pqload
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_pqload(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(pqload::oclass);
		if (*obj!=NULL)
		{
			pqload *my = OBJECTDATA(*obj,pqload);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(pqload);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to the object to be initialized
* @return 1 on success, 0 on error
*/
EXPORT int init_pqload(OBJECT *obj)
{
	try {
		pqload *my = OBJECTDATA(obj,pqload);
		return my->init(obj->parent);
	} 
	INIT_CATCHALL(pqload);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_pqload(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		pqload *pObj = OBJECTDATA(obj,pqload);
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
	SYNC_CATCHALL(pqload);
}

EXPORT int isa_pqload(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,pqload)->isa(classname);
}

/**@}*/

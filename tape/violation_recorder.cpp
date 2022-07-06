//Extra include - lets the odd "new" constructor call be used, without having to do it kludgy-manual way
#include <iostream>

#include "violation_recorder.h"

CLASS *violation_recorder::oclass = NULL;
CLASS *violation_recorder::pclass = NULL;
violation_recorder *violation_recorder::defaults = NULL;

void new_violation_recorder(MODULE *mod){
	new violation_recorder(mod);
}

violation_recorder::violation_recorder(MODULE *mod){
	if(oclass == NULL)
		{
#ifdef _DEBUG
		gl_debug("construction violation_recorder class");
#endif
		oclass = gl_register_class(mod, const_cast<char *>("violation_recorder"), sizeof(violation_recorder), PC_POSTTOPDOWN);
        if(oclass == NULL)
            GL_THROW(const_cast<char *>("unable to register object class implemented by %s"), __FILE__);

        if(gl_publish_variable(oclass,
			PT_char256, "file", PADDR(filename), PT_DESCRIPTION, "output file name",
			PT_char256, "summary", PADDR(summary), PT_DESCRIPTION, "summary output file name",
			PT_char256, "virtual_substation", PADDR(virtual_substation), PT_DESCRIPTION, "name of the substation node to monitor for reverse flow violation",
			PT_double, "interval[s]", PADDR(dInterval), PT_DESCRIPTION, "recording interval (0 'every iteration', -1 'on change')",
			PT_double, "flush_interval[s]", PADDR(dFlush_interval), PT_DESCRIPTION, "file flush interval (0 never, negative on samples)",
			PT_bool, "strict", PADDR(strict), PT_DESCRIPTION, "causes the violation_recorder to stop the simulation should there be a problem opening or writing with the violation_recorder",
			PT_bool, "echo", PADDR(echo), PT_DESCRIPTION, "causes the violation_recorder to echo messages to the screen",
			PT_int32, "limit", PADDR(limit), PT_DESCRIPTION, "the maximum number of lines to write to the file",
			PT_int32, "violation_delay", PADDR(violation_start_delay), PT_DESCRIPTION, "the number of seconds to skip before recording violations",
			PT_double, "xfrmr_thermal_limit_upper", PADDR(xfrmr_thermal_limit_upper), PT_DESCRIPTION, "xfrmr_thermal_limit_upper",
			PT_double, "xfrmr_thermal_limit_lower", PADDR(xfrmr_thermal_limit_lower), PT_DESCRIPTION, "xfrmr_thermal_limit_lower",
			PT_double, "line_thermal_limit_upper", PADDR(line_thermal_limit_upper), PT_DESCRIPTION, "line_thermal_limit_upper",
			PT_double, "line_thermal_limit_lower", PADDR(line_thermal_limit_lower), PT_DESCRIPTION, "line_thermal_limit_lower",
			PT_double, "node_instantaneous_voltage_limit_upper", PADDR(node_instantaneous_voltage_limit_upper), PT_DESCRIPTION, "node_instantaneous_voltage_limit_upper",
			PT_double, "node_instantaneous_voltage_limit_lower", PADDR(node_instantaneous_voltage_limit_lower), PT_DESCRIPTION, "node_instantaneous_voltage_limit_lower",
			PT_double, "node_continuous_voltage_limit_upper", PADDR(node_continuous_voltage_limit_upper), PT_DESCRIPTION, "node_continuous_voltage_limit_upper",
			PT_double, "node_continuous_voltage_limit_lower", PADDR(node_continuous_voltage_limit_lower), PT_DESCRIPTION, "node_continuous_voltage_limit_lower",
			PT_double, "node_continuous_voltage_interval", PADDR(node_continuous_voltage_interval), PT_DESCRIPTION, "node_continuous_voltage_interval",
			PT_double, "secondary_dist_voltage_rise_upper_limit", PADDR(secondary_dist_voltage_rise_upper_limit), PT_DESCRIPTION, "secondary_dist_voltage_rise_upper_limit",
			PT_double, "secondary_dist_voltage_rise_lower_limit", PADDR(secondary_dist_voltage_rise_lower_limit), PT_DESCRIPTION, "secondary_dist_voltage_rise_lower_limit",
			PT_double, "substation_breaker_A_limit", PADDR(substation_breaker_A_limit), PT_DESCRIPTION, "breaker_phase_A_limit",
			PT_double, "substation_breaker_B_limit", PADDR(substation_breaker_B_limit), PT_DESCRIPTION, "breaker_phase_B_limit",
			PT_double, "substation_breaker_C_limit", PADDR(substation_breaker_C_limit), PT_DESCRIPTION, "breaker_phase_C_limit",
			PT_double, "substation_pf_lower_limit", PADDR(substation_pf_lower_limit), PT_DESCRIPTION, "substation_pf_lower_limit",
			PT_double, "inverter_v_chng_per_interval_upper_bound", PADDR(inverter_v_chng_per_interval_upper_bound), PT_DESCRIPTION, "inverter_v_chng_per_interval_upper_bound",
			PT_double, "inverter_v_chng_per_interval_lower_bound", PADDR(inverter_v_chng_per_interval_lower_bound), PT_DESCRIPTION, "inverter_v_chng_per_interval_lower_bound",
			PT_double, "inverter_v_chng_interval", PADDR(inverter_v_chng_interval), PT_DESCRIPTION, "inverter_v_chng_interval",
			PT_set, "violation_flag", PADDR(violation_flag), PT_DESCRIPTION, "bit set for determining which violations to check",
				PT_KEYWORD,"VIOLATION0",(set)VIOLATION0,
				PT_KEYWORD,"VIOLATION1",(set)VIOLATION1,
				PT_KEYWORD,"VIOLATION2",(set)VIOLATION2,
				PT_KEYWORD,"VIOLATION3",(set)VIOLATION3,
				PT_KEYWORD,"VIOLATION4",(set)VIOLATION4,
				PT_KEYWORD,"VIOLATION5",(set)VIOLATION5,
				PT_KEYWORD,"VIOLATION6",(set)VIOLATION6,
				PT_KEYWORD,"VIOLATION7",(set)VIOLATION7,
				PT_KEYWORD,"VIOLATION8",(set)VIOLATION8,
				PT_KEYWORD,"ALLVIOLATIONS",(set)ALLVIOLATIONS,
		NULL) < 1){
			;//GL_THROW("unable to publish properties in %s",__FILE__);
		}
        // TODO set defaults here
		defaults = this;
		memset(this, 0, sizeof(violation_recorder));
    }
}

int violation_recorder::create(){
	memcpy(this, defaults, sizeof(violation_recorder));
	strict = false;
	return 1;
}

int violation_recorder::init(OBJECT *obj){

	// check for filename
	if(0 == filename[0]){
		// if no filename, auto-generate based on class
		if(strict){
			gl_error("violation_recorder::init(): no filename defined in strict mode");
			return 0;
		} else {
			sprintf(filename, "%s-violation-log.csv", oclass->name);
			gl_warning("violation_recorder::init(): no filename defined, auto-generating '%s'", filename.get_string());
		}
	}

	// check for summary
	if(0 == summary[0]){
		// if no summary, auto-generate based on class
		if(strict){
			gl_error("violation_recorder::init(): no summary defined in strict mode");
			return 0;
		} else {
			sprintf(summary, "%s-violation-summary.csv", oclass->name);
			gl_warning("violation_recorder::init(): no summary defined, auto-generating '%s'", summary.get_string());
		}
	}

	// check valid write interval
	write_interval = (int64)(dInterval);
	if(-1 > write_interval){
		gl_error("violation_recorder::init(): invalid write_interval of %i, must be -1 or greater", write_interval);
		return 0;
	}

	// all flush intervals are valid
	flush_interval = (int64)dFlush_interval;

	// open file
	rec_file = fopen(filename.get_string(), "w");
	if(0 == rec_file){
		if(strict){
			gl_error("violation_recorder::init(): unable to open file '%s' for writing", filename.get_string());
			return 0;
		} else {
			gl_warning("violation_recorder::init(): unable to open file '%s' for writing", filename.get_string());
			tape_status = TS_ERROR;
			return 1;
		}
	}

	tape_status = TS_OPEN;
	if(0 == write_header()){
		gl_error("violation_recorder::init(): an error occured when writing the file header");
		tape_status = TS_ERROR;
		return 0;
	}

	sim_start = -1;

	//Do the allocations semi-manually -- mostly to get the heap in the right place
	xfrmr_obj_list = vobjlist_alloc_fxn(xfrmr_obj_list);
	ohl_obj_list = vobjlist_alloc_fxn(ohl_obj_list);
	ugl_obj_list = vobjlist_alloc_fxn(ugl_obj_list);
	tplxl_obj_list = vobjlist_alloc_fxn(tplxl_obj_list);
	node_obj_list = vobjlist_alloc_fxn(node_obj_list);
	tplx_node_obj_list = vobjlist_alloc_fxn(tplx_node_obj_list);
	tplx_mtr_obj_list = vobjlist_alloc_fxn(tplx_mtr_obj_list);
	comm_mtr_obj_list = vobjlist_alloc_fxn(comm_mtr_obj_list);
	inverter_obj_list = vobjlist_alloc_fxn(inverter_obj_list);
	powerflow_obj_list = vobjlist_alloc_fxn(powerflow_obj_list);

	make_object_list(FT_CLASS, "transformer", xfrmr_obj_list);
	make_object_list(FT_CLASS, "overhead_line", ohl_obj_list);
	make_object_list(FT_CLASS, "underground_line", ugl_obj_list);
	make_object_list(FT_CLASS, "triplex_line", tplxl_obj_list);
	make_object_list(FT_CLASS, "node", node_obj_list);
	make_object_list(FT_CLASS, "triplex_node", tplx_node_obj_list);
	make_object_list(FT_CLASS, "triplex_meter", tplx_mtr_obj_list);
	make_object_list(FT_GROUPID, "commercialMeter", comm_mtr_obj_list);
	make_object_list(FT_CLASS, "inverter", inverter_obj_list); // this could catch all inverters, so...
	make_object_list(FT_MODULE, "powerflow", powerflow_obj_list);
	assoc_meter_w_xfrmr_node(tplx_mtr_obj_list, xfrmr_obj_list, node_obj_list);
	assoc_meter_w_xfrmr_node(comm_mtr_obj_list, xfrmr_obj_list, node_obj_list);
	find_substation_node(virtual_substation, node_obj_list);
	
	//Next semi-manual for uniqueLists - same idea
	xfrmr_list_v1 = uniqueList_alloc_fxn(xfrmr_list_v1);
	ohl_list_v1 = uniqueList_alloc_fxn(ohl_list_v1);
	ugl_list_v1 = uniqueList_alloc_fxn(ugl_list_v1);
	tplxl_list_v1 = uniqueList_alloc_fxn(tplxl_list_v1);
	node_list_v2 = uniqueList_alloc_fxn(node_list_v2);
	tplx_node_list_v2 = uniqueList_alloc_fxn(tplx_node_list_v2);
	tplx_meter_list_v2 = uniqueList_alloc_fxn(tplx_meter_list_v2);
	comm_meter_list_v2 = uniqueList_alloc_fxn(comm_meter_list_v2);
	tplx_node_list_v3 = uniqueList_alloc_fxn(tplx_node_list_v3);
	tplx_meter_list_v3 = uniqueList_alloc_fxn(tplx_meter_list_v3);
	comm_meter_list_v3 = uniqueList_alloc_fxn(comm_meter_list_v3);
	inverter_list_v6 = uniqueList_alloc_fxn(inverter_list_v6);
	tplx_meter_list_v7 = uniqueList_alloc_fxn(tplx_meter_list_v7);
	comm_meter_list_v7 = uniqueList_alloc_fxn(comm_meter_list_v7);

	return 1;
}

int violation_recorder::make_object_list(int type, const char * s_grp, vobjlist *q_obj_list){
	OBJECT *gr_obj = 0;
	//FINDLIST *items = gl_find_objects(FL_GROUP, s_grp);
	FINDLIST *items = gl_find_objects(FL_NEW, type, SAME, s_grp, FT_END);
	if(0 == items){
		if(strict){
			gl_error("violation_recorder::init(): unable to construct a set with group definition '%s'", s_grp);
			return 0;
		} else {
			gl_warning("violation_recorder::init(): unable to construct a set with group definition '%s'", s_grp);
			return 1;
		}
	}
	if(1 > items->hit_count){
		if(strict){
			gl_error("violation_recorder::init(): the defined group returned an empty set for '%s'", s_grp);
			return 0;
		} else {
			gl_warning("violation_recorder::init(): the defined group returned an empty set for '%s'", s_grp);
			return 1;
		}
	}

	for(gr_obj = gl_find_next(items, 0); gr_obj != 0; gr_obj = gl_find_next(items, gr_obj) ){
		// NOTE TO SELF:  Should this be gr_obj? q_obj_list is always 0 on the first pass (and therefore stays 0). If it is, then it breaks in the tack.  Maybe q_obj_list isn't allocating correctly?
		if(q_obj_list == 0){ 
			gl_error("violation_recorder::make_object_list(): requires a pointer to a vobjlist");
			return 0;
		} else {
			q_obj_list->tack(gr_obj);
		}
	}

	return 1;
}

// admittedly, this could be made more efficient with less looping...
int violation_recorder::assoc_meter_w_xfrmr_node(vobjlist *meter_list, vobjlist *xfrmr_list, vobjlist *node_list){
	vobjlist *meter, *xfrmr, *node, *node1, *node2;
	PROPERTY *p_ptr;
	char to[128], from[128];
	bool found;
//
//	// for whatever reason, using FL_GROUP and 'module=powerflow' doesn't work, so we do it the other way
//	powerflow_obj_list = new vobjlist();
//	FINDLIST *items = gl_find_objects(FL_NEW, FT_MODULE, SAME, "powerflow", FT_END);
//	OBJECT *gr_obj = 0;

//	// building a list of powerflow objects
//	for(gr_obj = gl_find_next(items, 0); gr_obj != 0; gr_obj = gl_find_next(items, gr_obj) ){
//		powerflow_obj_list->tack(gr_obj);
//	}

	for(xfrmr = xfrmr_list; xfrmr != 0; xfrmr = xfrmr->next){
		if (xfrmr->obj == 0) continue;
		//if (!has_phase(xfrmr->obj, PHASE_S)) continue;
		found = false;
		node1 = xfrmr;
		gl_output("");

		while (!found) {

			p_ptr = gl_get_property(node1->obj, const_cast<char *>("to"));
			gl_get_value(node1->obj, GETADDR(node1->obj, p_ptr), to, 127, p_ptr);

			// look through the meters to see if one matches the 'to' field
			for(meter = meter_list; meter != 0; meter = meter->next){
				if (meter->obj == 0) continue;
				// check to see if a meter name matches with 'to' field
				if (strcmp(to, meter->obj->name) == 0) {
					p_ptr = gl_get_property(xfrmr->obj, const_cast<char *>("from"));
					gl_get_value(xfrmr->obj, GETADDR(xfrmr->obj, p_ptr), from, 127, p_ptr);
					// get the node on the primary side of the transformer
					for(node = node_list; node != 0; node = node->next){
						if (node->obj == 0) continue;
						// found it
						if (strcmp(from, node->obj->name) == 0) {
							// gl_output("NODE: %s  METER: %s" , node->obj->name, meter->obj->name);
							// map the meter to the primary side node
							meter->ref_obj = node->obj;
							found = true;
							break;
						}
					}
					break;
				}
			}

			// loop through the powerflow objects to find one whose 'from' field matches the 'to' field
			for(node2 = powerflow_obj_list; node2 != 0; node2 = node2->next){
				if (node2->obj == 0) continue;
				p_ptr = gl_get_property(node2->obj, const_cast<char *>("from"));
				if (p_ptr != 0) {
					gl_get_value(node2->obj, GETADDR(node2->obj, p_ptr), from, 127, p_ptr);
					// found it
					if (strcmp(to, from) == 0) {
						// reset node so the next iteration walks down the chain
						node1 = node2;
						break;
					}
				}
			}

			// we ran through the list and weren't able to match a 'to' and 'from', so we should bail to prevent an infinite loop
			if (node2 == 0) {
				break;
			}
		}
	}

	return 1;
}

// node is a bit of a misnomer; it is really a link object that we want
int violation_recorder::find_substation_node(char256 node_name, vobjlist *node_list){

	char256 name;

	if (0 != node_name[0]) {
		OBJECT *obj = gl_get_object(node_name);
		if (obj != 0) {
			if (strcmp(node_name, obj->name) == 0) {
				link_monitor_obj = obj;
				return 1;
			}
		}
	} else {
		//gl_warning("No object specified for monitoring reverse flow. Attempting to find the correct object automatically.");
		gl_warning("No object specified for monitoring reverse flow; no reverse flow violations will be reported.");
	}

	return 0;
}

TIMESTAMP violation_recorder::postsync(TIMESTAMP t0, TIMESTAMP t1){

	// if every iteration
	if(0 == write_interval){
		// TODO: Not sure if we keep this. Need to think about this some more.
		check_violations(t1);
	} else if(0 < write_interval){
		// recalculate next_time, since we know commit() will fire
		if(last_write + write_interval <= t1){
			interval_write = true;
			last_write = t1;
			next_write = t1 + write_interval;
		}
		return next_write;
	} else {
		// on-change intervals simply short-circuit
		return TS_NEVER;
	}

	// the interval recorders have already return'ed out, earlier in the sequence.
	return TS_NEVER;
}

int violation_recorder::commit(TIMESTAMP t1){
	// short-circuit if strict & error
	if (!pass_error_check()){return 0;}

	// short-circuit if not open
	if(TS_OPEN != tape_status){
		return 1;
	}

	// if periodic interval, check for write
	if(write_interval > 0){
		if(interval_write){
			check_violations(t1);
			last_write = t1;
			interval_write = false;
		}
	}

	// if every timestep,
	//	* compare to last values
	//	* if different, write
	if(-1 == write_interval){
		// do the check here?
		check_violations(t1);
	}

	// check if strict & error ... a second time in case the periodic behavior failed.
	if (!pass_error_check()){return 0;}

	return 1;
}

int violation_recorder::pass_error_check () {
	// check if strict & error ... a second time in case the periodic behavior failed.
	if((TS_ERROR == tape_status) && strict){
		gl_error("violation_recorder::commit(): the object has error'ed and is halting the simulation");
		return 0;
	}
	return 1;
}

int violation_recorder::isa(char *classname){
	return (strcmp(classname, oclass->name) == 0);
}

/**
	@return 0 on failure, 1 on success
 **/
int violation_recorder::write_header(){

	time_t now = time(NULL);
	//quickobjlist *qol = 0;
	OBJECT *obj=OBJECTHDR(this);

	if(TS_OPEN != tape_status){
		// could be ERROR or CLOSED
		return 0;
	}
	if(0 == rec_file){
		gl_error("violation_recorder::write_header(): the output file was not opened");
		tape_status = TS_ERROR;
		return 0; // serious problem
	}

	// write model file name
	if(0 > fprintf(rec_file,"# file...... %s\n", filename.get_string())){ return 0; }
	if(0 > fprintf(rec_file,"# date...... %s", asctime(localtime(&now)))){ return 0; }
#ifdef _WIN32
	if(0 > fprintf(rec_file,"# user...... %s\n", getenv("USERNAME"))){ return 0; }
	if(0 > fprintf(rec_file,"# host...... %s\n", getenv("MACHINENAME"))){ return 0; }
#else
	if(0 > fprintf(rec_file,"# user...... %s\n", getenv("USER"))){ return 0; }
	if(0 > fprintf(rec_file,"# host...... %s\n", getenv("HOST"))){ return 0; }
#endif
	if(0 > fprintf(rec_file,"# limit..... %d\n", limit)){ return 0; }
	if(0 > fprintf(rec_file,"# interval.. %lld\n", write_interval)){ return 0; }
	if(0 > fprintf(rec_file,"# timestamp, violation, observation, upper_limit, lower_limit, object(s), object type(s), phase, message\n")){ return 0; }
//	if(0 > fprintf(rec_file, "\n")){ return 0; }
	return 1;
}

int violation_recorder::check_violations(TIMESTAMP t1) {

	if (sim_start == -1) {
		sim_start = t1;
	}

	if ((t1-sim_start) < violation_start_delay) return 1;

	if (violation_flag & VIOLATION1)
		check_violation_1(t1);

	if (violation_flag & VIOLATION2)
		check_violation_2(t1);

	if (violation_flag & VIOLATION3)
		check_violation_3(t1);

	if (violation_flag & VIOLATION4)
		check_violation_4(t1);

	if (violation_flag & VIOLATION5)
		check_violation_5(t1);

	if (violation_flag & VIOLATION6)
		check_violation_6(t1);

	if (violation_flag & VIOLATION7)
		check_violation_7(t1);

	if (violation_flag & VIOLATION8)
		check_violation_8(t1);

	return 1;
}

// Exceeding device thermal limit
int violation_recorder::check_violation_1(TIMESTAMP t1) {
//	gl_output("VIOLATION 1");
	vobjlist *curr = 0;

	check_xfrmr_thermal_limit(t1, xfrmr_obj_list, xfrmr_list_v1, XFMR, xfrmr_thermal_limit_upper, xfrmr_thermal_limit_lower);
	check_line_thermal_limit(t1, ohl_obj_list, ohl_list_v1, OHLN, line_thermal_limit_upper, line_thermal_limit_lower);
	check_line_thermal_limit(t1, ugl_obj_list, ugl_list_v1, UGLN, line_thermal_limit_upper, line_thermal_limit_lower);
	check_line_thermal_limit(t1, tplxl_obj_list, tplxl_list_v1, TPXL, line_thermal_limit_upper, line_thermal_limit_lower);

	return 1;

}

int violation_recorder::check_line_thermal_limit(TIMESTAMP t1, vobjlist *list, uniqueList *uniq_list, int type, double upper_bound, double lower_bound) {

	vobjlist *curr = 0;
	double nominal, nominalA, nominalB, nominalC; // FIXME: add default value in case it's not initialized.
	char objname[128];
	double retval;

	for(curr = list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		//p_ptr = gl_get_property(curr->obj, "continuous_rating");
		//nominal = get_observed_double_value(curr->obj, p_ptr);
		if (has_phase(curr->obj, PHASE_S)) { // split phase line
			triplex_line *pTriplex_line = OBJECTDATA(curr->obj,triplex_line);
			triplex_line_configuration *pConfiguration1 = OBJECTDATA(pTriplex_line->configuration,triplex_line_configuration);
			
			triplex_line_conductor *pConfigurationA = OBJECTDATA(pConfiguration1->phaseA_conductor,triplex_line_conductor);
			nominal = pConfigurationA->summer.continuous;
			if (fails_static_condition(curr->obj, const_cast<char *>("current_out_A"), upper_bound, lower_bound, nominal, &retval)) {
				uniq_list->insert(curr->obj->name);
				increment_violation(VIOLATION1, type);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, S1, Current violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
			triplex_line_conductor *pConfigurationB = OBJECTDATA(pConfiguration1->phaseB_conductor,triplex_line_conductor);
			nominal = pConfigurationB->summer.continuous;
			if (fails_static_condition(curr->obj, const_cast<char *>("current_out_B"), upper_bound, lower_bound, nominal, &retval)) {
				uniq_list->insert(curr->obj->name);
				increment_violation(VIOLATION1, type);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, S2, Current violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		} else { // 'normal' 3-phase line
			if ( gl_object_isa(curr->obj, const_cast<char *>("underground_line"), const_cast<char *>("powerflow")) ) {
				underground_line *pThree_phase_line = OBJECTDATA(curr->obj,underground_line);
				line_configuration *pConfiguration1 = OBJECTDATA(pThree_phase_line->configuration,line_configuration);

				if (has_phase(curr->obj, PHASE_A)) {
					underground_line_conductor *pConfigurationA = OBJECTDATA(pConfiguration1->phaseA_conductor,underground_line_conductor);
					if ( pConfigurationA == NULL)
						nominalA = pConfiguration1->summer.continuous;
					else
						nominalA = pConfigurationA->summer.continuous;
				}
					
				if (has_phase(curr->obj, PHASE_B)) {
					underground_line_conductor *pConfigurationB = OBJECTDATA(pConfiguration1->phaseB_conductor,underground_line_conductor);	
					if ( pConfigurationB == NULL)
						nominalB = pConfiguration1->summer.continuous;
					else
						nominalB = pConfigurationB->summer.continuous;
				}

				if (has_phase(curr->obj, PHASE_C)) {
					underground_line_conductor *pConfigurationC = OBJECTDATA(pConfiguration1->phaseC_conductor,underground_line_conductor);	
					if ( pConfigurationC == NULL)
						nominalC = pConfiguration1->summer.continuous;
					else
						nominalC = pConfigurationC->summer.continuous;
				}
			}
			else {
				overhead_line *pThree_phase_line = OBJECTDATA(curr->obj,overhead_line);
				line_configuration *pConfiguration1 = OBJECTDATA(pThree_phase_line->configuration,line_configuration);

				if (has_phase(curr->obj, PHASE_A)) {
					overhead_line_conductor *pConfigurationA = OBJECTDATA(pConfiguration1->phaseA_conductor,overhead_line_conductor);
					if ( pConfigurationA == NULL)
						nominalA = pConfiguration1->summer.continuous;
					else
						nominalA = pConfigurationA->summer.continuous;
				}

				if (has_phase(curr->obj, PHASE_B)) {
					overhead_line_conductor *pConfigurationB = OBJECTDATA(pConfiguration1->phaseB_conductor,overhead_line_conductor);
					if ( pConfigurationB == NULL)
						nominalB = pConfiguration1->summer.continuous;
					else
						nominalB = pConfigurationB->summer.continuous;
				}

				if (has_phase(curr->obj, PHASE_C)) {
					overhead_line_conductor *pConfigurationC = OBJECTDATA(pConfiguration1->phaseC_conductor,overhead_line_conductor);
					if ( pConfigurationC == NULL)
						nominalC = pConfiguration1->summer.continuous;
					else
						nominalC = pConfigurationC->summer.continuous;
				}				
				
			}		
			if (has_phase(curr->obj, PHASE_A)) {
				if (fails_static_condition(curr->obj, const_cast<char *>("current_out_A"), upper_bound, lower_bound, nominalA, &retval)) {
					uniq_list->insert(curr->obj->name);
					increment_violation(VIOLATION1, type);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, A, Current violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
				}
			}
			if (has_phase(curr->obj, PHASE_B)) {
				if (fails_static_condition(curr->obj, const_cast<char *>("current_out_B"), upper_bound, lower_bound, nominalB, &retval)) {
					uniq_list->insert(curr->obj->name);
					increment_violation(VIOLATION1, type);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, B, Current violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
				}
			}
			if (has_phase(curr->obj, PHASE_C)) {
				if (fails_static_condition(curr->obj, const_cast<char *>("current_out_C"), upper_bound, lower_bound, nominalC, &retval)) {
					uniq_list->insert(curr->obj->name);
					increment_violation(VIOLATION1, type);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, C, Current violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
				}
			}
		}
	}

	return 1;

}

int violation_recorder::check_xfrmr_thermal_limit(TIMESTAMP t1, vobjlist *list, uniqueList *uniq_list, int type, double upper_bound, double lower_bound) {

	vobjlist *curr = 0;
	double nominal;
	char objname[128];
	double retval;

	for(curr = list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		// this is for the triplex transformers b/c phase is meaningless
		if (has_phase(curr->obj, PHASE_S)) {
			transformer *pTransformer = OBJECTDATA(curr->obj,transformer);
			transformer_configuration *pConfiguration = OBJECTDATA(pTransformer->configuration,transformer_configuration);
			nominal = pConfiguration->kVA_rating*1000.;
			if (fails_static_condition(curr->obj, const_cast<char *>("power_out"), upper_bound, lower_bound, nominal, &retval)) {
				uniq_list->insert(curr->obj->name);
				increment_violation(VIOLATION1, type);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, S, Power violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		// this is for the other transformers which have 3 phases, each of which can violate the limit
		} else {
			transformer *pTransformer = OBJECTDATA(curr->obj,transformer);
			transformer_configuration *pConfiguration = OBJECTDATA(pTransformer->configuration,transformer_configuration);
			if (has_phase(curr->obj, PHASE_A)) {
				nominal = pConfiguration->phaseA_kVA_rating*1000.;
				if (fails_static_condition(curr->obj, const_cast<char *>("power_out_A"), upper_bound, lower_bound, nominal, &retval)) {
					uniq_list->insert(curr->obj->name);
					increment_violation(VIOLATION1, type);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, A, Power violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
				}
			}
			if (has_phase(curr->obj, PHASE_B)) {
				nominal = pConfiguration->phaseB_kVA_rating*1000.;
				if (fails_static_condition(curr->obj, const_cast<char *>("power_out_B"), upper_bound, lower_bound, nominal, &retval)) {
					uniq_list->insert(curr->obj->name);
					increment_violation(VIOLATION1, type);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, B, Power violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
				}
			}
			if (has_phase(curr->obj, PHASE_C)) {
				nominal = pConfiguration->phaseC_kVA_rating*1000.;
				if (fails_static_condition(curr->obj, const_cast<char *>("power_out_C"), upper_bound, lower_bound, nominal, &retval)) {
					uniq_list->insert(curr->obj->name);
					increment_violation(VIOLATION1, type);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION1, %f, %f, %f, %s, %s, C, Power violates thermal limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
				}
			}
		}
	}

	return 1;

}

// Instantaneous voltage of node over 1.1pu
int violation_recorder::check_violation_2(TIMESTAMP t1) {
//	gl_output("VIOLATION 2");
	vobjlist *curr = 0;
	PROPERTY *p_ptr;
	double nominal;
	char objname[128];
	int c = 0;
	double node_upper_bound = node_instantaneous_voltage_limit_upper;
	double node_lower_bound = node_instantaneous_voltage_limit_lower;
	double retval;

	for(curr = node_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		nominal = get_observed_double_value(curr->obj, p_ptr);
		if (has_phase(curr->obj, PHASE_A)) {
			if (fails_static_condition(curr->obj, const_cast<char *>("voltage_A"), node_upper_bound, node_lower_bound, nominal, &retval)) {
				node_list_v2->insert(curr->obj->name);
				increment_violation(VIOLATION2, NODE);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION2, %f, %f, %f, %s, %s, A, Per unit voltage violates limit."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		}
		if (has_phase(curr->obj, PHASE_B)) {
			if (fails_static_condition(curr->obj, const_cast<char *>("voltage_B"), node_upper_bound, node_lower_bound, nominal, &retval)) {
				node_list_v2->insert(curr->obj->name);
				increment_violation(VIOLATION2, NODE);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION2, %f, %f, %f, %s, %s, B, Per unit voltage violates limit."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		}
		if (has_phase(curr->obj, PHASE_C)) {
			if (fails_static_condition(curr->obj, const_cast<char *>("voltage_C"), node_upper_bound, node_lower_bound, nominal, &retval)) {
				node_list_v2->insert(curr->obj->name);
				increment_violation(VIOLATION2, NODE);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION2, %f, %f, %f, %s, %s, C, Per unit voltage violates limit."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		}
	}

	for(curr = comm_mtr_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		nominal = get_observed_double_value(curr->obj, p_ptr);
		if (has_phase(curr->obj, PHASE_A)) {
			if (fails_static_condition(curr->obj, const_cast<char *>("voltage_A"), node_upper_bound, node_lower_bound, nominal, &retval)) {
				comm_meter_list_v2->insert(curr->obj->name);
				increment_violation(VIOLATION2, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION2, %f, %f, %f, %s, %s, A, Per unit voltage violates limit."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		}
		if (has_phase(curr->obj, PHASE_B)) {
			if (fails_static_condition(curr->obj, const_cast<char *>("voltage_B"), node_upper_bound, node_lower_bound, nominal, &retval)) {
				comm_meter_list_v2->insert(curr->obj->name);
				increment_violation(VIOLATION2, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION2, %f, %f, %f, %s, %s, B, Per unit voltage violates limit."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		}
		if (has_phase(curr->obj, PHASE_C)) {
			if (fails_static_condition(curr->obj, const_cast<char *>("voltage_C"), node_upper_bound, node_lower_bound, nominal, &retval)) {
				comm_meter_list_v2->insert(curr->obj->name);
				increment_violation(VIOLATION2, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION2, %f, %f, %f, %s, %s, C, Per unit voltage violates limit."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		}
	}

	for(curr = tplx_node_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		nominal = get_observed_double_value(curr->obj, p_ptr) * 2.;
		if (has_phase(curr->obj, PHASE_S1) && has_phase(curr->obj, PHASE_S2)) {
			if (fails_static_condition(curr->obj, const_cast<char *>("voltage_12"), node_upper_bound, node_lower_bound, nominal, &retval)) {
				tplx_node_list_v2->insert(curr->obj->name);
				increment_violation(VIOLATION2, TPXN);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION2, %f, %f, %f, %s, %s, S, Per unit voltage violates limit."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		}
	}

	for(curr = tplx_mtr_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		nominal = get_observed_double_value(curr->obj, p_ptr) * 2.;
		if (has_phase(curr->obj, PHASE_S1) && has_phase(curr->obj, PHASE_S2)) {
			if (fails_static_condition(curr->obj, const_cast<char *>("voltage_12"), node_upper_bound, node_lower_bound, nominal, &retval)) {
				tplx_meter_list_v2->insert(curr->obj->name);
				increment_violation(VIOLATION2, TPXM);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION2, %f, %f, %f, %s, %s, S, Per unit voltage violates limit."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name);
			}
		}
	}

	return 1;

}

// Voltage of node over 1.05pu or under 0.95pu for 5 minutes or more
int violation_recorder::check_violation_3(TIMESTAMP t1) {
//	gl_output("VIOLATION 3");
	vobjlist *curr = 0;
	PROPERTY *p_ptr;
	double nominal;
	char objname[128];
	int c = 0;
	double node_upper_bound = node_continuous_voltage_limit_upper;
	double node_lower_bound = node_continuous_voltage_limit_lower;
	double interval = node_continuous_voltage_interval;
	double retval;

	for(curr = tplx_node_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		nominal = get_observed_double_value(curr->obj, p_ptr) * 2.;
		if (has_phase(curr->obj, PHASE_S1) && has_phase(curr->obj, PHASE_S2)) { // We are now checking all objects regardless of parent // && gl_object_isa(curr->obj->parent,"transformer")) {
			if (fails_continuous_condition(curr, 0, const_cast<char *>("voltage_12"), t1, interval, node_upper_bound, node_lower_bound, nominal, &retval)) {
				tplx_node_list_v3->insert(curr->obj->name);
				increment_violation(VIOLATION3, TPXN);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION3, %f, %f, %f, %s, %s, S, Per unit voltage violates limit continuously over %is interval."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)node_continuous_voltage_interval);
			}
		}
	}

	for(curr = tplx_mtr_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		nominal = get_observed_double_value(curr->obj, p_ptr) * 2.;
		if (has_phase(curr->obj, PHASE_S1) && has_phase(curr->obj, PHASE_S2)) { // We are now checking all objects regardless of parent // && gl_object_isa(curr->obj->parent,"transformer")) {
			if (fails_continuous_condition(curr, 0, const_cast<char *>("voltage_12"), t1, interval, node_upper_bound, node_lower_bound, nominal, &retval)) {
				tplx_meter_list_v3->insert(curr->obj->name);
				increment_violation(VIOLATION3, TPXM);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION3, %f, %f, %f, %s, %s, S, Per unit voltage violates limit continuously over %is interval."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)node_continuous_voltage_interval);
			}
		}
	}

	for(curr = comm_mtr_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		nominal = get_observed_double_value(curr->obj, p_ptr);
		if (has_phase(curr->obj, PHASE_A)) {
			if (fails_continuous_condition(curr, 0, const_cast<char *>("voltage_A"), t1, interval, node_upper_bound, node_lower_bound, nominal, &retval)) {
				comm_meter_list_v3->insert(curr->obj->name);
				increment_violation(VIOLATION3, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION3, %f, %f, %f, %s, %s, A, Per unit voltage violates limit continuously over %is interval."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)node_continuous_voltage_interval);
			}
		}
		if (has_phase(curr->obj, PHASE_B)) {
			if (fails_continuous_condition(curr, 0, const_cast<char *>("voltage_B"), t1, interval, node_upper_bound, node_lower_bound, nominal, &retval)) {
				comm_meter_list_v3->insert(curr->obj->name);
				increment_violation(VIOLATION3, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION3, %f, %f, %f, %s, %s, B, Per unit voltage violates limit continuously over %is interval."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)node_continuous_voltage_interval);
			}
		}
		if (has_phase(curr->obj, PHASE_C)) {
			if (fails_continuous_condition(curr, 0, const_cast<char *>("voltage_C"), t1, interval, node_upper_bound, node_lower_bound, nominal, &retval)) {
				comm_meter_list_v3->insert(curr->obj->name);
				increment_violation(VIOLATION3, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION3, %f, %f, %f, %s, %s, C, Per unit voltage violates limit continuously over %is interval."), retval, node_upper_bound, node_lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)node_continuous_voltage_interval);
			}
		}
	}

	return 1;

}

// Reverse power exceeds 50% of the minimum trip setting of the substation breaker (fuse) or a recloser.
int violation_recorder::check_violation_4(TIMESTAMP t1) {
	return check_reverse_flow_violation(t1, VIOLATION4, 0.50, const_cast<char *>("Reverse flow current warning."));
}

// Reverse power exceeds 75% of the minimum trip setting of the substation breaker (fuse) or a recloser.
int violation_recorder::check_violation_5(TIMESTAMP t1) {
	return check_reverse_flow_violation(t1, VIOLATION5, 0.75,
                                        const_cast<char *>("Reverse flow current violates limit."));
}

int violation_recorder::check_reverse_flow_violation(TIMESTAMP t1, int violation_num, double pct, char *str) {
//	gl_output("%s", violation_name);
	if (link_monitor_obj == 0)
		return 0;

	double retval;
	char objname[128];
	double breaker_limit_A_upper = substation_breaker_A_limit*pct;
	double breaker_limit_A_lower = 0.;
	double breaker_limit_B_upper = substation_breaker_B_limit*pct;
	double breaker_limit_B_lower = 0.;
	double breaker_limit_C_upper = substation_breaker_C_limit*pct;
	double breaker_limit_C_lower = 0.;

	double nominal = 1.;
	PROPERTY *p_ptr;
	set *directions;
	p_ptr = gl_get_property(link_monitor_obj, const_cast<char *>("flow_direction"));
	if (p_ptr == NULL)
		return 0;
	directions = gl_get_set(link_monitor_obj, p_ptr);

	if (has_phase(link_monitor_obj, PHASE_A) && ((*directions & _AR) == _AR)) {
		if (fails_static_condition(link_monitor_obj, const_cast<char *>("current_out_A"), breaker_limit_A_upper, breaker_limit_A_lower, nominal, &retval)) {
			increment_violation(violation_num);
			write_to_stream(t1, echo, const_cast<char *>("VIOLATION%i, %f, %f, %f, %s, %s, A, %s"), (int)l2((double)violation_num) + 1, retval, breaker_limit_A_upper, breaker_limit_A_lower, gl_name(link_monitor_obj, objname, 127), link_monitor_obj->oclass->name, str);
		}
	}
	if (has_phase(link_monitor_obj, PHASE_B) && ((*directions & _BR) == _BR)) {
		if (fails_static_condition(link_monitor_obj, const_cast<char *>("current_out_B"), breaker_limit_B_upper, breaker_limit_B_lower, nominal, &retval)) {
			increment_violation(violation_num);
			write_to_stream(t1, echo, const_cast<char *>("VIOLATION%i, %f, %f, %f, %s, %s, B, %s"), (int)l2((double)violation_num) + 1, retval, breaker_limit_B_upper, breaker_limit_B_lower, gl_name(link_monitor_obj, objname, 127), link_monitor_obj->oclass->name, str);
		}
	}
	if (has_phase(link_monitor_obj, PHASE_C) && ((*directions & _CR) == _CR)) {
		if (fails_static_condition(link_monitor_obj, const_cast<char *>("current_out_C"), breaker_limit_C_upper, breaker_limit_C_lower, nominal, &retval)) {
			increment_violation(violation_num);
			write_to_stream(t1, echo, const_cast<char *>("VIOLATION%i, %f, %f, %f, %s, %s, C, %s"), (int)l2((double)violation_num) + 1, retval, breaker_limit_C_upper, breaker_limit_C_lower, gl_name(link_monitor_obj, objname, 127), link_monitor_obj->oclass->name, str);
		}
	}

	return 1;
}

// Any voltage change at a PV POC that is greater than 1.5% between two one-minute simulation time-steps.
int violation_recorder::check_violation_6(TIMESTAMP t1) {
//	gl_output("VIOLATION 6");
	vobjlist *curr = 0;
	char objname[128];
	double upper_bound = inverter_v_chng_per_interval_upper_bound;
	double lower_bound = inverter_v_chng_per_interval_lower_bound;
	double interval = inverter_v_chng_interval;
	double nominal = 1.0;
	double retval;

	for(curr = inverter_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		if (has_phase(curr->obj, PHASE_S)) { // inverter connected to a triplex system, only checking one phase here
			if (fails_dynamic_condition(curr, 1, const_cast<char *>("phaseB_V_Out"), t1, interval, upper_bound, lower_bound, nominal, &retval)) { // this is S1 !?!
				inverter_list_v6->insert(curr->obj->name);
				increment_violation(VIOLATION6);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION6, %f, %f, %f, %s, %s, S, Voltage change between %is intervals violates limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)inverter_v_chng_interval);
			}
		} else { // assume we are a three phase inverter
			if (has_phase(curr->obj, PHASE_A)) {
				if (fails_dynamic_condition(curr, 0, const_cast<char *>("phaseA_V_Out"), t1, interval, upper_bound, lower_bound, nominal, &retval)) {
					inverter_list_v6->insert(curr->obj->name);
					increment_violation(VIOLATION6);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION6, %f, %f, %f, %s, %s, A, Voltage change between %is intervals violates limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)inverter_v_chng_interval);
				}
			}
			if (has_phase(curr->obj, PHASE_B)) {
				if (fails_dynamic_condition(curr, 1, const_cast<char *>("phaseB_V_Out"), t1, interval, upper_bound, lower_bound, nominal, &retval)) {
					inverter_list_v6->insert(curr->obj->name);
					increment_violation(VIOLATION6);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION6, %f, %f, %f, %s, %s, B, Voltage change between %is intervals violates limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)inverter_v_chng_interval);
				}
			}
			if (has_phase(curr->obj, PHASE_C)) {
				if (fails_dynamic_condition(curr, 2, const_cast<char *>("phaseC_V_Out"), t1, interval, upper_bound, lower_bound, nominal, &retval)) {
					inverter_list_v6->insert(curr->obj->name);
					increment_violation(VIOLATION6);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION6, %f, %f, %f, %s, %s, C, Voltage change between %is intervals violates limit."), retval, upper_bound, lower_bound, gl_name(curr->obj, objname, 127), curr->obj->oclass->name, (int)inverter_v_chng_interval);
				}
			}
		}
	}

	return 1;
}

// 3V rise across the secondary distribution system
int violation_recorder::check_violation_7(TIMESTAMP t1) {
//	gl_output("VIOLATION 7");
	vobjlist *curr = 0;
	PROPERTY *p_ptr;
	double meter_voltage, meter_nominal, xfrmr_voltage, xfrmr_nominal, pu;
	char metername[128];
	char xfrmrname[128];
	double pu_upper_bound = secondary_dist_voltage_rise_upper_limit;
	double pu_lower_bound = secondary_dist_voltage_rise_lower_limit;
	double retval;

	for(curr = tplx_mtr_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		if (curr->ref_obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		meter_nominal = get_observed_double_value(curr->obj, p_ptr);
		pu = 0;
		if (has_phase(curr->obj, PHASE_S1)) {
			p_ptr = gl_get_property(curr->obj, const_cast<char *>("voltage_1"));
			meter_voltage = get_observed_double_value(curr->obj, p_ptr);
			if (has_phase(curr->obj, PHASE_A)) {
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
				xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_A"));
				xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
				pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
				if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
					tplx_meter_list_v7->insert(curr->obj->name);
					increment_violation(VIOLATION7, TPXM);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, A S1, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
				}
			}
			if (has_phase(curr->obj, PHASE_B)) {
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
				xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_B"));
				xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
				pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
				if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
					tplx_meter_list_v7->insert(curr->obj->name);
					increment_violation(VIOLATION7, TPXM);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, B S1, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
				}
			}
			if (has_phase(curr->obj, PHASE_C)) {
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
				xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_C"));
				xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
				pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
				if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
					tplx_meter_list_v7->insert(curr->obj->name);
					increment_violation(VIOLATION7, TPXM);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, C S1, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
				}
			}
		}
		if (has_phase(curr->obj, PHASE_S2)) {
			p_ptr = gl_get_property(curr->obj, const_cast<char *>("voltage_2"));
			meter_voltage = get_observed_double_value(curr->obj, p_ptr);
			if (has_phase(curr->obj, PHASE_A)) {
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
				xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_A"));
				xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
				pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
				if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
					tplx_meter_list_v7->insert(curr->obj->name);
					increment_violation(VIOLATION7, TPXM);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, A S2, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
				}
			}
			if (has_phase(curr->obj, PHASE_B)) {
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
				xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_B"));
				xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
				pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
				if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
					tplx_meter_list_v7->insert(curr->obj->name);
					increment_violation(VIOLATION7, TPXM);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, B S2, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
				}
			}
			if (has_phase(curr->obj, PHASE_C)) {
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
				xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
				p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_C"));
				xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
				pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
				if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
					tplx_meter_list_v7->insert(curr->obj->name);
					increment_violation(VIOLATION7, TPXM);
					write_to_stream(t1, echo,
                                    const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, C S2, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
				}
			}
		}
	}

	for(curr = comm_mtr_obj_list; curr != 0; curr = curr->next){
		if (curr->obj == 0) continue;
		if (curr->ref_obj == 0) continue;
		p_ptr = gl_get_property(curr->obj, const_cast<char *>("nominal_voltage"));
		meter_nominal = get_observed_double_value(curr->obj, p_ptr);
		pu = 0;
		if (has_phase(curr->obj, PHASE_A)) {
			p_ptr = gl_get_property(curr->obj, const_cast<char *>("voltage_A"));
			meter_voltage = get_observed_double_value(curr->obj, p_ptr);
			p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
			xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
			p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_A"));
			xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
			pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
			if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
				comm_meter_list_v7->insert(curr->obj->name);
				increment_violation(VIOLATION7, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, A, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
			}
		}
		if (has_phase(curr->obj, PHASE_B)) {
			p_ptr = gl_get_property(curr->obj, const_cast<char *>("voltage_B"));
			meter_voltage = get_observed_double_value(curr->obj, p_ptr);
			p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
			xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
			p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_B"));
			xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
			pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
			if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
				comm_meter_list_v7->insert(curr->obj->name);
				increment_violation(VIOLATION7, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, B, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
			}
		}
		if (has_phase(curr->obj, PHASE_C)) {
			p_ptr = gl_get_property(curr->obj, const_cast<char *>("voltage_C"));
			meter_voltage = get_observed_double_value(curr->obj, p_ptr);
			p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("nominal_voltage"));
			xfrmr_nominal = get_observed_double_value(curr->ref_obj, p_ptr);
			p_ptr = gl_get_property(curr->ref_obj, const_cast<char *>("voltage_C"));
			xfrmr_voltage = get_observed_double_value(curr->ref_obj, p_ptr);
			pu = meter_voltage/meter_nominal-xfrmr_voltage/xfrmr_nominal;
			if (fails_static_condition(pu, pu_upper_bound, pu_lower_bound, 1.0, &retval)) {
				comm_meter_list_v7->insert(curr->obj->name);
				increment_violation(VIOLATION7, CMTR);
				write_to_stream(t1, echo,
                                const_cast<char *>("VIOLATION7, %f, %f, %f, %s %s, %s %s, C, Per unit voltage difference between objects violates limit."), retval, pu_upper_bound, pu_lower_bound, gl_name(curr->obj, metername, 127), gl_name(curr->ref_obj, xfrmrname, 127), curr->obj->oclass->name, curr->ref_obj->oclass->name);
			}
		}
	}

	return 1;

}

// Powerfactor at substation is below limit
int violation_recorder::check_violation_8(TIMESTAMP t1) {

	if (link_monitor_obj == 0)
			return 0;

	double retval;
	char objname[128];

	double nominal = 1.;
	PROPERTY *p_ptr;
	p_ptr = gl_get_property(link_monitor_obj, const_cast<char *>("power_in"));

	if (p_ptr == NULL)
		return 0;

	gld::complex power_in = get_observed_complex_value(link_monitor_obj, p_ptr);
	double pf = power_in.Re()/power_in.Mag();

	if (fails_static_condition (pf, 1.0, substation_pf_lower_limit, 1.0, &retval)) {
		increment_violation(VIOLATION8);
		write_to_stream(t1, echo, const_cast<char *>("VIOLATION8, %f, %f, %f, %s, %s,, %s"), retval, 1.0, substation_pf_lower_limit, gl_name(link_monitor_obj, objname, 127), link_monitor_obj->oclass->name, "Power factor violates limit.");
	}

	return 1;
}

double violation_recorder::get_observed_double_value(OBJECT *curr, PROPERTY *prop) {
	char objname[128];
	double part_value = 0.0;
	if(prop->ptype == PT_complex){
		gld::complex *cptr = 0;
		// get value as a complex
		cptr = gl_get_complex(curr, prop);
		if(0 == cptr){
			gl_error("group_recorder::read_line(): unable to get complex property '%s' from object '%s'", prop->name, gl_name(curr, objname, 127));
			/* TROUBLESHOOT
				Could not read a complex property as a complex value.
			 */
			return 0;
		}
		part_value = cptr->Mag();
	} else {
		part_value = *(gl_get_double(curr, prop));
	}
	return part_value;
}

gld::complex violation_recorder::get_observed_complex_value(OBJECT *curr, PROPERTY *prop) {
	char objname[128];
	gld::complex *cptr = 0;
	if(prop->ptype == PT_complex){
		// get value as a complex
		cptr = gl_get_complex(curr, prop);
		if(0 == cptr){
			gl_error("group_recorder::read_line(): unable to get complex property '%s' from object '%s'", prop->name, gl_name(curr, objname, 127));
			/* TROUBLESHOOT
				Could not read a complex property as a complex value.
			 */
			return 0;
		}
	}
	return *cptr;
}

int violation_recorder::fails_static_condition (OBJECT *curr, char *prop_name, double upper_bound, double lower_bound, double normalization_value, double *retval) {
	PROPERTY *p_ptr;
	double value;
	p_ptr = gl_get_property(curr, prop_name);
	if (p_ptr == NULL)
		return 0;
	value = get_observed_double_value(curr, p_ptr);
	return fails_static_condition (value, upper_bound, lower_bound, normalization_value, retval);
}

int violation_recorder::fails_static_condition (double value, double upper_bound, double lower_bound, double normalization_value, double *retval) {
	double pu;
	(normalization_value != 0. && normalization_value != 1.) ? pu = value/normalization_value : pu = value;
	if (pu > upper_bound || pu < lower_bound) {
		*retval = pu;
		return 1;
	}
	return 0;
}

int violation_recorder::fails_dynamic_condition (vobjlist *curr, int i, char *prop_name, TIMESTAMP t1, double interval, double upper_bound, double lower_bound, double normalization_value, double *retval) {
	PROPERTY *p_ptr;
	double value, pu;
	p_ptr = gl_get_property(curr->obj, prop_name);
	if (p_ptr == NULL)
		return 0;
	value = get_observed_double_value(curr->obj, p_ptr);
	pu = value/normalization_value;
	if (curr->last_t[i] == 0) {
		// this one can not violate on the first timestep
		curr->update_last(i, pu, t1, 0);
		return 0;
	}
	// relative change since the last update
	double pct = ((pu-curr->last_v[i])/curr->last_v[i]);
	*retval = pct;
	int s_curr, s_prev;
	// we've exceeded the limit and we did
	if (pct > upper_bound || pct < lower_bound) {
		// the elapsed time has exceeded the interval
		// this throws the violation flag and updates
		// time and value
		s_prev = sign(curr->last_s[i]);
		s_curr = sign(pct);
		if ((s_prev == 0) || (s_prev == s_curr)) {
			if ((t1-curr->last_t[i]) >= (long)interval) {
				curr->update_last(i, pu, t1, s_curr);
				return 1;
			// the elapsed time has not exceed the interval,
			// we want to keep the flag, but not update
			// time or value
			} else {
				curr->update_last(i, curr->last_v[i], curr->last_t[i], s_curr); // this indexing stuff is sloppy :(
				return 0;
			}
		}
	}
	curr->update_last(i, pu, t1, 0);
	return 0;
}

int violation_recorder::fails_continuous_condition (vobjlist *curr, int i, const char *prop_name, TIMESTAMP t1, double interval, double upper_bound, double lower_bound, double normalization_value, double *retval) {
	PROPERTY *p_ptr;
	double value, pu;
	p_ptr = gl_get_property(curr->obj, const_cast<char *>(prop_name));
	if (p_ptr == NULL)
		return 0;
	int s_curr = 0, s_prev = 0;
	value = get_observed_double_value(curr->obj, p_ptr);
	pu = value/normalization_value;
	*retval = pu;
	// first time through
	if (curr->last_t[i] == 0) {
		// this one can violate on the first timestep
		if (pu > upper_bound || pu < lower_bound) {
			s_curr = sign(pu);
		}
		curr->update_last(i, pu, t1, s_curr);
		return 0;
	}
	// change since the last update
	// we've exceeded the limit
	if (pu > upper_bound || pu < lower_bound) {
		// the elapsed time has exceeded the interval
		// this throws the violation flag and updates
		// time and value
		s_prev = sign(curr->last_s[i]);
		s_curr = sign(pu);
		if ((s_prev == 0) || (s_prev == s_curr)) {
			if ((t1-curr->last_t[i]) >= (long)interval) {
				curr->update_last(i, pu, t1, s_curr);
				return 1;
			// the elapsed time has not exceed the interval,
			// we want to keep the flag, and update
			// time and value only if the violation hasn't been
			// seen before
			} else if (s_prev == 0) {
				curr->update_last(i, pu, t1, s_curr);
				return 0;
			} else {
				curr->update_last(i, curr->last_v[i], curr->last_t[i], s_curr); // this indexing stuff is sloppy :(
				return 0;
			}
		}
	}
	curr->update_last(i, pu, t1, 0);
	return 0;
}

int violation_recorder::has_phase(OBJECT *obj, int phase) {
	PROPERTY *p_ptr;
	set *phases;
	p_ptr = gl_get_property(obj, const_cast<char *>("phases"));
	phases = gl_get_set(obj, p_ptr);
	if ((*phases & phase) == phase)
		return true;
	return 0;
}

int violation_recorder::increment_violation(int number) {
	violation_count[(int)l2(number)][0]++;
	return 1;
}

int violation_recorder::increment_violation(int number, int type) {
	violation_count[(int)l2(number)][0]++;
	violation_count[(int)l2(number)][type]++;
	return 1;
}

int violation_recorder::get_violation_count(int number) {
	return violation_count[(int)l2(number)][0];
}

int violation_recorder::get_violation_count(int number, int type) {
	return violation_count[(int)l2(number)][type];
}

int violation_recorder::write_to_stream (TIMESTAMP t1, bool echo, char *fmt, ...) {
	char time_str[64];
	DATETIME dt;
	if(TS_OPEN != tape_status){
		gl_error("violation_recorder::write_line(): trying to write line when the tape is not open");
		// could be ERROR or CLOSED, should not have happened
		return 0;
	}
	if(0 == rec_file){
		gl_error("violation_recorder::write_line(): no output file open and state is 'open'");
		/* TROUBLESHOOT
			violation_recorder claimed to be open and attempted to write to a file when
			a file had not successfully	opened.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	if(0 == gl_localtime(t1, &dt)){
		gl_error("violation_recorder::write_line(): error when converting the sync time");
		/* TROUBLESHOOT
			Unprintable timestamp.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	if(0 == gl_strtime(&dt, time_str, sizeof(time_str) ) ){
		gl_error("violation_recorder::write_line(): error when writing the sync time as a string");
		/* TROUBLESHOOT
			Error printing the timestamp.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	char buffer[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(buffer,fmt,ptr); /* note the lack of check on buffer overrun */
	va_end(ptr);
	// print line to file
	if(0 >= fprintf(rec_file, "%s,%s\n", time_str, buffer)){
		gl_error("violation_recorder::write_line(): error when writing to the output file");
		/* TROUBLESHOOT
			File I/O error.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	++write_count;
	// if periodic flush, check for flush
	if(flush_interval > 0){
		if(last_flush + flush_interval <= t1){
			last_flush = t1;
		}
	} else if(flush_interval < 0){
		if( ((write_count + 1) % (-flush_interval)) == 0 ){
			flush_line();
		}
	} // if 0, no flush
	// check if write limit
	if(limit > 0 && write_count >= limit){
		// write footer
		write_footer();
		fclose(rec_file);
		rec_file = 0;
		free(line_buffer);
		line_buffer = 0;
		line_size = 0;
		tape_status = TS_DONE;
	}
	if (echo)
		gl_output("%s", buffer);

	return 1;
}

/**
	@return 0 on failure, 1 on success
 **/
int violation_recorder::flush_line(){
	if(TS_OPEN != tape_status){
		gl_error("violation_recorder::flush_line(): tape is not open");
		// could be ERROR or CLOSED, should not have happened
		return 0;
	}
	if(0 == rec_file){
		gl_error("violation_recorder::flush_line(): output file is not open");
		/* TROUBLESHOOT
			violation_recorder claimed to be open and attempted to flush to a file when
			a file had not successfully	opened.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	if(0 != fflush(rec_file)){
		gl_error("violation_recorder::flush_line(): unable to flush output file");
		/* TROUBLESHOOT
			An IO error has occured.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	return 1;
}

/**
	@return 0 on failure, 1 on success
 **/

int violation_recorder::write_summary() {
	FILE *f;

	f = fopen(summary.get_string(), "w");
	if(0 == f){
		gl_warning("violation_recorder::init(): unable to open file '%s' for writing", summary.get_string());
		return 0;
	}

	char buffer[1024];
	time_t now = time(NULL);

	if(0 > fprintf(f,"# file...... %s\n", summary.get_string())){ return 0; }
	if(0 > fprintf(f,"# date...... %s", asctime(localtime(&now)))){ return 0; }
#ifdef _WIN32
	if(0 > fprintf(f,"# user...... %s\n", getenv("USERNAME"))){ return 0; }
	if(0 > fprintf(f,"# host...... %s\n", getenv("MACHINENAME"))){ return 0; }
#else
	if(0 > fprintf(f,"# user...... %s\n", getenv("USER"))){ return 0; }
	if(0 > fprintf(f,"# host...... %s\n", getenv("HOST"))){ return 0; }
#endif
	if(0 > fprintf(f,"VIOLATION1 TOTAL,%i\n", get_violation_count(VIOLATION1))){ return 0; }
	if (xfrmr_list_v1 != NULL)
	{
		if(0 > fprintf(f,"    TRANSFORMER (%i of %i transformers in violation),%i\n", xfrmr_list_v1->length(), xfrmr_obj_list->length(), get_violation_count(VIOLATION1,XFMR))){ return 0; }
	}
	if (ohl_list_v1 != NULL)
	{
		if(0 > fprintf(f,"    OVERHEAD LINE (%i of %i lines in violation),%i\n", ohl_list_v1->length(), ohl_obj_list->length(), get_violation_count(VIOLATION1,OHLN))){ return 0; }
	}
	if (ugl_list_v1 != NULL)
	{
		if(0 > fprintf(f,"    UNDERGROUND LINE (%i of %i lines in violation),%i\n", ugl_list_v1->length(), ugl_obj_list->length(), get_violation_count(VIOLATION1,UGLN))){ return 0; }
	}
	if (tplxl_list_v1 != NULL)
	{
		if(0 > fprintf(f,"    TRIPLEX LINE (%i of %i lines in violation),%i\n", tplxl_list_v1->length(), tplxl_obj_list->length(), get_violation_count(VIOLATION1,TPXL))){ return 0; }
	}
	if(0 > fprintf(f,"VIOLATION2 TOTAL,%i\n", get_violation_count(VIOLATION2))){ return 0; }
	if (node_list_v2 != NULL)
	{
		if(0 > fprintf(f,"    NODE (%i of %i nodes in violation),%i\n", node_list_v2->length(), node_obj_list->length(), get_violation_count(VIOLATION2,NODE))){ return 0; }
	}
	if (tplx_node_list_v2 != NULL)
	{
		if(0 > fprintf(f,"    TRIPLEX NODE (%i of %i nodes in violation),%i\n", tplx_node_list_v2->length(), tplx_node_obj_list->length(), get_violation_count(VIOLATION2,TPXN))){ return 0; }
	}
	if (tplx_meter_list_v2 != NULL)
	{
		if(0 > fprintf(f,"    TRIPLEX METER (%i of %i meters in violation),%i\n", tplx_meter_list_v2->length(), tplx_mtr_obj_list->length(), get_violation_count(VIOLATION2,TPXM))){ return 0; }
	}
	if (comm_meter_list_v2 != NULL)
	{
		if(0 > fprintf(f,"    COMMERCIAL METER (%i of %i meters in violation),%i\n", comm_meter_list_v2->length(), comm_mtr_obj_list->length(), get_violation_count(VIOLATION2,CMTR))){ return 0; }
	}
	
	if(0 > fprintf(f,"VIOLATION3 TOTAL,%i\n", get_violation_count(VIOLATION3))){ return 0; }
	if (tplx_node_list_v3 != NULL)
	{
		if(0 > fprintf(f,"    TRIPLEX NODE (%i of %i nodes in violation),%i\n", tplx_node_list_v3->length(), tplx_node_obj_list->length(), get_violation_count(VIOLATION3,TPXN))){ return 0; }
	}
	if (tplx_meter_list_v3 != NULL)
	{
		if(0 > fprintf(f,"    TRIPLEX METER (%i of %i meters in violation),%i\n", tplx_meter_list_v3->length(), tplx_mtr_obj_list->length(), get_violation_count(VIOLATION3,TPXM))){ return 0; }
	}
	if (comm_meter_list_v3 != NULL)
	{
		if(0 > fprintf(f,"    COMMERCIAL METER (%i of %i meters in violation),%i\n", comm_meter_list_v3->length(), comm_mtr_obj_list->length(), get_violation_count(VIOLATION3,CMTR))){ return 0; }
	}

	if(0 > fprintf(f,"VIOLATION4 TOTAL,%i\n", get_violation_count(VIOLATION4))){ return 0; }
	if(0 > fprintf(f,"VIOLATION5 TOTAL,%i\n", get_violation_count(VIOLATION5))){ return 0; }
	if (inverter_list_v6 != NULL)
	{
		if(0 > fprintf(f,"VIOLATION6 TOTAL (%i of %i inverters in violation),%i\n", inverter_list_v6->length(), inverter_obj_list->length(), get_violation_count(VIOLATION6))){ return 0; }
	}
	if(0 > fprintf(f,"VIOLATION7 TOTAL,%i\n", get_violation_count(VIOLATION7))){ return 0; }
	
	if (tplx_meter_list_v7 != NULL)
	{
		if(0 > fprintf(f,"    TRIPLEX METER (%i of %i meters in violation),%i\n", tplx_meter_list_v7->length(), tplx_mtr_obj_list->length(), get_violation_count(VIOLATION7,TPXM))){ return 0; }
	}
	if (comm_meter_list_v7 != NULL)
	{
		if(0 > fprintf(f,"    COMMERCIAL METER (%i of %i meters in violation),%i\n", comm_meter_list_v7->length(), comm_mtr_obj_list->length(), get_violation_count(VIOLATION7,CMTR))){ return 0; }
	}
	
	if(0 > fprintf(f,"VIOLATION8 TOTAL,%i\n", get_violation_count(VIOLATION8))){ return 0; }

	fclose(f);

	return 1;
}

STATUS violation_recorder::finalize(OBJECT *obj) {
	write_summary();
	return SUCCESS;
}

int violation_recorder::write_footer(){
	if(TS_OPEN != tape_status){
		gl_error("violation_recorder::write_footer(): tape is not open");
		// could be ERROR or CLOSED, should not have happened
		return 0;
	}
	if(0 == rec_file){
		gl_error("violation_recorder::write_footer(): output file is not open");
		/* TROUBLESHOOT
			violation_recorder claimed to be open and attempted to write to a file when
			a file had not successfully	opened.
		 */
		tape_status = TS_ERROR;
		return 0;
	}

	// not a lot to this one.
	if(0 >= fprintf(rec_file, "# end of file\n")){ return 0; }

	return 1;
}

//Allocation subfunctions - put into a function because copy/pasting this too many times was arduous
//Allocate a vobjlist
vobjlist *violation_recorder::vobjlist_alloc_fxn(vobjlist *input_list)
{
	OBJECT *obj = OBJECTHDR(this);

	//Null the address, for giggles
	input_list = NULL;

	//Perform the allocation
	input_list = (vobjlist *)gl_malloc(sizeof(vobjlist));

	//Check it
	if (input_list == NULL)
	{
		GL_THROW(const_cast<char *>("violation_recorder:%d %s - Failed to allocate space for object list to check"),obj->id,obj->name ? obj->name : "Unnamed");
		/*  TROUBLESHOOT
		While attempting to allocate the memory for an object list within the violation recorder, an error occurred.  Please check your
		file and try again.  If the error persists, please submit you code and a bug report via the ticketing system.
		*/
	}

	//Call the constructor routine, non-allocating-ly
	new (input_list) vobjlist();

	return input_list;
}

//Allocate a uniqueList
uniqueList *violation_recorder::uniqueList_alloc_fxn(uniqueList *input_unlist)
{
	OBJECT *obj = OBJECTHDR(this);

	//Null the address, for giggles
	input_unlist = NULL;

	//Perform the allocation
	input_unlist = (uniqueList *)gl_malloc(sizeof(uniqueList));

	//Check it
	if (input_unlist == NULL)
	{
		GL_THROW(const_cast<char *>("violation_recorder:%d %s - Failed to allocate space for unique list"),obj->id,obj->name ? obj->name : "Unnamed");
		/*  TROUBLESHOOT
		While attempting to allocate the memory for a list of unique objects within the violation recorder, an error occurred.  Please check your
		file and try again.  If the error persists, please submit you code and a bug report via the ticketing system.
		*/
	}

	//Call the constructor routine, non-allocating-ly
	new (input_unlist) uniqueList();

	return input_unlist;
}

//////////////////////////////


EXPORT int create_violation_recorder(OBJECT **obj, OBJECT *parent){
	int rv = 0;
	try {
		*obj = gl_create_object(violation_recorder::oclass);
		if(*obj != NULL){
			violation_recorder *my = OBJECTDATA(*obj, violation_recorder);
			gl_set_parent(*obj, parent);
			rv = my->create();
		}
	}
	catch (char *msg){
		gl_error("create_violation_recorder: %s", msg);
	}
	catch (const char *msg){
		gl_error("create_violation_recorder: %s", msg);
	}
	catch (...){
		gl_error("create_violation_recorder: unexpected exception caught");
	}
	return rv;
}

EXPORT int init_violation_recorder(OBJECT *obj){
	violation_recorder *my = OBJECTDATA(obj, violation_recorder);
	int rv = 0;
	try {
		rv = my->init(obj->parent);
	}
	catch (char *msg){
		gl_error("init_violation_recorder: %s", msg);
	}
	catch (const char *msg){
		gl_error("init_violation_recorder: %s", msg);
	}
	return rv;
}

EXPORT TIMESTAMP sync_violation_recorder(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass){
	violation_recorder *my = OBJECTDATA(obj, violation_recorder);
	TIMESTAMP rv = 0;
	try {
		switch(pass){
			case PC_PRETOPDOWN:
				rv = TS_NEVER;
				break;
			case PC_BOTTOMUP:
				rv = TS_NEVER;
				break;
			case PC_POSTTOPDOWN:
				rv = my->postsync(obj->clock, t0);
				obj->clock = t0;
				break;
			default:
				throw "invalid pass request";
		}
	}
	catch(char *msg){
		gl_error("sync_violation_recorder: %s", msg);
	}
	catch(const char *msg){
		gl_error("sync_violation_recorder: %s", msg);
	}
	return rv;
}

EXPORT int commit_violation_recorder(OBJECT *obj){
	int rv = 0;
	violation_recorder *my = OBJECTDATA(obj, violation_recorder);
	try {
		rv = my->commit(obj->clock);
	}
	catch (char *msg){
		gl_error("commit_violation_recorder: %s", msg);
	}
	catch (const char *msg){
		gl_error("commit_violation_recorder: %s", msg);
	}
	return rv;
}

EXPORT int isa_violation_recorder(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj, violation_recorder)->isa(classname);
}

EXPORT STATUS finalize_violation_recorder(OBJECT *obj)
{
	violation_recorder *my = OBJECTDATA(obj,violation_recorder);
	try {
		return obj!=NULL ? my->finalize(obj) : FAILED;
	}
	//T_CATCHALL(pw_model,finalize);
	catch (char *msg) {
		gl_error("finalize_violation_recorder" "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return FAILED;
	}
	catch (const char *msg) {
		gl_error("finalize_violation_recorder" "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return FAILED;
	}
	catch (...) {
		gl_error("finalize_violation_recorder" "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed");
		return FAILED;
	}
}

// EOF

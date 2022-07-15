#include "collect.h"

#include <algorithm>
#include <stdexcept>

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF BID collect for primary frequency control
//////////////////////////////////////////////////////////////////////////

collect::collect(void) {
	len_on = 0;
	len_off = 0;
	supervisor_bid_on = NULL;
	supervisor_bid_off = NULL;
	number_of_bids_on = 0;
	number_of_bids_off = 0;
}

collect::~collect(void) {
	delete [] supervisor_bid_on;
	delete [] supervisor_bid_off;
}

void collect::clear(void) {
	number_of_bids_on = 0;
	number_of_bids_off = 0;
}

int collect::submit_on(OBJECT *from, double power, double voltage_deviation, int key, int state) {
	if (len_on == 0) { // if this is the first time, create the array of bid structs
		len_on = 8;
		supervisor_bid_on = new SUPERVISORBID[len_on];
	}
	else if (len_on == number_of_bids_on) { // time to expand the bid array 
		SUPERVISORBID *new_supervisor_bid = new SUPERVISORBID[len_on*2];
		memcpy(new_supervisor_bid,supervisor_bid_on,len_on*sizeof(SUPERVISORBID));

		delete[] supervisor_bid_on;
		supervisor_bid_on = new_supervisor_bid;
		len_on*=2;
	}
	supervisor_bid_on[number_of_bids_on].from = from;
	supervisor_bid_on[number_of_bids_on].voltage_deviation = voltage_deviation;
	supervisor_bid_on[number_of_bids_on].power = power;
	supervisor_bid_on[number_of_bids_on].state = state;

	return ++number_of_bids_on;
}

int collect::submit_off(OBJECT *from, double power, double voltage_deviation, int key, int state) {
	if (len_off == 0) { // if this is the first time, create the array of bid structs
		len_off = 8;
		supervisor_bid_off = new SUPERVISORBID[len_off];
	}
	else if (len_off == number_of_bids_off) { // time to expand the bid array
		SUPERVISORBID *new_supervisor_bid = new SUPERVISORBID[len_off*2];
		memcpy(new_supervisor_bid,supervisor_bid_off,len_off*sizeof(SUPERVISORBID));

		delete[] supervisor_bid_off;
		supervisor_bid_off = new_supervisor_bid;
		len_off*=2;
	}
	supervisor_bid_off[number_of_bids_off].from = from;
	supervisor_bid_off[number_of_bids_off].voltage_deviation = voltage_deviation;
	supervisor_bid_off[number_of_bids_off].power = power;
	supervisor_bid_off[number_of_bids_off].state = state;
	
	return ++number_of_bids_off;
}

int collect::calculate_freq_thresholds(double droop, double nom_freq, double frequency_deadband, int PFC_mode) {
	if (PFC_mode == 1 || PFC_mode == 2) { //under frequengy and both
		double power_cum_on = 0;
		double *device;
		int *mode;
		double temp_trig_freq = 0;
		for(int i=0; i < number_of_bids_on; i++){ //loop to assign trigger frequencies
			power_cum_on += supervisor_bid_on[i].power;
			fetch_double(&device, "trigger_point_under_frequency", supervisor_bid_on[i].from);
			temp_trig_freq = nom_freq - (power_cum_on * droop);
			if (temp_trig_freq > (nom_freq - frequency_deadband))
				temp_trig_freq = nom_freq - frequency_deadband;
			*device = temp_trig_freq;
			fetch_int(&mode, "PFC_mode", supervisor_bid_on[i].from);
			*mode = PFC_mode;
		}
	}
	if (PFC_mode == 0 || PFC_mode == 2) { //over frequency and both
		double power_cum_off = 0;  
		double *device;
		int *mode;
		double temp_trig_freq = 0;
		for(int i=0; i < number_of_bids_off; i++){ //loop to assign trigger frequencies
			power_cum_off += supervisor_bid_off[i].power;
			fetch_double(&device, "trigger_point_over_frequency", supervisor_bid_off[i].from);
			temp_trig_freq = nom_freq + (power_cum_off * droop);
			if (temp_trig_freq < (nom_freq + frequency_deadband))
				temp_trig_freq = nom_freq + frequency_deadband;				
			*device = temp_trig_freq;
			fetch_int(&mode, "PFC_mode", supervisor_bid_off[i].from);
			*mode = PFC_mode;
		}
	}
	if (PFC_mode > 2 || PFC_mode < 0){
		gl_error("we are in an unknown primary frequency mode");
		return -1;
	}
	return 0;
}

int collect::sort(int sort_mode) {
	try {
	for (int i = 0; i < number_of_bids_on; ++i){ //set mode for on bids
		supervisor_bid_on[i].sort_by = sort_mode;
	}
	for (int i = 0; i < number_of_bids_off; ++i){ //set mode for off bids
		supervisor_bid_off[i].sort_by = sort_mode;
	}	
	
	if (sort_mode != 0) { //we need to sort
		if (number_of_bids_on > 0) {
			std::sort(supervisor_bid_on, supervisor_bid_on+number_of_bids_on);
		}
		if (number_of_bids_off > 0) {
			std::sort(supervisor_bid_off, supervisor_bid_off+number_of_bids_off);
		}
	}
	}
	catch (std::logic_error &ex){
		gl_error(ex.what());
	}
	return 0;
}

void collect::fetch_double(double **prop, const char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, name);
	if(*prop == nullptr){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "collect:%i", hdr->id);
		if(*name == static_cast<char>(0))
			sprintf(msg, "%s: collect unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: collect unable to find %s", namestr, name);
		throw(msg);
	}
}

void collect::fetch_int(int **prop, const char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_int32_by_name(parent, name);
	if(*prop == nullptr){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "collect:%i", hdr->id);
		if(*name == static_cast<char>(0))
			sprintf(msg, "%s: collect unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: collect unable to find %s", namestr, name);
		throw(std::runtime_error(msg));
	}
}

// End of collect.cpp

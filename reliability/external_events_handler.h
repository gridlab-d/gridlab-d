/** $Id: external_event_handler.h afisher $
    Copyright (C) 2019 Battelle Memorial Institute
    @file external_event_handler.h
 
 @{
 **/

#ifndef _external_event_handler_H
#define _external_event_handler_H

#include <string>

#include "gridlabd.h"
#include "reliability.h"

class external_event {
public:
	external_event(std::string name, complex fault_impedance, std::string fault_type, std::string fault_object, TIMESTAMP start, TIMESTAMP end) {
		this->name = name;
		this->fault_impedance = fault_impedance;
		this->fault_type = fault_type;
		this->fault_object = gl_get_object(fault_object.c_str());
		this->start = start;
		this->stop = end;
	}
	std::string name;
	complex fault_impedance;
	std::string fault_type;
	OBJECT *fault_object;
	TIMESTAMP start;
	TIMESTAMP stop;
	bool is_valid() {
		if(this-fault_object != NULL) {
			return true;
		} else {
			return false;
		}
	}

};

#endif

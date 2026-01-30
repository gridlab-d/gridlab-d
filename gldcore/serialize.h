#ifndef _SERIALIZE_H
#define _SERIALIZE_H

#include "gridlabd.h"
#include <nlohmann/json.hpp>

// Local definitions for serialization
typedef struct s_objeventdetails_local {
	OBJECT *obj_of_int;
	OBJECT *obj_made_int;
	TIMESTAMP fail_time;
	unsigned int fail_time_ns;
	double fail_time_dbl;
	TIMESTAMP rest_time;
	unsigned int rest_time_ns;
	double rest_time_dbl;
	TIMESTAMP fail_length;
	unsigned int fail_length_ns;
	double fail_length_dbl;
	TIMESTAMP rest_length;
	unsigned int rest_length_ns;
	double rest_length_dbl;
	bool in_fault;
	int implemented_fault;
	int customers_affected;
	int customers_affected_sec;
} OBJEVENTDETAILS_LOCAL;

typedef struct s_relevantstruct_local {
	OBJEVENTDETAILS_LOCAL objdetails;
	char32 event_type;
	struct s_relevantstruct_local *prev;
	struct s_relevantstruct_local *next;
} RELEVANTSTRUCT_LOCAL;

typedef struct s_indices_local {
	char256 MetricName;
	gld_property *MetricLoc;
	gld_property *MetricLocInterval;
} INDEXARRAY_LOCAL;

typedef struct s_custarray_local {
	OBJECT *CustomerObj;
	gld_property *CustInterrupted;
	gld_property *CustInterrupted_Secondary;
} CUSTARRAY_LOCAL;

nlohmann::json serialize_objeventdetails(const OBJEVENTDETAILS_LOCAL *details);
nlohmann::json serialize_indexarray(INDEXARRAY_LOCAL *index);
nlohmann::json serialize_custarray(CUSTARRAY_LOCAL *cust);
int get_int_property(OBJECT *obj, const char *name);

#endif

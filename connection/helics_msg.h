/*
 * helics_msg.h
 *
 *  Created on: Mar 15, 2017
 *      Author: afisher
 */

#ifndef CONNECTION_HELICS_MSG_H_
#define CONNECTION_HELICS_MSG_H_

#include "gridlabd.h"
#include "varmap.h"
#include "connection.h"
#if HAVE_HELICS
#ifdef OPTIONAL
#undef OPTIONAL
#endif
#include <helics/helics98.hpp>
//#include <helics/application_api/CombinationFederate.hpp>
//#include <helics/application_api/Endpoints.hpp>
//#include <helics/application_api/Inputs.hpp>
//#include <helics/application_api/Publications.hpp>
//#include <helics/helics_enums.h>
#endif
#include<sstream>
#include<vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <complex>
#include <json/json.h>
using std::string;
using std::vector;

class helics_msg;

EXPORT void publish_helics_string(OBJECT *helicsMsgObj, string helicsObjName, string helicsData);

class json_publication {
public:
	json_publication(string objName, string propName){
		object_name = objName;
		property_name = propName;
		const char *pObjName = objName.c_str();
		const char *pPropName = propName.c_str();
		char *pObjBuf = new char[strlen(pObjName)+1];
		char *pPropBuf = new char[strlen(pPropName)+1];
		strcpy(pObjBuf, pObjName);
		strcpy(pPropBuf, pPropName);
		if(objName.compare("globals") != 0){
			gl_verbose("helics_msg::init(): Creating publication of property %s for object %s.",pPropBuf,pObjBuf);
			prop = new gld_property(pObjBuf, pPropBuf);
			if(!prop->is_valid()){
				gl_warning("helics_msg::init(): There is no object %s with property %s.",pObjBuf,pPropBuf);
			}
		} else {
			gl_verbose("helics_msg::init(): Creating publication of global property %s.",pPropBuf);
			prop = new gld_property(pPropBuf);
			if(!prop->is_valid()){
				gl_warning("helics_msg::init(): There is no global variable named %s.",pPropBuf);
			}
		}
	}
	string object_name;
	string property_name;
	gld_property *prop;
};
#if HAVE_HELICS
class helics_value_publication {
public:
	helics_value_publication(){
		pObjectProperty = NULL;
	}
	string objectName;
	string propertyName;
	string name;
	gld_property *pObjectProperty;
	helicscpp::Publication HelicsPublication;
};

class helics_value_subscription {
public:
	helics_value_subscription(){
		pObjectProperty = NULL;
	}
	string objectName;
	string propertyName;
	string target;
	gld_property *pObjectProperty;
	helicscpp::Input HelicsSubscription;
};

class helics_endpoint_publication {
public:
	helics_endpoint_publication(){
		pObjectProperty = NULL;
	}
	string objectName;
	string propertyName;
	string name;
	gld_property *pObjectProperty;
	string destination;
	helicscpp::Endpoint HelicsPublicationEndpoint;
};

class helics_endpoint_subscription {
public:
	helics_endpoint_subscription(){
		pObjectProperty = NULL;
	}
	string objectName;
	string propertyName;
	string name;
	gld_property *pObjectProperty;
	helicscpp::Endpoint HelicsSubscriptionEndpoint;
};

class json_helics_value_publication {
public:
	json_helics_value_publication(){
	}
	Json::Value objectPropertyBundle;
	vector<json_publication*> jsonPublications;
	string name;
	helicscpp::Publication HelicsPublication;
};

class json_helics_value_subscription {
public:
	json_helics_value_subscription(){
	}
	string target;
	helicscpp::Input HelicsSubscription;
};

class json_helics_endpoint_publication {
public:
	json_helics_endpoint_publication(){
	}
	Json::Value objectPropertyBundle;
	vector<json_publication*> jsonPublications;
	string name;
	string destination;
	helicscpp::Endpoint HelicsPublicationEndpoint;
};

class json_helics_endpoint_subscription {
public:
	json_helics_endpoint_subscription(){
	}
	string name;
	helicscpp::Endpoint HelicsSubscriptionEndpoint;
};
#endif

typedef enum {
	HMT_GENERAL,
	HMT_JSON,
} HMESSAGETYPE;

class helics_msg : public gld_object {
public:
	GL_ATOMIC(double,version);
	// TODO add published properties here

private:
#if HAVE_HELICS
	vector<helics_value_subscription*> helics_value_subscriptions;
	vector<helics_value_publication*> helics_value_publications;
	vector<helics_endpoint_subscription*> helics_endpoint_subscriptions;
	vector<helics_endpoint_publication*> helics_endpoint_publications;
	vector<json_helics_value_subscription*> json_helics_value_subscriptions;
	vector<json_helics_value_publication*> json_helics_value_publications;
	vector<json_helics_endpoint_subscription*> json_helics_endpoint_subscriptions;
	vector<json_helics_endpoint_publication*> json_helics_endpoint_publications;
#endif
	vector<string> *inFunctionTopics;
	varmap *vmap[14];

	TIMESTAMP last_approved_helics_time;
	TIMESTAMP initial_sim_time;
	TIMESTAMP publish_time;
	double last_delta_helics_time;
	bool exitDeltamode;
	string *federate_configuration_file;
	// TODO add other properties here as needed.

public:
	// required implementations
	helics_msg(MODULE*);
	int create(void);
	int init(OBJECT* parent);
	int precommit(TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t0,TIMESTAMP t1);
	int configure(char *value);
//	int parse_helics_function(char *value, COMMUNICATIONTYPE comstype);
//	void incoming_helics_function(void);
	int publishVariables();
	int subscribeVariables();
	int publishJsonVariables();
	int subscribeJsonVariables();
	char simulationName[1024];
	void term(TIMESTAMP t1);
	TIMESTAMP clk_update(TIMESTAMP t1);
	SIMULATIONMODE deltaInterUpdate(unsigned int delta_iteration_counter, TIMESTAMP t0, unsigned int64 dt);
	SIMULATIONMODE deltaClockUpdate(double t1, unsigned long timestep, SIMULATIONMODE sysmode);
	enumeration message_type;
	int32 publish_period;
#if HAVE_HELICS
	helicscpp::CombinationFederate *gld_helics_federate;
#endif
	// TODO add other event handlers here
public:
	// special variables for GridLAB-D classes
	static CLASS *oclass;
	static helics_msg *defaults;
};

#endif /* CONNECTION_HELICS_MSG_H_ */

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
//#ifdef min
//#undef min
//#endif
#ifdef OPTIONAL
#undef OPTIONAL
#endif
#include <helics/application_api/CombinationFederate.hpp>
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

///< Function relays
/*typedef struct s_functionsrelay {
	char localclass[64]; ///< local class info
	char localcall[64]; ///< local function call address (NULL for outgoing)
	char remoteclass[64]; ///< remote class name
	char remotename[64]; ///< remote function name
	helics_msg *route; ///< routing of relay
	TRANSLATOR *xlate; ///< output translation call
	struct s_functionsrelay *next;
	DATAEXCHANGEDIRECTION drtn;
	COMMUNICATIONTYPE ctype; ///<identifies which helics communication function to call. Only used for communicating with helics.
} FUNCTIONSRELAY;*/
//extern "C" FUNCTIONADDR add_helics_function(helics_msg *route, const char *fclass,const char *flocal, const char *rclass, const char *rname, TRANSLATOR *xlate, DATAEXCHANGEDIRECTION direction, COMMUNICATIONTYPE ctype );
//extern "C" FUNCTIONSRELAY *find_helics_function(const char *rclass, const char *rname);
//extern "C" size_t helics_from_hex(void *buf, size_t len, const char *hex, size_t hexlen);

#if HAVE_HELICS
class helics_value_publication {
public:
	helics_value_publication(){
		pObjectProperty = NULL;
		pHelicsPublicationId = NULL;
	}
	string objectName;
	string propertyName;
	string topicName;
	gld_property *pObjectProperty;
	helics::publication_id_t pHelicsPublicationId;
};

class helics_value_subscription {
public:
	helics_value_subscription(){
		pObjectProperty = NULL;
		pHelicsSubscriptionId = NULL;
	}
	string objectName;
	string propertyName;
	string subscription_topic;
	gld_property *pObjectProperty;
	helics::subscription_id_t pHelicsSubscriptionId;
};

class helics_endpoint_publication {
public:
	helics_endpoint_publication(){
		pObjectProperty = NULL;
		pHelicsPublicationEndpointId = NULL;
	}
	string objectName;
	string propertyName;
	string topicName;
	gld_property *pObjectProperty;
	string destination;
	helics::endpoint_id_t pHelicsPublicationEndpointId;
};

class helics_endpoint_subscription {
public:
	helics_endpoint_subscription(){
		pObjectProperty = NULL;
		pHelicsSubscriptionEndpointId = NULL;
	}
	string objectName;
	string propertyName;
	string subscription_topic;
	gld_property *pObjectProperty;
	helics::endpoint_id_t pHelicsSubscriptionEndpointId;
};
#endif
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
#endif
	vector<string> *inFunctionTopics;
	varmap *vmap[14];
#if HAVE_HELICS
	helics::CombinationFederate *helics_federate;
#endif
	TIMESTAMP last_approved_helics_time;
	TIMESTAMP initial_sim_time;
	double last_delta_helics_time;
	bool exitDeltamode;
	string *core_init_string;
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
	char simulationName[1024];
	void term(TIMESTAMP t1);
	TIMESTAMP clk_update(TIMESTAMP t1);
	SIMULATIONMODE deltaInterUpdate(unsigned int delta_iteration_counter, TIMESTAMP t0, unsigned int64 dt);
	SIMULATIONMODE deltaClockUpdate(double t1, unsigned long timestep, SIMULATIONMODE sysmode);
	// TODO add other event handlers here
public:
	// special variables for GridLAB-D classes
	static CLASS *oclass;
	static helics_msg *defaults;
};

#endif /* CONNECTION_HELICS_MSG_H_ */

/*
 * helics_msg.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: afisher
 */
/** $Id$
 * HELICS message object
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "helics_msg.h"

EXPORT_CREATE(helics_msg);
EXPORT_INIT(helics_msg);
EXPORT_PRECOMMIT(helics_msg);
EXPORT_SYNC(helics_msg);
EXPORT_COMMIT(helics_msg);
EXPORT_LOADMETHOD(helics_msg,configure);
static helics::CombinationFederate *pHelicsFederate;
EXPORT TIMESTAMP clocks_update(void *ptr, TIMESTAMP t1)
{
	helics_msg*my = (helics_msg*)ptr;
	return my->clk_update(t1);
}

EXPORT SIMULATIONMODE dInterupdate(void *ptr, unsigned int dIntervalCounter, TIMESTAMP t0, unsigned int64 dt)
{
	helics_msg *my = (helics_msg *)ptr;
	return my->deltaInterUpdate(dIntervalCounter, t0, dt);
}

EXPORT SIMULATIONMODE dClockupdate(void *ptr, double t1, unsigned long timestep, SIMULATIONMODE sysmode)
{
	helics_msg *my = (helics_msg *)ptr;
	return my->deltaClockUpdate(t1, timestep, sysmode);
}

//static FUNCTIONSRELAY *first_helicsfunction = NULL;

CLASS *helics_msg::oclass = NULL;
helics_msg *helics_msg::defaults = NULL;

//Constructor
helics_msg::helics_msg(MODULE *module)
{
	// register to receive notice for first top down. bottom up, and second top down synchronizations
	oclass = gld_class::create(module,"helics_msg",sizeof(helics_msg),PC_AUTOLOCK|PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_OBSERVER);
	if (oclass == NULL)
		throw "connection/helics_msg::helics_msg(MODULE*): unable to register class connection:helics_msg";
	else
		oclass->trl = TRL_UNKNOWN;

	defaults = this;
	if (gl_publish_variable(oclass,
		PT_double, "version", get_version_offset(), PT_DESCRIPTION, "fncs_msg version",
		// TODO add published properties here
		NULL)<1)
			throw "connection/helics_msg::helics_msg(MODULE*): unable to publish properties of connection:helics_msg";
	if ( !gl_publish_loadmethod(oclass,"configure",loadmethod_helics_msg_configure) )
		throw "connection/helics_msg::helics_msg(MODULE*): unable to publish configure method of connection:helics_msg";
}

int helics_msg::create(){
	add_clock_update((void *)this,clocks_update);
	//register_object_interupdate((void *)this, dInterupdate);
	//register_object_deltaclockupdate((void *)this, dClockupdate);


	return 1;
}

int helics_msg::configure(char *value)
{
	int rv = 1;
	char1024 configFile;
	strcpy(configFile, value);
	if (strcmp(configFile, "") != 0) {
		ifstream ifile;
		string confLine;
		string value = "";
		helics_value_publication *pub = NULL;
		helics_value_subscription *sub = NULL;
		helics_endpoint_publication *ep_pub = NULL;
		helics_endpoint_subscription *ep_sub = NULL;
		string object_name_temp = "";
		string property_name_temp = "";
		Json::ValueIterator it;
		Json::Value publish_json_config;

		char buf[1024];
		char cval[1024];

		ifile.open(configFile, ifstream::in);
		if (ifile.good()) {
			stringstream json_config_stream ("");
			string json_config_line;
			string json_config_string;
			Json::Reader json_reader;
			while (ifile >> json_config_line) { //Place the entire contents of the file into a stringstream
				json_config_stream << json_config_line;
			}
			json_config_string = json_config_stream.str();
			gl_verbose("helics_msg::configure(): json string read from configure file: %s .\n", json_config_string.c_str()); //renke debug
			json_reader.parse(json_config_string, publish_json_config);
			if(publish_json_config.isMember("publications")) {
				for(it = publish_json_config["publications"].begin(); it != publish_json_config["publications"].end(); ++it) {
					object_name_temp = it.name();
					for(Json::ValueIterator it1 = publish_json_config["publications"][it.name()].begin(); it1 != publish_json_config["publications"][it.name()].end(); ++it1) {
						pub = new helics_value_publication();
						pub->objectName = object_name_temp;
						pub->propertyName = it1.name();
						pub->topicName = publish_json_config["publications"][it.name()][it1.name()].asString();
						helics_value_publications.push_back(pub);
					}
				}
			}
			if(publish_json_config.isMember("subscriptions")) {
				for(it = publish_json_config["subscriptions"].begin(); it != publish_json_config["subscriptions"].end(); ++it) {
					object_name_temp = it.name();

					for(Json::ValueIterator it1 = publish_json_config["subscriptions"][it.name()].begin(); it1 != publish_json_config["subscriptions"][it.name()].end(); ++it1) {
						sub = new helics_value_subscription();
						sub->objectName = object_name_temp;
						sub->propertyName = it1.name();
						sub->subscription_topic = publish_json_config["subscriptions"][it.name()][it1.name()].asString();
						helics_value_subscriptions.push_back(sub);
					}
				}
			}
			if(publish_json_config.isMember("endpoint_publications")) {
				for(it = publish_json_config["endpoint_publications"].begin(); it != publish_json_config["endpoint_publications"].end(); ++it) {
					object_name_temp = it.name();
					for(Json::ValueIterator it1 = publish_json_config["endpoint_publications"][it.name()].begin(); it1 != publish_json_config["endpoint_publications"][it.name()].end(); ++it1) {
						ep_pub = new helics_endpoint_publication();
						ep_pub->objectName = object_name_temp;
						ep_pub->propertyName = it1.name();
						ep_pub->topicName = ep_pub->objectName + "/" + ep_pub->propertyName;
						ep_pub->destination = publish_json_config["endpoint_publications"][it.name()][it1.name()].asString();
						helics_endpoint_publications.push_back(ep_pub);
					}
				}
			}
			if(publish_json_config.isMember("endpoint_subscriptions")) {
				for(it = publish_json_config["endpoint_subscriptions"].begin(); it != publish_json_config["endpoint_subscriptions"].end(); ++it) {
					object_name_temp = it.name();

					for(Json::ValueIterator it1 = publish_json_config["endpoint_subscriptions"][it.name()].begin(); it1 != publish_json_config["endpoint_subscriptions"][it.name()].end(); ++it1) {
						ep_sub = new helics_endpoint_subscription();
						ep_sub->objectName = object_name_temp;
						ep_sub->propertyName = it1.name();
						ep_sub->subscription_topic = publish_json_config["endpoint_subscriptions"][it.name()][it1.name()].asString();
						helics_endpoint_subscriptions.push_back(ep_sub);
					}
				}
			}
			if(publish_json_config.isMember("core_init_string")) {
				core_init_string = publish_json_config["core_init_string"].asString();
			} else {
				core_init_string = "1";
			}
			if (rv == 0) {
				return 0;
			}
		}
		else {
			gl_error("helics_msg::configure(): failed to open the configuration file %s \n", (char *)configFile);
			rv = 0;
		} // end of if (ifile.good())
	}

	return rv;
}

void send_die(void)
{
	//need to check the exit code. send die with an error exit code.
	int a;
	a = 0;
	gld_global exitCode("exit_code");
	if(exitCode.get_int16() != 0){
		//TODO find equivalent helics die message
#if HAVE_HELICS
		gl_verbose("helics_msg: Calling error");
		pHelicsFederate->error((int)(exitCode.get_int16()));
#endif
	}/* else {
		//TODO find equivalent helics clean exit message
#if HAVE_HELICS
		gl_verbose("helics_msg: Calling finalize\n");
		pHelicsFederate->finalize();
#endif
	}*/
}

int helics_msg::init(OBJECT *parent){

	gl_verbose("entering helics_msg::init()");

	int rv;
#if HAVE_HELICS
	rv = 1;
#else
	gl_error("helics_msg::init ~ helics was not linked with GridLAB-D at compilation. helics_msg cannot be used if helics was not linked with GridLAB-D.");
	rv = 0;
#endif
	if (rv == 0)
	{
		return 0;
	}
	//write zplfile
	bool defer = false;
	OBJECT *obj = OBJECTHDR(this);
	OBJECT *vObj = NULL;
	char buffer[1024] = "";
	string simName = string(gl_name(obj, buffer, 1023));
	string dft;
	char defaultBuf[1024] = "";
	string type;
	string gld_prop_string = "";
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		if((*pub)->pObjectProperty == NULL) {
			const char *pObjName = (*pub)->objectName.c_str();
			const char *pPropName = (*pub)->propertyName.c_str();
			char *pObjBuf = new char[strlen(pObjName)+1];
			char *pPropBuf = new char[strlen(pPropName)+1];
			strcpy(pObjBuf, pObjName);
			strcpy(pPropBuf, pPropName);
			(*pub)->pObjectProperty = new gld_property(pObjBuf, pPropBuf);
			if(!(*pub)->pObjectProperty->is_valid()) {
				rv = 0;
				gl_error("helics_msg::init(): There is not object %s with property %s",(char *)(*pub)->objectName.c_str(), (char *)(*pub)->propertyName.c_str());
				break;
			}
		}
	}
	if(rv == 0) {
		return rv;
	}
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++) {
		if((*sub)->pObjectProperty == NULL) {
			const char *pObjName = (*sub)->objectName.c_str();
			const char *pPropName = (*sub)->propertyName.c_str();
			char *pObjBuf = new char[strlen(pObjName)+1];
			char *pPropBuf = new char[strlen(pPropName)+1];
			strcpy(pObjBuf, pObjName);
			strcpy(pPropBuf, pPropName);
			(*sub)->pObjectProperty = new gld_property(pObjBuf, pPropBuf);
			if(!(*sub)->pObjectProperty->is_valid()) {
				rv = 0;
				gl_error("helics_msg::init(): There is not object %s with property %s",(char *)(*sub)->objectName.c_str(), (char *)(*sub)->propertyName.c_str());
				break;
			}
		}
	}
	if(rv == 0) {
		return rv;
	}
	for(vector<helics_endpoint_publication*>::iterator pub = helics_endpoint_publications.begin(); pub != helics_endpoint_publications.end(); pub++) {
		if((*pub)->pObjectProperty == NULL) {
			const char *pObjName = (*pub)->objectName.c_str();
			const char *pPropName = (*pub)->propertyName.c_str();
			char *pObjBuf = new char[strlen(pObjName)+1];
			char *pPropBuf = new char[strlen(pPropName)+1];
			strcpy(pObjBuf, pObjName);
			strcpy(pPropBuf, pPropName);
			(*pub)->pObjectProperty = new gld_property(pObjBuf, pPropBuf);
			if(!(*pub)->pObjectProperty->is_valid()) {
				rv = 0;
				gl_error("helics_msg::init(): There is not object %s with property %s",(char *)(*pub)->objectName.c_str(), (char *)(*pub)->propertyName.c_str());
				break;
			}
		}
	}
	if(rv == 0) {
		return rv;
	}
	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++) {
		if((*sub)->pObjectProperty == NULL) {
			const char *pObjName = (*sub)->objectName.c_str();
			const char *pPropName = (*sub)->propertyName.c_str();
			char *pObjBuf = new char[strlen(pObjName)+1];
			char *pPropBuf = new char[strlen(pPropName)+1];
			strcpy(pObjBuf, pObjName);
			strcpy(pPropBuf, pPropName);
			(*sub)->pObjectProperty = new gld_property(pObjBuf, pPropBuf);
			if(!(*sub)->pObjectProperty->is_valid()) {
				rv = 0;
				gl_error("helics_msg::init(): There is not object %s with property %s",(char *)(*sub)->objectName.c_str(), (char *)(*sub)->propertyName.c_str());
				break;
			}
		}
	}
	if(rv == 0) {
		return rv;
	}
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		vObj = (*pub)->pObjectProperty->get_object();
		if((vObj->flags & OF_INIT) != OF_INIT){
			defer = true;
		}
	}
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++) {
		vObj = (*sub)->pObjectProperty->get_object();
		if((vObj->flags & OF_INIT) != OF_INIT){
			defer = true;
		}
	}
	for(vector<helics_endpoint_publication*>::iterator pub = helics_endpoint_publications.begin(); pub != helics_endpoint_publications.end(); pub++) {
		vObj = (*pub)->pObjectProperty->get_object();
		if((vObj->flags & OF_INIT) != OF_INIT){
			defer = true;
		}
	}
	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++) {
		vObj = (*sub)->pObjectProperty->get_object();
		if((vObj->flags & OF_INIT) != OF_INIT){
			defer = true;
		}
	}
	if(defer == true){
		gl_verbose("helics_msg::init(): %s is defering initialization.", obj->name);
		return 2;
	}
	//get a string vector of the unique function subscriptions
/*	for(relay = first_helicsfunction; relay != NULL; relay = relay->next){
		if(relay->drtn == DXD_READ){
			uniqueTopic = true;
			for(i = 0; i < inFunctionTopics->size(); i++){
				if((*inFunctionTopics)[i].compare(string(relay->remotename)) == 0){
					uniqueTopic = false;
				}
			}
			if(uniqueTopic == true){
				inFunctionTopics->push_back(string(relay->remotename));
				zplfile << "    " << string(relay->remotename) << endl;
				zplfile << "        topic = " << string(relay->remotename) << endl;
				zplfile << "        list = true" << endl;
			}
		}
	}*/
	//register with helics
#if HAVE_HELICS
	helics::FederateInfo helics_config;
	helics_config.name = string(obj->name);
	helics_config.uninterruptible = false;
	helics_config.timeDelta = 1.0;//1 second
	//helics_config.lookAhead = 1.0;
	//:helics_config.impactWindow = 1.0;
	helics_config.coreType = helics::core_type::ZMQ;
	helics_config.coreInitString = core_init_string;
	gl_verbose("helics_msg: Calling ValueFederate Constructor");
	helics_federate = new helics::CombinationFederate(helics_config);
	pHelicsFederate = helics_federate;
	//register helics publications
	string pub_sub_name = "";
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		if((*pub)->pObjectProperty->is_complex()) {
			gl_verbose("helics_msg: Calling registerPubliscation<std::complex>\n");
			(*pub)->pHelicsPublicationId = helics_federate->registerPublication((*pub)->topicName, "complex", "");
		} else if((*pub)->pObjectProperty->is_integer()) {
			gl_verbose("helics_msg: Calling registerPubliscation<int64>\n");
			(*pub)->pHelicsPublicationId = helics_federate->registerPublication((*pub)->topicName, "int", "");
		} else if((*pub)->pObjectProperty->is_double()) {
			gl_verbose("helics_msg: Calling registerPubliscation<double>\n");
			(*pub)->pHelicsPublicationId = helics_federate->registerPublication((*pub)->topicName, "double", "");
		} else {
			gl_verbose("helics_msg: Calling registerPubliscation<string>\n");
			(*pub)->pHelicsPublicationId = helics_federate->registerPublication((*pub)->topicName, "string", "");
		}
	}
	//register helics subscriptions
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++) {
		pub_sub_name.clear();
		pub_sub_name.append((*sub)->subscription_topic);
		if((*sub)->pObjectProperty->is_complex()) {
			gl_verbose("helics_msg: Calling registerOptionalSubscription<std::complex>\n");
			(*sub)->pHelicsSubscriptionId = helics_federate->registerRequiredSubscription(pub_sub_name, "complex", "");
		} else if((*sub)->pObjectProperty->is_integer()) {
			gl_verbose("helics_msg: Calling registerOptionalSubscription<int64>\n");
			(*sub)->pHelicsSubscriptionId = helics_federate->registerRequiredSubscription(pub_sub_name, "int64", "");
		} else if((*sub)->pObjectProperty->is_double()) {
			gl_verbose("helics_msg: Calling registerOptionalSubscription<double>\n");
			(*sub)->pHelicsSubscriptionId = helics_federate->registerRequiredSubscription(pub_sub_name, "double", "");
		} else {
			gl_verbose("helics_msg: Calling registerOptionalSubscription<string>\n");
			(*sub)->pHelicsSubscriptionId = helics_federate->registerRequiredSubscription(pub_sub_name, "string", "");
		}
	}
	// register helics output endpoints
	for(vector<helics_endpoint_publication*>::iterator pub = helics_endpoint_publications.begin(); pub != helics_endpoint_publications.end(); pub++) {
		gl_verbose("helics_msg: Calling registerEndpoint");
		(*pub)->pHelicsPublicationEndpointId = helics_federate->registerEndpoint((*pub)->topicName, "string");
	}
	// register helics output endpoints
	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++) {
		pub_sub_name.clear();
		pub_sub_name.append((*sub)->subscription_topic);
		gl_verbose("helics_msg: Calling registerOptionalSubscription");
		(*sub)->pHelicsSubscriptionEndpointId = helics_federate->registerEndpoint(pub_sub_name, "string");
	}
	//TODO Call finished initializing
	gl_verbose("helics_msg: Calling enterInitializationState");
	helics_federate->enterInitializationState();
	gl_verbose("helics_msg: Calling enterExecutionState");
	helics_federate->enterExecutionState();
#endif
	atexit(send_die);
	last_approved_helics_time = gl_globalclock;
	last_delta_helics_time = (double)(gl_globalclock);
	initial_sim_time = gl_globalclock;
	return rv;
}

int helics_msg::precommit(TIMESTAMP t1){
	int result = 0;

	result = subscribeVariables();
	if(result == 0){
		return result;
	}

	return 1;
}

TIMESTAMP helics_msg::presync(TIMESTAMP t1){
	return TS_NEVER;
}


TIMESTAMP helics_msg::sync(TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP helics_msg::postsync(TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP helics_msg::commit(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

SIMULATIONMODE helics_msg::deltaInterUpdate(unsigned int delta_iteration_counter, TIMESTAMP t0, unsigned int64 dt)
{
	int result = 0;
	gld_global dclock("deltaclock");
	if (!dclock.is_valid()) {
		gl_error("helics_msg::deltaInterUpdate: Unable to find global deltaclock!");
		return SM_ERROR;
	}
	if(dclock.get_int64() > 0){
		if(delta_iteration_counter == 0){
			//precommit: read variables from cache
			result = subscribeVariables();
			if(result == 0){
				return SM_ERROR;
			}
			return SM_DELTA_ITER;
		}

		if(delta_iteration_counter == 1)
		{
			return SM_DELTA_ITER;
		}

		if(delta_iteration_counter == 2)
		{
			return SM_DELTA_ITER;
		}

		if(delta_iteration_counter == 3)
		{
			return SM_DELTA_ITER;
		}

		if(delta_iteration_counter == 4)
		{
			//post_sync: publish variables
			result = publishVariables();
			if(result == 0){
				return SM_ERROR;
			}
		}
	}
	return SM_EVENT;
}

SIMULATIONMODE helics_msg::deltaClockUpdate(double t1, unsigned long timestep, SIMULATIONMODE sysmode)
{
#if HAVE_HELICS
	if (t1 > last_delta_helics_time){
//		helics::time helics_time = 0;
		helics::Time helics_time = 0;
//		helics::time t = 0;
		helics::Time t = 0;
		double dt = 0;
		dt = (t1 - (double)initial_sim_time);
//		t = (helics::time)((dt + ((double)(timestep) / 2.0)) - fmod((dt + ((double)(timestep) / 2.0)), (double)timestep));
		t = (helics::Time)(dt);
		helics_federate->setTimeDelta((helics::Time)(((double)timestep)/DT_SECOND));
		helics_time = helics_federate->requestTime(t);
		//TODO call helics time update function
		if(sysmode == SM_EVENT)
			exitDeltamode = true;
		if(helics_time != t){
			gl_error("helics_msg::deltaClockUpdate: Cannot return anything other than the time GridLAB-D requested in deltamode.");
			return SM_ERROR;
		} else {
			last_delta_helics_time = (double)(helics_time) + (double)(initial_sim_time);
		}
	}
#endif
	if(sysmode == SM_DELTA) {
		return SM_DELTA;
	} else if (sysmode == SM_EVENT) {
		return SM_EVENT;
	} else {
		return SM_ERROR;
	}
}

TIMESTAMP helics_msg::clk_update(TIMESTAMP t1)
{
	// TODO move t1 back if you want, but not to global_clock or before
	TIMESTAMP helics_time = 0;
	if(exitDeltamode == true){
#if HAVE_HELICS
		//TODO update time delta in helics
		gl_verbose("helics_msg: Calling setTimeDelta");
		helics_federate->setTimeDelta(1.0);
#endif
		exitDeltamode = false;
		return t1;
	}
	if(t1 > last_approved_helics_time){
		int result = 0;
		result = publishVariables();
		if(result == 0){
			return TS_INVALID;
		}
		if(gl_globalclock == gl_globalstoptime){
#if HAVE_HELICS
			gl_verbose("helics_msg: Calling finalize");
			pHelicsFederate->finalize();
#endif
			return t1;
		} else if (t1 > gl_globalstoptime && gl_globalclock < gl_globalstoptime){
			t1 == gl_globalstoptime;
		}
#if HAVE_HELICS
		helics::Time t((double)((t1 - initial_sim_time)));
//		helics_time = ((TIMESTAMP)helics::time_request(t))/1000000000 + initial_sim_time;
		//TODO call appropriate helics time update function
		gl_verbose("helics_msg: Calling requestime");
		helics::Time rt;
		rt = helics_federate->requestTime(t);
		helics_time = (TIMESTAMP)rt + initial_sim_time;
#endif
		if(helics_time <= gl_globalclock){
			gl_error("helics_msg::clock_update: Cannot return the current time or less than the current time.");
			return TS_INVALID;
		} else {
			last_approved_helics_time = helics_time;
			t1 = helics_time;
		}
	}
	return t1;
}

/*int helics_msg::helics_link(char *value, COMMUNICATIONTYPE comtype){
	int rv = 0;
	int n = 0;
	char command[1024] = "";
	char argument[1024] = "";
	VARMAP *mp = NULL;
	//parse argument to fill the relay function link list and the varmap link list.
	if(sscanf(value, "%[^:]:%[^\n]", command, argument) == 2){
		if(strncmp(command,"init", 4) == 0){
			gl_warning("helics_msg::publish: It is not possible to pass information at init time with helics. communication is ignored");
			rv = 1;
		} else if(strncmp(command, "function", 8) == 0){
			rv = parse_helics_function(argument, comtype);
		} else {
			n = get_varmapindex(command);
			if(n != 0){
				rv = vmap[n]->add(argument, comtype);
			}
		}
	} else {
		gl_error("helics_msg::publish: Unable to parse input %s.", value);
		rv = 0;
	}
	return rv;
}*/
/*int helics_msg::parse_helics_function(char *value, COMMUNICATIONTYPE comtype){
	int rv = 0;
	char localClass[64] = "";
	char localFuncName[64] = "";
	char direction[8] = "";
	char remoteClassName[64] = "";
	char remoteFuncName[64] = "";
	char topic[1024] = "";
	CLASS *fclass = NULL;
	FUNCTIONADDR flocal = NULL;
	if(sscanf(value, "%[^/]/%[^-<>\t ]%*[\t ]%[-<>]%*[\t ]%[^\n]", localClass, localFuncName, direction, topic) != 4){
		gl_error("helics_msg::parse_helics_function: Unable to parse input %s.", value);
		return rv;
	}
	// get local class structure
	fclass = callback->class_getname(localClass);
	if ( fclass==NULL )
	{
		gl_error("helics_msg::parse_helics_function(const char *spec='%s'): local class '%s' does not exist", value, localClass);
		return rv;
	}
	flocal = callback->function.get(localClass, localFuncName);
	// setup outgoing call
	if(strcmp(direction, "->") == 0){
		// check local class function map
		if ( flocal!=NULL )
			gl_warning("helics_msg::parse_helics_function(const char *spec='%s'): outgoing call definition of '%s' overwrites existing function definition in class '%s'",value,localFuncName,localClass);

		sscanf(topic, "%[^/]/%[^\n]", remoteClassName, remoteFuncName);
		// get relay function
		flocal = add_helics_function(this,localClass, localFuncName,remoteClassName,remoteFuncName,NULL,DXD_WRITE, comtype);

		if ( flocal==NULL )
			return rv;

		// define relay function
		rv = callback->function.define(fclass,localFuncName,flocal)!=NULL;
		if(rv == 0){
			gl_error("helics_msg::parse_helics_function(const char *spec='%s'): failed to define the function '%s' in local class '%s'.", value, localFuncName, localClass);
			return rv;
		}
	// setup incoming call
	} else if ( strcmp(direction,"<-")==0 ){
		// check to see is local class function is valid
		if( flocal == NULL){
			gl_error("helics_msg::parse_helics_function(const char *spec='%s'): local function '%s' is not valid.",value, localFuncName);
			return 0;
		}
		flocal = add_helics_function(this, localClass, localFuncName, "", topic, NULL, DXD_READ, comtype);
		if( flocal == NULL){
			rv = 1;
		}
	}
	return rv;
}*/

/*void helics_msg::incoming_helics_function()
{
	FUNCTIONSRELAY *relay = NULL;
	vector<string> functionCalls;
	const char *message;
	char from[64] = "";
	char to[64] = "";
	char funcName[64] = "";
	char payloadString[3000] = "";
	char payloadLengthstr[64] = "";
	int payloadLength = 0;
	int payloadStringLength = 0;
	size_t s = 0;
	size_t rplen = 0;
	OBJECT *obj = NULL;
	FUNCTIONADDR funcAddr = NULL;

	for(relay = first_helicsfunction; relay!=NULL; relay=relay->next){
		if(relay->drtn == DXD_READ){
#if HAVE_HELICS
			//functionCalls = helics::get_values(string(relay->remotename));
			//TODO call appropiate helics function to get value from endpoint?
#endif
			s = functionCalls.size();
			if(s > 0){
				for(int i = 0; i < s; i++){
					message = functionCalls[i].c_str();
					//parse the message
					memset(from,'\0',64);
					memset(to,'\0',64);
					memset(funcName,'\0',64);
					memset(payloadString,'\0',3000);
					memset(payloadLengthstr, '\0', 64);
					if(sscanf(message,"\"{\"from\":\"%[^\"]\", \"to\":\"%[^\"]\", \"function\":\"%[^\"]\", \"data\":\"%[^\"]\", \"data length\":\"%[^\"]\"}\"",from, to, funcName, payloadString,payloadLengthstr) != 5){
						throw("helics_msg::incomming_helics_function: unable to parse function message %s", message);
					}

					//check function is correct
					if(strcmp(funcName, relay->localcall) != 0){
						throw("helics_msg::incomming_helics_function: The remote side function call, %s, is not the same as the local function name, %s.", funcName, relay->localcall);
					}
					payloadLength = atoi(payloadLengthstr);
					payloadStringLength = payloadLength*2;
					void *rawPayload = new char[payloadLength];
					memset(rawPayload, 0, payloadLength);
					//unhex raw payload
					rplen = helics_from_hex(rawPayload, (size_t)payloadLength, payloadString, (size_t)payloadStringLength);
					if( rplen < strlen(payloadString)){
						throw("helics_msg::incomming_helics_function: unable to decode function payload %s.", payloadString);
					}
					//call local function
					obj = gl_get_object(to);
					if( obj == NULL){
						throw("helics_msg::incomming_helics_function: the to object does not exist. %s.", to);
					}
					funcAddr = (FUNCTIONADDR)(gl_get_function(obj, relay->localcall));
					((void (*)(char *, char *, char *, char *, void *, size_t))(*funcAddr))(from, to, relay->localcall, relay->localclass, rawPayload, (size_t)payloadLength);
				}
			}
		}
	}
}*/

//publishes gld properties to the cache
int helics_msg::publishVariables(){
	char buffer[1024] = "";
	memset(&buffer[0], '\0', 1024);
	int buffer_size = 0;
	string temp_value = "";
	stringstream message_buffer_stream;
	std::complex<double> complex_temp = {0.0, 0.0};
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		buffer_size = 0;
#if HAVE_HELICS
		if( (*pub)->pObjectProperty->is_complex() ) {
			double real_part = (*pub)->pObjectProperty->get_part("real");
			double imag_part = (*pub)->pObjectProperty->get_part("imag");
			gld_unit *val_unit = (*pub)->pObjectProperty->get_unit();
			complex_temp = {real_part, imag_part};

			gl_verbose("helics_msg: calling publish<complex<double>>");
			helics_federate->publish<std::complex<double>>((*pub)->pHelicsPublicationId, complex_temp);

		} else if((*pub)->pObjectProperty->is_integer()) {
			int64_t integer_temp = 0;
			(*pub)->pObjectProperty->getp(integer_temp);
			gl_verbose("helics_msg: calling publish<int>");
			helics_federate->publish<int64_t>((*pub)->pHelicsPublicationId, integer_temp);
		} else if((*pub)->pObjectProperty->is_double()) {
			double double_temp = 0;
			(*pub)->pObjectProperty->getp(double_temp);
			gl_verbose("helics_msg: calling publish<complex<double>>");
			helics_federate->publish<double>((*pub)->pHelicsPublicationId, double_temp);
		} else {
			buffer_size = (*pub)->pObjectProperty->to_string(&buffer[0], 1023);
			if(buffer_size <= 0) {
				temp_value = "";
			} else {
				temp_value = string(buffer, (size_t)(buffer_size));
				gl_verbose("helics_msg: Calling publish\n");
				helics_federate->publish<std::string>((*pub)->pHelicsPublicationId, temp_value);
			}
		}
#endif
		memset(&buffer[0], '\0', 1024);
	}

	for(vector<helics_endpoint_publication*>::iterator pub = helics_endpoint_publications.begin(); pub != helics_endpoint_publications.end(); pub++) {
		buffer_size = 0;
		message_buffer_stream.clear();
		if( (*pub)->pObjectProperty->is_complex() ) {
			double real_part = (*pub)->pObjectProperty->get_part("real");
			double imag_part = (*pub)->pObjectProperty->get_part("imag");
			complex_temp = {real_part, imag_part};
			buffer_size = snprintf(&buffer[0], 1023, "%.3f%+.3fj", real_part, imag_part);
		} else {
			buffer_size = (*pub)->pObjectProperty->to_string(&buffer[0], 1023);
		}
		message_buffer_stream << buffer;
		string message_buffer = message_buffer_stream.str();
#if HAVE_HELICS
        try {
			if(helics_federate->getCurrentState() == helics::ValueFederate::op_states::execution){
				gl_verbose("calling helics sendMessage");
				helics::data_view message_data(message_buffer);
				helics_federate->sendMessage((*pub)->pHelicsPublicationEndpointId, (*pub)->destination, message_data);
			}
        } catch (const std::exception& e) { // reference to the base of a polymorphic object
        	gl_error("calling HELICS sendMessage resulted in an unknown error.");
             std::cout << e.what() << std::endl; // information from length_error printed
        }
#endif
		memset(&buffer[0], '\0', 1024);
	}
	return 1;
}

//read variables from the cache
int helics_msg::subscribeVariables(){
	string value_buffer = "";
	char valueBuf[1024] = "";
	string temp_val = "";
	gld::complex gld_complex_temp(0.0, 0.0);
	int64_t integer_temp = 0;
	double double_temp = 0.0;
	std::complex<double> complex_temp = {0.0, 0.0};
#if HAVE_HELICS
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++){
		try {
			if((*sub)->pObjectProperty->is_complex()) {
				gl_verbose("helics_msg: Calling getValue<complex<double>>\n");
				helics_federate->getValue<std::complex<double>>((*sub)->pHelicsSubscriptionId, complex_temp);
				if(!std::isnan(complex_temp.real()) && !std::isnan(complex_temp.imag())) {
					gld_complex_temp.SetReal(complex_temp.real());
					gld_complex_temp.SetImag(complex_temp.imag());
					(*sub)->pObjectProperty->setp(gld_complex_temp);
				}
			} else if((*sub)->pObjectProperty->is_integer()) {
				gl_verbose("helics_msg: Calling getValue<long long int>\n");
				helics_federate->getValue<int64_t>((*sub)->pHelicsSubscriptionId, integer_temp);
				(*sub)->pObjectProperty->setp(integer_temp);
			} else if((*sub)->pObjectProperty->is_double()) {
				gl_verbose("helics_msg: Calling getValue<double>\n");
				helics_federate->getValue<double>((*sub)->pHelicsSubscriptionId, double_temp);
				if(!std::isnan(double_temp)) {
					(*sub)->pObjectProperty->setp(double_temp);
				}
			} else {
				gl_verbose("helics_msg: Calling getValue<string>\n");
				helics_federate->getValue<string>((*sub)->pHelicsSubscriptionId, value_buffer);
				if(!value_buffer.empty()) {
					strncpy(valueBuf, value_buffer.c_str(), 1023);
					(*sub)->pObjectProperty->from_string(valueBuf);
					memset(&valueBuf[0], '/0', 1024);
				}
			}

		} catch(...) {
			value_buffer = "";
			gl_verbose("helics_msg: Calling getValue<string>");
			helics_federate->getValue<string>((*sub)->pHelicsSubscriptionId, value_buffer);
			if(!value_buffer.empty()){
				strncpy(valueBuf, value_buffer.c_str(), 1023);
				(*sub)->pObjectProperty->from_string(valueBuf);
			}
		}
		value_buffer = "";

	}

	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++){
		if(helics_federate->hasMessage((*sub)->pHelicsSubscriptionEndpointId)){
			helics::Message* mesg;
			for(int i = 0; i < (int) helics_federate->receiveCount((*sub)->pHelicsSubscriptionEndpointId); i++) {
				gl_verbose("calling getMessage()");
				mesg = helics_federate->getMessage((*sub)->pHelicsSubscriptionEndpointId).get();
			}
			const string message_buffer = mesg->to_string();
			if(!message_buffer.empty()){
				strncpy(valueBuf, message_buffer.c_str(), 1023);
				(*sub)->pObjectProperty->from_string(valueBuf);
			}
			value_buffer = "";
		}

	}
#endif

	return 1;
}




/*static char helics_hex(char c)
{
	if ( c<10 ) return c+'0';
	else if ( c<16 ) return c-10+'A';
	else return '?';
}

static char helics_unhex(char h)
{
	if ( h>='0' && h<='9' )
		return h-'0';
	else if ( h>='A' && h<='F' )
		return h-'A'+10;
	else if ( h>='a' && h<='f' )
		return h-'a'+10;
}

static size_t helics_to_hex(char *out, size_t max, const char *in, size_t len)
{
	size_t hlen = 0;
	for ( size_t n=0; n<len ; n++,hlen+=2 )
	{
		char byte = in[n];
		char lo = in[n]&0xf;
		char hi = (in[n]>>4)&0xf;
		*out++ = helics_hex(lo);
		*out++ = helics_hex(hi);
		if ( hlen>=max ) return -1; // buffer overrun
	}
	*out = '\0';
	return hlen;
}

extern "C" size_t helics_from_hex(void *buf, size_t len, const char *hex, size_t hexlen)
{
	char *p = (char*)buf;
	char lo = NULL;
	char hi = NULL;
	char c = NULL;
	size_t n = 0;
	for(n = 0; n < hexlen && *hex != '\0'; n += 2)
	{
		c = helics_unhex(*hex);
		if ( c==-1 ) return -1; // bad hex data
		lo = c&0x0f;
		c = helics_unhex(*(hex+1));
		hi = (c<<4)&0xf0;
		if ( c==-1 ) return -1; // bad hex data
		*p = hi|lo;
		p++;
		hex = hex + 2;
		if ( (n/2) >= len ) return -1; // buffer overrun
	}
	return n;
}



/// relay function to handle outgoing function calls
extern "C" void outgoing_helics_function(char *from, char *to, char *funcName, char *funcClass, void *data, size_t len)
{
	int64 result = -1;
	char *rclass = funcClass;
	char *lclass = from;
	size_t hexlen = 0;
	FUNCTIONSRELAY *relay = find_helics_function(funcClass, funcName);
	if(relay == NULL){
		throw("helics_msg::outgoing_route_function: the relay function for function name %s could not be found.", funcName);
	}
	if( relay->drtn != DXD_WRITE){
		throw("helics_msg:outgoing_helics_function: the relay function for the function name ?s could not be found.", funcName);
	}
	char message[3000] = "";

	size_t msglen = 0;

	// check from and to names
	if ( to==NULL || from==NULL )
	{
		throw("from objects and to objects must be named.");
	}

	// convert data to hex
	hexlen = helics_to_hex(message,sizeof(message),(const char*)data,len);

	if(hexlen > 0){
		//TODO: deliver message to helics
		stringstream payload;
		char buffer[sizeof(len)];
		sprintf(buffer, "%d", len);
		payload << "\"{\"from\":\"" << from << "\", " << "\"to\":\"" << to << "\", " << "\"function\":\"" << funcName << "\", " <<  "\"data\":\"" << message << "\", " << "\"data length\":\"" << buffer <<"\"}\"";
		string key = string(relay->remotename);
		if( relay->ctype == CT_PUBSUB){
#if HAVE_HELICS
//			helics::publish(key, payload.str());
			//TODO call appropriate helics function for publishing a value
#endif
		} else if( relay->ctype == CT_ROUTE){
			string sender = string((const char *)from);
			string recipient = string((const char *)to);
#if HAVE_HELICS
//			helics::route(sender, recipient, key, payload.str());
			//TODO call appropriate helics function for publishing a communication message
#endif
		}
	}
}

extern "C" FUNCTIONADDR add_helics_function(helics_msg *route, const char *fclass, const char *flocal, const char *rclass, const char *rname, TRANSLATOR *xlate, DATAEXCHANGEDIRECTION direction, COMMUNICATIONTYPE ctype)
{
	// check for existing of relay (note only one relay is allowed per class pair)
	FUNCTIONSRELAY *relay = find_helics_function(rclass, rname);
	if ( relay!=NULL )
	{
		gl_error("helics_msg::add_helics_function(rclass='%s', rname='%s') a relay function is already defined for '%s/%s'", rclass,rname,rclass,rname);
		return 0;
	}

	// allocate space for relay info
	relay = (FUNCTIONSRELAY*)malloc(sizeof(FUNCTIONSRELAY));
	if ( relay==NULL )
	{
		gl_error("helics_msg::add_helics_function(rclass='%s', rname='%s') memory allocation failed", rclass,rname);
		return 0;
	}

	// setup relay info
	strncpy(relay->localclass,fclass, sizeof(relay->localclass)-1);
	strncpy(relay->localcall,flocal,sizeof(relay->localcall)-1);
	strncpy(relay->remoteclass,rclass,sizeof(relay->remoteclass)-1);
	strncpy(relay->remotename,rname,sizeof(relay->remotename)-1);
	relay->drtn = direction;
	relay->next = first_helicsfunction;
	relay->xlate = xlate;

	// link to existing relay list (if any)
	relay->route = route;
	relay->ctype = ctype;
	first_helicsfunction = relay;

	// return entry point for relay function
	if( direction == DXD_WRITE){
		return (FUNCTIONADDR)outgoing_helics_function;
	} else {
		return NULL;
	}
}

extern "C" FUNCTIONSRELAY *find_helics_function(const char *rclass, const char*rname)
{
	// TODO: this is *very* inefficient -- a hash should be used instead
	FUNCTIONSRELAY *relay;
	for ( relay=first_helicsfunction ; relay!=NULL ; relay=relay->next )
	{
		if (strcmp(relay->remotename, rname)==0 && strcmp(relay->remoteclass, rclass)==0)
			return relay;
	}
	return NULL;
}*/





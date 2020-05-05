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
static helics::CombinationFederate *pHelicsFederate(nullptr);
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
		PT_double, "version", get_version_offset(), PT_DESCRIPTION, "helics_msg version",
		// TODO add published properties here
		NULL)<1)
			throw "connection/helics_msg::helics_msg(MODULE*): unable to publish properties of connection:helics_msg";
	if ( !gl_publish_loadmethod(oclass,"configure",loadmethod_helics_msg_configure) )
		throw "connection/helics_msg::helics_msg(MODULE*): unable to publish configure method of connection:helics_msg";
}

int helics_msg::create(){
	add_clock_update((void *)this,clocks_update);
	register_object_interupdate((void *)this, dInterupdate);
	register_object_deltaclockupdate((void *)this, dClockupdate);


	return 1;
}

int helics_msg::configure(char *value)
{
	int rv = 1;
	char1024 configFile;
	strcpy(configFile, value);
	if (strcmp(configFile, "") != 0) {
		federate_configuration_file = new string(configFile);
	} else {
		gl_error("helics_msg::configure(): No configuration file was give. Please provide a configuration file!");
		rv = 0;
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
		const helics::Federate::modes fed_state = pHelicsFederate->getCurrentMode();
		if(fed_state != helics::Federate::modes::finalize) {
			pHelicsFederate->error((int)(exitCode.get_int16()));
			pHelicsFederate->finalize();
		}
		helics::cleanupHelicsLibrary();
#endif
	} else {
		//TODO find equivalent helics clean exit message
#if HAVE_HELICS
		gl_verbose("helics_msg: Calling finalize (die)\n");
		const helics::Federate::modes fed_state = pHelicsFederate->getCurrentMode();
		if(fed_state != helics::Federate::modes::finalize) {
			pHelicsFederate->finalize();
		}
		helics::cleanupHelicsLibrary();
#endif
	}
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
#if HAVE_HELICS
	if(helics_federate == NULL) {
		try {
			helics_federate = new helics::CombinationFederate(*federate_configuration_file);
			int pub_count = helics_federate->getPublicationCount();
			int sub_count = helics_federate->getInputCount();
			int ep_count = helics_federate->getEndpointCount();
			int idx = 0;
			helics_value_publication *gld_pub;
			helics_value_subscription *gld_sub;
			helics_endpoint_publication *gld_ep_pub;
			helics_endpoint_subscription *gld_ep_sub;
			string config_info_temp = "";
			Json::Reader json_reader;
			Json::Value config_info;
			for( idx = 0; idx < pub_count; idx++ ) {
				helics::Publication pub = helics_federate->getPublication(idx);
				if( pub.isValid() ) {
					gld_pub = new helics_value_publication();
					gld_pub->key = pub.getKey();
					config_info_temp = pub.getInfo();
					json_reader.parse(config_info_temp, config_info);
					gld_pub->objectName = config_info["object"].asString();
					gld_pub->propertyName = config_info["property"].asString();
					gld_pub->HelicsPublication = pub;
					helics_value_publications.push_back(gld_pub);
				}
			}
			for( idx = 0; idx < sub_count; idx++ ) {
				helics::Input sub = helics_federate->getInput(idx);
				if( sub.isValid() ) {
					gld_sub = new helics_value_subscription();
					gld_sub->key = sub.getKey();
					config_info_temp = sub.getInfo();
					json_reader.parse(config_info_temp, config_info);
					gld_sub->objectName = config_info["object"].asString();
					gld_sub->propertyName = config_info["property"].asString();
					gld_sub->HelicsSubscription = sub;
					helics_value_subscriptions.push_back(gld_sub);
				}
			}
			for( idx = 0; idx < ep_count; idx++ ) {
				helics::Endpoint ep = helics_federate->getEndpoint(idx);
				if( ep.isValid() ) {
					string dest = ep.getDefaultDestination();
					config_info_temp = ep.getInfo();
					json_reader.parse(config_info_temp, config_info);
					if( config_info.isMember("object") && config_info.isMember("property") ){
						if( !dest.empty() ){
							gld_ep_pub = new helics_endpoint_publication();
							gld_ep_pub->name = ep.getName();
							gld_ep_pub->destination = dest;
							gld_ep_pub->objectName = config_info["object"].asString();
							gld_ep_pub->propertyName = config_info["property"].asString();
							gld_ep_pub->HelicsPublicationEndpoint = ep;
							helics_endpoint_publications.push_back(gld_ep_pub);
							gl_verbose("helics_msg::init(): registering publishing endpoint: %s", gld_ep_pub->name);
						} else {
							gld_ep_sub = new helics_endpoint_subscription();
							gld_ep_sub->name = ep.getName();
							gld_ep_sub->objectName = config_info["object"].asString();
							gld_ep_sub->propertyName = config_info["property"].asString();
							gld_ep_sub->HelicsSubscriptionEndpoint = ep;
							helics_endpoint_subscriptions.push_back(gld_ep_sub);
							gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", gld_ep_sub->name.c_str());
						}
					}
					if( config_info.isMember("publication_info") ) {
						gld_ep_pub = new helics_endpoint_publication();
						gld_ep_pub->name = ep.getName();
						gld_ep_pub->destination = dest;
						gld_ep_pub->objectName = config_info["publication_info"]["object"].asString();
						gld_ep_pub->propertyName = config_info["publication_info"]["property"].asString();
						gld_ep_pub->HelicsPublicationEndpoint = ep;
						helics_endpoint_publications.push_back(gld_ep_pub);
						gl_verbose("helics_msg::init(): registering publishing endpoint: %s", gld_ep_pub->name);
					}
					if( config_info.isMember("subscription_info") ) {
						gld_ep_sub = new helics_endpoint_subscription();
						gld_ep_sub->name = ep.getName();
						gld_ep_sub->objectName = config_info["subscription_info"]["object"].asString();
						gld_ep_sub->propertyName = config_info["subscription_info"]["property"].asString();
						gld_ep_sub->HelicsSubscriptionEndpoint = ep;
						helics_endpoint_subscriptions.push_back(gld_ep_sub);
						gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", gld_ep_sub->name.c_str());
					}
				}
			}
		} catch(const std::exception &e) {
			gl_error("helics_msg::init: an unexpected error occurred when trying to create a CombinationFederate using the configuration string: \n%s\nThis is the error that was caught:\n%s",federate_configuration_file->c_str(), e.what());
			return 0;
		}
	}
#endif
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
	//register with helics
#if HAVE_HELICS
	pHelicsFederate = helics_federate;

	string pub_sub_name = "";
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		if((*pub)->HelicsPublication.isValid()) {
			if((*pub)->pObjectProperty->is_complex()) {
				if(string("complex").compare((*pub)->HelicsPublication.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered publication %s is intended to publish a complex type but the publication has a type = %s", ((*pub)->key).c_str(), ((*pub)->HelicsPublication.getType()).c_str());
					return 0;
				}
			} else if((*pub)->pObjectProperty->is_integer()) {
				if((*pub)->HelicsPublication.getType().find("int") == string::npos ) {
					gl_error("helics_msg::init: The registered publication %s is intended to publish an integer type but the publication has a type = %s", ((*pub)->key).c_str(), ((*pub)->HelicsPublication.getType()).c_str());
					return 0;
				}
			} else if((*pub)->pObjectProperty->is_double()) {
				if(string("double").compare((*pub)->HelicsPublication.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered publication %s is intended to publish a double type but the publication has a type = %s", ((*pub)->key).c_str(), ((*pub)->HelicsPublication.getType()).c_str());
					return 0;
				}
			} else {
				if(string("string").compare((*pub)->HelicsPublication.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered publication %s is intended to publish a string type but the publication has a type = %s", ((*pub)->key).c_str(), ((*pub)->HelicsPublication.getType()).c_str());
					return 0;
				}
			}
		} else {
			gl_error("helics_msg::init: There is no registered publication with a name = %s", (*pub)->key.c_str());
			return 0;
		}
	}
	//register helics subscriptions
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++) {
		if((*sub)->HelicsSubscription.isValid()) {
			if((*sub)->pObjectProperty->is_complex()) {
				if(string("complex").compare((*sub)->HelicsSubscription.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered subscription %s is intended to subscribe to a complex type but the subscription has a type = %s", ((*sub)->key.c_str()), ((*sub)->HelicsSubscription.getType()).c_str());
					return 0;
				}
			} else if((*sub)->pObjectProperty->is_integer()) {
				if((*sub)->HelicsSubscription.getType().find("int") == string::npos ) {
					gl_error("helics_msg::init: The registered subscription %s is intended to subscribe to an integer type but the subscription has a type = %s", ((*sub)->key.c_str()), ((*sub)->HelicsSubscription.getType()).c_str());
					return 0;
				}
			} else if((*sub)->pObjectProperty->is_double()) {
				if(string("double").compare((*sub)->HelicsSubscription.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered subscription %s is intended to subscribe to double type but the subscription has a type = %s", ((*sub)->key.c_str()), ((*sub)->HelicsSubscription.getType()).c_str());
					return 0;
				}
			} else {
				if(string("string").compare((*sub)->HelicsSubscription.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered subscription %s is intended to subscribe to string type but the subscription has a type = %s", ((*sub)->key.c_str()), ((*sub)->HelicsSubscription.getType()).c_str());
					return 0;
				}
			}
		} else {
			gl_error("helics_msg::init: There is no registered subscription with a key = %s", (*sub)->key.c_str());
			return 0;
		}
	}
	// register helics output endpoints
	for(vector<helics_endpoint_publication*>::iterator pub = helics_endpoint_publications.begin(); pub != helics_endpoint_publications.end(); pub++) {
		if(!(*pub)->HelicsPublicationEndpoint.isValid()){
			gl_error("helics_msg::init: There is no registered endpoint with a name = %s", (*pub)->name.c_str());
			return 0;
		}
	}
	// register helics output endpoints
	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++) {
		if(!(*sub)->HelicsSubscriptionEndpoint.isValid()){
			gl_error("helics_msg::init: There is no registered endpoint with a target = %s", (*sub)->name.c_str());
			return 0;
		}
	}
	//TODO Call finished initializing
	gl_verbose("helics_msg: Calling enterInitializationState");
	helics_federate->enterInitializingMode();
	gl_verbose("helics_msg: Calling enterExecutionState");
	helics_federate->enterExecutingMode();
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
		dt = (t1 - (double)initial_sim_time)*1000000000.0;
		if(sysmode == SM_EVENT) {
			t = (helics::Time)(((dt + (1000000000.0 / 2.0)) - fmod((dt + (1000000000.0 / 2.0)), 1000000000.0))/1000000000.0);
		} else {
			t = (helics::Time)(((dt + ((double)(timestep) / 2.0)) - fmod((dt + ((double)(timestep) / 2.0)), (double)timestep))/1000000000.0);
		}
		helics_federate->setProperty(helics_properties::helics_property_time_period, (helics::Time)(((double)timestep)/DT_SECOND));
		helics_time = helics_federate->requestTime(t);
		//TODO call helics time update function
		if(sysmode == SM_EVENT)
			exitDeltamode = true;
		if(helics_time != t){
			gl_error("helics_msg::deltaClockUpdate: Cannot return anything other than the time GridLAB-D requested in deltamode. Time requested %f. Time returned %f.", (double)t, (double)helics_time);
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
		helics_federate->setProperty(helics_properties::helics_property_time_period, 1.0);// 140 is the option for the period property.
#endif
		exitDeltamode = false;
	}
	if(t1 > last_approved_helics_time){
		int result = 0;
		if(gl_globalclock == gl_globalstoptime){
#if HAVE_HELICS
			gl_verbose("helics_msg: Calling finalize");
			pHelicsFederate->finalize();
#endif
			return t1;
		} else if (t1 > gl_globalstoptime && gl_globalclock < gl_globalstoptime){
			t1 == gl_globalstoptime;
		}
		result = publishVariables();
		if(result == 0){
			return TS_INVALID;
		}
#if HAVE_HELICS
		helics::Time t((double)((t1 - initial_sim_time)));
//		helics_time = ((TIMESTAMP)helics::time_request(t))/1000000000 + initial_sim_time;
		//TODO call appropriate helics time update function
		gl_verbose("helics_msg: Calling requestime");
		gl_verbose("helics_msg: Requesting %f", (double)t);
		helics::Time rt;
		rt = helics_federate->requestTime(t);
		gl_verbose("helics_msg: Granted %f", (double)rt);
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

//publishes gld properties to the cache
int helics_msg::publishVariables(){
	char buffer[1024] = "";
	memset(&buffer[0], '\0', 1024);
	int buffer_size = 0;
	string temp_value = "";
	std::stringstream message_buffer_stream;
	std::complex<double> complex_temp = {0.0, 0.0};
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		buffer_size = 0;
#if HAVE_HELICS
		if( (*pub)->pObjectProperty->is_complex() ) {
			double real_part = (*pub)->pObjectProperty->get_part("real");
			double imag_part = (*pub)->pObjectProperty->get_part("imag");
			gld_unit *val_unit = (*pub)->pObjectProperty->get_unit();
			complex_temp = {real_part, imag_part};

			gl_verbose("helics_msg: calling publish(complex<double>)");
			(*pub)->HelicsPublication.publish(complex_temp);

		} else if((*pub)->pObjectProperty->is_integer()) {
			int64 integer_temp = (*pub)->pObjectProperty->get_integer();
			gl_verbose("helics_msg: calling publishInt()");
			(*pub)->HelicsPublication.publish(integer_temp);
		} else if((*pub)->pObjectProperty->is_double()) {
			double double_temp = 0;
			(*pub)->pObjectProperty->getp(double_temp);
			gl_verbose("helics_msg: calling publish(double)");
			(*pub)->HelicsPublication.publish(double_temp);
		} else {
			buffer_size = (*pub)->pObjectProperty->to_string(&buffer[0], 1023);
			if(buffer_size <= 0) {
				temp_value = "";
			} else {
				temp_value = string(buffer, (size_t)(buffer_size));
				gl_verbose("helics_msg: Calling publish(string)\n");
				(*pub)->HelicsPublication.publish(temp_value);
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
		message_buffer_stream.str(string());
#if HAVE_HELICS
        try {
			if(helics_federate->getCurrentMode() == helics::Federate::modes::executing){
				gl_verbose("calling helics sendMessage");
				helics::data_view message_data(message_buffer);
				(*pub)->HelicsPublicationEndpoint.send(message_data);
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
		if((*sub)->HelicsSubscription.isUpdated()) {
			try {
				if((*sub)->pObjectProperty->is_complex()) {
					gl_verbose("helics_msg: Calling getValue<complex<double>>\n");
					(*sub)->HelicsSubscription.getValue(complex_temp);
					if(!std::isnan(complex_temp.real()) && !std::isnan(complex_temp.imag())) {
						gld_complex_temp.SetReal(complex_temp.real());
						gld_complex_temp.SetImag(complex_temp.imag());
						(*sub)->pObjectProperty->setp(gld_complex_temp);
					}
				} else if((*sub)->pObjectProperty->is_integer()) {
					gl_verbose("helics_msg: Calling getValue<long long int>\n");
					(*sub)->HelicsSubscription.getValue(integer_temp);
					(*sub)->pObjectProperty->setp(integer_temp);
				} else if((*sub)->pObjectProperty->is_double()) {
					gl_verbose("helics_msg: Calling getValue<double>\n");
					(*sub)->HelicsSubscription.getValue(double_temp);
					if(!std::isnan(double_temp)) {
						(*sub)->pObjectProperty->setp(double_temp);
					}
				} else {
					gl_verbose("helics_msg: Calling getValue<string>\n");
					(*sub)->HelicsSubscription.getValue(value_buffer);
					if(!value_buffer.empty()) {
						strncpy(valueBuf, value_buffer.c_str(), 1023);
						(*sub)->pObjectProperty->from_string(valueBuf);
						memset(&valueBuf[0], '\0', 1023);
					}
				}
			} catch(...) {
				value_buffer = "";
				gl_verbose("helics_msg: Calling getValue<string>");
				(*sub)->HelicsSubscription.getValue(value_buffer);
				if(!value_buffer.empty()){
					strncpy(valueBuf, value_buffer.c_str(), 1023);
					(*sub)->pObjectProperty->from_string(valueBuf);
					memset(&valueBuf[0], '\0', 1023);
				}
			}
			value_buffer = "";
		}
	}

	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++){
        gl_verbose("Message status for endpoint %s:  %d", (*sub)->name.c_str(), helics_federate->hasMessage((*sub)->HelicsSubscriptionEndpoint));
        if(helics_federate->hasMessage((*sub)->HelicsSubscriptionEndpoint)){
			std::unique_ptr<helics::Message> mesg;
			int pendingMessages = (int) (*sub)->HelicsSubscriptionEndpoint.pendingMessages();
			for(int i = 0; i < pendingMessages; i++) {
				gl_verbose("calling getMessage() for endpoint %s", (*sub)->name.c_str());
				mesg = (*sub)->HelicsSubscriptionEndpoint.getMessage();
			}
			const string message_buffer = mesg->to_string();
			if(!message_buffer.empty()){
				strncpy(valueBuf, message_buffer.c_str(), 1023);
				(*sub)->pObjectProperty->from_string(valueBuf);
				memset(&valueBuf[0], '\0', 1023);
			}
		}

	}
#endif

	return 1;
}


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
#include <gld_complex.h>

#include "helics_msg.h"

EXPORT_CREATE(helics_msg);
EXPORT_INIT(helics_msg);
EXPORT_PRECOMMIT(helics_msg);
EXPORT_SYNC(helics_msg);
EXPORT_COMMIT(helics_msg);
EXPORT_LOADMETHOD(helics_msg,configure);
static helicscpp::CombinationFederate *pHelicsFederate{nullptr};
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

//static FUNCTIONSRELAY *first_helicsfunction = nullptr;

CLASS *helics_msg::oclass = nullptr;
helics_msg *helics_msg::defaults = nullptr;

//Constructor
helics_msg::helics_msg(MODULE *module)
{
	// register to receive notice for first top down. bottom up, and second top down synchronizations
	oclass = gld_class::create(module,"helics_msg",sizeof(helics_msg),PC_AUTOLOCK|PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_OBSERVER);
	if (oclass == nullptr)
		throw "connection/helics_msg::helics_msg(MODULE*): unable to register class connection:helics_msg";
	else
		oclass->trl = TRL_UNKNOWN;

	defaults = this;
	if (gl_publish_variable(oclass,
		PT_double, "version", get_version_offset(), PT_DESCRIPTION, "helics_msg version",
		PT_enumeration, "message_type", PADDR(message_type), PT_DESCRIPTION, "set the type of message format you wish to construct",
			PT_KEYWORD, "GENERAL", enumeration(HMT_GENERAL), PT_DESCRIPTION, "use this for sending a general HELICS topic/value pair.",
			PT_KEYWORD, "JSON", enumeration(HMT_JSON), PT_DESCRIPTION, "use this for want to send a bundled json formatted messag in a single topic.",
			PT_int32, "publish_period", PADDR(publish_period), PT_DESCRIPTION, "use this with json bundling to set the period [s] at which data is published.",
		// TODO add published properties here
		nullptr)<1)
			throw "connection/helics_msg::helics_msg(MODULE*): unable to publish properties of connection:helics_msg";
	if ( !gl_publish_loadmethod(oclass,"configure",[]( void *val,char *str)->int{return loadmethod_helics_msg_configure(static_cast<OBJECT *>(val),str);}) )
		throw "connection/helics_msg::helics_msg(MODULE*): unable to publish configure method of connection:helics_msg";
}

int helics_msg::create(){
	add_clock_update((void *)this,clocks_update);
	register_object_interupdate((void *)this, dInterupdate);
	register_object_deltaclockupdate((void *)this, dClockupdate);
	message_type = HMT_GENERAL;
	publish_period = 0;
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
		const HelicsFederateState fed_state = pHelicsFederate->getCurrentMode();
		if(fed_state != HELICS_STATE_FINALIZE) {
			if(fed_state != HELICS_STATE_ERROR) {
				string fed_name = string(pHelicsFederate->getName());
				string error_msg = fed_name + string(":The GridLAB-D Federate encountered an internal Error.");
				pHelicsFederate->globalError((int)(exitCode.get_int16()), error_msg);
				pHelicsFederate->finalize();
			}
		}
		helicscpp::cleanupHelicsLibrary();
#endif
	} else {
		//TODO find equivalent helics clean exit message
#if HAVE_HELICS
		gl_verbose("helics_msg: Calling finalize\n");
		const HelicsFederateState fed_state = pHelicsFederate->getCurrentMode();
		if(fed_state != HELICS_STATE_FINALIZE) {
			pHelicsFederate->finalize();
		}
		helicscpp::cleanupHelicsLibrary();
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
	OBJECT *vObj = nullptr;
	char buffer[1024] = "";
	string simName = string(gl_name(obj, buffer, 1023));
	string dft;
	char defaultBuf[1024] = "";
	string type;
	string gld_prop_string = "";
#if HAVE_HELICS
	if(gld_helics_federate == nullptr) {
		try {
			gld_helics_federate = new helicscpp::CombinationFederate(*federate_configuration_file);
			int pub_count = gld_helics_federate->getPublicationCount();
			int sub_count = gld_helics_federate->getInputCount();
			int ep_count = gld_helics_federate->getEndpointCount();
			int idx = 0;
			helics_value_publication *gld_pub;
			helics_value_subscription *gld_sub;
			helics_endpoint_publication *gld_ep_pub;
			helics_endpoint_subscription *gld_ep_sub;
			json_helics_value_publication *json_gld_pub;
			json_helics_value_subscription *json_gld_sub;
			json_helics_endpoint_publication *json_gld_ep_pub;
			json_helics_endpoint_subscription *json_gld_ep_sub;
			string config_info_temp = "";
			string individual_message_type = "";
			Json::CharReaderBuilder json_builder;
			Json::Value config_info;
			Json::String parse_err;
			if(message_type == HMT_GENERAL){
				for( idx = 0; idx < pub_count; idx++ ) {
					helicscpp::Publication pub = gld_helics_federate->getPublication(idx);
					if( pub.isValid() ) {
						config_info_temp = string(pub.getInfo());
						const int config_length = static_cast<int>(config_info_temp.size());
						const std::unique_ptr<Json::CharReader> jReader(json_builder.newCharReader());
						if(jReader->parse(config_info_temp.c_str(),config_info_temp.c_str() + config_length, &config_info, &parse_err)){
							if( config_info.isMember("message_type")){
								individual_message_type = config_info["message_type"].asString();
								if( individual_message_type.compare("JSON") == 0 ) {
									json_gld_pub = new json_helics_value_publication();
									json_gld_pub->name = string(pub.getName());
									json_gld_pub->objectPropertyBundle = config_info["publication_info"];
									json_publication *gldProperty = nullptr;
									for(Json::ValueIterator it = json_gld_pub->objectPropertyBundle.begin(); it != json_gld_pub->objectPropertyBundle.end(); it++){
										const string gldObjName = it.name();
										string gldPropName;
										int n = json_gld_pub->objectPropertyBundle[gldObjName].size();
										for(int i = 0; i < n; i++){
											gldPropName = json_gld_pub->objectPropertyBundle[gldObjName][i].asString();
											gldProperty = new json_publication(gldObjName, gldPropName);
											json_gld_pub->jsonPublications.push_back(gldProperty);
										}
									}
									json_gld_pub->HelicsPublication = pub;
									json_helics_value_publications.push_back(json_gld_pub);
								} else if( individual_message_type.compare("GENERAL") == 0 ){
									gld_pub = new helics_value_publication();
									gld_pub->name = string(pub.getName());
									gld_pub->objectName = config_info["object"].asString();
									gld_pub->propertyName = config_info["property"].asString();
									gld_pub->HelicsPublication = pub;
									helics_value_publications.push_back(gld_pub);
								} else {
									throw("The info field of the publication:%s defines an unknown message_type:%s. Valid message types are JSON and GENERAL", pub.getName(), individual_message_type.c_str());
								}
							} else {
								gld_pub = new helics_value_publication();
								gld_pub->name = string(pub.getName());
								gld_pub->objectName = config_info["object"].asString();
								gld_pub->propertyName = config_info["property"].asString();
								gld_pub->HelicsPublication = pub;
								helics_value_publications.push_back(gld_pub);
							}
							gl_verbose("helics_msg::init(): Successfully processed helics publication %s.", pub.getName());
						} else {
							throw("Couldn't parse the HelicsPublication %s's info field. %s.", pub.getName(), parse_err.c_str());
						}
					}
				}
				for( idx = 0; idx < sub_count; idx++ ) {
					helicscpp::Input sub = gld_helics_federate->getInput(idx);
					if( sub.isValid() ) {
						config_info_temp = string(sub.getInfo());
						const int config_length = static_cast<int>(config_info_temp.size());
						const std::unique_ptr<Json::CharReader> jReader(json_builder.newCharReader());
						if(jReader->parse(config_info_temp.c_str(),config_info_temp.c_str() + config_length, &config_info, &parse_err)){
							if( config_info.isMember("message_type")) {
								individual_message_type = config_info["message_type"].asString();
								if( individual_message_type.compare("JSON") == 0 ) {
									json_gld_sub = new json_helics_value_subscription();
									json_gld_sub->target = string(sub.getTarget());
									json_gld_sub->HelicsSubscription = sub;
									json_helics_value_subscriptions.push_back(json_gld_sub);
								} else if( individual_message_type.compare("GENERAL") == 0 ){
									gld_sub = new helics_value_subscription();
									gld_sub->target = string(sub.getTarget());
									gld_sub->objectName = config_info["object"].asString();
									gld_sub->propertyName = config_info["property"].asString();
									gld_sub->HelicsSubscription = sub;
									helics_value_subscriptions.push_back(gld_sub);
								} else {
									throw("The info field of the subscription:%s defines an unknown message_type:%s. Valid message types are JSON and GENERAL", sub.getTarget(), individual_message_type.c_str());
								}
							} else {
								gld_sub = new helics_value_subscription();
								gld_sub->target = string(sub.getTarget());
								gld_sub->objectName = config_info["object"].asString();
								gld_sub->propertyName = config_info["property"].asString();
								gld_sub->HelicsSubscription = sub;
								helics_value_subscriptions.push_back(gld_sub);
							}
							gl_verbose("helics_msg::init(): Successfully processed helics subscription %s.", sub.getName());
						} else {
							throw("Couldn't parse the HelicsSubscription %s's info field. %s.", sub.getTarget(), parse_err.c_str());
						}
					}
				}
				for( idx = 0; idx < ep_count; idx++ ) {
					helicscpp::Endpoint ep = gld_helics_federate->getEndpoint(idx);
					if( ep.isValid() ) {
						string dest = string(ep.getDefaultDestination());
						config_info_temp = string(ep.getInfo());
						const int config_length = static_cast<int>(config_info_temp.size());
						const std::unique_ptr<Json::CharReader> jReader(json_builder.newCharReader());
						if(jReader->parse(config_info_temp.c_str(),config_info_temp.c_str() + config_length, &config_info, &parse_err)){
							if( config_info.isMember("message_type")) {
								individual_message_type = config_info["message_type"].asString();
								if( individual_message_type.compare("JSON") == 0 ) {
									if( !dest.empty() ){
										json_gld_ep_pub = new json_helics_endpoint_publication();
										json_gld_ep_pub->name = string(ep.getName());
										json_gld_ep_pub->destination = dest;
										json_gld_ep_pub->objectPropertyBundle = config_info["publication_info"];
										json_publication *gldProperty = nullptr;
										for(Json::ValueIterator it = json_gld_ep_pub->objectPropertyBundle.begin(); it != json_gld_ep_pub->objectPropertyBundle.end(); it++){
											const string gldObjName = it.name();
											string gldPropName;
											int n = json_gld_ep_pub->objectPropertyBundle[gldObjName].size();
											for(int i = 0; i < n; i++){
												gldPropName = json_gld_ep_pub->objectPropertyBundle[gldObjName][i].asString();
												gldProperty = new json_publication(gldObjName, gldPropName);
												json_gld_ep_pub->jsonPublications.push_back(gldProperty);
											}
										}
										json_gld_ep_pub->HelicsPublicationEndpoint = ep;
										json_helics_endpoint_publications.push_back(json_gld_ep_pub);
										gl_verbose("helics_msg::init(): registering publishing endpoint: %s", json_gld_ep_pub->name.c_str());
										if( config_info.isMember("receives_messages") ) {
											if( config_info["receives_messages"].asBool() ) {
												json_gld_ep_sub = new json_helics_endpoint_subscription();
												json_gld_ep_sub->name = ep.getName();
												json_gld_ep_sub->HelicsSubscriptionEndpoint = ep;
												json_helics_endpoint_subscriptions.push_back(json_gld_ep_sub);
												gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", json_gld_ep_sub->name.c_str());
											}
										}
									} else {
										json_gld_ep_sub = new json_helics_endpoint_subscription();
										json_gld_ep_sub->name = ep.getName();
										json_gld_ep_sub->HelicsSubscriptionEndpoint = ep;
										json_helics_endpoint_subscriptions.push_back(json_gld_ep_sub);
										gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", json_gld_ep_sub->name.c_str());
									}
								} else if( individual_message_type.compare("GENERAL") == 0 ){
									if( config_info.isMember("object") && config_info.isMember("property") ){
										if( !dest.empty() ){
											gld_ep_pub = new helics_endpoint_publication();
											gld_ep_pub->name = string(ep.getName());
											gld_ep_pub->destination = dest;
											gld_ep_pub->objectName = config_info["object"].asString();
											gld_ep_pub->propertyName = config_info["property"].asString();
											gld_ep_pub->HelicsPublicationEndpoint = ep;
											helics_endpoint_publications.push_back(gld_ep_pub);
											gl_verbose("helics_msg::init(): registering publishing endpoint: %s", gld_ep_pub->name.c_str());
										} else {
											gld_ep_sub = new helics_endpoint_subscription();
											gld_ep_sub->name = string(ep.getName());
											gld_ep_sub->objectName = config_info["object"].asString();
											gld_ep_sub->propertyName = config_info["property"].asString();
											gld_ep_sub->HelicsSubscriptionEndpoint = ep;
											helics_endpoint_subscriptions.push_back(gld_ep_sub);
											gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", gld_ep_sub->name.c_str());
										}
									}
									if( config_info.isMember("publication_info") ) {
										gld_ep_pub = new helics_endpoint_publication();
										gld_ep_pub->name = string(ep.getName());
										gld_ep_pub->destination = dest;
										gld_ep_pub->objectName = config_info["publication_info"]["object"].asString();
										gld_ep_pub->propertyName = config_info["publication_info"]["property"].asString();
										gld_ep_pub->HelicsPublicationEndpoint = ep;
										helics_endpoint_publications.push_back(gld_ep_pub);
										gl_verbose("helics_msg::init(): registering publishing endpoint: %s", gld_ep_pub->name.c_str());
									}
									if( config_info.isMember("subscription_info") ) {
										gld_ep_sub = new helics_endpoint_subscription();
										gld_ep_sub->name = string(ep.getName());
										gld_ep_sub->objectName = config_info["subscription_info"]["object"].asString();
										gld_ep_sub->propertyName = config_info["subscription_info"]["property"].asString();
										gld_ep_sub->HelicsSubscriptionEndpoint = ep;
										helics_endpoint_subscriptions.push_back(gld_ep_sub);
										gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", gld_ep_sub->name.c_str());
									}
								}
							} else {
								if( config_info.isMember("object") && config_info.isMember("property") ){
									if( !dest.empty() ){
										gld_ep_pub = new helics_endpoint_publication();
										gld_ep_pub->name = string(ep.getName());
										gld_ep_pub->destination = dest;
										gld_ep_pub->objectName = config_info["object"].asString();
										gld_ep_pub->propertyName = config_info["property"].asString();
										gld_ep_pub->HelicsPublicationEndpoint = ep;
										helics_endpoint_publications.push_back(gld_ep_pub);
										gl_verbose("helics_msg::init(): registering publishing endpoint: %s", gld_ep_pub->name.c_str());
									} else {
										gld_ep_sub = new helics_endpoint_subscription();
										gld_ep_sub->name = string(ep.getName());
										gld_ep_sub->objectName = config_info["object"].asString();
										gld_ep_sub->propertyName = config_info["property"].asString();
										gld_ep_sub->HelicsSubscriptionEndpoint = ep;
										helics_endpoint_subscriptions.push_back(gld_ep_sub);
										gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", gld_ep_sub->name.c_str());
									}
								}
								if( config_info.isMember("publication_info") ) {
									gld_ep_pub = new helics_endpoint_publication();
									gld_ep_pub->name = string(ep.getName());
									gld_ep_pub->destination = dest;
									gld_ep_pub->objectName = config_info["publication_info"]["object"].asString();
									gld_ep_pub->propertyName = config_info["publication_info"]["property"].asString();
									gld_ep_pub->HelicsPublicationEndpoint = ep;
									helics_endpoint_publications.push_back(gld_ep_pub);
									gl_verbose("helics_msg::init(): registering publishing endpoint: %s", gld_ep_pub->name.c_str());
								}
								if( config_info.isMember("subscription_info") ) {
									gld_ep_sub = new helics_endpoint_subscription();
									gld_ep_sub->name = string(ep.getName());
									gld_ep_sub->objectName = config_info["subscription_info"]["object"].asString();
									gld_ep_sub->propertyName = config_info["subscription_info"]["property"].asString();
									gld_ep_sub->HelicsSubscriptionEndpoint = ep;
									helics_endpoint_subscriptions.push_back(gld_ep_sub);
									gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", gld_ep_sub->name.c_str());
								}
							}
						} else {
							throw("Couldn't parse the HelicsEndpoint %s's info field. %s.", ep.getName(), parse_err.c_str());
						}
					}
				}
			} else if(message_type == HMT_JSON) {
				for(idx = 0; idx < pub_count; idx++) {
					helicscpp::Publication pub = gld_helics_federate->getPublication(idx);
					if(pub.isValid()) {
						json_gld_pub = new json_helics_value_publication();
						json_gld_pub->name = string(pub.getName());
						config_info_temp = string(pub.getInfo());
						const int config_length = static_cast<int>(config_info_temp.size());
						const std::unique_ptr<Json::CharReader> jReader(json_builder.newCharReader());
						if(jReader->parse(config_info_temp.c_str(),config_info_temp.c_str() + config_length, &(json_gld_pub->objectPropertyBundle), &parse_err)){
							json_publication *gldProperty = nullptr;
							for(Json::ValueIterator it = json_gld_pub->objectPropertyBundle.begin(); it != json_gld_pub->objectPropertyBundle.end(); it++){
								const string gldObjName = it.name();
								string gldPropName;
								int n = json_gld_pub->objectPropertyBundle[gldObjName].size();
								for(int i = 0; i < n; i++){
									gldPropName = json_gld_pub->objectPropertyBundle[gldObjName][i].asString();
									gldProperty = new json_publication(gldObjName, gldPropName);
									json_gld_pub->jsonPublications.push_back(gldProperty);
								}
							}
							json_gld_pub->HelicsPublication = pub;
							json_helics_value_publications.push_back(json_gld_pub);
						} else {
							throw("Couldn't parse the HelicsPublication %s's info field. %s.", pub.getName(), parse_err.c_str());
						}
					}
				}
				for(idx = 0; idx < sub_count; idx++){
					helicscpp::Input sub = gld_helics_federate->getInput(idx);
					if(sub.isValid()){
						json_gld_sub = new json_helics_value_subscription();
						json_gld_sub->target = string(sub.getTarget());
						json_gld_sub->HelicsSubscription = sub;
						json_helics_value_subscriptions.push_back(json_gld_sub);
					}
				}
				for( idx = 0; idx < ep_count; idx++ ) {
					helicscpp::Endpoint ep = gld_helics_federate->getEndpoint(idx);
					if( ep.isValid() ) {
						string dest = string(ep.getDefaultDestination());
						if( !dest.empty() ){
							json_gld_ep_pub = new json_helics_endpoint_publication();
							json_gld_ep_pub->name = string(ep.getName());
							json_gld_ep_pub->destination = dest;
							config_info_temp = string(ep.getInfo());
							const int config_length = static_cast<int>(config_info_temp.size());
							const std::unique_ptr<Json::CharReader> jReader(json_builder.newCharReader());
							if(jReader->parse(config_info_temp.c_str(),config_info_temp.c_str() + config_length, &(json_gld_ep_pub->objectPropertyBundle), &parse_err)){
								json_publication *gldProperty = nullptr;
								for(Json::ValueIterator it = json_gld_ep_pub->objectPropertyBundle.begin(); it != json_gld_ep_pub->objectPropertyBundle.end(); it++){
									const string gldObjName = it.name();
									string gldPropName;
									int n = json_gld_ep_pub->objectPropertyBundle[gldObjName].size();
									for(int i = 0; i < n; i++){
										gldPropName = json_gld_ep_pub->objectPropertyBundle[gldObjName][i].asString();
										gldProperty = new json_publication(gldObjName, gldPropName);
										json_gld_ep_pub->jsonPublications.push_back(gldProperty);
									}
								}
								json_gld_ep_pub->HelicsPublicationEndpoint = ep;
								json_helics_endpoint_publications.push_back(json_gld_ep_pub);
								gl_verbose("helics_msg::init(): registering publishing endpoint: %s", json_gld_ep_pub->name.c_str());
							} else {
								throw("Couldn't parse the HelicsEndpoint %s's info field. %s.", ep.getName(), parse_err.c_str());
							}
						} else {
							json_gld_ep_sub = new json_helics_endpoint_subscription();
							json_gld_ep_sub->name = ep.getName();
							json_gld_ep_sub->HelicsSubscriptionEndpoint = ep;
							json_helics_endpoint_subscriptions.push_back(json_gld_ep_sub);
							gl_verbose("helics_msg::init(): registering subscribing endpoint: %s", json_gld_ep_sub->name.c_str());
						}
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
		if((*pub)->pObjectProperty == nullptr) {
			const char *pObjName = (*pub)->objectName.c_str();
			const char *pPropName = (*pub)->propertyName.c_str();
			char *pObjBuf = new char[strlen(pObjName)+1];
			char *pPropBuf = new char[strlen(pPropName)+1];
			strcpy(pObjBuf, pObjName);
			strcpy(pPropBuf, pPropName);
			if((*pub)->objectName.compare("global") != 0) {
				(*pub)->pObjectProperty = new gld_property(pObjBuf, pPropBuf);
				if(!(*pub)->pObjectProperty->is_valid()) {
					rv = 0;
					gl_error("helics_msg::init(): There is no object %s with property %s",(char *)(*pub)->objectName.c_str(), (char *)(*pub)->propertyName.c_str());
					break;
				}
			} else {
				(*pub)->pObjectProperty = new gld_property(pPropBuf);
				if(!(*pub)->pObjectProperty->is_valid()) {
					rv = 0;
					gl_error("helics_msg::init(): There is no global property %s",(char *)(*pub)->propertyName.c_str());
					break;
				}
			}
		}
	}
	if(rv == 0) {
		return rv;
	}
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++) {
		if((*sub)->pObjectProperty == nullptr) {
			const char *pObjName = (*sub)->objectName.c_str();
			const char *pPropName = (*sub)->propertyName.c_str();
			char *pObjBuf = new char[strlen(pObjName)+1];
			char *pPropBuf = new char[strlen(pPropName)+1];
			strcpy(pObjBuf, pObjName);
			strcpy(pPropBuf, pPropName);
			if((*sub)->objectName.compare("global") != 0) {
				(*sub)->pObjectProperty = new gld_property(pObjBuf, pPropBuf);
				if(!(*sub)->pObjectProperty->is_valid()) {
					rv = 0;
					gl_error("helics_msg::init(): There is no object %s with property %s",(char *)(*sub)->objectName.c_str(), (char *)(*sub)->propertyName.c_str());
					break;
				}
			} else {
				(*sub)->pObjectProperty = new gld_property(pPropBuf);
				if(!(*sub)->pObjectProperty->is_valid()) {
					rv = 0;
					gl_error("helics_msg::init(): There is no global property %s",(char *)(*sub)->propertyName.c_str());
					break;
				}
			}
		}
	}
	if(rv == 0) {
		return rv;
	}
	for(vector<helics_endpoint_publication*>::iterator pub = helics_endpoint_publications.begin(); pub != helics_endpoint_publications.end(); pub++) {
		if((*pub)->pObjectProperty == nullptr) {
			const char *pObjName = (*pub)->objectName.c_str();
			const char *pPropName = (*pub)->propertyName.c_str();
			char *pObjBuf = new char[strlen(pObjName)+1];
			char *pPropBuf = new char[strlen(pPropName)+1];
			strcpy(pObjBuf, pObjName);
			strcpy(pPropBuf, pPropName);
			if((*pub)->objectName.compare("global") != 0) {
				(*pub)->pObjectProperty = new gld_property(pObjBuf, pPropBuf);
				if(!(*pub)->pObjectProperty->is_valid()) {
					rv = 0;
					gl_error("helics_msg::init(): There is no object %s with property %s",(char *)(*pub)->objectName.c_str(), (char *)(*pub)->propertyName.c_str());
					break;
				}
			} else {
				(*pub)->pObjectProperty = new gld_property(pPropBuf);
				if(!(*pub)->pObjectProperty->is_valid()) {
					rv = 0;
					gl_error("helics_msg::init(): There is no global property %s",(char *)(*pub)->propertyName.c_str());
					break;
				}
			}
		}
	}
	if(rv == 0) {
		return rv;
	}
	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++) {
		if((*sub)->pObjectProperty == nullptr) {
			const char *pObjName = (*sub)->objectName.c_str();
			const char *pPropName = (*sub)->propertyName.c_str();
			char *pObjBuf = new char[strlen(pObjName)+1];
			char *pPropBuf = new char[strlen(pPropName)+1];
			strcpy(pObjBuf, pObjName);
			strcpy(pPropBuf, pPropName);
			if((*sub)->objectName.compare("global") != 0) {
				(*sub)->pObjectProperty = new gld_property(pObjBuf, pPropBuf);
				if(!(*sub)->pObjectProperty->is_valid()) {
					rv = 0;
					gl_error("helics_msg::init(): There is no object %s with property %s",(char *)(*sub)->objectName.c_str(), (char *)(*sub)->propertyName.c_str());
					break;
				}
			} else {
				(*sub)->pObjectProperty = new gld_property(pPropBuf);
				if(!(*sub)->pObjectProperty->is_valid()) {
					rv = 0;
					gl_error("helics_msg::init(): There is no global property %s",(char *)(*sub)->propertyName.c_str());
					break;
				}
			}
		}
	}
	if(rv == 0) {
		return rv;
	}
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		vObj = (*pub)->pObjectProperty->get_object();
		if(vObj != nullptr) {
			if((vObj->flags & OF_INIT) != OF_INIT){
				defer = true;
			}
		}
	}
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++) {
		vObj = (*sub)->pObjectProperty->get_object();
		if(vObj != nullptr) {
			if((vObj->flags & OF_INIT) != OF_INIT){
				defer = true;
			}
		}
	}
	for(vector<helics_endpoint_publication*>::iterator pub = helics_endpoint_publications.begin(); pub != helics_endpoint_publications.end(); pub++) {
		vObj = (*pub)->pObjectProperty->get_object();
		if(vObj != nullptr) {
			if((vObj->flags & OF_INIT) != OF_INIT){
				defer = true;
			}
		}
	}
	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++) {
		vObj = (*sub)->pObjectProperty->get_object();
		if(vObj != nullptr) {
			if((vObj->flags & OF_INIT) != OF_INIT){
				defer = true;
			}
		}
	}
	if(defer == true){
		gl_verbose("helics_msg::init(): %s is defering initialization.", obj->name);
		return 2;
	}
	//register with helics
#if HAVE_HELICS
	pHelicsFederate = gld_helics_federate;

	string pub_sub_name = "";
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		if((*pub)->HelicsPublication.isValid()) {
			if((*pub)->pObjectProperty->is_complex()) {
				if(string("complex").compare((*pub)->HelicsPublication.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered publication %s is intended to publish a complex type but the publication has a type = %s", ((*pub)->name).c_str(), (*pub)->HelicsPublication.getType());
					return 0;
				}
			} else if((*pub)->pObjectProperty->is_integer()) {
				if(string((*pub)->HelicsPublication.getType()).find("int") == string::npos ) {
					gl_error("helics_msg::init: The registered publication %s is intended to publish an integer type but the publication has a type = %s", ((*pub)->name).c_str(), (*pub)->HelicsPublication.getType());
					return 0;
				}
			} else if((*pub)->pObjectProperty->is_double()) {
				if(string("double").compare((*pub)->HelicsPublication.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered publication %s is intended to publish a double type but the publication has a type = %s", ((*pub)->name).c_str(), (*pub)->HelicsPublication.getType());
					return 0;
				}
			} else {
				if(string("string").compare((*pub)->HelicsPublication.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered publication %s is intended to publish a string type but the publication has a type = %s", ((*pub)->name).c_str(), (*pub)->HelicsPublication.getType());
					return 0;
				}
			}
		} else {
			gl_error("helics_msg::init: There is no registered publication with a name = %s", (*pub)->name.c_str());
			return 0;
		}
	}
	//register helics subscriptions
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++) {
		if((*sub)->HelicsSubscription.isValid()) {
			if((*sub)->pObjectProperty->is_complex()) {
				if(string("complex").compare((*sub)->HelicsSubscription.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered subscription %s is intended to subscribe to a complex type but the subscription has a type = %s", ((*sub)->target.c_str()), (*sub)->HelicsSubscription.getType());
					return 0;
				}
			} else if((*sub)->pObjectProperty->is_integer()) {
				if(string((*sub)->HelicsSubscription.getType()).find("int") == string::npos ) {
					gl_error("helics_msg::init: The registered subscription %s is intended to subscribe to an integer type but the subscription has a type = %s", ((*sub)->target.c_str()), (*sub)->HelicsSubscription.getType());
					return 0;
				}
			} else if((*sub)->pObjectProperty->is_double()) {
				if(string("double").compare((*sub)->HelicsSubscription.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered subscription %s is intended to subscribe to double type but the subscription has a type = %s", ((*sub)->target.c_str()), (*sub)->HelicsSubscription.getType());
					return 0;
				}
			} else {
				if(string("string").compare((*sub)->HelicsSubscription.getType()) != 0 ) {
					gl_error("helics_msg::init: The registered subscription %s is intended to subscribe to string type but the subscription has a type = %s", ((*sub)->target.c_str()), (*sub)->HelicsSubscription.getType());
					return 0;
				}
			}
		} else {
			gl_error("helics_msg::init: There is no registered subscription with a key = %s", (*sub)->target.c_str());
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
	gld_helics_federate->enterInitializingMode();
	gl_verbose("helics_msg: Calling enterExecutionState");
	gld_helics_federate->enterExecutingMode();
#endif
	atexit(send_die);
	last_approved_helics_time = gl_globalclock;
	last_delta_helics_time = (double)(gl_globalclock);
	initial_sim_time = gl_globalclock;
	if(message_type == HMT_JSON && publish_period == 0) {
		publish_period = 3;
	}
	if(publish_period > 0) {
		publish_time = initial_sim_time + (TIMESTAMP)publish_period;
	}
	return rv;
}

int helics_msg::precommit(TIMESTAMP t1){
	int result = 0;
	if(message_type == HMT_GENERAL){
		result = subscribeVariables();
		if(result == 0){
			return result;
		}
	}
	return 1;
}

TIMESTAMP helics_msg::presync(TIMESTAMP t1){
	if(message_type == HMT_JSON){
		int result = 0;
		result = subscribeJsonVariables();
		if(result == 0){
			return TS_INVALID;
		}
	}
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
		HelicsTime helics_t = 0;
//		helics::time t = 0;
		HelicsTime t = 0;
		double dt = 0;
		int t_ns = 0;
		int helics_t_ns = 0;
		dt = (t1 - (double)initial_sim_time)*1000000000.0;
		if(sysmode == SM_EVENT) {
			t = (HelicsTime)(((dt + (1000000000.0 / 2.0)) - fmod((dt + (1000000000.0 / 2.0)), 1000000000.0))/1000000000.0);
		} else {
			t = (HelicsTime)(((dt + ((double)(timestep) / 2.0)) - fmod((dt + ((double)(timestep) / 2.0)), (double)timestep))/1000000000.0);
		}
		gld_helics_federate->setProperty(HELICS_PROPERTY_TIME_PERIOD, (HelicsTime)(((double)timestep)/DT_SECOND));
		helics_t = gld_helics_federate->requestTime(t);
		//TODO call helics time update function
		if(sysmode == SM_EVENT)
			exitDeltamode = true;
		t_ns = (int)(round(t * 1000000000.0));
		helics_t_ns = (int)(round(helics_t * 1000000000.0));
		if(helics_t_ns != t_ns){
			gl_error("helics_msg::deltaClockUpdate: Cannot return anything other than the time GridLAB-D requested in deltamode. Time requested %f. Time returned %f.", (double)t, (double)helics_t);
			return SM_ERROR;
		} else {
			last_delta_helics_time = (double)(helics_t) + (double)(initial_sim_time);
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
	TIMESTAMP helics_t = 0;
	if(exitDeltamode == true){
#if HAVE_HELICS
		//TODO update time delta in helics
		gl_verbose("helics_msg: Calling setTimeDelta");
		gld_helics_federate->setProperty(HELICS_PROPERTY_TIME_PERIOD, 1.0);// 140 is the option for the period property.
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
			t1 = gl_globalstoptime;
		}
		if(message_type == HMT_GENERAL){
			if (publish_period == 0 || t1 == publish_time){
				if(publish_period > 0){
					publish_time = t1 + (TIMESTAMP)publish_period;
				}
				result = publishVariables();
				if(result == 0){
					return TS_INVALID;
				}
			}
		} else if(message_type == HMT_JSON){
			if (publish_period == 0 || t1 == publish_time){
				if(publish_period > 0){
					publish_time = t1 + (TIMESTAMP)publish_period;
				}
				if(t1<gl_globalstoptime){
					result = publishJsonVariables();
					if(result == 0){
						return TS_INVALID;
					}
				}
			}
		}
#if HAVE_HELICS
		HelicsTime t((double)((t1 - initial_sim_time)));
//		HelicsTime = ((TIMESTAMP)helics::time_request(t))/1000000000 + initial_sim_time;
		//TODO call appropriate helics time update function
		gl_verbose("helics_msg: Calling requestime");
		gl_verbose("helics_msg: Requesting %f", (double)t);
		HelicsTime rt;
		rt = gld_helics_federate->requestTime(t);
		gl_verbose("helics_msg: Granted %f", (double)rt);
		helics_t = (TIMESTAMP)rt + initial_sim_time;
#endif
		if(helics_t <= gl_globalclock){
			gl_error("helics_msg::clock_update: Cannot return the current time or less than the current time.");
			return TS_INVALID;
		} else {
			last_approved_helics_time = helics_t;
			t1 = helics_t;
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
	Json::Value jsonPublishData;
	gld_property *prop = nullptr;
	jsonPublishData.clear();
	std::stringstream complex_val;
	Json::StreamWriterBuilder writer_builder;
	string jsonMessageStr = "";
	for(vector<helics_value_publication*>::iterator pub = helics_value_publications.begin(); pub != helics_value_publications.end(); pub++) {
		buffer_size = 0;
#if HAVE_HELICS
		if( (*pub)->pObjectProperty->is_complex() ) {
			double real_part = (*pub)->pObjectProperty->get_part("real");
			double imag_part = (*pub)->pObjectProperty->get_part("imag");
			gld_unit *val_unit = (*pub)->pObjectProperty->get_unit();
			complex_temp = {real_part, imag_part};

			gl_verbose("helics_msg: calling publish(%f%+fj) on publication %s", real_part, imag_part, (*pub)->name.c_str());
			(*pub)->HelicsPublication.publish(complex_temp);

		} else if((*pub)->pObjectProperty->is_integer()) {
			int64_t integer_temp = (*pub)->pObjectProperty->get_integer();
			gl_verbose("helics_msg: calling publishInt(%d) on publication %s", integer_temp, (*pub)->name.c_str());
			(*pub)->HelicsPublication.publish(integer_temp);
		} else if((*pub)->pObjectProperty->is_double()) {
			double double_temp = 0;
			(*pub)->pObjectProperty->getp(double_temp);
			gl_verbose("helics_msg: calling publish(%f) on publication %s", double_temp, (*pub)->name.c_str());
			(*pub)->HelicsPublication.publish(double_temp);
		} else {
			buffer_size = (*pub)->pObjectProperty->to_string(&buffer[0], 1023);
			if(buffer_size <= 0) {
				temp_value = "";
			} else {
				temp_value = string(buffer, (size_t)(buffer_size));
				gl_verbose("helics_msg: Calling publish(\"%s\") on publication %s", temp_value.c_str(), (*pub)->name.c_str());
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
			if(gld_helics_federate->getCurrentMode() == HELICS_STATE_EXECUTION){
				helicscpp::Message *msg = new helicscpp::Message((*pub)->HelicsPublicationEndpoint);
				msg->data(message_buffer);
				gl_verbose("calling helics sendMessage on endpoint %s. Message: %s",(*pub)->name.c_str(), message_buffer.c_str());
				(*pub)->HelicsPublicationEndpoint.sendMessage(*msg);
				delete msg;
			}
        } catch (const std::exception& e) { // reference to the base of a polymorphic object
        	gl_error("calling HELICS sendMessage resulted in an unknown error.");
             std::cout << e.what() << std::endl; // information from length_error printed
        }
#endif
		memset(&buffer[0], '\0', 1024);
	}
#if HAVE_HELICS
	for(vector<json_helics_value_publication*>::iterator pub = json_helics_value_publications.begin(); pub != json_helics_value_publications.end(); pub++){
		jsonPublishData.clear();
		for(vector<json_publication*>::iterator o = (*pub)->jsonPublications.begin(); o != (*pub)->jsonPublications.end(); o++) {
			prop = (*o)->prop;
			if(prop->is_valid()){
				if(!jsonPublishData.isMember((*o)->object_name)){
					jsonPublishData[(*o)->object_name];
				}
				if(prop->is_double()){
					jsonPublishData[(*o)->object_name][(*o)->property_name] = prop->get_double();
				} else if(prop->is_complex()){
					double real_part = prop->get_part("real");
					double imag_part =prop->get_part("imag");
					gld_unit *val_unit = prop->get_unit();
					complex_val.str(string());
					complex_val << std::fixed << real_part;
					if(imag_part >= 0){
						complex_val << std::fixed << "+" << fabs(imag_part) << "j";
					} else {
						complex_val << std::fixed << imag_part << "j";
					}
					if(val_unit != nullptr && val_unit->is_valid()){
						string unit_name = string(val_unit->get_name());
						complex_val << " " << unit_name;
					}
					jsonPublishData[(*o)->object_name][(*o)->property_name] = complex_val.str();
				} else if(prop->is_integer()){
					jsonPublishData[(*o)->object_name][(*o)->property_name] = (Json::Value::Int64)prop->get_integer();
				} else if(prop->is_timestamp()){
					jsonPublishData[(*o)->object_name][(*o)->property_name] = (Json::Value::Int64)prop->get_timestamp();
				} else {
					char chTemp[1024];
					prop->to_string(chTemp, 1023);
					jsonPublishData[(*o)->object_name][(*o)->property_name] = string((char *)chTemp);
				}
			}
		}
		jsonMessageStr = Json::writeString(writer_builder, jsonPublishData);
		gl_verbose("publishing json message on publication %s: %s",(*pub)->name.c_str(), jsonMessageStr.c_str());
		(*pub)->HelicsPublication.publish(jsonMessageStr);
	}
	for(vector<json_helics_endpoint_publication*>::iterator pub = json_helics_endpoint_publications.begin(); pub != json_helics_endpoint_publications.end(); pub++){
		jsonPublishData.clear();
		for(vector<json_publication*>::iterator o = (*pub)->jsonPublications.begin(); o != (*pub)->jsonPublications.end(); o++) {
			prop = (*o)->prop;
			if(prop->is_valid()){
				if(!jsonPublishData.isMember((*o)->object_name)){
					jsonPublishData[(*o)->object_name];
				}
				if(prop->is_double()){
					jsonPublishData[(*o)->object_name][(*o)->property_name] = prop->get_double();
				} else if(prop->is_complex()){
					double real_part = prop->get_part("real");
					double imag_part =prop->get_part("imag");
					gld_unit *val_unit = prop->get_unit();
					complex_val.str(string());
					complex_val << std::fixed << real_part;
					if(imag_part >= 0){
						complex_val << std::fixed << "+" << fabs(imag_part) << "j";
					} else {
						complex_val << std::fixed << imag_part << "j";
					}
					if(val_unit != nullptr && val_unit->is_valid()){
						string unit_name = string(val_unit->get_name());
						complex_val << " " << unit_name;
					}
					jsonPublishData[(*o)->object_name][(*o)->property_name] = complex_val.str();
				} else if(prop->is_integer()){
					jsonPublishData[(*o)->object_name][(*o)->property_name] = (Json::Value::Int64)prop->get_integer();
				} else if(prop->is_timestamp()){
					jsonPublishData[(*o)->object_name][(*o)->property_name] = (Json::Value::Int64)prop->get_timestamp();
				} else {
					char chTemp[1024];
					prop->to_string(chTemp, 1023);
					jsonPublishData[(*o)->object_name][(*o)->property_name] = string((char *)chTemp);
				}
			}
		}
		helicscpp::Message *msg = new helicscpp::Message((*pub)->HelicsPublicationEndpoint);
		jsonMessageStr = Json::writeString(writer_builder, jsonPublishData);
		msg->data(jsonMessageStr);
		gl_verbose("sending JSON message on endpoint %s: %s",(*pub)->name.c_str(), jsonMessageStr.c_str());
		(*pub)->HelicsPublicationEndpoint.sendMessage(*msg);
	}
#endif
	return 1;
}

//read variables from the cache
int helics_msg::subscribeVariables(){
	string value_buffer = "";
	gld::complex gld_complex_temp(0.0, 0.0);
	int64_t integer_temp = 0;
	double double_temp = 0.0;
	std::complex<double> complex_temp = {0.0, 0.0};
	OBJECT *obj = OBJECTHDR(this);
	char buf[1024] = "";
	string simName = string(gl_name(obj, buf, 1023));
	Json::Value jsonMessage;
	Json::String parse_err;
	Json::CharReaderBuilder json_builder;
	string value = "";
	string objectName = "";
	string propertyName = "";
	gld_property *gldProperty = nullptr;
#if HAVE_HELICS
	for(vector<helics_value_subscription*>::iterator sub = helics_value_subscriptions.begin(); sub != helics_value_subscriptions.end(); sub++){
		if((*sub)->HelicsSubscription.isUpdated()) {
			try {
				if((*sub)->pObjectProperty->is_complex()) {
					gl_verbose("helics_msg: Calling getComplex on subscription %s",(*sub)->target.c_str());
					complex_temp = (*sub)->HelicsSubscription.getComplex();
					if(!std::isnan(complex_temp.real()) && !std::isnan(complex_temp.imag())) {
						gld_complex_temp.SetReal(complex_temp.real());
						gld_complex_temp.SetImag(complex_temp.imag());
						(*sub)->pObjectProperty->setp(gld_complex_temp);
					}
				} else if((*sub)->pObjectProperty->is_integer()) {
					gl_verbose("helics_msg: Calling getInteger on subscription %s",(*sub)->target.c_str());
					gld_type property_type = (*sub)->pObjectProperty->get_type();
					integer_temp = (*sub)->HelicsSubscription.getInteger();
					if(property_type == PT_int64){
						(*sub)->pObjectProperty->setp(integer_temp);
					} else if(property_type == PT_int32){
						int32_t int32_temp = static_cast<int32_t>(integer_temp);
						(*sub)->pObjectProperty->setp(int32_temp);
					}else if(property_type == PT_int16){
						int16_t int16_temp = static_cast<int16_t>(integer_temp);
						(*sub)->pObjectProperty->setp(int16_temp);
					}
				} else if((*sub)->pObjectProperty->is_double()) {
					gl_verbose("helics_msg: Calling getDouble on subscription %s",(*sub)->target.c_str());
					double_temp = (*sub)->HelicsSubscription.getDouble();
					if(!std::isnan(double_temp)) {
						(*sub)->pObjectProperty->setp(double_temp);
					}
				} else {
					gl_verbose("helics_msg: Calling getString on subscription %s",(*sub)->target.c_str());
					value_buffer = (*sub)->HelicsSubscription.getString();
					if(!value_buffer.empty()) {
						char *valueBuf = new char[value_buffer.size() + 1];
						memset(&valueBuf[0], '\0', value_buffer.size()+1);
						strncpy(valueBuf, value_buffer.c_str(), value_buffer.size());
						(*sub)->pObjectProperty->from_string(valueBuf);
						delete[] valueBuf;
					}
				}
			} catch(...) {
				value_buffer = "";
				gl_verbose("helics_msg: Calling getString on subscription %s",(*sub)->target.c_str());
				value_buffer = (*sub)->HelicsSubscription.getString();
				if(!value_buffer.empty()){
					char *valueBuf = new char[value_buffer.size() + 1];
					memset(&valueBuf[0], '\0', value_buffer.size()+1);
					strncpy(valueBuf, value_buffer.c_str(), value_buffer.size());
					(*sub)->pObjectProperty->from_string(valueBuf);
					delete[] valueBuf;
				}
			}
			value_buffer = "";
		}
	}
	for(vector<helics_endpoint_subscription*>::iterator sub = helics_endpoint_subscriptions.begin(); sub != helics_endpoint_subscriptions.end(); sub++){
        gl_verbose("Has message status for endpoint %s: %s", (*sub)->name.c_str(), (*sub)->HelicsSubscriptionEndpoint.hasMessage() ? "True" : "False");
        if((*sub)->HelicsSubscriptionEndpoint.hasMessage()){
			helicscpp::Message mesg;
			int pendingMessages = (int) (*sub)->HelicsSubscriptionEndpoint.pendingMessageCount();
			for(int i = 0; i < pendingMessages; i++) {
				gl_verbose("calling getMessage() for endpoint %s", (*sub)->name.c_str());
				mesg = (*sub)->HelicsSubscriptionEndpoint.getMessage();
			}
			const char *message_buffer = mesg.c_str();
			int message_size = mesg.size();
			if(message_size != 0){
				char *valueBuf = new char[sizeof(message_buffer) + 1];
				memset(valueBuf, '\0', sizeof(message_buffer) + 1);
				strncpy(valueBuf, message_buffer, message_size);
				(*sub)->pObjectProperty->from_string(valueBuf);
				delete[] valueBuf;
			}
		}
	}

	for(vector<json_helics_value_subscription*>::iterator sub = json_helics_value_subscriptions.begin(); sub != json_helics_value_subscriptions.end(); sub++){
		if((*sub)->HelicsSubscription.isUpdated()){
			gl_verbose("JSON subscription %s updated.", (*sub)->target.c_str());
			value = (*sub)->HelicsSubscription.getString();
			const int value_length = static_cast<int>(value.size());
			const std::unique_ptr<Json::CharReader> jReader(json_builder.newCharReader());
			if(jReader->parse(value.c_str(),value.c_str() + value_length, &jsonMessage, &parse_err)){
				for(Json::ValueIterator o = jsonMessage.begin(); o != jsonMessage.end(); o++){
					objectName = o.name();
					for(Json::ValueIterator p = jsonMessage[objectName].begin(); p != jsonMessage[objectName].end(); p++){
						propertyName = p.name();
						const char *expr1 = objectName.c_str();
						const char *expr2 = propertyName.c_str();
						char *bufObj = new char[strlen(expr1)+1];
						char *bufProp = new char[strlen(expr2)+1];
						strcpy(bufObj, expr1);
						strcpy(bufProp, expr2);
						gldProperty = new gld_property(bufObj, bufProp);
						if(gldProperty->is_valid()){
							if(gldProperty->is_integer()){
								int64_t itmp = jsonMessage[objectName][propertyName].asInt64();
								gld_type property_type = gldProperty->get_type();
								if(property_type == PT_int64){
									gldProperty->setp(itmp);
								} else if(property_type == PT_int32){
									int32_t int32_temp = static_cast<int32_t>(itmp);
									gldProperty->setp(int32_temp);
								}else if(property_type == PT_int16){
									int16_t int16_temp = static_cast<int16_t>(itmp);
									gldProperty->setp(int16_temp);
								}
							} else if(gldProperty->is_double()){
								double dtmp = jsonMessage[objectName][propertyName].asDouble();
								gldProperty->setp(dtmp);
							} else {
								string stmp = jsonMessage[objectName][propertyName].asString();
								char sbuf[1024] = "";
								strncpy(sbuf, stmp.c_str(), 1023);
								gldProperty->from_string(sbuf);
							}
						} else {
							gl_error("helics_msg::subscribeVariables(): There is no object %s with property %s",objectName.c_str(), propertyName.c_str());
							delete gldProperty;
							return 0;
						}
						delete gldProperty;
					}
				}
			} else {
				throw("Couldn't parse the json message for HelicsSubscription %s. %s.", (*sub)->target.c_str(), parse_err.c_str());
			}
		}
	}
	for(vector<json_helics_endpoint_subscription*>::iterator sub = json_helics_endpoint_subscriptions.begin(); sub != json_helics_endpoint_subscriptions.end(); sub++){
		if((*sub)->HelicsSubscriptionEndpoint.hasMessage()){
			helicscpp::Message mesg;
			int pendingMessages = (int) (*sub)->HelicsSubscriptionEndpoint.pendingMessageCount();
			for(int i = 0; i < pendingMessages; i++) {
				gl_verbose("calling getMessage() for endpoint %s", (*sub)->name.c_str());
				mesg = (*sub)->HelicsSubscriptionEndpoint.getMessage();
			}
			const string message_buffer = string(mesg.c_str());
			const int message_length = static_cast<int>(message_buffer.size());
			const std::unique_ptr<Json::CharReader> jReader(json_builder.newCharReader());
			if(jReader->parse(message_buffer.c_str(),message_buffer.c_str() + message_length, &jsonMessage, &parse_err)){
				for(Json::ValueIterator o = jsonMessage.begin(); o != jsonMessage.end(); o++){
					objectName = o.name();
					for(Json::ValueIterator p = jsonMessage[objectName].begin(); p != jsonMessage[objectName].end(); p++){
						propertyName = p.name();
						const char *expr1 = objectName.c_str();
						const char *expr2 = propertyName.c_str();
						char *bufObj = new char[strlen(expr1)+1];
						char *bufProp = new char[strlen(expr2)+1];
						strcpy(bufObj, expr1);
						strcpy(bufProp, expr2);
						gldProperty = new gld_property(bufObj, bufProp);
						if(gldProperty->is_valid()){
							if(gldProperty->is_integer()){
								int64_t itmp = jsonMessage[objectName][propertyName].asInt64();
								gld_type property_type = gldProperty->get_type();
								if(property_type == PT_int64){
									gldProperty->setp(itmp);
								} else if(property_type == PT_int32){
									int32_t int32_temp = static_cast<int32_t>(itmp);
									gldProperty->setp(int32_temp);
								}else if(property_type == PT_int16){
									int16_t int16_temp = static_cast<int16_t>(itmp);
									gldProperty->setp(int16_temp);
								}
							} else if(gldProperty->is_double()){
								double dtmp = jsonMessage[objectName][propertyName].asDouble();
								gldProperty->setp(dtmp);
							} else {
								string stmp = jsonMessage[objectName][propertyName].asString();
								char sbuf[1024] = "";
								strncpy(sbuf, stmp.c_str(), 1023);
								gldProperty->from_string(sbuf);
							}
						} else {
							gl_error("helics_msg::subscribeVariables(): There is no object %s with property %s",objectName.c_str(), propertyName.c_str());
							delete gldProperty;
							return 0;
						}
						delete gldProperty;
					}
				}
			} else {
				throw("Couldn't parse the json message for HelicsEndpoint %s. %s.", (*sub)->name.c_str(), parse_err.c_str());
			}
		}
	}
#endif
	return 1;
}

int helics_msg::publishJsonVariables(){
	OBJECT *obj = OBJECTHDR(this);
	char buf[1024] = "";
	string simName = string(gl_name(obj, buf, 1023));
	Json::Value jsonPublishData;
	gld_property *prop = nullptr;
	jsonPublishData.clear();
	jsonPublishData[simName];
	std::stringstream complex_val;
	Json::StreamWriterBuilder jsonWriterBuilder;
	string jsonMessageStr = "";
#if HAVE_HELICS
	for(vector<json_helics_value_publication*>::iterator pub = json_helics_value_publications.begin(); pub != json_helics_value_publications.end(); pub++){
		jsonPublishData[simName].clear();
		for(vector<json_publication*>::iterator o = (*pub)->jsonPublications.begin(); o != (*pub)->jsonPublications.end(); o++) {
			prop = (*o)->prop;
			if(prop->is_valid()){
				if(!jsonPublishData[simName].isMember((*o)->object_name)){
					jsonPublishData[simName][(*o)->object_name];
				}
				if(prop->is_double()){
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = prop->get_double();
				} else if(prop->is_complex()){
					double real_part = prop->get_part("real");
					double imag_part =prop->get_part("imag");
					gld_unit *val_unit = prop->get_unit();
					complex_val.str(string());
					complex_val << std::fixed << real_part;
					if(imag_part >= 0){
						complex_val << std::fixed << "+" << fabs(imag_part) << "j";
					} else {
						complex_val << std::fixed << imag_part << "j";
					}
					if(val_unit != nullptr && val_unit->is_valid()){
						string unit_name = string(val_unit->get_name());
						complex_val << " " << unit_name;
					}
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = complex_val.str();
				} else if(prop->is_integer()){
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = (Json::Value::Int64)prop->get_integer();
				} else if(prop->is_timestamp()){
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = (Json::Value::Int64)prop->get_timestamp();
				} else {
					char chTemp[1024];
					prop->to_string(chTemp, 1023);
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = string((char *)chTemp);
				}
			}
		}
		jsonMessageStr = Json::writeString(jsonWriterBuilder, jsonPublishData);
		(*pub)->HelicsPublication.publish(jsonMessageStr);
	}
	for(vector<json_helics_endpoint_publication*>::iterator pub = json_helics_endpoint_publications.begin(); pub != json_helics_endpoint_publications.end(); pub++){
		jsonPublishData[simName].clear();
		for(vector<json_publication*>::iterator o = (*pub)->jsonPublications.begin(); o != (*pub)->jsonPublications.end(); o++) {
			prop = (*o)->prop;
			if(prop->is_valid()){
				if(!jsonPublishData[simName].isMember((*o)->object_name)){
					jsonPublishData[simName][(*o)->object_name];
				}
				if(prop->is_double()){
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = prop->get_double();
				} else if(prop->is_complex()){
					double real_part = prop->get_part("real");
					double imag_part =prop->get_part("imag");
					gld_unit *val_unit = prop->get_unit();
					complex_val.str(string());
					complex_val << std::fixed << real_part;
					if(imag_part >= 0){
						complex_val << std::fixed << "+" << fabs(imag_part) << "j";
					} else {
						complex_val << std::fixed << imag_part << "j";
					}
					if(val_unit != nullptr && val_unit->is_valid()){
						string unit_name = string(val_unit->get_name());
						complex_val << " " << unit_name;
					}
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = complex_val.str();
				} else if(prop->is_integer()){
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = (Json::Value::Int64)prop->get_integer();
				} else if(prop->is_timestamp()){
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = (Json::Value::Int64)prop->get_timestamp();
				} else {
					char chTemp[1024];
					prop->to_string(chTemp, 1023);
					jsonPublishData[simName][(*o)->object_name][(*o)->property_name] = string((char *)chTemp);
				}
			}
		}
		helicscpp::Message *msg = new helicscpp::Message((*pub)->HelicsPublicationEndpoint);
		jsonMessageStr = Json::writeString(jsonWriterBuilder, jsonPublishData);
		msg->data(jsonMessageStr);
		(*pub)->HelicsPublicationEndpoint.sendMessage(*msg);
	}
#endif
	return 1;
}

int helics_msg::subscribeJsonVariables(){
	OBJECT *obj = OBJECTHDR(this);
	char buf[1024] = "";
	string simName = string(gld_helics_federate->getName());
	Json::Value jsonMessage;
	Json::Value jsonData;
	Json::CharReaderBuilder jsonReaderBuilder;
	Json::String parse_err;
	string value = "";
	string objectName = "";
	string propertyName = "";
	gld_property *gldProperty = nullptr;
#if HAVE_HELICS
	for(vector<json_helics_value_subscription*>::iterator sub = json_helics_value_subscriptions.begin(); sub != json_helics_value_subscriptions.end(); sub++){
		if((*sub)->HelicsSubscription.isUpdated()){
			value = (*sub)->HelicsSubscription.getString();
			const int value_length = static_cast<int>(value.size());
			const std::unique_ptr<Json::CharReader> jReader(jsonReaderBuilder.newCharReader());
			if(jReader->parse(value.c_str(),value.c_str() + value_length, &jsonMessage, &parse_err)){
				if(!jsonMessage.isMember(simName.c_str())){
					jsonData = jsonMessage;
					for(Json::ValueIterator o = jsonData.begin(); o != jsonData.end(); o++){
						objectName = o.name();
						for(Json::ValueIterator p = jsonData[objectName].begin(); p != jsonData[objectName].end(); p++){
							propertyName = p.name();
							const char *expr1 = objectName.c_str();
							const char *expr2 = propertyName.c_str();
							char *bufObj = new char[strlen(expr1)+1];
							char *bufProp = new char[strlen(expr2)+1];
							strcpy(bufObj, expr1);
							strcpy(bufProp, expr2);
							gldProperty = new gld_property(bufObj, bufProp);
							if(gldProperty->is_valid()){
								if(gldProperty->is_integer()){
									int64_t itmp = jsonData[objectName][propertyName].asInt64();
									gld_type property_type = gldProperty->get_type();
									if(property_type == PT_int64){
										gldProperty->setp(itmp);
									} else if(property_type == PT_int32){
										int32_t int32_temp = static_cast<int32_t>(itmp);
										gldProperty->setp(int32_temp);
									}else if(property_type == PT_int16){
										int16_t int16_temp = static_cast<int16_t>(itmp);
										gldProperty->setp(int16_temp);
									}
								} else if(gldProperty->is_double()){
									double dtmp = jsonData[objectName][propertyName].asDouble();
									gldProperty->setp(dtmp);
								} else {
									string stmp = jsonData[objectName][propertyName].asString();
									char sbuf[1024] = "";
									strncpy(sbuf, stmp.c_str(), 1023);
									gldProperty->from_string(sbuf);
								}
							}
							delete gldProperty;
						}
					}
				} else {
					jsonData = jsonMessage[simName];
					for(Json::ValueIterator o = jsonData.begin(); o != jsonData.end(); o++){
						objectName = o.name();
						for(Json::ValueIterator p = jsonData[objectName].begin(); p != jsonData[objectName].end(); p++){
							propertyName = p.name();
							const char *expr1 = objectName.c_str();
							const char *expr2 = propertyName.c_str();
							char *bufObj = new char[strlen(expr1)+1];
							char *bufProp = new char[strlen(expr2)+1];
							strcpy(bufObj, expr1);
							strcpy(bufProp, expr2);
							gldProperty = new gld_property(bufObj, bufProp);
							if(gldProperty->is_valid()){
								if(gldProperty->is_integer()){
									int64_t itmp = jsonData[objectName][propertyName].asInt64();
									gld_type property_type = gldProperty->get_type();
									if(property_type == PT_int64){
										gldProperty->setp(itmp);
									} else if(property_type == PT_int32){
										int32_t int32_temp = static_cast<int32_t>(itmp);
										gldProperty->setp(int32_temp);
									}else if(property_type == PT_int16){
										int16_t int16_temp = static_cast<int16_t>(itmp);
										gldProperty->setp(int16_temp);
									}
								} else if(gldProperty->is_double()){
									double dtmp = jsonData[objectName][propertyName].asDouble();
									gldProperty->setp(dtmp);
								} else {
									string stmp = jsonData[objectName][propertyName].asString();
									char sbuf[1024] = "";
									strncpy(sbuf, stmp.c_str(), 1023);
									gldProperty->from_string(sbuf);
								}
							}
							delete gldProperty;
						}
					}
				}
			} else {
				throw("Couldn't parse the json message for HelicsSubscription %s. %s.", (*sub)->target.c_str(), parse_err.c_str());
			}
		}
	}
	for(vector<json_helics_endpoint_subscription*>::iterator sub = json_helics_endpoint_subscriptions.begin(); sub != json_helics_endpoint_subscriptions.end(); sub++){
		if((*sub)->HelicsSubscriptionEndpoint.hasMessage()){
			helicscpp::Message mesg;
			int pendingMessages = (int) (*sub)->HelicsSubscriptionEndpoint.pendingMessageCount();
			for(int i = 0; i < pendingMessages; i++) {
				gl_verbose("calling getMessage() for endpoint %s", (*sub)->name.c_str());
				mesg = (*sub)->HelicsSubscriptionEndpoint.getMessage();
			}
			const string message_buffer = string(mesg.c_str());
			const int message_length = static_cast<int>(message_buffer.size());
			const std::unique_ptr<Json::CharReader> jReader(jsonReaderBuilder.newCharReader());
			if(jReader->parse(message_buffer.c_str(),message_buffer.c_str() + message_length, &jsonMessage, &parse_err)){
				if(!jsonMessage.isMember(simName.c_str())){
					jsonData = jsonMessage;
					for(Json::ValueIterator o = jsonData.begin(); o != jsonData.end(); o++){
						objectName = o.name();
						for(Json::ValueIterator p = jsonData[objectName].begin(); p != jsonData[objectName].end(); p++){
							propertyName = p.name();
							const char *expr1 = objectName.c_str();
							const char *expr2 = propertyName.c_str();
							char *bufObj = new char[strlen(expr1)+1];
							char *bufProp = new char[strlen(expr2)+1];
							strcpy(bufObj, expr1);
							strcpy(bufProp, expr2);
							gldProperty = new gld_property(bufObj, bufProp);
							if(gldProperty->is_valid()){
								if(gldProperty->is_integer()){
									int64_t itmp = jsonData[objectName][propertyName].asInt64();
									gld_type property_type = gldProperty->get_type();
									if(property_type == PT_int64){
										gldProperty->setp(itmp);
									} else if(property_type == PT_int32){
										int32_t int32_temp = static_cast<int32_t>(itmp);
										gldProperty->setp(int32_temp);
									}else if(property_type == PT_int16){
										int16_t int16_temp = static_cast<int16_t>(itmp);
										gldProperty->setp(int16_temp);
									}
								} else if(gldProperty->is_double()){
									double dtmp = jsonData[objectName][propertyName].asDouble();
									gldProperty->setp(dtmp);
								} else {
									string stmp = jsonData[objectName][propertyName].asString();
									char sbuf[1024] = "";
									strncpy(sbuf, stmp.c_str(), 1023);
									gldProperty->from_string(sbuf);
								}
							}
							delete gldProperty;
						}
					}
				} else {
					jsonData = jsonMessage[simName];
					for(Json::ValueIterator o = jsonData.begin(); o != jsonData.end(); o++){
						objectName = o.name();
						for(Json::ValueIterator p = jsonData[objectName].begin(); p != jsonData[objectName].end(); p++){
							propertyName = p.name();
							const char *expr1 = objectName.c_str();
							const char *expr2 = propertyName.c_str();
							char *bufObj = new char[strlen(expr1)+1];
							char *bufProp = new char[strlen(expr2)+1];
							strcpy(bufObj, expr1);
							strcpy(bufProp, expr2);
							gldProperty = new gld_property(bufObj, bufProp);
							if(gldProperty->is_valid()){
								if(gldProperty->is_integer()){
									int64_t itmp = jsonData[objectName][propertyName].asInt64();
									gld_type property_type = gldProperty->get_type();
									if(property_type == PT_int64){
										gldProperty->setp(itmp);
									} else if(property_type == PT_int32){
										int32_t int32_temp = static_cast<int32_t>(itmp);
										gldProperty->setp(int32_temp);
									}else if(property_type == PT_int16){
										int16_t int16_temp = static_cast<int16_t>(itmp);
										gldProperty->setp(int16_temp);
									}
								} else if(gldProperty->is_double()){
									double dtmp = jsonData[objectName][propertyName].asDouble();
									gldProperty->setp(dtmp);
								} else {
									string stmp = jsonData[objectName][propertyName].asString();
									char sbuf[1024] = "";
									strncpy(sbuf, stmp.c_str(), 1023);
									gldProperty->from_string(sbuf);
								}
							}
							delete gldProperty;
						}
					}
				}
			} else {
				throw("Couldn't parse the json message for HelicsEndpoint %s. %s.", (*sub)->name.c_str(), parse_err.c_str());
			}
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
	char lo = nullptr;
	char hi = nullptr;
	char c = nullptr;
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
	if(relay == nullptr){
		throw("helics_msg::outgoing_route_function: the relay function for function name %s could not be found.", funcName);
	}
	if( relay->drtn != DXD_WRITE){
		throw("helics_msg:outgoing_helics_function: the relay function for the function name ?s could not be found.", funcName);
	}
	char message[3000] = "";

	size_t msglen = 0;

	// check from and to names
	if ( to==nullptr || from==nullptr )
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
	if ( relay!=nullptr )
	{
		gl_error("helics_msg::add_helics_function(rclass='%s', rname='%s') a relay function is already defined for '%s/%s'", rclass,rname,rclass,rname);
		return 0;
	}

	// allocate space for relay info
	relay = (FUNCTIONSRELAY*)malloc(sizeof(FUNCTIONSRELAY));
	if ( relay==nullptr )
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
		return nullptr;
	}
}

extern "C" FUNCTIONSRELAY *find_helics_function(const char *rclass, const char*rname)
{
	// TODO: this is *very* inefficient -- a hash should be used instead
	FUNCTIONSRELAY *relay;
	for ( relay=first_helicsfunction ; relay!=nullptr ; relay=relay->next )
	{
		if (strcmp(relay->remotename, rname)==0 && strcmp(relay->remoteclass, rclass)==0)
			return relay;
	}
	return nullptr;
}*/





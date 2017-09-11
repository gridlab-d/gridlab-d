/** $Id$
 * FNCS message object
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "fncs_msg.h"

EXPORT_CREATE(fncs_msg);
EXPORT_INIT(fncs_msg);
EXPORT_PRECOMMIT(fncs_msg);
EXPORT_SYNC(fncs_msg);
EXPORT_COMMIT(fncs_msg);
EXPORT_FINALIZE(fncs_msg);
EXPORT_NOTIFY(fncs_msg);
EXPORT_PLC(fncs_msg);
EXPORT_LOADMETHOD(fncs_msg,route);
EXPORT_LOADMETHOD(fncs_msg,option);
EXPORT_LOADMETHOD(fncs_msg,publish);
EXPORT_LOADMETHOD(fncs_msg,subscribe);
EXPORT_LOADMETHOD(fncs_msg,configure);

EXPORT TIMESTAMP clocks_update(void *ptr, TIMESTAMP t1)
{
	fncs_msg*my = (fncs_msg*)ptr;
	return my->clk_update(t1);
}

EXPORT SIMULATIONMODE dInterupdate(void *ptr, unsigned int dIntervalCounter, TIMESTAMP t0, unsigned int64 dt)
{
	fncs_msg *my = (fncs_msg *)ptr;
	return my->deltaInterUpdate(dIntervalCounter, t0, dt);
}

EXPORT SIMULATIONMODE dClockupdate(void *ptr, double t1, unsigned long timestep, SIMULATIONMODE sysmode)
{
	fncs_msg *my = (fncs_msg *)ptr;
	return my->deltaClockUpdate(t1, timestep, sysmode);
}

static FUNCTIONSRELAY *first_fncsfunction = NULL;

CLASS *fncs_msg::oclass = NULL;
fncs_msg *fncs_msg::defaults = NULL;

//Constructor
fncs_msg::fncs_msg(MODULE *module)
{
	// register to receive notice for first top down. bottom up, and second top down synchronizations
	oclass = gld_class::create(module,"fncs_msg",sizeof(fncs_msg),PC_AUTOLOCK|PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_OBSERVER);
	if (oclass == NULL)
		throw "connection/fncs_msg::fncs_msg(MODULE*): unable to register class connection:fncs_msg";
	else
		oclass->trl = TRL_UNKNOWN;

	defaults = this;
	if (gl_publish_variable(oclass,
		PT_double, "version", get_version_offset(), PT_DESCRIPTION, "fncs_msg version",
		PT_enumeration, "message_type", PADDR(message_type), PT_DESCRIPTION, "set the type of message format you wish to construct",
			PT_KEYWORD, "GENERAL", enumeration(MT_GENERAL), PT_DESCRIPTION, "use this for sending a general fncs topic/value pair",
			PT_KEYWORD, "JSON", enumeration(MT_JSON), PT_DESCRIPTION, "use this for wanting to send a bundled json formatted message in a single topic",
			// PT_KEYWORD, "JSON_SB", enumeration(MT_JSON_SB), PT_DESCRIPTION, "use this for wanting to subsribe a bundled json formatted message in a single topic",
		// TODO add published properties here
		NULL)<1)
			throw "connection/fncs_msg::fncs_msg(MODULE*): unable to publish properties of connection:fncs_msg";
	if ( !gl_publish_loadmethod(oclass,"route",loadmethod_fncs_msg_route) )
		throw "connection/fncs_msg::fncs_msg(MODULE*): unable to publish route method of connection:fncs_msg";
	if ( !gl_publish_loadmethod(oclass,"option",loadmethod_fncs_msg_option) )
		throw "connection/fncs_msg::fncs_msg(MODULE*): unable to publish option method of connection:fncs_msg";
	if ( !gl_publish_loadmethod(oclass,"publish",loadmethod_fncs_msg_publish) )
		throw "connection/fncs_msg::fncs_msg(MODULE*): unable to publish publish method of connection:fncs_msg";
	if ( !gl_publish_loadmethod(oclass,"subscribe",loadmethod_fncs_msg_subscribe) )
		throw "connection/fncs_msg::fncs_msg(MODULE*): unable to publish subscribe method of connection:fncs_msg";
	if ( !gl_publish_loadmethod(oclass,"configure",loadmethod_fncs_msg_configure) )
		throw "connection/fncs_msg::fncs_msg(MODULE*): unable to publish configure method of connection:fncs_msg";
}

int fncs_msg::create(){
	version = 1.0;
	message_type = MT_GENERAL;
	add_clock_update((void *)this,clocks_update);
	register_object_interupdate((void *)this, dInterupdate);
	register_object_deltaclockupdate((void *)this, dClockupdate);
	// setup all the variable maps
	for ( int n=1 ; n<14 ; n++ )
		vmap[n] = new varmap;
	port = new string("");
	header_version = new string("");
	hostname = new string("");
	inFunctionTopics = new vector<string>();

	return 1;
}

int fncs_msg::publish(char *value)
{
	// gl_warning("entering fncs_msg::publish()"); //renke debug

	int rv = 0;
	rv = fncs_link(value, CT_PUBSUB);
	return rv;
}

int fncs_msg::subscribe(char *value)
{
	// gl_warning("entering fncs_msg::subscribe()"); //renke debug

	int rv = 0;
	rv = fncs_link(value, CT_PUBSUB);
	return rv;
}

int fncs_msg::route(char *value)
{
	int rv = 0;
	rv = fncs_link(value, CT_ROUTE);
	return rv;
}

int fncs_msg::option(char *value){
	int rv = 0;
	char target[256];
	char command[1024];
	char *cmd;
	string p;
	string hversion;
	string host;
	// parse the pseudo-property
	if ( sscanf(value,"%[^:]:%[^\n]", target, command)==2 )
	{
		gl_verbose("fncs_msg::option(char *value='%s') parsed ok", value);
	}
	else
	{
		gl_error("fncs_msg::option(char *value='%s'): unable to parse option argument", value);
		return 0;
	}
	cmd = &command[0];
	if(strncmp(target, "connection", 10) == 0){
		gl_warning("connection is always a client when operating with fncs. ingnoring option.");
		rv = 1;
	} else if(strncmp(target, "transport", 9) == 0){
		char param[256], val[1024];
		while ( cmd!=NULL && *cmd!='\0' )
		{
			memset(param,'\0',256);
			memset(val,'\0',1024);
			switch ( sscanf(cmd,"%256[^ =]%*[ =]%[^,;]",param,val) ) {
			case 1:
				gl_error("fncs_msg::option \"transport:%s\" not recognized", cmd);
				return 0;
			case 2:
				if ( strcmp(param,"port")==0 )
				{
					*port = val;
				}
				else if ( strcmp(param,"header_version")==0 )
				{
					*header_version = val;
				}
				else if ( strcmp(param,"hostname")==0 )
				{
					*hostname = val;
				}
				else
				{
					error("fncs_msg::option \"transport:%s\" not recognized", cmd);
					return 0;
				}
				break;
			default:
				error("fncs_msg::option \"transport:%s\" cannot be parsed", cmd);
				return 0;
			}
			char *comma = strchr(cmd,',');
			char *semic = strchr(cmd,';');
			if ( comma && semic )
				cmd = min(comma,semic);
			else if ( comma )
				cmd = comma;
			else if ( semic )
				cmd = semic;
			else
				cmd = NULL;
			if ( cmd )
			{
				while ( isspace(*cmd) || *cmd==',' || *cmd==';' ) cmd++;
			}
		}
		rv = 1;
	}
	return rv;
}

int fncs_msg::configure(char *value)
{
	int rv = 1;
	strcpy(configFile, value);
	if (strcmp(configFile, "") != 0) {
		ifstream ifile;
		string confLine;
		string value = "";

		char buf[1024];
		char cval[1024];

		ifile.open(configFile, ifstream::in);
		if (ifile.good()) {
			if (message_type == MT_GENERAL) { // renke debug
				while (!ifile.eof()) {
					memset(buf, '\0', 1024);// clear the buffer
					ifile.getline(buf, 1024);// place the line in the buffer
					confLine = string(buf);
					if (confLine.find("publish") != string::npos){
						gl_verbose("fncs_msg.configure(): processing line: %s", confLine.c_str());
						value = confLine.substr(confLine.find("publish") + 9, confLine.length() - confLine.find("publish") - 9 - 2);
						strcpy(cval, value.c_str());
						rv = publish(&cval[0]);
						if (rv == 0) {
							return 0;
						}
					} else if (confLine.find("subscribe") != string::npos) {
						gl_verbose("fncs_msg.configure(): processing line: %s", confLine.c_str());
						value = confLine.substr(confLine.find("subscribe") + 11, confLine.length() - confLine.find("subscribe") - 11 - 2);
						strcpy(cval, value.c_str());
						rv = subscribe(&cval[0]);
						if (rv == 0) {
							return 0;
						}
					} else if (confLine.find("route") != string::npos) {
						gl_verbose("fncs_msg.configure(): processing line: %s", confLine.c_str());
						value = confLine.substr(confLine.find("route") + 7, confLine.length() - confLine.find("route") - 7 - 2);
						strcpy(cval, value.c_str());
						rv = route(&cval[0]);
						if (rv == 0) {
							return 0;
						}
					} else if (confLine.find("option") != string::npos) {
						gl_verbose("fncs_msg.configure(): processing line: %s", confLine.c_str());
						value = confLine.substr(confLine.find("option") + 8, confLine.length() - confLine.find("option") - 8 - 2);
						strcpy(cval, value.c_str());
						rv = option(&cval[0]);
						if (rv == 0) {
							return 0;
						}
					}
				}  // end of while (!ifile.eof())
			} //end of if (message_type == MT_GENERAL)
			else if (message_type == MT_JSON) {
				stringstream json_config_stream ("");
				string json_config_line;
				string json_config_string;
				Json::Reader json_reader;
				while (ifile >> json_config_line) { //Place the entire contents of the file into a stringstream
					json_config_stream << json_config_line << "\n";
				}
				json_config_string = json_config_stream.str();
				gl_verbose("fncs_msg::configure(): json string read from configure file: %s .\n", json_config_string.c_str()); //renke debug
				json_reader.parse(json_config_string, publish_json_config);

				//test code start

				//Json::FastWriter jsonwriter;
				//string pubjsonstr;
				//pubjsonstr = jsonwriter.write(publish_json_config);
				//gl_verbose("fncs_msg::configure(): string from publish_json_config: %s .\n", pubjsonstr.c_str()); //renke debug

				//test code end

				rv = publish_fncsjson_link();
				if (rv == 0) {
					return 0;
				}
			}
		}
		//else if(message_type == MT_JSON_SB){
		//	rv = 1;

		//}
		else {
			gl_error("fncs_msg::configure(): failed to open the configuration file %s \n", configFile.get_string());
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
		fncs::die();
	} else {
		fncs::finalize();
	}
}

int fncs_msg::init(OBJECT *parent){

	gl_verbose("entering fncs_msg::init()");

	int rv;
#if HAVE_FNCS
	rv = 1;
#else
	gl_error("fncs_msg::init ~ fncs was not linked with GridLAB-D at compilation. fncs_msg cannot be used if fncs was not linked with GridLAB-D.");
	rv = 0;
#endif
	if (rv == 0)
	{
		return 0;
	}
	//write zplfile
	stringstream zplfile;
	VARMAP *pMap;
	FUNCTIONSRELAY *relay;
	bool uniqueTopic;
	bool defer = false;
	vector<string> inVariableTopics;
	vector<string> inFunctionTopcis;
	int n = 0;
	int i = 0;
	int d = 0;
	OBJECT *obj = OBJECTHDR(this);
	OBJECT *vObj = NULL;
	char buffer[1024] = "";
	string simName = string(gl_name(obj, buffer, 1023));
	string dft;
	char defaultBuf[1024] = "";
	string type;
	if(hostname->empty()){
		*hostname = "localhost";
	}
	if(port->empty()){
		*port = "39036";
	}
	//resolve all the map variables
	for(n = 1; n < 14; n++){
		vmap[n]->resolve();
		//defer till all other objects have initialized
		for(pMap = vmap[n]->getfirst(); pMap != NULL; pMap = pMap->next){
			vObj = pMap->obj->get_object();
			if((vObj->flags & OF_INIT) != OF_INIT){
				defer = true;
			}
		}
	}

	if(defer == true){
		gl_verbose("fncs_msg::init(): %s is defering initialization.", obj->name);
		return 2;
	}

	// resolve all the json_configure gl_properties for publishing json style string, renke
	// TODO
	// gl_warning("before resolve all json objects, defer: %d \n", defer);

	int nsize = vjson_publish_gld_property_name.size();
	gld_property *gldpro_obj;
	OBJECT *gld_obj;
	for (int isize=0 ; isize<nsize ; isize++){
		if(!vjson_publish_gld_property_name[isize]->is_header) {
			const string gld_property_name = vjson_publish_gld_property_name[isize]->object_name + "." + vjson_publish_gld_property_name[isize]->object_property;
			const char *expr1 = vjson_publish_gld_property_name[isize]->object_name.c_str();
			const char *expr2 = vjson_publish_gld_property_name[isize]->object_property.c_str();
			char *bufObj = new char[strlen(expr1)+1];
			char *bufProp = new char[strlen(expr2)+1];
			strcpy(bufObj, expr1);
			strcpy(bufProp, expr2);
			vjson_publish_gld_property_name[isize]->prop = new gld_property(bufObj,bufProp);

			if ( vjson_publish_gld_property_name[isize]->prop->is_valid() ){
				gl_verbose("connection: local variable '%s' resolved OK, object id %d",
						gld_property_name.c_str(), vjson_publish_gld_property_name[isize]->prop->get_object()->id); //renke debug
			}
			else {
				gl_error("connection: local variable '%s' cannot be resolved", gld_property_name.c_str());
				return 0;
			}
		} else {
			if(vjson_publish_gld_property_name[isize]->object_property.compare("parent") == 0) {
				char objname[1024];
				const char *oName = vjson_publish_gld_property_name[isize]->object_name.c_str();
				strcpy(objname, oName);
				vjson_publish_gld_property_name[isize]->obj = gl_get_object(objname);
				vjson_publish_gld_property_name[isize]->hdr_val = string(vjson_publish_gld_property_name[isize]->obj->parent->name);
			}
		}
	}

	for (int isize=0 ; isize<nsize ; isize++){
		if(!vjson_publish_gld_property_name[isize]->is_header) {
			vObj = vjson_publish_gld_property_name[isize]->prop->get_object();
		} else {
			vObj = vjson_publish_gld_property_name[isize]->obj;
		}
		if((vObj->flags & OF_INIT) != OF_INIT){
			gl_warning("%d, ncs_msg::init(): vjson_publish_gld_property %s.%s not initialized!:  \n", isize,
					vjson_publish_gld_property_name[isize]->object_name.c_str(), vjson_publish_gld_property_name[isize]->object_property.c_str());
			defer = true;
		}
	}

	if(defer == true){
		gl_warning("fncs_msg::init(): %s is deferring initialization.", obj->name);
		return 2;
	}


	//create zpl file for registering with fncs
	if (message_type == MT_GENERAL){
		zplfile << "name = " << simName << endl;
		zplfile << "time_delta = 1000000000ns" << endl; //TODO: minimum timestep needs to take into account deltamode steps eventually.
		zplfile << "broker = tcp://" << *hostname << ":" << *port << endl;
		zplfile << "values" << endl;
		for(n = 1; n < 14; n++){
			if( n >= 1 && n <= 13){
				for(pMap = vmap[n]->getfirst(); pMap != NULL; pMap = pMap->next){
					if(pMap->dir == DXD_READ){
						uniqueTopic = true;
						for(i = 0; i < inVariableTopics.size(); i++){
							if((*inFunctionTopics)[i].compare(string(pMap->remote_name)) == 0){
								uniqueTopic = false;
							}
						}
						if(uniqueTopic == true){
							inFunctionTopics->push_back(string(pMap->remote_name));
							if( pMap->obj->to_string(&defaultBuf[0], 1024 ) < 0){
								dft = "NA";
							} else {
								dft = string(defaultBuf);
							}
							zplfile << "    " << string(pMap->remote_name) << endl;
							zplfile << "        topic = " << string(pMap->remote_name) << endl;
							if(dft.empty() != true){
								zplfile << "        default = " << dft << endl;
							}
							if(type.empty() != true){
								zplfile << "        type = " << type << endl;
							}
							zplfile << "        list = false" << endl;
						}
					}
				}
			}
		}
	}
	else if (message_type == MT_JSON){

			string default_json_str = string("{\"")+simName+string("\":{}}");

			zplfile << "name = " << simName << endl;
			zplfile << "time_delta = 1000000000ns" << endl; //TODO: minimum timestep needs to take into account deltamode steps eventually.
			zplfile << "broker = tcp://" << *hostname << ":" << *port << endl;
			zplfile << "values" << endl;
			zplfile << "    " << simName <<"/fncs_input" << endl;
			zplfile << "        topic = " << simName << "/fncs_input" << endl;
			zplfile << "        default = " << default_json_str << endl;
			zplfile << "        type = " << "JSON" << endl;
			zplfile << "        list = false" << endl;

	}

	//get a string vector of the unique function subscriptions
	for(relay = first_fncsfunction; relay != NULL; relay = relay->next){
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
	}
	//register with fncs
	printf("%s",zplfile.str().c_str());
	fncs::initialize(zplfile.str());
	atexit(send_die);
	last_approved_fncs_time = gl_globalclock;
	last_delta_fncs_time = (double)(gl_globalclock);
	initial_sim_time = gl_globalclock;
	return rv;
}

int fncs_msg::precommit(TIMESTAMP t1){
	int result = 0;

	if (message_type == MT_GENERAL){

		//process external function calls
		incoming_fncs_function();
		//publish precommit variables
		result = publishVariables(vmap[4]);
		if(result == 0){
			return result;
		}
		//read precommit variables from cache
		result = subscribeVariables(vmap[4]);
		if(result == 0){
			return result;
		}
	}
	// read precommit json variables from GridAPPSD, renke
	//TODO
	else if (message_type == MT_JSON)
	{
		result = subscribeJsonVariables();
		if(result == 0){
			return result;
		}
	}

	return 1;
}

TIMESTAMP fncs_msg::presync(TIMESTAMP t1){

	int result = 0;
	result = publishVariables(vmap[5]);
	if(result == 0){
		return TS_INVALID;
	}
	//read presync variables from cache
	result = subscribeVariables(vmap[5]);
	if(result == 0){
		return TS_INVALID;
	}
	return TS_NEVER;
}

TIMESTAMP fncs_msg::plc(TIMESTAMP t1){

	int result = 0;
	result = publishVariables(vmap[12]);
	if(result == 0){
		return TS_INVALID;
	}
	//read plc variables from cache
	result = subscribeVariables(vmap[12]);
	if(result == 0){
		return TS_INVALID;
	}
	return TS_NEVER;
}

TIMESTAMP fncs_msg::sync(TIMESTAMP t1){

	int result = 0;
	TIMESTAMP t2;
	result = publishVariables(vmap[6]);
	if(result == 0){
		return TS_INVALID;
	}
	//read sync variables from cache
	result = subscribeVariables(vmap[6]);
	if(result == 0){
		return TS_INVALID;
	}

	if (message_type == MT_GENERAL)
		return TS_NEVER;
	else if (message_type == MT_JSON ){
		t2=t1+1;
		return t2;
	}
}

TIMESTAMP fncs_msg::postsync(TIMESTAMP t1){

	int result = 0;
	result = publishVariables(vmap[7]);
	if(result == 0){
		return TS_INVALID;
	}
	//read postsync variables from cache
	result = subscribeVariables(vmap[7]);
	if(result == 0){
		return TS_INVALID;
	}
	return TS_NEVER;
}

TIMESTAMP fncs_msg::commit(TIMESTAMP t0, TIMESTAMP t1){

	int result = 0;
	result = publishVariables(vmap[8]);
	if(result == 0){
		return TS_INVALID;
	}

	// publish json_configure variables, renke
	// TODO
	if (message_type == MT_JSON)
	{
		result = publishJsonVariables();
		if(result == 0){
			return TS_INVALID;
		}
	}

	//read commit variables from cache
	// put a if to check the message_type
	result = subscribeVariables(vmap[8]);
	if(result == 0){
		return TS_INVALID;
	}
	return TS_NEVER;
}

int fncs_msg::prenotify(PROPERTY* p,char* v){

	int result = 0;
	//publish prenotify variables
	result = publishVariables(vmap[9]);
	if(result == 0){
		return result;
	}
	//read prenotify variables from cache
	result = subscribeVariables(vmap[9]);
	if(result == 0){
		return result;
	}
	return 1;
}

int fncs_msg::postnotify(PROPERTY* p,char* v){

	int result = 0;
	//publish postnotify variables
	result = publishVariables(vmap[10]);
	if(result == 0){
		return result;
	}
	//read postnotify variables from cache
	result = subscribeVariables(vmap[10]);
	if(result == 0){
		return result;
	}
	return 1;
}

SIMULATIONMODE fncs_msg::deltaInterUpdate(unsigned int delta_iteration_counter, TIMESTAMP t0, unsigned int64 dt)
{
	int result = 0;
	gld_global dclock("deltaclock");
	if (!dclock.is_valid()) {
		gl_error("fncs_msg::deltaInterUpdate: Unable to find global deltaclock!");
		return SM_ERROR;
	}
	if(dclock.get_int64() > 0){
		if(delta_iteration_counter == 0){
			//publish commit variables
			result = publishVariables(vmap[8]);
			if(result == 0){
				return SM_ERROR;
			}
			//read commit variables from cache
			result = subscribeVariables(vmap[8]);
			if(result == 0){
				return SM_ERROR;
			}

			//process external function calls
			incoming_fncs_function();

			//publish precommit variables
			result = publishVariables(vmap[4]);
			if(result == 0){
				return SM_ERROR;
			}
			//read precommit variables from cache
			result = subscribeVariables(vmap[4]);
			if(result == 0){
				return SM_ERROR;
			}
			return SM_DELTA_ITER;
		}

		if(delta_iteration_counter == 1)
		{
			//publish presync variables
			result = publishVariables(vmap[5]);
			if(result == 0){
				return SM_ERROR;
			}
			//read presync variables from cache
			result = subscribeVariables(vmap[5]);
			if(result == 0){
				return SM_ERROR;
			}
			return SM_DELTA_ITER;
		}

		if(delta_iteration_counter == 2)
		{
			//publish plc variables
			result = publishVariables(vmap[12]);
			if(result == 0){
				return SM_ERROR;
			}
			//read plc variables from cache
			result = subscribeVariables(vmap[12]);
			if(result == 0){
				return SM_ERROR;
			}
			return SM_DELTA_ITER;
		}

		if(delta_iteration_counter == 3)
		{
			//publish sync variables
			result = publishVariables(vmap[6]);
			if(result == 0){
				return SM_ERROR;
			}
			//read sync variables from cache
			result = subscribeVariables(vmap[6]);
			if(result == 0){
				return SM_ERROR;
			}
			return SM_DELTA_ITER;
		}

		if(delta_iteration_counter == 4)
			{
			//publish postsync variables
			result = publishVariables(vmap[7]);
			if(result == 0){
				return SM_ERROR;
			}
			//read postsync variables from cache
			result = subscribeVariables(vmap[7]);
			if(result == 0){
				return SM_ERROR;
			}
		}
	}
	return SM_EVENT;
}

SIMULATIONMODE fncs_msg::deltaClockUpdate(double t1, unsigned long timestep, SIMULATIONMODE sysmode)
{
#if HAVE_FNCS
	if (t1 > last_delta_fncs_time){
		fncs::time fncs_time = 0;
		fncs::time t = 0;
		double dt = 0;
		dt = (t1 - (double)initial_sim_time) * 1000000000.0;
		t = (fncs::time)((dt + ((double)(timestep) / 2.0)) - fmod((dt + ((double)(timestep) / 2.0)), (double)timestep));
		fncs::update_time_delta((fncs::time)timestep);
		fncs_time = fncs::time_request(t);
		if(sysmode == SM_EVENT)
			exitDeltamode = true;
		if(fncs_time != t){
			gl_error("fncs_msg::deltaClockUpdate: Cannot return anything other than the time GridLAB-D requested in deltamode.");
			return SM_ERROR;
		} else {
			last_delta_fncs_time = (double)(fncs_time)/1000000000.0 + (double)(initial_sim_time);
			t1 = fncs_time;
		}
	}
#endif
	return SM_DELTA; // We should've only gotten here by being in SM_DELTA to begin with.
}

TIMESTAMP fncs_msg::clk_update(TIMESTAMP t1)
{
	// TODO move t1 back if you want, but not to global_clock or before
	TIMESTAMP fncs_time = 0;
	if(exitDeltamode == true){
#if HAVE_FNCS
		fncs::update_time_delta(1000000000);
#endif
		exitDeltamode = false;
		return t1;
	}
	if(t1 > last_approved_fncs_time){
		if(gl_globalclock == gl_globalstoptime){
			return t1;
		} else if (t1 > gl_globalstoptime && gl_globalclock < gl_globalstoptime){
			t1 = gl_globalstoptime;
		}
#if HAVE_FNCS
		fncs::time t = 0;
		t = (fncs::time)((t1 - initial_sim_time)*1000000000);
		fncs_time = ((TIMESTAMP)fncs::time_request(t))/1000000000 + initial_sim_time;
#endif

		if(fncs_time <= gl_globalclock){
			gl_error("fncs_msg::clock_update: Cannot return the current time or less than the current time.");
			return TS_INVALID;
		} else {
			last_approved_fncs_time = fncs_time;
			t1 = fncs_time;
		}
	}
	return t1;
}

int fncs_msg::finalize(){

	int nvecsize = vjson_publish_gld_property_name.size();
	for (int isize=0 ; isize<nvecsize ; isize++){
	   delete vjson_publish_gld_property_name[isize]->obj;
	   delete vjson_publish_gld_property_name[isize]->prop;
	}

	return 1;
}

int fncs_msg::get_varmapindex(const char *name)
{
	char *varmapname[] = {"","allow","forbid","init","precommit","presync","sync","postsync","commit","prenotify","postnotify","finalize","plc","term"};
	int n;
	for ( n=1 ; n<14 ; n++ )
	{
		if ( strcmp(varmapname[n],name)==0 )
			return n;
	}
	return 0;
}

int fncs_msg::fncs_link(char *value, COMMUNICATIONTYPE comtype){
	int rv = 0;
	int n = 0;
	char command[1024] = "";
	char argument[1024] = "";
	VARMAP *mp = NULL;
	//parse argument to fill the relay function link list and the varmap link list.
	if(sscanf(value, "%[^:]:%[^\n]", command, argument) == 2){
		if(strncmp(command,"init", 4) == 0){
			gl_warning("fncs_msg::publish: It is not possible to pass information at init time with fncs. communication is ignored");
			rv = 1;
		} else if(strncmp(command, "function", 8) == 0){
			rv = parse_fncs_function(argument, comtype);
		} else {
			n = get_varmapindex(command);
			if(n != 0){
				rv = vmap[n]->add(argument, comtype);
			}
		}
	} else {
		gl_error("fncs_msg::publish: Unable to parse input %s.", value);
		rv = 0;
	}
	return rv;
}
int fncs_msg::parse_fncs_function(char *value, COMMUNICATIONTYPE comtype){
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
		gl_error("fncs_msg::parse_fncs_function: Unable to parse input %s.", value);
		return rv;
	}
	// get local class structure
	fclass = callback->class_getname(localClass);
	if ( fclass==NULL )
	{
		gl_error("fncs_msg::parse_fncs_function(const char *spec='%s'): local class '%s' does not exist", value, localClass);
		return rv;
	}
	flocal = callback->function.get(localClass, localFuncName);
	// setup outgoing call
	if(strcmp(direction, "->") == 0){
		// check local class function map
		if ( flocal!=NULL )
			gl_warning("fncs_msg::parse_fncs_function(const char *spec='%s'): outgoing call definition of '%s' overwrites existing function definition in class '%s'",value,localFuncName,localClass);

		sscanf(topic, "%[^/]/%[^\n]", remoteClassName, remoteFuncName);
		// get relay function
		flocal = add_fncs_function(this,localClass, localFuncName,remoteClassName,remoteFuncName,NULL,DXD_WRITE, comtype);

		if ( flocal==NULL )
			return rv;

		// define relay function
		rv = callback->function.define(fclass,localFuncName,flocal)!=NULL;
		if(rv == 0){
			gl_error("fncs_msg::parse_fncs_function(const char *spec='%s'): failed to define the function '%s' in local class '%s'.", value, localFuncName, localClass);
			return rv;
		}
	// setup incoming call
	} else if ( strcmp(direction,"<-")==0 ){
		// check to see is local class function is valid
		if( flocal == NULL){
			gl_error("fncs_msg::parse_fncs_function(const char *spec='%s'): local function '%s' is not valid.",value, localFuncName);
			return 0;
		}
		flocal = add_fncs_function(this, localClass, localFuncName, "", topic, NULL, DXD_READ, comtype);
		if( flocal == NULL){
			rv = 1;
		}
	}
	return rv;
}

void fncs_msg::incoming_fncs_function()
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

	for(relay = first_fncsfunction; relay!=NULL; relay=relay->next){
		if(relay->drtn == DXD_READ){
#if HAVE_FNCS
			functionCalls = fncs::get_values(string(relay->remotename));
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
						throw("fncs_msg::incomming_fncs_function: unable to parse function message %s", message);
					}

					//check function is correct
					if(strcmp(funcName, relay->localcall) != 0){
						throw("fncs_msg::incomming_fncs_function: The remote side function call, %s, is not the same as the local function name, %s.", funcName, relay->localcall);
					}
					payloadLength = atoi(payloadLengthstr);
					payloadStringLength = payloadLength*2;
					void *rawPayload = new char[payloadLength];
					memset(rawPayload, 0, payloadLength);
					//unhex raw payload
					rplen = fncs_from_hex(rawPayload, (size_t)payloadLength, payloadString, (size_t)payloadStringLength);
					if( rplen < strlen(payloadString)){
						throw("fncs_msg::incomming_fncs_function: unable to decode function payload %s.", payloadString);
					}
					//call local function
					obj = gl_get_object(to);
					if( obj == NULL){
						throw("fncs_msg::incomming_fncs_function: the to object does not exist. %s.", to);
					}
					funcAddr = (FUNCTIONADDR)(gl_get_function(obj, relay->localcall));
					((void (*)(char *, char *, char *, char *, void *, size_t))(*funcAddr))(from, to, relay->localcall, relay->localclass, rawPayload, (size_t)payloadLength);
				}
			}
		}
	}
}

//publishes gld properties to the cache
int fncs_msg::publishVariables(varmap *wmap){
	VARMAP *mp;
	char buffer[1024] = "";
	char fromBuf[1024] = "";
	char toBuf[1024] = "";
	char keyBuf[1024] = "";
	string key;
	string value;
	string from;
	string to;
	complex cval;
	complex lst_cval;
	double dval;
	double lst_dval;
	int64 ival;
	int64 lst_ival;
	string lst_sval;
	bool pub_value = false;
	for(mp = wmap->getfirst(); mp != NULL; mp = mp->next){
		pub_value = false;
		if(mp->dir == DXD_WRITE){
			if( mp->obj->to_string(&buffer[0], 1023 ) < 0){
				value = "";
			} else {
				value = string(buffer);
			}
			if(value.empty() == false){
				if(strcmp(mp->threshold,"") == 0)
				{
					pub_value = true;
				} else {
					if(mp->obj->is_complex() == true)
					{
						cval = *(complex *)mp->obj->get_addr();
						if(mp->last_value == NULL)
						{
							pub_value = true;
							mp->last_value = (void *)(new complex(cval.Re(),cval.Im()));
						}
						else
						{
							lst_cval = *((complex *)(mp->last_value));
							if(fabs(cval.Mag() - lst_cval.Mag()) > atof(mp->threshold)){
								pub_value = true;
								memcpy(mp->last_value, (void *)(&cval), sizeof(cval));
							}
						}
					}
					else if(mp->obj->is_double() == true)
					{
						dval = *(double *)mp->obj->get_addr();
						if(mp->last_value == NULL)
						{
							pub_value = true;
							mp->last_value = (void *)(new double(dval));
						}
						else
						{
							lst_dval = *((double *)(mp->last_value));
							if(fabs(dval - lst_dval) > atof(mp->threshold)){
								pub_value = true;
								memcpy(mp->last_value, (void *)(&dval), sizeof(dval));
							}
						}
					}
					else if(mp->obj->is_integer() == true)
					{
						ival = *(int64 *)mp->obj->get_addr();
						if(mp->last_value == NULL)
						{
							pub_value = true;
							mp->last_value = (void *)(new int64(ival));
						}
						else
						{
							lst_ival = *((int64 *)(mp->last_value));
							if(fabs(ival - lst_ival) > atof(mp->threshold)){
								pub_value = true;
								memcpy(mp->last_value, (void *)(&ival), sizeof(ival));
							}
						}
					}
					else if(mp->obj->is_enumeration() == true || mp->obj->is_character() == true)
					{
						if(mp->last_value == NULL)
						{
							pub_value = true;
							mp->last_value = (void *)(new string(value));
						}
						else
						{
							lst_sval = *((string *)(mp->last_value));
							if(value.compare(lst_sval) != 0)
							{
								pub_value = true;
								memcpy(mp->last_value, (void *)(&value), sizeof(value));
							}
						}
					}
				}
				if(pub_value == true)
				{
					if(mp->ctype == CT_PUBSUB){
						key = string(mp->remote_name);
#if HAVE_FNCS
						fncs::publish(key, value);
#endif
					} else if(mp->ctype == CT_ROUTE){
						memset(fromBuf,'\0',1024);
						memset(toBuf,'\0',1024);
						memset(keyBuf,'\0',1024);
						if(sscanf(mp->local_name, "%[^.].", fromBuf) != 1){
							gl_error("fncs_msg::publishVariables: unable to parse 'from' name from %s.", mp->local_name);
							return 0;
						}
						if(sscanf(mp->remote_name, "%[^/]/%[^\n]", toBuf, keyBuf) != 2){
							gl_error("fncs_msg::publishVariables: unable to parse 'to' and 'key' from %s.", mp->remote_name);
							return 0;
						}
						from = string(fromBuf);
						to = string(toBuf);
						key = string(keyBuf);
#if HAVE_FNCS
						fncs::route(from, to, key, value);
#endif
					}
				}
			}
		}
	}
	return 1;
}

//read variables from the cache
int fncs_msg::subscribeVariables(varmap *rmap){
	string value = "";
	char valueBuf[1024] = "";
	VARMAP *mp = NULL;
	for(mp = rmap->getfirst(); mp != NULL; mp = mp->next){
		if(mp->dir == DXD_READ){
			if(mp->ctype == CT_PUBSUB){
#if HAVE_FNCS
				value = fncs::get_value(string(mp->remote_name));
#endif
				if(value.empty() == false){
					strncpy(valueBuf, value.c_str(), 1023);
					mp->obj->from_string(valueBuf);
				}
			}
		}
	}
	return 1;
}

int fncs_msg::publishJsonVariables( )  //Renke add
{
	gld_property *gldpro_obj;
	int nsize = vjson_publish_gld_property_name.size();
	int nvecsize = vjson_publish_gld_property_name.size();
	int vecidx = 0;

	OBJECT *obj = OBJECTHDR(this);
	char buffer[1024] = "";
	string simName = string(gl_name(obj, buffer, 1023));
	//simName = simName+"/fncs_output";

	//gl_verbose("entering fncs_msg::publishJsonVariables(): vjson_publish_gld_property_name size: %d \n", nvecsizeproname);
	//gl_verbose("entering fncs_msg::publishJsonVariables(): vjson_publish_gld_property size: %d \n", nvecsize);

	//need to clean the Json publish data first!!
	publish_json_data.clear();
	publish_json_data[simName];
	stringstream complex_val;
	for(int isize=0; isize<nsize; isize++) {
		if(!publish_json_data[simName].isMember(vjson_publish_gld_property_name[isize]->object_name)){
			publish_json_data[simName][vjson_publish_gld_property_name[isize]->object_name];
		}
		if(!vjson_publish_gld_property_name[isize]->is_header) {
			gldpro_obj = vjson_publish_gld_property_name[isize]->prop;
			if(gldpro_obj->is_double()) {
				publish_json_data[simName][vjson_publish_gld_property_name[isize]->object_name][vjson_publish_gld_property_name[isize]->object_property] = gldpro_obj->get_double();
			} else if (gldpro_obj->is_complex()) {
				double real_part = gldpro_obj->get_part("real");
				double imag_part =gldpro_obj->get_part("imag");
				gld_unit *val_unit = gldpro_obj->get_unit();
				complex_val.str(string());
				complex_val << fixed << real_part;
				if(imag_part >= 0){
					complex_val << fixed << "+" << imag_part << "j";
				} else {
					complex_val << fixed << imag_part << "j";
				}
				if(val_unit->is_valid()){
					string unit_name = string(val_unit->get_name());
					complex_val << " " << unit_name;
				}
				publish_json_data[simName][vjson_publish_gld_property_name[isize]->object_name][vjson_publish_gld_property_name[isize]->object_property] = complex_val.str();
			} else if (gldpro_obj->is_integer()) {
				publish_json_data[simName][vjson_publish_gld_property_name[isize]->object_name][vjson_publish_gld_property_name[isize]->object_property] = (Json::Value::Int64)gldpro_obj->get_integer();
			} else if (gldpro_obj->is_character() || gldpro_obj->is_enumeration() || gldpro_obj->is_complex() || gldpro_obj->is_objectref() || gldpro_obj->is_set()) {
				char chtmp[1024];
				gldpro_obj->to_string(chtmp, 1024);
				publish_json_data[simName][vjson_publish_gld_property_name[isize]->object_name][vjson_publish_gld_property_name[isize]->object_property] = string((char *)chtmp);
			} else {
				gl_error("fncs_msg::publishJsonVariables(): the type of the gld_property: %s.%s is not a recognized type! \n",vjson_publish_gld_property_name[isize]->object_name.c_str(), vjson_publish_gld_property_name[isize]->object_property.c_str() );
				return 0;
			}
		} else {
			publish_json_data[simName][vjson_publish_gld_property_name[isize]->object_name][vjson_publish_gld_property_name[isize]->object_property] = vjson_publish_gld_property_name[isize]->hdr_val;
		}
	}
	// write publish_json_data to a string and publish it through fncs API
	Json::FastWriter jsonwriter;
	string pubjsonstr;
	pubjsonstr = jsonwriter.write(publish_json_data);
	string skey = "";

	skey = "fncs_output";

	gl_verbose("fncs_msg::publishJsonVariables() fncs_publish: key: %s value %s \n", skey.c_str(),
							pubjsonstr.c_str());

#if HAVE_FNCS
	fncs::publish(skey, pubjsonstr);
#endif

	return 1;
}

int fncs_msg::subscribeJsonVariables( ) //Renke add
{
	// in this function, need to consider the gl_property resolve problem //renke
	// throw a warning inside subscribejsonvariables(); and return a 0

	string value = "";
	OBJECT *obj = OBJECTHDR(this);
	char buffer[1024] = "";
	string simName = string(gl_name(obj, buffer, 1023));
	string skey = simName+"/fncs_input";

#if HAVE_FNCS
	value = fncs::get_value(skey);
#endif

	gl_verbose("fncs_msg::subscribeJsonVariables(), skey: %s, reading json data as string: %s", skey.c_str(), value.c_str());

	if(value.empty() == false){

		Json::Value subscribe_json_data_full;

		Json::Reader json_reader;
		json_reader.parse(value, subscribe_json_data_full);

		//use isMember to check the simName is in the subscribe_json_data_full
		if (!subscribe_json_data_full.isMember(simName.c_str())){
			gl_warning("fncs_msg::subscribeJsonVariables(), the simName: %s is not a member in the subscribed json data!! \n",
					simName.c_str());
			return 1;

		}else {
			subscribe_json_data = subscribe_json_data_full[simName];
		}


		for (Json::ValueIterator it = subscribe_json_data.begin(); it != subscribe_json_data.end(); it++) {

			const string gldObjectName = it.name();
			double dtmp;
			int itmp;
			string stmp;
			const char * cstmp;

			for (Json::ValueIterator it1 = subscribe_json_data[it.name()].begin();
					it1 != subscribe_json_data[it.name()].end(); it1++){

				const string gldPropertyName = it1.name();
				string gldObjpropertyName = gldObjectName + ".";
				gldObjpropertyName = gldObjpropertyName + gldPropertyName;
				gld_property *gldpro_obj;

				const char *expr1 = gldObjectName.c_str();
				const char *expr2 = gldPropertyName.c_str();
				char *bufObj = new char[strlen(expr1)+1];
				char *bufProp = new char[strlen(expr2)+1];
				strcpy(bufObj, expr1);
				strcpy(bufProp, expr2);
				gldpro_obj = new gld_property(bufObj, bufProp);

				//gl_verbose("fncs_msg::subscribeJsonVariables(): %s is get from json data \n",
												//gldObjpropertyName.c_str());

				if ( gldpro_obj->is_valid() ){
					//gl_verbose("connection: local variable '%s' resolved OK, object id %d",
							//gldObjpropertyName.c_str(), gldpro_obj->get_object()->id);

					//get the value of the property
					Json::Value sub_value = subscribe_json_data[gldObjectName][gldPropertyName];

					//check the type of property and json value need to be the same
					if ( sub_value.isInt() && gldpro_obj->is_integer() ){
						itmp = sub_value.asInt();
						gldpro_obj->setp(itmp);
						gl_verbose("fncs_msg::subscribeJsonVariables(): %s is set value with int: %d \n",
								gldObjpropertyName.c_str(), itmp);

					}
					else if ( sub_value.isDouble()&& gldpro_obj->is_double()){
						dtmp = sub_value.asDouble();
						gldpro_obj->setp(dtmp);
						gl_verbose("fncs_msg::subscribeJsonVariables(): %s is set value with double: %f \n",
								gldObjpropertyName.c_str(), dtmp);

					}
					//if the gl_property type is char*, enumeration, or complex number
					else if ( sub_value.isString() &&
							(gldpro_obj->is_complex() || gldpro_obj->is_character() || gldpro_obj->is_enumeration()) ){

						char valueBuf[1024] = "";
						string subvaluestring = sub_value.asString();

						if(subvaluestring.empty() == false){
							strncpy(valueBuf, subvaluestring.c_str(), 1023);
							gldpro_obj->from_string(valueBuf);
						}
						gl_verbose("fncs_msg::subscribeJsonVariables(): %s is set value with : %s \n",
													gldObjpropertyName.c_str(), subvaluestring.c_str());
					}
					else {
						gl_error("fncs_msg::fncs json subscribe: fncs type does not match property type: ",
								gldObjpropertyName.c_str());
						delete gldpro_obj;
						return 0;
					}

					delete gldpro_obj;
				}  // end of the if condition to check whether gldpro_obj is valid
				else {
					gl_error("connection: local variable '%s' cannot be resolved", gldObjpropertyName.c_str());
					delete gldpro_obj;
					return 0;

				}

			} // end of second level for loop, ValueIterator it1

		} // end of first level for loop, ValueIterator it

	}  // end of if(value.empty() == false)

	return 1;
}

int fncs_msg::publish_fncsjson_link()  //Renke add
{
	// check whether the json configure has content
	if (publish_json_config.isNull()) {
		gl_warning(" publish json configure is empty!!! \n");
		return 1;
	}

	vjson_publish_gld_property_name.clear();
	JsonProperty *gldProperty = NULL;
	for (Json::ValueIterator it = publish_json_config.begin(); it != publish_json_config.end(); it++) {

		const string gldObjectName = it.name();
		string gldPropertyName;
		string gldObjpropertyName;


		int nsize = publish_json_config[gldObjectName].size();
		//gl_verbose("fncs_msg.publish_fncsjson_link(): gldObjectName: %s, nsize: %d . \n", gldObjectName.c_str(), nsize); //renke debug

		for (int isize=0; isize<nsize ; isize++) {
			gldPropertyName = publish_json_config[gldObjectName][isize].asString();
			gldProperty = new JsonProperty(gldObjectName, gldPropertyName);
			//gldObjpropertyName = gldObjectName + ".";
			//gldObjpropertyName = gldObjpropertyName + gldPropertyName;
			//gl_verbose("fncs_msg.publish_fncsjson_link(): processing json configure publish properties: %s \n",
					//gldObjpropertyName.c_str());  //renke debug
			vjson_publish_gld_property_name.push_back(gldProperty);
		}

	}

	return 1;
}

static char fncs_hex(char c)
{
	if ( c<10 ) return c+'0';
	else if ( c<16 ) return c-10+'A';
	else return '?';
}

static char fncs_unhex(char h)
{
	if ( h>='0' && h<='9' )
		return h-'0';
	else if ( h>='A' && h<='F' )
		return h-'A'+10;
	else if ( h>='a' && h<='f' )
		return h-'a'+10;
}

static size_t fncs_to_hex(char *out, size_t max, const char *in, size_t len)
{
	size_t hlen = 0;
	for ( size_t n=0; n<len ; n++,hlen+=2 )
	{
		char byte = in[n];
		char lo = in[n]&0xf;
		char hi = (in[n]>>4)&0xf;
		*out++ = fncs_hex(lo);
		*out++ = fncs_hex(hi);
		if ( hlen>=max ) return -1; // buffer overrun
	}
	*out = '\0';
	return hlen;
}

extern "C" size_t fncs_from_hex(void *buf, size_t len, const char *hex, size_t hexlen)
{
	char *p = (char*)buf;
	char lo = NULL;
	char hi = NULL;
	char c = NULL;
	size_t n = 0;
	for(n = 0; n < hexlen && *hex != '\0'; n += 2)
	{
		c = fncs_unhex(*hex);
		if ( c==-1 ) return -1; // bad hex data
		lo = c&0x0f;
		c = fncs_unhex(*(hex+1));
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
extern "C" void outgoing_fncs_function(char *from, char *to, char *funcName, char *funcClass, void *data, size_t len)
{
	int64 result = -1;
	char *rclass = funcClass;
	char *lclass = from;
	size_t hexlen = 0;
	FUNCTIONSRELAY *relay = find_fncs_function(funcClass, funcName);
	if(relay == NULL){
		throw("fncs_msg::outgoing_route_function: the relay function for function name %s could not be found.", funcName);
	}
	if( relay->drtn != DXD_WRITE){
		throw("fncs_msg:outgoing_fncs_function: the relay function for the function name ?s could not be found.", funcName);
	}
	char message[3000] = "";

	size_t msglen = 0;

	// check from and to names
	if ( to==NULL || from==NULL )
	{
		throw("from objects and to objects must be named.");
	}

	// convert data to hex
	hexlen = fncs_to_hex(message,sizeof(message),(const char*)data,len);

	if(hexlen > 0){
		//TODO: deliver message to fncs
		stringstream payload;
		char buffer[sizeof(len)];
		sprintf(buffer, "%d", len);
		payload << "\"{\"from\":\"" << from << "\", " << "\"to\":\"" << to << "\", " << "\"function\":\"" << funcName << "\", " <<  "\"data\":\"" << message << "\", " << "\"data length\":\"" << buffer <<"\"}\"";
		string key = string(relay->remotename);
		if( relay->ctype == CT_PUBSUB){
#if HAVE_FNCS
			fncs::publish(key, payload.str());
#endif
		} else if( relay->ctype == CT_ROUTE){
			string sender = string((const char *)from);
			string recipient = string((const char *)to);
#if HAVE_FNCS
			fncs::route(sender, recipient, key, payload.str());
#endif
		}
	}
}

extern "C" FUNCTIONADDR add_fncs_function(fncs_msg *route, const char *fclass, const char *flocal, const char *rclass, const char *rname, TRANSLATOR *xlate, DATAEXCHANGEDIRECTION direction, COMMUNICATIONTYPE ctype)
{
	// check for existing of relay (note only one relay is allowed per class pair)
	FUNCTIONSRELAY *relay = find_fncs_function(rclass, rname);
	if ( relay!=NULL )
	{
		gl_error("fncs_msg::add_fncs_function(rclass='%s', rname='%s') a relay function is already defined for '%s/%s'", rclass,rname,rclass,rname);
		return 0;
	}

	// allocate space for relay info
	relay = (FUNCTIONSRELAY*)malloc(sizeof(FUNCTIONSRELAY));
	if ( relay==NULL )
	{
		gl_error("fncs_msg::add_fncs_function(rclass='%s', rname='%s') memory allocation failed", rclass,rname);
		return 0;
	}

	// setup relay info
	strncpy(relay->localclass,fclass, sizeof(relay->localclass)-1);
	strncpy(relay->localcall,flocal,sizeof(relay->localcall)-1);
	strncpy(relay->remoteclass,rclass,sizeof(relay->remoteclass)-1);
	strncpy(relay->remotename,rname,sizeof(relay->remotename)-1);
	relay->drtn = direction;
	relay->next = first_fncsfunction;
	relay->xlate = xlate;

	// link to existing relay list (if any)
	relay->route = route;
	relay->ctype = ctype;
	first_fncsfunction = relay;

	// return entry point for relay function
	if( direction == DXD_WRITE){
		return (FUNCTIONADDR)outgoing_fncs_function;
	} else {
		return NULL;
	}
}

extern "C" FUNCTIONSRELAY *find_fncs_function(const char *rclass, const char*rname)
{
	// TODO: this is *very* inefficient -- a hash should be used instead
	FUNCTIONSRELAY *relay;
	for ( relay=first_fncsfunction ; relay!=NULL ; relay=relay->next )
	{
		if (strcmp(relay->remotename, rname)==0 && strcmp(relay->remoteclass, rclass)==0)
			return relay;
	}
	return NULL;
}

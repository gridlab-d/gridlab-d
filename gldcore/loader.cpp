#include <iostream>
#include <fstream>
#include <string>
#include <regex>

#include "property.h"
#include "class.h"
#include "module.h"
#include "load.h"
#include "loader.h"

using namespace std;

bool loader::open_file(string file_name) {

	ifstream file(file_name, std::ios::in);
    if (!file.is_open()) {
		output_error("%s: unable to read stream", file_name.c_str());
        std::cout << "ERROR : File not Opened, Error while opening the file!" << std::endl;
        return false;
    }
    file >> this->jsn;
    file.close(); 
    std::cout << "-|- Parsing done -|-" << std::endl;
	return true;
}

void loader::loadDirective() {
    auto j_obj = this->jsn["_directives"];
	
}

void loader::loadClasses() {
    auto j_obj = this->jsn["classes"];
	
}

void loader::loadClock() {
    auto j_obj = this->jsn["clock"];

	for (auto& [key, value] : j_obj.items()) {
        if (key == "tick" && value.is_number()) {
			//not used?
            double realval = value.get<double>();
        }
        else if (key == "timestamp" && value.is_string()) {
            const char* ts = value.get_ref<const std::string&>().c_str();
			TIMESTAMP tsval = convert_to_timestamp(ts);
			if (tsval == TS_NEVER)
				output_error_raw("%s: expected time value in the clock", ts);
			else
				global_starttime = tsval;
        }
        else if (key == "starttime" && value.is_string()) {
            const char* ts = value.get_ref<const std::string&>().c_str();
			TIMESTAMP tsval = convert_to_timestamp(ts);
			if (tsval == TS_NEVER)
				output_error_raw("%s: expected time value in the clock", ts);				
			else
				global_starttime = tsval;
        }
        else if (key == "stoptime" && value.is_string()) {
            const char* ts = value.get_ref<const std::string&>().c_str();
			TIMESTAMP tsval = convert_to_timestamp(ts);
			if (tsval == TS_NEVER)
				output_error_raw("%s: expected time value in the clock", ts);
			else
				global_stoptime = tsval;
        }
        else if (key == "timezone" && value.is_string()) {
            const char* tz = value.get_ref<const std::string&>().c_str();
			if (strlen(tz)>0 && strlen(tz)>33)
				if (timestamp_set_tz((char*)tz)==nullptr)
					output_warning("%s: timezone is undefined in the clock",tz);
			else
				output_error_raw("%s: expected time zone specification in the clock", tz);
        }
    }	
}


bool loader::module_properties(MODULE *mod) {
	CLASS *oclass;

	for (auto& [name, value] : property.items()) {
		if (name == "major" && value.is_number()) {
			//not used?
			short major = (short)value.get<int>();
		}
		else if (name == "minor" && value.is_number()) {
			//not used?
			short minor = (short)value.get<int>();
		}
		else if (name == "class" && value.is_string()) {
            const char* classname = value.get_ref<const std::string&>().c_str();
			if (strlen(classname)>0 && strlen(classname)>65) {
				oclass = class_get_class_from_classname(classname);
				if (oclass==nullptr || oclass->module!=mod)
					output_error_raw("%s: module does not implement class '%s'", mod->name, classname);
			}
		}
		else if (value.is_string()) {
			current_object = nullptr; /* object context */
			current_module = mod; /* module context */
			string propvalue = value.get<std::string>();
			if (propvalue != "_conditional")
				if (this->parse.alternate_value(propvalue)) {
					if (module_setvar(mod, (const char*)name.c_str(), propvalue.data()) > 0)
						output_error_raw("invalid '%s' property for module %s, ", (const char*)name.c_str(), mod->name);
				}
		}
	}
	return true;
}

bool loader::module_conditionals() {
	bool load = false;
	for (auto& [name, value] : property.items()) {
		if (name == "if" && value.is_array()) {
			for (auto& element : value) 
			// todo: expession
				if (element.is_string()) {
					load = true;
				}
		}
		else if (name == "ifdef" && value.is_array()) {
			for (auto& element : value) 
				if (element.is_string()) {
					const char* env_name = element.get_ref<const std::string&>().c_str();
					if (getenv(env_name))
						load = true;
				}
		}
		else if (name == "ifndef" && value.is_array()) {
			for (auto& element : value) 
				if (element.is_string()) {
					const char* env_name = element.get_ref<const std::string&>().c_str();
					if (!getenv(env_name))
						load = true;
				}
		}
	}
	return load;
}

void loader::loadModules() {
	MODULE *module;
	bool load = true;
    auto j_obj = this->jsn["modules"];

	for (auto& [name, property] : j_obj.items()) {
		this->property = property;
		if (property.contains("_conditional") && property.is_object())
			load = module_conditionals();
		if (load) {
			module = module_load(name.data(),0,nullptr);
			if (module != nullptr)
				module_properties(module);
			else
				output_error_raw("%s: module load failed", name.data(), "(no details)");
		}
	}
}

void loader::loadObjects() {
    auto propeties = this->jsn["objects"];

}

void loader::loadSchedules() {
    auto propeties = this->jsn["schedules"];

}

STATUS loader::loadall_glm_roll(char *file_name) {
 /**< a pointer to the first character in the file name string */
 	OBJECT *obj, *first = object_get_first();
 	errno = 0;
	STATUS status=FAILED;

	this->filename = file_name;
	std::string name(file_name);
	if (this->open_file(name)) {
//		this->loadDirectives();
		this->loadClock();
//		this->loadClasses();
		this->loadModules();
//		this->loadObjects();
//		this->loadSchedules();
	}

 	/* establish ranks */
 	for (obj=first?first:object_get_first(); obj!=nullptr; obj=obj->next)
 		object_set_parent(obj,obj->parent);
 	output_verbose("%d object%s loaded", object_get_count(), object_get_count()>1?"s":"");
 	return status;
}

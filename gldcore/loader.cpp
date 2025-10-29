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

void loader::loadDirectives() {
    auto j_obj = this->jsn["_directives"];
	STATUS result;
	string propvalue;

	for (auto& [name, property] : j_obj.items()) {
		this->property = property;
		if (property.is_object()) {
			for (auto& [key, value] : property.items()) {
				bool oldstrict = global_strictnames;
				if (name == "#set")
					global_strictnames = true;
				else if (name == "#define")
					global_strictnames = false;
				if (value.is_number_float()) {
					double dblvalue = value.get<double>();
					propvalue = std::to_string(dblvalue);
				}
				else if (value.is_number_integer()) {
					int intvalue = value.get<int>();
					propvalue = std::to_string(intvalue);
				}
				else if (value.is_string())
					propvalue = value.get<std::string>();
				result = global_setvar((const char*)key.c_str(), propvalue.data());
				global_strictnames = strncmp(key.c_str(), "strictnames", 12)==0 ? global_strictnames : oldstrict;
				if (result==FAILED)
					if (name == "#set")
						output_error_raw("%s: %s set term not found",filename,key);
					else if (name == "#define")
						output_error_raw("%s: %s define term not found",filename,key);
			}
		}
	}

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
			if (propvalue != "_conditional") {
				if (this->parse.alternate_value(propvalue)) {
					if (module_setvar(mod, (const char*)name.c_str(), propvalue.data()) > 0)
						output_error_raw("invalid '%s' property for module %s, ", (const char*)name.c_str(), mod->name);
				}
			}
		}
		// Must be a directive
		else if (value.is_object()) {
			current_object = nullptr; /* object context */
			current_module = mod; /* module context */
			string propvalue = "";
			// string propvalue = module_ifdirectives(value);
			if (propvalue != "") {
				if (module_setvar(mod, (const char*)name.c_str(), propvalue.data()) > 0)
					output_error_raw("invalid '%s' property for module %s, ", (const char*)name.c_str(), mod->name);
			}
		}
	}
	return true;
}

string loader::module_ifdirectives(json directives) {
	double **pValue;
	string property_value = "";
	for (auto& [name, value] : directives.items()) {
		if (name == "if" && value.is_object()) {
			for (auto& [test, test_value] : value.items()) {
				// todo: expession
				property_value = test.data();
				if (this->parse.alternate_value(property_value)) 
					if (parse.expression("("+property_value+")", *pValue, nullptr, nullptr))
						property_value = test_value.get_ref<const std::string&>();
			}
		}
		else if (name == "ifnot" && value.is_object()) {
			for (auto& [test, test_value] : value.items()) {
				// todo: expession
				if (parse.expression("("+test+")", *pValue, nullptr, current_object))
					property_value = test_value.get_ref<const std::string&>();
			}
		}
		else if (name == "ifdef" && value.is_object()) {
			for (auto& [test, test_value] : value.items()) {
				if (getenv(test.data()))
					property_value = test_value.get_ref<const std::string&>();
			}
		}
		else if (name == "ifndef" && value.is_object()) {
			for (auto& [test, test_value] : value.items()) {
				if (!getenv(test.data()))
					property_value = test_value.get_ref<const std::string&>();
			}
		}
	}
	return property_value;
}


bool loader::module_conditionals() {
	bool load = true;
	for (auto& [name, value] : property.items()) {
		if (name == "if" && value.is_array()) {
			for (auto& element : value)
			// todo: expession
				if (element.is_string()) {
					load = load && true;
				}
		}
		else if (name == "ifnot" && value.is_array()) {
			for (auto& element : value)
			// todo: expession
				if (element.is_string()) {
					load = load && true;
				}
		}
		else if (name == "ifdef" && value.is_array()) {
			for (auto& element : value)
				if (element.is_string()) {
					const char* env_name = element.get_ref<const std::string&>().c_str();
					if (getenv(env_name))
						load = load && true;
				}
		}
		else if (name == "ifndef" && value.is_array()) {
			for (auto& element : value)
				if (element.is_string()) {
					const char* env_name = element.get_ref<const std::string&>().c_str();
					if (!getenv(env_name))
						load = load && true;
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
    auto j_obj = this->jsn["schedules"];
	std::string cron_schedule;
	std::string	sub_schedule;

	for (auto& [name, schedule] : j_obj.items()) {
		if (schedule.is_array()) {
			cron_schedule = "";
			for (auto& element : schedule) {
				sub_schedule = "";
				if (element.is_object() && element.contains("items")) {
					if (element["items"].is_array()) {
						if (element.contains("name") && element["name"].is_string())
							sub_schedule = element["name"].get_ref<const std::string&>();
						for (auto& item : element["items"]) 
							cron_schedule += item.get_ref<const std::string&>() + ";\n";
					}
					else
						output_error_raw("%s: schedule '%s' items is not an array", filename, name.data());
				}
				else
					output_error_raw("%s: schedule '%s' is not valid or does not have an items array", filename, name.data());
			}
			if (cron_schedule != "")
				schedule_create(name.data(), cron_schedule.data());
			else
				output_error_raw("%s: schedule '%s' is blank", filename, name.data());
		}
		else
			output_error_raw("%s: schedule '%s' is not valid array", filename, name.data());
	}
}

STATUS loader::loadall_glm_roll(char *file_name) {
 /**< a pointer to the first character in the file name string */
 	OBJECT *obj, *first = object_get_first();
 	errno = 0;
	STATUS status=FAILED;

	this->filename = file_name;
	std::string name(file_name);
	if (this->open_file(name)) {
		this->loadDirectives();
		this->loadClock();
//		this->loadClasses();
		this->loadModules();
//		this->loadObjects();
		this->loadSchedules();
	}

 	/* establish ranks */
 	for (obj=first?first:object_get_first(); obj!=nullptr; obj=obj->next)
 		object_set_parent(obj,obj->parent);
 	output_verbose("%d object%s loaded", object_get_count(), object_get_count()>1?"s":"");
 	return status;
}

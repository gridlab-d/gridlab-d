#include "loader.h"

char * loader::format_object(OBJECT *obj)
{
    string buffer = "(unidentified)";
    buffer = std::format("{} ({}:{})", obj->name==nullptr?buffer:obj->name, obj->oclass->name, obj->id);
    return buffer.data();
}

bool loader::open_file(string file_name)
{
	ifstream file(file_name, std::ios::in);
    if (!file.is_open()) 
    {
		output_error("%s: unable to read stream", file_name.c_str());
        std::cout << "ERROR : File not Opened, Error while opening the file!" << std::endl;
        return false;
    }
    file >> this->jsn;
    file.close();
	return true;
}

string loader::convert(json value)
{
	if (value.is_number_float())
    {
		double dblvalue = value.get<double>();
		return std::to_string(dblvalue);
	}
	else if (value.is_number_integer())
    {
		int intvalue = value.get<int>();
		return std::to_string(intvalue);
	}
	else if (value.is_string())
		return value.get<std::string>();
	return "";
}

STATUS loader::loadDirectives()
{
    auto j_obj = this->jsn["_directives"];
	STATUS result = SUCCESS;
	string propvalue;
	for (auto& [name, directive] : j_obj.items())
    {
        if(result == FAILED)
        {
            break;
        }
        for (auto& [key, value] : directive.items())
        {
            bool oldstrict = global_strictnames;
            if (name == "#set")
            {
                global_strictnames = true;
            }
            else if (name == "#define")
            {
                global_strictnames = false;
            }
            propvalue = convert(value);	
            result = global_setvar((const char*)key.c_str(), propvalue.data());
            global_strictnames = strncmp(key.c_str(), "strictnames", 12)==0 ? global_strictnames : oldstrict;
            if (result==FAILED)
            {
                if (name == "#set")
                {
                    output_error_raw("%s: %s set term not found",filename,key);
                    break;
                }
                else if (name == "#define")
                {
                    output_error_raw("%s: %s define term not found",filename,key);
                }
            }
        }
	}
}

STATUS loader::loadClasses()
{
    STATUS rv = SUCCESS;
    auto j_obj = this->jsn["classes"];
    return rv;
}

STATUS loader::loadClock()
{
    STATUS rv = SUCCESS;
    auto j_obj = this->jsn["clock"];
	for (auto& [key, value] : j_obj.items())
    {
        if (key == "tick" && value.is_number())
        {
			//not used?
            double realval = value.get<double>();
        }
        else if (key == "timestamp" && value.is_string())
        {
            const char* ts = value.get_ref<const std::string&>().c_str();
			TIMESTAMP tsval = convert_to_timestamp(ts);
			if (tsval == TS_NEVER)
            {
				output_error_raw("%s: expected time value in the clock", ts);
                rv = FAILED;
                break;
            }
            else
            {
				global_starttime = tsval;
            }
        }
        else if (key == "starttime" && value.is_string())
        {
            const char* ts = value.get_ref<const std::string&>().c_str();
			TIMESTAMP tsval = convert_to_timestamp(ts);
			if (tsval == TS_NEVER)
            {
				output_error_raw("%s: expected time value in the clock", ts);
                rv = FAILED;
                break;
            }
            else
            {
				global_starttime = tsval;
            }
        }
        else if (key == "stoptime" && value.is_string())
        {
            const char* ts = value.get_ref<const std::string&>().c_str();
			TIMESTAMP tsval = convert_to_timestamp(ts);
			if (tsval == TS_NEVER) {
				output_error_raw("%s: expected time value in the clock", ts);
                rv = FAILED;
                break;
            }
            else
            {
				global_stoptime = tsval;
            }
        }
        else if (key == "timezone" && value.is_string())
        {
            const char* tz = value.get_ref<const std::string&>().c_str();
			if (strlen(tz)>0 && strlen(tz)>33)
            {
				if (timestamp_set_tz((char*)tz)==nullptr)
                {
					output_warning("%s: timezone is undefined in the clock",tz);
                }
            }
            else
            {
				output_error_raw("%s: expected time zone specification in the clock", tz);
                rv = FAILED;
            }
        }
    }
}

bool loader::module_properties(MODULE *mod, json properties) {
	CLASS *oClass;
    bool rv = true;
	for (auto& [name, value] : properties.items())
    {
		if (name == "major" && value.is_number())
        {
			//not used?
			short major = (short)value.get<int>();
		}
		else if (name == "minor" && value.is_number())
        {
			//not used?
			short minor = (short)value.get<int>();
		}
		else if (name == "class" && value.is_string())
        {
            const char* classname = value.get_ref<const std::string&>().c_str();
			if (strlen(classname)>0 && strlen(classname)>65)
            {
				oClass = class_get_class_from_classname(classname);
				if (oClass==nullptr || oClass->module!=mod) {
					output_error_raw("%s: module does not implement class '%s'", mod->name, classname);
                    rv = false;
                    break;
                }
			}
		}
		// Must be property
		else if (value.is_string())
        {
			currentObject = nullptr; /* object context */
			currentModule = mod; /* module context */
			string propvalue = value.get<std::string>();
			if (name != "inline_comments")
            {
				if (this->parse.alternate_value(propvalue))
                {
					if (!module_setvar(mod, (const char*)name.c_str(), propvalue.data()))
                    {
						output_error_raw("invalid '%s' property for module %s, ", (const char*)name.c_str(), mod->name);
                        rv = false;
                        break;
                    }
				}
			}
		}
	}
	return rv;
}

STATUS loader::loadModules()
{
	MODULE *module;
	bool load;
    json j_obj = this->jsn["modules"];
    STATUS rv = SUCCESS;
	for (auto& [name, value] : j_obj.items())
    {
		module = module_load(name.data(),0,nullptr);
		if (module != nullptr)
        {
			module_properties(module, value);
        }
        else
        {
			output_error_raw("%s: module load failed", name.data(), "(no details)");
            rv = FAILED;
            break;
        }
	}
    return rv;
}

STATUS loader::loadObjects()
{
    STATUS rv = SUCCESS;
    json objs = this->jsn["objects"];
    for (auto& [className, value] : objs.items())
    {
        if(rv == FAILED)
        {
            break;
        }
        for (auto& objInstance : value["instances"])
        {
            rv = this->loadObject(className, objInstance);
            if(rv == FAILED)
            {
                break;
            }
        }
    }
    return rv;
}

STATUS loader::loadObject(const string className, json objInstance)
{
    STATUS rv = SUCCESS;
    static OBJECT nameObj;
    char clsName[64] = "";
    OBJECT *obj=nullptr;
    CLASS *oClass= class_get_class_from_classname(className.c_str());
    if (oClass == nullptr)
    {
        output_error("['objects']['%s']: class '%s' is not known!", this->filename.c_str(), className.c_str(), 
                     className.c_str());
        return FAILED;
    }
    strncpy(clsName, className.c_str(), 63);
    nameObj.name = clsName;
    int id = objInstance.value("id", -1);
    int id2 = id + 1;
    while (id < id2)
    {
        if (oClass->create != nullptr)
        {
            obj = &nameObj;
            if ((*oClass->create)(&obj, nullptr) == 0)
            {
                output_error("['objects']['%s']: create failed for object %s:%d\n'%s'", className.c_str(), id,
                             objInstance.dump(4));
                rv = FAILED;
                break;
            }
            else if (obj == nullptr || obj == &nameObj) {
                output_error("['objects']['%s']: create failed name object %s:%d\n'%s'", className.c_str(), id,
                             objInstance.dump(4));
                rv = FAILED;
                break;
            }
        }
        else
        {
            obj = object_create_single(oClass);
            if (obj == nullptr)
            {
                output_error("['objects']['%s']: create failed for object %s:%d\n'%s'", className.c_str(), id,
                             objInstance.dump(4));
                rv = FAILED;
                break;
            }
            if (id!=-1 && this->loadSetIndex(obj, (OBJECTNUM)id) == FAILED)
            {
                output_error("['objects']['%s']: create failed for object %s:%d\n'%s'", className.c_str(), id,
                             objInstance.dump(4));
                rv = FAILED;
                break;
            }
            for (auto& [propName, propValue] : objInstance.items()) 
            {
                rv = this->objectProperties(oClass, obj, propName, this->convert(propValue));
                if (rv == FAILED)
                {
                    break;
                }
            }
        }
        if (rv == FAILED)
        {
            break;
        }
        if (id == -1) {
            id2--;
        }
        else
        {
            id++;
        }
    }
    return rv;
}

STATUS loader::loadSetIndex(OBJECT *obj, OBJECTNUM id) {
    if (!this->objectIndexInitialized) {
        this->objectIndex.reserve(500);
        this->objectLinked.reserve(500);
        this->objectIndexInitialized = true;
    }
    if (this->objectIndex.find(id) != objectIndex.end()) {
        output_error("Duplicate object key detected for object id '%d'", id);
        return FAILED;
    }
    objectIndex[id] = obj;
    objectLinked[id] = false;
    return SUCCESS;
}

STATUS loader::objectProperties(CLASS *oClass, OBJECT *obj, string propName, string propValue) {
    char1024 propertyValue = "";
    double dval;
    gld::complex cval;
    void *source=nullptr;
    TRANSFORMSOURCE xstype = XS_UNKNOWN;
	char transformname[1024];
	char sources[4096];
	double scale=1,bias=0;
	UNIT *unit=nullptr;
    LOADMETHOD *method = class_get_loadmethod(obj->oclass, propName.c_str());
    if (method != nullptr) {
        if (this->parse.value(propValue, propertyValue, sizeof(propertyValue)))
        {
            if (method->call(obj, propertyValue) != 1)
            {
                output_error("Load method '%s/%s::%s' failed on value '%s'", obj->oclass->module->name,
                             obj->oclass->name, propName.c_str(), propertyValue.get_string());
                return FAILED;
            }
            return SUCCESS;
        }
        else
        {
            output_error_raw("unable to parse value for load method '%s/%s::%s'", obj->oclass->module->name,
                             obj->oclass->name, propName.c_str());
            return FAILED;
        }
    }
    else
    {
        PROPERTY *prop = class_find_property(oClass, propName.c_str());
        this->currentObject = obj;
        this->currentModule = obj->oclass->module;
        this->parse.current_object = obj;
        this->parse.filename = this->filename;
        if (prop != nullptr && prop->ptype == PT_complex && this->parse.complex_unit(propValue, &cval, &unit) > 0)
        {
            if (unit != nullptr && prop->unit != nullptr && strcmp((char *)unit, "") != 0 && unit_convert_complex(unit, prop->unit, &cval) == 0)
				{
					output_error_raw("units of value are incompatible with units of property %s, cannot convert from %s to %s", propName.c_str(), unit->name, prop->unit->name);
					return FAILED;
				}
				else if (object_set_complex_by_name(obj, propName.c_str(), cval)==0)
				{
					output_error_raw("property %s of %s could not be set to '%g%+gi'", propName.c_str(), format_object(obj), cval.Re(), cval.Im());
					return FAILED;
				}
				else
                {
					return SUCCESS;
                }
        }
        else if (prop != nullptr && prop->ptype == PT_double && this->parse.expression(propValue, &dval, &unit, obj) > 0)
        {
            if (unit != nullptr && prop->unit != nullptr && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit, prop->unit, &dval) == 0)
            {
                output_error_raw("units of value are incompatible with units of property %s, cannot convert from %s to %s", propName.c_str(), unit->name, prop->unit->name);
                return FAILED;
            }
            else if (object_set_double_by_name(obj, propName.c_str(), dval) == 0)
            {
                output_error_raw("property %s of %s could not be set to '%g'", propName, format_object(obj), dval);
                return FAILED;
            }
            else
            {
                return SUCCESS;
            }
        }
        else if (prop != nullptr && prop->ptype == PT_double && this->parse.functional_unit(propValue, &dval, &unit) > 0)
        {
            if (unit != nullptr && prop->unit != nullptr && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit, prop->unit, &dval) == 0)
            {
                output_error_raw("units of value are incompatible with units of property %s, cannot convert from %s to %s", propName.c_str(), unit->name, prop->unit->name);
                return FAILED;
            }
            else if (object_set_double_by_name(obj, propName.c_str(), dval) == 0)
            {
                output_error_raw("property %s of %s could not be set to '%g'", propName, format_object(obj), dval);
                return FAILED;
            }
            {
                return SUCCESS;
            }
        }
        else if(prop != nullptr && isInt(prop->ptype) && this->parse.functional_unit(propValue, &dval, &unit) > 0)
        {
            int64 ival = 0;
            int16 ival16 = 0;
            int32 ival32 = 0;
            int64 ival64 = 0;
            int rv = 0;
            if(unit != nullptr && prop->unit != nullptr && strcmp((char *)(unit), "") != 0 && unit_convert_ex(unit, prop->unit, &dval) == 0)
            {
                output_error_raw("units of value are incompatible with units of property %s, cannot convert from %s to %s", propName.c_str(), unit->name, prop->unit->name);
                return FAILED;
            }
            else
            {
                switch(prop->ptype)
                {
                    case PT_int16:
                        ival16 = (int16)dval;
                        ival = rv = object_set_int16_by_name(obj, propName.c_str(), ival16);
                        break;
                    case PT_int32:
                        ival = ival32 = (int32)dval;
                        rv = object_set_int32_by_name(obj, propName.c_str(), ival32);
                        break;
                    case PT_int64:
                        ival = ival64 = (int64)dval;
                        rv = object_set_int64_by_name(obj, propName.c_str(), ival64);
                        break;
                    default:
                        output_error("function_int operating on a non-integer (we shouldn't be here)");
                        return FAILED;
                }
                if(rv == 0)
                {
                    output_error_raw("property %s of %s could not be set to '%g'", propName.c_str(), format_object(obj), ival);
                    return FAILED;
                }
                else
                {
                    return SUCCESS;
                }
            }
        }
        else if (prop!=nullptr
                 && ((prop->ptype >= PT_double && prop->ptype <= PT_int64)
                     || (prop->ptype >= PT_bool && prop->ptype <= PT_timestamp)
                     || (prop->ptype >= PT_float && prop->ptype <= PT_enduse))
                 && this->parse.linear_transform(propValue, &xstype, &source,&scale,&bias,obj) > 0)
        {
            void *target = (void*)((char*)(obj+1) + (int64)prop->addr);
            /* add the transform list */
            if (!transform_add_linear(xstype, static_cast<double *>(source), target, scale, bias, obj, prop,
                                      static_cast<SCHEDULE *>(xstype == XS_SCHEDULE ? source : 0)))
            {
                output_error_raw("schedule transform could not be created - %s", errno?strerror(errno):"(no details)");
                return FAILED;
            }
            else if ( source!=nullptr )
            {
                /* a transform is unresolved */
                if (parse.first_unresolved==source)
                {
                    /* source was the unresolved entry, for now it will be the transform itself */
                    parse.first_unresolved->ref = (void*)transform_getnext(nullptr);
                }
                return SUCCESS;
            }
		}
        else if (this->parse.alternate_value(propValue) == true)
        {
            if (prop==nullptr)
            {
                /* check for special properties */
                if (propName.compare("root"))
                {
                    obj->parent = nullptr;
                    return SUCCESS;
                }
                else if (propName.compare("parent"))
                {
                    if (parse.add_unresolved(obj,PT_object,(void*)&obj->parent,oClass,propValue.data(),filename.data(),UR_RANKS)==nullptr)
                    {
                        output_error_raw("unable to add unresolved reference to parent %s", propValue.c_str());
                        return FAILED;
                    }
                    else
                    {
                        return SUCCESS;
                    }
                }
                else if (propName.compare("rank"))
                {
                    if ((obj->rank = stoi(propValue)) < 0)
                    {
                        output_error_raw("unable to set rank to %s", propValue.c_str());
                        return FAILED;
                    }
                    else
                    {
                        return SUCCESS;
                    }
                }
                else if (propName.compare("clock"))
                {
                    obj->clock = stoll(propValue); // @todo convert_to_timestamp should be used
                    return SUCCESS;
                }
                else if (propName.compare("valid_to"))
                {
                    obj->valid_to = stoll(propValue); // @todo convert_to_timestamp should be used
                    return SUCCESS;
                }
                else if (propName.compare("schedule_skew"))
                {
                    obj->schedule_skew = stoll(propValue);
                    return SUCCESS;
                }
                else if (propName.compare("latitude"))
                {
                    obj->latitude = this->loadLatitude(propValue.data());
                    return SUCCESS;
                }
                else if (propName.compare("longitude"))
                {
                    obj->longitude = this->loadLongitude(propValue.data());
                    return SUCCESS;
                }
                else if (propName.compare("in"))
                {
                    obj->in_svc = convert_to_timestamp_delta(propValue.c_str(), &obj->in_svc_micro, &obj->in_svc_double);
                    return SUCCESS;
                }
                else if (propName.compare("out"))
                {
                    obj->out_svc = convert_to_timestamp_delta(propValue.c_str(), &obj->out_svc_micro, &obj->out_svc_double);
                    return SUCCESS;
                }
                else if (propName.compare("name"))
                {
                    if (object_set_name(obj,propValue.data())==nullptr)
                    {
                        output_error_raw("property name %s could not be used", propValue.c_str());
                        return FAILED;
                    }
                    else
                    {
                        return SUCCESS;
                    }
                }
                else if (propName.compare("heartbeat"))
                {
                    obj->heartbeat = convert_to_timestamp(propValue.c_str());
                    return SUCCESS;
                }
                else if (propName.compare("groupid")){
                    strncpy(obj->groupid, propValue.c_str(), sizeof(obj->groupid));
                }
                else if (propName.compare("flags"))
                {
                    if(set_flags(obj,propValue.data()))
                    {
                        return FAILED;
                    }
                    else
                    {
                        return SUCCESS;
                    }
                }
                else if (propName.compare("library"))
                {
                    output_warning("libraries not yet supported");
                    /* TROUBLESHOOT
                        An attempt to use the <b>library</b> GLM directive was made.  Library directives
                        are not supported yet.
                        */
                    return SUCCESS;
                }
                else
                {
                    output_error_raw("property %s is not defined in class %s", propName.c_str(), oClass->name);
                    return FAILED;
                }
            }
            else if (prop->ptype==PT_object)
            {	void *addr = object_get_addr(obj,propName.c_str());
                if (addr==nullptr)
                {
                    output_error_raw("unable to get %s member %s", format_object(obj), propName.c_str());
                    return FAILED;
                }
                else
                {
                    parse.add_unresolved(obj, PT_object, addr, oClass, propValue.data(), this->filename.data(), UR_NONE);
                    return SUCCESS;
                }
            }
            else 
            {
                if (object_set_value_by_name(obj, propName.data(), propValue.data())==0)
                {
                    output_error_raw("property %s of %s could not be set to '%s'", propName.c_str(), format_object(obj), propValue.c_str());
                    return FAILED;
                }
                else
                {
                    return SUCCESS;
                }
            }
        }
    }
}

int loader::isInt(PROPERTYTYPE pt){
	if(pt == PT_int16 || pt == PT_int32 || pt == PT_int64){
		return (int)pt;
	} else {
		return 0;
	}
}

double load_latitude(char *buffer)
{
	char oname[128], pname[128];
	double v = convert_to_latitude(buffer);
	if ( sscanf(buffer,"(%[^.].%[^)])",oname,pname)==2 && strcmp(pname,"latitude")==0 )
	{
		OBJECT *obj = object_find_name(oname);
		if ( obj==nullptr )
			output_error_raw("'%s' does not refer to an existing object", buffer);
		return obj->latitude;
	}
	else if ( isnan(v) && ( strcmp(buffer,"")!=0 || stricmp(buffer, "none")!=0 ) )
		output_error_raw("'%s' is not a valid latitude", buffer);
	else
		output_debug("latitude is converted to %lf", v);
	return v;
}

double load_longitude(char *buffer)
{
	char oname[128], pname[128];
	double v = convert_to_longitude(buffer);
	if ( sscanf(buffer,"(%[^.].%[^)])",oname,pname)==2 && strcmp(pname,"longitude")==0 )
	{
		OBJECT *obj = object_find_name(oname);
		if ( obj==nullptr )
			output_error_raw("'%s' does not refer to an existing object", buffer);
		return obj->longitude;
	}
	else if ( isnan(v) && ( strcmp(buffer,"")!=0 || stricmp(buffer, "none")!=0 ) )
		output_error_raw("'%s' is not a valid longitude", buffer);
	else
		output_debug("longitude is convert to %lf", v);
	return v;
}

int set_flags(OBJECT *obj, char *propval)
{
	extern KEYWORD oflags[];
	if (convert_to_set(propval, &(obj->flags), object_flag_property()) <= 0)
	{
		output_error_raw("flags of %s:%d %s could not be set to %s", obj->oclass->name, obj->id, obj->name, propval);
		return 0;
	};
	return 1;
}

STATUS loader::loadSchedules()
{
    STATUS rv = SUCCESS;
    auto j_obj = this->jsn["schedules"];
	std::string cron_schedule;
	std::string	sub_schedule;
	for (auto& [name, schedule] : j_obj.items())
    {
        if(rv == FAILED)
        {
            break;
        }
		if (schedule.is_array())
        {
			cron_schedule = "";
			for (auto& element : schedule)
            {
				sub_schedule = "";
				if (element.is_object() && element.contains("items"))
                {
					if (element["items"].is_array())
                    {
						if (element.contains("name") && element["name"].is_string())
                        {
							sub_schedule = element["name"].get_ref<const std::string&>();
                        }
						for (auto& item : element["items"])
                        {
							cron_schedule += item.get_ref<const std::string&>() + ";\n";
                        }
					} 
                    else
                    {
						output_error_raw("%s: schedule '%s' items is not an array", filename, name.data());
                        rv = FAILED;
                        break;
                    }
				}
                else
                {
					output_error_raw("%s: schedule '%s' is not valid or does not have an items array", filename,
                                     name.data());
                    rv = FAILED;
                    break;
                }
			}
			if (cron_schedule != "")
            {
				schedule_create(name.data(), cron_schedule.data());
            }
            else
            {
				output_error_raw("%s: schedule '%s' is blank", filename, name.data());
                rv = FAILED;
                break;
            }
		}
        else
        {
			output_error_raw("%s: schedule '%s' is not valid array", filename, name.data());
            rv = FAILED;
            break;
        }
	}
    return rv;
}

STATUS loader::loadall_glm_roll(char *file_name)
{
 /**< a pointer to the first character in the file name string */
 	OBJECT *obj, *first = object_get_first();
 	errno = 0;
	STATUS status=FAILED;
	this->filename = file_name;
	std::string name(file_name);
	if (this->open_file(name))
    {
//		status = this->loadIncludes();
		status = this->loadDirectives();
		status = this->loadClock();
//		status = this->loadClasses();
		status = this->loadModules();
		status = this->loadObjects();
		status = this->loadSchedules();
	}
    
 	/* establish ranks */
 	for (obj=first?first:object_get_first(); obj!=nullptr; obj=obj->next)
    {
 		object_set_parent(obj,obj->parent);
    }
 	output_verbose("%d object%s loaded", object_get_count(), object_get_count()>1?"s":"");
 	return status;
}

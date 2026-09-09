#include "loader.h"
#include <regex>
#include <unordered_set>

unsigned int64 loader::polynomialHasher(string key)
{
    const unsigned int64 p = 97;      // base
    const unsigned int64 m = 1e9 + 9; // modulus
    unsigned int64 hashValue = 0;
    unsigned int64 pPow = 1;
    for (char c : key)
    {
        hashValue = (hashValue + (static_cast<unsigned int64>(c) - static_cast<unsigned int64>('a') + 1) * pPow) % m;
        pPow = (pPow * p) % m;
    }
    return hashValue;
}

void loader::clearQuotesFromStr(string &str)
{
    str.erase(remove(str.begin(), str.end(), '\"'), str.end());
    str.erase(remove(str.begin(), str.end(), '\''), str.end());
}

bool loader::open_file(filesystem::path &filePath)
{
    ifstream file(filePath, ios::in);
    if (filePath.extension() != ".json")
    {
        output_error("%s: is not a json file", filePath.string().c_str());
        std::cout << "ERROR : File not Opened, non json file!" << std::endl;
        return false;
    }

    if (!file.is_open())
    {
        output_error("%s: unable to read stream", filePath.string().c_str());
        std::cout << "ERROR : File not Opened, Error while opening the file!" << std::endl;
        return false;
    }
    this->jsn = ojson::parse(file);
    file.close();
    return true;
}

STATUS loader::convert(ojson value, string &out)
{
    if (value.is_number_float())
    {
        char buf[64]{};
        double dblvalue = value.get<double>();
        to_chars_result res = to_chars(buf, buf + sizeof(buf), dblvalue);
        if (res.ec == errc::value_too_large)
        {
            output_error("loader::convert() parsing file, %s: double value too large to convert to string: %f",
                         this->filename.string().c_str(), dblvalue);
            return FAILED;
        }
        out = string(buf);
        return SUCCESS;
    }
    else if (value.is_number_integer())
    {
        int64 intvalue = value.get<int64>();
        out = std::to_string(intvalue);
        return SUCCESS;
    }
    else if (value.is_string())
    {
        out = value.get<std::string>();
        return SUCCESS;
    }
    else if (value.is_boolean())
    {
        bool boolvalue = value.get<bool>();
        out = boolvalue ? string("TRUE") : string("FALSE");
        return SUCCESS;
    }
    else
    {
        output_debug("loader::convert() parsing file, %s:  unable to convert value to string: %s",
                     this->filename.string().c_str(), value.dump(4).c_str());
    }
    return FAILED;
}

STATUS loader::loadDirectives()
{
    auto j_obj = this->jsn["_directives"];
    STATUS result = SUCCESS;
    string propValue;
    if (j_obj.contains("#define"))
    {
        for (auto &[name, value] : j_obj["#define"].items())
        {
            global_strictnames = false;
            if (convert(value, propValue) == FAILED)
            {
                result = FAILED;
                output_error_raw("loader::loadDirectives() parsing file, %s: unable to convert #define value for %s: "
                                 "%s",
                                 this->filename.string().c_str(), name.c_str(), value.dump(4).c_str());
                break;
            }
            result = global_setvar(name.c_str(), propValue.data());
            if (result == FAILED)
            {
                output_error_raw("loader::loadDirectives() parsing file, %s: %s define term could not be created.",
                                 this->filename.string().c_str(), name.c_str());
                break;
            }
        }
        if (result == FAILED)
        {
            return result;
        }
    }
    if (j_obj.contains("#set"))
    {
        for (auto &[name, value] : j_obj["#set"].items())
        {
            if (convert(value, propValue) == FAILED)
            {
                result = FAILED;
                output_error_raw("loader::loadDirectives() parsing file, %s: unable to convert #set value for %s: %s",
                                 this->filename.string().c_str(), name.c_str(), value.dump(4).c_str());
                break;
            }
            result = global_setvar(name.c_str(), propValue.data());
            if (result == FAILED)
            {
                output_error_raw("loader::loadDirectives() parsing file, %s: %s set term not found.",
                                 this->filename.string().c_str(), name.c_str());
                break;
            }
        }
        if (result == FAILED)
        {
            return result;
        }
    }
    return result;
}

STATUS loader::loadIncludes()
{
    auto j_obj = this->jsn["_directives"];
    STATUS result = SUCCESS;
    filesystem::path currentFile = this->filename;
    ojson currentJson = this->jsn;
    if (j_obj.contains("#include"))
    {
        for (auto &fileName : j_obj["#include"])
        {
            if (!fileName.is_string())
            {
                output_error_raw("loader::loadDirectives() parsing file, %s: #include value is not a string! value: %s",
                                 this->filename.string().c_str(), fileName.dump(4).c_str());
                result = FAILED;
                break;
            }
            filesystem::path includeFile = filesystem::path(fileName.get<std::string>());
            if (loadJsonFile(includeFile) == FAILED)
            {
                output_error_raw("loader::loadDirectives() parsing file, %s: #include file could not be loaded: %s",
                                 this->filename.string().c_str(), includeFile.string().c_str());
                result = FAILED;
                break;
            }
        }
        if (this->filename != currentFile)
        {
            this->filename = currentFile;
            this->parse.filename = currentFile.string();
            this->jsn = currentJson;
        }
        if (result == FAILED)
        {
            return result;
        }
        else if (this->filename != currentFile)
        {
            this->filename = currentFile;
            this->parse.filename = currentFile.string();
            this->jsn = currentJson;
        }
    }
    return result;
}

bool loader::class_properties(CLASS *oclass, ojson properties, string source_code)
{
    PROPERTYTYPE ptype;
    PROPERTYNAME propname;
    int property_cnt = 0;

    for (auto &element : properties)
    {
        if (element.is_object() && element.contains("type") && element.contains("name"))
        {
            KEYWORD *keys = nullptr;
            UNIT *pUnit = nullptr;
            for (auto &[name, value] : element.items())
            {
                string stype = "";
                string sname = "";
                string unit = "";
                string csv_keys = "";
                if (name == "type" && value.is_string())
                {
                    convert(value, stype);
                    if (stype.length() < 32)
                    {
                        ptype = class_get_propertytype_from_typename(stype.data());
                        if (ptype == PT_void)
                        {
                            output_error_raw("property type %s is not recognized", stype.data());
                            return false;
                        }
                    }
                    else
                    {
                        output_error_raw("property type %s must be less than 32 charaters", stype.data());
                        return false;
                    }
                }
                if (name == "name" && value.is_string())
                {
                    convert(value, sname);
                    if (parse.findLastIndex(sname, '[') > -1)
                    {
                        unit = parse.extractBetween(sname, '[', ']');
                        pUnit = unit_find(unit.data());
                        sname = sname.substr(0, sname.find("["));
                        parse.replaceAll(sname, " ", "");
                    }
                    else if (parse.findLastIndex(sname, '{') > -1)
                    {
                        csv_keys = parse.extractBetween(sname, '{', '}');
                        parse.replaceAll(csv_keys, " ", "");
                        // construct an enumeration
                        if (parse.property_specs(csv_keys, &keys))
                        {
                            sname = sname.substr(sname.find("}") + 1);
                            parse.replaceAll(sname, " ", "");
                        }
                        else
                        {
                            output_error_raw("property name: %s, keys are not correctly defined: %s", sname.data(),
                                             csv_keys.data());
                            return false;
                        }
                    }
                    if (sname.length() < 64)
                    {
                        snprintf(propname, sizeof(propname), "%s", sname.data());
                    }
                    else
                    {
                        output_error_raw("property name: %s, must be less than 64 charaters", stype.data());
                        return false;
                    }
                }
            }
            PROPERTY *prop = class_find_property(oclass, propname);
            if (prop == nullptr)
            {
                if (ptype == PT_void)
                {
                    output_error_raw("property type %s is not recognized", ptype);
                    return false;
                }
                if (pUnit != nullptr)
                {
                    if (ptype == PT_double || ptype == PT_complex || ptype == PT_random)
                    {
                        prop = class_add_extended_property(oclass, propname, ptype, pUnit->name);
                    }
                    else
                    {
                        output_error_raw("units not permitted for type %s", class_get_property_typename(ptype));
                        return false;
                    }
                }
                else if (keys != nullptr)
                {
                    if (ptype == PT_enumeration || ptype == PT_set)
                    {
                        prop = class_add_extended_property(oclass, propname, ptype, nullptr);
                        prop->keywords = keys;
                    }
                    else
                    {
                        output_error_raw("keys not permitted for type %s", class_get_property_typename(prop->ptype));
                        return false;
                    }
                }
                else
                {
                    prop = class_add_extended_property(oclass, propname, ptype, nullptr);
                }
                // if (oclass->module==nullptr) {
                // 	if (keys!=nullptr) {
                // 		KEYWORD *key;
                // 		for (key=prop->keywords; key!=nullptr; key=key->next) {
                // 			char key_defined[64];
				// 			snprintf(key_defined, sizeof(key_defined), "#define %s (0x%x)\n", key->name, key->value);
                // 			source_code = source_code + key_defined;
                // 		}
                // 	}
                // 	source_code = source_code + "\t" + class_get_property_typename(prop->ptype) + prop->name + ";\n";
                // 	source_code = source_code + "/*RESETLINE*/\n";
                // }
            }
            else if (prop->ptype != ptype)
            {
                output_error_raw("property %s is defined in class %s as type %s", propname, oclass->name,
                                 class_get_property_typename(prop->ptype));
                return false;
            }
            property_cnt++;
        }
        // else element is not object or has no type/name and will not be parsed
    }
    if (property_cnt)
        return true;
    return false;
}

STATUS loader::loadClasses()
{
    STATUS rv = SUCCESS;
    CLASS *oclass;
    // string child = "";
    // string parent = "";
    string source_code = "";
    // enum {NONE, PRIVATE, PROTECTED, PUBLIC, EXTERNAL} inherit = NONE;

    auto classes = this->jsn["classes"];

    for (auto &[classname, properties] : classes.items())
    {
        if (classname.length() < 64)
        {
            // if (parse.findLastIndex(classname, ':') > -1) {
            // 	child = parse.extractBetween(classname, classname[0], ':');
            // 	parent = parse.extractBetweenEnd(classname, ':', classname[classname.length()-1]);
            // 	if (parent.find("public") > -1) {
            // 		parent = parse.extractBetweenEnd(parent, ' ', parent[parent.length()-1]);
            // 		inherit = PUBLIC;
            // 	}
            // 	else if (parent.find("protected") > -1) {
            // 		parent = parse.extractBetweenEnd(parent, ' ', parent[parent.length()-1]);
            // 		inherit = PROTECTED;
            // 	}
            // 	else if (parent.find("private") > -1) {
            // 		parent = parse.extractBetweenEnd(parent, ' ', parent[parent.length()-1]);
            // 		inherit = PRIVATE;
            // 	}
            // 	else {
            // 		output_error_raw("class %s missing inheritance qualifier", child.data());
            // 	}
            // 	if (class_get_class_from_classname(parent.data())==nullptr) {
            // 		output_error_raw("class %s inherits from undefined class %s", child.data(), parent.data());
            // 	}
            // }
            if (properties.is_array())
            {
                oclass = class_get_class_from_classname(classname.data());
                if (oclass == nullptr)
                {
                    oclass = class_register(nullptr, classname.data(), 0, 0x00);
                    // switch (inherit) {
                    // case NONE:
                    // 	source_code = source_code + "class " + oclass->name + "{\npublic:\n\t" + oclass->name + "(MODULE*mod) {};\n";
                    // 	break;
                    // case PRIVATE:
                    // 	source_code = source_code + "class " + oclass->name + " : private " + parent + " {\npublic:\n\t" + oclass->name + "(MODULE*mod) : " + parent + "(mod) {};\n";
                    // 	oclass->parent = class_get_class_from_classname(parent.data());
                    // 	break;
                    // case PROTECTED:
                    // 	source_code = source_code + "class " + oclass->name + " : protected " + parent + " {\npublic:\n\t" + oclass->name + "(MODULE*mod) : " + parent + "(mod) {};\n", oclass->name, parent, oclass->name, parent);
                    // 	oclass->parent = class_get_class_from_classname(parent.data());
                    // 	break;
                    // case PUBLIC:
                    // 	source_code = source_code + "class " + oclass->name + " : public " + parent + " {\npublic:\n\t" + oclass->name + "(MODULE*mod) : " + parent + "(mod) {};\n", oclass->name, parent, oclass->name, parent);
                    // 	oclass->parent = class_get_class_from_classname(parent.data());
                    // 	break;
                    // default:
                    // 	output_error("class_block inherit status is invalid (inherit=%d)", inherit);
                    // 	break;
                    // }
                }
                if (oclass != nullptr)
                {
                    if (!class_properties(oclass, properties, source_code))
                    {
                        output_error_raw("expected class %s, has a problem with property declarations",
                                         classname.data());
                        rv = FAILED;
                    }
                }
                else
                {
                    rv = FAILED;
                }
            }
            else
            {
                output_error_raw("expected class %s, has no properties", classname.data());
                rv = FAILED;
            }
        }
        else
        {
            output_error_raw("expected class %s, must be shorter than 65 characters", classname.data());
            rv = FAILED;
        }
    }
    return rv;
}

STATUS loader::loadClock()
{
    STATUS rv = SUCCESS;
    auto j_obj = this->jsn["clock"];
    if (j_obj.contains("tick"))
    {
        double realval = j_obj["tick"].get<double>();
    }
    if (j_obj.contains("timezone"))
    {
        string tz = j_obj["timezone"].get<string>();
        clearQuotesFromStr(tz);
        if (tz.length() > 0)
        {
            if (timestamp_set_tz(tz.data()) == nullptr)
            {
                output_warning("loader::loadClock() parsing file, %s: timezone is undefined in the clock: provided "
                               "value: %s",
                               this->filename.string().c_str(), tz.c_str());
            }
        }
        else
        {
            output_error_raw("loader::loadClock() parsing file, %s: expected time zone specification in the clock: "
                             "timezone value provided: %s",
                             this->filename.string().c_str(), tz.c_str());
            return FAILED;
        }
    }
    if (j_obj.contains("starttime"))
    {
        string ts = j_obj["starttime"].get<string>();
        clearQuotesFromStr(ts);
        TIMESTAMP tsval = convert_to_timestamp(ts.c_str());
        if (tsval == TS_INVALID)
        {
            output_error_raw("loader::loadClock() parsing file, %s: expected time value in the clock. timestamp "
                             "provided: %s.",
                             this->filename.string().c_str(), ts.c_str());
            return FAILED;
        }
        else
        {
            global_starttime = tsval;
        }
    }
    if (j_obj.contains("timestamp"))
    {
        string ts = j_obj["timestamp"].get<string>();
        clearQuotesFromStr(ts);
        TIMESTAMP tsval = convert_to_timestamp(ts.c_str());
        if (tsval == TS_INVALID)
        {
            output_error_raw("loader::loadClock() parsing file, %s: expected time value in the clock. starttime "
                             "provided: %s.",
                             this->filename.string().c_str(), ts.c_str());
            return FAILED;
        }
        else
        {
            global_starttime = tsval;
        }
    }
    if (j_obj.contains("stoptime"))
    {
        string ts = j_obj["stoptime"].get<string>();
        clearQuotesFromStr(ts);
        TIMESTAMP tsval = convert_to_timestamp(ts.c_str());
        if (tsval == TS_INVALID)
        {
            output_error_raw("loader::loadClock() parsing file, %s: expected time value in the clock. stoptime "
                             "provided: %s.",
                             this->filename.string().c_str(), ts.c_str());
            return FAILED;
        }
        else
        {
            global_stoptime = tsval;
        }
    }

    return rv;
}

bool loader::module_properties(MODULE *mod, ojson properties)
{
    CLASS *oClass;
    bool rv = true;
    string propValue = "";
    for (auto &[name, value] : properties.items())
    {
        if (value.is_string())
        {
            string strValue = value.get<std::string>();
            if (parse.replace_variables(strValue))
            {
                value = strValue;
            }
            else
            {
                output_error("loader::module_properties() parsing file, %s: unable perform variable substitution in "
                             "module property value %s for property %s of module %s.",
                             this->filename.string().c_str(),
                             strValue.c_str(), name.c_str(), mod->name);
                return false;
            }
        }
        if (name == "inline_comments" || name == "outside_comments" || name == "inside_comments")
        {
            continue;
        }
        if (name == "major" && value.is_number())
        {
            // not used?
            short major = (short)value.get<int>();
        }
        else if (name == "minor" && value.is_number())
        {
            // not used?
            short minor = (short)value.get<int>();
        }
        else if (name == "class" && value.is_string())
        {
            const char *classname = value.get_ref<const std::string &>().c_str();
            if (strlen(classname) > 0 && strlen(classname) > 65)
            {
                oClass = class_get_class_from_classname(classname);
                if (oClass == nullptr || oClass->module != mod)
                {
                    output_error_raw("loader::module_properties() parsing file, %s: module, %s, does not implement "
                                     "class, %s.",
                                     this->filename.string().c_str(), mod->name, classname);
                    rv = false;
                    break;
                }
            }
        }
        // Must be property
        else if (convert(value, propValue) == 1)
        {
            currentObject = nullptr; /* object context */
            currentModule = mod;     /* module context */
            if (name != "inline_comments")
            {
                if (this->parse.alternate_value(propValue))
                {
                    if (!module_setvar(mod, (const char *)name.c_str(), propValue.data()))
                    {
                        output_error_raw("loader::module_properties() parsing file, %s: invalid property, %s, for "
                                         "module, %s, specified.",
                                         this->filename.string().c_str(), name.c_str(),
                                         mod->name);
                        rv = false;
                        break;
                    }
                }
            }
        }
        else
        {
            output_error_raw("loader::module_properties() parsing file %s: invalid %s module property value provided."
                             "property %s couldn't be set to value %s.",
                             this->filename.string().c_str(), mod->name,
                             name.c_str(), value.dump().c_str());
            rv = false;
            break;
        }
    }
    return rv;
}

STATUS loader::loadModules()
{
    MODULE *module;
    bool load;
    ojson j_obj = this->jsn["modules"];
    STATUS rv = SUCCESS;
    for (auto &[name, value] : j_obj.items())
    {
        module = module_load(name.data(), 0, nullptr);
        if (module != nullptr)
        {
            if (!module_properties(module, value))
            {
                output_error_raw("loader::loadModules() parsing file, %s: failed to load module %s.",
                                 this->filename.string().c_str(), module->name);
                rv = FAILED;
                break;
            }
        }
        else
        {
            output_error_raw("loader::loadModules() parsing file, %s: module load failed",
                             this->filename.string().c_str(), name.data());
            rv = FAILED;
            break;
        }
    }
    return rv;
}

STATUS loader::loadObjects()
{
    STATUS rv = SUCCESS;
    ojson objs = this->jsn["objects"];
    for (auto &[className, value] : objs.items())
    {
        if (rv == FAILED)
        {
            break;
        }
        for (auto &objInstance : value["instances"])
        {
            rv = this->loadObject(className, objInstance);
            if (rv == FAILED)
            {
                break;
            }
        }
    }
    return rv;
}

STATUS loader::loadObject(const string className, ojson objInstance)
{
    STATUS rv = SUCCESS;
    static OBJECT nameObj;
    char clsName[64] = "";
    OBJECT *obj = nullptr;
    CLASS *oClass = class_get_class_from_classname(className.c_str());
    string propValueStr = "";
    if (oClass == nullptr)
    {
        output_error("loader::loadObject() parsing file, %s: class, %s, is not known!", this->filename.string().c_str(),
                     className.c_str());
        return FAILED;
    }
    strncpy(clsName, className.c_str(), 63);
    nameObj.name = clsName;
    int64 id = -1;
    int64 id2 = -1;
    bool idSpecified = false;
    if (objInstance.contains("object_declaration"))
    {
        string objectDeclaration = objInstance["object_declaration"].get<string>();
        size_t strIdx = objectDeclaration.find(':');
        string classNameStripped = objectDeclaration.substr(strIdx + 1);
        if (classNameStripped.empty())
        {
            output_error("loader::loadObject() parsing file, %s: invalid object declaration %s. Missing identifier "
                         "syntax after the ':'.",
                         this->filename.string().c_str(), objectDeclaration.c_str());
            return FAILED;
        }
        strIdx = classNameStripped.find("..");
        if (strIdx != string::npos)
        {
            if (strIdx > 0)
            {
                id = stoll(classNameStripped, &strIdx);
                id2 = stoll(classNameStripped.substr(strIdx + 2)) + 1;
                if (id2 <= id)
                {
                    output_error("loader::loadObject() parsing file, %s: invalid object id ranges %s",
                                 this->filename.string().c_str(), objectDeclaration.c_str());
                    return FAILED;
                }
            }
            else
            {
                id = -1;
                id2 = stoll(classNameStripped.substr(strIdx + 2)) - 1;
                if (id2 <= id)
                {
                    output_error("loader::loadObject() parsing file, %s: invalid object count %s",
                                 this->filename.string().c_str(), objectDeclaration.c_str());
                    return FAILED;
                }
            }
        }
        else
        {
            idSpecified = true;
            id = stoll(classNameStripped);
        }
    }
    if (id2 <= id)
    {
        id2 = id + 1;
    }
    while (id < id2)
    {
        if (oClass->create != nullptr)
        {
            obj = &nameObj;
            if ((*oClass->create)(&obj, nullptr) == 0)
            {
                output_error("loader::loadObject() parsing file, %s: create failed for object %s:%d\n%s",
                             this->filename.string().c_str(), className.c_str(), id, objInstance.dump(4).c_str());
                rv = FAILED;
                break;
            }
            else if (obj == nullptr || obj == &nameObj)
            {
                output_error("loader::loadObject() parsing file, %s: create failed name object %s:%d\n%s",
                             this->filename.string().c_str(), className.c_str(), id, objInstance.dump(4).c_str());
                rv = FAILED;
                break;
            }
        }
        else
        {
            obj = object_create_single(oClass);
            if (obj == nullptr)
            {
                output_error("loader::loadObject() parsing file, %s: create failed for object %s:%d\n%s",
                             this->filename.string().c_str(), className.c_str(), id, objInstance.dump(4).c_str());
                rv = FAILED;
                break;
            }
            object_set_parent(obj, nullptr);
        }
        if (id != -1 && this->parse.load_set_index(obj, (OBJECTNUM)id) == FAILED)
        {
            output_error("loader::loadObject() parsing file, %s: create failed for object %s:%d\n%s",
                         this->filename.string().c_str(), className.c_str(), id, objInstance.dump(4).c_str());
            rv = FAILED;
            break;
        }
        // For checkpoint restore, ensure object has its saved unique name before
        // property iteration so post-init checkpoint values can be keyed by name.
        if (global_checkpoint_loaded && obj->name == nullptr &&
            objInstance.contains("name") && objInstance["name"].is_string())
        {
            std::string checkpoint_name = objInstance["name"].get<std::string>();
            if (!checkpoint_name.empty() && object_set_name(obj, checkpoint_name.data()) == nullptr)
            {
                output_error_raw("loader::loadObject() parsing file, %s: property name %s could not be used",
                                 this->filename.string().c_str(), checkpoint_name.c_str());
                rv = FAILED;
                break;
            }
        }
        for (auto &[propName, propValue] : objInstance.items())
        {
            if (propName == "inline_comments" || propName == "outside_comments" || propName == "inside_comments" || propName == "object_declaration" || propName == "_id")
            {
                continue;
            }
            // Name is assigned before this loop during checkpoint restore. Skip
            // re-applying it to avoid duplicate-name collisions in the object tree.
            if (global_checkpoint_loaded && propName == "name")
            {
                continue;
            }
            if (propName.compare("rng_state") == 0)
            {
                propValueStr = std::to_string(propValue.get<unsigned int>());
            }
            else if (this->convert(propValue, propValueStr) == FAILED)
            {
                // The JSON value is an array or object (e.g. saturation_calculated_vals
                // stored as a 12-element complex array).  We can't convert it to a plain
                // string, so skip it — it needs special post-init handling elsewhere.
                output_debug("loader::loadObject(): skipping non-scalar property '%s' on %s (array/object value)",
                             propName.c_str(), className.c_str());
                continue;
            }
            rv = this->objectProperties(oClass, obj, propName, propValueStr);
            if (rv == FAILED)
            {
                if (global_checkpoint_loaded)
                {
                    // When restoring a checkpoint, some properties written by the
                    // checkpoint writer may not be settable (e.g. read-only outputs
                    // like player::value).  Skip them with a debug message rather
                    // than aborting the entire load.
                    output_debug("loader::loadObject(): skipping unrestorable property '%s' on %s:%s in checkpoint",
                                 propName.c_str(), className.c_str(),
                                 obj->name ? obj->name : "?");
                    rv = SUCCESS;
                    continue;
                }
                break;
            }
            // Store property value for post-init restore when loading a checkpoint.
            // After init() runs (which may reset published properties to defaults),
            // object_restore_checkpoint_properties() re-applies these values so the
            // saved checkpoint state takes precedence.
            // Skip PT_object properties — they are resolved via the unresolved-reference
            // mechanism during load and init() does not reset them.
            // Skip transform expressions (word.word patterns) — the transform is
            // established during loading via linear_transform() and does not need
            // re-application; object_set_value_by_addr can't parse them anyway.
            if (global_checkpoint_loaded)
            {
                PROPERTY *cprop = class_find_property(oClass, propName.c_str());
                bool is_transform_expr = false;
                if (cprop != nullptr && cprop->ptype != PT_object && !propValueStr.empty())
                {
                    // Treat both object.property and schedule-name expressions as
                    // transforms already handled by linear_transform().
                    static const std::regex transform_pat(R"([A-Za-z_]\w*\.[A-Za-z_]\w*)");
                    is_transform_expr = std::regex_search(propValueStr, transform_pat);
                    if (!is_transform_expr)
                    {
                        TRANSFORMSOURCE cp_xstype = XS_UNKNOWN;
                        void *cp_source = nullptr;
                        double cp_scale = 1.0;
                        double cp_bias = 0.0;
                        is_transform_expr = this->parse.linear_transform(propValueStr, &cp_xstype, &cp_source,
                                                                         &cp_scale, &cp_bias, obj) > 0;
                    }
                }
                if (cprop == nullptr || (cprop->ptype != PT_object && !is_transform_expr))
                    object_store_checkpoint_property(obj, propName.c_str(), propValueStr.c_str());
            }
        }
        if (obj->name == nullptr)
        {
            if (idSpecified)
            {
                string objName = className + ":" + to_string(id);
                if (object_set_name(obj, objName.data()) == nullptr)
                {
                    output_error_raw("loader::loadObject() parsing file, %s: property name %s could not be used",
                                     this->filename.string().c_str(), objName.c_str());
                    rv = FAILED;
                }
            }
            else
            {
                string objName = className + ":" + to_string(obj->id);
                if (object_set_name(obj, objName.data()) == nullptr)
                {
                    output_error_raw("loader::loadObject() parsing file, %s: property name %s could not be used",
                                     this->filename.string().c_str(), objName.c_str());
                    rv = FAILED;
                }
            }
        }
        if (rv == FAILED)
        {
            break;
        }
        if (id == -1)
        {
            id2--;
        }
        else
        {
            id++;
        }
    }

    // Override rng_state with name (if present) and randomseed (if present) based hash
    if (rv != FAILED && global_randomseed > 0 && obj->flags & OF_RANDOMSEEDSET != OF_RANDOMSEEDSET)
    {
        if (obj->name == nullptr)
        {
            obj->rng_state = static_cast<unsigned int>(polynomialHasher(std::to_string(global_randomseed)));
        }
        else
        {
            obj->rng_state = static_cast<unsigned int>(polynomialHasher(std::string(obj->name) + std::to_string(global_randomseed)));
        }
        obj->flags |= OF_RANDOMSEEDSET;
    }

    return rv;
}

STATUS loader::objectProperties(CLASS *oClass, OBJECT *obj, string propName, string propValue)
{
    char1024 propertyValue = "";
    double dval;
    gld::complex cval;
    void *source = nullptr;
    TRANSFORMSOURCE xstype = XS_UNKNOWN;
    char transformname[1024];
    char sources[4096];
    double scale = 1, bias = 0;
    UNIT *unit = nullptr;
    STATUS status = SUCCESS;
    if (this->parse.replace_variables(propValue) == false)
    {
        output_error("loader::objectProperties() parsing file, %s: unable perform variable substitution in "
                     "object property value %s for property %s of object %s/%s.",
                     this->filename.string().c_str(),
                     propValue.c_str(), propName.c_str(), obj->oclass->module->name, obj->oclass->name);
        return FAILED;
    }
    clearQuotesFromStr(propValue);
    LOADMETHOD *method = class_get_loadmethod(obj->oclass, propName.c_str());
    if (method != nullptr)
    {
        method->args = propValue;
        if (this->parse.value(propValue, propertyValue, sizeof(propertyValue)))
        {
            if (method->call(obj, propertyValue) != 1)
            {
                output_error("loader::objectProperties() parsing file, %s: Load method, %s/%s::%s, failed on value, "
                             "%s.",
                             this->filename.string().c_str(), obj->oclass->module->name, obj->oclass->name,
                             propName.c_str(), propertyValue.get_string());
                status = FAILED;
            }
        }
        else
        {
            output_error_raw("loader::objectProperties() parsing file, %s: unable to parse value for load method, "
                             "%s/%s::%s.",
                             this->filename.string().c_str(), obj->oclass->module->name,
                             obj->oclass->name, propName.c_str());
            status = FAILED;
        }
    }
    else
    {
        PROPERTY *prop = class_find_property(oClass, propName.c_str());
        // Legacy dishwasher checkpoints may contain "myshape" even though the
        // published residential_enduse property is "shape".
        if (global_checkpoint_loaded && prop == nullptr && propName == "myshape")
        {
            propName = "shape";
            prop = class_find_property(oClass, propName.c_str());
        }
        if (prop != nullptr)
            object_set_raw_value_by_name(obj, propName.c_str(), propValue.data());
        this->currentObject = obj;
        this->currentModule = obj->oclass->module;
        this->parse.current_object = obj;

        // Protect against empty property values to prevent segfaults in parsing functions
        if (propValue.empty())
        {
            if (prop == nullptr)
            {
                output_error_raw("loader::objectProperties() parsing file, %s: property %s is not defined in class "
                                 "%s and has empty value",
                                 this->filename.string().c_str(), propName.c_str(),
                                 oClass->name);
                return FAILED;
            }
            // Empty value for valid property - just skip parsing
            return SUCCESS;
        }

        if (prop != nullptr && prop->ptype == PT_complex && this->parse.complex_unit(propValue, &cval, &unit) > 0)
        {
            if (unit != nullptr && prop->unit != nullptr && strcmp((char *)unit, "") != 0 && unit_convert_complex(unit, prop->unit, &cval) == 0)
            {
                output_error_raw("loader::objectProperties() parsing file, %s: units of value are incompatible "
                                 "with units of property %s, cannot convert from %s to %s",
                                 this->filename.string().c_str(), propName.c_str(), unit->name, prop->unit->name);
                status = FAILED;
            }
            else if (object_set_complex_by_name(obj, propName.c_str(), cval) == 0)
            {
                output_error_raw("loader::objectProperties() parsing file, %s: property %s of %s could not be set "
                                 "to %g%+gi",
                                 this->filename.string().c_str(), propName.c_str(),
                                 this->parse.format_object(obj), cval.Re(), cval.Im());
                status = FAILED;
            }
        }
        else if (prop != nullptr && prop->ptype == PT_double && this->parse.expression(propValue, &dval, &unit, obj) > 0)
        {
            if (unit != nullptr && prop->unit != nullptr && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit, prop->unit, &dval) == 0)
            {
                output_error_raw("loader::objectProperties() parsing file, %s: units of value are incompatible with "
                                 "units of property %s, cannot convert from %s to %s",
                                 this->filename.string().c_str(),
                                 propName.c_str(), unit->name, prop->unit->name);
                status = FAILED;
            }
            else if (object_set_double_by_name(obj, propName.c_str(), dval) == 0)
            {
                output_error_raw("loader::objectProperties() parsing file, %s: property %s of %s could not be set to "
                                 "%g",
                                 this->filename.string().c_str(), propName.c_str(), this->parse.format_object(obj), dval);
                status = FAILED;
            }
        }
        else if (prop != nullptr && prop->ptype == PT_double && this->parse.functional_unit(propValue, &dval, &unit) > 0)
        {
            if (unit != nullptr && prop->unit != nullptr && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit, prop->unit, &dval) == 0)
            {
                output_error_raw("loader::objectProperties() parsing file, %s: units of value are incompatible with "
                                 "units of property %s, cannot convert from %s to %s",
                                 this->filename.string().c_str(),
                                 propName.c_str(), unit->name, prop->unit->name);
                status = FAILED;
            }
            else if (object_set_double_by_name(obj, propName.c_str(), dval) == 0)
            {
                output_error_raw("loader::objectProperties() parsing file, %s: property %s of %s could not be set to "
                                 "%g",
                                 this->filename.string().c_str(), propName.c_str(), this->parse.format_object(obj), dval);
                status = FAILED;
            }
        }
        else if (prop != nullptr && isInt(prop->ptype) && this->parse.functional_unit(propValue, &dval, &unit) > 0)
        {
            int64 ival = 0;
            int16 ival16 = 0;
            int32 ival32 = 0;
            int64 ival64 = 0;
            int rv = 0;
            if (unit != nullptr && prop->unit != nullptr && strcmp((char *)(unit), "") != 0 && unit_convert_ex(unit, prop->unit, &dval) == 0)
            {
                output_error_raw("loader::objectProperties() parsing file, %s: units of value are incompatible with "
                                 "units of property %s, cannot convert from %s to %s",
                                 this->filename.string().c_str(),
                                 propName.c_str(), unit->name, prop->unit->name);
                status = FAILED;
            }
            else
            {
                switch (prop->ptype)
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
                case PT_timestamp:
                    ival = ival64 = (int64)dval;
                    rv = object_set_int64_by_name(obj, propName.c_str(), ival64);
                    break;
                default:
                    output_error("loader::objectProperties() parsing file, %s: function_int operating on a "
                                 "non-integer (we shouldn't be here!)",
                                 this->filename.string().c_str());
                    rv = 0;
                }
                if (rv == 0)
                {
                    output_error_raw("loader::objectProperties() parsing file, %s: property %s of %s could not be set "
                                     "to %g",
                                     this->filename.string().c_str(), propName.c_str(),
                                     this->parse.format_object(obj), ival);
                    status = FAILED;
                }
            }
        }
        else if (prop != nullptr && (((prop->ptype >= PT_double && prop->ptype <= PT_int64) || (prop->ptype >= PT_bool && prop->ptype <= PT_timestamp) || (prop->ptype >= PT_float && prop->ptype <= PT_enduse) || prop->ptype == PT_enumeration || prop->ptype == PT_set) && this->parse.linear_transform(propValue, &xstype, &source, &scale, &bias, obj) > 0))
        {
            void *target = (void *)((char *)(obj + 1) + (int64)prop->addr);
            /* add the transform list */
            if (!transform_add_linear(xstype, static_cast<double *>(source), target, scale, bias, obj, prop,
                                      static_cast<SCHEDULE *>(xstype == XS_SCHEDULE ? source : 0)))
            {
                output_error_raw("loader::objectProperties() parsing file, %s: schedule transform could not be "
                                 "created - %s",
                                 this->filename.string().c_str(), errno ? strerror(errno) : "(no details)");
                status = FAILED;
            }
            else if (source != nullptr)
            {
                /* a transform is unresolved */
                if (parse.first_unresolved == source)
                {
                    /* source was the unresolved entry, for now it will be the transform itself */
                    parse.first_unresolved->ref = (void *)transform_getnext(nullptr);
                }
            }
        }
        else if (this->parse.alternate_value(propValue) == true)
        {
            if (prop == nullptr)
            {
                /* check for special properties */
                if (propName.compare("root") == 0)
                {
                    obj->parent = nullptr;
                }
                else if (propName.compare("parent") == 0)
                {
                    if (parse.add_unresolved(obj, PT_object, (void *)&obj->parent, oClass, propValue.data(),
                                             this->filename.string().data(), UR_RANKS) == nullptr)
                    {
                        output_error_raw("loader::objectProperties() parsing file, %s: unable to add unresolved "
                                         "reference to parent %s",
                                         this->filename.string().c_str(), propValue.c_str());
                        status = FAILED;
                    }
                }
                else if (propName.compare("id") == 0)
                {
                    status = SUCCESS;
                    // if (obj->id = stoi(propValue) < 0)
                    // {
                    //     output_error_raw("loader::objectProperties() parsing file, %s: unable to set id to %s",
                    //                      this->filename.string().c_str(), propValue.c_str());
                    //     status = FAILED;
                    // }
                }
                else if (propName.compare("rank") == 0)
                {
                    if ((obj->rank = stoi(propValue)) < 0)
                    {
                        output_error_raw("loader::objectProperties() parsing file, %s: unable to set rank to %s",
                                         this->filename.string().c_str(), propValue.c_str());
                        status = FAILED;
                    }
                }
                else if (propName.compare("clock") == 0)
                {
                    obj->clock = stoll(propValue); // @todo convert_to_timestamp should be used
                }
                else if (propName.compare("valid_to") == 0)
                {
                    obj->valid_to = stoll(propValue); // @todo convert_to_timestamp should be used
                }
                else if (propName.compare("schedule_skew") == 0)
                {
                    obj->schedule_skew = stoll(propValue);
                }
                else if (propName.compare("latitude") == 0)
                {
                    obj->latitude = this->loadLatitude(propValue.data());
                }
                else if (propName.compare("longitude") == 0)
                {
                    obj->longitude = this->loadLongitude(propValue.data());
                }
                else if (propName.compare("in") == 0 || propName.compare("in_svc") == 0)
                {
                    obj->in_svc = convert_to_timestamp_delta(propValue.c_str(), &obj->in_svc_micro,
                                                             &obj->in_svc_double);
                }
                else if (propName.compare("out") == 0 || propName.compare("out_svc") == 0)
                {
                    obj->out_svc = convert_to_timestamp_delta(propValue.c_str(), &obj->out_svc_micro,
                                                              &obj->out_svc_double);
                }
                else if (propName.compare("in_svc_double") == 0)
                {
                    obj->in_svc_double = stod(propValue);
                }
                else if (propName.compare("out_svc_double") == 0)
                {
                    obj->out_svc_double = stod(propValue);
                }
                else if (propName.compare("rng_state") == 0)
                {
                    obj->rng_state = static_cast<unsigned int>(stoul(propValue));
                    obj->flags |= OF_RANDOMSEEDSET;
                }
                else if (propName.compare("name") == 0)
                {
                    if (object_set_name(obj, propValue.data()) == nullptr)
                    {
                        output_error_raw("loader::objectProperties() parsing file, %s: property name %s could not be "
                                         "used",
                                         this->filename.string().c_str(), propValue.c_str());
                        status = FAILED;
                    }
                }
                else if (propName.compare("heartbeat") == 0)
                {
                    obj->heartbeat = convert_to_timestamp(propValue.c_str());
                }
                else if (propName.compare("groupid") == 0)
                {
                    strncpy(obj->groupid, propValue.c_str(), sizeof(obj->groupid));
                }
                else if (propName.compare("flags") == 0)
                {
                    if (this->set_flags(obj, propValue.data()) == 0)
                    {
                        status = FAILED;
                    }
                }
                else if (propName.compare("library") == 0)
                {
                    output_warning("loader::objectProperties() parsing file, %s: libraries not yet supported",
                                   this->filename.string().c_str());
                    /* TROUBLESHOOT
                        An attempt to use the <b>library</b> GLM directive was made.  Library directives
                        are not supported yet.
                    */
                }
                else
                {
                    output_error_raw("loader::objectProperties() parsing file, %s: property %s is not defined in class "
                                     "%s",
                                     this->filename.string().c_str(), propName.c_str(), oClass->name);
                    status = FAILED;
                }
            }
            else if (prop->ptype == PT_object)
            {
                void *addr = object_get_addr(obj, propName.c_str());
                if (addr == nullptr)
                {
                    output_error_raw("loader::objectProperties() parsing file, %s: unable to get %s member %s",
                                     this->filename.string().c_str(), this->parse.format_object(obj), propName.c_str());
                    status = FAILED;
                }
                else
                {
                    parse.add_unresolved(obj, PT_object, addr, oClass, propValue.data(), this->filename.string().data(),
                                         UR_NONE);
                }
            }
            else
            {
                if (object_set_value_by_name(obj, propName.data(), propValue.data()) == 0)
                {
                    output_error_raw("loader::objectProperties() parsing file, %s: property %s of %s could not be set "
                                     "to %s",
                                     this->filename.string().c_str(), propName.c_str(),
                                     this->parse.format_object(obj), propValue.c_str());
                    status = FAILED;
                }
            }
        }
        else
        {
            output_error("loader::objectProperties() parsing file, %s: Encountered invalid property value pairing! "
                         "Property %s of %s with value of %s",
                         this->filename.string().c_str(), propName.c_str(),
                         this->parse.format_object(obj), propValue.c_str());
            status = FAILED;
        }
    }
    return status;
}

int loader::isInt(PROPERTYTYPE pt)
{
    if (pt == PT_int16 || pt == PT_int32 || pt == PT_int64 || pt == PT_timestamp)
    {
        return (int)pt;
    }
    else
    {
        return 0;
    }
}

double loader::loadLatitude(char *buffer)
{
    char oname[128], pname[128];
    double v = convert_to_latitude(buffer);
    if (sscanf(buffer, "(%[^.].%[^)])", oname, pname) == 2 && strcmp(pname, "latitude") == 0)
    {
        OBJECT *obj = object_find_name(oname);
        if (obj == nullptr)
        {
            output_error_raw("loader::loadLatitude() parsing file, %s: %s does not refer to an existing object.",
                             this->filename.string().c_str(), buffer);
        }
        return obj->latitude;
    }
    else if (isnan(v) && (strcmp(buffer, "") != 0 || strcasecmp(buffer, "none") != 0))
    {
        output_error_raw("loader::loadLatitude() parsing file, %s: %s is not a valid latitude",
                         this->filename.string().c_str(), buffer);
    }
    else
    {
        output_debug("loader::loadLatitude() parsing file, %s: latitude is converted to %lf",
                     this->filename.string().c_str(), v);
    }
    return v;
}

double loader::loadLongitude(char *buffer)
{
    char oname[128], pname[128];
    double v = convert_to_longitude(buffer);
    if (sscanf(buffer, "(%[^.].%[^)])", oname, pname) == 2 && strcmp(pname, "longitude") == 0)
    {
        OBJECT *obj = object_find_name(oname);
        if (obj == nullptr)
        {
            output_error_raw("loader::loadLongitude() parsing file, %s: %s does not refer to an existing object",
                             this->filename.string().c_str(), buffer);
        }
        return obj->longitude;
    }
    else if (isnan(v) && (strcmp(buffer, "") != 0 || strcasecmp(buffer, "none") != 0))
    {
        output_error_raw("loader::loadLongitude() parsing file, %s: %s is not a valid longitude",
                         this->filename.string().c_str(), buffer);
    }
    else
    {
        output_debug("loader::loadLongitude() parsing file, %s: longitude is convert to %lf",
                     this->filename.string().c_str(), v);
    }
    return v;
}

int loader::set_flags(OBJECT *obj, char *propval)
{
    extern KEYWORD oflags[];
    if (convert_to_set(propval, &(obj->flags), object_flag_property()) <= 0)
    {
        output_error_raw("loader::set_flags() parsing file, %s: flags of %s:%d %s could not be set to %s",
                         this->filename.string().c_str(), obj->oclass->name, obj->id, obj->name, propval);
        return 0;
    };
    return 1;
}

STATUS loader::loadSchedules()
{
    STATUS rv = SUCCESS;
    auto j_obj = this->jsn["schedules"];
    string cron_schedule;
    string schedule_block;
    size_t numberOfBlocks = 0;
    SCHEDULE *sch;
    for (auto &[name, schedule] : j_obj.items())
    {
        if (rv == FAILED)
        {
            break;
        }
        if (schedule.is_string())
        {
            cron_schedule = schedule.get<std::string>();
            sch = schedule_create(name.data(), cron_schedule.data());
            if (sch != nullptr)
            {
                sch->raw = cron_schedule;
            }
            else
            {
                output_error_raw("loader::loadSchedules() parsing file, %s: schedule '%s' could not be created",
                                 this->filename.string().c_str(), name.data());
                rv = FAILED;
                break;
            }
        }
        else if (schedule.is_array())
        {
            cron_schedule = "";
            numberOfBlocks = schedule.size();
            for (auto &block : schedule)
            {
                schedule_block = "";
                if (block.is_object())
                {
                    if (numberOfBlocks > 1)
                    {
                        schedule_block += block["name"].get<std::string>();
                        if (schedule_block == "")
                        {
                            schedule_block += "{";
                        }
                        else
                        {
                            schedule_block += " {";
                        }
                    }
                    for (auto &item : block["items"])
                    {
                        schedule_block += item.get<string>() + ";";
                    }
                    if (numberOfBlocks > 1)
                    {
                        schedule_block += "}";
                    }
                }
                else
                {
                    output_error_raw("loader::loadSchedules() parsing file, %s: schedule %s is not valid or does not "
                                     "have an items array",
                                     this->filename.string().c_str(), name.data());
                    rv = FAILED;
                    break;
                }
                cron_schedule += schedule_block;
            }

            sch = schedule_create(name.data(), cron_schedule.data());
            if (sch != nullptr)
            {
                sch->raw = schedule.dump();
            }
            else
            {
                output_error_raw("loader::loadSchedules() parsing file, %s: schedule '%s' could not be created",
                                 this->filename.string().c_str(), name.data());
                rv = FAILED;
                break;
            }
        }
        else
        {
            output_error_raw("loader::loadSchedules() parsing file, %s: schedule '%s' is not valid array",
                             this->filename.string().c_str(), name.data());
            rv = FAILED;
            break;
        }
    }
    return rv;
}

STATUS loader::loadGlobals()
{
    if (!this->jsn.contains("globals") || !this->jsn["globals"].is_object())
    {
        return SUCCESS; // no globals section — nothing to do
    }

    // Read-only or runtime-set globals that must not be overridden on reload
    static const std::unordered_set<std::string> skip_globals = {
        "version",
        "version.major",
        "version.minor",
        "version.patch",
        "version.build",
        "version.branch",
        "platform",
        "exename",
        "execdir",
        "checkpoint_loaded",
    };

    STATUS result = SUCCESS;
    std::string propValue;
    for (auto &[name, value] : this->jsn["globals"].items())
    {
        if (skip_globals.count(name))
        {
            continue;
        }
        if (name.compare("randomseed") == 0)
        {
            global_randomseed = value.get<unsigned int>();
            continue;
        }
        if (convert(value, propValue) == FAILED)
        {
            output_warning("loader::loadGlobals() parsing file, %s: unable to convert value for global '%s', skipping",
                           this->filename.string().c_str(), name.c_str());
            continue;
        }

        GLOBALVAR *gvar = global_find(name);
        if (gvar != nullptr && gvar->prop != nullptr && gvar->prop->ptype == PT_timestamp)
        {
            // convert_to_timestamp() treats bare numeric values as invalid; epoch
            // values in JSON checkpoints should be interpreted as seconds.
            if (value.is_number())
            {
                propValue += "s";
            }
            else if (value.is_string())
            {
                static const std::regex numeric_epoch_pattern(R"(^[0-9]+(?:\.[0-9]+)?$)");
                if (std::regex_match(propValue, numeric_epoch_pattern))
                {
                    propValue += "s";
                }
            }
        }

        STATUS rv = global_setvar(name.c_str(), propValue.data());
        if (rv == FAILED)
        {
            output_warning("loader::loadGlobals() parsing file, %s: could not set global '%s', skipping",
                           this->filename.string().c_str(), name.c_str());
            // Non-fatal — continue with remaining globals
        }
    }
    return result;
}

STATUS loader::loadJsonFile(filesystem::path filename)
{
    if (this->open_file(filename))
    {
        if (this->filename.empty() || this->filename != filename)
        {
            this->filename = filename;
            this->parse.filename = this->filename.string();
        }
        if (this->jsn.empty())
        {
            output_error("loader::loadJsonFile() parsing file, %s: file is empty!", filename.string().c_str());
            return FAILED;
        }
        // Check if this is a checkpoint file
        if (this->jsn.contains("_checkpoint") && this->jsn["_checkpoint"].is_boolean() && this->jsn["_checkpoint"].get<bool>() == true)
        {
            global_checkpoint_loaded = 1;
            output_verbose("loader::loadJsonFile(): checkpoint file detected");
        }
        if (this->loadDirectives() == FAILED)
        {
            return FAILED;
        }
        if (this->loadModules() == FAILED)
        {
            return FAILED;
        }
        if (global_checkpoint_loaded && this->loadGlobals() == FAILED)
        {
            return FAILED;
        }
        if (this->loadIncludes() == FAILED)
        {
            return FAILED;
        }
        if (this->loadClock() == FAILED)
        {
            return FAILED;
        }
        if (this->loadClasses() == FAILED)
        {
            return FAILED;
        }
        if (this->loadSchedules() == FAILED)
        {
            return FAILED;
        }
        if (this->loadObjects() == FAILED)
        {
            return FAILED;
        }
    }
    else
    {
        return FAILED;
    }
    return SUCCESS;
}

STATUS loader::loadall_json_roll(char *file_name)
{
    /**< a pointer to the first character in the file name string */
    OBJECT *obj, *first = object_get_first();
    errno = 0;
    STATUS status = SUCCESS;
    if (loadJsonFile(filesystem::path(file_name)) == FAILED)
    {
        output_error("loader::loadall_json_roll() parsing file, %s: unable to load JSON file!", file_name);
        return FAILED;
    }
    else if (this->parse.load_resolve_all() == FAILED)
    {
        return FAILED;
    }
    /* establish ranks */
    for (obj = first ? first : object_get_first(); obj != nullptr; obj = obj->next)
    {
        object_set_parent(obj, obj->parent);
    }
    output_verbose("loader::loadall_json_roll(): %d object%s loaded", object_get_count(), object_get_count() > 1 ? "s" : "");
    return status;
}

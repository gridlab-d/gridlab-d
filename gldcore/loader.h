/*
 *  Created on: Aug 15, 2025
 *      Author: d3j331 - Mitch Pelton, Andy Fisher
 */

#ifndef _LOADER_H_
#define _LOADER_H_

#include "class.h"
#include "globals.h"
#include "module.h"
#include "output.h"
#include "parser.h"
#include "property.h"

#include <format>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <regex>
#include <nlohmann/json.hpp>  // Requires JSON for Modern C++ library

using namespace std;
using json = nlohmann::json;

#ifdef __cplusplus

class loader {

private:
	parser parse = parser();
    json jsn;
    string filename;
    OBJECT *currentObject = nullptr;
    MODULE *currentModule = nullptr;
    STATUS convert(json value, string &out);
    queue<string> included_files;
    STATUS loadObject(const string className, json objInstance);
    STATUS objectProperties(CLASS *oClass, OBJECT *obj, const string propName, string propValue);
    int isInt(PROPERTYTYPE pt);
    double loadLatitude(char *buffer);
    double loadLongitude(char *buffer);
    int set_flags(OBJECT *obj, char *propval);

public:
    bool open_file(string file_name);
    STATUS loadDirective();
    STATUS loadClasses();
    STATUS loadClock();

    bool module_properties(MODULE *mod, json properties);
    STATUS loadModules();
    STATUS loadObjects();
    STATUS loadSchedules();
    STATUS loadDirectives();
    STATUS loadall_json_roll(char *file_name);
};

#endif // C++

#endif // _LOADER_H_


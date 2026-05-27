/*
 *  Created on: Aug 15, 2025
 *      Author: Mitch Pelton, Andy Fisher, Ryal O'Neil
 */

#ifndef _LOADER_H_
#define _LOADER_H_

#include "class.h"
#include "globals.h"
#include "module.h"
#include "output.h"
#include "parser.h"
#include "property.h"

#include <charconv>
// #include <format>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <regex>
#include <nlohmann/json.hpp>  // Requires JSON for Modern C++ library

using namespace std;
using ojson = nlohmann::ordered_json;

#ifdef __cplusplus

class loader
{

private:
    parser parse = parser();
    ojson jsn;
    filesystem::path filename;
    OBJECT *currentObject = nullptr;
    MODULE *currentModule = nullptr;
    STATUS convert(ojson value, string &out);
    STATUS loadObject(const string className, ojson objInstance);
    STATUS objectProperties(CLASS *oClass, OBJECT *obj, const string propName, string propValue);
    int isInt(PROPERTYTYPE pt);
    double loadLatitude(char *buffer);
    double loadLongitude(char *buffer);
    int set_flags(OBJECT *obj, char *propval);
    void clearQuotesFromStr(string &str);
    STATUS loadJsonFile(filesystem::path filePath);
    unsigned int64 polynomialHasher(string key);

public:
    bool open_file(filesystem::path &file_name);
    STATUS loadDirective();
    bool class_properties(CLASS *oclass, ojson properties, string source_code);
    STATUS loadClasses();
    STATUS loadClock();

    bool module_properties(MODULE *mod, ojson properties);
    STATUS loadModules();
    STATUS loadObjects();
    STATUS loadSchedules();
    STATUS loadDirectives();
    STATUS loadIncludes();
    STATUS loadGlobals();
    STATUS loadall_json_roll(char *file_name);
};

#endif // C++

#endif // _LOADER_H_

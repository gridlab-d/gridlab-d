/*
 *  Created on: Aug 15, 2025
 *      Author: d3j331 - Mitch Pelton, Andy Fisher
 */

#ifndef _LOADER_H_
#define _LOADER_H_


#include "globals.h"
#include "module.h"
#include "parser.h"

#include <string>
#include <nlohmann/json.hpp>  // Requires JSON for Modern C++ library

using namespace std;
using json = nlohmann::json;

#ifdef __cplusplus

class loader {

private:
	parser parse;

    json jsn;
    json property;
    string filename;

public:
    bool open_file(string file_name);
    void loadDirective();
    void loadClasses();
    void loadClock();

    bool module_properties(MODULE *mod);
    bool module_conditionals();
    void loadModules();
    void loadObjects();
    void loadSchedules();
    STATUS loadall_glm_roll(char *file_name);
};

#endif // C++

#endif // _LOADER_H_


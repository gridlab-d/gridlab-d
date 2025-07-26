/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include<vector>

#include "gridlabd.h"

#include "gld_assert.h"
#include "double_assert.h"
#include "complex_assert.h"
#include "enum_assert.h"
#include "int_assert.h"


std::vector<std::pair<std::unique_ptr<gld_object>, std::string>> allocated_objects;

template <typename T>
void register_object(MODULE* module) {

	//std::cout << "Attempting to register type: " << typeid(T).name() << std::endl;

	T* obj = new T(module);

	allocated_objects.emplace_back(std::make_unique<T>(module), typeid(T).name());


	//std::cout << "Registered object of type: " << typeid(T).name() << ", at: " << obj << std::endl;
}




EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==nullptr)
	{
		errno = EINVAL;
		return nullptr;
	}

	/*new g_assert(module);
	new double_assert(module);
	new complex_assert(module);
	new enum_assert(module);
    new int_assert(module);
*/

	register_object<g_assert>(module);
	register_object < double_assert>(module);
	register_object < complex_assert>(module);
	register_object < enum_assert>(module);
	register_object < int_assert>(module);


	/* always return the first class registered */
	return g_assert::oclass;
}


EXPORT int do_kill(void*)
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	/* if any assert objects have bad filenames, they'll fail on init() */
	return 0;
}

/** $Id: weather_reader.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file weather_reader.cpp
	@author Matthew L Hauer
 **/

#include "weather_reader.h"

weather_reader::weather_reader(){
	infile = 0;
	data_head = 0;
	data_tail = 0;
}

weather_reader::~weather_reader(){
	;
}

// EOF

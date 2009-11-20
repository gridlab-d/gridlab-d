/** $Id: weather_reader.cpp 1182 2009-11-03 22:08:36Z mhauer $
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

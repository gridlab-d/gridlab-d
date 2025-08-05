/** $Id: csv_reader.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file csv_reader.cpp
	@author Matthew L. Hauer

 **/

#include <memory> // For std::unique_ptr
#include <vector>
#include <sstream> // For safer string tokenization

#include "csv_reader.h"

CLASS *csv_reader::oclass = 0;

EXPORT int create_csv_reader(OBJECT **obj, OBJECT *parent){
	csv_reader *my = 0;
	*obj = gl_create_object(csv_reader::oclass);
	if(*obj != nullptr){
		return 1;
	}
	//printf("create_csv_reader\n");
	return 0;	// don't want it to get called, but better to have it not be fatal
}

EXPORT int init_csv_reader(OBJECT **obj, OBJECT *parent){
	csv_reader *my = /*OBJECTDATA(obj, csv_reader)*/   object_data<csv_reader>(obj) ;
	return 1; // let the climate object cause the file to open
}

/// Synchronize the cliamte object
EXPORT TIMESTAMP sync_csv_reader(OBJECT *obj, TIMESTAMP t0){
	return TS_NEVER; // really doesn't do anything
}

csv_reader::csv_reader(){
	////memset(this, 0, sizeof(csv_reader));
}

csv_reader::csv_reader(MODULE *module){
	////memset(this, 0, sizeof(csv_reader));
	if (oclass==nullptr)
	{
		oclass = gl_register_class(module,"csv_reader",sizeof(csv_reader), 0);
		if (gl_publish_variable(oclass,
			PT_int32,"index",PADDR(index),PT_ACCESS,PA_REFERENCE,
			PT_char32,"city_name",PADDR(city_name),
			PT_char32,"state_name",PADDR(state_name),
			PT_double,"lat_deg",PADDR(lat_deg),
			PT_double,"lat_min",PADDR(lat_min),
			PT_double,"long_deg", PADDR(long_deg),
			PT_double,"long_min",PADDR(long_min),
			PT_double,"low_temp",PADDR(low_temp),PT_ACCESS,PA_REFERENCE,
			PT_double,"high_temp",PADDR(high_temp),PT_ACCESS,PA_REFERENCE,
			PT_double,"peak_solar",PADDR(peak_solar),PT_ACCESS,PA_REFERENCE,
			PT_int32,"elevation",PADDR(elevation),
			PT_enumeration,"status",PADDR(status),PT_ACCESS,PA_REFERENCE,
				PT_KEYWORD,"INIT",(enumeration)CR_INIT,
				PT_KEYWORD,"OPEN",(enumeration)CR_OPEN,
				PT_KEYWORD,"ERROR",(enumeration)CR_ERROR,
			PT_char32,"timefmt",PADDR(timefmt),
			PT_char32,"timezone",PADDR(timezone),
			PT_double,"timezone_offset",PADDR(tz_numval),
			PT_char256,"columns",PADDR(columns_str),
			PT_char256,"filename",PADDR(filename),
			nullptr)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		//memset(this,0,sizeof(csv_reader));
	}
}

/**
	Open a CSV file and parse it as
 **/
int csv_reader::open(const char *file){
	char line[1024];
	char filename[128];
	int has_cols = 0;
	int linenum = 0;
	int i = 0;
	OBJECT *obj = object_header(this);
	weather *wtr = 0;

	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	

	gl_debug("Reading weather file from %s\\%s",cwd, file);

	if(file == 0){
		gl_error("csv_reader has no input file name!");
		/* TROUBLESHOOT
			No input file was specified for the csv_reader object.  Double-check the
			input model and re-run GridLAB-D.
		*/
		return 0;
	}

	strncpy(filename, file, 127);
	infile = fopen(filename, "r");
	if(infile == 0){
		gl_error("csv_reader could not open \'%s\' for input!", file);
		/* TROUBLESHOOT
			The specified input file could not be opened for reading.  Verify that no
			other applications are using that file, double-check the input model, and
			re-run GridLAB-D.
		*/
		return 0;
	}

	if(columns_str[0] != 0){
		if(0 == read_header(columns_str)){
			gl_error("csv_reader::open ~ column header read failure from explicit headers");
			return 0;
		} else {
			has_cols = 1;
		}
	}
	while(fgets(line, 1024, infile) != nullptr){
		++linenum;
		// consume leading whitespace?
		// comments following valid lines?
		size_t _len = strlen(line);
		if(line[0] == '#'){	// comment
			continue;
		}
		else if(strlen(line) < 1){
			continue; // blank line
		}
		else if(line[0] == '$'){	// property
			if(0 == read_prop(line+1)){
				gl_error("csv_reader::open ~ property read failure on line %i", linenum);
				return 0;
			} else {
				continue;
			}
		}
		else if(has_cols == 0){
			if(0 == read_header(line)){
				gl_error("csv_reader::open ~ column header read failure on line %i", linenum);
				return 0;
			} else {
				has_cols = 1;
			}
		} else {
			int line_rv = read_line(line, linenum);
			if(0 == line_rv){
				gl_error("csv_reader::open ~ data line read failure on line %i", linenum);
				return 0;
			} else if (1 == line_rv){ // good read
				++sample_ct;
			} else if (2 == line_rv){ // read went 'backwards' or was blank, line discarded.
				;
			}
		}
	}

	//if (samples) {
	//	free(samples);  // cleanup from previous allocation
	//	samples = nullptr;
	//}


	/* move list into double pointer */
	//samples = (weather**)malloc(sizeof(weather *) * (size_t) sample_ct);

	samples = std::vector<weather*>();

	for(i = 0, wtr = weather_root.get(); i < sample_ct && wtr != nullptr; ++i, wtr=wtr->next.get()){
		if (!wtr) {
			gl_error("weather_root list terminated early at index %d", i);
			break;
		}
		//samples[i] = wtr;
		samples.push_back(wtr);

	}

	if (samples.empty()) {
		gl_error("csv_reader::open ~ no valid weather samples loaded");
		return 0;
	}

	sample_ct = i; // if wtr was the limiting factor, truncate the count

//	index = -1;	// forces to start on zero-eth index

	// post-process
	// calculate object lat/long
	obj->latitude = lat_deg + (lat_deg > 0 ? lat_min : -lat_min) / 60;
	obj->longitude = long_deg + (long_deg > 0 ? long_min : -long_min) / 60;

	return 1;
}

int csv_reader::read_prop(char *line){ // already pulled the '$' off the front
	OBJECT *my = object_header(this);
	char *split = strchr(line, '=');
	char propstr[256], valstr[256];
	PROPERTY *prop = 0;

	if(split == nullptr){
		gl_error("csv_reader::read_prop ~ missing \'=\' seperator");
		/* TROUBLESHOOT
			Property lines must have the property name and property value seperated by an
			equals sign.  Please correct the CSV file and re-run GridLAB-D.
		*/
		return 0;
	}

	if(2 != sscanf(line, "%[^=]=%[^\n#]", propstr, valstr)){
		gl_error("csv_reader::read_prop ~ error reading property & value");
		/* TROUBLESHOOT
			The line was not read properly by the parser.  Property lines must be of the format
			"prop=val".  Please review the CSV file and re-run GridLAB-D.
		*/
		return 0;
	}
	
	prop = gl_find_property(oclass, propstr);
	if(prop == 0){
		gl_error("csv_reader::read_prop ~ unrecognized csv_reader property \'%s\'", propstr);
		/* TROUBLESHOOT
			The property specified within the CSV file is not published by csv_reader.
			Please review the list of published variables, correct the CSV file, and
			re-run GridLAB-D.
		*/
		return 0;
	}

	// Windows pointers on my Dell Precision 390 are apparently ulli -MH
	/* BAD BAD BAD!  cannot use 'my' with this, it may not have an associate object header!!! */
//	if(0 == gl_set_value(my, (void *)((unsigned long long int)this + (unsigned long long int)prop->addr), valstr, prop)){
//		gl_error("csv_reader::read_prop ~ unable to set property \'%s\' to \'%s\'", propstr, valstr);
//		return 0;
//	}
	void *addr = (void *)((uint64)this + (uint64)prop->addr);
	if(prop->ptype == PT_double){
		if(1 != sscanf(valstr, "%lg", static_cast<double*>(addr))){
			gl_error(R"(csv_reader::read_prop ~ unable to set property '%s' to '%s')", propstr, valstr);
			/* TROUBLESHOOT
				The double parser was not able to convert the property value into a number.  Please
				review the input line for non-numeric characters and re-run GridLAB-D.
			*/
			return 0;
		}

	} else if(prop->ptype == PT_char32){
		strncpy((char *)addr, valstr, 32);
//	} else if(prop->ptype == PT_char256){
//		strncpy((char *)addr, valstr, 256);
	} else if(prop->ptype == PT_int32){
		if(1 != sscanf(valstr, "%ld", static_cast<long*>(addr))){
			gl_error(R"(csv_reader::read_prop ~ unable to set property '%s' to '%s')", propstr, valstr);
			/* TROUBLESHOOT
				The int parser was not able to convert the property value into a number.  Please
				review the input line for non-numeric characters and re-run GridLAB-D.
			*/
			return 0;
		}
	} else {
		gl_error("csv_reader::read_prop ~ unable to convert property \'%s\' due to type restrictions", propstr);
		/* TROUBLESHOOT
			This is a programming problem.  The property parser within the csv_reader is only able to
			properly handle char32, int32 and double properties.  Please contact matthew.hauer@pnl.gov for
			technical support.
		 */
		return 0;
	}
	return 1;
}

int csv_reader::read_header(char *line){
	struct cmnlist {
		char *name;
		PROPERTY *column;
		//struct cmnlist *next;
		std::unique_ptr<cmnlist> next; // Use unique_ptr for automatic memory management
	};
	char buffer[1024];
	int index = 0, start_idx = 0;
	int done = 0;
	int i = 0;
	PROPERTY* prop = 0;
	std::unique_ptr<struct cmnlist> first = nullptr;
	struct cmnlist *last = nullptr;  //, * temp = 0;
	std::unique_ptr<cmnlist> temp; // Use unique_ptr for automatic memory management

	// expected format: x,y,z\n

	memset(buffer, 0, 1024);
	strncpy(buffer, line, 1023);

	// split column header list
	while(index < 1024 && 0 == done){
		while(buffer[index] != 0 && buffer[index] != ',' && buffer[index] != '\n' && buffer[index] != '\r' && buffer[index] != '#'){
			++index;
		}
		if(buffer[index] == ','){
			buffer[index] = 0;
			++index;
		}
		if(buffer[index] == '\n' || buffer[index] == '\r' || buffer[index] == '#'){
			buffer[index] = 0;
		}

		//temp = (struct cmnlist *)malloc(sizeof(struct cmnlist));
		temp = std::make_unique<cmnlist>(); // Use unique_ptr for automatic memory management
		temp->name = buffer+start_idx;
		temp->column = prop;
		temp->next = 0;

		start_idx = index;
		++column_ct;

		if(first == 0){
			first = std::move(temp);
			last = first.get();
		} else {
			last->next = std::move(temp);
			last = temp.get();
		}

		if(buffer[index] == 0 || buffer[index] == '\n' || buffer[index] == '\r'){
			done = 1;
			break;
		}
	}

	//	find properties for each column header
	temp = std::move(first);
	//columns = (PROPERTY **)malloc(sizeof(PROPERTY *) * (size_t)column_ct);
	columns.resize(column_ct);

	while(temp != 0 && i < column_ct){
		temp->column = gl_find_property(weather::oclass, temp->name);
		if(temp->column == 0){
			gl_error("csv_reader::read_header ~ unable to find column property \'%s\''", temp->name);
			/* TROUBLESHOOT
				The specified property in the header was not found published by the weather
				class.  Please check the column header input and re-run GridLAB-D.
			*/
			return 0;
		}
		columns[i] = temp->column; // Move ownership to the vector
		//columns[i] = temp->column;
		temp = std::move(temp->next);
		++i;
	}
	return 1;
}



// When adding a new weather object
void csv_reader::add_weather(weather::unique_ptr_type new_weather) {
	if (!weather_root) {
		// First element
		weather_root = std::move(new_weather);
		weather_last = weather_root.get();
	}
	else {
		// Append to the end of the list
		weather_last->next = std::move(new_weather);
		weather_last = weather_last->next.get();
	}
}


csv_reader::~csv_reader() {
	weather_root.reset();
	weather_last = nullptr;
}


int csv_reader::read_line(char* line, int linenum) {
	std::vector<std::string> tokens;
	std::string line_str(line);
	std::istringstream iss(line_str);
	std::string token;

	// Split the line into tokens
	while (std::getline(iss, token, ',')) {
		token.erase(0, token.find_first_not_of(" \t\r\n"));
		token.erase(token.find_last_not_of(" \t\r\n") + 1);
		tokens.push_back(token);
	}

	if (tokens.empty()) {
		return 2; // blank line
	}

	// Use a raw pointer and manage memory explicitly
	auto sample = weather::unique_ptr_type(new weather());

	try {
		std::string timestamp_str = tokens[0];

		// More explicit timestamp parsing
		int parsed_year = 0, parsed_month = 0, parsed_day = 0,
			parsed_hour = 0, parsed_minute = 0, parsed_second = 0;
		char timezone[32] = { 0 };

		// Try parsing with different format specifiers
		if (sscanf(timestamp_str.c_str(), "%d-%d-%d %d:%d:%d %31[^ \t\n\r]",  //weather_dst.csv
			&parsed_year, &parsed_month, &parsed_day,
			&parsed_hour, &parsed_minute, &parsed_second,
			timezone) >= 6) {
			// Successfully parsed full timestamp with timezone
			sample->month = parsed_month;
			sample->day = parsed_day;
			sample->hour = parsed_hour;
			sample->minute = parsed_minute;
			sample->second = parsed_second;
		}
		else if (sscanf(timestamp_str.c_str(), "%d-%d-%d %d:%d:%d",
			&parsed_year, &parsed_month, &parsed_day,
			&parsed_hour, &parsed_minute, &parsed_second) >= 6) {
			// Successfully parsed full timestamp without timezone
			sample->month = parsed_month;
			sample->day = parsed_day;
			sample->hour = parsed_hour;
			sample->minute = parsed_minute;
			sample->second = parsed_second;
		}
		else if (sscanf(timestamp_str.c_str(), "%d:%d:%d",				//weather.csv
			&parsed_month, &parsed_day, &parsed_hour) == 3) {
			// Successfully parsed full timestamp without timezone
			sample->month = parsed_month;
			sample->day = parsed_day;
			sample->hour = parsed_hour;
			sample->minute = 0;
			sample->second = 0;
		}
		else if (sscanf(timestamp_str.c_str(), "%d:%d:%d:%d:%d",		//weather_elev.csv
			 &parsed_month, &parsed_day,
			&parsed_hour, &parsed_minute, &parsed_second) >= 5) {
			// Successfully parsed full timestamp without timezone
			//sample->month = parsed_month;
			//sample->day = parsed_day;
			sample->hour = parsed_hour;
			sample->minute = parsed_minute;
			sample->second = parsed_second;
		}
		else {
			// Fallback to timestamp conversion
			TIMESTAMP ts = callback->time.convert_to_timestamp(timestamp_str.c_str());
			DATETIME dt;
			dt.nanosecond = 0;

			if (ts != TS_INVALID && ts != TS_NEVER && callback->time.local_datetime(ts, &dt)) {
				sample->month = dt.month;
				sample->day = dt.day; 
				sample->hour = dt.hour;
				sample->minute = dt.minute;
				sample->second = dt.second;
			}
			else {
				gl_error("csv_reader::read_line ~ unable to read time string '%s'", timestamp_str.c_str());
				//delete sample;  // Clean up on error
				return 0;
			}
		}

		// Process remaining columns
		for (size_t col = 1; col < tokens.size() && col <= column_ct; ++col) {
			if (columns[col - 1]->ptype == PT_double) {
				const std::string& name = columns[col - 1]->name;
				double value;
				if (sscanf(tokens[col].c_str(), "%lg", &value) != 1) {
					gl_error("Unable to set value '%s' to double property '%s'",
						tokens[col].c_str(), name.c_str());
					//delete sample;  // Clean up on error
					return 0;
				}

				// Property mapping
				if (name == "temperature")
					sample->temperature = value;
				else if (name == "humidity")
					sample->humidity = value;
				else if (name == "solar_direct" || name == "solar_dir")
					sample->solar_dir = value;
				else if (name == "solar_diffuse" || name == "solar_diff")
					sample->solar_diff = value;
				else if (name == "pressure")
					sample->pressure = value;
				else if (name == "wind_speed")
					sample->wind_speed = value;
				else if (name == "solar_global")
					sample->solar_global = value;
				else {
					gl_error("Unknown property name '%s'", name.c_str());
					//delete sample;  // Clean up on error.  Important to handle this correctly.
					return 0;
				}
			}
		}
	}
	catch (const std::exception& e) {
		gl_error("Parsing error: %s", e.what());
		//delete sample;  // Clean up on exception
		return 0;
	}

	add_weather(std::move(sample));


	return 1;
}


TIMESTAMP csv_reader::get_data(TIMESTAMP t0, double *temp, double *humid, double *direct, double *diffuse, double *global, double *extra_global,  double *wind,double *winddir, double *opaque, double *total, double *rain, double *snow, double *pressure){
	DATETIME now, then;
//	TIMESTAMP until;
	int next_year = 0;
	int i = 0;
	int idx = index;
	int start = index;
	now.nanosecond = 0;
	then.nanosecond = 0;

	int localres;

	if(t0 < next_ts){ /* still good ~ go home */
		return -next_ts;
	}

	localres = gl_localtime(t0, &now); // error check

	gl_debug("csv_reader::get_data start");
	if(next_ts == 0){
		//	initialize to the correct index & next_ts
		DATETIME guess_dt;
		guess_dt.nanosecond = 0;
		TIMESTAMP guess_ts;
		int i;
#if 0
		/*	This method worked until it was realized that if there are January entries
		 *	at the end of a full-year would be caught by the going-backwards method.
		 *	This led to some strange results.
		 */
		for(i = 0; i < sample_ct; ++i){
			guess_dt.year = now.year;
			guess_dt.month = samples[sample_ct-i-1]->month;
			guess_dt.day = samples[sample_ct-i-1]->day;
			guess_dt.hour = samples[sample_ct-i-1]->hour;
			guess_dt.minute = samples[sample_ct-i-1]->minute;
			guess_dt.second = samples[sample_ct-i-1]->second;
			strcpy(guess_dt.tz, now.tz);
//			strcpy(guess_dt.tz, "GMT");
			guess_ts = (TIMESTAMP)gl_mktime(&guess_dt);

			if(guess_ts <= t0){
				break;
			}
		}
		index = sample_ct - i - 1;
#endif
		for(i = 0; i < sample_ct; ++i){
			guess_dt.year = now.year;
			guess_dt.month = samples[i]->month;
			guess_dt.day = samples[i]->day;
			guess_dt.hour = samples[i]->hour;
			guess_dt.minute = samples[i]->minute;
			guess_dt.second = samples[i]->second;
			strcpy(guess_dt.tz, now.tz);
//			strcpy(guess_dt.tz, "GMT");
			if(guess_dt.month == 2 && guess_dt.day == 29){
				if(!ISLEAPYEAR(now.year))
					continue; // skip leap days on non-leap years
			}
			guess_ts = (TIMESTAMP)gl_mktime(&guess_dt);

			if(guess_ts >= t0){
				i -= 1; // we want the sample *before* this one
				break;
			}
		}

		index = i;

		if(index > -1 && index < sample_ct){
			*temp = samples[index]->temperature;
			*humid = samples[index]->humidity;
			*direct = samples[index]->solar_dir;
			*diffuse = samples[index]->solar_diff;
			*global = samples[index]->solar_global;
			*extra_global = samples[index]->global_horizontal_extra;
			*wind = samples[index]->wind_speed;
      *winddir = samples[index]->wind_dir;
			*opaque = samples[index]-> opq_sky_cov;
			*total = samples[index]-> tot_sky_cov;
			*rain = samples[index]->rainfall;
			*snow = samples[index]->snowdepth;
			*pressure = samples[index]->pressure;
		} else { // somewhere between the last and the first element

			gl_debug("samples.size() = %zu", samples.size());
			if (!samples.empty()) {
				gl_debug("samples.back() = %p", samples.back());
			}

			if (samples.empty() || samples.back() == nullptr) {
				gl_error("csv_reader::get_data ~ last sample is null or missing");
				return -1;
			}


			if (!samples[sample_ct - 1]) {
				gl_error("csv_reader::get_data ~ sample at index %d is null", sample_ct - 1);
				return -1; // Or choose a fallback value
			}

			*temp = samples[sample_ct - 1]->temperature;
			*humid = samples[sample_ct - 1]->humidity;
			*direct = samples[sample_ct - 1]->solar_dir;
			*diffuse = samples[sample_ct - 1]->solar_diff;
			*global = samples[sample_ct - 1]->solar_global;
			*extra_global = samples[sample_ct - 1]->global_horizontal_extra;
			*wind = samples[sample_ct - 1]->wind_speed;
      *winddir = samples[sample_ct - 1]->wind_dir;
			*opaque = samples[sample_ct - 1]-> opq_sky_cov;
			*total = samples[sample_ct - 1]-> tot_sky_cov;
			*rain = samples[sample_ct - 1]->rainfall;
			*snow = samples[sample_ct - 1]->snowdepth;
			*pressure = samples[sample_ct - 1]->pressure;
		}

		then.year = now.year + (index+1 == sample_ct ? 1 : 0);
		then.month = samples[(index+1)%sample_ct]->month;
		then.day = samples[(index+1)%sample_ct]->day;
		then.hour = samples[(index+1)%sample_ct]->hour;
		then.minute = samples[(index+1)%sample_ct]->minute;
		then.second = samples[(index+1)%sample_ct]->second;
		then.nanosecond = 0;
		strcpy(then.tz, now.tz);

		next_ts = (TIMESTAMP)gl_mktime(&then);
		//next_ts = (TIMESTAMP)gl_mktime(&then);

		return -next_ts;
	}

	if(sample_ct == 1){ /* only one sample ~ ignore it and keep feeding the same data back, but in a year */
		next_ts += 365 * 24 * 3600;
		return -next_ts;
	}

	do{
		// should we roll the year over?
		if(index+1 >= sample_ct){
			index = 0;
		} else {
			++index;
		}

		if(index+1 == sample_ct){
			next_year = 1;
		} else {
			next_year = 0;
		}

		then.year = now.year + next_year;
		then.month = samples[(index+1)%sample_ct]->month;
		then.day = samples[(index+1)%sample_ct]->day;
		then.hour = samples[(index+1)%sample_ct]->hour;
		then.minute = samples[(index+1)%sample_ct]->minute;
		then.second = samples[(index+1)%sample_ct]->second;
		if(then.month == 2 && then.day == 29){
				if(!ISLEAPYEAR(then.year))
					continue; // skip leap days on non-leap years
			}
		strcpy(then.tz, now.tz);

		// next_ts is the time the current sample is overwritten by another sample.
		next_ts = (TIMESTAMP)gl_mktime(&then);
	} while (next_ts < t0 && index != start); // skip samples that try to reverse the time
	
	*temp = samples[index]->temperature;
	*humid = samples[index]->humidity;
	*direct = samples[index]->solar_dir;
	*diffuse = samples[index]->solar_diff;
	*global = samples[index]->solar_global;
	*extra_global = samples[index]->global_horizontal_extra;
	*wind = samples[index]->wind_speed;
  *winddir = samples[index]->wind_dir;
	*opaque = samples[index]->opq_sky_cov;
	*total = samples[index]->tot_sky_cov;
	*rain = samples[index]->rainfall;
	*snow = samples[index]->snowdepth;
	*pressure = samples[index]->pressure;

	// having found the index, update the data
	if(index == start){
		GL_THROW("something strange happened with the schedule in csv_reader");
		/*	TROUBLESHOOT
			An unidentified error occured while reading data and constructing the weather
			data schedule.  Please post a ticket detailing this event on the GridLAB-D
			SourceForge page.
		*/
	}
	
	gl_debug("csv_reader::get_data end");

	return -next_ts;
}

// EOF

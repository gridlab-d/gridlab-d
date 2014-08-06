/** $Id: csv_reader.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file csv_reader.cpp
	@author Matthew L. Hauer

 **/

#include "csv_reader.h"

CLASS *csv_reader::oclass = 0;

EXPORT int create_csv_reader(OBJECT **obj, OBJECT *parent){
	csv_reader *my = 0;
	*obj = gl_create_object(csv_reader::oclass);
	if(*obj != NULL){
		return 1;
	}
	//printf("create_csv_reader\n");
	return 0;	// don't want it to get called, but better to have it not be fatal
}

EXPORT int init_csv_reader(OBJECT **obj, OBJECT *parent){
	csv_reader *my = OBJECTDATA(obj,csv_reader);
	return 1; // let the climate object cause the file to open
}

/// Synchronize the cliamte object
EXPORT TIMESTAMP sync_csv_reader(OBJECT *obj, TIMESTAMP t0){
	return TS_NEVER; // really doesn't do anything
}

csv_reader::csv_reader(){
	memset(this, 0, sizeof(csv_reader));
}

csv_reader::csv_reader(MODULE *module){
	memset(this, 0, sizeof(csv_reader));
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"csv_reader",sizeof(csv_reader),NULL);
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
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(csv_reader));
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
	OBJECT *obj = OBJECTHDR(this);
	weather *wtr = 0;

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
	while(fgets(line, 1024, infile) > 0){
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

	/* move list into double pointer */
	samples = (weather**)malloc(sizeof(weather *) * (size_t) sample_ct);
	for(i = 0, wtr = weather_root; i < sample_ct && wtr != NULL; ++i, wtr=wtr->next){
		samples[i] = wtr;
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
	OBJECT *my = OBJECTHDR(this);
	char *split = strchr(line, '=');
	char propstr[256], valstr[256];
	PROPERTY *prop = 0;

	if(split == NULL){
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
		if(1 != sscanf(valstr, "%lg", addr)){
			gl_error("csv_reader::read_prop ~ unable to set property \'%s\' to \'%s\'", propstr, valstr);
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
	} else {
		gl_error("csv_reader::read_prop ~ unable to convert property \'%s\' due to type restrictions", propstr);
		/* TROUBLESHOOT
			This is a programming problem.  The property parser within the csv_reader is only able to
			properly handle char32 and double properties.  Please contact matthew.hauer@pnl.gov for
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
		struct cmnlist *next;
	};
	char buffer[1024];
	int index = 0, start_idx = 0;
	int done = 0;
	int i = 0;
	PROPERTY *prop = 0;
	struct cmnlist *first = 0, *last = 0, *temp = 0;

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

		temp = (struct cmnlist *)malloc(sizeof(struct cmnlist));
		temp->name = buffer+start_idx;
		temp->column = prop;
		temp->next = 0;

		start_idx = index;
		++column_ct;

		if(first == 0){
			first = last = temp;
		} else {
			last->next = temp;
			last = temp;
		}

		if(buffer[index] == 0 || buffer[index] == '\n' || buffer[index] == '\r'){
			done = 1;
			break;
		}
	}

	//	find properties for each column header
	temp = first;
	columns = (PROPERTY **)malloc(sizeof(PROPERTY *) * (size_t)column_ct);
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
		columns[i] = temp->column;
		temp = temp->next;
		++i;
	}
	return 1;
}

int csv_reader::read_line(char *line, int linenum){
	int done = 0;
	int col = 0;
	char buffer[1024];
	char *token = 0;
	weather *sample = 0;
	int64 t1, t2;
//	OBJECT *my = 0;

	strncpy(buffer, line, 1023);
	token = strtok(buffer, " ,\t\n\r");

	if(token == 0){
		return 2; // blank line 
	}

	sample = new weather();

	if(timefmt[0] == 0){
		TIMESTAMP ts = callback->time.convert_to_timestamp(token);
		DATETIME dt;
		dt.nanosecond = 0;
		if ( ts!=TS_INVALID && ts!=TS_NEVER && callback->time.local_datetime(ts,&dt) )
		{
			 sample->month = dt.month;
			 sample->day = dt.day;
			 sample->hour = dt.hour;
			 sample->minute = dt.minute;
			 sample->second = dt.second;
			 // IMPORTANT NOTE: if DST is not handled properly by sample, don't try to fix
			 // the problem here.  The weather class may need to be fixed so it uses UTC internally.
		}
		else if(sscanf(token, "%d:%d:%d:%d:%d", &sample->month, &sample->day, &sample->hour, &sample->minute, &sample->second) < 1){
			gl_error("csv_reader::read_line ~ unable to read time string \'%s\' with default format", token);
			/* TROUBLESHOOT
				The input timestamp could not be parsed.  Verify that all time strings are formatted
				as 'MM:dd:hh:mm:ss', 'MM:dd:hh:mm', 'MM:dd:hh', 'MM:dd', or 'MM'.
			*/
			delete sample;
			return 0;
		}
	} else {
		if(sscanf(token, timefmt.get_string(), &sample->month, &sample->day, &sample->hour, &sample->minute, &sample->second) < 1){
			gl_error("csv_reader::read_line ~ unable to read time string \'%s\' with format \'%s\'", token, timefmt.get_string());
			/* TROUBLESHOOT
				The input timestamp could not be parsed using the specified time format.  Please
				review the specified file's time format and input time strings.
			*/
			delete sample;
			return 0;
		}
	}

	if(weather_last != 0){
		t1 = weather_last->month * 31*24*60*60 +
			 weather_last->day * 24*60*60 +
			 weather_last->hour * 60*60 +
			 weather_last->minute * 60 +
			 weather_last->second;
		t2 = sample->month * 31*24*60*60 +
			 sample->day * 24*60*60 +
			 sample->hour * 60*60 +
			 sample->minute * 60 +
			 sample->second;
		if(t1 >= t2){
			gl_warning("csv_reader::read_line ~ sample on line %i does not advance in time and has been discarded", linenum);
			delete sample;
			return 2;
		}
	}

	while((token=strtok(NULL, ",\n\r")) != 0 && col < column_ct){
		if(columns[col]->ptype == PT_double){
			double *dptr = (double *)((uint64)(columns[col]->addr) + (uint64)(sample));
			if(sscanf(token, "%lg", dptr) != 1){
				gl_error("csv_reader::read_line ~ unable to set value \'%s\' to double property \'%s\'", token, columns[col]->name);
				/* TROUBLESHOOT
					The specified property value could not be parsed as a number.  Please check
					the CSV file for non-numeric characters in the data fields on that line.
				*/
				delete sample;
				return 0;
			}
		}
		++col;
	}

	if(weather_root == 0){
		weather_root = sample;
	} else {
		weather_last->next = sample;
	}
	weather_last = sample;

	return 1;
}

TIMESTAMP csv_reader::get_data(TIMESTAMP t0, double *temp, double *humid, double *direct, double *diffuse, double *global, double *wind, double *rain, double *snow, double *pressure){
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
			*wind = samples[index]->wind_speed;
			*rain = samples[index]->rainfall;
			*snow = samples[index]->snowdepth;
			*pressure = samples[index]->pressure;
		} else { // somewhere between the last and the first element
			*temp = samples[sample_ct - 1]->temperature;
			*humid = samples[sample_ct - 1]->humidity;
			*direct = samples[sample_ct - 1]->solar_dir;
			*diffuse = samples[sample_ct - 1]->solar_diff;
			*global = samples[sample_ct - 1]->solar_global;
			*wind = samples[sample_ct - 1]->wind_speed;
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
	*wind = samples[index]->wind_speed;
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

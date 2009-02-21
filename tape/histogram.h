// $Id: histogram.h 1182 2009-02-16 15:18:36Z mhauer $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _HISTOGRAM_H
#define _HISTOGRAM_H

#include <ctype.h>

#include "tape.h"

typedef struct s_histogram_bin {
	double low_val;
	double high_val;
	int low_inc;	/* 0 exclusive, nonzero inclusive */
	int high_inc;
} BIN;

class histogram
{
protected:
	FINDLIST *group_list;
	BIN *bin_list;
	int *binctr;
	TIMESTAMP next_count;
	TIMESTAMP next_sample;
public:
    static CLASS *oclass;
    static CLASS *pclass;
	static histogram *defaults;

	char1024 filename;
	char1024 group;
	char32 property;
	char1024 bins;
	int32 bin_count;
	double min;
	double max;
	int32 sampling_interval;	//	add values into bins
	int32 counting_interval;	//	dump the bins into a row
	int32 line_count;			//	number of lines to write
public:
	histogram(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	int isa(char *classname);
};

#endif // _HISTOGRAM_H_

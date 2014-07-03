// $Id: histogram.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _HISTOGRAM_H
#define _HISTOGRAM_H

#include <ctype.h>

#include "tape.h"

EXPORT void new_histogram(MODULE *mod);

#ifdef __cplusplus
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
	int *binctr;
	CPLPT comp_part;
	PROPERTY *prop_ptr;
	TAPEOPS *ops;

public:
    static CLASS *oclass;
    static CLASS *pclass;
	static histogram *defaults;

	TIMESTAMP next_count, t_count;
	TIMESTAMP next_sample, t_sample;

	char1024 filename;
	char1024 fname;
	char32 ftype;
	char32 mode;
	char8 flags;
	char1024 group;
	char256 property;
	char1024 bins;
	int32 bin_count;
	double min;
	double max;
	double sampling_interval;	//	add values into bins
	double counting_interval;	//	dump the bins into a row
	int32 limit;			//	number of lines to write
	
	FILETYPE type;
	union {
		FILE *fp;
		MEMORY *memory;
		void *tsp;
		/** add handles for other type of sources as needed */
	};

	BIN *bin_list;
public:
	histogram(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	int isa(char *classname);
protected:
	void test_for_complex(char *, char *);
	int feed_bins(OBJECT *);
};

#endif // C++

#endif // _HISTOGRAM_H_

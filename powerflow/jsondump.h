// $Id: jsondump.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _jsondump_H
#define _jsondump_H

#include "powerflow.h"
#include "power_metrics.h"
#include "line.h"
#include "fuse.h"
#include "recloser.h"
#include "sectionalizer.h"
#include "regulator.h"
#include "capacitor.h"
#include "switch_object.h"
#include "line_configuration.h"
#include "triplex_line_configuration.h"
#include "transformer.h"


class jsondump : public gld_object
{
public:
	int first_run;
	char32 group;
	char256 filename_dump_system;
	char256 filename_dump_reliability;
	bool write_system;
	bool write_reliability;
	bool write_per_unit;
	double system_VA_base;
	power_metrics **pPowerMetrics;
	fuse **pFuse;
	line **pOhLine;
	recloser **pRecloser;
	regulator **pRegulator;
	//link_object **pRelay;
	sectionalizer **pSectionalizer;
	//link_object **pSeriesReactor;
	switch_object **pSwitch;
	line **pTpLine;
	line **pUgLine;
	OBJECT **pLineConf;
	OBJECT **pTpLineConf;
	OBJECT **pTransConf;
	OBJECT **pRegConf;
	capacitor **pCapacitor;
	transformer **pTransformer;
	TIMESTAMP runtime;
	int32 runcount;
	gld::complex *node_voltage;
	double min_volt_value;
	double max_volt_value;

private:
	double get_double_value(OBJECT *obj, const char *name);
	gld::complex get_complex_value(OBJECT *obj, const char *name);
	set get_set_value(OBJECT *obj, const char *name);
	enumeration get_enum_value(OBJECT *obj, const char *name);
	OBJECT *get_object_value(OBJECT *obj, const char *name);

public:
	static CLASS *oclass;

public:
	jsondump(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t);
	STATUS finalize();
	int isa(char *classname);
	STATUS dump_system(void);
	STATUS dump_reliability(void);
};

#endif // _jsondump_H


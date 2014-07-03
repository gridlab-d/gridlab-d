// $Id: underground_line.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _UNDERGROUNDLINE_H
#define _UNDERGROUNDLINE_H

class underground_line : public line
{
public:
    static CLASS *oclass;
    static CLASS *pclass;

public:
	int init(OBJECT *parent);
	void recalc(void);
	underground_line(MODULE *mod);
	inline underground_line(CLASS *cl=oclass):line(cl){};
	int isa(char *classname);
	int create(void);
private:
	void test_phases(line_configuration *config, const char ph);
};

EXPORT int create_fault_ugline(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data);
EXPORT int fix_fault_ugline(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name, void* Extra_Data);
#endif // _UNDERGROUNDLINE_H

// $Id: overhead_line.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _OVERHEADLINE_H
#define _OVERHEADLINE_H

class overhead_line : public line
{
public:
    static CLASS *oclass;
    static CLASS *pclass;
public:
	void recalc(void);
public:
	int init(OBJECT *parent);
	overhead_line(MODULE *mod);
	inline overhead_line(CLASS *cl=oclass):line(cl){};
	int isa(char *classname);
	int create(void);
	double calc_image_dist(double dist1_to_e, double dist2_to_e, double dist1_to_2); //Calculates image distance
private:
	void test_phases(line_configuration *config, const char ph);
};
EXPORT int create_fault_ohline(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data);
EXPORT int fix_fault_ohline(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name, void *Extra_Data);
#endif // _OVERHEADLINE_H

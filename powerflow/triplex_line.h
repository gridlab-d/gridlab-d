// $Id: triplex_line.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRIPLEXLINE_H
#define _TRIPLEXLINE_H

class triplex_line : public line
{
public:
    static CLASS *oclass;
    static CLASS *pclass;
        
public:
	int init(OBJECT *parent);
	triplex_line(MODULE *mod);
	void recalc(void);
	inline triplex_line(CLASS *cl=oclass):line(cl){};
	int isa(char *classname);
	int create(void);
	void phase_conductor_checks(void);
};

EXPORT int recalc_triplex_line(OBJECT *obj);
#endif // _TRIPLEXLINE_H

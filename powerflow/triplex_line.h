// $Id: triplex_line.h 4738 2014-07-03 00:55:39Z dchassin $
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
};


#endif // _TRIPLEXLINE_H

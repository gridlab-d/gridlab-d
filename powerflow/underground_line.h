// $Id: underground_line.h 1182 2008-12-22 22:08:36Z dchassin $
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
	underground_line(MODULE *mod);
	inline underground_line(CLASS *cl=oclass):line(cl){};
	int isa(char *classname);
	int create(void);
};

#endif // _UNDERGROUNDLINE_H

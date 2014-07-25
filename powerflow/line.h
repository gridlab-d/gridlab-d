// $Id: line.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _LINE_H
#define _LINE_H

#define hasphase(ph) (SET_HAS(phase,PHASE##ph) | (phase_orig & PHASE_##ph))
#define set_hasphase(set,ph) (SET_HAS(set,ph))
#define PERMITIVITTY_AIR 0.014240	//microF/mile
#define PERMITIVITTY_FREE 0.01420	//microF/mile
#define EARTH_RESISTIVITY 100		//Ohm-meter

typedef struct {
	double continuous;
	double emergency;
} LINERATINGS;

extern bool show_matrix_values;

#include "powerflow.h"
#include "link.h"
#include "line_spacing.h"
#include "overhead_line_conductor.h"
#include "underground_line_conductor.h"
#include "line_configuration.h"
#include "triplex_line_configuration.h"
#include "triplex_line_conductor.h"

class line : public link_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
	
	OBJECT *configuration;
	double length;     // length of line in feet
	line(MODULE *mod);
	inline line(CLASS *cl=oclass):link_object(cl){};
	int init(OBJECT *parent);
	int isa(char *classname);
    int create(void);

protected:
	void load_matrix_based_configuration(complex Zabc_mat[3][3], complex Yabc_mat[3][3]);
	void recalc_line_matricies(complex Zabc_mat[3][3], complex Yabc_mat[3][3]);
};

#include "triplex_line.h"
#include "overhead_line.h"
#include "underground_line.h"

#endif // _LINE_H

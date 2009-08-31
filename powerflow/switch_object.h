/** $Id: switch_object.h,v 1.5 2008/02/04 23:08:12 d3p988 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file switch_object.h

	@note care should be taken when working with this class because \e switch
	is a reserved word in C/C++ and \p switch_object is used whenever that
	can cause problems.
 @{
 **/

#ifndef SWITCH_OBJECT_H
#define SWITCH_OBJECT_H

#include "powerflow.h"
#include "link.h"

class switch_object : public link
{
public:
	static CLASS *oclass;
	static CLASS *pclass;

public:
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	switch_object(MODULE *mod);
	inline switch_object(CLASS *cl=oclass):link(cl){};
	int isa(char *classname);

	TIMESTAMP prev_SW_time;	//Used to track switch opens/closes in NR.  Zeros end voltage on first run for other
};

#endif // SWITCH_OBJECT_H
/**@}**/

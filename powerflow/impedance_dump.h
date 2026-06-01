// $Id: impedance_dump.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _impedance_dump_H
#define _impedance_dump_H

#include "capacitor.h"
#include "line.h"
#include "node.h"
#include "powerflow.h"
#include "regulator.h"
#include "switch_object.h"
#include "transformer.h"

typedef enum
{
    IDM_RECT,
    IDM_POLAR
} IDMODE;

class impedance_dump : public gld_object
{
public:
    char32 group;
    char256 filename;
    TIMESTAMP runtime;
    int32 runcount;

public:
    static CLASS *oclass;

public:
    impedance_dump(MODULE *mod);
    int create(void);
    int init(OBJECT *parent);
    TIMESTAMP commit(TIMESTAMP t);
    static int isa(const char *classname);
    int dump(TIMESTAMP t);
    static gld::complex *get_complex(OBJECT *obj, const char *name);
};

#endif // _impedance_dump_H

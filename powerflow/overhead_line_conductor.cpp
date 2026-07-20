/** $Id: overhead_line_conductor.cpp 4738 2014-07-03 00:55:39Z dchassin $
        Copyright (C) 2008 Battelle Memorial Institute
        @file overhead_line_conductor.cpp
        @addtogroup overhead_line_conductor
        @ingroup line

        @{
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
using namespace std;

#include "line.h"

CLASS *overhead_line_conductor::oclass = nullptr;
CLASS *overhead_line_conductor::pclass = nullptr;

overhead_line_conductor::overhead_line_conductor(MODULE *mod)
    : powerflow_library(mod) {
  if (oclass == nullptr) {
    oclass = gl_register_class(mod, "overhead_line_conductor",
                               sizeof(overhead_line_conductor), 0x00);
    if (oclass == nullptr)
      throw "unable to register class overhead_line_conductor";
    else
      oclass->trl = TRL_PROVEN;

    if (gl_publish_variable(
            oclass, PT_double, "geometric_mean_radius[ft]",
            PADDR(geometric_mean_radius), PT_DESCRIPTION,
            "radius of the conductor", PT_double, "resistance[Ohm/mile]",
            PADDR(resistance), PT_DESCRIPTION,
            "resistance in Ohms/mile of the conductor", PT_double,
            "diameter[in]", PADDR(cable_diameter), PT_DESCRIPTION,
            "Diameter of line for capacitance calculations", PT_double,
            "rating.summer.continuous[A]", PADDR(summer.continuous),
            PT_DESCRIPTION, "Continuous summer amp rating", PT_double,
            "rating.summer.emergency[A]", PADDR(summer.emergency),
            PT_DESCRIPTION, "Emergency summer amp rating", PT_double,
            "rating.winter.continuous[A]", PADDR(winter.continuous),
            PT_DESCRIPTION, "Continuous winter amp rating", PT_double,
            "rating.winter.emergency[A]", PADDR(winter.emergency),
            PT_DESCRIPTION, "Emergency winter amp rating", nullptr) < 1)
      GL_THROW("unable to publish overhead_line_conductor properties in %s",
               __FILE__);
  }
}

int overhead_line_conductor::create(void) {
  int result = powerflow_library::create();

  cable_diameter = 0.0;
  geometric_mean_radius = resistance = 0.0;
  summer.continuous = winter.continuous = 1000;
  summer.emergency = winter.emergency = 2000;

  return result;
}

int overhead_line_conductor::init(OBJECT *parent) {
  OBJECT *obj_this = object_header(this);

#ifdef __APPLE__
  parent =
      obj_this
          ->parent; // AppleClang seems to have an issue with the parent pointer
#endif
  // Check resistance
  if (resistance == 0.0) {
    if (solver_method == SM_NR) {
      GL_THROW("overhead_line_conductor:%d - %s - NR: resistance is zero",
               get_id(), get_name());
      /*  TROUBLESHOOT
      The overhead_line_conductor has a resistance of zero.  This will cause
      problems with the Newton-Raphson solution.  Please put a valid resistance
      value.
      */
    } else // Assumes FBS
    {
      gl_warning("overhead_line_conductor:%d - %s - FBS: resistance is zero",
                 get_id(), get_name());
      /*  TROUBLESHOOT
      The overhead_line_conductor has a resistance of zero.  This will cause
      problems with the Newton-Raphson solver - if you intend to swap powerflow
      solvers, this must be fixed. Please put a valid resistance value if that
      is the case.
      */
    }
  }
  return 1;
}

int overhead_line_conductor::isa(char *classname) {
  return strcmp(classname, "overhead_line_conductor") == 0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: overhead_line_conductor
//////////////////////////////////////////////////////////////////////////

/**
 * REQUIRED: allocate and initialize an object.
 *
 * @param obj a pointer to a pointer of the last object in the list
 * @param parent a pointer to the parent of this object
 * @return 1 for a successfully created object, 0 for error
 */
EXPORT int create_overhead_line_conductor(OBJECT **obj, OBJECT *parent) {
  try {
    *obj = gl_create_object(overhead_line_conductor::oclass);
    if (*obj != nullptr) {
      overhead_line_conductor *my = object_data<overhead_line_conductor>(*obj);
      // gl_set_parent(*obj,parent);
      return my->create();
    } else
      return 0;
  }
  CREATE_CATCHALL(overhead_line_conductor);
}

EXPORT int init_overhead_line_conductor(OBJECT *obj) {
  try {
    overhead_line_conductor *my = object_data<overhead_line_conductor>(obj);
    return my->init(obj->parent);
  }
  INIT_CATCHALL(overhead_line_conductor);
}

static TIMESTAMP sync_overhead_line_conductor_impl(OBJECT *obj, TIMESTAMP t1,
                                                   PASSCONFIG pass) {
  return TS_NEVER;
}

#ifndef __APPLE__
extern "C" MODULE_API TIMESTAMP sync_overhead_line_conductor(OBJECT *obj,
                                                             TIMESTAMP t1,
                                                             PASSCONFIG pass) {
  return sync_overhead_line_conductor_impl(obj, t1, pass);
}
#else
extern "C" MODULE_API TIMESTAMP sync_overhead_line_conductor(OBJECT *obj, ...) {
  va_list args;
  va_start(args, obj);
  TIMESTAMP t1 = va_arg(args, TIMESTAMP);
  PASSCONFIG pass = va_arg(args, PASSCONFIG);
  va_end(args);
  return sync_overhead_line_conductor_impl(obj, t1, pass);
}
#endif

EXPORT int isa_overhead_line_conductor_impl(OBJECT *obj, char *classname) {
  return object_data<overhead_line_conductor>(obj)->isa(classname);
}

#ifndef __APPLE__
extern "C" MODULE_API int isa_overhead_line_conductor(OBJECT *obj, char *classname) {
  return isa_overhead_line_conductor_impl(obj, classname);
}
#else
extern "C" MODULE_API int isa_overhead_line_conductor(OBJECT *obj, ...) {
  va_list args;
  va_start(args, obj);
  char *classsname = va_arg(args, char *);
  va_end(args);
  return isa_overhead_line_conductor_impl(obj, classsname);
}
#endif

/**@}**/

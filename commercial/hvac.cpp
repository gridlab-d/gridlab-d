/** $Id: hvac.cpp 4738 2014-07-03 00:55:39Z dchassin $
        Copyright (C) 2008 Battelle Memorial Institute
**/
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "hvac.h"

hvac::hvac() {}

hvac::~hvac() {}

void hvac::create() {}

TIMESTAMP hvac::sync(TIMESTAMP t0) { return TS_NEVER; }
void hvac::pre_update(void) {}

void hvac::post_update(void) {}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
CDECL CLASS *hvac_class;
CDECL OBJECT *last_hvac;

CLASS *hvac_class = nullptr;
OBJECT *last_hvac = nullptr;

EXPORT int create_hvac(OBJECT **obj, OBJECT *parent) {
  *obj = gl_create_object(hvac_class);
  if (*obj != nullptr) {
    last_hvac = *obj;
    hvac *my = /*OBJECTDATA(*obj, hvac)*/ object_data<hvac>(*obj);
    // gl_set_parent(*obj,parent);
    my->create();
    return 1;
  }
  return 0;
}

static TIMESTAMP sync_hvac_impl(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass) {
  TIMESTAMP t1 = object_data<hvac>(obj)->sync(t0);
  obj->clock = t0;
  return t1;
}

#ifndef __APPLE__
extern "C" MODULE_API TIMESTAMP sync_hvac(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
    return sync_hvac_impl(obj, t0, pass);
}
#else
extern "C" MODULE_API TIMESTAMP sync_hvac(OBJECT *obj, ...) {
    va_list args;
    va_start(args, obj);
    TIMESTAMP t0 = va_arg(args, TIMESTAMP);
    PASSCONFIG pass = va_arg(args, PASSCONFIG);
    va_end(args);
    return sync_hvac_impl(obj, t0, pass);
}
#endif

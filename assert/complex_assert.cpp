/* complex_assert

   Very simple test that compares complex values to any corresponding complex
   value.  It breaks the tests down into a test for the real value and a test
   for the imagniary portion.  If either test fails at any time, it t.rows() a
   'zero' to the commit function and breaks the simulator out with a failure
   code.
*/

#include "gld_complex.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "complex_assert.h"

EXPORT_CREATE(complex_assert);
EXPORT_INIT(complex_assert);
EXPORT_COMMIT(complex_assert);
EXPORT_NOTIFY(complex_assert);

CLASS *complex_assert::oclass = nullptr;
static complex_assert defaults_storage; // POD storage for defaults
complex_assert *complex_assert::defaults = &defaults_storage;

complex_assert::complex_assert(MODULE *module) {
  if (oclass == nullptr) {
    // register to receive notice for first top down. bottom up, and second top
    // down synchronizations
    oclass = gl_register_class(module, "complex_assert", sizeof(complex_assert),
                               PC_AUTOLOCK | PC_OBSERVER);
    if (oclass == nullptr)
      throw "unable to register class complex_assert";
    else
      oclass->trl = TRL_PROVEN;

    if (gl_publish_variable(
            oclass,
            // TO DO:  publish your variables here
            PT_enumeration, "status", get_status_offset(), PT_DESCRIPTION,
            "Conditions for the assert checks", PT_KEYWORD, "ASSERT_TRUE",
            (enumeration)ASSERT_TRUE, PT_KEYWORD, "ASSERT_FALSE",
            (enumeration)ASSERT_FALSE, PT_KEYWORD, "ASSERT_NONE",
            (enumeration)ASSERT_NONE, PT_enumeration, "once", get_once_offset(),
            PT_DESCRIPTION, "Conditions for a single assert check", PT_KEYWORD,
            "ONCE_FALSE", (enumeration)ONCE_FALSE, PT_KEYWORD, "ONCE_TRUE",
            (enumeration)ONCE_TRUE, PT_KEYWORD, "ONCE_DONE",
            (enumeration)ONCE_DONE, PT_enumeration, "operation",
            get_operation_offset(), PT_DESCRIPTION,
            "Complex operation for the comparison", PT_KEYWORD, "FULL",
            (enumeration)FULL, PT_KEYWORD, "REAL", (enumeration)REAL,
            PT_KEYWORD, "IMAGINARY", (enumeration)IMAGINARY, PT_KEYWORD,
            "MAGNITUDE", (enumeration)MAGNITUDE, PT_KEYWORD, "ANGLE",
            (enumeration)ANGLE, // specify in radians
            PT_complex, "value", get_value_offset(), PT_DESCRIPTION,
            "Value to assert", PT_double, "within", get_within_offset(),
            PT_DESCRIPTION, "Tolerance for a successful assert", PT_char1024,
            "target", get_target_offset(), PT_DESCRIPTION,
            "Property to perform the assert upon", nullptr) < 1) {
      char msg[256];
      sprintf(msg, "unable to publish properties in %s", __FILE__);
      throw msg;
    }

    oclass->notify = notify_complex_assert;

    // status = ASSERT_TRUE;
    // within = 0.0;
    // value = 0.0;
    // once = ONCE_FALSE;
    // once_value = 0;
    // operation = FULL;

    // memcpy(this, defaults, sizeof(complex_assert));
  }
}

/* Object creation is called once for each object that is created by the core */
int complex_assert::create(void) {

  // memcpy(this, defaults, sizeof(complex_assert));

  status = defaults->status;
  within = defaults->within;
  value = defaults->value;
  once = defaults->once;
  once_value = defaults->once_value;
  operation = defaults->operation;
  strcpy(target, defaults->target);

  return 1; /* return 1 on success, 0 on failure */
}

int complex_assert::init(OBJECT *parent) {
  OBJECT *obj = object_header(this);

#ifdef __APPLE__
  parent = obj->parent;
#endif

  if (within <= 0.0) {
    throw "A non-positive value has been specified for within.";
    /*  TROUBLESHOOT
    Within is the range in which the check is being performed.  Please check to
    see that you have specified a value for "within" and it is positive.
    */
  }

  return 1;
}

TIMESTAMP complex_assert::commit(TIMESTAMP t1, TIMESTAMP t2) {
  gl_verbose("complex_assert::commit called for %s (operation=%d) on %s",
             get_target().c_str(), operation,
             get_parent() ? get_parent()->get_name() : "unknown");

  // Instead of checking global_modelname, check a local property
  bool is_error_test = false;

  // Try to determine if this is an error test based on the object or parent
  // name
  if (get_parent() && get_parent()->get_name()) {
    const char *parent_name = get_parent()->get_name();
    is_error_test = strstr(parent_name, "_err") != nullptr;
  }

  // handle once mode
  if (once == ONCE_TRUE) {
    once_value = value;
    once = ONCE_DONE;
  } else if (once == ONCE_DONE) {
    if (once_value.Re() == value.Re() && once_value.Im() == value.Im()) {
      gl_verbose("Assert skipped with ONCE logic");
      return TS_NEVER;
    } else {
      once_value = value;
    }
  }

  // get the target property
  // gld_property target_prop(get_parent(),get_target());
  gld_property target_prop(get_parent(), get_target().c_str());
  if (!target_prop.is_valid() || target_prop.get_type() != PT_complex) {
    gl_error("Specified target %s for %s is not valid.", get_target().c_str(),
             get_parent()->get_name());
    /*  TROUBLESHOOT
    Check to make sure the target you are specifying is a published variable for
    the object that you are pointing to.  Refer to the documentation of the
    command flag --modhelp, or check the wiki page to determine which variables
    can be published within the object you are pointing to with the assert
    function.
    */
    return TS_INVALID;
  }

  // test the target value
  gld::complex x;
  target_prop.getp(x);
  if (status == ASSERT_TRUE) {
    if (operation == FULL || operation == REAL || operation == IMAGINARY) {
      gld::complex error = x - value;
      double real_error = error.Re();
      double imag_error = error.Im();
      if ((_isnan(real_error) || fabs(real_error) > within) &&
          (operation == FULL || operation == REAL)) {
        gl_error("Assert failed on %s: real part of %s %g not within %f of "
                 "given value %g",
                 get_parent()->get_name(), get_target().c_str(), x.Re(), within,
                 value.Re());
        return TS_INVALID;
      }
      if ((_isnan(imag_error) || fabs(imag_error) > within) &&
          (operation == FULL || operation == IMAGINARY)) {
        gl_error("Assert failed on %s: imaginary part of %s %+gi not within %f "
                 "of given value %+gi",
                 get_parent()->get_name(), get_target().c_str(), x.Im(), within,
                 value.Im());
        return TS_INVALID;
      }
    } else if (operation == MAGNITUDE) {
      double magnitude_error = x.Mag() - value.Mag();
      if (_isnan(magnitude_error) || fabs(magnitude_error) > within) {
        gl_error("Assert failed on %s: Magnitude of %s (%g) not within %f of "
                 "given value %g",
                 get_parent()->get_name(), get_target().c_str(), x.Mag(),
                 within, value.Mag());
        return TS_INVALID;
      }
    } else if (get_operation() == ANGLE) {
      double angle_error = x.Arg() - value.Arg();
      if (_isnan(angle_error) || fabs(angle_error) > within) {
        gl_error("Assert failed on %s: Angle of %s (%g) not within %f of given "
                 "value %g",
                 get_parent()->get_name(), get_target().c_str(), x.Arg(),
                 within, value.Arg());

        // Check if this is an error test
        if (is_error_test) {
          gl_verbose("Expected failure in error test file");
          // Return TS_INVALID to signal failure without throwing exception
          return TS_INVALID;
        } else {
          // For regular tests that fail unexpectedly
          gl_error("Unexpected assertion failure in non-error test");
          // For non-error tests, still return TS_INVALID
          return TS_INVALID;
        }
      }
    }
    gl_verbose("Assert passed on %s", get_parent()->get_name());
    return TS_NEVER;
  } else if (get_status() == ASSERT_FALSE) {
    if (operation == FULL || operation == REAL || operation == IMAGINARY) {
      gld::complex error = x - value;
      double real_error = error.Re();
      double imag_error = error.Im();
      if ((_isnan(real_error) || fabs(real_error) < within) &&
          (operation == FULL || operation == REAL)) {
        gl_error("Assert failed on %s: real part of %s %g is within %f of "
                 "given value %g",
                 get_parent()->get_name(), get_target().c_str(), x.Re(), within,
                 value.Re());
        return TS_INVALID;
      }
      if ((_isnan(imag_error) || fabs(imag_error) < within) &&
          (operation == FULL || operation == IMAGINARY)) {
        gl_error("Assert failed on %s: imaginary part of %s %+gi is within %f "
                 "of given value %+gi",
                 get_parent()->get_name(), get_target().c_str(), x.Im(), within,
                 value.Im());
        return TS_INVALID;
      }
    } else if (operation == MAGNITUDE) {
      double magnitude_error = x.Mag() - value.Mag();
      if (_isnan(magnitude_error) || fabs(magnitude_error) < within) {
        gl_error("Assert failed on %s: Magnitude of %s %g is within %f of "
                 "given value %g",
                 get_parent()->get_name(), get_target().c_str(), x.Mag(),
                 within, value.Mag());
        return TS_INVALID;
      }
    } else if (get_operation() == ANGLE) {
      double angle_error = x.Arg() - value.Arg();
      if (_isnan(angle_error) || fabs(angle_error) < within) {
        gl_error("Assert failed on %s: Angle of %s %g is within %f of given "
                 "value %g",
                 get_parent()->get_name(), get_target().c_str(), x.Arg(),
                 within, value.Arg());
        // return 0;
        // Return TS_INVALID instead of 0 to indicate failure properly
        return TS_INVALID;
      }
    }
    gl_verbose("Assert passed on %s", get_parent()->get_name());
    return TS_NEVER;
  } else {
    gl_verbose("Assert test is not being run on %s", get_parent()->get_name());
    return TS_NEVER;
  }
}

int complex_assert::prenotify(PROPERTY *prop, char *value) {

  // printf("prenotify called for %s\n", prop ? prop->name : "(null)");

  // Only block or handle specific properties if needed
  // Otherwise, always return 1 (success)
  return 1;
}

int complex_assert::postnotify(PROPERTY *prop, char *value) {
  if (!prop || !prop->name)
    return 1; // don't fail on nulls
  if (once == ONCE_DONE && strcmp(prop->name, "value") == 0) {
    once = ONCE_TRUE;
  }
  return 1;
}

EXPORT SIMULATIONMODE update_complex_assert(OBJECT *obj, TIMESTAMP t0,
                                            unsigned int64 delta_time,
                                            unsigned long dt,
                                            unsigned int iteration_count_val) {
  char buff[128];
  char dateformat[16] = "";
  char error_output_buff[2048];
  char datebuff[128];
  /*complex_assert *da = OBJECTDATA(obj,complex_assert);*/
  complex_assert *da = object_data<complex_assert>(obj);

  DATETIME delta_dt_val;
  double del_clock;
  TIMESTAMP del_clock_int;
  int del_microseconds;
  gld::complex *x;

  if (da->get_once() == da->ONCE_TRUE) {
    da->set_once_value(da->get_value());
    da->set_once(da->ONCE_DONE);
  } else if (da->get_once() == da->ONCE_DONE) {
    if (da->get_once_value().Re() == da->get_value().Re() &&
        da->get_once_value().Im() == da->get_value().Im()) {
      gl_verbose("Assert skipped with ONCE logic");
      return SM_EVENT;
    } else {
      da->set_once_value(da->get_value());
    }
  }

  // Iteration checker - assert only valid on the first timestep
  if (iteration_count_val == 0) {
    // Skip first timestep of any delta iteration -- nature of delta means it
    // really isn't checking the right one
    if (delta_time >= dt) {
      // Get value
      // x = (complex*)gl_get_complex_by_name(obj->parent,da->get_target());
      x = (gld::complex *)gl_get_complex_by_name(obj->parent,
                                                 da->get_target().c_str());

      if (x == nullptr) {
        gl_error("Specified target %s for %s is not valid.",
                 da->get_target().c_str(), gl_name(obj->parent, buff, 64));
        /*  TROUBLESHOOT
        Check to make sure the target you are specifying is a published variable
        for the object that you are pointing to.  Refer to the documentation of
        the command flag --modhelp, or check the wiki page to determine which
        variables can be published within the object you are pointing to with
        the assert function.
        */
        return SM_ERROR;
      } else if (da->get_status() == da->ASSERT_TRUE) {
        if (da->get_operation() == da->FULL ||
            da->get_operation() == da->REAL ||
            da->get_operation() == da->IMAGINARY) {
          gld::complex error = *x - da->get_value();
          double real_error = error.Re();
          double imag_error = error.Im();
          if ((_isnan(real_error) || fabs(real_error) > da->get_within()) &&
              (da->get_operation() == da->FULL ||
               da->get_operation() == da->REAL)) {
            // Calculate time
            if (delta_time >= dt) // After first iteration
              del_clock =
                  (double)t0 + (double)(delta_time - dt) / (double)DT_SECOND;
            else // First second different, don't back out
              del_clock = (double)t0 + (double)(delta_time) / (double)DT_SECOND;

            del_clock_int = (TIMESTAMP)
                del_clock; /* Whole seconds - update from global clock because
                              we could be in delta for over 1 second */
            del_microseconds = (int)((del_clock - (int)(del_clock)) * 1000000 +
                                     0.5); /* microseconds roll-over - biased
                                              upward (by 0.5) */

            // Convert out
            gl_localtime(del_clock_int, &delta_dt_val);

            // Determine output format
            gl_global_getvar("dateformat", dateformat, sizeof(dateformat));

            // Output date appropriately
            if (strcmp(dateformat, "ISO") == 0)
              sprintf(datebuff,
                      "ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.year, delta_dt_val.month, delta_dt_val.day,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "US") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.month, delta_dt_val.day, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "EURO") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.day, delta_dt_val.month, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else
              sprintf(datebuff, "ERROR    %.09f : ", del_clock);

            // Actual error part
            sprintf(error_output_buff,
                    "Assert failed on %s - real part of %s (%g) not within %f "
                    "of given value %g",
                    gl_name(obj->parent, buff, 64), da->get_target().c_str(),
                    (*x).Re(), da->get_within(), da->get_value().Re());

            // Send it out
            gl_output("%s%s", datebuff, error_output_buff);

            return SM_ERROR;
          }
          if ((_isnan(imag_error) || fabs(imag_error) > da->get_within()) &&
              (da->get_operation() == da->FULL ||
               da->get_operation() == da->IMAGINARY)) {
            // Calculate time
            if (delta_time >= dt) // After first iteration
              del_clock =
                  (double)t0 + (double)(delta_time - dt) / (double)DT_SECOND;
            else // First second different, don't back out
              del_clock = (double)t0 + (double)(delta_time) / (double)DT_SECOND;

            del_clock_int = (TIMESTAMP)
                del_clock; /* Whole seconds - update from global clock because
                              we could be in delta for over 1 second */
            del_microseconds = (int)((del_clock - (int)(del_clock)) * 1000000 +
                                     0.5); /* microseconds roll-over - biased
                                              upward (by 0.5) */

            // Convert out
            gl_localtime(del_clock_int, &delta_dt_val);

            // Determine output format
            gl_global_getvar("dateformat", dateformat, sizeof(dateformat));

            // Output date appropriately
            if (strcmp(dateformat, "ISO") == 0)
              sprintf(datebuff,
                      "ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.year, delta_dt_val.month, delta_dt_val.day,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "US") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.month, delta_dt_val.day, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "EURO") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.day, delta_dt_val.month, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else
              sprintf(datebuff, "ERROR    %.09f : ", del_clock);

            // Actual error part
            sprintf(error_output_buff,
                    "Assert failed on %s - imaginary part of %s (%g) not "
                    "within %f of given value %g",
                    gl_name(obj->parent, buff, 64), da->get_target().c_str(),
                    (*x).Im(), da->get_within(), da->get_value().Im());

            // Send it out
            gl_output("%s%s", datebuff, error_output_buff);

            return SM_ERROR;
          }
        } else if (da->get_operation() == da->MAGNITUDE) {
          double magnitude_error = (*x).Mag() - da->get_value().Mag();
          if (_isnan(magnitude_error) ||
              fabs(magnitude_error) > da->get_within()) {
            // Calculate time
            if (delta_time >= dt) // After first iteration
              del_clock =
                  (double)t0 + (double)(delta_time - dt) / (double)DT_SECOND;
            else // First second different, don't back out
              del_clock = (double)t0 + (double)(delta_time) / (double)DT_SECOND;

            del_clock_int = (TIMESTAMP)
                del_clock; /* Whole seconds - update from global clock because
                              we could be in delta for over 1 second */
            del_microseconds = (int)((del_clock - (int)(del_clock)) * 1000000 +
                                     0.5); /* microseconds roll-over - biased
                                              upward (by 0.5) */

            // Convert out
            gl_localtime(del_clock_int, &delta_dt_val);

            // Determine output format
            gl_global_getvar("dateformat", dateformat, sizeof(dateformat));

            // Output date appropriately
            if (strcmp(dateformat, "ISO") == 0)
              sprintf(datebuff,
                      "ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.year, delta_dt_val.month, delta_dt_val.day,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "US") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.month, delta_dt_val.day, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "EURO") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.day, delta_dt_val.month, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else
              sprintf(datebuff, "ERROR    %.09f : ", del_clock);

            // Actual error part
            sprintf(error_output_buff,
                    "Assert failed on %s - Magnitude of %s (%g) not within %f "
                    "of given value %g",
                    gl_name(obj->parent, buff, 64), da->get_target().c_str(),
                    (*x).Mag(), da->get_within(), da->get_value().Mag());

            // Send it out
            gl_output("%s%s", datebuff, error_output_buff);

            return SM_ERROR;
          }
        } else if (da->get_operation() == da->ANGLE) {
          double angle_error = (*x).Arg() - da->get_value().Arg();
          if (_isnan(angle_error) || fabs(angle_error) > da->get_within()) {
            // Calculate time
            if (delta_time >= dt) // After first iteration
              del_clock =
                  (double)t0 + (double)(delta_time - dt) / (double)DT_SECOND;
            else // First second different, don't back out
              del_clock = (double)t0 + (double)(delta_time) / (double)DT_SECOND;

            del_clock_int = (TIMESTAMP)
                del_clock; /* Whole seconds - update from global clock because
                              we could be in delta for over 1 second */
            del_microseconds = (int)((del_clock - (int)(del_clock)) * 1000000 +
                                     0.5); /* microseconds roll-over - biased
                                              upward (by 0.5) */

            // Convert out
            gl_localtime(del_clock_int, &delta_dt_val);

            // Determine output format
            gl_global_getvar("dateformat", dateformat, sizeof(dateformat));

            // Output date appropriately
            if (strcmp(dateformat, "ISO") == 0)
              sprintf(datebuff,
                      "ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.year, delta_dt_val.month, delta_dt_val.day,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "US") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.month, delta_dt_val.day, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "EURO") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.day, delta_dt_val.month, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else
              sprintf(datebuff, "ERROR    %.09f : ", del_clock);

            // Actual error part
            sprintf(error_output_buff,
                    "Assert failed on %s - Angle of %s (%g) not within %f of "
                    "given value %g",
                    gl_name(obj->parent, buff, 64), da->get_target().c_str(),
                    (*x).Arg(), da->get_within(), da->get_value().Arg());

            // Send it out
            gl_output("%s%s", datebuff, error_output_buff);

            return SM_ERROR;
          }
        }
        gl_verbose("Assert passed on %s", gl_name(obj->parent, buff, 64));
        return SM_EVENT;
      } else if (da->get_status() == da->ASSERT_FALSE) {
        if (da->get_operation() == da->FULL ||
            da->get_operation() == da->REAL ||
            da->get_operation() == da->IMAGINARY) {
          gld::complex error = *x - da->get_value();
          double real_error = error.Re();
          double imag_error = error.Im();
          if ((_isnan(real_error) || fabs(real_error) < da->get_within()) &&
              (da->get_operation() == da->FULL ||
               da->get_operation() == da->REAL)) {
            // Calculate time
            if (delta_time >= dt) // After first iteration
              del_clock =
                  (double)t0 + (double)(delta_time - dt) / (double)DT_SECOND;
            else // First second different, don't back out
              del_clock = (double)t0 + (double)(delta_time) / (double)DT_SECOND;

            del_clock_int = (TIMESTAMP)
                del_clock; /* Whole seconds - update from global clock because
                              we could be in delta for over 1 second */
            del_microseconds = (int)((del_clock - (int)(del_clock)) * 1000000 +
                                     0.5); /* microseconds roll-over - biased
                                              upward (by 0.5) */

            // Convert out
            gl_localtime(del_clock_int, &delta_dt_val);

            // Determine output format
            gl_global_getvar("dateformat", dateformat, sizeof(dateformat));

            // Output date appropriately
            if (strcmp(dateformat, "ISO") == 0)
              sprintf(datebuff,
                      "ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.year, delta_dt_val.month, delta_dt_val.day,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "US") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.month, delta_dt_val.day, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "EURO") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.day, delta_dt_val.month, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else
              sprintf(datebuff, "ERROR    %.09f : ", del_clock);

            // Actual error part
            sprintf(error_output_buff,
                    "Assert failed on %s - real part of %s (%g) not within %f "
                    "of given value %g",
                    gl_name(obj->parent, buff, 64), da->get_target().c_str(),
                    (*x).Re(), da->get_within(), da->get_value().Re());

            // Send it out
            gl_output("%s%s", datebuff, error_output_buff);

            return SM_ERROR;
          }
          if ((_isnan(imag_error) || fabs(imag_error) < da->get_within()) &&
              (da->get_operation() == da->FULL ||
               da->get_operation() == da->IMAGINARY)) {
            // Calculate time
            if (delta_time >= dt) // After first iteration
              del_clock =
                  (double)t0 + (double)(delta_time - dt) / (double)DT_SECOND;
            else // First second different, don't back out
              del_clock = (double)t0 + (double)(delta_time) / (double)DT_SECOND;

            del_clock_int = (TIMESTAMP)
                del_clock; /* Whole seconds - update from global clock because
                              we could be in delta for over 1 second */
            del_microseconds = (int)((del_clock - (int)(del_clock)) * 1000000 +
                                     0.5); /* microseconds roll-over - biased
                                              upward (by 0.5) */

            // Convert out
            gl_localtime(del_clock_int, &delta_dt_val);

            // Determine output format
            gl_global_getvar("dateformat", dateformat, sizeof(dateformat));

            // Output date appropriately
            if (strcmp(dateformat, "ISO") == 0)
              sprintf(datebuff,
                      "ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.year, delta_dt_val.month, delta_dt_val.day,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "US") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.month, delta_dt_val.day, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "EURO") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.day, delta_dt_val.month, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else
              sprintf(datebuff, "ERROR    %.09f : ", del_clock);

            // Actual error part
            sprintf(error_output_buff,
                    "Assert failed on %s - imaginary part of %s (%g) not "
                    "within %f of given value %g",
                    gl_name(obj->parent, buff, 64), da->get_target().c_str(),
                    (*x).Im(), da->get_within(), da->get_value().Im());

            // Send it out
            gl_output("%s%s", datebuff, error_output_buff);

            return SM_ERROR;
          }
        } else if (da->get_operation() == da->MAGNITUDE) {
          double magnitude_error = (*x).Mag() - da->get_value().Mag();
          if (_isnan(magnitude_error) ||
              fabs(magnitude_error) < da->get_within()) {
            // Calculate time
            if (delta_time >= dt) // After first iteration
              del_clock =
                  (double)t0 + (double)(delta_time - dt) / (double)DT_SECOND;
            else // First second different, don't back out
              del_clock = (double)t0 + (double)(delta_time) / (double)DT_SECOND;

            del_clock_int = (TIMESTAMP)
                del_clock; /* Whole seconds - update from global clock because
                              we could be in delta for over 1 second */
            del_microseconds = (int)((del_clock - (int)(del_clock)) * 1000000 +
                                     0.5); /* microseconds roll-over - biased
                                              upward (by 0.5) */

            // Convert out
            gl_localtime(del_clock_int, &delta_dt_val);

            // Determine output format
            gl_global_getvar("dateformat", dateformat, sizeof(dateformat));

            // Output date appropriately
            if (strcmp(dateformat, "ISO") == 0)
              sprintf(datebuff,
                      "ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.year, delta_dt_val.month, delta_dt_val.day,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "US") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.month, delta_dt_val.day, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "EURO") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.day, delta_dt_val.month, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else
              sprintf(datebuff, "ERROR    %.09f : ", del_clock);

            // Actual error part
            sprintf(error_output_buff,
                    "Assert failed on %s - Magnitude of %s (%g) not within %f "
                    "of given value %g",
                    gl_name(obj->parent, buff, 64), da->get_target().c_str(),
                    (*x).Mag(), da->get_within(), da->get_value().Mag());

            // Send it out
            gl_output("%s%s", datebuff, error_output_buff);

            return SM_ERROR;
          }
        } else if (da->get_operation() == da->ANGLE) {
          double angle_error = (*x).Arg() - da->get_value().Arg();
          if (_isnan(angle_error) || fabs(angle_error) < da->get_within()) {
            // Calculate time
            if (delta_time >= dt) // After first iteration
              del_clock =
                  (double)t0 + (double)(delta_time - dt) / (double)DT_SECOND;
            else // First second different, don't back out
              del_clock = (double)t0 + (double)(delta_time) / (double)DT_SECOND;

            del_clock_int = (TIMESTAMP)
                del_clock; /* Whole seconds - update from global clock because
                              we could be in delta for over 1 second */
            del_microseconds = (int)((del_clock - (int)(del_clock)) * 1000000 +
                                     0.5); /* microseconds roll-over - biased
                                              upward (by 0.5) */

            // Convert out
            gl_localtime(del_clock_int, &delta_dt_val);

            // Determine output format
            gl_global_getvar("dateformat", dateformat, sizeof(dateformat));

            // Output date appropriately
            if (strcmp(dateformat, "ISO") == 0)
              sprintf(datebuff,
                      "ERROR    [%04d-%02d-%02d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.year, delta_dt_val.month, delta_dt_val.day,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "US") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.month, delta_dt_val.day, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else if (strcmp(dateformat, "EURO") == 0)
              sprintf(datebuff,
                      "ERROR    [%02d-%02d-%04d %02d:%02d:%02d.%.06d %s] : ",
                      delta_dt_val.day, delta_dt_val.month, delta_dt_val.year,
                      delta_dt_val.hour, delta_dt_val.minute,
                      delta_dt_val.second, del_microseconds, delta_dt_val.tz);
            else
              sprintf(datebuff, "ERROR    %.09f : ", del_clock);

            // Actual error part
            sprintf(error_output_buff,
                    "Assert failed on %s - Angle of %s (%g) not within %f of "
                    "given value %g",
                    gl_name(obj->parent, buff, 64), da->get_target().c_str(),
                    (*x).Arg(), da->get_within(), da->get_value().Arg());

            // Send it out
            gl_output("%s%s", datebuff, error_output_buff);

            return SM_ERROR;
          }
        }
        gl_verbose("Assert passed on %s", gl_name(obj->parent, buff, 64));
        return SM_EVENT;
      } else {
        gl_verbose("Assert test is not being run on %s",
                   gl_name(obj->parent, buff, 64));
        return SM_EVENT;
      }
    } else // First timestep, just proceed
      return SM_EVENT;
  } else // Iteration, so don't care
    return SM_EVENT;
}

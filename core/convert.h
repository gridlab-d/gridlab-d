/** $Id: convert.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file convert.h
	@addtogroup convert
 @{
 **/

#ifndef _CONVERT_H
#define _CONVERT_H

#include "class.h"
#include "complex.h"

#ifdef __cplusplus
extern "C" {
#endif

int convert_from_void(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_void(const char *buffer, void *data, PROPERTY *prop);
int convert_from_double(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_double(const char *buffer, void *data, PROPERTY *prop);
int convert_from_complex(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_complex(const char *buffer, void *data, PROPERTY *prop);
int convert_from_enumeration(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_enumeration(const char *buffer, void *data, PROPERTY *prop);
int convert_from_set(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_set(const char *buffer, void *data, PROPERTY *prop);
int convert_from_int16(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_int16(const char *buffer, void *data, PROPERTY *prop);
int convert_from_int32(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_int32(const char *buffer, void *data, PROPERTY *prop);
int convert_from_int64(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_int64(const char *buffer, void *data, PROPERTY *prop);
int convert_from_char8(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_char8(const char *buffer, void *data, PROPERTY *prop);
int convert_from_char32(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_char32(const char *buffer, void *data, PROPERTY *prop);
int convert_from_char256(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_char256(const char *buffer, void *data, PROPERTY *prop);
int convert_from_char1024(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_char1024(const char *buffer, void *data, PROPERTY *prop);
int convert_from_object(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_object(const char *buffer, void *data, PROPERTY *prop);
int convert_from_delegated(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_delegated(const char *buffer, void *data, PROPERTY *prop);
int convert_from_boolean(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_boolean(const char *buffer, void *data, PROPERTY *prop);
int convert_from_timestamp_stub(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_timestamp_stub(const char *buffer, void *data, PROPERTY *prop);
int convert_from_double_array(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_double_array(const char *buffer, void *data, PROPERTY *prop);
int convert_from_complex_array(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_complex_array(const char *buffer, void *data, PROPERTY *prop);

int convert_unit_double(char *buffer,char *unit, double *data);
int convert_unit_complex(char *buffer,char *unit, complex *data);

int convert_from_real(char *a, int b, void *c, PROPERTY *d);
int convert_to_real(const char *a, void *b, PROPERTY *c);
int convert_from_float(char *a, int b, void *c, PROPERTY *d);
int convert_to_float(const char *a, void *b, PROPERTY *c);

int convert_from_struct(char *buffer, size_t len, void *data, PROPERTY *prop);
int convert_to_struct(const char *buffer, void *data, PROPERTY *prop);

#ifdef __cplusplus
}
#endif

#endif
/**@}**/

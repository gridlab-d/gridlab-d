/** $Id: convert.h 1182 2008-12-22 22:08:36Z dchassin $
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
int convert_to_void(char *buffer, void *data, PROPERTY *prop);
int convert_from_double(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_double(char *buffer, void *data, PROPERTY *prop);
int convert_from_complex(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_complex(char *buffer, void *data, PROPERTY *prop);
int convert_from_enumeration(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_enumeration(char *buffer, void *data, PROPERTY *prop);
int convert_from_set(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_set(char *buffer, void *data, PROPERTY *prop);
int convert_from_int16(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_int16(char *buffer, void *data, PROPERTY *prop);
int convert_from_int32(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_int32(char *buffer, void *data, PROPERTY *prop);
int convert_from_int64(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_int64(char *buffer, void *data, PROPERTY *prop);
int convert_from_char8(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_char8(char *buffer, void *data, PROPERTY *prop);
int convert_from_char32(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_char32(char *buffer, void *data, PROPERTY *prop);
int convert_from_char256(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_char256(char *buffer, void *data, PROPERTY *prop);
int convert_from_char1024(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_char1024(char *buffer, void *data, PROPERTY *prop);
int convert_from_object(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_object(char *buffer, void *data, PROPERTY *prop);
int convert_from_delegated(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_delegated(char *buffer, void *data, PROPERTY *prop);
int convert_from_boolean(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_boolean(char *buffer, void *data, PROPERTY *prop);
int convert_from_timestamp_stub(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_timestamp_stub(char *buffer, void *data, PROPERTY *prop);
int convert_from_double_array(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_double_array(char *buffer, void *data, PROPERTY *prop);
int convert_from_complex_array(char *buffer, int size, void *data, PROPERTY *prop);
int convert_to_complex_array(char *buffer, void *data, PROPERTY *prop);

int convert_unit_double(char *buffer,char *unit, double *data);
int convert_unit_complex(char *buffer,char *unit, complex *data);

#ifdef __cplusplus
}
#endif

#endif
/**@}**/

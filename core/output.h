/** $Id: output.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file output.h
	@addtogroup output
 @{
 **/

#ifndef _ERROR_H
#define _ERROR_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "timestamp.h"

typedef int (*PRINTFUNCTION)(char *,...);

typedef enum {FS_IN = 0, FS_STD = 1, FS_ERR = 2} FILESTREAM;

#ifdef __cplusplus
extern "C" {
#endif

PRINTFUNCTION output_set_stdout(PRINTFUNCTION call);
PRINTFUNCTION output_set_stderr(PRINTFUNCTION call);

int output_init(int argc, char *argv[]);
void output_cleanup(void);

void output_prefix_enable(void);
void output_both_stdout();
FILE *output_set_stream(FILESTREAM fs, FILE *newfp);
FILE* output_redirect(char *name, char *path);
FILE* output_redirect_stream(char *name, FILE *fp);
int output_fatal(const char *format,...);
int output_error(const char *format,...);
int output_error_raw(const char *format,...);
int output_warning(const char *format,...);
int output_debug(const char *format,...);
int output_verbose(const char *format,...);
int output_message(const char *format,...);
int output_raw(const char *format,...);
int output_test(const char *format,...);
int output_progress(void);
int output_profile(const char *format,...);

int output_notify_error(void (*)(void));

void output_set_time_context(TIMESTAMP ts);
void output_set_delta_time_context(TIMESTAMP ts, DELTAT delta_ts);
char *output_get_time_context(void);

int output_xsd(char *spec);
int output_xsl(char *fname, int n_mods, char *p_mods[]);

#ifdef __cplusplus
}
#endif

#endif

/**@}*/

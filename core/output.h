/** $Id: output.h 1182 2008-12-22 22:08:36Z dchassin $
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

typedef int (*PRINTFUNCTION)(char *,...);

typedef enum {FS_IN = 0, FS_STD = 1, FS_ERR = 2} FILESTREAM;

#ifdef __cplusplus
extern "C" {
#endif

PRINTFUNCTION output_set_stdout(PRINTFUNCTION call);
PRINTFUNCTION output_set_stderr(PRINTFUNCTION call);

void output_both_stdout();
FILE *output_set_stream(FILESTREAM fs, FILE *newfp);
FILE* output_redirect(char *name, char *path);
FILE* output_redirect_stream(char *name, FILE *fp);
int output_fatal(char *format,...);
int output_error(char *format,...);
int output_warning(char *format,...);
int output_debug(char *format,...);
int output_verbose(char *format,...);
int output_message(char *format,...);
int output_raw(char *format,...);
int output_test(char *format,...);
int output_progress(void);
int output_profile(char *format,...);

int output_notify_error(void (*)(void));

int output_xsd(char *spec);
int output_xsl(char *fname, int n_mods, char *p_mods[]);

#ifdef __cplusplus
}
#endif

#endif

/**@}*/

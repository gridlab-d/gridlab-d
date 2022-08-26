/* module.h
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#ifndef _MODULE_H
#define _MODULE_H

#include <stdio.h>
#include <float.h>

#include "object.h"
#include "gui.h"
#include "transform.h"
#include "stream.h"
#include "test_callbacks.h"

struct s_module_list {
	void *hLib;
	unsigned int id;
	char name[1024];
	CLASS *oclass;
	unsigned short major;
	unsigned short minor;
	void* (*getvar)(const char *varname,char *value,unsigned int size);
	int (*setvar)(const char *varname,char *value);
	int (*import_file)(const char *file);
	int (*export_file)(const char *file);
	int (*check)();
	/* deltamode */
	unsigned long (*deltadesired)(DELTAMODEFLAGS*);
	unsigned long (*preupdate)(void*,int64,unsigned int64);
	SIMULATIONMODE (*interupdate)(void*,int64,unsigned int64,unsigned long,unsigned int);
	SIMULATIONMODE (*deltaClockUpdate)(void *, double, unsigned long, SIMULATIONMODE);
	STATUS (*postupdate)(void*,int64,unsigned int64);
	/* clock hook*/
	TIMESTAMP (*clockupdate)(TIMESTAMP *);
	int (*cmdargs)(int,char**);
	int (*kmldump)(int(*)(const char*,...),OBJECT*);
	void (*test)(int argc, char *argv[]);	
	MODULE *(*subload)(char *, MODULE **, CLASS **, int, char **);
	PROPERTY *globals;
	void (*term)(void);
#ifndef STREAM_MODULE
	STREAMCALL stream;
#else
	void *stream;
#endif
	struct s_module_list *next;
}; /* MODULE */

#ifndef _MODULE_DEFINED_
#define _MODULE_DEFINED_
typedef struct s_module_list MODULE;
#endif

typedef CLASS *(*LIBINIT)(const CALLBACKS*,void*,int,char*[]);

#ifdef __cplusplus
extern "C" {
#endif
	int module_get_exe_path(char *buf, int len);
	int module_get_path(char *buf, int len, MODULE *mod);
	MODULE *module_find(const char *module_name);
	MODULE *module_load(const char *file, int argc, char *argv[]);
	void module_list(void);
	size_t module_getcount(void);
	void* module_getvar(MODULE *mod, const char *varname, char *value, unsigned int size);
	double *module_getvar_addr(MODULE *mod, const char *varname);
	int module_depends(const char *name, unsigned char major, unsigned char minor, unsigned short build);
	int module_setvar(MODULE *mod, const char *varname, char *value);
	int module_import(MODULE *mod, const char *filename);
	int module_export(MODULE *mod, const char *filename);
	int module_check(MODULE *mod);
	int module_checkall();
	int module_saveall(FILE *fp);
	int module_saveall_xml(FILE *fp);
	int module_dumpall();
	void module_libinfo(const char *module_name);
	const char *module_find_transform_function(TRANSFORMFUNCTION function);
#ifndef _NO_CPPUNIT
	int module_test(TEST_CALLBACKS *callbacks,int argc,char* argv[]);
#endif
	int module_cmdargs(int argc, char **argv);
	int module_saveobj_xml(FILE *fp, MODULE *mod);
	MODULE *module_get_first(void);
	void *module_malloc(size_t size);
	void module_free(void *ptr);

//#ifndef __MINGW32__
	// added in module.c because it has WIN32 API
	void sched_init(int readonly);
	void sched_clear(void);
	void sched_print(int flags);
	void sched_update(TIMESTAMP clock, enumeration status);
	void sched_pkill(pid_t pid);
	void sched_controller(void);
	unsigned short sched_get_cpuid(unsigned short n);
	pid_t sched_get_procid();
//#endif

	int module_load_function_list(char *libname, char *fnclist);
	TRANSFORMFUNCTION module_get_transform_function(const char *function);

	int module_compile(const char *name, const char *code, int flags, const char *prefix, const char *file, int line);
	void module_profiles(void);
	CALLBACKS *module_callbacks(void);
	void module_termall(void);
	MODULE *module_get_next(MODULE*);

#ifdef __cplusplus
}
#endif

#endif

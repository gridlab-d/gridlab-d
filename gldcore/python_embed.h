// python.h

#ifndef _PYTHON_H
#define _PYTHON_H

//#if ! defined _GLDCORE_H && ! defined _GRIDLABD_H
//#error "this header may only be included from gldcore.h or gridlabd.h"
//#endif

#include <Python.h>
#include "structmember.h"

#ifdef DEBUG_REFTRACE // enable this to trace Py_INCREF/Py_DECREF calls

#warning DEBUG_REFTRACE is enabled

#undef Py_INCREF
#define Py_INCREF(X) fprintf(stderr,__FILE__ ":%d: warning: Py_INCREF(" #X "=%p) -> %d\n", __LINE__, X, (int)++((X)->ob_refcnt))

#undef Py_DECREF
#define Py_DECREF(X) fprintf(stderr,__FILE__ ":%d: warning: Py_DECREF(" #X "=%p) -> %d\n", __LINE__, X, (int)--((X)->ob_refcnt))

#endif

PyMODINIT_FUNC PyInit_gridlabd(void);

void python_embed_init(int argc, const char *argv[]);
void *python_loader_init(int argc, const char **argv);
void python_embed_term();
PyObject *python_embed_import(const char *module, const char *path=NULL);
bool python_embed_call(PyObject *pModule, const char *name, const char *vargsfmt, va_list varargs, void *result);
std::string python_eval(const char *command);
bool python_parser(const char *line=NULL, void *context = NULL);
void python_traceback(const char *command);

// forward declarations to gldcore/link/python.c
MODULE *python_module_load(const char *, int, const char *[]);

// Function: convert_from_double
DEPRECATED int convert_from_python(char *buffer, int size, void *data, PROPERTY *prop);

// Function: convert_to_double
DEPRECATED int convert_to_python(const char *buffer, void *data, PROPERTY *prop);

DEPRECATED int initial_from_python(char *buffer, int size, void *data, PROPERTY *prop);

DEPRECATED int python_create(void *ptr);

double python_get_part(void *c, const char *name);

inline int PyDict_SetItemString(PyObject* obj, const char *key, const char *value) 
{
	PyObject *item = Py_BuildValue("s",value);
	int rc = PyDict_SetItemString(obj,key,item);
	Py_DECREF(item);
	return rc;
}

inline int PyDict_SetItemString(PyObject* obj, const char *key, int value) 
{
	PyObject *item = Py_BuildValue("i",value);
	int rc = PyDict_SetItemString(obj,key,item);
	Py_DECREF(item);
	return rc;
}

inline int PyDict_SetItemString(PyObject* obj, const char *key, unsigned long long value) 
{
	PyObject *item = Py_BuildValue("L",value);
	int rc = PyDict_SetItemString(obj,key,item);
	Py_DECREF(item);
	return rc;
}

inline int PyDict_SetItemString(PyObject* obj, const char *key, double value) 
{
	PyObject *item = Py_BuildValue("d",value);
	int rc = PyDict_SetItemString(obj,key,item);
	Py_DECREF(item);
	return rc;
}

#endif

#ifndef _PYTHON_PROPERTY_H
#define _PYTHON_PROPERTY_H

/// Typedef for struct #s_object
typedef struct s_python_property python_property;

/// python_property data (see #object)
struct s_python_property 
{
    PyObject_HEAD
    OBJECT *obj;
    PROPERTY *prop;
    void (*unlock)(LOCKVAR*);
};

/// Convert a data structure to python
#define to_python(X) ((PyObject*)(X))

/// Convert python property to python_property data
inline python_property *to_python_property(PyObject *obj) { return (python_property*)obj; };

/// Get python type data for python_property
PyObject *python_property_gettype(void);

/// Convert python_property to a python string
PyObject *python_property_str(PyObject *obj);

/// Allocate a new python property object
PyObject *python_property_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

/// Delete a python property object
void python_property_dealloc(PyObject *self);

/// Create a python property object
int python_property_create(PyObject *obj, PyObject *args, PyObject *kwds);

/// Check a python property object
int python_property_check(PyObject *obj, bool nullok = false);

/// Get the property description
PyObject *python_property_get_description(PyObject *self, PyObject *args, PyObject *kwds);

/// Get the python representation of obj
PyObject *python_property_repr(PyObject *obj);

/// Get the property value
PyObject *python_property_get_initial(PyObject *self, PyObject *args, PyObject *kwds);

/// Get the property value
PyObject *python_property_get_value(PyObject *self, PyObject *args, PyObject *kwds);

/// Set the property value
PyObject *python_property_set_value(PyObject *self, PyObject *args, PyObject *kwds);

/// {Read,Write,Un}lock
PyObject *python_property_rlock(PyObject *self, PyObject *args, PyObject *kwds);
PyObject *python_property_wlock(PyObject *self, PyObject *args, PyObject *kwds);
PyObject *python_property_unlock(PyObject *self, PyObject *args, PyObject *kwds);

/// Property get/set
PyObject *python_property_get_object(PyObject *self, PyObject *args, PyObject *kwds);
PyObject *python_property_set_object(PyObject *self, PyObject *args, PyObject *kwds);
PyObject *python_property_get_name(PyObject *self, PyObject *args, PyObject *kwds);
PyObject *python_property_set_name(PyObject *self, PyObject *args, PyObject *kwds);
PyObject *python_property_get_unit(PyObject *self, PyObject *args, PyObject *kwds);

/// Property compare
PyObject *python_property_compare(PyObject *a, PyObject *b, int op);

/// Porperty unit conversion
PyObject *python_property_convert_unit(PyObject *self, PyObject *args, PyObject *kwds);

#endif

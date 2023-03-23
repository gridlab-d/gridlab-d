// File: gridlabx/source/python_property.cpp
// Author: dchassin@slac.stanford.edu
// Copyright: 2020, Regents of the Leland Stanford Junior University

#include "gldcore.h"

#define LF "\n"

static PyMethodDef python_property_methods[] = 
{
    { "get_object", (PyCFunction)python_property_get_object, METH_NOARGS, "Get the property object"},
    { "set_object", (PyCFunction)python_property_set_object, METH_VARARGS, "Set the property object"},
    { "get_name", (PyCFunction)python_property_get_name, METH_NOARGS, "Get the property name"},
    { "set_name", (PyCFunction)python_property_set_name, METH_VARARGS, "Set the property name"},
    { "get_initial", (PyCFunction)python_property_get_initial, METH_NOARGS, "Get the property's initial value"},
    { "get_value", (PyCFunction)python_property_get_value, METH_NOARGS, "Get the property value"},
    { "set_value", (PyCFunction)python_property_set_value, METH_VARARGS, "Set the property value"},
    { "get_unit", (PyCFunction)python_property_get_unit, METH_NOARGS, "Get the property unit"},
    { "rlock", (PyCFunction)python_property_rlock, METH_NOARGS, "Lock the property for reading"},
    { "wlock", (PyCFunction)python_property_wlock, METH_NOARGS, "Lock the property for writing"},
    { "unlock", (PyCFunction)python_property_unlock, METH_NOARGS, "Unlock the property"},
    { "convert_unit", (PyCFunction)python_property_convert_unit, METH_VARARGS, "Convert a property to a new unit"},
    { NULL } // sentinel for iterators
};

static PyMemberDef python_property_members[] = 
{
    { NULL } // sentinel for iterators
};

static const char *python_property_doc = PACKAGE ".property" LF
    LF "Use this object to get direct access to object properties";

PyTypeObject python_property_type = 
{
    PyVarObject_HEAD_INIT(NULL, 0)
    PACKAGE ".property",             // tp_name 
    sizeof(python_property_type),    // tp_basicsize 
    0,                               // tp_itemsize 
    python_property_dealloc,         // tp_dealloc 
    0,                               // tp_vectorcall_offset 
    0,                               // tp_getattr 
    0,                               // tp_setattr 
    0,                               // tp_as_async 
    python_property_repr,            // tp_repr 
    0,                               // tp_as_number 
    0,                               // tp_as_sequence 
    0,                               // tp_as_mapping 
    0,                               // tp_hash 
    0,                               // tp_call 
    python_property_str,             // tp_str 
    0,                               // tp_getattro 
    0,                               // tp_setattro 
    0,                               // tp_as_buffer 
    Py_TPFLAGS_DEFAULT,              // tp_flags 
    python_property_doc,             // tp_doc 
    0,                               // tp_traverse 
    0,                               // tp_clear 
    python_property_compare,         // tp_richcompare 
    0,                               // tp_weaklistoffset 
    0,                               // tp_iter 
    0,                               // tp_iternext 
    python_property_methods,         // tp_methods 
    python_property_members,         // tp_members 
    0,                               // tp_getset 
    0,                               // tp_base 
    0,                               // tp_dict 
    python_property_get_description, // tp_descr_get 
    0,                               // tp_descr_set 
    0,                               // tp_dictoffset 
    python_property_create,          // tp_init 
    0,                               // tp_alloc 
    python_property_new,             // tp_new 
};

/// @returns Python object type definition
PyObject *python_property_gettype ( void )
{
    return to_python(&python_property_type);
}

/// Get the python representation of obj
PyObject *python_property_repr(PyObject *self)
{
    python_property *pyprop = to_python_property(self);
    PyObject *repr;
    if ( pyprop->obj->name )
    {
        repr = PyUnicode_FromFormat("<" PACKAGE ".property:%s.%s>",pyprop->obj->name,pyprop->prop->name);
    }
    else
    {
        repr = PyUnicode_FromFormat("<" PACKAGE ".property:%s:%d.%s>",pyprop->obj->oclass->name,pyprop->obj->id,pyprop->prop->name);
    }
    Py_INCREF(repr);
    return repr;
}

/// @returns Python unicode representation of the **gridlabx.object**
PyObject *python_property_str ( PyObject * self ) ///< Python **gridlabx.object** reference
{
    python_property *pyprop = to_python_property(self);
    PROPERTYSPEC *spec = property_getspec(pyprop->prop->ptype);
    int size = spec->csize;
    if ( size == PSZ_DYNAMIC )
    {
        size = object_get_value(pyprop->obj,pyprop->prop,NULL,0);
    }
    char buffer[size+2];
    object_get_value(pyprop->obj,pyprop->prop,buffer,size+1);
    PyObject *str = PyUnicode_FromFormat("%s",buffer);
    Py_INCREF(str);
    return str;
}

/// @returns Python object reference
PyObject *python_property_new (
    PyTypeObject* type, /// **gridlabx.object** type specification
    PyObject*, 
    PyObject* )
{
    python_property *self = to_python_property(type->tp_alloc(type,0));
    if ( self != NULL )
    {
        self->obj = NULL;
        self->prop = NULL;
        self->unlock = NULL;
    }
    return to_python(self);
}

void python_property_dealloc ( PyObject * self ) ///< Object reference
{
    Py_TYPE(self)->tp_free(self);
}

/// @returns 
///   * gridlabd.property
///   * -1: Invalid arguments
///   * -2: Missing or invalid object name/id
///   * -3: Missing or invalid property name
///   * -4: Name set failed
///   * -5: Parent set failed
int python_property_create (
    PyObject* self, ///< Python **gridlabd.property** reference
    PyObject* args, ///< Python argument list (none)
    PyObject* kwds ) ///< Python keywords list (object_id, object_name, property_name)
{
    python_property *pyprop = to_python_property(self);
    static char *kwlist[] = 
    {
        strdup("object"),
        strdup("property"), 
        NULL,
    };
    PyObject *object_info = NULL;
    char *property_name = NULL;

    if ( ! PyArg_ParseTupleAndKeywords(args, kwds, "|Os", kwlist,
            &object_info, &property_name))
    {
        return -1;
    }

    if ( object_info != NULL && PyLong_Check(object_info) )
    {
        pyprop->obj = object_find_by_id(PyLong_AsLong(object_info));
    }
    else if ( object_info != NULL && PyUnicode_Check(object_info) )
    {
        pyprop->obj = object_find_name(PyUnicode_AsUTF8(object_info));
    }
    else
    {
        PyErr_SetString(PyExc_Exception,"object name/id not specified");
        return -1;
    }

    pyprop->prop = object_get_property(pyprop->obj,property_name,NULL);
    if ( pyprop->prop == NULL )
    {
        PyErr_SetString(PyExc_Exception,"property name not specified");
        return -1;
    }

    return 0;
}

/// @returns Non-zero if reference is to an #object
int python_property_check (
    PyObject* self, ///< Python **gridlabx.object** reference
    bool nullok ) ///< NULL is ok
{
    if ( self == NULL && nullok )
    {
        return 1;
    }
    return ( self && PyObject_IsInstance(self,python_property_gettype()) > 0 ) ? 1 : 0;
}

/// Get the description
PyObject *python_property_get_description(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    if ( pyprop->prop->description )
        return PyUnicode_FromFormat("%s",pyprop->prop->description);
    else
        return PyUnicode_FromFormat(PACKAGE " %s %s",pyprop->obj->oclass->name,pyprop->prop->name);
}

/// Get the property value
PyObject *python_property_get_initial(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    void *addr = property_addr(pyprop->obj,pyprop->prop);
    PROPERTYSPEC *spec = property_getspec(pyprop->prop->ptype);
    if ( spec->to_initial == NULL )
    {
        return python_property_str(self);
    }
    int len = spec->to_initial(NULL,0,addr,pyprop->prop);
    char buffer[len+2];
    memset(buffer,0,len+2);
    if ( spec->to_initial(buffer,len+1,addr,pyprop->prop) < 0 )
    {
        PyErr_SetString(PyExc_Exception,"unable to obtain property initializer");
        return NULL;
    }
    return PyUnicode_FromFormat("%s",buffer);
}

static PyObject *set_to_python(set items, PROPERTY *prop)
{
    // TODO change this to a python set
    return PyLong_FromLong((int64)items);
}

static PyObject *enumeration_to_python(enumeration item, PROPERTY *prop)
{
    // TODO change this to a python enumeration
    return PyLong_FromLong((int64)item);
}

/// Get the property value
PyObject *python_property_get_value(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    void *addr = property_addr(pyprop->obj,pyprop->prop);
    switch ( pyprop->prop->ptype )
    {
    case PT_void:
        Py_RETURN_NONE;
    case PT_double:
    case PT_loadshape:
    case PT_enduse:
    case PT_random:    
        return PyFloat_FromDouble(*(double*)addr);
    case PT_complex:
        return PyComplex_FromDoubles(*(double*)addr,*(((double*)addr)+1));
    case PT_enumeration:
        return enumeration_to_python(*(enumeration*)addr,pyprop->prop);
    case PT_set:
        return set_to_python(*(enumeration*)addr,pyprop->prop);
    case PT_int16:
        return PyLong_FromLong((long)*(int16*)addr);
    case PT_int32:
        return PyLong_FromLong((long)*(int32*)addr);
    case PT_int64:
        return PyLong_FromLong((long)*(int64*)addr);
    case PT_char8:
    case PT_char32:
    case PT_char256:
    case PT_char1024:
        return PyUnicode_FromFormat("%s",(const char*)addr);
    case PT_object:
        if ( *(OBJECT**)addr == NULL )
        {
            Py_RETURN_NONE;
        }
        else if ( (*(OBJECT**)addr)->name )
        {
            return PyUnicode_FromFormat("%s",(*(OBJECT**)addr)->name);
        }
        else
        {
            return PyUnicode_FromFormat("%s:%d",(*(OBJECT**)addr)->oclass->name,(*(OBJECT**)addr)->id);
        }
    case PT_delegated:
        // TODO
        PyErr_SetString(PyExc_Exception,"unable to get delegated value");
        return NULL;
    case PT_bool:
        if ( *(bool*)addr )
        {
            Py_RETURN_TRUE;
        }
        else
        {
            Py_RETURN_FALSE;
        }
    case PT_timestamp:
        return PyLong_FromLong((long)*(int64*)addr);
    case PT_double_array:
        // TODO
        PyErr_SetString(PyExc_Exception,"unable to get double array");
        return NULL;
    case PT_complex_array:
        // TODO
        PyErr_SetString(PyExc_Exception,"unable to get complex array");
        return NULL;
    case PT_real:
        // TODO
        PyErr_SetString(PyExc_Exception,"unable to get real value");
        return NULL;
    case PT_float:
        // TODO
        PyErr_SetString(PyExc_Exception,"unable to get float value");
        return NULL;
    case PT_method:
        // TODO
        PyErr_SetString(PyExc_Exception,"unable to get method value");
        return NULL;
    case PT_string:
        return PyUnicode_FromFormat("%s",(*(STRING*)addr)->c_str());
    case PT_python:
        return *(PyObject**)addr;
    default:
        PyErr_SetString(PyExc_Exception,"unable to get value of unknown type");
        return NULL;
    }
}

/// Set the property value
PyObject *python_property_set_value(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    void *addr = property_addr(pyprop->obj,pyprop->prop);
    PyObject *value = NULL;
    if ( ! PyArg_ParseTuple(args,"O",&value) )
    {
        return NULL;
    }
    if ( PyUnicode_Check(value) )
    {
        int size = PyUnicode_GetLength(value);
        const char *str = PyUnicode_AsUTF8(value);
        int count = property_read(pyprop->prop,addr,str);
        if ( count <= 0 )
        {
            char msg[1024];
            snprintf(msg,1023,"unable to read property from string '%-.32s%s'",str,size>32?"...":"");
            PyErr_SetString(PyExc_Exception,msg);
            return NULL;
        }
        Py_RETURN_NONE;
    }
    switch ( pyprop->prop->ptype )
    {
    case PT_void:
        Py_RETURN_NONE;
    case PT_double:
        if ( PyFloat_Check(value) )
        {
            *(double*)addr = PyFloat_AsDouble(value);
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not a float");
            return NULL;            
        }
    case PT_complex:
        if ( PyComplex_Check(value) )
        {
            double x = PyComplex_RealAsDouble(value);
            double y = PyComplex_ImagAsDouble(value);
            ((complex*)addr)->SetRect(x,y);
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not a complex");
            return NULL;
        }
    case PT_enumeration:
        // support python enumeration
        if ( PyLong_Check(value) )
        {
            *(enumeration*)addr = (enumeration)PyLong_AsLong(value);
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not an integer");
            return NULL;
        }
    case PT_set:
        // TODO support python set
        if ( PyLong_Check(value) )
        {
            *(set*)addr = (set)PyLong_AsLong(value);
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not an integer");
            return NULL;
        }
    case PT_int16:
        if ( PyLong_Check(value) )
        {
            int64 integer = PyLong_AsLong(value);
            if ( integer < -32768 || integer > 65535 )
            {
                PyErr_SetString(PyExc_Exception,"value is not in range of int16");
                return NULL;
            }
            *(int16*)addr = PyLong_AsLong(value);
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not an integer");
            return NULL;
        }
    case PT_int32:
        if ( PyLong_Check(value) )
        {
            int64 integer = PyLong_AsLong(value);
            if ( integer < INT_MIN || integer > 0xffffffff )
            {
                PyErr_SetString(PyExc_Exception,"value is not in range of int32");
                return NULL;
            }
            *(int32*)addr = PyLong_AsLong(value);
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not an integer");
            return NULL;
        }
    case PT_int64:
        if ( PyLong_Check(value) )
        {
            *(int32*)addr = PyLong_AsLong(value);
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not an integer");
            return NULL;
        }
    case PT_char8:
        if ( PyUnicode_Check(value) )
        {
            if ( PyUnicode_GetLength(value) > 8 )
            {
                PyErr_SetString(PyExc_Exception,"value is too long for char8");
                return NULL;
            }
            strcpy((char*)addr,PyUnicode_AsUTF8(value));
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not a string");
            return NULL;
        }
    case PT_char32:
        if ( PyUnicode_Check(value) )
        {
            if ( PyUnicode_GetLength(value) > 32)
            {
                PyErr_SetString(PyExc_Exception,"value is too long for PT_char32");
                return NULL;
            }
            strcpy((char*)addr,PyUnicode_AsUTF8(value));
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not a string");
            return NULL;
        }
    case PT_char256:
        if ( PyUnicode_Check(value) )
        {
            if ( PyUnicode_GetLength(value) > 256 )
            {
                PyErr_SetString(PyExc_Exception,"value is too long for PT_char256");
                return NULL;
            }
            strcpy((char*)addr,PyUnicode_AsUTF8(value));
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not a string");
            return NULL;
        }
    case PT_char1024:
        if ( PyUnicode_Check(value) )
        {
            if ( PyUnicode_GetLength(value) >= 1024 )
            {
                PyErr_SetString(PyExc_Exception,"value is too long for PT_char1024");
                return NULL;
            }
            strcpy((char*)addr,PyUnicode_AsUTF8(value));
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not a string");
            return NULL;
        }
    case PT_object:
        if ( value == Py_None )
        {
            *(OBJECT**)addr = NULL;
            Py_RETURN_NONE;
        }
        else if ( PyLong_Check(value) )
        {
            *(OBJECT**)addr = object_find_by_id(PyLong_AsLong(value));
            Py_RETURN_NONE;
        }
        else if ( PyUnicode_Check(value) )
        {
            *(OBJECT**)addr = object_find_name(PyUnicode_AsUTF8(value));
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not object id or name");
            return NULL;
        }
    case PT_delegated:
        PyErr_SetString(PyExc_Exception,"cannot set a delegated type");
        return NULL;
    case PT_bool:
        *(bool*)addr = PyObject_IsTrue(value) ? true : false;
        Py_RETURN_NONE;
    case PT_timestamp:
        if ( PyLong_Check(value) )
        {
            *(TIMESTAMP*)addr = PyLong_AsLong(value);
            Py_RETURN_NONE;
        }
        else
        {
            PyErr_SetString(PyExc_Exception,"value is not an integer");
            return NULL;
        }
    case PT_double_array:
        // TODO support double array
        PyErr_SetString(PyExc_Exception,"cannot set double array");
        return NULL;
    case PT_complex_array:
        // TODO support complex array
        PyErr_SetString(PyExc_Exception,"cannot set complex array");
        return NULL;
    case PT_real:
        PyErr_SetString(PyExc_Exception,"cannot set real");
        return NULL;
    case PT_float:
        PyErr_SetString(PyExc_Exception,"cannot set float");
        return NULL;
    case PT_loadshape:
        PyErr_SetString(PyExc_Exception,"cannot set loadshape");
        return NULL;
    case PT_enduse:
        PyErr_SetString(PyExc_Exception,"cannot set enduse");
        return NULL;
    case PT_random:
        if ( PyDict_Check(value) )
        {
            Py_ssize_t pos = 0;
            PyObject *key, *item;
            while ( PyDict_Next(value,&pos,&key,&item) )
            {
                const char *name = PyUnicode_AsUTF8(key);
                const char *text = PyUnicode_AsUTF8(PyObject_Str(item));
                if ( random_set_part(addr,name,text) == 0 )
                {
                    static char msg[1024];
                    snprintf(msg,1023,"unable to set random.%s to '%s'",name,text);
                    PyErr_SetString(PyExc_Exception,msg);
                    return NULL;
                }
            }
            Py_RETURN_NONE;
        }
        else
        {   
            PyErr_SetString(PyExc_Exception,"value is not a dict");
            return NULL;
        }
    case PT_method:
        PyErr_SetString(PyExc_Exception,"cannot set method");
        return NULL;
    case PT_string:
        PyErr_SetString(PyExc_Exception,"cannot set string");
        return NULL;
    case PT_python:
        Py_DECREF(*(PyObject**)addr);
        Py_INCREF(value);
        *(PyObject**)addr = value;
        Py_RETURN_NONE;
    default:
        PyErr_SetString(PyExc_Exception,"unable to set value of unknown/unsupported type");
        return NULL;
    }
}

PyObject *python_property_rlock(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    ::rlock(&pyprop->obj->lock);
    pyprop->unlock = ::runlock;
    Py_RETURN_NONE;
}

PyObject *python_property_wlock(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    ::wlock(&pyprop->obj->lock);
    pyprop->unlock = ::wunlock;
    Py_RETURN_NONE;
}

PyObject *python_property_unlock(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    if ( pyprop->unlock ) 
    {
        pyprop->unlock(&pyprop->obj->lock);
        pyprop->unlock = NULL;
    }
    Py_RETURN_NONE;
}

PyObject *python_property_get_object(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    if ( pyprop->obj->name )
    {
        return PyUnicode_FromFormat("%s",pyprop->obj->name);
    }
    else
    {
        return PyUnicode_FromFormat("%s:%d",pyprop->obj->oclass->name,pyprop->obj->id);
    }
}

PyObject *python_property_set_object(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    PyObject *value = NULL;
    if ( ! PyArg_ParseTuple(args,"O",&value) )
    {
        return NULL;
    }

    OBJECT *obj = NULL;
    if ( PyLong_Check(value) )
    {
        obj = object_find_by_id(PyLong_AsLong(value));
    }
    else if ( PyUnicode_Check(value) )
    {
        obj = object_find_name(PyUnicode_AsUTF8(value));
    }
    if ( obj == NULL )
    {
        PyErr_SetString(PyExc_Exception,"object name/id not valid/specified");
        return NULL;
    }
    pyprop->obj = obj;
    Py_RETURN_NONE;
}

PyObject *python_property_get_name(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    return PyUnicode_FromFormat("%s",pyprop->prop->name);
}

PyObject *python_property_set_name(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    PyObject *value = NULL;
    if ( ! PyArg_ParseTuple(args,"O",&value) )
    {
        return NULL;
    }

    PROPERTY *prop = PyUnicode_Check(value) ? object_get_property(pyprop->obj,PyUnicode_AsUTF8(value),NULL) : NULL;
    if ( prop == NULL )
    {
        PyErr_SetString(PyExc_Exception,"property name not specified/valid");
        return NULL;
    }
    pyprop->prop = prop;
    Py_RETURN_NONE;
}

PyObject *python_property_get_unit(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    if ( pyprop->prop->unit == NULL )
    {
        Py_RETURN_NONE;
    }
    else
    {
        return PyUnicode_FromFormat("%s",pyprop->prop->unit->name);
    }
}

PyObject *python_property_compare(PyObject *obj1, PyObject *obj2, int op)
{
    PyObject *result = NULL;
    python_property *a = to_python_property(obj1);
    python_property *b = to_python_property(obj2);
    int c;
    switch (op)
    {
    case Py_EQ: 
        c = a->obj == b->obj && a->prop == b->prop; 
        break;
    case Py_NE: 
        c = a->obj != b->obj || a->prop != b->prop ; 
        break;
    default:
        result = Py_NotImplemented;
        break;
    }
    if ( result == NULL )
    {
        result = c ? Py_True : Py_False;
    }
    Py_INCREF(result);
    return result;
}

PyObject *python_property_convert_unit(PyObject *self, PyObject *args, PyObject *kwds)
{
    python_property *pyprop = to_python_property(self);
    if ( pyprop->prop->unit == NULL )
    {
        PyErr_SetString(PyExc_Exception,"property does have any units");
        return NULL;
    }
    void *addr = property_addr(pyprop->obj,pyprop->prop);
    const char *value;
    if ( ! PyArg_ParseTuple(args,"s",&value) )
    {
        return NULL;
    }
    UNIT *unit = unit_find(value);
    if ( unit == NULL )
    {
        PyErr_SetString(PyExc_Exception,"unit not found");
        return NULL;
    }
    if ( pyprop->prop->ptype == PT_complex )
    {
        complex z = *(complex*)addr;
        if ( unit_convert_complex(pyprop->prop->unit,unit,&z) )
        {
            return PyComplex_FromDoubles(z.Re(),z.Im());
        }
    }
    else
    {
        double x = *(double*)addr;
        if ( unit_convert_ex(pyprop->prop->unit,unit,&x) )
        {
            return PyFloat_FromDouble(x);
        }
    }
    PyErr_SetString(PyExc_Exception,"unable to convert to specified unit");
    return NULL;
}

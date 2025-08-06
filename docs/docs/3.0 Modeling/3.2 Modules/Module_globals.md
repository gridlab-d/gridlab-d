# Module globals

How to publish a global variable from a module 

## Synopsis

C
    
    
     gl_global_create("_#Module name|module-name_ ::_variable-name_ ", 
      PT__[built-in_type],_variable-address _,_
      PT_SIZE,_array-size_ ,
      PT_UNITS,_[units]_ ,
      PT_ACCESS,_access-control-flags_ ,
      PT_DESCRIPTION,_brief-description_ ,
      NULL);
    

C++
    
    
     class [gld_global] {
       [gld_global](const char *name, [PROPERTYTYPE] t, void *p);
     }
    

## Description

The naming convention for module globals requires that the module name precede the global variable name separated by a double colon, as in `_module--name_ ::_variable-name_`. This allows the core to associate the global with the module. If the variable name does not include the module name, it will be treated as a core global. There is nothing to prevent module programmers for doing this, and in some cases this may be useful. 

## Parameters

The follow parameter may be used to define a module or class global variable. 

### Access control flags

    This identifies how the global may be accessed by other modules and object. See [PT_ACCESS] for details. If omitted, the variable is assumed to be [public].

### Array size

    Identifies the array size, which is optional. If omitted it is assumed to be 1.

### Brief description

    This identifies a string constant that provide a brief synopsis of the variable. This description is displayed by --[modhelp] and [XML] output.

### [Built-in type]

    This identifies what [built-in type] the variable is. This option is required.

### Module name

    This identifies the module name. The module name is optional, and when omitted causes the variable to become a core global.

### [Units]

    This identifies the [units] for double and complex variables. The units are optional and if omitted, the variable is considered unitless.

### Variable name

    This identifies the variable name and is required.

## Examples

### Module globals

Module globals are created whenever a module is loaded. These must be placed in the module [init] function to ensure that they are always created when modules are loaded. 

Note
    Most module implementation is a callback function table to define **gl_create_global**. Therefore calls to this function cannot be completed until the callback table is built using the **set_callback** function. See [source documentation] on **set_callback** for details.

Example (main.cpp)
    
    
    
     #include "gridlabd.h"
     #include "my_class.h"
     char256 my_data = "initial value";
     EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *[])
     {
       if (set_callback(fntable)==NULL)
       {
         errno = EINVAL;
         return NULL;
       }
       **if ( gl_global_create("my_module::my_data",PT_char256,my_data,**
       **PT_ACCESS,PA_PUBLIC,**
       **PT_DESCRIPTION,"my data example",NULL)==NULL )**
       **throw "unable to create module global char256 my_module::my_data";**
       new my_class(module);
       return my_class::oclass;
     }
    

### Class globals

Class global are created whenever a class is referenced. These must be placed in the class constructor, which is called only when the class is first referenced. 

Note
    It is unusual to have class globals because they are only created when the class is referenced, but there are cases where this may be preferred to a module global. An example would be a situation in which all object of a given class must share a variable that may be altered by the user but would not be available to the user if the class when the class is not used.

Example (my_class.cpp)
    
    
    
     class my_class {
     public:  my_class(MODULE *);
     private: char256 my_variable;
     };
     static char256 my_class::my_variable = "initial value";
     my_class::my_class(MODULE *module)
     {
       if ( oclass==NULL )
       {
         oclass = gld_class::create(module,"my_class",sizeof(my_class),PC_AUTOLOCK);
         if ( oclass==NULL ) throw "unable to register my_class";
         else oclass->trl = TRL_UNKNOWN;
         defaults = this;
         if ( gl_publish_variables( oclass, 
               NULL ) < 1 ) throw "unable to publish my_class properties";
       }
       else throw "invalid attempt to define class more than once";
       **gld_global my_global("my_module::my_variable",PT_char256,my_variable);**
       **if ( !my_global.isvalid() )**
       **throw "unable to create class global char256 my_module::my_variable";**
       memset(this,0,sizeof(my_class));
       }
     }
    

## See also

  * [Guide to Programming GridLAB-D]
    * Introduction 
      * [Developer prerequisites]
      * [Programming conventions]
      * [Build/release process]
      * [Documentation Guide]
      * [Theory of operation]
    * [Creating a module]
      * Module globals
      * [Module functions]
      * [Subsecond processing]
      * [Import/export]
      * [Check]
      * [KML output]
      * [Example 1]
    * [Creating a class]
      * [Class functions]
      * [Class globals]
      * [Publishing properties]
      * [Publishing methods]
      * [Notifications]
      * [Load methods] 
      * [Example 2]
    * Special Topics 
      * [Data types]
      * [Multithreading]
      * [Application links]
      * [Realtime server]
      * [Graphical user interfaces]
      * [Troubleshooting messages]
      * [Example 3]
    * [Source documentation]
      * [C/C++ Module API documentation (trunk)](http://gridlab-d.sourceforge.net/doxygen/trunk/group__module__api.html)
      * [C/C++ Module API Guide]
      * [Example 4]
    * [Validation]
      * [Example 5]
    * Debugging 
      * [Debug option]
      * [VS2005 (MS Windows)]
        * [use_msvc]
      * [gdb option (linux/mac)]
        * [gdb_window]
      * [Runtime Class Debugging]
        * [compile_once]
    * [Code templates]

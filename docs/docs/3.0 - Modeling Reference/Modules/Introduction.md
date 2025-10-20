# Introduction

TODO - Introduction - What is a GridLAB-D module?
TODO - Introduction - What is a module function?
TODO - Introduction - What is a module global?

## Module functions

Required export functions:    
    
    EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[]);
    EXPORT void term(void);
    CDECL int do_kill();
    

Optional export functions:
    
    
    EXPORT int check();
    EXPORT int export(const char *file);
    EXPORT int import(const char *file);
    EXPORT int kmldump( int(*)(const char *file,...), OBJECT *obj);
    EXPORT void test(int argc, const char *argv[]);
    EXPORT size_t stream(void *ptr, size_t len, bool is_str=false, void *match==NULL);
    

Subsecond export functions
    
    EXPORT unsigned long deltamode_desired(int *flags);
    EXPORT unsigned long preupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt);
    EXPORT SIMULATIONMODE interupdate(MODULE *module, TIMESTAMP t0, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
    EXPORT STATUS postupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt);ed int64 _dt_);
    

### Required functions

#### init

The `init` function is required for all GridLAB-D™ modules. It is called once when the module is loaded. The `init` should use this opportunity to register all **classes** and **module globals**. The template for this function is: 
    
    
    // module/main.cpp (init template)
    #define DLMAIN
    #include <stdlib.h>
    #include "gridlabd.h"
    EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
    {
      if (set_callback(fntable)==NULL)
      {
        errno = EINVAL;
        return NULL;
      }
      // TODO: add gl_global_create() calls here (see module globals for details)
      // TODO: call new for each class here (see create class for details)
      return NULL; // TODO: return oclass member of first new class
    }
    

#### do_kill

The `do_kill` function is required for all GridLAB-D™ modules. It is called when GridLAB-D™ terminates. The `do_kill` function should be used only to cleanup temporary files and memory allocation used by the module. 
    
    
    // module/main.cpp (do_kill template)
    #define DLMAIN
    #include <stdlib.h>
    #include "gridlabd.h"
    CDECL int do_kill()
    {
      // TODO: perform cleanup actions if needed
      return 0;
    }
    

### Optional functions

#### term

The `term` function is called when the simulation stop (normally or on error). 
    
    
    // module/main.cpp (do_kill template)
    #define DLMAIN
    #include <stdlib.h>
    #include "gridlabd.h"
    EXPORT void term(void)
    {
      // TODO: perform simulation end operations
    }
    

#### check

The `check` function is used to allow user to perform module checks before running a simulation. These can be used to verify model consistency, disk space available, and other verification procedures that are not always needed, but can be helpful in diagnosing problems. 
    
    
    // module/main.cpp (do_kill template)
    #define DLMAIN
    #include <stdlib.h>
    #include "gridlabd.h"
    CDECL int check()
    {
      // TODO: perform check operations and report issues
      return 0;
    } 
    

#### export

The `export` function allows a module to define a method for exporting a GridLAB-D™ model file to an arbitrary file format. If defined, the export routine is called after the simulation is completed. 

`module/main.cpp`
    
    EXPORT int (*export)(const char *file)
    {
      // your export code 
      return count; // count of entities exported
    }
    

#### import

The `import` function is used to load a model from an arbitrary file format. The `import` GLM directive is used to initiative the import process. 
    
    
    EXPORT int import(const char *file)
    {
      // import processing code
      return n; // n=0: failed; n<0: error after loading n entities; n>0: successfully loaded n entities
    }

#### kmldump

The `kmldump` function is used to output KML (Google Earth) data. 
    
    
    typedef int (*KMLOUT)(const char *format, ...);
    EXPORT int kmldump(KMLOUT kmlout, OBJECT *obj)
    {
      kmlout("kml data");
      return 0; // return value is ignored
    }

See [Google KML Documentation](https://developers.google.com/kml/documentation/) for details on the KML format. 

#### test

TODO - Update - The `test` function is relatively unused and was intended to support module tests. 

#### stream

TODO - Update - The `stream` function will soon be required to support checkpoints. 


### Subsecond functions

- `deltamode_desired`: indicate whether delta mode is desired
- ` preupdate`: returns `deltamode_timestep`
- `interupdate`: module-level call at each deltatimestep, including iterations
- `postupdate`: returns `SUCCESS`

See [dev:Subsecond](../../4.0%20Developing/4.5%20-%20Deltamode%20Development/4.5.1%20-%20Dev_Subsecond.md) for more details. 

### Contingent functionalities

Modules may have runtime functionalities that are not always available, e.g., when an external application is not installed. In such cases, it is highly recommended that the module create a global flag variable only when the functionality is available. You can create a **global variable** in init, for example: 
    
        
    bool mytool_ok = false;
    if ( load_mytool() )
    {
      mytool_ok = true;
      gl_create_global("module-name",PT_bool, &mytool,NULL);
    }
    

This will allow users to create GLM files that have contingent models depending on the presence of the external tools: 
    
    
    #ifdef module-name
    class ...
    object ...
    #endif
    

Some examples of contingent functionalities are MATLAB and MYSQL. 

**VS2005** - 
  Often these functionalities are only supported when the proper libraries are installed on the build machine. These modules usually have a special flag set, e.g., HAVE_MYSQL or HAVE_MATLAB so that Linux/Mac machine can automatically build the proper code based on what is installed. In Windows this is not possible. If the libraries are not available, we recommend you unload the project and not build it rather than changing the flag.

## Module globals

How to publish a global variable from a module 

### Synopsis

C
    
    
     gl_global_create("_#Module name|module-name_ ::_variable-name_ ", 
      PT__built-in_type,_variable-address _,_
      PT_SIZE,_array-size_ ,
      PT_UNITS,_units_ ,
      PT_ACCESS,_access-control-flags_ ,
      PT_DESCRIPTION,_brief-description_ ,
      NULL);
    

C++
    
    
     class gld_global {
       gld_global(const char *name, PROPERTYTYPE t, void *p);
     }
    

### Description

The naming convention for module globals requires that the module name precede the global variable name separated by a double colon, as in `module--name ::variable-name`. This allows the core to associate the global with the module. If the variable name does not include the module name, it will be treated as a core global. There is nothing to prevent module programmers for doing this, and in some cases this may be useful. 

## Parameters

The follow parameter may be used to define a module or class global variable. 

Parameter | Description
-- | --
**Access control flags** | This identifies how the global may be accessed by other modules and object. See PT_ACCESS for details. If omitted, the variable is assumed to be public.
**Array size**| Identifies the array size, which is optional. If omitted it is assumed to be 1.
**Brief description** | This identifies a string constant that provide a brief synopsis of the variable. This description is displayed by --modhelp and XML output.
**Built-in type** | This identifies what built-in type the variable is. This option is required.
**Module name** |     This identifies the module name. The module name is optional, and when omitted causes the variable to become a core global.
**Units** |     This identifies the units for double and complex variables. The units are optional and if omitted, the variable is considered unitless.
**Variable name** | This identifies the variable name and is required.

### Examples

#### Module globals

Module globals are created whenever a module is loaded. These must be placed in the module init function to ensure that they are always created when modules are loaded. 

Note:
    Most module implementation is a callback function table to define **gl_create_global**. Therefore calls to this function cannot be completed until the callback table is built using the **set_callback** function. See source documentation on **set_callback** for details.

Example (main.cpp):   
    
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
    

#### Class globals

Class global are created whenever a class is referenced. These must be placed in the class constructor, which is called only when the class is first referenced. 

Note:
  It is unusual to have class globals because they are only created when the class is referenced, but there are cases where this may be preferred to a module global. An example would be a situation in which all object of a given class must share a variable that may be altered by the user but would not be available to the user if the class when the class is not used.

Example (my_class.cpp):   
    
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
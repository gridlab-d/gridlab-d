# Module functions

Required and optional module export functions 

## Synopsis

Required functions
    
    
    
    EXPORT CLASS *init(CALLBACKS *_fntable_ , MODULE *_module_ , int _argc_ , char *[argv][]);
    EXPORT void term(void);
    CDECL int do_kill();
    

Optional functions
    
    
    
    EXPORT int check();
    EXPORT int export(const char *_file_);
    EXPORT int import(const char *_file_);
    EXPORT int kmldump( int(*)(const char *_file_ ,...), OBJECT *_obj_);
    EXPORT void test(int _argc_ , const char *_argv_[]);
    EXPORT size_t stream(void *_ptr_ , size_t _len_ , bool is_str=**false** , void *_match_ ==**NULL**);
    

Subsecond functions
    
    
    
    EXPORT unsigned long [deltamode_desired](int *_flags_);
    EXPORT unsigned long [preupdate](module *_module_ , [TIMESTAMP] _t0_ , unsigned int64 _dt_);
    EXPORT [SIMULATIONMODE] [interupdate](module *_module_ , [TIMESTAMP] _t0_ , unsigned int64 _delta_time_ , unsigned long _dt_ , unsigned int _iteration_count_val_);
    EXPORT [STATUS] [postupdate](module *_module_ , [TIMESTAMP] _t0_ , unsigned int64 _dt_);
    

## Required functions

### init

The init function is required for all GridLAB-D modules. It is called once when the module is loaded. The init should use this opportunity to register all [classes] and [module globals]. The template for this function as of [Hassayampa (Version 3.0)] is: 
    
    
    // module/main.cpp (init template)
    #define DLMAIN
    #include <stdlib.h>
    #include "gridlabd.h"
    EXPORT class *init([CALLBACKS] *fntable, module *module, int argc, char *argv[])
    {
      if (set_callback(fntable)==NULL)
      {
        errno = EINVAL;
        return NULL;
      }
      // **TODO**: add gl_global_create() calls here (see [module globals] for details)
      // **TODO**: call new for each class here (see [create class] for details)
      return NULL; // **TODO**: return **oclass** member of first new class
    }
    

### do_kill

The do_kill function is required for all GridLAB-D modules. It is called when GridLAB-D terminates. The do_kill function should be used only to cleanup temporary files and memory allocation used by the module. 
    
    
    // module/main.cpp (do_kill template)
    #define DLMAIN
    #include <stdlib.h>
    #include "gridlabd.h"
    CDECL int do_kill()
    {
      // **TODO**: perform cleanup actions if needed
      return 0;
    }
    

## Optional functions

### term

The term function is called when the simulation stop (normally or on error). 
    
    
    // module/main.cpp (do_kill template)
    #define DLMAIN
    #include <stdlib.h>
    #include "gridlabd.h"
    EXPORT void term(void)
    {
      // **TODO**: perform simulation end operations
    }
    

### check

The check function is used to allow user to perform module checks before running a simulation. These can be used to verify model consistency, disk space available, and other verification procedures that are not always needed, but can be helpful in diagnosing problems. 
    
    
    // module/main.cpp (do_kill template)
    #define DLMAIN
    #include <stdlib.h>
    #include "gridlabd.h"
    CDECL int check()
    {
      // **TODO**: perform check operations and report issues
      return 0;
    }
    

### export

The export function allows a module to define a method for exporting a GridLAB-D to an arbitrary file format. If defined, the export routine is called after the simulation is completed. 

_module_ /main.cpp
    
    
    EXPORT int (*export)(const char *file)
    {
      // your export code 
      return count; // count of entities exported
    }
    

### import

The import function is used to load a model from an arbitrary file format. The [import] GLM directive is used to initiative the import process. 
    
    
    EXPORT int import(const char *_file_)
    {
      // import processing code
      return n; // n=0: failed; n<0: error after loading n entities; n>0: successfully loaded n entities
    }
    

### kmldump

The kmldump function is used to output KML (Google Earth) data. 
    
    
    typedef int (*KMLOUT)(const char *_format_ , ...);
    EXPORT int kmldump(KMLOUT _kmlout_ , OBJECT *_obj_)
    {
      kmlout("_kml data_ ");
      return 0; // return value is ignored
    }
    

See [Google KML Documentation](https://developers.google.com/kml/documentation/) for details on the KML format. 

### test

The test function is relatively unused and was intended to support module tests. 

### stream

The stream function will soon be required to support checkpoints. 

**TODO**: 

## Subsecond functions

See [Dev:Subsecond] for details. 

## Contingent functionalities

Modules may have runtime functionalities that are not always available, e.g., when an external application is not installed. In such cases, it is highly recommended that the module create a global flag variable only when the functionality is available. You can create a [global variable] in init, for example: 
    
    
    bool mytool_ok = false;
    if ( load_mytool() )
    {
      mytool_ok = true;
      gl_create_global("_module-name_ ",PT_bool, &mytool,NULL);
    }
    

This will allow users to create GLM files that have contingent models depending on the presence of the external tools: 
    
    
    #ifdef _module-name_
    class ...
    object ...
    #endif
    

Some examples of contingent functionalities are [MATLAB] and [MYSQL]. 

[VS2005]
    Often these functionalities are only supported when the proper libraries are installed on the build machine. These modules usually have a special flag set, e.g., HAVE_MYSQL or HAVE_MATLAB so that Linux/Mac machine can automatically build the proper code based on what is installed. In Windows this is not possible. If the libraries are not available, we recommend you unload the project and not build it rather than changing the flag.

## Version

Prior to [Hassayampa (Version 3.0)]
    The implementation functions and file structures are different and deprecated.

## See also

  * [Guide to Programming GridLAB-D]
    * Introduction 
      * [Developer prerequisites]
      * [Programming conventions]
      * [Build/release process]
      * [Documentation Guide]
      * [Theory of operation]
    * [Creating a module]
      * [Module globals]
      * Module functions
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
  * [Subsecond]
    * Development 
      * [Requirements]
      * [Specifications]
      * [Programmer's Manual]
    * [Global variables]
      * [simulation_mode]
      * [deltamode_maximumtime]
      * [deltamode_timestep]
      * [deltamode_preferred_module_order]
      * [deltamode_updateorder]
      * [deltamode_iteration_limit]
      * [deltamode_forced_extra_timesteps]
      * [deltamode_forced_always]


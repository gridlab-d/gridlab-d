# Options

This reference page details options that can be used to customize your model.

## Escape Character

The **escape character** prevents GLM parser from interpreting the next character in the usual way. The escape character in GLM files is the backslash (\\). It is only considered when loading a value, such as 
    
    
    object my_test {
      value "\", \', or \; would stop parsing the value";
    }
    

You can also use the escape character to prevent the macro parser from processor the `${name}` sequence, such as 
    
    
    #define MYVAR=1
    object my_test {
      value "$\{MYVAR\} could be confused as a macro expansion"; 
    }

### Example

A good example of usage of the escape character can be found in the [core mainloop test](http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/trunk/core/autotest/test_exec_mainloop.glm). 

!!! caveat

    The escape character does not work while parsing macros, directives, names, blocks, etc. In these cases the \ is left in place and allowed to pass through to the loader. 

    MS Windows
        Although the convention in Windows originally is that directory names in paths be delimited using a \, Windows supports / delimiters. Consequently, the convention in GridLAB-D is to use exclusively / for paths. If you must use \ in a path value, then you must escape it by using \\\\.


##  Macros
The .glm loader allows the use of macros to control the behavior of the parser and to a limited extent also the behavior of GridLAB-D™. Macros are lines that begin with a '#' sign. The following macros are available


Macro | Description
-- | --
**#define** | name=value is used to define a global variable. This allows the creation of a new global variable, in contrast to the #set macro which requires the global variable already exist.
**#set** | name=value is used to set a global variable. For a list of defined global variables, see the [Doxygen Documentation](https://gridlab-d.readthedocs.io/en/docs/gridlabd/group__gridlabd__h__globals/).
**#undef** | is used to remove the definition of a global variable. `#ifdef\|ifndef <expression>` <br/> `...` <br/> `[#else` <br/> `... ]` <br/>
**#endif** | is used to conditionally process a block of text in the .glm only if the variable used is defined. For `#ifdef` and `#ifndef` (conditionally execute a block of GLM code only if the variable used is *not* defined.) the expression is simply the name of a global variable. <br/> `#ifdef variable` <br/> `// conditional block` <br/> `#endif`
**#if** |  is used to conditionally execute a block of GLM code only if the test succeeds. Each `#if` macro must have a correspond `#endif` macro matching it in the same GLM file.  When `#if` is used, the expression is a conditional test in the form of name op value where the operator op is one of `<`, `>`, `&lt=`, `>=`, `==`, or `!=`. <br/> `#if test` <br/> `// conditional block` <br/> `#endif`
**#print** | name is used to display the value of the global variable `name` at the moment it is encountered by the loader.
**#ifexist** | macro is is used to conditionally execute a block of GLM code only if the specified file can be found in the GridLAB-D search path specified by GLPATH. <br/> `#ifexist filename` <br/> `// conditional block` <br/> `#endif`
**#include** | macro is used to include another file during the parser load. The include path determines where the compiler will find the include files needed to compile runtime classes. <br/> `#set include=path` <br/> `#include "filename"` <br/> `#include using(name=value[,...]) "filename"` <br/> `#include <filename>` <br/> `#include [url]` <br/> There are three recognized types of `#include` macro directives. The first is used to include a regular GLM or CONF file. The second is used to include a C or C++ header. The third is to include an external URL (e.g., http://...). <br/> When the variant `using(name =value)` is used, then the global variable name is set to value before the include file is loaded. This option disables strict global variable naming (strictnames and enables multiple include files `allow_reinclude`). Multiple global variables may be set using comma delimiters. <br/> - **GLM or CONF**: The first file of the specified name found in the GridLAB-D search path, `GLPATH`, will be loaded at the point at which the #include macro is found, after which the rest of the GLM file is loaded. <br/> - **C or C++ Source**: The exact filename given will be added to the include statements written to the C++ source code before it is compiled. The header include statement will be added in the order in which they are found in the GLM file, including the `class` statements. <br/> - **URL**: The URL is downloaded from the internet and stored in a local cache file using the URL as a name template with only valid characters showing. The file is not downloaded again once it is successfully copied and is always stored in the current directory.
**#setenv PATH=path** | macro is used to set an environment variable
**#binpath** | macro sets the path for binary searches (compiler PATH environment) **DEPRECATED** 
**#libpath** | macro sets the path for library searches (compiler GLPATH environment) **DEPRECATED** 
**#incpath** | macro sets the path for include file searches (compiler INCLUDE environment). Sets the search path for the inline C compiler to use for finding header files. This is the same as `#setenv INCLUDE=PATH` **DEPRECATED** 
**#error** | message triggers a parser error condition
**#warning** | message triggers a parser warning condition
**#option** | command-option applied the command line option
**#system** | command makes an operating system call and waits for completion before continuing processing the GLM file
**#start** | command makes an operating system call and continues processing the GLM file
**#set allow_reinclude** | allows an include file to be included more than once

Normally, GridLAB-D does not allow an include file to included more than once. However, when parameter include files are used, the ability to use an include file more than once is usually desired. The `allow_reinclude` global variable disables the restriction. 

In file `main.glm`:
    
    
    #set allow_reinclude=TRUE
    #define OPTION=1
    #include "option.glm"
    #set OPTION=2
    #include "option.glm"

In file `option.glm`
    
    
    #print Option = $OPTION


### Autoglobals

Automatic globals have no value associated with them but they otherwise appear to be defined. The autoglobals are typical defined when GridLAB-D executable is compiled and can be used to determine which options were compiled into the current executable. 

Autoglobal | Description | Syntax
-- | -- | --
WINDOWS | Always defined on Microsoft Windows platforms. | #ifdef WINDOWS
APPLE | Always defined on Apple Mac OS X platforms. | #ifdef APPLE
LINUX | Always defined on Linux platforms. | #ifdef LINUX
DEBUG | Always defined when _DEBUG option enabled during compile. | #ifdef DEBUG
MATLAB | Always defined when Matlab was available during compile. | #ifdef MATLAB
XERCES | Always defined when Xerces-C was available during compile. | #ifdef XERCES
CPPUNIT | Always defined when Cppunit was available during compile. | #ifdef CPPUNIT


### Runtime Compiler Support

#### CXX

The default C++ compiler and linker for runtime classes is g++. If you specify an alternative compiler, you may need to also set **CXXFLAGS** and **LDFLAGS** appropriately

    #setenv CXX=path-to-C++-compiler
    /bin/bash$ export CXX=path-to-C++-compiler

#### CXXFLAGS

The default C++ runtime class compiler options are as follows: 

Option | Description
-- | --
`-w` | No warning messages generated during compilation of classes.
`-I/usr/local/share/gridlabd` | GridLAB-D data folder. On cygwin systems this automatically converted to `-Ic:/mingw/msys/1.0/local/share/gridlabd`.
`-O0` | No optimization

If **CXXFLAGS** is set to anything (including an empty string), compiler warnings are enabled. If you wish to keep warnings suppressed you should include the -w option in addition to the other options you include.

If debug output is enabled, the `-g` optional to enable compiling with debugging symbols is added automatically. If you wish to enable debugging without debug output generated, you must add the `-g` option to both **CXXFLAGS** and **LDFLAGS**.

You can added additional include paths using the `-I path` option.

Any use of the `-O level` option will override the default optimization. This is not advised, especially if you are debugging.

## Exporting functions from runtime classes

You can publish a function from a runtime class by using the following syntax.

    class example1 {
      export myfunction(void *arg1, ...)
      {
        // your code goes here
        return result; // int64 value is returned
      }
    }

and access that function from another class using the syntax:

    class example2 {
      function example1::myfunction;
      intrinsic sync(TIMESTAMP t0, TIMESTAMP t1)
      {
          // your code goes here
          int64 result = myfunction(arg1, arg2);
          return t2;
      }

## Debugging Runtime Classes

To debug runtime class behavior, you must install gdb on your system. The debugger does not appear to be fully functional at this time. As more is learned about its limitations and workarounds, tip and tricks will be posted at Runtime debugging.

Windows users can debug with MS Visual Studio 2005TM. To enable this debugger the following environment variables should be set

    // set this to your MSVC installation folder
    #setenv MSVC=C:/Program Files/Microsoft Visual Studio 8

    // enable use of MSVC instead of GNUtools
    #define use_msvc=1

    // this disables automatic rebuild suppression of runtime classes 
    // compilation based on modification time
    #set force_compile=1

    // customize to your local setup
    #setenv path=${MSVC}/Common7/IDE;${MSVC}/VC/bin
    #setenv include=${MSVC}/VC/include
    #setenv lib=${SystemRoot}/system32;${MSVC}/VC/lib;${MSVC}/VC/PlatformSDK/Lib`

You can get all these settings by simply including the debugger configuration file:

    #include "debugger.conf"

You should set the debug program to the debug version of gridlabd.exe which is usually placed in the `$GRIDLABD/VS2005/win32/debug` folder. Once you have done this, you can set breakpoints in the C++ code block of your .glm files and the debugger will stop and offer most of the usual debugging features needed to debug your runtime class.
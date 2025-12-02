# Macros

Macros are used by the GridLAB-D™ GLM loader to control how the GLM is interpreted. All macros are introduced by a hash mark, e.g., 
    
    
    #if _test_
    // conditional 
    #endif
    
Macro | Description
-- | --
**#binpath _path_** | Define the path to search for executables.  **DEPRECATED** 
**#debug _string_** | Prints _string_ when debugging is enabled. 
**#define _variable_ =_value_** | Define new macro variables.
**#endif** | End an `#if`, `#ifdef`, or `#ifndef` block.
**#else** | Open an alternative block after an `#if`, `#ifexist`, `#ifdef`, or `#ifndef` block.
**#error _message_** | Force the GLM loader to print an error message and stop.
**#if _test_** | Open an #if block.
**#ifdef _variable_** | Open an `#ifdef` block.
**#ifexist _file_** | Open an `#ifexist` block.
**#ifndef _variable_** | Open an `#ifndef` block.
**#include using(_name_ =_value_,...) _file_** | Include another GLM file.
**#incpath _path_** | Define the path to search for include files.  **DEPRECATED** 
**#libpath _path_** | Define the path to search for library files.  **DEPRECATED** 
**#option _command-option_** | Runs a command option 
**#print _message_** | Print a message.
**#set _variable_ =_value_** | Set global variables.
**#setenv _variable_ =_value_** | Set environment variables.
**#start _file-name_** | Start a program asynchronously.
**#system _command_** | Execute a command in an operating system shell.
**#warning _message_** | Display a warning message and continue loading the GLM file.
**#wget _url_** | Download a web resource from the specified URL. 

# Variables

Macro variables are expanded in-line while the GLM file is being loaded, so expansion is immediate. The normal syntax for defining and including a macro variables is 
    
    
    #define my_class=test
    class ${my_class} {
      // declarations
    }
    

Macro expansions will expand macro variables, global variables, and environment variables. 

# Related Concepts:

  * Environment variables
  * GLM syntax
    * Comments
    * Directives
  * Global variables


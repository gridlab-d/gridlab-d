# Link (directive)

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
  
	**This page does not reflect the current state of GridLAB-D™**

An external application link can be added during the main synchronization process by adding a link directive to a GLM file. Multiple links can be added, in which case the links are processed in the order in which they are listed in the GLM file. 

The implementation of the link depends on the application, but as a general rule the link behavior is as follows: 

* **Initialization** -
    The external application is started and if possible global variables, if any, are exported.

* **Synchronization** -
    If possible specified values, if any, are copied and the external application entry point is called. The timestamp, if any, if copied back. If none is specified, NEVER is assumed.

* **Shutdown** -
    If possible, the application is shut down.

Example:

    link control_file;

## Link control file

All link control files support the following commands: 

Command | Description
-- | --
**target**  | Use the `target` option to establish the target application to link with. GridLAB-D™ will search for the dynamic link library (e.g., .dll or .so file) to load and use to interface with the application. On windows systems, the DLL must be named `glx target.dll` and must be found in the GridLAB-D™ path. On linux and MacOSX system, the DLL must be named `libglx target.so` and must be found in the GridLAB-D™ path.
**global**  | Use the `global` option to enumerate which global variables should be copied to the target application. If none are listed, the default is to send all the globals.
**object**  | Use the `object` option to enumerate which objects should be copied to the target application. If none are listed, the default is to send all the objects.
**export**  | Use the `export` option to enumerate which object properties are to be copied to the target application and under what name.
**import**  | Use the `import` option to enumerate which object properties are to copied from the target application and from what name. Imported variables are automatically exported.
**on_error**  | Use the `on_error` option to control how link error are handled. Possible values are ABORT, RETRY, IGNORE.
**ABORT**  | Use the `on_error ABORT` option to indicate that the link should stop the simulation when an error occurs by returning INVALID to the core's sync process (which causes the sync loop to exit). This is the default behavior.
**RETRY**  | Use the `on_error RETRY` option to indicate that the link should stop the simulation when an error occurs by returning the current time to the core's sync process (which causes another iteration).
**IGNORE**  | Use the `on_error IGNORE` option to indicate that the link should stop the simulation when an error occurs by returning NEVER to the core's sync process (which causes it to continue without using any data from the link).

## Performance instrumentation

When verbose is enabled, a performance measurement of each link is output at the end of the simulation. For example the output:
    
    
      ... link 'json' performance statistics {
      ...   packets count............ 11
      ...   mean response time....... 289.5 ms
      ...   stdev response time...... 92.4 ms
      ...   min response time........ 214.0 ms
      ...   response pareto k........ 5.6
      ...   max response time........ 548.0 ms
      ...   bad/lost packets......... 0
      ...   }
    

provides basic information about the link performance. 

## See also

  * Link (directive)
    * Matlab link
    * JSON link Template:NEW30
    * Technical manual
  * **How To**
    * How to plot data using Matlab
    * How to create a movie

# Simulation Options

This page outlines the different ways to run GridLAB-D™, using either the command line or server mode. The various command line options available to users are described, with look-up tables provided for easy reference.

## Installation Notes

    * For installation instructions, refer to the [Installation Guide](../../2.0%20-%20New%20Users/2.1%20-%20Installation%20Guide.md).

  * Note that minimally, the environment variable `GLPATH` should be set to the directory where GridLAB-D™ is installed.

If `runtime classes` are being used, please observe the following. 

  * MinGW must be installed (MinGW does not work on 64-bit Windows at this time)


  * GridLAB-D™ uses the temporary folder `HOMEDRIVE\HOMEPATH\Local Settings\Temp\gridlabd` to store files associated with runtime classes (source files, object files). This will cause errors if this folder does not exist or you do not have permission to write to this folder. If a different directory is desired for temporary files, the environment variable `GLTEMP` can be created and its value set to the path to an existing temporary directory you can write to. Alternatively, to set the path to the temporary directory on a case by case basis, the following line of code may be added to each GLM file:    
    
        #set tmp="path to temporary directory"
    

  * Note that there must be no spaces between "tmp" and "=" and also between "=" and the path.


  * Also note that it is best to enclose the path in double quotes.


## Running Simulations

The GridLAB-D™ software can be run using the _command-line_ or the _server mode_. 

### **GridLAB-D™ Command-Line Tool**

GridLAB-D™ takes the path to a GridLAB-D™ model file (GLM file) as input. For more information about creating GLM files refer to the guide to [Creating GLM Files](../../2.0%20-%20New%20Users/Tutorial/2.5.2%20-%20GLM%20Models.md). 

**Syntax Highlighting**

The syntax highlighting rules for C++ work well for GLM files. To set the syntax highlighting language, click `Language` -> `C` -> `C++`. The current syntax highlighting rules are available in the [SourceForge repository](http://GridLAB-D™.svn.sourceforge.net/viewvc/gridlab-d/) for the file [gridlabd.syn](http://GridLAB-D™.svn.sourceforge.net/viewvc/gridlab-d/trunk/core/rt/gridlabd.syn). A more complete, custom set of syntax highlighting rules is being developed and will be available soon. To import these settings, click `View` - > `User-Defined Dialogue...` In the Window that pops up, click `Import...` and select the file from above. To apply these settings to a file click `Language` -> `GLM`. Note that for files with the ".glm" extension, these syntax highlighting rules will be applied automatically. 

**NppExec**

The plug-in NppExec allows the user to run console commands from within Notepad++. To install this plug-in click `Plugins` - > `Plugin Manager` -> `Show Plugin Manager`. In the window that pops up, click on the `Available` tab and find the `NppExec` plug-in from the list. Click the check-box next to `NppExec` and then click `Install`. Once NppExec is installed click `Plugins` - > `NppExec` -> `Execute`. In the `Execute...` window, type the following command:   


    cmd /c cd "$(CURRENT_DIRECTORY)" && "gridlabd.exe" "$(FULL_CURRENT_PATH)" 


This command will change to the directory of the currently opened file, then run GridLAB-D™ with the current file as input. Next, click `Save...`, type in `GridLAB-D` as the script name, and click `Save` again. To run the command on the currently opened file click `OK`. To subsequently run this command, simply click `F6` , select the correct script, and click `OK`. To run the same script that was previously run, simply click `Ctrl + F6`. GridLAB-D™ will be executed, with the current file as input and the output will be shown on a console window at the bottom of the Notepad++ window. 

It is useful to define several such scripts, for example, to run GridLAB-D™ with verbose output:   


    cmd /c cd "$(CURRENT_DIRECTORY)" && "gridlabd.exe" -v "$(FULL_CURRENT_PATH)"


or to print profiling information:   


    cmd /c cd "$(CURRENT_DIRECTORY)" && "gridlabd.exe" --profile "$(FULL_CURRENT_PATH)" 


or to redirect output:   


    cmd /c cd "$(CURRENT_DIRECTORY)" && "gridlabd.exe" --redirect all "$(FULL_CURRENT_PATH)"


Each of these commands can be saved and the appropriate command can be selected from the `Plugins` - > `NppExec` -> `Execute...` window. 

To further speed things up, another option may be enabled which automatically saves the current file before executing the command. To toggle this option click `Plugins` - > `NppExec` -> `Save all files on execute`. 

#### **Command Line Arguments**

The command-line argument module processes arguments as they are encountered. 

The following command-line options are supported 


#### Table 1: Basic Command Line Options

Option | Description
-- | --
`-W\|--workdir path ` | Sets the working directory for the remainder of the run.
`--quiet \| -q` | Toggles display all messages except error and fatal messages.
`--verbose \| -v` | Toggles display of verbose messages. Verbose messages can be useful in understanding why certain error or warning occur.
`--warn \| -w` | Toggles display of warning messages. Warning messages relate to problems that might affect results.
`--debug` | Toggles display of debug messages. Debugging messages are highly detailed messages about the internal state of the simulation.
`--debugger` | Enables the debugger and turns on debug messages.
`--dumpall` | Enables a complete model dump when the simulation exits.
`--output file \| -o file` | Directs model output to the specified file.
`--profile` | Enables performance profiling of the model and displays profile output when the simulation exits.
`--check` | Enables calls to module check functions before the simulation starts. This can be used to detect models errors, but not all modules support such check functions. See --libinfo for details on module functions.

The following command-line processes can be called 

#### Table 2: Global and Module Control Options

Option | Description
-- | --
`--define name=value \| -D name=value` | Defines a global variable
`--globals` | Displays the global variables and their values
`--libinfo module \| -L module` | Displays information about a module, including API version, classes defined, functions implemented and global variables.

!!! example

        host% gridlabd --libinfo|-L module_name

    The information displayed relates to the following capabilities that may be implemented by a module 

    * Version:
        The major and minor version relate to the API level supported by the module. The major version is changed when features that are not backward compatible are altered. The minor version is changed when features that are backward compatible are changed. In other words, a module can always be loaded only if the major version is the same, however only a module with a same or higher minor version number that GridLAB-D's module API can be loaded.

    * Classes:
        A list of the implemented classes is displayed.

    * Implementations:
        A list of exported functions and support function is displayed.

    * Globals:
        A list of the module's global variables is displayed.


#### Table 3: Informational Options

Option | Description
-- | --
`--version \| -V` | Displays the full version/build number.
`--license` | Displays the software license.
`--copyright` | Displays the copyright.

#### Table 4: Test Processes Options

Option | Description
-- | --
`--dsttest` | Performs a daylight saving time definitions in tzinfo.txt
`--endusetest` | Performs a test of the end use pseudo-objects
`--globaldump` | Perform a global dump of the system and immediately exits.
`--loadshapetest` | Performs a test of the loadshape pseudo-objects
`--locktest` | Performs memory locking test
`--modtest module` | Performs the module self-test for the specified module
`--randtest` | Performs a test of the random number generators
`--scheduletest` | Performs a test of the schedule pseudo-objects
`--test` | Perform all the internal core self-test routines
`--testall file` | Performs module selftests of modules those listed in a file.
`--unitstest` | Performs a test of the units in unitfile.txt
`--validate` | Perform model validation check 

#### Table 5: File and I/O Formatting Options

Option | Description
-- | --
`--xmlencoding num` | Sets the XML encoding (8, 16, or 32)
`--xmlstrict` | Toggles XML to be strict, which is needed for compliance with certain XML loaders.
`--stream` | Enables streaming I/O (binary I/O instead of GLM/XML)
`--xsd module[:object]` | Prints the XSD of a module or object.
`--xsl modulelist` | Creates the XSL for the modules listed.
`--kml=file` | Output the KML (Google Earth) file of model (only supported by some modules).


#### Table 6: Help Options

Option | Description
-- | --
`--example module:class ` | Output an example of GLM code that will create a object of class given from the module given .
`--help \| -h` | Command line help.
`--info keyword ` | Open a browser and searches these documentation for the keyword given. Spaces may be entered as underscores in keywords .
`--modhelp module[:class]` | Output the GLM definition of class from module. All the classes from the specified module will be listed in alphabetical order if no class is given.

#### Table 7: Process Control Options

Option | Description
-- | --
`--threadcount n\| -T n` | Changes the number of threads to use during simulation (0 means as many as useful, default is 1)
`--clearmap` | DEPRECATED 
`--pclear` | Clears the processor map of defunct processes 
`--pcontrol ` | Enter interactive process control 
`--pkill n ` | Kills job n in the process map 
`--pstatus ` | Displays the processor status 

#### Table 8: System Options

Option | Description
-- | --
`--checkversion` | Perform online version check to see if any updates are available (as of 3.0).
`--compile` | Enables compile-only mode (the GLM file is loaded but the simulation does not start)
`--relax` | Allows implicit variable definition when assignments made
`--pause` | Enable pause at exit (waits for user input before exiting)
`--bothstdout` | Sends all output to stdout
`--check \| -c` | Run global checks of models (only supported by some modules)
`--avlbalance` | Controls automatic balancing of object index
`--output file \| -o file` | Saves dump output to file (default is **gridlabd.glm**)
`--environment app \| -e app` | Starts the app as the processing environment (default is **batch**). Recognized environments are **matlab**, **html**, **gui**, and **X11**. All but **batch** are experimental or under development.


#### Table 9: Job Control Options

Option | Description
-- | --
`--job ` | Runs all the GLM files found in the current folder as a single job .

### GridLAB-D™ Server Mode

The server mode was introduced in version 2.0 as a supporting feature of the Realtime server. Currently, the server mode is used in the gui development work. The implementation of server mode is done in ` core/server.c ` file. 

The server mode allows a web-based application to access the global variable and properties of named objects. 

To start GridLAB-D™ in server mode, simply include the command line argument `--server`, e.g., 

    host% gridlabd _modelname_.glm --server

The port used in server mode is 6267 and was assigned by Internet Assigned Numbers Authority (IANA) in December 2010. . It can be change in a GLM file using the directive 

    #set server_portnum=6267

To get the value of a global variable, use the following query 

    host% wget http://_host.domain_ /_variable-name_ -q -O -

To get the value of an object property, use the following query 

    host% **wget http://_host.domain_ /_object-name_ /_property-name_ -q -O -

To set the value of an object property, use the following query 

    host% **wget http://_host.domain_ /_object-name_ /_property-name_ =_value_ -q -O -

The value can include units (separate by a space) and they will be converted automatically. The value is read back after it is set to confirm that it was accepted (including unit conversion). 

When server mode is used for running GridLAB-D™, the following steps are performed: 

  * start up server environment and the server
  * create a new socket
  * bind the socket to the server address
  * server listens to the port for connections
  * accept client request and get its address; functions that receive and send data from/to the client are included in this procedural step.
  * process client's request;

    * The client's request falls in one of the categories: "/xml/...", "/runtime/...","/gui/...", "/output/...", "/action/...", "/rt/...", "/perl/...", "/gnuplot/...", "/java/...", "/python/...", "/r/...", "/scilab/...", "/octave/...".


For instance, read and write any global or object property can be done as follows:      

  * global property read uses the syntax ` <http://localhost/xml/property-name>`.   

  * write uses the syntax ` <http://localhost/xml/property-name=value>`.   

  * object property read uses the syntax ` <http://localhost/xml/object-nameproperty-name>`.   

  * write uses the syntax `<http://localhost/xml/object-name/property-name=value>`.  
  
  * send the results back to the client.

  * shutdown server when done or connection lost.

Running server-side scripts of various types is also an available feature of GRIDLAB-D™ server mode. The general syntax is <http://localhost/language/script-name>, where language is r (extension `.r`), scilab (extension `.sce`), perl (extension `.pl`), python (extension `.py`), octave (extension `.m`), java (extension `.jar`), gnuplot(extension `.plt`). The set of available languages will be soon expended even more.   


!!! note

    To execute a Java script:
        
        http://server:port/java/filename.jar
        
    
    To execute an Octave script:

        http://server:port/octave/filename.m

    The specified _filename_ must exist on the server. The stdout and stderr are sent to the server's output streams. The output file is sent to the client as MIME-type content. 


To retrieve output files, such as CSV files, is also available in GridLAB-D™ server mode by using the syntax <http://localhost/output/filename.ext>.   

The work done for the gui capability enables generation of HTML code by using the syntax <http://localhost/gui/pagename.ext>. 

All the features presented above are in early stages of development and under constinuous improvement. Example of concept tests are presented in ` core/test/gui_example...`. The examples are not fully functional at this point because we are trying to understand the limitations of each on various platforms. 

#### Table 10: Server Mode Command Options

Option | Description
-- | --
`--pidfile[=filename]` | Creates a process id file while GridLAB-D™is running (default is gridlabd.pid). Note: this is only supported in POSIX platforms.
`--redirect stream[:file]` | Redirects output stream to file (or null). Valid streams are **output**, **error**, **warning**, **debug**, **verbose**, **profile**, **progress**, **none** and **all**.
`--server` | Runs in server mode (uses **pidfile** and redirects all output)
`--server_portnum n \| -P n` | Sets the server port number (default is 6267)


#### Control

When operating in server mode, GridLAB-D accepts main loop state control messages from HTTP clients connected on the server's port. All control messages are in the form of HTTP 1.1 requests with the standard query format:

     http://server :port /control/command

The following control actions are recognized:


#### Table 11: Control Actions

Action | Description
-- | --
`/control/resume` | Resumes a simulation that is in a PAUSED state.
`/control/pauseat=datetime` | Resumes a simulation that is in a PAUSED state as long as the timestamp datetime is greater than the current clock time. The simulation will pause again when the clock reached the datetime.
`/control/shutdown` | Initiates an immediate server shutdown.


#### **Level 1 Requirements**

All Level 1 requirements require Project Manager approval to be adopted. 

##### **Platforms supported**

The server mode shall be accessible on all the existent platforms (i.e., Windows, Linux, Mac). 

##### **Presentment encoding**

The server mode layout shall be encoded so that it is driven by the GLM file. Ancillary files may be referenced by the GLM file. 

##### **Input entities**

All server mode input entities functionality shall be implemented by the core. 

##### **Output entities**

?. 

##### **Client/server model**

A client/server model over a TCP port registered with IANA is used. 

##### **Communication protocol**

The server shall support HTTP 1.1 traffic according to [RFC2616](http://www.w3.org/Protocols/rfc2616/rfc2616.html). All major clients conforming to HTTP 1.1 shall be supported. 

#### **Level 2 Requirements**

#### **Level 3 Requirements**

#### **Level 4 Requirements**

### Exit Codes

The GridLAB-D core solver and most GridLAB-D modules use the following exit code: 

  * -1 exec failed
  * 0 success
  * 1 bad command line
  * 2 environment startup failed
  * 3 test failed
  * 4 user rejected terms of use
  * 5 simulation did not complete as desired
  * 6 initialization failed
  * 7 process control error
  * 8 server killed
  * 9 I/O error
  * 127 shell failure
  * 128-254 signal received (low order bits are signal number)
  * 255 exception caught

!!! caveat

    Some libraries used by GridLAB-D module do not following the GridLAB-D exit code standard. 

    Some systems cannot distinguish between -1 and 255.


  

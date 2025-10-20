# Command Options
The command-line argument processing module processes arguments as they are encountered.

**Note:** some modules can process command arguments as well. Those options are not listed here.

## Command-line options

`-W|--workdir path `

Sets the working directory for the remainder of the run.

`--quiet | -q`

Toggles display all messages except error and fatal messages.

`--verbose | -v`

Toggles display of verbose messages. Verbose messages can be useful in understanding why certain error or warning occur.

`--warn | -w`

Toggles display of warning messages. Warning messages relate to problems that might affect results.

`--debug`

Toggles display of debug messages. Debugging messages are highly detailed messages about the internal state of the simulation.

`--debugger`

Enables the debugger and turns on debug messages.

`--dumpall`

Enables a complete model dump when the simulation exits.

`--output file | -o file`

Directs model output to the specified file.

`--profile`

Enables performance profiling of the model and displays profile output when the simulation exits.

`--check`

Enables calls to module check functions before the simulation starts. This can be used to detect models errors, but not all modules support such check functions. See --libinfo for details on module functions.


## Global and module control

`--define name=value | -D name=value`

Defines a global variable

`--globals`

Displays the global variables and their values

`--libinfo module | -L module`

Displays information about a module, including API version, classes defined, functions implemented and global variables.


## Information

`--version | -V`

Displays the full version/build number.

`--license`

Displays the software license.

`--copyright`

Displays the copyright.


## Test processes

`--dsttest`

Performs a daylight saving time definitions in tzinfo.txt

`--endusetest`

Performs a test of the end-use pseudo-objects

`--globaldump`

Perform a global dump of the system and immediately exits.

`--loadshapetest`

Performs a test of the loadshape pseudo-objects

`--locktest`

Performs memory locking test

`--modtest module`

Performs the module self-test for the specified module

`--randtest`

Performs a test of the random number generators

`--scheduletest`

Performs a test of the schedule pseudo-objects

`--test`

Perform all the internal core self-test routines

`--testall file`

Performs module selftests of modules those listed in a file.

`--unitstest`

Performs a test of the units in unitfile.txt

`--validate`

Perform model validation check 


## File and I/O Formatting

`--xmlencoding num`

Sets the XML encoding (8, 16, or 32)

`--xmlstrict`

Toggles XML to be strict, which is needed for compliance with certain XML loaders.

`--stream`

Enables streaming I/O (binary I/O instead of GLM/XML)

`--xsd module[:object]`

Prints the XSD of a module or object.

`--xsl modulelist`

Creates the XSL for the modules listed.

`--kml=file`

Output the KML (Google Earth) file of model (only supported by some modules).


## Help

`--example module:class `

Output an example of GLM code that will create a object of class given from the module given .

`--help | -h`

Command line help.

`--info keyword `

Open a browser and searches these Wiki docs for the keyword given. Spaces may be entered as underscores in keywords .

`--modhelp module[:class]`

Output the GLM definition of class from module. All the classes from the specified module will be listed in alphabetical order if no class is given.


## Process control

``--threadcount n| -T n`

Changes the number of threads to use during simulation (0 means as many as useful, default is 1)

`--clearmap` DEPRECATED 

`--pclear`

Clears the processor map of defunct processes 

`--pcontrol `

Enter interactive process control 

`--pkill n `

Kills job n in the process map 

`--pstatus `

Displays the processor status 


## System options

`--checkversion`

Perform online version check to see if any updates are available (as of 3.0).

`--compile`

Enables compile-only mode (the GLM file is loaded but the simulation does not start)

`--relax`

Allows implicit variable definition when assignments made

`--pause`

Enable pause at exit (waits for user input before exiting)

`--bothstdout`

Sends all output to stdout

`--check | -c`

Run global checks of models (only supported by some modules)

`--avlbalance`

Controls automatic balancing of object index

`--output file | -o file`

Saves dump output to file (default is **gridlabd.glm**)

`--environment app | -e app`

Starts the app as the processing environment (default is **batch**). Recognized environments are **matlab**, **html**, **gui**, and **X11**. All but **batch** are experimental or under development.


## Server mode

`--pidfile[=filename]`

Creates a process id file while GridLAB-D™is running (default is gridlabd.pid). Note: this is only supported in POSIX platforms.

`--redirect stream[:file]`

Redirects output stream to file (or null). Valid streams are **output**, **error**, **warning**, **debug**, **verbose**, **profile**, **progress**, **none** and **all**.

`--server`

Runs in server mode (uses **pidfile** and redirects all output)

`--server_portnum n | -P n`

Sets the server port number (default is 6267)


## Job control

`--job `

Runs all the GLM files found in the current folder as a single job .
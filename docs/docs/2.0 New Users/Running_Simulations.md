# Running Simulations
TODO: Turn this into a new-user example of how to run a simple simulation. Move more in-depth content to later sections where appropriate.

## Installation Notes

  * For installation instructions, refer to the [Installation Guide].


  * Note that minimally, the environment variable `GLPATH` should be set to the directory where GridLAB-D is installed.
If [runtime classes] are being used, please observe the following. 

  * MinGW must be installed (MinGW does not work on 64-bit Windows at this time)


  * GridLAB-D uses the temporary folder `HOMEDRIVE\HOMEPATH\Local Settings\Temp\gridlabd` to store files associated with runtime classes (source files, object files). This will cause errors if this folder does not exist or you do not have permission to write to this folder. If a different directory is desired for temporary files, the environment variable `GLTEMP` can be created and its value set to the path to an existing temporary directory you can write to. Alternatively, to set the path to the temporary directory on a case by case basis, the following line of code may be added to each GLM file:    
    
        #set tmp="path to temporary directory"
    

  * Note that there must be no spaces between "tmp" and "=" and also between "=" and the path.


  * Also note that it is best to enclose the path in double quotes.


## Running Simulations

The GridLAB-D software can be run using the _command-line_ or the _server mode_. 

### **GridLAB-D Command-Line Tool**

GridLAB-D takes the path to a GridLAB-D model file (GLM file) as input. For more information about creating GLM files refer to the guide to [Creating GLM Files]. 

**Syntax Highlighting**

The syntax highlighting rules for C++ work well for GLM files. To set the syntax highlighting language, click _**Language - > C -> C++**_. The current syntax highlighting rules are available in the [SourceForge repository](http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/) for the file [gridlabd.syn](http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/trunk/core/rt/gridlabd.syn). A more complete, custom set of syntax highlighting rules is being developed and will be available soon. To import these settings, click _**View - > User-Defined Dialogue...**_. In the Window that pops up, click _Import..._ and select the file from above. To apply these settings to a file click _**Language - > GLM**_. Note that for files with the ".glm" extension, these syntax highlighting rules will be applied automatically. 

**NppExec**

The plug-in NppExec allows the user to run console commands from within Notepad++. To install this plug-in click _**Plugins - > Plugin Manager -> Show Plugin Manager**_. In the window that pops up, click on the _**Available**_ tab and find the _**NppExec**_ plug-in from the list. Click the check-box next to _**NppExec**_ and then click _**Install**_. Once NppExec is installed click _**Plugins - > NppExec -> Execute**_. In the _**Execute...**_ window, type the following command:   


    `cmd /c cd "$(CURRENT_DIRECTORY)" && "gridlabd.exe" "$(FULL_CURRENT_PATH)" `  


This command will change to the directory of the currently opened file, then run GridLAB-D with the current file as input. Next, click _**Save...**_ , type in "GridLAB-D" as the script name, and click _**Save**_ again. To run the command on the currently opened file click _**OK**_. To subsequently run this command, simply click **F6** , select the correct script, and click _**OK**_. To run the same script that was previously run, simply click **Ctrl + F6**. GridLAB-D will be executed, with the current file as input and the output will be shown on a console window at the bottom of the Notepad++ window. 

It is useful to define several such scripts, for example, to run GridLAB-D with verbose output:   


    `cmd /c cd "$(CURRENT_DIRECTORY)" && "gridlabd.exe" -v "$(FULL_CURRENT_PATH)" `  


or to print profiling information:   


    `cmd /c cd "$(CURRENT_DIRECTORY)" && "gridlabd.exe" --profile "$(FULL_CURRENT_PATH)" `  


or to redirect output:   


    `cmd /c cd "$(CURRENT_DIRECTORY)" && "gridlabd.exe" --redirect all "$(FULL_CURRENT_PATH)" `  


Each of these commands can be saved and the appropriate command can be selected from the _**Plugins - > NppExec -> Execute...**_ window. 

To further speed things up, another option may be enabled which automatically saves the current file before executing the command. To toggle this option click _**Plugins - > NppExec -> Save all files on execute**_. 

#### **Command Line Arguments**

The command-line argument module processes arguments as they are encountered. 

The following command-line toggles are supported 

  * **\--warn** toggles the warning mode
  * **\--check** toggles calls to module check functions
  * **\--debug** toggles debug messages
  * **\--debugger** enables the debugger and turns on debug messages
  * **\--dumpall** toggles a complete model dump when the simulation exits
  * **\--quiet** toggles all messages except error and fatal messages
  * **\--profile** toggles performance profiling
The following command-line processes can be called 

  * **\--license** prints the software license
  * **\--dsttest** performs a daylight saving time definitions in tzinfo.txt
  * **\--unitstest** performs a test of the units in unitfile.txt
  * **\--randtest** performs a test of the random number generators
  * **\--testall** file performs module selftests of modules those listed in file
  * **\--test** run the internal core self-test routines
  * **\--define** define a global variable
  * **\--libinfo _module_** prints information about the module
  * **\--xsd _module_[:_object_]** prints the xsd of a module or object
  * **\--xsl** creates the xsl for this version of gridlab-d
  * **\--kml=_file_** output kml (Google Earth) file of model
  * **\--modhelp _module_[:_class_]** output definition of _class_ from _module_. All the classes from the specified module will be listed in alphabetical order if no class is given
  * **\--server** runs in server mode (uses **pidfile** and redirects all output)
  * **\--pidfile[=_filename_]** creates a process id file while GridLAB-D is running (default is gridlabd.pid)
  * **\--redirect _stream_[:_file_]** redirects output stream to file
The following system options may be changed 

  * **\--threadcount _n_** changes the number of thread to use during simulation (default is 0, meaning as many as useful)
  * **\--output _file_** saves dump output to file (default is **gridlabd.glm**)
  * **\--environment _app_** start the app as the processing environment (default is **batch**)
  * **\--xmlencoding _num_** sets the XML encoding (8, 16, or 32)
  * **\--xmlstrict** toggles XML to be strict
  * **\--relax** allows implicit variable definition when assignments made
### **GridLAB-D Server Mode**

The server mode was introduced in version 2.0 as a supporting feature of the Realtime server. Currently, the server mode is used in the gui development work. The implementation of server mode is done in ` core/server.c ` file. 

The server mode allows a web-based application to access the global variable and properties of named objects. 

To start GridLAB-D in server mode, simply include the command line argument `--server`, e.g., 

    ` host% **gridlabd _modelname_.glm --server** `

The port used in server mode is 6267 and was assigned by Internet Assigned Numbers Authority (IANA) in December 2010. . It can be change in a GLM file using the directive 

    `**#set server_portnum=6267**`

To get the value of a global variable, use the following query 

    `host% **wget http://_host.domain_ /_variable-name_ -q -O -**`

To get the value of an object property, use the following query 

    `host% **wget http://_host.domain_ /_object-name_ /_property-name_ -q -O -** `

To set the value of an object property, use the following query 

    `host% **wget http://_host.domain_ /_object-name_ /_property-name_ =_value_ -q -O -**`

The value can include units (separate by a space) and they will be converted automatically. The value is read back after it is set to confirm that it was accepted (including unit conversion). 

When server mode is used for running GridLAB-D, the following steps are performed: 

    

  * start up server environment and the server
  * create a new socket
  * bind the socket to the server address
  * server listens to the port for connections
  * accept client request and get its address; functions that receive and send data from/to the client are included in this procedural step.
  * process client's request;
    The client's request falls in one of the categories: "/xml/...", "/runtime/...","/gui/...", "/output/...", "/action/...", "/rt/...", "/perl/...", "/gnuplot/...", "/java/...", "/python/...", "/r/...", "/scilab/...", "/octave/...".
    For instance, read and write any global or object property can be done as follows:   


    

  * global property read uses the syntax ` <http://localhost/xml/property-name>`.   

  * write uses the syntax ` <http://localhost/xml/property-name=value>`.   

  * object property read uses the syntax ` <http://localhost/xml/object-nameproperty-name>`.   

  * write uses the syntax `<http://localhost/xml/object-name/property-name=value>`.   * send the results back to the client.
  * shutdown server when done or connection lost.
Running server-side scripts of various types is also an available feature of GRIDLAB-D server mode. The general syntax is <http://localhost/language/script-name>, where language is r (extension _.r_), scilab (extension _.sce_), perl (extension _.pl_), python (extension _.py_), octave (extension _.m_), java (extension _.jar_), gnuplot(extension _.plt_). The set of available languages will be soon expended even more.   
Retrieve output files, such as CSV files, is also available in GRIDLAB-D server mode by using the syntax <http://localhost/output/filename.ext>.   
The work done for the gui capability enables generation of HTML code by using the syntax <http://localhost/gui/pagename.ext>. 

All the features presented above are in early stages of development and under constinuous improvement. Example of concept tests are presented in ` core/test/gui_example...`. The examples are not fully functional at this point because we are trying to understand the limitations of each on various platforms. 

  


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


  

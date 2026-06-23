# Command Line Options

The command line options are used to alter the mode of operation of GridLAB-D™. The normal setting for a mode of operation is called the default, and command line options are one way to override those defaults.

## Avlbalance

Another example, to control the use of automatic object index balancing routines. Normally, the object index is automatically rebalanced to prevent low entropy lists from creating inefficient search trees when the `object_tree_balance` global variable is set to **1** or **TRUE**. When it is set to **0** or **FALSE** the tree balancing is disabled.

    gridlabd --avlbalance filename.glm

The `object_tree_balance` global variable is used to control the use of automatic object index balancing routines. Normally, the object index is automatically rebalanced to prevent low entropy lists from creating inefficient search trees when the ` ` global variable is set to 1 or TRUE. When it is set to 0 or FALSE the tree balancing is disabled.

## Bothstdout

The `--bothstdout` command line option controls whether error messages are sent to the normal message stream. When specified the stderr stream is merged into the stdout stream.The command line usage is 
    
    
    host% gridlabd --bothstdout

## Browser

When GridLAB-D™ needs a web browser window, such as for the `--info` command line option or for the user interface it uses the browser global variable to start a web browser. The `browser` variable is used differently depending the platform. The command line usage is:

    gridlabd -D browser=program
    gridlabd --define browser=program

In a GLM file:

    #set browser=program

  
Table: Default browser by OS { #tbl:default-browser-by-os }

OS | Default Browswer
--| --
Windows | iexplore
Mac OSX | safari
Linux | firefox

## Check

The module check function is called whenever the --check command line option is given. The module is expected to perform data and model consistency checks to assist users in debugging problem with GLM files. It is managed by the global variable **runcheck**, which is by default **FALSE**. 

The command line usage is:
    
    host% gridlabd --check
  
* To enable warn mode use the option   
    
        host% gridlabd -D runcheck=1
    
* To disable warn mode use the option 
    
        host% gridlabd -D runcheck=0

In a GLM file:

* To enable check mode use the directive   
    
        #set runcheck=1
    
* To disable check mode use the directive 

        #set runcheck=0

Example:

    #include "gridlabd.h"
    EXPORT int check(void);
    {
      unsigned int errcount = 0;
      // TODO write your check code here
      return errcount;
    }

## Client allowed

Restrict internet address from which clients can connect to server. Any address that doesn't match the incoming address is denied. If the address is partial, the leading part of the address must match.

    #set client_allowed=address

For example, the address "127.0.0.1" matches "127.0." but does not match "128.0."

## Compile

The compile command line option instructs GridLAB-D™ to only load and compile the specified GLM file, but not run them.    
    
    host% gridlabd --compile file

## Compile Once

Control whether runtime classes are recompiled when no change is detected. When GridLAB-D™ loads a GLM file and encounters C/C++ source code, it invokes a compiler. If the GLM file is newer than the object code, the compiler is normally not invoked (`compile_once=TRUE`). When `compile_once==FALSE`, the compiler is invoke every time the GLM file is loaded. This can be important if compile flags or dependencies exist that the GLM loader does not recognize.



    host% gridlabd -D compile_once=TRUE|FALSE
    #set compile_once=TRUE|FALSE
        
## Compile Only

The compileonly global variable is used to indicate that the current file is to be loaded only and the simulation should not be run. The command line usage is:   
    
    host% gridlabd -D compileonly=1

In **the glm**:

    #set compileonly=1
    

## Define

The **define** command line option and the `#define` directive may be used to set global variables.

To define or set a global variable From the **command line**, you can use to available syntax options:

    host% gridlabd --define global =value

or

    host% gridlabd -D global =value


When defined a previously undefined global variable, use the following syntax: 
    
    #define variable =value
    

**Example:**
    
    
    #define DG2_MG2_RatedVA=600000
    
    object diesel_dg {
    	parent meter_135;
    	name Gen2;
    	Rated_V 4156; //Line-to-Line value
    	Rated_VA ${DG2_MG2_RatedVA};
           ...
    }

If you wish to set an existing global variable you must use the `#set` directive. Alternatively, you can set the **strictnames** global variable to **FALSE** to allow implicit creation of global variables without using set and allow **define** to overwrite already existing global variables.

## Double format

The **double_format** global variable controls the format to use when reading and write double precision numbers. The default double format is `%+lg`. The double format must be compatible with a double precision digit according the C-library implementation of printf and scanf. From the **command line**: 

    host% **gridlabd -D double_format="%.4le"**

In a glm:

    #set double_format="%.4le"

## Dsttest

The `--dsttest` command line option is used to test the daylight savings time rules. The daylight savings test checks that the difference between the second before and the second at which daylight savings goes into and out of effect is separate by only 1 second.

When the test is run the output is sent to the file indicated by **testoutputfile** global variable, which is set to `test.txt` by default. The output of the test is formatted to examine the rule for each year between 1970 and 2019. Each test will produce a one-line report such as

    2011-03-13 01:59:59 PST + 1.000000s = 2011-03-13 03:00:00 PDT

From the **command line**:

    host% **gridlabd --dsttest**

!!! note

    On some platforms, the local timezone cannot be automatically determined by the simulation. In such cases, the environment variable TZ must be set before the test will run. For example: 


        host% **export TZ=PST8PDT**


## Dumpall

The **dumpall** option produces a model dump output from GridLAB-D™ when the simulation is done. It is managed by the global variable **dumpall**, which is by default **FALSE**. From the **command line**:

* To toggle dumpall mode use the option 
    
        host% gridlabd --dumpall
    

* To enable dumpall mode use the option    
    
        host% gridlabd -D dumpall=1
    

* To disable dumpall mode use the option 
     
        host% gridlabd -D dumpall=0

In **the glm**:

* To enable dumpall mode use the directive 
     
        #set dumpall=1
    

* To disable dumpall mode use the directive 
    
        #set dumpall=0


## Error

The `#error` macro displays an error message and stop load the GLM file when it is encountered by the loader.

    #error message
    

Note that the message is not followed by a semicolon unless the semicolon is part of the message

## Force Compile

`The force_compile global variable is used to force the runtime class compiler to rebuild the executable for each class every time GridLAB-D™ is run. 
`
Normally, GridLAB-D™ only rebuilds the implementation file if one of the source files that defines it has changed since the last time the executable was built. However, there are cases when this automatic process does not work correctly or cannot detect changes in include files. 

From the **command line**: 

    host% gridlabd -D force_compile=1

In **the glm**:

    #set force_compile=1

## Global

The global directive is used to define a global variable.

    global type name value;
    global type name[units] value;

!!! bugs

    Currently there is no GLM syntax that allows users to define global variable keywords for sets and enumerations.

## GUID

The GUID global variable dynamically generates a unique 128-bit identifier each time it is referenced. This can be used to generate object names, file names, and database entities that are practically guaranteed to be unique.

The following code defines a class test with a random variable x. The name of the object is unique.

    class test {
    random x;
    }
    object test {
    name test-${GUID};
    x "type:normal(0,1); refresh:1min";
    }

!!! bugs

    Prior to Hassayampa (Version 3.0) The random number generated is seeded using the current system time with a resolution of 1 second. Consequently, if two runs are started within the same second they are very likely to generate the same sequence of unique ids.

## Iteration Limit

The `iteration_limit` global variable determines that maximum number of iterations of the main synchronization loop permitted before the clock must advanced. If the main loop reaches the iteration limit and the clock has not advanced, GridLAB-D™ will terminate the simulation with a convergence error.

From the command line, to set the iteration at the command line, use the syntax:

    host% gridlabd -D iteration_limit=100

In a glm, to set the iteration in a GLM file, use the syntax:

    #set iteration_limit=100

## Maximum Synctime

The maximum synctime global variable is used to control how long GridLAB-D™ waits for a single sync operation before raising a global alarm that halts the simulation. Under certain circumstances applications may use sync operations to acquire information and wait for data sources that take longer the 60 seconds to respond. In such cases, it may be desirable to increase the `maximum_synctime` to allow for long sync response delays.

    host% gridlabd -D maximum_synctime=60
    host% gridlabd -define maximum_synctime=60
    #set maximum_synctime=60

## Minimum Timestep

The minimum timestep is the shortest simulation time change allowed during a simulation. By default the minimum timestep is 1 second and for more simulation this will not cause any performance issues.

However, for certain kinds of very large simulations the result of a short `minimum_timestep` can be very slow progress. In such cases, an increase in the `minimum_timestep` can improve performance.

    %host gridlabd -D minimum_timestep=1
    %host gridlabd --define minimum_timestep=1
    #set minimum_timestep=1

!!! caveat

    There is one very important caveat that must be considered before using `minimum_timestep` to improve simulation performance. Increasing the `minimum_timestep` can cause the emergence of adverse effects such as state coherence. Under these circumstances, the state changes of object with time constants similar to the `minimum_timestep` can become highly coherent. This can lead to erroneous results if the diversity of state is an critical property of the aggregate simulation. Use of the `minimum_timestep` should only be considered after verification of whether a) state diversity is not a critical property and b) the time constants of objects are not on the order of the `minimum_timestep`.


## Modelname

The `modelname` global variable is a string that represents the name of the current model.

    #print ${modelname}

## Modhelp

Obtain detailed information about the implementation of the classes in a module.

    host% gridlabd --modhelp module
    host% gridlabd --modhelp module:class

!!! note

    Only properties that are published by modules without the PT_HIDDEN option will be displayed.

## Mt profile

The `mt_profile` option produces profiler output from GridLAB-D™ for multithreaded operation. It is managed by the global variable `mt_profile` , which is by default 0. The value of the `mt_profile` determine the maximum number of thread to analyze.

From the **command line**, to toggle profile mode use the option:

    host% gridlabd --mt_profile 64

To enable profile mode use the option

    host% gridlabd -D mt_analysis=64

To disable profile mode use the option

    host% gridlabd -D mt_analysis=0

In **the glm**, to enable profile mode use the directive:

    #set mt_analysis=64

To disable profile mode use the directive:

    #set mt_analysis=0

## No Deprecate

The no_deprecate global variable suppressed deprecated usage when set to 1 or TRUE. By default no_deprecate is set to 0 or FALSE.

Deprecated usage is flagged for any capabilities or features that are considered obsolete and will be removed in future versions of GridLAB-D™.

From the **command line**:

    host% gridlabd -D no_deprecate=1

In **the glm**:

    #set no_deprecate=1


## Nolocks

The nolocks global variable is used to prevent the object index from being shuffled.

    #set nolocks=1

## NOW

The NOW global variable dynamically generates a current system timestamp. This can be used to generate object names, file names, and database entities that are related the current system time.

The format of the timestamp is 
    
    
    YYYYMMDD-hhmmss

where 

  * _YYYY_ is the year (1970-2038)
  * _MM_ is the month of the year (01-12)
  * _DD_ is the day of the month (01-31)
  * _hh_ is the hour of the day (00-23)
  * _mm_ is the minute of the hour (00-59)
  * _ss_ is the second of the minute (00-59)

The timestamp is given only as UTC. 

### Example

The following code defines a class test with a random variable x. The name of the object is based on the timestamp.

    class test {
    random x;
    }
    object test {
    name test-${NOW};
    x "type:normal(0,1); refresh:1min";
    }

## Open

Request a GridLAB-D™ server load a GLM file: `http://server:port /open/filename.glm`

## Option

Macro to run a command option. Only command options that do not take parameters are supported at this time.

Not all command options are well suited to being dispatched from inside GLM files. In particular, certain command options modify models in unpredictable way, run destructive tests, and/or exit immediately upon completion

    #option command-option

## Output

Request download of the contents of an output file from a GridLAB-D™ server. The file requested is obtained from the workdir folder.

    http://server :port /output/filename.ext

## Pause

The --pause command option enables the pause-at-exit feature.

    host% gridlabd --pause

## Pauseatexit

The `pauseatexit` global variable enables a pause feature when GridLAB-D™ exits. Some shell automatically close when the gridlabd process exits and messages displayed are lost. Enabling this feature makes it possible to read those message before the shell is closed.

From the command line, 

    host% gridlabd -D pauseatexit=1

In a glm,

    #set pauseatexit=1

## Pclear

The global process map can become corrupted in the event of the failure of an instance of GridLAB-D™ that leaves a zombie entry in the process map. In such circumstance the **\--clearmap** command can be used to purge the map. 
    
    
    host% gridlabd --pclear


!!! tip

    Sometimes the system administrator doesn’t properly install GridLAB-D™ and forgets to add the `gridlabd` command to the system command search path. When this happens and you enter `gridlabd` at the command prompt, you will see some error message to the effect that the command cannot be found or is not recognized. If this occurs, contact the system administrator and ask to have the GridLAB-D™ installation fixed so that it works from your command line. Alternatively, you will need to provide the full path name to the GridLAB-D™ executable at the command prompt.

!!! note
    
    Some operating systems are not case sensitive, but GridLAB-D™ is always case sensitive. Therefore, even though it may be possible to type `GRIDLABD` as well as `gridlabd` at the command line, the command line options may still be case sensitive.

## Pcontrol

Interactive process control. 

    gridlabd --pcontrol
    
The process control window is a continuous interactive screen that shows all the current GridLAB-D™ simulations active on the local host. The display is typically as follows: 
    
    
    GridLAB-D™ Process Control - Version 3.0.0-4595 (Hassayampa)
    
    PROC PID   RUNTIME    STATE   CLOCK                   MODEL
    ---- ----- ---------- ------- ----------------------- -----------------------------------------------------------------
    **0 72169        34s Running 2000-01-03 22:29:49 UTC /Users/david/gridlabd_3.0/core ... test_groupid/test_groupid.glm**
       1 72563         6s Running 2000-01-01 15:53:27 UTC /Users/david/gridlabd_3.0/core ... test_groupid/test_groupid.glm
       2 72698         4s Running 2000-01-21 06:57:28 UTC /Users/david/gridlabd_3.0/comm ... ial_Qi/test_commercial_Qi.glm
       3   -
       4   -
       5   -
       6   -
       7   -
    -----------------------------------------------------------------------------------------------------------------------
    
    2012/08/19 15:36:54: Ready.
    C to clear defunct, Up/Down to select, K to kill, Q to quit: 
    

The columns in the listing are relative self-explanatory: 

  * **PROC** is the GridLAB-D™ process map entry number. It is assumed that there can be no more than one entry per CPU available on the host.
  * **PID** is the host process id. This is used to send signals to the process.
  * **RUNTIME** is the elapsed wall clock time since the simulation started.
  * **STATE** is the state of the simulation. Possible states are

    * **Init** - 
        Initialization in progress.
    * **Running** -
        Run in progress.
    * **Paused** -
        Paused for user or server I/O.
    * **Done** -
        Simulation completed.
    * **Locked** -
        Simulation locked for synchronization or event.
    * **Defunct** -
        Simulation no longer running but did not complete normally. These can be cleared using the 'C' key.

  * **CLOCK** is the current simulation clock time. All clock times are presented in UTC regardless of the timezone used by the simulation.
  * **MODEL** is the name of the model

The bolded entry is the selected entry. The up and down arrow keys allows the selection to be changed. Pressing the 'K' key kills the selected entry. 'C' will clear the process list of any defunct simulations that could be left from unexpected crashes. The 'Q' key will quit. The 'Ctrl-C' button will also quit.

!!! bugs

    The screen refreshes every second and this cannot be changed.

    Because the process map is sampled once every second, it is quite likely that a quick simulation might never show up on the list.

## Perl

Execute a PERL script on a GridLAB-D™ server. Only scripts already installed on the server may be executed. The script is executed with output connected to the server stdout stream.

    http://server:port/per/filename.pl

## Pf signed

Determines if a "signed" (leading/lagging) designation on the desired power-factor is accepted. The value for **pf_signed** is used for setting whether desired_pf can have a sign associated with it. If **pf_signed** is enabled, desired_pf is set up as a positive value for a capacitive system (leading) and a negative value for an inductive system (lagging).

The default value for **pf_signed** is **false**.

    module powerflow;
    class volt_var_control {
        bool pf_signed;
    }
    object volt_var_control {
        pf_signed value;
    }

## Pidfile

The `--pidfile` command line option is used to specify the name of the process id file.

    host% gridlabd --pidfile=filename

!!! note

    This is a linux/unix only feature.

## Pkill

A run associated with a processor can be killed and removed from the global process list using the `--pkill` option:

    host% **gridlabd --pkill 0**

## Platform

Specifies the current operating platform. The platform global variable is set according to which operating platform is running GridLAB-D™. The platform global variable cannot be set at runtime.

    #print ${platform}
    #if ${platform}=WINDOWS|LINUX|MACOSX
    ...
    #endif

Possible options include: WINDOWS, LINUX, or MACOSX

## Print

The `#print` macro displays a message when it is encountered by the loader. When the quiet global variable is set, the output is suppressed.

    #print message

!!! note

    The message is not followed by a semicolon unless the semicolon is part of the message.

## Profile

The profile option produces profiler output from GridLAB-D™. It is managed by the global variable profiler, which is by default **FALSE**.

From the command line, 

* to toggle profile mode use the option 
    
        host% gridlabd --profile
    

* To enable profile mode use the option   
    
        host% gridlabd -D profile=1
    

* To disable profile mode use the option   
    
        host% gridlabd -D profile=0

In a glm, 

* to enable profile mode use the directive 
      
        #set profile=1
    
* To disable profile mode use the directive   
    
        #set profile=0

## Profiler

The profiler measures simulation performance and output core and model performance statistics. The profiler is enabled using the profile command line option or the global variable profiler.

### Single-threaded profiles

The output from the profiler generally looks as follows for a single-threaded model: 
    
    
    Core profiler results
    ======================
    
    Total objects                101 objects
    Parallelism                    1 thread
    Total time                 117.0 seconds
      Core time                 29.8 seconds (25.4%)
        Compiler                 1.6 seconds (1.4%)
        Schedules                0.0 seconds (0.0%)
        Loadshapes               0.0 seconds (0.0%)
        Enduses                  2.9 seconds (2.4%)
        Transforms               0.1 seconds (0.1%)
      Model time                87.2 seconds/thread (74.6%)
    Simulation time               31 days
    Simulation speed             642 object.hours/second
    Syncs completed           128694 passes
    Time steps completed      128694 timesteps
    Convergence efficiency      1.00 passes/timestep
    Memory lock contention       0.0%
    Average timestep              21 seconds/timestep
    Simulation rate            22892 x realtime
    
    
    Model profiler results
    ======================
    
    Class            Time (s) Time (%) msec/obj
    ---------------- -------- -------- --------
    house             87.120     99.9%    871.2
    collector          0.110      0.1%    110.0
    ================ ======== ======== ========
    Total             87.230    100.0%    863.7
    

### Multi-threaded profiles

When running a multi-threaded model, the profiler output looks as follows: 
    
    
    Core profiler results
    ======================
    
    Total objects                101 objects
    Parallelism                    4 threads
    Total time                 109.0 seconds
      Core time                 45.9 seconds (42.1%)
        Compiler                 1.7 seconds (1.5%)
        Schedules                0.0 seconds (0.0%)
        Loadshapes               0.0 seconds (0.0%)
        Enduses                 14.0 seconds (12.8%)
        Transforms               0.3 seconds (0.3%)
      Model time                63.1 seconds/thread (57.9%)
    Simulation time               31 days
    Simulation speed             689 object.hours/second
    Syncs completed           128694 passes
    Time steps completed      128694 timesteps
    Convergence efficiency      1.00 passes/timestep
    Memory lock contention       0.0%
    Average timestep              21 seconds/timestep
    Simulation rate            24572 x realtime
     
    
    Model profiler results
    ======================
    
    Class            Time (s) Time (%) msec/obj
    ---------------- -------- -------- --------
    house            252.000     99.8%   2520.0
    collector          0.550      0.2%    550.0
    ================ ======== ======== ========
    Total            252.550    100.0%   2500.5


## Pstatus

The processor affinity API uses a global map of the processor affinities for all instances of GridLAB-D™ running on a machine. To display the global process map, use the `--pstatus` command line option. 
    
    
    host% **gridlabd --randtest & gridlabd --pstatus**
    PROC   PID STATE                      CLOCK COMMAND
       0 16807 Running                     INIT /usr/lib/gridlabd/gridlabd.bin --pstatus
       1 16808 Running                     INIT /usr/lib/gridlabd/gridlabd.bin --randtest
   
This global map can become corrupted in the event of the failure of an instance of GridLAB-D™ that leaves a zombie entry in the process map. In such circumstances the `--clearmap` command option can be used to purge the map. A process can be killed and removed from the list using the `--pkill` command option.

## Python

Execute a Python script on a GridLAB-D™ server. The specified filename must exist on the server. The stdout and stderr are sent to the server's output streams. The output file is sent to the client as MIME-type content.

    http://server :port /python/filename.py

## Quiet

The **quiet** option silences all but the most critical output from GridLAB-D™. It is managed by the global variable quiet, which is by default **FALSE**.

From the command line:

* To toggle quiet mode use the option 
 
        host% gridlabd --quiet
    

* To enable quiet mode use the option 
      
        host% gridlabd -D quiet=1
    

* To disable quiet mode use the option   
    
        host% gridlabd -D quiet=0

In a glm:

* To enable quiet mode use the directive 
    
        #set quiet=1
    

* To disable quiet mode use the directive 
     
        #set quiet=0


## R 

Execute an R script on a GridLAB-D™ server. The specified filename must exist on the server. The stdout and stderr are sent to the server's output streams. The output file is sent to the client as MIME-type content.

    http://server :port /r/filename.r

## Randomvar

Built-in random valued property type. The **randomvar** property type is an object property that changes randomly over time. The way in which it changes is controlled by the definition of the randomvar property. The definition of a random property uses a string defined either in the source GLM file or update by a player or external feed that contains all the attributes needed to define or modity the random property. The specification string is formatted as follows:

    "type:distribution(a[,b]); refresh:delay [time_unit]; min:low; max:high; state:seed"

For example, the following code adds a randomized property called clouds to the climate class, creates an object that has a randomvar property updated hourly from a truncated Weibull distribution, and records the value every 5 minutes in a CSV file:

    clock {
        timezone PST+8PDT;
        starttime '2001-01-01 0:00:00 PDT';
        stoptime '2002-01-01 0:00:00 PDT';
    }
    module climate;
    module tape;
    
    class climate {
        randomvar clouds;
    } 
    
    object climate {
        clouds "type:weibull(0.5,0.5); min:0.0; max:1.0; refresh:1h;";
        object recorder {
            property clouds;
            interval 600;
            file "random_builtin.csv";
        };
    }

Random numbers may be generated by one of the following supported distributions:

* bernoulli
* beta
* degenerate
* exponential
* gamma
* lognormal
* normal
* pareto
* rayleigh
* samples
* triangle
* uniform
* weibull

### ASCII Formatting

When converting from a randomvar to a string, only the current value of the random number is formatted using the **double_format** global variable.

When converting from a string to a **randomvar**, the specification string is used to modify the behavior of the random number generator that underlies the property.


## Random Number Generator

The random_number_generator global variable determines which random number generation method is used during simulation.

* **RNG2** - This specifies the platform dependent style of random number generation. It is also not thread-safe.

* **RNG3** - This specifies the platform independent style of random number generation. It is also thread-safe. *This is the default as of Grizzly, Version 2.3*.

From the command line, 

    host% gridlabd --define random_number_generator=RNG3

In the glm,

    globals {
    random_number_generator RNG3;
    }
    #set random_number_generator=RNG3

## Random Seed

Sets the random number generator seed. Both random number generators uses a pseudo-random number generator that must be seeded with an initial value that determines the sequence of number generated. Setting the randomseed to a non-zero value will control this process such that it is deterministic on any given platform.

Note that when random_number_generator global variable is set to RNG2 random number seed will exhibit different behavior on different platforms and are not guaranteed to be deterministic when using multiple threads. RNG3 does not exhibit these problems.

    host% gridlabd -D randomseed=0
    host% gridlabd --define randomseed=0
    #set [randomseed]=0

## Realtime Metric

The `realtime_metric` global variable is used to monitor the performance of the realtime simulation. The value of the metric is calculated using the infinite impulse response (IIR) filter $realtime\_metric = 0.9 realtime\_metric + 0.1 (1 - t_{update})$ where $t_{update}$ is the time required to make a single 1 second update of the simulation. This value is updated every second. The IIR filter has unit step response of about 30 seconds to reach 95% of the steady state value.

A value near 1 indicates that the simulation has plenty of spare time to complete each update. A value near 0 indicates that the simulation is very little time available to complete each update.

    host% wget http://server:portnum/protocol/realtime_metric

## Redirect

The `--redirect` command option is used to instruct GridLAB-D™ to redirect one of the output message stream to a file. The following stream may be redirected

* **output** -
    The default stream is `gridlabd.out`

* **error** -
    The default stream is `gridlabd.err`

* **warning** -
    The default stream is `gridlabd.wrn`

* **debug** -
    The default stream is `gridlabd.dbg`

* **verbose** -
    The default stream is `gridlabd.inf`

* **profile** -
    The default stream is `gridlabd.pro`

* **progress** -
    The default stream is `gridlabd.prg`

The special term **all** may be specified, in which case all streams are directed to their default output files.

The special term **none** may be specified to direct validate to not redirect any output to files.

## Relax

The `--relax` command option is used to allow implicit naming of variables when assignments are made.

!!! note

    This is most commonly used in the context of a naming convention where the name of the objects begin with a numeric (i.e., name 1234_ab;). Without relaxing the naming conventions, an error will be thrown.

From the command line, 

    host% gridlabd --relax

or

    host% gridlabd -D relax_naming_rules=1

In a glm,

    #set relax_naming_rules=1

### Examples

    host% **gridlabd --redirect all**
    
    host% **gridlabd --redirect output:outfile.txt**


## Restrict Observer Access

Global variable to determine how access to properties of observer object is handled. Certain classes are considered observers and have the PC_OBSERVER pass control flag set. Object of these classes should not be accessed by other objects. Doing so would violate the theory of operation and could lead to potential problems with model consistence and simulation stability.

* **NONE** - 
There is not restriction or warning about violations of the access rule.

* **WARNING** - 
Violations of the access rule are reported as a warning. This is useful for detecting potential problems with the model. This is the default value.

* **ERROR** -
Violations of the access rule are reported as an error. This is useful for protecting against potential problems with the model.


## Resume

The control message resume is used when operating in server mode. When receiving this message, GridLAB-D™ resumes the main loop state processing by setting the pauseat global variable to **NEVER**.

    http://servername:portnum/control/resume


## Return Code

Return code value from system shell commands. System commands executed using the `#system macro` return values based on the exit or return code of the last command executed. This value is placed in the return code global variable. If the return code is -1, this is considered an error. On POSIX compliant systems, a return code of 127 is also considered an error indicating that there is fork error.

    #print ${return_code}

## Rt

Request a download of runtime file from a GridLAB-D™ server. The file requested is obtained from the first folder in GLPATH where is can be found.

    http://server :port /rt/filename.ext


## Run

Global variable to get a unique run identifier. The RUN global variable dynamically generates a unique 128-bit identifier that is generated the first time it is referenced. This can be used to generate object names, file names, and database entities that are unique for a single run. The following code defines a class test with a random variable x. The name of the object is unique and shared in the current run.

    class test {
    random x;
    }
    object test {
    name test-${RUN};
    x "type:normal(0,1); refresh:1min";
    }

!!! bugs

    The random number generated is seeded using the current system time with a resolution of 1 second. Consequently, if two runs are started within the same second they are very likely to generate the same sequence of unique ids.

## Runcheck (Check)

The check option enables module check routines when GridLAB-D™ starts. It is managed by the global variable **runcheck**, which is by default **FALSE**.

From the command line,

* To toggle check mode use the option

        host% gridlabd --check

* To enable warn mode use the option

        host% gridlabd -D runcheck=1

* To disable warn mode use the option

        host% gridlabd -D runcheck=0

From the glm,

* To enable check mode use the directive

        #set runcheck=1

* To disable check mode use the directive

        #set runcheck=0

The **runchecks** global variable instructs GridLAB-D™ to call all the modules' check routines after initialization.

## Sanitize

Command option to sanitize models. The sanitizing process destroys the name and position data of objects in a GLM file. This can be used to protect sensitive data is models.

Object names are obfuscated by generating a unique hexadecimal number and appending it to the sanitize_prefix string.

Object positions are obfuscated by moving the latitude by +/-5 degrees and the longtitude by +/-180 degrees.

When the process runs, an index file can be output to allow future recovery of the names and positions, if desired.

The follow global variables can be used to control the sanitizing process.

* **sanitize** - Specifies the sanitizing options. The current valid options are NAMES and POSITIONS. The default is to sanitize both names and positions.

* **sanitize_index** - Specifies the name of the sanitizing index file. If the index is ".xml" or ".txt", then "-index.xml" or "-index.txt" will be appended to the base model name (the ".glm" is truncated). Only XML and TXT file types are recognized. The default index file is "modelname-index.txt".

* **sanitize_offset** - Specifies the lat/lon offset to use when changing the position of objects. Offsets are specified as either `lat,lon` or `lat/lon` where lat and lon are decimal values in the range ±90 and ±180, respectively. If the offset is blank, a random value in the range ±5 and ±180, respectively, is used. If the offset is `destroy` all the latitude and longitude information found in objects is completely erased. The erased values cannot be recovered from the index file.

* **sanitize_prefix** - Specifies the prefix to use on names. The default is "GLD_". Prefixes are strongly recommended because the obfuscated names can begin with a digit, which is not allowed in GridLAB-D™ unless `relax_naming_rules` is used.

The following example sanitizes the GLM file `sensitive.glm` and outputs an index file `sensitive-index.txt`
    
    
    host% gridlabd sensitive.glm --sanitize
    

The following example only sanitized the object names and outputs the index file as an XML file named `index.xml`: 
    
    
    host% gridlabd sensitive.glm -D sanitize=NAMES -D sanitize_index=index.xml --sanitize

!!! caveat

    Coordinates above +85N latitude or below -85S latitude may be corrupted irretrievably because positions can be moved by as much as ±5 degrees, go above or below ±90 and get truncated.

### Sanitize Index

The `sanitize_index` specifies the name of the sanitize index output file. If the spec begins with a period, then it is considered only a formatting hint. The current modelname is used as the index name, with the string "-index.type " appended. Only ".xml" and ".txt" are supported.

If the spec is an empty string, no index file is produced.

### Sanitize Offset

Specifies the offset to use when sanitizing models of position information. When position information in objects is sanitized it can be altered by a specified offset, moved by a randomly chosen constant offset, or it can be completely destroyed.

To move objects by a specified offset, the Δlat and Δlon must be provided.

To move objects by a randomly chosen offset, an empty string must be provided. All objects will be moved by the same random amount.

To complete destroy position information, the term "destroy" must be provided.

    gridlabd -D|--define sanitize_offset=Δlat,Δlon
    gridlabd -D|--define sanitize_offset=Δlat/Δlon
    gridlabd -D|--define sanitize_offset=
    gridlabd -D|--define sanitize_offset=destroy

    #set sanitize_offset=Δlat,Δlon
    #set sanitize_offset=Δlat/Δlon
    #set sanitize_offset=
    #set sanitize_offset=destroy

### Sanitizing Options

* **NAMES** - Specifies that the names of objects should be obfuscated. 

* **POSITIONS** - Specifies that the latitude and longitude information in objects should be obfuscated. 

### Sanitize Prefix

Specifies the sanitized name prefix to use when generating obfuscated names. The sanitizing prefix is used to construct obfuscated names that are guaranteed not to begin with a digit. In addition, the prefix can be used to generate names that are globally unique, if desired.

The default prefix is "GLD_".

    host% gridlabd -D|--define sanitize_prefix=prefix

## Savefile

Specify the file to which final simulation state is written. When a GridLAB-D™ simulation ends or aborts, the state of the model is saved in the file specified by savefile, if defined.

    host% gridlabd -D savefile="gridlabd.xml"
    host% gridlabd --define savefile="gridlabd.xml"
    #set savefile="gridlabd.xml"

## Scilab 

Execute a Scilab script on a GridLAB-D™ server. The specified *filename* must exist on the server. The stdout and stderr are sent to the server's output streams. The output file is sent to the client as MIME-type content.

    http://server :port /scilab/filename.sce

## SEQ

The SEQ_ global variables provide a mechanism for automatically generating a sequence of integers during the loading process. When the sequence options are referenced, the appropriate action is taken. Sequence actions are as follows.

* **INIT** -
Initialize the sequence to the value 0 before evaluating the variable.

* **INC** - 
Increment the sequence value before evaluating the variable.

Example: The following GLM file 
    
    
    #print Initializing: SEQ_A=${SEQ_A:INIT}
    #print SEQ_A=${SEQ_A}
    #print Incrementing: SEQ_A=${SEQ_A:INC}
    #print SEQ_A=${SEQ_A}
    

outputs the following text 
    
    
    example.glm(1): Initializing: SEQ_A=0
    example.glm(2): SEQ_A=0
    example.glm(3): Incrementing: SEQ_A=1
    example.glm(4): SEQ_A=1

!!! caveat

    The INC action can be applied to any global of type int32. This means that you can define a global variable of type int32 using the global directive and start with a non-zero value. For example:

        global int32 SEQ_B 12;
        #print ${SEQ_B:INC}
        is acceptable.

    Also note that using the SEQ variables in macros may result in unexpected behavior because the macros are processed before certain load functions can be processed. For example, to create multiple objects using sequences, you must use expansions rather than macros, such as

        module residential;
        global int32 SEQ_A 0;
        object house:..10 {
        ~~name "House_${SEQ_A:INC}";~~
        name `House_{SEQ_A:INC}`;
        }

## Server

The `--server` command line option instructs GridLAB-D™ to run in server mode with an extra thread to service HTTP requests on the server port number specified by the `server_portnum` global variable. Once server mode is started, incoming messages on the server port will be handled as HTTP request. Response can be either in HTML, XML, or data files such as images, CSV files, etc., depending on the type of request made.

### Shutdown

The control message shutdown is used when operating in server mode. When receiving this message, GridLAB-D™ stops the server and exits the simulation.

     http://servername:portnum/control/shutdown


### Server Portnum

The global variable `server_portnum` controls on which TCP port the server will accept incoming connections. The default server port number 6267 is assigned by IANA (see IANA TCP port listing for details).

The server port number is used only when the server begins listening for incoming connections. Once the listen process is started, the server port number can be changed, but that will not have any affect on the listen process.

If the port number desired is already in use, GridLAB-D™ will increment the port by 1 and try again until it can find an available port.

### Server Quit on Close

Controls whether GridLAB-D™ shuts down when the last server connection closes. When a server finishes servicing an incoming request, it evaluates whether there are any pending requests. If there are none and the connection is closed, GridLAB-D™ will shutdown the simulation if the server_quit_on_close evaluates to a non-zero quantity.

    host% gridlabd -D server_quit_on_close=0|1|FALSE|TRUE
    host% gridlabd --define server_quit_on_close=0|1|FALSE|TRUE
    #set server_quit_on_close=0|1|FALSE|TRUE

## Set (Macro)

The `#set` macro is used to set a global variable in GridLAB-D™. Set only works if the variable is already defined and will not create a new variable if it does not already exist unless the strictnames global variable is set to 1 or TRUE.

    #set variable = value

!!! note

    The macro is not followed by a semicolon unless the semicolon is part of the value.


## Set (Property)

The set built-in data type is used to describe a set of properties that can occur in various combinations. Sets are distinguished from enumerations in that the values in sets can occur simultaneously whereas the values in enumerations can only occur one at a time.

To declare a set in a class, use the following syntax:

    class my_class {
        set {A=1, B=2, C=4, D=8} my_set;
        }
    object my_class {
        my_set A|B|D;
        }
    object my_class {
        my_set BCD;
        }

!!! note

    If all the members of the set are defined as being a single character, then the values of that set can be defined as a string combining those letters without separating, i.e., ABC to signify A+B+C. However, if even one of the members is a multi-letter value, then values of the set must defined using the or-syntax, i.e., A|B|C to signify A+B+C.

## Setenv

The `#setenv` directive is used to override an environment variable.

    #setenv MYVAR="my_variable"

## Show progress

The `show_progress` global variable is used to control the ongoing progress reports while the simulation is running. When the value of the variable `show_progress` is **0** or **FALSE**, progress updates are not output. When the value is 1 or TRUE, progress update are output.

From the command line,

    host% gridlabd -D show_progress=0

In a glm,

    #set show_progress=0


## Simulation Mode

Controls the simulation mode of GridLAB-D™. This variable is typically not set via the #DEFINE or command line approach. It is best to let the internal models control the simulation mode.

* **EVENT** -
The normal simulation mode is the event-based time solver (EVENT). In event-based operation the clock advances in variable time-step of 1 second or more.

        #set simulation_mode=EVENT

* **DELTA** -
The subsecond simulation mode is the finite-difference time solver (DELTA). In delta-based operation the clock advances in fixed time-steps of less than 1 second.

        #set simulation_mode=DELTA


## Start

Execute a command asynchronously in an operating system shell. The shell executes command in a new thread returns immediately.

    #start command

!!! caveat

    Shell commands are almost by definition not portable. Users seeking portable models should avoid using start in a GLM files.


# Strictnames

The strictnames global variable enforces rules that prevent implicit creation of variable using the `#set` directive. By default strict naming rules are enabled.

From the command line,

* To enforce strict assignment rules, use the command

        host% gridlabd -D strictnames=TRUE

    or

        host% gridlabd --define strictnames=TRUE

* To relax strict assignment rules, use the command

        host% gridlabd -D strictnames=FALSE
    
    or

        host% gridlabd --define strictnames=FALSE

!!! caution

    Do not confuse `strictnames` with `relax_naming_rules`, which affect whether certain legacy naming conventions are permitted.


## Suppress Repeat Messages

Control how duplicate messages in output streams are handled. Some situations during simulations can result is some messages being repeated many times and clogging up output buffers. When `suppress_repeat_messages` is set, messages that are similar (not necessarily identical) are suppressed and only a single message is output followed by a message indicated how many time it was repeated.

    host% gridlabd -D suppress_repeat_messages=0|1|TRUE|FALSE
    host% gridlabd --define suppress_repeat_messages=0|1|TRUE|FALSE
    #set suppress_repeat_messages=0|1|TRUE|FALSE

By default `suppress_repeat_messages` is enabled.

## Test

Self-test command line option. The `--test` option runs the specified test routines. 

        gridlabd --test test1 [ test2 [... [ testN ] ] ]

The test routines may be any of the internal test routines, i.e.,

* **dst** - daylight savings time rule test (`--dsttest`)
* **rand** - random number generator statistics test (`--randtest`)
* **units** - unit conversion system test (`--unitstest`)
* **schedule** - schedule system test (`--scheduletest`)
* **loadshape** - load shape generation test (`--loadshapetest`)
* **enduse** - enduse property test (`--endusetest`)
* **lock** - memory locking test (`--locktest`)

In addition, each module may export a test routine, as indicate by the `--libinfo` output.

## Threadcount

The `--threadcount` command line option is used to indicate how many threads are allocated to the GridLAB-D™ run. The threadcount global variable is set using this option. The default thread count is **1**.

* From the command line,

        host% gridlabd --threadcount 4
    or

        host% gridlabd -T 4

* In a glm,

        #set threadcount=4

!!! warning

    The use of multithreading and the process table are **not compatible** with each other. The process table assume that one processor is assigned to one simulation, which is impossible when the thread count is not 1.

## Unittest

The `--unitstest` start a self-test of the unit conversion system in GridLAB-D™. The output of the test is written to the file defined by the global variable testoutputfile, which is by default set to test.txt.

From the command line,

    host% gridlabd --unitstest


## USE GLSOLVERS

Compiler flag to enable use of GridLAB-D™'s internal solvers. The `USE_GLSOLVERS` compile flag causes the gridlabd.h header file to include the API code that supports use of the internal solvers.


    #define USE_GLSOLVERS
    #include "gridlabd.h"


## Use MSVC

Global flag to force the use of **VS2005** instead of **mingw**. Normally, GridLAB-D™ uses mingw to compile runtime code in the GLM files. However, when debugging modules using VS2005, it can be helpful to debugging if GridLAB-D™ uses the runtime compiler instead.


    host% gridlabd -D use_msvc=TRUE
    #set use_msvc=TRUE


## Verbose

The **verbose** option produces all but the debugging output from GridLAB-D™. It is managed by the global variable **verbose**, which is by default **FALSE**.

From the command line,

* To toggle verbose mode use the option

        host% gridlabd --verbose

* To enable verbose mode use the option

        host% gridlabd -D verbose=1

* To disable verbose mode use the option

        host% gridlabd -D verbose=0

In a glm, 

* To enable verbose mode use the directive

        #set verbose=1

* To disable verbose mode use the directive

        #set verbose=0

## Version

Obtain version information. The --version command line option is used to display the version of GridLAB-D™. The message displayed is actually a concatenation of the major and minor numbers, the patch number, the build number, and the branch name.

* **Major** -
The major version of GridLAB-D™ is changed whenever changes are made to the simulation that are not backward compatible. Changes such as the removal of a class or module, or a substantial change in the implementation that will significantly alter the result of simulation will be made only when major versions are released.

* **Minor** -
The minor version of GridLAB-D™ is changed whenever changes are made that are backward compatible within a major version, but are nonetheless substantive.

* **Patch** -
The patch version is changed only when a trivial change or bug fix is released. No other substantive change in simulation results should be expected.

* **Build** -
The build number is changed every time a new changeset is incorporated into a build of the simulation. If the build includes a local modification that is not included in the repository, the build number will have the letter "M" appended.

* **Branch** -
A new name is assigned to each branch. Names are assigned alphabetically based of WECC [1] 500kV busses. See the History page for details on the branch names.

### Version Format

For release builds of GridLAB-D™ the version information is displayed as follows 
    
    
    host% **gridlabd --version**
    GridLAB-D™ 3.0.0-2746 (Grizzly)
              ^ ^ ^ ^    ^
              | | | |    +------- **Branch name**   - Alphabetic sequence named after WECC transmission system buses.
              | | | +------------ **Build number**  - A unique number that identifies which revision the build is based on.
              | | +-------------- **Path number**   - A sequence number identifying which patch the version is.
              | +---------------- **Minor version** - Identifies the minor version number. Minor versions are backward compatible within a major version.
              +------------------ **Major version** - Identifies the major version number. Major versions are not backward compatible with previous major versions.
    

For development builds the branch information will indicate which code tree base was built and a summary of how it was modified. See `svn help status` for details on the modification flags. 


### Example

The version number can be used for conditional tests in GLM files 
    
    
    #if ${version.major}<3
    // version 2 code ...
    #else
    // version 3 code ...
    #endif

## Warn

The warn option produces all the warning output from GridLAB-D™. It is managed by the global variable **warn**, which is by default **FALSE**.

From the command line,

* To toggle warn mode use the option

        host% gridlabd --warn

* To enable warn mode use the option

        host% gridlabd -D warn=1

* To disable warn mode use the option

        host% gridlabd -D warn=0

In a glm,

* To enable warn mode use the directive

        #set warn=1

* To disable warn mode use the directive

        #set warn=0

## Warning

The `#warning` macro displays a message when it is encountered by the loader. When the **quiet** global variable is set or the **warn** global variable is **0** or **FALSE**, the output is suppressed.

In a glm,

    #print message

!!! note

    The message is not followed by a semicolon unless the semicolon is part of the message


## Windows

Global variable indicating whether the current platform is an Windows system. The WINDOWS flag is defined only on Windows systems. On all other platforms, the flag is undefined.

    #ifdef WINDOWS
    #ifndef WINDOWS


## Workdir

Set the working directory for a gridlabd run.

    host% gridlabd --workdir dir
    host% gridlabd -W dir

## XML

In server mode HTTP clients can read and write data entities. The replies to **Xml** queries are always presented in XML. 


    http://server:port/xml/object/property
    http://server:port/xml/object/*
    http://server:port/xml/object/property=value
    http://server:port/xml/global
    http://server:port/xml/module::global

!!! note

    In the following description GNU wget is used to illustrate the query method because it is available on all supported platforms. However, depending on the programming language used to make the queries, different query functions may be required. Some example include send ("C"), urlread (Matlab), and GetMethod (Java). Often these function calls require that an socket connection environment be established using calls such as connect ("C") or HttpClient (Java).

To read a value use the following query:

    host% wget http://hostname:6267/xml/specification

where the specification may take the forms

* **varname** to read a global variable
* **module::varname** to read a module variable
* **name:property** to read an object property

To write data entities, use the following query:

    host% wget http://hostname:6267/xml/specification=value

where value is a string describing the value as you would in a GLM file.

### Return Value

The response to global variable requests will be in the form

    <globalvar>
    <name>variable_name</name>
    <value>value[ unit]</value>
    </globalvar>
    
The response to object property requests will be in the form

    <property>
    <object>object_name</object>
    <name>property_name</name>
    <value>value[ unit]</value>
    </property>

### Errors

The HTTP 1.1 return status may be

* **200 - OK** - The query is valid, the result could formatted and the result was returned.
* **202 - ACCEPTED** - The query is valid and was accepted.
* **404 - NOT FOUND** - The query was not valid or the result could not be formatted. No result was returned.


## Xsd

The      command line option is used to request the XML Schema Document or XSD for a GridLAB-D™ module or class, which described the rules to which XML files that are processed by GridLAB-D™ must conform.

The primary reason for defining an XML schema is to formally describe an XML document; however the resulting schema has a number of other uses that go beyond simple validation.

Code generation The schema can be used to generate code, referred to as XML Data Binding. This code allows contents of XML documents to be treated as objects within the programming environment.

Document generation The schema can be used to generate human-readable documentation; this is especially useful where the authors have made use of the annotation elements. No formal standard exists for documentation generation, but a number of tools are available, such as the Xs3p stylesheet, that will produce high quality readable HTML and printed material.

From the command line,

* To generate the XSD for a module, use the syntax

        host% gridlabd --xsd module
        <?xml version="1.0" encoding="utf-8"?>
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema" targetNamespace="http://www.w3.org/" xmlns="http://www.w3.org/" elementFormDefault="qualified">
        <xs:element name="class_1">
        </xs:element>
        <xs:element name="class_2">
        </xs:element>
        <xs:element name="class_N">
        </xs:element>
        </xs:schema>

* To generate the XSD for a single class, use the syntax

        host% gridlabd --xsd module:class
        <?xml version="1.0" encoding="utf-8"?>
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema" targetNamespace="http://www.w3.org/" xmlns="http://www.w3.org/" elementFormDefault="qualified">
        <xs:element name="class">
        </xs:element>
        </xs:schema>

## Xsl

The `--xsl` command line option is used to generate an Extensible Stylesheet Language (XSL) document for GridLAB-D™ modules and for the core objects. XSL data is used to render and transform XML data.

From the command line,

To generate an XSL document for a set of modules, use the syntax 

    host% gridlabd --xsl module_1,module_2,...,module_N

The output will be written to a file entitled "gridlabd-major_minor.xls", where **major** is the major version of GridLAB-D™ and **minor** is the minor version of GridLAB-D™.


# Global Variables

To get a list of global variables:

    gridlabd --globals

!!! note

    GridLAB-D™ defines the following global variables:

      * APPLE = varies
      * allow_reinclude = FALSE
      * browser = { "iexplore", "safari", "firefox" }
      * check_version = 0
      * checkpoint_file = ""
      * checkpoint_interval = 0
      * checkpoint_keepall = 0
      * checkpoint_seqnum = 0
      * checkpoint_type = NONE
      * clean = Undefined
      * clock = INIT
      * command_line = "CMD "
      * compileonly = 0
      * complex_format = "%+lg%+lg%c"
      * complex_output_format = DEFAULT
      * dateformat = ISO
      * debug = 0
      * debugger = 0
      * delta_current_clock = 0.0
      * deltaclock = 0
      * deltamode_forced_extra_timesteps = 0
      * deltamode_forced_always = FALSE
      * deltamode_iteration_limit = 10
      * deltamode_maximumtime = 3600000000000 (1 hr)
      * deltamode_preferred_module_order = TRUE
      * deltamode_timestep = 10000000 (10 ms)
      * deltamode_updateorder = ""
      * double_format = "%+lg"
      * dumpall = 0
      * dumpfile = "gridlabd.xml"
      * environment = "batch"
      * execdir = "EXE "
      * force_compile = 0
      * gdb = 0
      * gdb_window = 0
      * GUID = varies
      * include = ""
      * infourl = "http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Special:Search/"
      * init_sequence = DEFERRED
      * init_max_defer = 0
      * inline_block_size = 1048576
      * iteration_limit = 100
      * kmlfile = ""
      * LINUX = varies
      * mailto = undefined
      * mainloop_state = INIT
      * MATLAB = varies
      * maximum_synctime = 60
      * minimum_timestep = 1
      * modelname = ""
      * module_compiler_flags = NONE;
      * mt_profile = 0;
      * MYSQL = varies
      * no_deprecate = 0
      * nolocks = 0
      * NOW = varies
      * object_format = "%s:%d"
      * object_scan = "%[^:]:%d"
      * object_tree_balance = FALSE
      * pauseat = NEVER
      * pauseatexit = 0
      * platform = LINUX
      * profiler = 0
      * quiet = 0
      * random_number_generator = RNG3
      * randomseed = 0
      * relax_naming_rules = 0
      * return_code = 0
      * RUN = varies
      * run_realtime = 0
      * runchecks = 0
      * savefile = ""
      * server_portnum = 6267
      * server_quit_on_close = 0
      * show_progress = 1
      * skipsafe = 0
      * starttime = 'YYYY-MM-DD hh:mm:ss ZZZ'
      * stoptime = NEVER
      * streaming_io = 0
      * strictnames = TRUE
      * suppress_repeat_messages = 1
      * technology_readiness_level = UNKNOWN
      * test = 0
      * testoutputfile = "test.txt"
      * threadcount = 1
      * tmp = "TMP "
      * trace = ""
      * urlbase = "http://www.gridlabd.org/"
      * validate = TSTD|RALL
      * validate_report = undefined
      * verbose = 0
      * version.major = 3
      * version.minor = 0
      * warn = 1
      * website = "http://www.gridlabd.org/"
      * WINDOWS = varies
      * workdir = "CWD "
      * xml_encoding = 8
# Matlab link

!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
  
	**This page does not reflect the current state of GridLAB-D™**

The matlab link control file is used establish a connection to an external Matlab instance. It is always referenced by a link directive in a GLM file, i.e., 
    
    link control-file;
    
**GLM file** : 
    
    link target-control-file;
    

**Target control file** : 
    
    target matlab
    command start-cmd
    workdir folder-path
    output buffer-size
    window hide|show|keep
    global variable
    on_init command-list
    on_sync command-list
    on_term command-list
    root root-variable-name
    skipsafe yes|no


The link control file may contain one or more of the following commands. 

Command | Description
-- | --
command | Specifies the command used to start matlab. If omitted, the default **matlab** command is used.
global | Exports a GridLAB-D™ global variable to the **global** structure in the Matlab workspace. Exported variables are members of the **global** structure. For example:     `global clock` exports the **clock** global structure and can be accessed by evaluating **global.clock** in Matlab. Note that globals are **not** copied back to GridLAB-D™.
on_init | Specifies the command-list to execute in Matlab when GridLAB-D™ initializes. This initialization command is executed after all the GridLAB-D™ objects have been initialized and exported to Matlab. You must set **ans** to the return value **GLD_OK** to indicate the initialization command completed successfully.
on_sync | Specifies the command-list to execute in Matlab prior to the standard sync function call. You must set **ans** to the clock time of the next required call of **on_sync** or **TS_NEVER** if none is required. Note that subsequent calls to **on_sync** may occur before the specified next time, and may even be repeated for the current time. See Theory of operation for details.
on_term | Specifies the command-list to execute in Matlab when GridLAB-D™ terminates. You must set **ans** to **GLD_OK** to indicate success.
output | Specifies the size (in bytes) of the output buffer to use when reading back responses from matlab. Responses are displayed when verbose is enabled.
root | Specifies the root variable name to use when publishing the GridLAB-D™ model.
target | Use the target parameter to select the matlab link target.
window | Use the window parameter to specify whether and when the Matlab command window is visible. When the command window is visible, you may interact with Matlab using Matlab commands. <br/> - `hide`: Use the window hide parameter option is hide the Matlab window while running. <br/> - `show`: Use the window show parameter option is show the Matlab window while running. <br/> - `keep`: Use the window keep parameter option is keep the Matlab window open after GridLAB-D™ exits.
workdir | Use the workdir parameter to specify the work directory for matlab. Note that matlab uses its own working folder and by default this is usually not the current gridlabd folder. If the work directory does not exist, it is automatically created.
skipsafe | Use the skipsafe parameter to toggle control of mandatory evaluation of scheduled on_sync execution. <br/> - `yes`: Use the skipsafe yes parameter option to enable scheduled on_sync execution. <br/> - `no`: Use the skipsafe no parameter option to disable scheduled on_sync execution.

!!! warning

    If skipsafe is enabled and `on_sync` returns `TS_NEVER` then `on_sync` will not be executed for the rest of the simulation.

## Example

GLM file: 
    
    
    // if necessary use the appropriate path based on your platform
    #setenv PATH=c:\program files\matlab\r2011a\bin\win64
    #setenv PATH=/usr/bin:/bin:/Applications/MATLAB_R2011a.app/bin
    #setenv PATH=/usr/bin:/bin:/usr/local/bin
    
    link matlab.link:
    clock {
      starttime '2001-01-01 0:00:00';
      stoptime '2001-01-01 1:00:00';
    }
    class test {
      randomvar x;
      double y;
    }
    object test {
      x "type:uniform(0,1); refresh:1min";
      y 0.0;
    }
    

Link file (matlab.link): 
    
    
    target matlab
    window hide
    on_init ans=GLD_OK;
    on_sync ans=TS_NEVER;
    skipsafe no
    on_term ans=GLD_OK;
    global clock
    object my_test0
    export my_test0.x x
    import my_test0.y y
    

## Runtime environment

To use the matlab link target you must have [Matlab®](http://www.mathworks.com) installed on your system. Also the Environmental variables %PATH% must contain the path to MATLAB dlls- <MATLAB install>\bin\win64 or <MATLAB install>\bin\win32. 

Per the [forum post here](https://sourceforge.net/p/gridlab-d/discussion/842562/thread/9cd4060a/#324e), it seems Linux/Unix environments must also have the C shell (csh) installed under /bin/csh. 

## Compiling

!!! note

    The instructions here are basically a duplicate of the versions on the MSYS2 compiling page. While the paths below are all given for a Windows-based system, the same idea applies to Linux and MacOS compiling -- find the MATLAB folder/library/binary and include it where appropriate. 

In order to build the MATLAB link inside of GridLAB-D™ a version of MATLAB must be installed on the system. The following option must be added to your `./configure` command during compiling: 
    
    --with-matlab=<path to MATLAB install>

It is important to note that the MATLAB installation path contain no spaces. If there are spaces in the paths you can find the short name windows uses for the directory by opening up a command window and typing `dir /X` one directory above the directory with the spaces. An example of what `--with-matlab` might look like is below. 

  * Install path on the windows explorer: `C:\Program Files\MATLAB\R2017A`
  * Option set with the short names: `--with-matlab=c:/PROGRA~1/MATLAB/R2017A`

!!! attention

    Before running a simulation with MATLAB, the environment variable `PATH` must contain the path to the MATLAB DLLs, e.g., `MATLAB_DIR\bin\win64` or `MATLAB_DIR\bin\win32`. For more information on the MATLAB functionality within GridLAB-D™ see MATLAB link. 

!!! note

    With MATLAB r2018a and newer, there has been a slight change to external interfaces for complex numbers. You need to tell GridLAB-D™ to use the older/legacy interface via the compiler definition `MATLAB_DEFAULT_RELEASE=R2017b`. An example configuration command would be: 
    
    
    ./configure --prefix=$PWD/install64 --with-xerces=/mingw64/lib --with-matlab=C:/PROGRA~1/MATLAB/R2018a  --enable-silent-rules 'CFLAGS=-O2 -w -DMATLAB_DEFAULT_RELEASE=R2017b' 'CXXFLAGS=-O2 -w -DMATLAB_DEFAULT_RELEASE=R2017b' 'LDFLAGS=-O2 -w'
    

### Legacy instructions for Visual Studio 2005

By default, the MATLAB link is not built by the normal Hassayampa (Version 3.0) Visual Studio GridLAB-D™ solution. Linux compilers will attempt to locate MATLAB installs and enable all proper variables. If it fails to find the link, the same requirements outlined for the Visual Studio build below will need to be manually implemented. Note that most of these are handled automatically by _configure.bat_ inside Visual Studio, but if manual configuration is required, these steps can still be used. 

To get the MATLAB link working under a Visual Studio-compiled build, a few changes must be made to the glxmatlab project. 

  1. The HAVE_MATLAB preprocessor directive must be defined.
  2. The C++ include folders must point to the _include_ folder of the MATLAB version. This is something similar to `C:\Program Files\MATLAB\R2011a\extern\include`
  3. The Linker Additional Library Directories must point to the proper folder of the MATLAB install. This should be similar to `C:\Program Files\MATLAB\R2011a\extern\lib\win64\microsoft`
  4. The Linker Input Additional Dependencies must include _libmx.lib_ and _libeng.lib_ to properly build.

After these changes are accomplished, Visual Studio will be able to compile a GridLAB-D™ version with the MATLAB link interface. 

!!! note

    MATLAB appears to have some sensitivity to the architecture of the version compiled into GridLAB-D™. For example, if it was built to an x64 version of MATLAB, but the .link file points to a 32-bit version, sometimes it fails. Furthermore, if multiple installations exist, you may need to register the appropriate version with _matlab /regserver_ on the command line before using it with GridLAB-D™ (not always necessary). 

Developer's Note (9/16/2013): 

_The configuration of MATLAB is supposed to be handled via a script now, which places some of the preprocessor directives and links to MATLAB elsewhere. To get it working, there are two main options:_

_The first is to revert the glxmatlab project to its base state. Open a command window under /Utilities in the repository you downloaded. Then run configure SOURCE=.. /f (make sure Visual Studio is not open). After this completes, it should build. The configure command is a script that runs during the build process and sets many of these links up. It creates two .vsprops files under /core/link/matlab that point to your specific installations of MATLAB._ **We have no idea how well those translate to Visual Studio 2008 or other compilers.** If you can't build successfully (you may see an error that matrix.h cannot be found), check that the .vsprops files in your /core/link/matlab folder are being updated when you run configure.bat. If they are not, try deleting them and manually copying the newly-created .vsprops files from the utilities folder in to the /core/link/matlab folder. 

_The alternative is to revert to the old method (above) of including MATLAB. Edit the matlab.vcproj either in a text editor or inside Visual Studio. You'll want to remove any references to MATALB_specific_win32.vsprops and MATLAB_specific_x64.vsprops (the files mentioned above) and then configure everything manually according to the directions on the Wiki. Inside Visual Studio 2005, these are added under Configuration Properties- >General->Inherited Project Property Sheets._

_If all else fails, and you do not require the MATLAB connection, then the project can be unloaded._

## See also

  * Link (directive)
    * Matlab link
    * JSON link Template:NEW30
    * Technical manual
  * **How To**
    * How to plot data using Matlab
    * How to create a movie


# Mac OSX/Setup

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Mac_OSX/Setup

---
 
 



_Out of Date:_ This page has been tagged as out of date and may contain data which does not represent current recommended use or functionality. 

## Contents

  * 1 Overview
  * 2 Build Environment Setup Procedure
    * 2.1 Install Xcode Command Line Tools (Development Tools)
    * 2.2 Install homebrew
    * 2.3 Install autoconf, automake, and libtool
    * 2.4 Install gnu-sed
    * 2.5 Symlink gsed to sed
    * 2.6 Install xerces-c
    * 2.7 Install mysql-connector-c (optional)
    * 2.8 Link matlab (optional)
    * 2.9 Link the new commands (only in event of errors)
  * 3 El Capitan debugging using gdb
  * 4 Bugs
  * 5 See also
**Mac OSX Setup** \-- Building GridLAB-D™ on a Mac 

## Overview

Building GridLAB-D™ on a Mac uses much the same process as on Linux/Unix. We recommend that you use Xcode and homebrew to install GridLAB-D's dependencies on your system. 

## Build Environment Setup Procedure

### Install Xcode Command Line Tools (Development Tools)

From the command line prompt (Terminal): 
    
    
    xcode-select --install
    

A dialog box will pop-up asking to confirm the installation of the command line tools. 

### Install homebrew
    
    
    /bin/bash -c "$(curl -fsSL <https://raw.githubusercontent.com/Homebrew/install/master/install.sh>)"
    

### Install autoconf, automake, and libtool
    
    
    brew install autoconf
    brew install automake
    brew install libtool
    

### Install gnu-sed
    
    
    brew install gnu-sed
    

### Symlink gsed to sed
    
    
    ln -s /usr/local/bin/gsed /usr/local/bin/sed
    

### Install xerces-c

This is a required package for GridLAB-D™. 
    
    
    brew install xerces-c
    

### Install mysql-connector-c (optional)

Required only when using MySQL 
    
    
    brew install mysql-connector-c
    ln -s /usr/local/Cellar/mysql-connector-c/<version> /usr/local/mysql-connector-c
    

where <version> is replaced by your particular version, e.g., "6.1.6". 

### Link matlab (optional)

If you want to use Matlab you must have an active license. Add matlab to your path in ~/.bash_profile so 'configure' can find it, e.g., 
    
    
    PATH=/usr/local/bin:/usr/bin:/bin:/usr/texbin:/usr/sbin:/sbin:/Applications/MATLAB_<version>.app/bin
    

where <version> is replaced by your particular version, e.g., "R2015a". You can test your path by asking matlab to report its environment: 
    
    
    matlab -e
    

which, if configured properly produces a series of environment variables and their values, e.g., 
    
    
    TERM_PROGRAM=Apple_Terminal
    MATLABPATH=/Applications/MATLAB_R2015a.app/toolbox/local
    TERM=xterm-256color
    SHELL=/bin/bash
    ...
    

### Link the new commands (only in event of errors)

The above commands should create symlinks in /usr/local/bin to each of these tools. On some combinations of XCode version and Mac OSX version, the symlinks don't get created. If this is the case, create them with 
    
    
    brew ln --force autoconf
    brew ln --force automake
    brew ln --force libtool
    

You should now be prepared to [build GridLAB-D™ on your Mac]. 

## El Capitan debugging using `gdb`

To enable `gdb` on OS X El Capitan you must do the following (source: <http://unixnme.blogspot.com/2016/04/how-to-enable-gdb-on-mac-os-x-el-capitan.html>). 

First,install `gdb`. 
    
    
    brew install gdb
    

At this point if you debug a program with `gdb` you get this rather unhelpful message: 
    
    
    Unable to find Mach task port for process-id 627: (os/kern) failure (0x5).
    (please check gdb is codesigned - see taskgated(8))
    

Normally you would follow this advice and codesign `gdb` (see for example <http://stackoverflow.com/questions/33162757/how-to-install-gdb-debugger-in-mac-osx-el-capitan>). But an alternative to codesigning gdb is to enable the old Tiger convention for the `task_for_pid` access control daemon: 

  1. Restart OS X. Enter recovery mode by pressing and holding [command + R] until you see Apple logo (for details see <https://support.apple.com/en-us/HT201314>).
  2. In the recovery mode, choose utilities menu and open up terminal
  3. In the terminal, disable system integrity protection (SIP) 

    $ **csrutil disable && reboot**
  4. Add `-p` option to `/System/Library/LaunchDaemons/com.apple.taskgated.plist` file. After your edit, it should like something like this around line 22 

    `<array>`
    ` <string>/usr/libexec/taskgated</string>`
    ` <string>-sp</string>`
    `</array>`
  5. (Optional) Re-enable SIP by repeating steps 1~3 with the command and reboot. 

    `$ **csrutil enable && reboot**`
  6. . Add your username to procmod group 

    `$ **sudo dseditgroup -o edit -a username -t user procmod**`
  7. Locate gdb executable file and run 

    `$ **sudo chgrp procmod /user/local/Cellar/gdb/7.10.1/bin/gdb**`
    `$ **sudo chmod g+s /user/local/Cellar/gdb/7.10.1/bin/gdb**`
You need to reboot your system for the change to take effect. 

If your `gdb` path differs from above, search for the location using 

    ` $ find / -name 'gdb' -type f -print 2>/dev/null`

  


## Bugs

Some tickets have a bad configuration and produce invalid Makefile variables for matlab-related targets, such as 
    
    
    MATLAB = matlab
    MATLAB_CPPFLAGS = -Imatlab/extern/include
    MATLAB_LDFLAGS = -L
    MATLAB_LDPATH = 
    

which causes the core/link/matlab folder build to fail. If you run into this you can patch the Makefile as follows 
    
    
    MATLAB = /Applications/MATLAB_R2015a.app
    MATLAB_CPPFLAGS = -I${MATLAB}/extern/include
    MATLAB_LDFLAGS = -L${MATLAB}/bin/maci64
    MATLAB_LDPATH = ${MATLAB}/bin/maci64
    

or you can download a fixed version of the automake files. 

## See also

  * [Installation Guide]


## Important Note

This repository is SLAC National Accelerator's develop fork of GridLAB-D (see https://github.com/gridlab-d/gridlab-d for the official code repository of GridLAB-D).  Only SLAC projects may contribute to this repository.  Changes made in this fork will be migrated back to PNNL's official repository at PNNL's discretion.

# User quick start

The preferred method of using SLAC releases of GridLAB-D is to download the SLAC master image from docker hub (see https://cloud.docker.com/u/gridlabd/repository/docker/gridlabd/slac-master).  You must install the docker daemon to use docker images.  See https://www.docker.com/get-started for details.

Once you have installed docker, you may issue the following commands to run GridLAB-D at the command line:
~~~
  host% docker run -it -v $PWD:/model gridlabd/slac-master gridlabd -W /model [load-options] [filename.glm] [run-options] 
~~~ 
On many systems, an alias can be used to make this a simple command that resemble the command you would normally issue to run a host-based installation:
~~~
  host% alias gridlabd='docker run -it -v $PWD:/model gridlabd/slac-master gridlabd -W /model'
~~~
Note that this alias will interfere with the host-based installation.

# Developer quick start

Assuming your development system is ready (see https://github.com/dchassin/gridlabd/wiki/Install#mac-osx-and-linux for details), you can "quickly" download and build a host-based installation from a branch using the following commands:
~~~
  host% git clone https://github.com/dchassin/gridlabd -b _branch-name_ _work-folder_
  host% cd _work-folder_
  host% autoreconf -isf
  host% ./configure --enable-silent-rules --prefix=$PWD/install [_options_]
  host% make -j install
  host% export PATH=$PWD/install/bin:$PATH
  host% gridlabd --version
  host% gridlabd --validate
~~~
## Useful configure options
 - `--with-mysql=/usr/local` to enable support for mysql (assuming you install mysql-dev on your system)
 - `CXXFLAGS='-w -O0 -g'` to enable debugging of C++ source code (e.g., module code)
 - `CFLAGS='-w -O0 -g'` to enable debugging of C source code (e.g., core code)

## Notes
- The version number should contain the _branch-name_.  If not, use the `which gridlabd` command to check that the path is correct.
- You can control whether your local version run the docker image instead of the local install using the `--docker` command-line option.
- In theory all validate tests of the master should pass. However, sometimes issues arise that aren't caught until after a merge into master.  If you encounter a validation error, please check the issues to see if it has not already been reported.  When reporting such a problem, please include the `--origin` command line option output, the `validate.txt` output, and the output from `uname -a` to assist in reproducing and diagnosing the problem.

## Contributions

Please see https://github.com/dchassin/gridlabd/blob/master/CONTRIBUTING.md for information on making contributions to this repository.

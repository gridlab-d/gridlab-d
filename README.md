The repository is only for building and installing GridLAB-D from source.  The 
installers for released versions are available at http://sourceforge.net/projects/gridlab-d/files/.

# Installing from source

Download the source code, build, install and validate (you will need autotools)
```
  host% git clone git@code.stanford.edu:/gridlabd/gridlabd source
  host% cd source
  host% autoreconf -isf
  host% ./configure --enable-silent-rules --prefix=$HOME 'CXXFLAGS=-w' 'CFLAGS=-w'
  host% make install
  host% export PATH=~/bin:$PATH
  host% gridlabd --validate
```

# Using docker

See https://github.com/dchassin/gridlabd/tree/master/utilities/docker for DockerFile and readme.

## Windows build

Windows users will need to install cygwin to enable building using autotools.
See http://gridlab-d.sourceforge.net/wiki/index.php/MinGW/Eclipse_Installation
for details.

## Eclipse Editor Setup

You can setup Eclipse as your GridLAB-D modeling editor.  See 
http://gridlab-d.sourceforge.net/wiki/index.php/Eclipse for details.

# Online Documentation

The primary manuals for GridLAB-D are online at Main Page for GridLAB-D (http://gridlab-d.sourceforge.net/wiki/index.php/Main_Page).
Documentation of local projects can be found on this project's wiki (https://code.stanford.edu/gridlabd/gridlabd/wikis/home).

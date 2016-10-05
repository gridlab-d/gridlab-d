# Installing from source

Download the source code, build, install and validate (you will need autotools)

>  host% git clone git@code.stanford.edu:/gridlabd/gridlabd source
  
>  host% cd source
  
>  host% autoreconf -isf
  
>  host% ./configure --enable-silent-rules --prefix=$HOME 'CXXFLAGS=-w' 'CFLAGS=-w'
  
>  host% make install
  
>  host% export PATH=~/bin:$PATH
  
>  host% gridlabd --validate
  
## Windows build

Windows users will need to install cygwin to enable building using autotools.
See http://gridlab-d.sourceforge.net/wiki/index.php/MinGW/Eclipse_Installation
for details.

## Eclipse Editor Setup

You can setup Eclipse as your GridLAB-D modeling editor.  See 
http://gridlab-d.sourceforge.net/wiki/index.php/Eclipse for details.

Reverting To Old Build Scripts Linux and Other Unix Variants
Copyright (C) 2013 Battelle Memorial Institute

If you wish to use the old build scripts for building from a source distribution,
you must copy the files contained within each subfolder their counterpart subfolder in
the top level folder. The top level folder one directory up from where this readme
folder is contained. 

For example the configure.ac file found in the same directory you are now would need
to replace the one found one directory up. As another example, the Makefile.am
found in the assert folder in this directory would need to be copied to ../assert.

After this has been completed please follow the steps found in README-MACOSX or
README-LINUX found in the top level folder to build GridLAB-D.
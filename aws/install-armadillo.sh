#!/bin/bash

#install armadillo - C++ linear algebra library
cd /home/ec2-user
wget http://sourceforge.net/projects/arma/files/armadillo-7.800.1.tar.xz
tar xf armadillo-7.800.1.tar.xz
rm -f armadillo-7.800.1.tar.xz
cd armadillo-7.800.1
cmake .
make install

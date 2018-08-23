#!/bin/bash
#
# docker centos-gridlabd setup script
#
# Starting docker on the host
#
#   host% docker run -it centos bash
#   Password: ...
#   ...
#   [root@... /]# cd /tmp
#   [root@... /tmp]# curl https://raw.githubusercontent.com/dchassin/gridlabd/master/utilities/docker/centos-setup.sh -o setup.sh
#   [root@... /tmp]# chmod +x setup.sh
#   [root@... /tmp]# ./setup.sh
#

### Docker commands to build gridlabd

# Install needed tools
yum update -y
yum groupinstall "Development Tools" -y

yum install cmake -y 
yum install https://dev.mysql.com/get/mysql57-community-release-el7-9.noarch.rpm -y
#yum install mysql56-server -y 
yum install mysql-libs -y

cd /usr/local/src
git clone https://github.com/dchassin/gridlabd gridlabd

# install xercesc
cd /usr/local/src/gridlabd/third_party
alias sudo=
. install_xercesc

# install mysql 
cd /usr/local/src/gridlabd/third_party
MYSQL=mysql-connector-c-6.1.11-linux-glibc2.12-x86_64
gunzip ${MYSQL}.tar.gz
tar xf ${MYSQL}.tar
cp -u ${MYSQL}/bin/* /usr/local/bin
cp -Ru ${MYSQL}/include/* /usr/local/include
cp -Ru ${MYSQL}/lib/* /usr/local/lib

# install armadillo
cd /usr/local/src/gridlabd/third_party
gunzip armadillo-7.800.1.tar.gz
tar xf armadillo-7.800.1.tar
cd armadillo-7.800.1
cmake .
make install

# install gridlabd
cd /usr/local/src/gridlabd
autoreconf -isf
./customize configure
make install

# Validate GridLAB-D
gridlabd --validate




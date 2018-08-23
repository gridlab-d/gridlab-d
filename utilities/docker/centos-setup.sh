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
#   [root@... /]# curl https://raw.githubusercontent.com/dchassin/gridlab/master/utilties/centos-setup.sh -o setup.sh
#   [root@... /]# chmod +x setup.sh
#   [root@... /]# ./setup.sh
#

### Docker commands to build gridlabd

# Install needed tools
yum update -y
yum groupinstall "Development Tools" -y

yum install cmake -y 
yum install https://dev.mysql.com/get/mysql57-community-release-el7-9.noarch.rpm -y
#yum install mysql56-server -y 
MYSQL=mysql-connector-c-6.1.11-linux-glibc2.12-x86_64
curl https://raw.githubusercontent.com/dchassin/gridlabd/master/third_party/${MYSQL}.tar.zip -o ${MYSQL}.tar.zip
unzip -y ${MYSQL}.tar
tar xf ${MYSQL}.tar
rm -f ${MYSQL}.tar
cp -u ${MYSQL}/bin/* /usr/local/bin
cp -Ru ${MYSQL}/include/* /usr/local/include
cp -Ru ${MYSQL}/lib/* /usr/local/lib
rm -rf ${MYSQL}*
yum install mysql-libs -y

# install armadillo
cd /usr/local/src
curl https://raw.githubusercontent.com/dchassin/gridlabd/master/third_party/armadillo-7.800.1.tar.xz -o armadillo-7.800.1.tar.xz
tar xf armadillo-7.800.1.tar.xz
rm -f armadillo-7.800.1.tar.xz
cd armadillo-7.800.1
cmake .
make install

cd /usr/local/src
git clone https://github.com/dchassin/gridlabd gridlabd

# install xercesc
cd gridlabd/third_party
alias sudo=
. install_xercesc

# install gridlabd
cd /usr/local/src/gridlabd
autoreconf -isf
./customize configure
make install

# Validate GridLAB-D
gridlabd --validate




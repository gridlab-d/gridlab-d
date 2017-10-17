#!/bin/bash

#elevate privileges to su
sudo su
# yum install wget -y  # install wget for redhat 7 on EC2

# yum update install git, php, apache, cmake, dev tools
wget https://s3-us-west-1.amazonaws.com/vader-lab/gridlabd-dependencies/install-base.sh
sh install-base.sh

# MySQL Install server, connector, and libs
wget https://s3-us-west-1.amazonaws.com/vader-lab/gridlabd-dependencies/install-mysql.sh
sh install-mysql.sh

#copy gridlab source and install lib-xercesc
wget https://s3-us-west-1.amazonaws.com/vader-lab/gridlabd-dependencies/install-libxercesc.sh
sh install-libxercesc.sh


# install armadillo - linear algebra library
cd /home/ec2-user
wget https://s3-us-west-1.amazonaws.com/vader-lab/gridlabd-dependencies/install-armadillo.sh
sh install-armadillo.sh

# clone IEEE123 model in www folder
cd /var/www/html
git clone https://github.com/dchassin/ieee123-aws
cp -R ieee123-aws/* .
rm -rf ieee123-aws/
mkdir data output
chmod -R 777 data output config
chown -R apache.apache .

#install gridlabd
cd /home/ec2-user/gridlabd/source
autoreconf -isf
./customize configure
make install
export PATH=/usr/local/bin:$PATH
gridlabd --validate

# edit httpd.conf file and add index.php
nano /etc/httpd/conf/httpd.conf

# install my web sql
# cd var/www/html
# wget http://downloads.sourceforge.net/project/mywebsql/stable/mywebsql-3.4.zip

#### this  needs to happen last ####
cd /etc
nano my.cnf
#----- text below without leading #
#[client]
#port=3306
#socket=/tmp/mysql.sock

#[mysqld]
#port=3306
#datadir=/var/lib/mysql
#socket=/tmp/mysql.sock
#-----
chmod 755 my.cnf #777 is not allowed - world writable. mysqld won't restart
# DONT DO THIS
#chown -R apache.apache .
##########################
service mysqld restart
mysql
# Create user gridlabd_ro and gridlabd in mysql database
#CREATE USER 'gridlabd'@'localhost' IDENTIFIED BY 'gridlabd';
#GRANT ALL PRIVILEGES ON *.* TO 'gridlabd'@'localhost' WITH GRANT OPTION;
#FLUSH PRIVILEGES;
#CREATE USER 'gridlabd_ro'@'%' IDENTIFIED BY 'gridlabd';
#GRANT SELECT ON *.* TO 'gridlabd_ro'@'%';
#FLUSH PRIVILEGES;
# --------------------------------

#start apache service
service httpd start
systemctl start httpd # on RHEL 7 only

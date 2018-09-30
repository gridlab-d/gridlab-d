#!/bin/bash
#
# docker centos-gridlabd setup script
#
# Building this docker image
#
#   host% docker build -f DockerFile .
#   host% docker save > centos-gridlabd
#
# Starting docker on the host
#
#   host% docker run -it -v $(pwd):/gridlabd centos-gridlabd gridlabd -W /gridlabd <options>
#

echo '
#####################################
# DOCKER BUILD
#   httpd
#   mysqld
#   mywebsql
#####################################
'

# Install MySQL
yum install https://dev.mysql.com/get/mysql57-community-release-el7-9.noarch.rpm -y
yum install mysql-server -y 
yum install mysql-libs -y

# Install httpd
yum install httpd -y
yum install php -y

# Install mywebsql
#cd /var/www/html
#curl http://downloads.sourceforge.net/project/mywebsql/stable/mywebsql-3.4.zip | gunzip

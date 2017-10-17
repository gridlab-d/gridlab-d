#!/bin/bash

cd /home/ec2-user

#install MySql Server
yum install https://dev.mysql.com/get/mysql57-community-release-el7-9.noarch.rpm -y
yum install mysql56-server -y # for Amazon linux

#install MySql Connector
wget https://dev.mysql.com/get/Downloads/Connector-C/mysql-connector-c-6.1.9-linux-glibc2.5-x86_64.tar.gz
tar xf mysql-connector-c-6.1.9-linux-glibc2.5-x86_64.tar.gz
#rm mysql-connector-c-6.1.9-linux-glibc2.5-x86_64.tar.gz
cd mysql-connector-c-6.1.9-linux-glibc2.5-x86_64/
cp bin/* /usr/local/bin
cp -R include/* /usr/local/include
cp -R lib/* /usr/local/lib
yum install mysql-libs -y

#test MySQL Service
service mysqld start
service mysqld stop

#yum install mysql -y #remove this later

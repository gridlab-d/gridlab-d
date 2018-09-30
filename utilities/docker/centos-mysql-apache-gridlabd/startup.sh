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
#   startup
#####################################
'

# Start web server
if [ ! -f /etc/httpd/conf/httpd.original ]; then
	cp /etc/httpd/conf/httpd.conf /etc/httpd/conf/httpd.original
	sed -e 's/DirectoryIndex .*/& index.php/' /etc/httpd/conf/httpd.original >/etc/httpd/conf/httpd.conf
fi
systemctl enable httpd
systemctl start httpd

# Start mysql server
if [ ! -f /etc/my.original ]; then
	cp /etc/my.cnf /etc/my.original
	sed -e 's/^port=.*/#&/g;s/^socket=.*/#&/g;s/^datadir=.*/#&/g' /etc/my.original >/etc/my.cnf
fi
systemctl enable mysqld
systemctl start httpd
mysql <<-END
	CREATE USER 'gridlabd'@'localhost' IDENTIFIED BY 'gridlabd';
	GRANT ALL PRIVILEGES ON *.* TO 'gridlabd'@'localhost' WITH GRANT OPTION;
	FLUSH PRIVILEGES;
	CREATE USER 'gridlabd_ro'@'%' IDENTIFIED BY 'gridlabd';
	GRANT SELECT ON *.* TO 'gridlabd_ro'@'%';
	FLUSH PRIVILEGES;
END
if [ ! -f /etc/bashrc.original ]; then
	cp /etc/bashrc /etc/bashrc.original
	echo 'export LD_LIBRARY_PATH=/usr/local/lib' >> /etc/bashrc
fi


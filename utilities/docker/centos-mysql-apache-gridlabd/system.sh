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
#   centos
#####################################
'

### Docker commands to build gridlabd

# Install needed tools
yum update -y ; yum clean all
yum install systemd -y ; yum clean all
yum groupinstall "Development Tools" -y
yum install cmake -y 

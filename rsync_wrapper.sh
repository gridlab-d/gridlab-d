#!/bin/bash

rsync -rvP --delete -e "ssh -i /home/buildbot/.ssh/id_dsa" $1 natet,gridlab-d@$2:$3
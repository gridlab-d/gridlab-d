#!/bin/bash

#export PATH="$PATH:/home/fncsdemo/fncs2-install/bin:/home/fncsdemo/gridlabd-master-install/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

echo $PATH

export FNCS_LOG_LEVEL=DEBUG4
export FNCS_LOG_STDOUT=yes

rm -f jeff_*.out

#export PATH

fncs_broker 2 >& jeff_broker.out &

#cd ~/FNCS-GLD-TESTCASE/gldfncs/fncs/tracer

fncs_tracer 12s the_trace_file_output.out >& jeff_tracer.out &


#cd ~/FNCS-GLD-TESTCASE/gldfncs/gld/

gridlabd  IEEE_Dynamic_2gen_GLD3_nogen_jsonpub.glm >& jeff_gld.out



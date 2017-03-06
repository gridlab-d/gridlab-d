#!/bin/bash

#export PATH="$PATH:/home/fncsdemo/fncs2-install/bin:/home/fncsdemo/gridlabd-master-install/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

echo $PATH

#export PATH

#export FNCS_LOG_LEVEL=DEBUG4
#export FNCS_LOG_STDOUT=yes

#rm -f *.out

fncs_broker 3 >& test_broker.out &

#cd ~/FNCS-GLD-TESTCASE/gldfncs/fncs/tracer

#fncs_tracer 12s the_trace_file_output.out &


#cd ~/FNCS-GLD-TESTCASE/gldfncs/gld/

#gridlabd  IEEE_Dynamic_2gen_GLD3_nogen_jsonpub.glm > glmpub.out &

fncs_player_anon 12s fncsplayerout.txt >& test_player.out &

fncs_tracer 12s the_trace_file_output.out >& test_tracer.out &

#gridlabd IEEE_Dynamic_2gen_GLD3_nogen_jsonsub.glm &

gridlabd IEEE_13nodes_regulator_jsonsub.glm &



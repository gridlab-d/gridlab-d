#!/bin/bash

#export PATH="$PATH:/home/fncsdemo/fncs2-install/bin:/home/fncsdemo/gridlabd-master-install/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

echo $PATH

#export PATH

fncs_broker 2  &

#cd ~/FNCS-GLD-TESTCASE/gldfncs/fncs/tracer

fncs_tracer 12s the_trace_file_output.out &


#cd ~/FNCS-GLD-TESTCASE/gldfncs/gld/

gridlabd IEEE_Dynamic_2gen_GLD3_nogen_tracer.glm &

wait

grep "6100" the_trace_file_output.out > tmpout.out

test -f tmpout.out && grep "+9734753" tmpout.out
breal="$?"
echo $breal

test -f tmpout.out && grep "4307220" tmpout.out
bimag="$?"
echo $bimag

if [  "$breal" == 0 ] && [ "$bimag" == 0 ]; then
    echo "test of Gridlabd and FNCS passed!"
    rm *csv
    rm *log
    rm *~
    rm *out
    exit 0
else
    echo "test of Gridlabd and FNCS failed!"
    # rm *csv
    # rm *log
    # rm *~
    # rm *out
    exit 1
fi



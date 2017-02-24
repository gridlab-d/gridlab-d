#!/bin/bash

#export PATH="$PATH:/home/fncsdemo/fncs2-install/bin:/home/fncsdemo/gridlabd-master-install/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

echo $PATH

#export PATH

fncs_broker 2  &

#cd ~/FNCS-GLD-TESTCASE/gldfncs/fncs/tracer

fncs_player_anon 12s fncsplayerout.txt &


#cd ~/FNCS-GLD-TESTCASE/gldfncs/gld/

gridlabd IEEE_Dynamic_2gen_GLD3_nogen_player.glm &


wait

grep "00:00:06 PST," Voltage_substation.csv > tmpout.out

test -f tmpout.out && grep "+80789," tmpout.out
breal="$?"
echo $breal

test -f tmpout.out && grep "8549.48" tmpout.out
bimag="$?"
echo $bimag

if [  "$breal" == 0 ] && [ "$bimag" == 0 ]; then
    echo "test of Gridlabd and FNCS Player passed!"
    rm *csv
    rm *log
    rm *~
    rm *out
    exit 0
else
    echo "test of Gridlabd and FNCS Player failed!"
    rm *csv
    rm *log
    rm *~
    rm *out
    exit 1
fi






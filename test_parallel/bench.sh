#!/bin/bash

a="$(TZ=UTC0 printf '%(%s)T\n' '-1')"    ### `-1`  is the current time
../build/bin/gridlabd bench_500_houses.json > house1.log
elapsedseconds=$(( $(TZ=UTC0 printf '%(%s)T\n' '-1') - a ))
echo "Elapsed time1 $elapsedseconds seconds"

a="$(TZ=UTC0 printf '%(%s)T\n' '-1')"    ### `-1`  is the current time
../build/bin/gridlabd --threadcount 4 bench_500_houses.json > house4.log
elapsedseconds=$(( $(TZ=UTC0 printf '%(%s)T\n' '-1') - a ))
echo "Elapsed time4 $elapsedseconds seconds"

a="$(TZ=UTC0 printf '%(%s)T\n' '-1')"    ### `-1`  is the current time
../build/bin/gridlabd --threadcount 8 bench_500_houses.json > house8.log
elapsedseconds=$(( $(TZ=UTC0 printf '%(%s)T\n' '-1') - a ))
echo "Elapsed time8 $elapsedseconds seconds"

a="$(TZ=UTC0 printf '%(%s)T\n' '-1')"    ### `-1`  is the current time
../build/bin/gridlabd --threadcount 16 bench_500_houses.json > house16.log
elapsedseconds=$(( $(TZ=UTC0 printf '%(%s)T\n' '-1') - a ))
echo "Elapsed time16 $elapsedseconds seconds"

a="$(TZ=UTC0 printf '%(%s)T\n' '-1')"    ### `-1`  is the current time
../build/bin/gridlabd --threadcount 20 bench_500_houses.json > house20.log
elapsedseconds=$(( $(TZ=UTC0 printf '%(%s)T\n' '-1') - a ))
echo "Elapsed time20 $elapsedseconds seconds"

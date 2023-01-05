#!/usr/bin/env bash

GIT_OUTPUT=$1

case $2 in
    -p|--pwd)
    RESULT=`pwd`
    ;;
    -r|--remote)
    RESULT=`cat ${GIT_OUTPUT} | grep "(fetch)" | sed -n 's/^.*\t//; $s/ .*$//p'`
    ;;
    -s|--status)
    RESULT=`cat ${GIT_OUTPUT} | cut -c1-3 | sort -u | sed 's/ //g' | sed ':a;N;$!ba;s/\n/ /g'`
    ;;
    -b|--build)
    RESULT=`cat ${GIT_OUTPUT} | sed -ne 's/ [\+\-][0-9]\{4\}$//p'`
    ;;
esac
echo "$RESULT"

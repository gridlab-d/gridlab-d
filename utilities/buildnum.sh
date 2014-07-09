#!/bin/bash
new=$(svn info $1 | grep '^Last Changed Rev: ' | cut -f2 -d':')
if [ -f build.h ]; then
	old=$(cat build.h | cut -f3 -d' ')
else
	old=0
fi
if [ $new -ne $old ]; then 
	echo "#define BUILDNUM $new" > build.h
fi

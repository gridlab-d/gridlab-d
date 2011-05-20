#!/bin/bash

for i in `find . -type d -name autotest`; 
do 
	for j in `find $i -name \\*.dll`; 
	do 
		rm $j; 
	done; 
done
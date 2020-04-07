#!/bin/bash

export DOCS=`pwd`/../documents/troubleshooting

if [ ! -e $DOCS ]
then
	mkdir -p $DOCS
fi

# Get the branch name
export branch=`git branch | grep \* | cut -b 1,2 --complement`

cp utilities/troubleshooting.css $DOCS
awk -v branch="$branch" -f ./utilities/troubleshooting.awk */*.{c,cpp} > $DOCS/index.html

#!/bin/bash

export DOCS=`pwd`/../documents/troubleshooting

if [ ! -e $DOCS ]
then
	mkdir -p $DOCS
fi

cp utilities/troubleshooting.css $DOCS
awk -f ./utilities/troubleshooting.awk */*.{c,cpp} > $DOCS/index.html

#!/bin/sh
# $Id$

if [ -s trace.r ]
then
    TRACE=trace.r
elif [ -s trace.s ]
then
    TRACE=trace.s
else
    echo ""
    echo "no trace files available!"
    echo ""
    exit 1;
fi

grep PSNR $TRACE | awk '{print $5}' > psnr.tr
awk 'begin {i = 0;} { print i++" "$1}' psnr.tr > psnr.xg


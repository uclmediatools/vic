#!/bin/sh
# deleting files

RM='rm -f'
LS='ls'

for i in $( $LS $PWD/*.xg $PWD/*.eps $PWD/trace.r $PWD/trace.s); do
	echo "deleting: $i"
	$RM $i
done 2> /dev/null

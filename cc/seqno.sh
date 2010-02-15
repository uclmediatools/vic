#!/bin/sh

gnuplot -persist << EOF

set style line 1 lt 1 pt 4 lw 1.5
set style line 2 lt 2 pt 4 lw 1.5
set style line 3 lt 3 pt 4 lw 1.5
set style line 4 lt 4 pt 4 lw 1.5
set style line 5 lt 5 pt 4 lw 1.5

set mxtics 2
set mytics 2
set grid

set xrange [$1:$2]
set yrange [$3:$4]

plot \
	"encs.xg" with impulses, \
	"ence.xg" with impulses, \
	"inXR.xg" with impulses, \
	"seqno.xg" ls 3

EOF

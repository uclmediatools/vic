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

gnuplot -persist << EOF
set terminal postscript eps enhanced color
set output "seqno.eps"

set title "time-seqno plot"
set xlabel "time (sec)"
set ylabel "seqno"

set mxtics 2
set mytics 2

set xrange [$1:$2]
set yrange [$3:$4]

plot \
"encs.xg" with impulses title "start encoding", \
"ence.xg" with impulses title "end encoding" lc 4 lt 1, \
"inXR.xg" with impulses title "ackvec arrival" lt 1 lc 2, \
"seqno.xg" pt 4 lc 3 title "packet"

EOF

gnuplot -persist << EOF
set terminal postscript eps enhanced color
set output "enct.eps"

set title "encoding time"
set xlabel "num encoding"
set ylabel "time (sec)"

set yrange [0:0.005]
set mxtics 2
set mytics 2

plot \
"enct.xg" with lines title "encoding time", \
"grt.xg" with lines lt 1 lc 3 title "grabbing time"

EOF

gnuplot -persist << EOF
set terminal postscript eps enhanced color
set output "cwnd.eps"

set title "cwnd"
set xlabel "vic run time (sec)"
set ylabel "cwnd"

set mxtics 5
set mytics 2
set xrange [$1:$2]
set grid xtics ytics mytics

plot "cwnd.xg" w lp lc 3 pt 4 ps .4 title "cwnd"
EOF

gnuplot -persist << EOF
set terminal postscript eps enhanced color
set output "ALI.eps"

set title "average loss interval"
set xlabel "vic run time (sec)"
set ylabel "ALI"

set mxtics 5
set mytics 2
set xrange [$1:$2]
set grid xtics ytics mytics

plot "ALI.xg" w lp lc 1 pt 5 ps .4 title "ALI"
EOF

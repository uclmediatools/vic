# $Id$

grep rtt trace.s | awk '{print $3" "$7*1000" "$9*1000}' > rtt.xg

gnuplot -persist << EOF
	set terminal postscript eps enhanced color
	set output "rtt.eps"

	set title "sampled RTT and smoothed RTT"
	set xlabel "Vic run time (sec)"
	set ylabel "rtt (msec)"
	set mxtics 5
	set mytics 2
	set xrange [$1:$2]
	set yrange [$3:$4]
	set grid xtics ytics mxtics mytics

	set label 1 at 5,150
	#set label 1 "real network RTT is set to 100 ms" tc lt 1 \
	#	font "Helvetica,24"
	#set label 1 "\"ping\" showed 1ms of rtt roughly" tc lt 1 \
	#	font "Helvetica,24"

	plot "rtt.xg" using 1:2 with l lt 1 lc 18 title "sampled rtt", \
	     "rtt.xg" using 1:3 with l lt 1 lc 3 lw 2.5 title "smoothed rtt"
	
EOF

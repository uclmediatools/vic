#
# Copyright (c) 1993-1995 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the names of the copyright holders nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

proc get-playout src {
	set d [$src handler]
	if { "$d" != "" } {
		#XXX assume 8Khz */
		return [expr [$d playout] >> 3]
	}
	return 0
}

proc create_stat_row { r name width cmd relief } {
	set f [smallfont]
	button $r.name -text $name -font $f -anchor w -width $width \
		-command $cmd -pady 2 -padx 2 -borderwidth 2 \
		-highlightthickness 0 -relief raised
	label $r.smooth -font $f -anchor e -width 8 \
		-relief $relief -borderwidth 1 -pady 1
	label $r.diff -font $f -anchor e -width 8 \
		-relief $relief -borderwidth 1 -pady 1
	label $r.total -font $f -anchor e -width 8 \
		-relief ridge -borderwidth 1 -pady 1

	pack $r.name -anchor w -fill x -side left -pady 1 -padx 4
	pack $r.smooth $r.diff $r.total \
		-expand 1 -fill both -anchor e -side left
}

proc create_stats_panel { w stats } {
	set f [smallfont]
	set p $w.f
	frame $p
	set top [winfo toplevel $w]
	set gain [resource statsFilter]
	global rv_diff rv_smooth rv_list win_src

	set r $p.legend
	frame $r
	label $r.smooth -font $f -anchor c -width 8 -text EWA \
		-relief ridge -borderwidth 1
	label $r.diff -font $f -anchor c -width 8 -text Delta \
		-relief ridge -borderwidth 1
	label $r.total -font $f -anchor c -width 8 -text Total \
		-relief ridge -borderwidth 1
	pack $r.total $r.diff $r.smooth -side right
	pack $r -anchor e

	#
	# save list of stats because they might change and we want to
	# remember the rate variables that we have created
	#
	set rv_list($top) $stats

	set n [llength $stats]

	set width 10
	set i 0
	while { $i < $n } {
		set v [string len [lindex $stats $i]]
		if { $v > $width } {
			set width $v
		}
		incr i 2
	}

	set src $win_src($top)
	set i 0
	while { $i < $n } {
		set name [lindex $stats $i]
		incr i
		set value [lindex $stats $i]
		incr i
		set id [string tolower $name]
		set r $p.$id
		frame $r 

		set gen "stat_generator \{$src stats\} $name"
		set cmd "create_plot_window $src $name \{$gen\}"
		create_stat_row $r $name $width $cmd ridge
		pack $r -pady 0

		set rv_diff($top:$id) $value
		set rv_smooth($top:$id) $value

		rate_variable rv_diff($top:$id) 1.0 "%.1f"
		rate_variable rv_smooth($top:$id) $gain "%.1f"
	}
	#
	# Special-case playout estimator since it's not a counter
	#
	if { $src != "session" && $src != "pixrate" } {
		set r $p.playout
		frame $r
		set cmd "create_plot_window $src Playout \{get-playout $src\}"
		create_stat_row $r Playout $width $cmd flat
		pack $r -pady 0
	}

	global stat_window maxStat
	set stat_window($top) $p
	catch "unset maxStat($top)"

	pack $w.f -anchor c
}

proc stats_changed { s1 s2 } {
	set n [llength $s1]
	if { $n != [llength $s2] } {
		return 1
	}
	set i 0
	while { $i < $n } {
		if { [lindex $s1 $i] != [lindex $s2 $i] } {
			return 1
		}
		incr i 2
	}
	return 0
}

proc stat_update w {
	global stat_window rv_diff rv_smooth rv_list \
		stat_method win_src

	set stats [eval $stat_method($w)]
	if [stats_changed $stats $rv_list($w)] {
		unset_rvs $w
		pack forget $w.frame
		destroy $w.frame
		frame $w.frame -borderwidth 2 -relief groove
		create_stats_panel $w.frame $stats
		pack $w.frame -after $w.title -expand 1 -fill x -anchor center
	}
	
	set p $stat_window($w)
	set i 0
	set n [llength $stats]
	while { $i < $n } {
		set id [string tolower [lindex $stats $i]]
		incr i
		set cntr [lindex $stats $i]
		incr i
		set rv_diff($w:$id) $cntr
		set rv_smooth($w:$id) $cntr
		$p.$id.total configure -text $cntr
		$p.$id.diff configure -text $rv_diff($w:$id)
		$p.$id.smooth configure -text $rv_smooth($w:$id)
	}
	if [winfo exists $p.playout.total] {
		$p.playout.total configure -text [get-playout $win_src($w)]ms
	}
}

proc info_update { w src } {
	set decoder [$src handler]
	set fmt [rtp_format $src]
	if { $fmt == "" } { set fmt "?" }
	$w.title.info configure -text [info_text $src]
	set t [$src lastdata]
	if { $t == "" } { set t "never" }
	$w.title.timeData configure -text "last data $t"
	set t [$src lastctrl]
	if { $t == "" } { set t "never" }
	$w.title.timeCtrl configure -text "last control $t"

	foreach sdes [resource sdesList] {
		$w.title.$sdes configure -text "$sdes: [$src sdes $sdes]"
	}
	$w.title.srcid configure -text "srcid: [$src srcid]/[$src addr]"
	if { [$src srcid] != [$src ssrc] } {
		if ![winfo exists $w.title.mixer] {
			label $w.title.mixer -borderwidth 0 \
				-font [smallfont] -anchor w
			pack $w.title.mixer -after $w.title.srcid -fill x
		}
		$w.title.mixer configure -text "mixer: [$src ssrc]/[$src addr]"
	} elseif [winfo exists $w.title.mixer] {
		pack forget $w.title.mixer
		destroy $w.title.mixer
	}
	set note [$src sdes note]
	if { $note != "" } {
		set bg [resource infoHighlightColor]
	} else {
		set bg [$w.title.srcid cget -bg]
	}
	$w.title.note configure -background $bg
}

#
# Unset all the rate variables associated with a window,
# so that the C storage is freed up
#
proc unset_rvs w {
	global rv_list rv_diff rv_smooth
	if [info exists rv_list($w)] {
		set n [llength $rv_list($w)]
		for { set i 0 } { $i < $n } { incr i 2 } {
			set id [string tolower [lindex $rv_list($w) $i]]
			unset rv_diff($w:$id) rv_smooth($w:$id)
		}
		unset rv_list($w)
	}
}

proc stat_destroy w {
	unset_rvs $w
	destroy $w
	global stat_method win_src
	unset stat_method($w) win_src($w)
}

proc info_destroy { w src } {
	global info_x info_y
	set info_x($src) [winfo rootx $w]
	set info_y($src) [winfo rooty $w]
	destroy $w
}

proc set_numDecoderLayers { src value } {
	global numDecoderLayers V decoderLayerValue

	$decoderLayerValue($src) configure -text $value
	
	#$V(encoder) loop_layer [expr {$numEncoderLayers + 1}]
	set d [$src handler]
	if { "$d" != "" } {
		$d maxChannel $numDecoderLayers($src)
		update_source_info $src
	}
}


proc create_decoder_control_window src {
	if { "[$src handler]" != "" } {
		set w .decoder_control$src

		if [winfo exists $w] {
			destroy $w
			return
		}
		create_toplevel $w [getid $src]

		set f [smallfont]

		frame $w.title -borderwidth 2 -relief groove
		label $w.title.main -borderwidth 0 -anchor w -text "Decoder Control"
		label $w.title.name -borderwidth 0 -anchor w \
			-textvariable src_nickname($src)

		pack $w.title.name -anchor w
		pack $w.title.main -anchor w
		pack $w.title -fill x

		wm geometry $w +[winfo pointerx .]+[winfo pointery .]
		wm deiconify $w

		set fmt [rtp_format $src]

		if { $fmt == "pvh" } {

			global numDecoderLayers numLayers decoderLayerValue

			frame $w.tb -borderwidth 2 -relief groove

			if ![info exists numDecoderLayers($src)] {
				set numDecoderLayers($src) $numLayers
			}

			label $w.tb.value -text 0 -font $f -width 3
			scale $w.tb.scale -font $f -orient horizontal \
				-showvalue 0 -from 0 -to $numLayers \
				-variable numDecoderLayers($src) \
				-width 12 -relief groove \
				-command "set_numDecoderLayers $src"


			set decoderLayerValue($src) $w.tb.value

			$decoderLayerValue($src) configure -text $numDecoderLayers($src)

			pack $w.tb.scale -side left -fill x -expand 1
			pack $w.tb.value -side left
			pack $w.tb -fill x -padx 6 -side left -expand 1
		}

		button $w.dismiss -relief raised -font $f \
			-command "destroy $w" -text Dismiss

		# added to catch window close action
		wm protocol $w WM_DELETE_WINDOW "destroy $w"

		pack $w.dismiss -anchor c -pady 4

	} else {
		open_dialog "Decoder not initialised yet"
	}
}

proc create_stats_window { w src titleText method } {

	if [winfo exists $w] {
		stat_destroy $w
		return
	}

	create_toplevel $w [getid $src]

	set f [smallfont]

	frame $w.title -borderwidth 2 -relief groove
	label $w.title.main -borderwidth 0 -anchor w -text $titleText
	label $w.title.name -borderwidth 0 -anchor w \
		-textvariable src_nickname($src)
	frame $w.frame -borderwidth 2 -relief groove

	global win_src stat_method
	set stat_method($w) $method
	set win_src($w) $src
	create_stats_panel $w.frame [eval $method]

	pack $w.title.name -anchor w
	pack $w.title.main -anchor w
	pack $w.title -fill x
	pack $w.frame -expand 1 -fill x -anchor center

	wm geometry $w +[winfo pointerx .]+[winfo pointery .]
	wm deiconify $w
	# start up the timer
	window_timer $w stat_update

	button $w.dismiss -relief raised -font $f \
		-command "stat_destroy $w" -text Dismiss

    # added to catch window close action
	wm protocol $w WM_DELETE_WINDOW "stat_destroy $w"

	pack $w.dismiss -anchor c -pady 4
}

proc create_rtp_window src {
	create_stats_window .rtp$src $src "RTP Statistics" "$src stats"
}

proc create_decoder_window src {
	if { "[$src handler]" != "" } {
		create_stats_window .decoder$src $src  \
			"Decoder Statistics" "\[$src handler\] stats"
	} else {
		open_dialog "no decoder stats yet"
	}
}


#
# delete any windows bound to this source's decoder
#
proc destroy_decoder_stats src {
	if [winfo exists .decoder$src] {
		stat_destroy .decoder$src
	}
}

#
# delete any windows bound to this source
# assumes destroy_decoder_stats has already taken
# care of decoder windows
#
proc destroy_rtp_stats src {
	if [winfo exists .rtp$src] {
		stat_destroy .rtp$src
	}
	if [winfo exists .info$src] {
		info_destroy .info$src $src
	}
	global rv_plot win_src
	foreach w [array names rv_plot] {
		if { "$win_src($w)" == "$src" } {
			plot_destroy $w
		}
	}
}

proc create_global_window {} {

	set w .global
	if [winfo exists $w] {
		stat_destroy $w
		return
	}

	create_toplevel $w "Session Stats"

	set f [smallfont]

	frame $w.title -borderwidth 2 -relief groove
	label $w.title.main -borderwidth 0 -anchor w \
		-text "Global Session Statistics"
	frame $w.frame -borderwidth 2 -relief groove

	global stat_method win_src
	set stat_method($w) "session stats"
	# hack
	set win_src($w) session
	create_stats_panel $w.frame [session stats]

	pack $w.title.main -anchor w
	pack $w.title -fill x
	pack $w.frame -expand 1 -fill x -anchor center

	wm geometry $w +[winfo pointerx .]+[winfo pointery .]
	wm deiconify $w
	# start up the timer
	window_timer $w stat_update

	button $w.dismiss -relief raised -font $f \
		-command "stat_destroy $w" -text Dismiss
	pack $w.dismiss -anchor c -pady 4
}

proc window_timer { w action } {
	#XXX window can be deleted and recreated before old time dies
	#XXX which gives us two active timers (I think)
	if ![winfo exists $w] {
		return
	}
	$action $w
	after 1000 "window_timer $w $action"
}

proc has_src w {
	global win_src
	if [string compare $win_src($w) GLOBAL] {
		return 1
	} else {
		return 0
	}
}

proc stat_generator { method id } {
	set stats [eval $method]
	set k [lsearch -exact $stats $id]
	return [lindex $stats [expr $k + 1]]
}

proc plot_get w {
	global plot_generator
	set v [eval $plot_generator($w)]
	return $v
}

proc plot_update w {
	global rv_plot
	set rv_plot($w) [plot_get $w]
	$w.frame.sc set $rv_plot($w)
}

proc plot_destroy w {
	global win_src rv_plot plot_generator
	unset win_src($w) plot_generator($w) rv_plot($w)
	destroy $w
}

proc relabel_stripchart {w min max perDiv} {
	$w configure -text " range $min to $max,  $perDiv/div"
}

proc create_plot_window { src name generator } {

	global win_src plot_generator
	set id [string tolower $name]
	set w .plot$src$id
	#XXX
	if [winfo exists $w] {
		plot_destroy $w
		return
	}
	set win_src($w) $src
	set plot_generator($w) $generator
	create_toplevel $w "plot window"
	catch "wm resizable $w true false"

	set f [smallfont]

	frame $w.title -borderwidth 2 -relief groove
	label $w.title.main -borderwidth 0 -anchor w -text $name
	frame $w.frame -borderwidth 2 -relief groove 

	stripchart $w.frame.sc -max 200 -min 1 -stripwidth 1 -width 1 \
		-autoscale 2 -rescale_command "relabel_stripchart $w.bf.lab" \
		-relief groove -striprelief flat -tickcolor gray95 -hticks 30
	pack $w.frame.sc -expand 1 -fill both

	# force more reasonable initial size
	frame $w.brace -width 250
	pack $w.brace

	if [has_src $w] {
		label $w.title.name -borderwidth 0 -anchor w \
			-textvariable src_nickname($src)
		pack $w.title.name -anchor w
	}
	pack $w.title.main -anchor w
	pack $w.title -fill x
	pack $w.frame -expand 1 -fill both -anchor center

	#
	# create a new rate-variable.  would be nice to just use	
	# rv_diff but the stat window that this plot comes from can
	# be deleted while leaving this one in place.
	# XXX hack: don't use a rate-variable for the playout estimator
	# since we want actual value not differences
	if { "$id" != "playout" } {
		rate_variable rv_plot($w) 1.0 "%.1f"
	}

	wm geometry $w +[winfo pointerx .]+[winfo pointery .]
	wm deiconify $w
	# start up the timer
	window_timer $w plot_update

	frame $w.bf
	label $w.bf.lab -borderwidth 0 -font $f -anchor w -text "No data"
	pack $w.bf.lab -side left -expand 1 -fill x
	button $w.bf.dismiss -relief raised -font $f -anchor e \
		-command "plot_destroy $w" -text Dismiss
	pack $w.bf.dismiss -side right -pady 4 -padx 4
	pack $w.bf -expand 1 -fill x
}


#
# create a top-level window with summary statistics
# for the indicated source
#
proc create_info_window src {
	set w .info$src
	if [winfo exists $w] {
		info_destroy $w $src
		return
	}
	create_toplevel $w [getid $src]
	set f [smallfont]
	frame $w.title -borderwidth 2 -relief groove
	label $w.title.name -borderwidth 0 -font $f -anchor w \
		-textvariable src_nickname($src)
	label $w.title.info -borderwidth 0 -font $f -anchor w \
		-text [$src addr]
	label $w.title.timeData -borderwidth 0 -font $f -anchor w
	label $w.title.timeCtrl -borderwidth 0 -font $f -anchor w

	frame $w.frame -borderwidth 2 -relief groove

	pack $w.title.name $w.title.info -fill x

        foreach sdes [resource sdesList] {
		label $w.title.$sdes -borderwidth 0 -font $f -anchor w
		pack $w.title.$sdes -fill x
	}
	label $w.title.srcid -borderwidth 0 -font $f -anchor w
	pack $w.title.srcid -fill x

	pack $w.title.timeData $w.title.timeCtrl -fill x

	pack $w.title -fill x

	set p $w.bot
	frame $p

	set m $p.mb.menu
	menubutton $p.mb -text Stats -menu $m -relief raised -width 8 \
		-font $f -indicatoron 1
	menu $m
	$m add command -label RTP -command "create_rtp_window $src" -font $f
	$m add command -label Decoder \
		-command "create_decoder_window $src" -font $f

	button $p.dismiss -relief raised -font $f \
		-command "info_destroy $w $src" -text Dismiss

	pack $p.mb -side left -padx 8
	pack $p.dismiss -side right -padx 8
	pack $p -anchor c -pady 4 -fill x

	info_update $w $src

	global info_x info_y
	if [info exists info_x($src) ] {
		set x $info_x($src)
		set y $info_y($src)
	} else {
		set x [winfo pointerx .]
		set y [winfo pointery .]
	}

	#
	# Need to do an update so that $w gets laid out allowing us to
	# look up its size.  This is tricky because of a possible race:
	# if the user releases the mouse, summary_window might get
	# destroyed during the update idletasks.  So we check
	# that the window still exists before proceeeding.
	#
	update idletasks
	if ![winfo exists $w] { return }

	#
	# Check if window goes off the bottom or right.  Don't need
	# to check top or left since upper-left corner is at mouse.
	#
	set right [expr [winfo screenwidth .] - [winfo reqwidth $w] - 5]
	if { $x > $right } {
		set x $right
	}
	set bot [expr [winfo screenheight .] - [winfo reqheight $w] - 5]
	if { $y > $bot } {
		set y $bot
	}
	wm geometry $w +$x+$y
	wm deiconify $w
	after 3000 "info_timer $w $src"
}

proc info_timer { w src } {
	if ![winfo exists $w] {
		return
	}
	info_update $w $src
	after 3000 "info_timer $w $src"
}

proc create_mtrace_window {src dir} {
	set w .mtrace$src
	if ![winfo exists $w] {
		create_toplevel $w "[getid $src] mtrace"
		set f [smallfont]

		frame $w.t
		scrollbar $w.t.yscroll -command "$w.t.text yview" -relief sunken
		scrollbar $w.t.xscroll -command "$w.t.text xview" -relief sunken \
			-orient horiz
		text $w.t.text -height 24 -width 80 -setgrid true -wrap none \
			-font fixed -relief sunken -borderwidth 2 \
			-xscrollcommand "$w.t.xscroll set" \
			-yscrollcommand "$w.t.yscroll set"
		pack $w.t.yscroll -side right -fill y
		pack $w.t.xscroll -side bottom -fill x
		pack $w.t.text -side left -padx 0 -pady 0 -fill both -expand yes

		set p $w.b
		frame $p
		button $p.dismiss -relief raised -font $f \
			-command "destroy $w" -text Dismiss
		pack $p.dismiss -side right -padx 8

		pack $w.t -side top -fill both -expand yes
		pack $p -side bottom -pady 2 -fill x

		wm geometry $w +[winfo pointerx .]+[winfo pointery .]
		wm deiconify $w
		update idletasks
		if ![winfo exists $w] { return }
	} else {
		$w.t.text yview end
	}

	global V
	if [file executable /usr/sbin/mtrace] {
		set mtrace_prog /usr/sbin/mtrace
	} else {
		set mtrace_prog mtrace
	}
	if {$dir=="to"} {
		set cmd "|$mtrace_prog [$V(data-net) interface] [$src addr] [$V(data-net) addr]"
	} else {
		set cmd "|$mtrace_prog [$src addr] [$V(data-net) addr]"
	}
	if [catch "open {$cmd} r" fd] {
		$w.t.text insert end "mtrace error: $fd"
		return
	}
	fconfigure $fd -blocking 0
	fileevent $fd readable "read_mtrace $fd $w"
}

proc read_mtrace {fd w} {
	if [winfo exists $w] {
		$w.t.text insert end [read $fd]
		$w.t.text yview end
		if [eof $fd] {
			fileevent $fd readable {}
			catch "close $fd"
		}
	} else {
		fileevent $fd readable {}
		catch "close $fd"
	}
}

proc build_info_menu {src m} {
	global V
	menu $m
	set f [smallfont]
	$m add command -label "Site Info" \
		-command "create_info_window $src" -font $f
	$m add command -label "RTP Stats"\
		-command "create_rtp_window $src" -font $f
	$m add command -label "Decoder Stats" \
		-command "create_decoder_window $src" -font $f
	$m add command -label "Decoder Control" \
		-command "create_decoder_control_window $src" -font $f

	if [$V(data-net) ismulticast] {
		$m add command -label "Mtrace from" \
			-command "create_mtrace_window $src from" -font $f
		$m add command -label "Mtrace to" \
			-command "create_mtrace_window $src to" -font $f
	}
}

proc create_info_menu {src x y} {
	set m .menu$src
	if ![winfo exists $m] {
		build_info_menu $src $m
	}
	tk_popup $m $x $y
}

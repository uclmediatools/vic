proc yesno attr {
	set v [option get . $attr Vdd]
	if { [string match \[0-9\]* $v] } {
		return $v
	}
	if { $v == "true" || $v == "True" || $v == "t" } {
		return 1
	} elseif { $v == "false" || $v == "False" || $v == "f" } {
		return 0
	}
	return 0
}

#
# Look through the list of available visuals, and choose
# the best one we support.
#
proc lookup_visual {} {
	set vlist [winfo visualsavailable .]
	if { [lsearch -exact $vlist "truecolor 24"] >= 0 || \
		 [lsearch -exact $vlist "truecolor 32"] >= 0 } {
		set visual "truecolor 24"
	} elseif { [lsearch -exact $vlist "pseudocolor 8"] >= 0 } {
		set visual "pseudocolor 8"
	} elseif { [lsearch -exact $vlist "staticgray 1"] >= 0 } {
		set visual "staticgray 1"
	} else {
		puts stderr "h261_play: no support for your display type"
		exit 1
	}
}

#
# initialize the visual, colormap, and dither global variables
# based on  X resources, available visuals, etc.  Create the
# specified window as a frame widget and set its visual.  We pick
# the visual only once and startup -- the user cannot change it
# interactively.  The user can change the dithering algorithms
# interactively constrained to the selected visual
# Sets the colormap global variable to $w since this window
# will always have the reference colormap.
#
proc init_visual w {
	global V

	set V(colormap) $w
	set dither [option get . dither $V(class)]
	if { $dither == "" } {
		set dither ed
	}
	if { $dither == "best" } {
		set dither ed
	}
	if { $dither == "dither" } {
		set dither od
	}
	if { [lsearch -exact "od ed gray quantize" $dither] < 0 } {
		puts stderr "$V(app): unknown dither: $dither"
		exit 1
	}
	set V(gamma) [option get . gamma $V(class)]
	set visual [option get . visual $V(class)]
	if { $visual == "" } {
		set visual [lookup_visual]
	} elseif { $visual == "pseudocolor" } {
		set visual "pseudocolor 8"
	}
	set cmap ""
	if [yesno privateColormap] {
		set cmap "-colormap new"
	}
	if [catch "frame $w -visual $visual $cmap"] {
		puts stderr "$V(app): bad visual: $visual"
	}
	if { [winfo depth $w] == 8 } {
		set V(dither) $dither
	}
	set V(visual) $visual
}

#
# initialize the colormodel based on the visual, dither, and colormap
# global variables.  if we run out of colors, free the colormodel
# and return false (leaving it up to the caller to fix things)
# otherwise, return true.  Set the global variable colorModel
# to the corresponding tcl object.
#
proc init_color {} {
	global V
	if [info exists V(colorModel)] {
		delete $V(colorModel)
		unset V(colorModel)
	}
	set v [winfo visual $V(colormap)]
	set d [winfo depth $V(colormap)]
	if { $d == 8 } {
		set id $v/$d/$V(dither)
	} else {
		set id $v/$d
	}
	set cm [new colormodel $id]
	if { $cm == "" } {
		puts stderr "$V(app): unsupported visual type: $v"
		exit 1
	}
	$cm visual $V(colormap)
	$cm gamma $V(gamma)
	#XXX revert to gray?
	if ![$cm alloc-colors] {
		delete $cm
		return 0
	}
	set V(colorModel) $cm
	return 1
}

#
# print out gob and macroblock number (starting from 0 not 1)
# XXX assumes CIF
#
proc putinfo { x y } {
	set h [expr int($x / 176)]
	set v [expr int($y / 48)]
	set gob [expr $h + 2 * $v]

	set x [expr $x - $h * 176]
	set y [expr $y - $v * 48]
	set h [expr int($x / 16)]
	set v [expr int($y / 16)]
	set mba [expr $h + 11 * $v]
	puts stderr "g $gob m $mba"
}

proc build { width height } {
	init_visual .top
	if ![init_color] {
		global V
		if { [winfo depth .top] != 8 } {
			puts stderr "$V(app): internal error: no colors"
			exit 2
		}
		puts stderr \
		    "$V(app): warning: ran out of colors; using private colormap"
		destroy .top
		global V
		frame .top -visual $V(visual) -colormap new
		if ![init_color] {
			puts stderr "$V(app): internal error: no colors"
			exit 2
		}
	}

	set w .top.video
	video $w $width $height
	pack $w
	pack .top

	bind $w <Enter> { focus %W }
	bind $w <Return> { next_frame }
	bind $w <1> { putinfo %x %y }
	bind $w <space> run
	bind $w <q> exit
	bind $w <d> { decoder dump t.ppm }
	bind $w <f> { puts [decoder frame] }

	update 
	global V showChanges
	set r [$V(colorModel) renderer $w 420]
	$r color 1
	if {$showChanges} {
		$r irthresh 2000
	}
	decoder renderer $r
}
proc next_frame {} {
	global showChanges
	if {$showChanges} {
		.top.video dim
		update idletasks
	}
	decoder next
}
proc run { } {
	global running
	if { !$running } {
		puts "running"
		set running 1
		after 0 tick
	} else {
		set running 0
		puts "paused"
	}
}
proc tick { } {
	next_frame
	update idletasks
	global running
	if { $running } {
		after 0 tick
	}
}

proc init_local {} {
	set script [option get . startupScript H261_play]
	if { $script != "" } {
		source $script
	}
	if [file exists ~/.h261_play.tcl] {
		uplevel #0 "source ~/.h261_play.tcl"
	}
}

proc h261_init_resources {} {
	option add H261_play.dither od startupFile
	option add H261_play.gamma 0.7 startupFile
	option add H261_play.medianCutColors 150 startupFile
	option add H261_play.privateColormap no startupFile
	option add H261_play.pause true startupFile
	option add H261_play.showChanges false startupFile
}

proc h261_main { w h } {
	global running showChanges
	set showChanges [yesno showChanges]
	init_local
	build $w $h
	set running [expr ![yesno pause]]
	if {$running} {
		after 100 tick
	} else {
		puts "paused"
	}
}

set V(class) H261_play
set V(app) h261_play
set V(media) video

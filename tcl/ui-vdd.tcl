
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
# the best one that vic supports.
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
		puts stderr "vic: no support for your display type"
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
	global vicPriv

	set vicPriv(colormap) $w
	set dither [option get . dither Vic]
	set vicPriv(gamma) [option get . gamma Vic]
	if { $dither == "dither" } {
		set dither od
	}
	if { [lsearch -exact "od ed gray quantize" $dither] < 0 } {
		puts stderr "vic: unknown dither: $dither"
		exit 1
	}
	set visual [option get . visual Vic]
	if { $visual == "" } {
		set visual [lookup_visual]
	} elseif { $visual == "pseudocolor" } {
		set visual "pseudocolor 8"
	}
	if [yesno privateColormap] {
		frame $w -visual $visual -colormap new
	} else {
		frame $w -visual $visual
	}
	if { [winfo depth $w] == 8 } {
		set vicPriv(dither) $dither
	}
	set vicPriv(visual) $visual
}

#
# initialize the colormodel based on the visual, dither, and colormap
# global variables.  if we run out of colors, free the colormodel
# and return false (leaving it up to the caller to fix things)
# otherwise, return true.  Set the global variable colorModel
# to the corresponding tcl object.
#
proc init_color {} {
	global vicPriv
	if [info exists vicPriv(colorModel)] {
		delete $vicPriv(colorModel)
		unset vicPriv(colorModel)
	}
	set v [winfo visual $vicPriv(colormap)]
	set d [winfo depth $vicPriv(colormap)]
	if { $d == 8 } {
		set id $v/$d/$vicPriv(dither)
	} else {
		set id $v/$d
	}
	set cm [new colormodel $id]
	if { $cm == "" } {
		puts stderr "vic: unsupported visual type: $v"
		exit 1
	}
	$cm visual $vicPriv(colormap)
	$cm gamma $vicPriv(gamma)
	#XXX revert to gray?
	if ![$cm alloc-colors] {
		delete $cm
		return 0
	}
	set vicPriv(colorModel) $cm
	return 1
}

#
# print out gob and macroblock number (starting from 0 not 1)
# XXX assumes CIF
#
proc putinfo { x y } {
	global width height

	if {[decoder format] == "cif"} {
		set x [expr $x * 352 / $width]
		set y [expr $y * 288 / $height]

		set h [expr int($x / 176)]
		set v [expr int($y / 48)]
		set gob [expr $h + 2 * $v]

		set x [expr $x - $h * 176]
		set y [expr $y - $v * 48]
		set h [expr int($x / 16)]
		set v [expr int($y / 16)]
		set mba [expr $h + 11 * $v]
		set blk 0
	} else {
		set x [expr $x * 176 / $width]
		set y [expr $y * 144 / $height]

		set h [expr int($x)]
		set v [expr int($y)]

		set gv [expr int($v / 48)]
		set gob [expr $gv]
		set v [expr $v - $gv*48]

		set mv [expr int($v / 16)]
		set mh [expr int($h / 16)]
		set mba [expr $mv*11 + $mh]

		set bh [expr int(($h - $mh*16) / 8)]
		set bv [expr int(($v - $mv*16) / 8)]
		set blk [expr $bv * 2 + $bh + 1]
	}
	puts stderr "p [decoder frame]  g $gob  m $mba  b $blk"
}

proc build { w h } {
	init_visual .top
	if ![init_color] {
		if { [winfo depth .top] != 8 } {
			puts stderr "vic: internal error: no colors"
			exit 2
		}
		puts stderr \
		    "vic: warning: ran out of colors; using private colormap"
		destroy .top
		global vicPriv
		frame .top -visual $vicPriv(visual) -colormap new
		if ![init_color] {
			puts stderr "vic: internal error: no colors"
			exit 2
		}
	}

	global width height
	set width $w
	set height $h
	set w .top.video
	video $w $width $height
	pack $w
	pack .top
update

	bind $w <Enter> { focus %W }
	bind $w <Return> { decoder next }
	bind $w <p> { decoder nextpacket }
	bind $w <1> { putinfo %x %y }
	bind $w <space> run
	bind $w <q> exit
	bind $w <d> { decoder dump t.ppm }
	bind $w <f> { puts [decoder frame] }

	update 

	#XXX
	set decimation 420
	global vicPriv
	set target [$vicPriv(colorModel) renderer $w $decimation]
	decoder target $target
}
set running 0
proc run { } {
	global running
	if { !$running } {
		set running 1
		after 0 tick
	} else {
		# race
		set running 0
	}
}
proc tick { } {
	decoder next
	update idletasks
	global running
	if { $running } {
		after 0 tick
	}
}

proc init_local {} {
	set script [option get . startupScript Vdd]
	if { $script != "" } {
		source $script
	}
	if [file exists ~/.vdd.tcl] {
		uplevel #0 "source ~/.vdd.tcl"
	}
}

proc vdd_init_resources {} {
	option add Vdd.dither od startupFile
}

proc vdd_main { w h } {
	init_local
	build $w $h
}

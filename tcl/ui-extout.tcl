#
# Copyright (c) 1995 The Regents of the University of California.
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

proc extout_build_thumbnail { w d } {
	set stamp $w.stamp
	frame $stamp -relief groove -borderwidth 2
	create_video_widget $stamp.video 80 60
	global win_is_slow win_use_hw extout_tn
	set win_is_slow($stamp.video) 1
	set extout_is_slow($d) 0
	set extout_tn($d) $stamp.video
	pack $stamp.video -anchor c -padx 2
	pack $stamp -fill y

	frame $w.b -background purple
	button $w.b.rev -bitmap rev -command "switcher_prev $d" 
	button $w.b.fwd -bitmap fwd -command "switcher_next $d"

	pack $w.b.rev $w.b.fwd -side left -fill x -expand 1 -padx 0
	pack $w.b -fill x

}

proc extout_build_modes { w d } {
	set f [smallfont]
	checkbutton $w.ts -text "Timer-Switched" -font $f -anchor w \
		-variable extout_is_timed($d) \
		-command "extout_set_timed $d"
	checkbutton $w.vs -text "Voice-Switched" -font $f -anchor w \
		-variable extout_is_switched($d) \
		-command "extout_set_switched $d"
	if ![have cb] {
		$w.vs configure -state disabled
	}
	#XXX
	checkbutton $w.sc -text "Save-CPU" -font $f -anchor w \
		-variable extout_is_slow($d) \
		-command "extout_set_slow $d"

	pack $w.vs $w.ts $w.sc -anchor w -fill x
}

proc extout_set_scale d {
	global extout_target extout_scale
	if [info exists extout_target($d)] {
		$extout_target($d) scale $extout_scale($d)
	}
}

proc extout_build_buttons { w d } {
	set f [smallfont]

	set m $w.mb.menu
	menubutton $w.mb -text Port -menu $m \
		-relief raised -font $f -indicatoron 1
	#XXX no supported devices have multiple external ports yet
	$w.mb configure -state disabled

	checkbutton $w.ext -text "Scale-Up" -font $f \
		-relief raised -anchor w -highlightthickness 0 \
		-variable extout_scale($d) \
		-command "extout_set_scale $d"
	button $w.sd -text "Shutdown" -font $f \
		-command "extout_shutdown $d [winfo toplevel $w]" \
		-highlightthickness 0


	pack $w.mb $w.ext $w.sd -anchor w -fill x
}

proc extout_mb_post { d flag x y } {
	if { $flag != "" } {
		global extout_mb src_nickname
		set menu $extout_mb($d).menu
		if [winfo exists $menu] {
			destroy $menu
		}
		menu $menu
		foreach src [session active] {
			$menu add command -label $src_nickname($src) \
				-command "switcher_set $d $src"
		}
		tkMbPost $flag $x $y
	}
}

proc extout_select d {

	set w .$d
	#XXX use toggle_window
	if [winfo exists $w] {
		if [winfo ismapped $w] {
			wm withdraw $w
		} else {
			wm deiconify $w
		}
		return
	}
	create_toplevel $w "[$d nickname]"
	catch "wm resizable $w false false"

	set f [smallfont]

	frame $w.title -borderwidth 2 -relief ridge
	label $w.title.main -borderwidth 0 -anchor w \
		-text "External Output Controls ([$d nickname])"
	frame $w.frame -borderwidth 2 -relief ridge

	frame $w.frame.v
	frame $w.frame.v.tn
	extout_build_thumbnail $w.frame.v.tn $d

	frame $w.frame.v.modes -relief groove -borderwidth 2
	extout_build_modes $w.frame.v.modes $d

	frame $w.frame.v.buttons
	extout_build_buttons $w.frame.v.buttons $d

	#XXX
	pack $w.frame.v.tn $w.frame.v.modes $w.frame.v.buttons \
		-side left -padx 2

	set mb $w.frame.src
	menubutton $mb -text "<no stream>" -menu $mb.menu \
		-relief raised -width 20
	global extout_mb
	set extout_mb($d) $mb
	bind $mb <1> "extout_mb_post $d \$tkPriv(inMenubutton) %X %Y"

	pack $w.frame.v $w.frame.src -fill x -pady 2 -padx 2

	pack $w.title.main -anchor w
	pack $w.title -fill x
	pack $w.frame -expand 1 -fill both -anchor center

	wm geometry $w +[winfo pointerx .]+[winfo pointery .]
	wm deiconify $w

	button $w.dismiss -relief raised \
		-command "wm withdraw $w" -text Dismiss -font $f
	pack $w.dismiss -anchor c -pady 2

	switcher_register $d "" extout_switch
}

proc extout_switch { d src } {
	extout_release $d
	extout_bind $d $src
}

proc extout_shutdown { d w } {
	extout_release $d
	destroy $w
}

proc extout_release d {
	global extout_target extout_src extout_tn
	if [info exists extout_src($d)] {
		detach_window $extout_src($d) $extout_tn($d)
		extout_detach $d
		unset extout_src($d)
	}
}

proc extout_detach d {
	global extout_target extout_src
	set src $extout_src($d)
	[$src handler] detach $extout_target($d)
	delete $extout_target($d)
	unset extout_target($d)
}

proc extout_bind { d src } {
	extout_release $d
	extout_attach $d $src
	global extout_tn
	# win_is_slow
	attach_window $src $extout_tn($d)
}

proc extout_attach { d src } {
	global extout_target extout_src extout_mb extout_scale src_nickname
	set fmt [rtp_format $src]
	if { $fmt == "jpeg" } {
		set fmt $fmt/[[$src handler] decimation]
	}
	set target [$d assistor $fmt]
	if { $target == "" } {
		set target [$d renderer [[$src handler] decimation]]
	}
	if { $target != "" } {
		set extout_src($d) $src
		set extout_target($d) $target
		$target scale $extout_scale($d)
		[$src handler] attach $target
		$extout_mb($d) configure -text $src_nickname($src)
		extout_set_slow $d
		return $target
	}
	#XXX need placeholder
	return ""
}

#
# detach $src from any external output paths that
# are viewing it.  return list of decoders that
# were viewing $src.
#
proc extout_detach_src src {
	global extout_src
	if ![info exists extout_src] {
		return
	}
	set list ""
	foreach d [array names extout_src] {
		if { $extout_src($d) == $src } {
			extout_detach $d
			set list "$list $d"
		}
	}
	return $list
}

#
# attach $src to the list of external output paths in $list
#
proc extout_attach_src { src list } {
	foreach d $list {
		extout_attach $d $src
	}
}

proc extout_set_switched d {
	global extout_is_switched
	if { $extout_is_switched($d) != 0 } {
		switcher_enable $d
	} else {
		switcher_disable $d
	}
}

proc extout_set_timed d {
	global extout_is_timed
	if { $extout_is_timed($d) != 0 } {
		switcher_set_timer $d
	} else {
		switcher_cancel_timer $d
	}
}

proc extout_set_slow d {
	global extout_target extout_is_slow
	if [info exists extout_target($d)] {
		if $extout_is_slow($d) {
			set interval [option get . stampInterval Vic]
		} else {
			set interval 0
		}
		$extout_target($d) update-interval $interval
	}
}
